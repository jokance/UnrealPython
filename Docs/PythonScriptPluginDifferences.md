# UnrealPython 与 UE PythonScriptPlugin 源码差异

本文对比对象：

- 本插件：`Plugins/UnrealPython`
- UE 内置插件：`Engine/Plugins/Experimental/PythonScriptPlugin`，当前工作区路径为 `UnrealEngine/Engine/Plugins/Experimental/PythonScriptPlugin`

结论先行：`UnrealPython` 不是对 `PythonScriptPlugin` 的小幅改名，而是保留了部分 CPython 嵌入、GIL、UObject/UStruct/UEnum 包装、类型转换等思路后，面向运行时游戏脚本重新组织的一套实现。UE 内置插件主要服务编辑器自动化；本插件目标是在打包游戏中可用，并提供可静态生成、可裁剪、可补全的 Python 绑定。

## 1. 插件定位和加载阶段

UE 内置 `PythonScriptPlugin.uplugin` 的主模块是 `UncookedOnly`，并带有 `PythonScriptPluginPreload` 早期预加载模块。它的描述也是 `Python integration for the Unreal Editor`，默认不启用，主要面向 Editor/Program。

`UnrealPython.uplugin` 改为：

- `UnrealPython`：`Runtime` 模块，`LoadingPhase=Default`
- `UnrealPythonEditor`：单独的 `Editor` 模块
- 不再保留 `PythonScriptPluginPreload`

这样修改的原因是目标场景变了：本插件需要在游戏运行时初始化 Python，而不是只在编辑器或命令行工具中执行 Python 自动化任务。编辑器菜单、代码生成等功能拆到 `UnrealPythonEditor`，避免运行时模块直接依赖大量编辑器功能。

## 2. Python 运行时来源

UE 内置插件通过 `Python3` 引擎模块接入 UE 自带 Python。它还包含 `Content/Python` 下的 pip 安装辅助、远程执行脚本、测试脚本等编辑器配套资源。

本插件在 `Source/UnrealPython/UnrealPython.Build.cs` 中不依赖 `Python3` 模块，而是直接接入 `ThirdParty/python314`：

- Win64 链接 `python314.lib`，拷贝 `python314.dll`
- Android 链接 `libpython3.14.a` 以及 OpenSSL、bz2、ffi、lzma、zstd、mpdec 等静态库
- Mac 链接并拷贝 `libpython3.14.dylib` 和裁剪后的 `python3.14` 标准库
- iOS 链接 `Python.xcframework`，把 `Runtime/IOSDevice` 或 `Runtime/IOSSimulator` 下的 `python` 与 `Frameworks` 打进 app bundle

这样修改是为了避免和 UE 自带 Python 的 ABI、符号、`sys.path`、版本生命周期耦合。本插件当前目标是 Python 3.14，而 UE 自带插件随引擎版本绑定；如果混用，尤其在打包平台上很容易出现动态库、标准库路径或扩展模块加载冲突。`Docs/PythonRuntimeBuild.md` 已单独记录了 Python 3.14 运行时的构建与打包细节。

## 3. 初始化流程

UE 内置插件的初始化集中在 `FPythonScriptPlugin`：

- 支持 `-EnablePython`、`-DisablePython`、`-ForceEnablePython`
- 支持用户设置里的启用/禁用覆盖
- 支持 commandlet 禁用列表
- 注册 Python、PythonREPL 控制台执行器
- 支持启动脚本、远程执行、pip install、Content Browser 集成、在线文档生成等
- 初始化 `unreal` 模块，并维护编辑器脚本执行上下文

本插件把初始化拆成更小的运行时组件：

- `FUPyVirtualMachine`：负责 `Py_PreInitialize`、`Py_InitializeFromConfig`、`sys.path`、`sys.argv`、GIL 主线程保存/恢复
- `FUPyModuleInitializer`：负责创建 `ue` 模块、注册内置类型、注册自动生成类型
- `UUPyManager`：作为 UObject 单例管理生命周期、UObject 删除监听、输出重定向、`ue_site` 生命周期回调
- `FUnrealPythonModule`：模块入口，只负责启动/关闭 Manager；编辑器下再额外注册控制台执行器和 ToolMenus 命令处理器

这样修改的原因是运行时插件需要确定性更强的生命周期：游戏启动时初始化，关闭时释放；不需要编辑器的启用策略、pip 安装任务、远程执行服务和文档命令。精简后也更容易在移动平台和打包环境中控制初始化路径。

## 4. `sys.path` 和脚本入口

UE 内置插件使用项目设置里的 `AdditionalPaths`、启动脚本、pip site-packages，以及引擎内置 Python 内容路径，主要围绕编辑器 Python 环境组织。

本插件在 `UPyVirtualMachine::ConfigureSearchPaths` 中显式设置隔离模式下的搜索路径：

- Mac/iOS 加入插件自带标准库路径
- 默认加入 `Content/Scripts`
- 可通过 `Game.ini` 的 `[UnrealPython] ScriptPath` 覆盖脚本目录
- 可通过 `UUPySettings::AdditionalPaths` 追加项目相对路径
- 非编辑器下不再复制 `Content/Scripts` 和 `Content/Settings` 到外部可读写路径，脚本和配置需要通过 stage/package 进入包体

这样修改的原因是打包后内容路径和编辑器路径不同，移动平台尤其不能假设 Python 可以直接读取项目源目录。本插件把脚本路径约束到项目内容和插件运行时目录，便于 Cook/Package 后运行。

## 5. Python 模块命名和暴露 API

UE 内置插件暴露的主模块名是 `unreal`。

本插件暴露的主模块名是 `ue`。`FUPyModuleInitializer` 创建了自定义模块类型 `UPyUEModule`，并重载 `tp_getattro`：当属性不在模块字典中时，会通过 `StaticFindAllObjects` 按名称查找 `UClass`、`UScriptStruct` 或 `UEnum`，再从 `FUPyWrapperTypeRegistry` 取对应 Python 类型并缓存到模块字典。

这样修改带来的效果是：

- `ue.Actor`、`ue.Vector` 等类型可以按需解析
- 不必在模块初始化时一次性生成全部反射类型
- 运行时脚本可以使用更短、更游戏侧的模块名

这也意味着本插件与 UE 内置 `unreal` API 不完全兼容；迁移脚本时不能只改 import 名，还要检查类型、函数、容器和生命周期语义。

## 6. 绑定生成策略

这是两者最大的实现差异。

UE 内置插件主要在运行时通过反射动态生成包装类型。源码中有 `PyWrapperTypeRegistry`、`PyGenUtil`、`PyWrapperObject/Struct/Enum`、Blueprint 类型生成、重实例化、编辑器对象清理等逻辑，重点是覆盖编辑器可见的反射对象和蓝图类型。

本插件保留了动态包装基础，但新增了一套 UHT 生成管线：

- `Source/UnrealPythonUhtGenerator` 是 UHT exporter
- 只有设置环境变量 `GENERATE_UNREAL_PYTHON` 时才执行生成
- `UnrealPythonCodeGenerator` 导出反射 JSON 和 `.pyi`
- `AutoWrapperGenerator` 根据 `Tools/GenerationSettings.json` 生成 C++ 包装代码
- 生成代码落在 `Public/Wrapper/AutoGen`
- `UPyAutoGenWrapper.h` 聚合初始化生成的类型
- `FUPyWrapperTypeRegistry::RegisterAutoWrappedClassCreateFunc` / `RegisterAutoWrappedStructCreateFunc` 先登记创建函数，再统一创建静态类型

这样修改的原因是运行时性能和可打包性。反射动态生成虽然灵活，但启动成本、编辑器依赖、平台行为都更难控制；静态生成的 C++ wrapper 可以提前编译，打包后不依赖编辑器生成逻辑，并且可以由 `GenerationSettings.json` 精确裁剪导出范围。`.pyi` 生成也改善了 IDE 补全和类型提示。

## 7. 类型 wrapper 体系差异

两边 wrapper 的基础思想相同：用一个 Python `PyTypeObject` 表示 UE 反射类型，用一个 Python wrapper instance 持有 UObject、UScriptStruct、容器或 delegate 的实际数据，然后通过 `PyGenUtil/UPyGenUtil` 保存属性、函数、操作符等调用元数据。

但本插件不是简单把 `PyWrapper*` 改名成 `UPyWrapper*`，而是在 wrapper 层做了几类关键改造。

### 7.1 文件和类型布局

UE 内置插件的 wrapper 基本都放在 `PythonScriptPlugin/Private`，包括：

- `PyWrapperObject`
- `PyWrapperStruct`
- `PyWrapperEnum`
- `PyWrapperArray`
- `PyWrapperFixedArray`
- `PyWrapperSet`
- `PyWrapperMap`
- `PyWrapperDelegate`
- `PyWrapperFieldPath`
- `PyWrapperName`
- `PyWrapperText`
- `PyWrapperMath`
- `PyWrapperTypeRegistry`

本插件把 wrapper 拆到 `Public/Wrapper` 和 `Private/Wrapper`，并新增自动生成目录 `Public/Wrapper/AutoGen`。核心文件包括：

- `UPyWrapperObjectBase`
- `UPyWrapperObject`
- `UPyWrapperStruct`
- `UPyWrapperEnum`
- `UPyWrapperArray`
- `UPyWrapperFixedArray`
- `UPyWrapperSet`
- `UPyWrapperMap`
- `UPyWrapperDelegate`
- `UPyWrapperFieldPath`
- `UPyWrapperTypeFactory`
- `UPyWrapperTypeRegistry`
- `Wrapper/AutoGen/.../UPyWrapper*.cpp/.h`

这样修改的原因是本插件的生成代码和游戏模块也需要复用 wrapper API。UE 内置插件把大部分实现藏在 Private 下，适合插件内部动态生成；本插件需要让 UHT 生成的 C++ wrapper、项目模块 wrapper、辅助导出库都能包含这些类型，所以把稳定的 wrapper 接口提升到 Public。

### 7.2 Object wrapper 拆分

UE 内置插件用 `FPyWrapperObject` 同时承担 UObject wrapper 的基类和具体 `PyWrapperObjectType` 实现。

本插件拆成两层：

- `FUPyWrapperObjectBase`：保存 `TObjectPtr<UObject> ObjectInstance`，实现属性读写、UFunction 调用、getter/setter、动态函数调用等通用逻辑
- `FUPyWrapperObject`：继承 `FUPyWrapperObjectBase`，作为普通 UObject 的默认 wrapper type

这样修改的原因是自动生成 class wrapper 需要一个更清晰的对象基类。`Actor`、`Widget`、`GameInstance` 等生成类型都可以直接复用 `FUPyWrapperObjectBase` 的调用实现，而默认 `UPyWrapperObjectType` 只作为兜底 UObject wrapper 存在。这个拆分也方便后续生成专门的 class wrapper 时保持相同 instance 内存布局和通用调用路径。

### 7.3 静态 AutoGen wrapper

UE 内置插件的 `FPyWrapperTypeRegistry::GenerateWrappedClassType/StructType/EnumType` 会在运行时构造 `PyTypeObject`、填充 `tp_methods`、`tp_getset`、meta-data，再注册到模块中。它支持蓝图生成类型、重实例化、弃用别名、包重载等编辑器场景。

本插件保留了 `GenerateWrappedClassType/StructType/EnumType` 这类动态路径，但核心路径转向静态生成：

- UHT 根据 `GenerationSettings.json` 生成 `UPyWrapperActor`、`UPyWrapperVector`、`UPyWrapperUserWidget` 等 C++ 类型
- 每个生成 wrapper 提供类似 `DoInitWrapperActor` 的创建函数
- `InitializeUPyWrapperActor` 只向 `FUPyWrapperTypeRegistry` 注册 `RegisterAutoWrappedClassCreateFunc`
- `FUPyWrapperTypeRegistry::StartCreateAutoWrappedTypes` 统一按继承关系创建 wrapper 类型

这样修改的原因是减少运行时反射扫描和动态组装成本。生成出来的 wrapper 是普通 C++，能参与编译检查，也能按 `GenerationSettings.json` 精确裁剪导出范围。对移动端和打包游戏来说，这比运行时生成全部可见反射类型更可控。

### 7.4 TypeRegistry 的 key 和职责变化

UE 内置 `FPyWrapperTypeRegistry` 用 `FSoftObjectPath` 做很多注册 key，因为它要处理蓝图资产、重命名、重载、弃用类型名、包 reload 等编辑器状态。

本插件 `FUPyWrapperTypeRegistry` 改为更多直接使用反射指针：

- `TMap<const UClass*, const PyTypeObject*> PythonWrappedClasses`
- `TMap<const UScriptStruct*, const PyTypeObject*> PythonWrappedStructs`
- `TMap<const UEnum*, const PyTypeObject*> PythonWrappedEnums`
- `TMap<const UFunction*, const PyTypeObject*> PythonWrappedDelegates`
- 反向保存 `PyTypeObject* -> UClass/UScriptStruct/UEnum/UFunction`
- 额外保存 `AutoWrappedClassCreateFuncs` 和 `AutoWrappedStructCreateFuncs`

这样修改的原因是运行时里类型集合更稳定，直接用 `UClass*`、`UScriptStruct*` 查找更简单、更快，也不需要承担完整编辑器资产路径重定向语义。需要按名称访问时，由 `ue` 模块的 `tp_getattro` 先找对象，再查 registry。

### 7.5 wrapper factory 独立化

UE 内置插件也有完整的 wrapper factory 抽象，定义在 `PyWrapperTypeRegistry.h` 中：

- `TPyWrapperTypeFactory`
- `FPyWrapperObjectFactory`
- `FPyWrapperStructFactory`
- `FPyWrapperDelegateFactory`
- `FPyWrapperMulticastDelegateFactory`
- `FPyWrapperNameFactory`
- `FPyWrapperTextFactory`
- `FPyWrapperArrayFactory`
- `FPyWrapperFixedArrayFactory`
- `FPyWrapperSetFactory`
- `FPyWrapperMapFactory`
- `FPyWrapperFieldPathFactory`

本插件的差异不是“新增 factory”，而是对这套 factory 做了运行时化改造：

- 把 factory 从 `PyWrapperTypeRegistry.h` 中拆到独立的 Public 头 `UPyWrapperTypeFactory.h`，供自动生成 wrapper 和游戏模块包含
- `TUPyWrapperTypeFactory` 的 `MappedInstances` 从内置插件的 `(WrapperKey, PyTypeObject*)` 复合 key 简化为 `UnrealType -> PythonType*`
- `FUPyWrapperObjectFactory` 的 Python 类型从 `FPyWrapperObject` 调整为 `FUPyWrapperObjectBase`，配合 Object wrapper 的基类拆分
- 去掉 `FName`、`FText` 专用 factory，对应类型主要走字符串转换，不再作为独立 wrapper 类型维护
- multicast delegate factory 增加属性地址和 `FMulticastDelegateProperty` 参数，服务运行时 delegate 绑定语义
- `FUPyWrapperObjectFactory` 还维护 `MappedOwnedPyProps`，用于 UObject 拥有的 Python 属性 wrapper 清理

这样修改的原因是运行时需要更明确地管理 wrapper instance 的所有权和复用，同时让生成代码可以直接依赖 factory API。特别是 UObject 销毁时，`UUPyManager::NotifyUObjectDeleted` 会通过 factory 解除映射并清理对象拥有的 Python 属性，避免 Python 侧继续访问已销毁对象。

### 7.6 PyWrapper 具体实现差异

除了 registry 和 factory，具体 wrapper 的实现也有不少差异，主要体现在生命周期、方法集合和代码组织上。

基础 wrapper 层面，UE 内置插件会在类型初始化时给基础 wrapper type 挂专用 metadata。例如 `InitializePyWrapperObject`、`InitializePyWrapperStruct`、`InitializePyWrapperEnum` 都会在 `PyType_Ready` 后创建静态 metadata，并调用 `FPyWrapperObjectMetaData::SetMetaData(&PyWrapperObjectType, &MetaData)` 这类函数。metadata 里保存 `UClass`、`UScriptStruct`、`UEnum`、Python 属性名/函数名映射、弃用信息和引用收集逻辑；后续 `GetClass`、`GetStruct`、`ResolvePropertyName`、`IsFunctionDeprecated` 等路径都依赖它。

本插件虽然保留了 `FUPyWrapperBaseMetaData` 和 `_wrapper_meta_data` helper，但具体 Object/Struct/Enum wrapper 初始化基本不再给基础 type 设置专用 metadata。`InitializeUPyWrapperObject` 只设置 `tp_base`、`tp_methods`、`tp_flags`，然后 `RegisterWrappedClassType(UObject::StaticClass(), PyType)`；自动生成的 `UPyWrapperActor` 等类型也是 `PyType_Ready` 后直接注册到 `FUPyWrapperTypeRegistry`，没有对应的 `FUPyWrapperObjectMetaData::SetMetaData` 路径。Struct/Enum/Delegate 里相关 metadata 访问或 `SetMetaData` 调用也大多被注释掉。

这样修改的核心原因是反射类型绑定信息从“挂在 Python type dict 上的 metadata 对象”迁移到了 `FUPyWrapperTypeRegistry` 和 AutoGen 描述符里：Class/Struct/Enum 通过 `PyTypeObject* -> UClass/UScriptStruct/UEnum` 反查，属性和函数通过生成代码里的 `UPyGenUtil::FGeneratedWrappedProperty`、`FGeneratedWrappedFunction` 保存。这样更适合静态生成和运行时裁剪，但也意味着本插件不再复用 UE 内置插件那套基于 `FPyWrapper*MetaData` 的继承解析、弃用提示和引用收集路径。

instance 跟踪方式也不同。UE 内置 `FPyWrapperBase::New/Free` 会把每个 wrapper instance 注册到 `FPyReferenceCollector`，用于编辑器 Python 与 UObject GC 的统一引用收集；本插件的 `FUPyWrapperBase::New/Free` 中这部分调用被注释掉，生命周期更多交给 `UUPyManager`、wrapper factory 映射、UObject 删除监听和 Python-owned 对象集合管理。同时，UE 内置插件还有 `UPythonObjectHandle`，用于把任意 Python 对象挂到 Python generated type 的 UPROPERTY 上；本插件没有保留对应的 `UUPythonObjectHandle` 类型。

Object wrapper 层面，UE 内置插件把 UObject wrapper、对象方法和 `UPythonGeneratedClass` 的大量实现都放在 `PyWrapperObject.cpp` 里，`FPyWrapperObject` 同时是对象 wrapper 基类和默认对象类型。本插件把它拆开：

- `UPyWrapperObjectBase.cpp`：保存对象实例并实现属性访问、函数调用、getter/setter、内部状态校验
- `UPyWrapperObject.cpp`：只保留普通 `Object` 默认类型，实际文件很薄
- `Subclassing/UPyGeneratedClass.cpp`：承接 Python 生成 UE class 的逻辑
- `MethodDefs/UPyMethodDefsObject.cpp`：集中放置暴露给 Python 的对象方法

这个拆分的直接影响是自动生成 wrapper 可以统一继承 `FUPyWrapperObjectBase`，而不必把默认 `Object` wrapper 当成所有 class wrapper 的实现载体。本插件还额外暴露了运行时所有权和根集相关方法，例如 `AddPythonOwned`、`RemovePythonOwned`、`IsPythonOwned`、`AddToRoot`、`RemoveFromRoot`；UE 内置插件更偏编辑器 API，常见方法是 `get_editor_property`、`set_editor_property`、`call_method` 这类 snake_case 命名。

Struct wrapper 层面，UE 内置 `PyWrapperStruct` 提供了较完整的动态 struct 工具方法，例如 `cast`、`static_struct`、`copy`、`assign`、`to_tuple`、`get_editor_property`、`set_editor_property`、`export_text`、`import_text`。本插件基础 `UPyWrapperStruct` 暴露的方法更少，主要保留 `StaticStruct` 和 `_post_init`，具体属性、函数、操作符更多依赖 AutoGen wrapper 生成出来。`FUPyWrapperStruct` 也更强调 const 反射类型指针和 inline storage 配置，服务静态生成 struct wrapper。

Enum wrapper 层面，UE 内置 `PyWrapperEnum` 仍保留 `cast`、`static_enum`、`get_display_name` 等方法，并包含弃用 enum value 的 warning 路径。本插件的 `UPyWrapperEnum` 中这些方法表和弃用 warning 逻辑基本被注释掉，只保留 enum entry 的名称、值、迭代和成员访问能力。原因是本插件的 enum 主要作为运行时脚本常量和生成类型的一部分使用，不再承担编辑器文档化、弃用提示和动态转换的全部职责。

容器 wrapper 层面，Array、Set、Map、Delegate、MulticastDelegate 的底层 storage 和迭代模型仍接近 UE 内置插件，但 Python 方法名和暴露范围明显不同：

- Array 从 `append/count/extend/index/insert/pop/clear/remove/reverse/sort/resize` 改为 `Append/Count/Extend/Index/Insert/Pop/Clear/Remove/Reverse/Sort/Resize`
- Set 从 `add/discard/remove/pop/clear/difference/update/isdisjoint/issubset/issuperset` 改为 `Add/Discard/Remove/Pop/Clear/Difference/Update/IsDisjoint/IsSubset/IsSuperset`
- Map 从 `copy/clear/fromkeys/get/setdefault/pop/popitem/update/items/keys/values` 改为 `Copy/Clear/FromKeys/Get/SetDefault/Pop/PopItem/Update/Items/Keys/Values`
- Delegate 从 `bind_function/bind_callable/unbind/execute/execute_if_bound` 改为 `BindFunction/Bind/Unbind/Execute/ExecuteIfBound`，部分 copy 或 delegate 专用方法被裁掉
- MulticastDelegate 从 `add_function/add_callable/remove/contains/broadcast` 改为 `AddFunction/Add/Remove/Contains/Broadcast` 等 PascalCase 方法

这说明本插件并不追求与 `unreal` 模块脚本 API 逐项兼容，而是把 wrapper API 调整为和生成出来的 UE 类型方法命名更一致。迁移 UE 内置 Python 脚本时，容器和 delegate 调用通常需要一起改名，不能只替换 `import unreal` 为 `import ue`。

最后，代码组织也体现了目标差异。UE 内置 wrapper 文件往往同时包含类型对象、方法表、编辑器兼容逻辑和 generated type 支撑代码；本插件则把 instance 行为、方法定义、子类生成、类型注册和 AutoGen wrapper 拆开。这样做会增加文件数量，但更适合运行时裁剪、静态生成和平台打包，也降低了每个 wrapper 文件的职责重量。

### 7.7 Struct wrapper 与 inline struct

UE 内置插件支持普通 struct wrapper，也支持一些内建数学类型的专门处理，例如 `PyWrapperMath`、`PyWrapperName`、`PyWrapperText`。

本插件去掉了单独的 `UPyWrapperName`、`UPyWrapperText`、`UPyWrapperMath` 文件，转而主要通过：

- `UPyConversion` 直接把 `FName`、`FText` 映射为 Python 字符串语义
- `AutoGen/CoreUObject/Struct/UPyWrapperVector`、`UPyWrapperRotator`、`UPyWrapperTransform` 等生成 wrapper 表达数学类型
- `GenerationSettings.json` 的 `IsInline` 配置决定小型 struct 是否使用 inline storage，减少堆分配

这样修改的原因是静态生成后，数学类型不再需要集中放在一个手写 `PyWrapperMath` 中维护。每个导出的 struct 都可以有自己的生成代码、属性、方法、操作符和常量，裁剪和补全也更直接。

### 7.8 MethodDefs 和 wrapper 解耦

UE 内置插件很多 Python 方法直接跟 wrapper 类型文件组织在一起，编辑器功能也混在核心模块中。

本插件新增 `Private/MethodDefs` 目录，把 `ue` 模块函数、Object/Struct/Array/Map/Set/Delegate 等 Python 方法定义拆开：

- `UPyMethodDefsUE.cpp`
- `UPyMethodDefsObject.cpp`
- `UPyMethodDefsStruct.cpp`
- `UPyMethodDefsArray.cpp`
- `UPyMethodDefsMap.cpp`
- `UPyMethodDefsSet.cpp`
- `UPyMethodDefsDelegate.cpp`
- `UPyMethodDefsMulticastDelegate.cpp`

这样修改的原因是让 wrapper 的“实例内存和基础行为”与“暴露给 Python 的方法集合”分离。静态生成 wrapper 时可以复用基础 wrapper，同时按类型生成不同的 `PyMethodDef`、`PyGetSetDef` 和 operator slot。

### 7.9 Python 子类生成仍保留，但更偏运行时

UE 内置插件有 `UPythonGeneratedClass/Struct/Enum`，用于把 Python 类型生成 UE 类型，服务编辑器脚本和蓝图相关场景。

本插件保留了类似能力，但改名并放到 `Subclassing`：

- `UUPyGeneratedClass`
- `UUPyGeneratedStruct`
- `UUPyGeneratedEnum`

这些类仍依赖 `UPyWrapperObjectBase`、`UPyWrapperStruct`、`UPyWrapperEnum` 判断 Python 类型并生成 UE 类型。差异在于，本插件的主要导出路径已经不是编辑器运行时动态生成反射 wrapper，而是“静态生成 UE 原生类型 wrapper + 必要时支持 Python 子类生成”。

### 7.10 对脚本 API 的影响

wrapper 差异最终会反映到 Python 脚本行为：

- 本插件暴露的是 `ue` 模块，不是 `unreal` 模块
- 本插件默认只稳定暴露 `GenerationSettings.json` 选中的自动生成类型和少量动态查找类型
- 数学类型、UMG、Engine 常用类的成员来自生成 wrapper 和 `UPyScript*` helper，不完全等同于 UE 内置插件的 `unreal.Vector`、`unreal.Object` 行为
- `FName`、`FText` 更偏 Python 字符串转换，而不是单独 wrapper 类型
- 对 UObject 生命周期更敏感，Python 持有对象需要关注 `AddPythonOwned`、`RemovePythonOwned`、`IsValid` 等运行时语义

因此，迁移 UE 内置 Python 脚本时，wrapper 层是最容易出现不兼容的地方。不能只把 `import unreal` 改成 `import ue`，还要检查类型构造、属性名、方法名、操作符、容器返回值和对象生命周期。

## 8. 类型导出 Meta

UE 内置插件使用 Epic 的脚本导出约定，例如 `ScriptMethod`、`ScriptConstant`、`ScriptOperator`、`ScriptName` 等。

本插件的 UHT 生成器额外使用 `UPy` 前缀的 Meta：

- `UPyScriptMethod`
- `UPyScriptMethodSelfReturn`
- `UPyScriptMethodMutable`
- `UPyScriptOperator`
- `UPyScriptConstant`
- `UPyScriptHost`
- `UPyScriptStatic`
- `UPyUseHelperMethod`

`UPyEditorScriptExportHelperLibrary` 里大量函数只用于编辑器期辅助生成；`UPyRuntimeScriptExportHelperLibrary` 里的 `UPyUseHelperMethod` 函数则是运行时实际会调用的实现。

这样修改的原因是把本插件的导出语义和 UE 内置插件隔离，避免和引擎已有 Python 元数据混在一起。同时，`UPyUseHelperMethod` 明确区分“只用于生成元数据的辅助函数”和“运行时真正调用的 helper 函数”，这对打包很关键：前者可以放在 `WITH_EDITOR`，后者不能被裁掉。

## 9. 包装类型覆盖范围

UE 内置插件覆盖面更偏编辑器脚本，包含 `PyEditor`、`PySlate`、在线文档、命令菜单、Content Browser 文件脚本执行等。

本插件覆盖面更偏游戏运行时，源码中新增或强化了：

- `PyGameFramework/UPyGameInstance`：从可配置的 Python factory 函数创建每个 UE GameInstance 对应的 Python 对象，并调用对象的 `init`、`on_start`、`tick`、`shutdown`，以及模块级 `after_shutdown`
- `PyGameFramework/UPyTimerManager`
- `Helper/UPyBlueprintLibrary`：Blueprint/C++ 调 Python 函数
- UMG、InputCore、Engine、CoreUObject 的一批自动生成 wrapper
- `UPyRuntimeScriptExportHelperLibrary`：补充运行时常用但原生 API 不适合直接导出的函数
- `UUPyManager` 中的 Python 拥有对象集合、UObject 删除监听、编辑器对象替换处理

这样修改是为了让 Python 能直接参与游戏逻辑和 UI 逻辑，而不只是执行编辑器命令。`UPyGameInstance` 提供了一个简单、稳定的游戏侧入口，`BlueprintLibrary` 则降低蓝图和 C++ 调用 Python 的成本。

## 10. 被移除或弱化的 UE 内置能力

本插件没有保留或没有完整保留以下 UE 内置插件能力：

- `IPythonScriptPlugin` 公开接口
- `PythonScriptCommandlet`
- `PythonOnlineDocsCommandlet`
- `PipInstall` 及 pip requirements 管线
- `PythonScriptRemoteExecution`
- `EditorPythonScriptingLibrary`
- Content Browser Python 文件集成
- `debugpy_unreal.py`、`remote_execution.py`、测试脚本等内容资源
- Python REPL 执行器的完整求值/返回值捕获逻辑

这些能力被移除的原因主要是控制依赖和运行时体积。它们对编辑器自动化很有价值，但会引入 `Analytics`、`AssetRegistry`、`DesktopPlatform`、`Networking`、`ContentBrowser*`、`KismetCompiler`、pip 工具链等依赖；对游戏运行时插件来说，这些依赖会增加打包复杂度，也不适合移动平台。

## 11. 编辑器功能改造

UE 内置插件的编辑器功能非常完整，包含菜单、控制台、REPL、文件执行、远程执行、Content Browser 集成、启动脚本进度提示等。

本插件的 `UnrealPythonEditor` 目前只做一件核心事情：在 `LevelEditor.MainMenu.Tools` 下添加 `Unreal Python -> Generate Python`，调用 `FUPyCodeGenerator::GenerateAllCode()` 导出 `Tools/ReflectionData/native_module.json`。

运行时模块在编辑器下额外注册了 `UnrealPython` 控制台执行器和 ToolMenus 字符串命令处理器，但实现很轻量，直接 `PyRun_SimpleString`。

这样修改的原因是编辑器只作为生成和调试入口存在，不再承担 Python 自动化平台的完整职责。

## 12. 运行时对象和 GC 处理

UE 内置插件有 `PyReferenceCollector`、编辑器对象清理、包重载、蓝图重实例化等复杂逻辑，以适应编辑器中对象频繁重载和蓝图热变更。

本插件保留了对象包装和重实例化相关基础，但更强调运行时 UObject 生命周期：

- `UUPyManager` 注册 `GUObjectArray` 删除监听
- UObject 删除时解除 Python wrapper 映射
- `PythonOwnedObjects` 防止 Python 持有对象过早被 UE GC
- `UPyGameInstance::PyGC` 提供手动触发 UE GC 的入口

这样修改的原因是游戏运行时里对象销毁通常来自关卡、Actor、Widget 生命周期；Python wrapper 如果继续持有已销毁 UObject，会导致悬挂引用。运行时管理需要比编辑器命令执行更关注对象所有权。

## 13. 其他容易漏掉的细节差异

除了前面的主干差异，还有一些实现细节会直接影响调试、迁移和后续维护。

控制台执行器注册位置不同。UE 内置插件在 `PythonScriptPlugin` 模块里注册两个执行器：`Python` 和 `PythonREPL`。`Python` 支持脚本或文件执行，`PythonREPL` 支持单语句求值并显示结果。本插件的 `FUnrealPythonModule` 是运行时模块，但控制台执行器代码被 `WITH_EDITOR` 包住，只在编辑器构建里注册一个 `UnrealPython` executor；它没有 REPL executor，也没有文件路径执行、补全和求值结果捕获，执行时只是加 GIL 后 `PyRun_SimpleString`。因此它是“运行时模块中的编辑器控制台入口”，不是打包游戏里的通用控制台执行器。

`ue` 模块初始化顺序也被重排。`FUPyModuleInitializer::InitializeModules` 先初始化 `__main__`，再创建自定义类型的 `ue` 模块，然后注册 core/wrapper/AutoGen 类型。内置插件则围绕 `unreal` 模块初始化，并带有 `PyCore`、`PyEngine`、`PyEditor`、`PySlate` 等子模块职责。本插件的 `ue` 模块还会写入 `BUILD_SHIPPING` 和 `GIsEditor`，并通过 `tp_getattro` 懒查找 native `UClass/UScriptStruct/UEnum`；查找时显式跳过 BlueprintGeneratedClass/UserDefinedStruct/UserDefinedEnum 这类非 native 类型。

Python VM 配置更硬。UE 内置插件的隔离模式、AdditionalPaths、StartupScripts、pip、remote execution、developer mode 等都来自 `UPythonScriptPluginSettings` 和 user settings。本插件的 `FUPyVirtualMachine` 目前硬编码 isolated interpreter，`parse_argv=0`、`utf8_mode=1`、`use_environment=0`、`module_search_paths_set=1`，然后手动写入插件 Python 运行时路径、`Content/Scripts` 和 `UUPySettings::AdditionalPaths`。它还会用 UE 原始命令行填充 `sys.argv`，但没有保留 UE 内置的 startup scripts 队列。

配置对象被大幅裁剪。UE 内置 `UPythonScriptPluginSettings` 是 `config=Engine`，还有 per-user settings，包含启用开关、开发者模式、类型提示、pip、远程执行、Content Browser 集成等选项。本插件的 `UUPySettings` 是 `config=Game`，目前只保留 `AdditionalPaths`，脚本主路径则通过 `Game.ini` 的 `[UnrealPython] ScriptPath` 单独读取。这个取舍让项目打包配置更直接，但也移除了很多编辑器 Python 的开关。

输出和生命周期 hook 也换了。本插件初始化后用一段 Python 代码把 `sys.stdout`、`sys.stderr` 替换成调用 `ue.Log`、`ue.LogError` 的 redirector，然后固定调用 `ue_site.on_init()`；关闭时调用 `ue_site.on_shutdown()`；编辑器 ticker 中每帧调用 `ue_site.on_tick()`。UE 内置插件更依赖自身的执行上下文、日志捕获、启动脚本和编辑器回调；本插件则把游戏侧生命周期留给项目脚本里的 `ue_site`。

模块函数命名和覆盖范围不一致。本插件 `ue` 模块方法是 PascalCase，例如 `Log`、`NewObject`、`FindObject`、`LoadObject`、`LoadClass`、`FindAsset`、`CollectGarbage`、`GetTypeFromClass`、`GetContentDir`。UE 内置 `unreal` 模块里很多函数和 wrapper 方法是 snake_case。本插件还保留了 `uclass/ustruct/uenum/uproperty/ufunction` 这类 Python 生成 UE 类型的 decorator 入口，但诸如 `reload`、`load_module`、`purge_object_references`、localized text helper、shutdown callback 等函数在方法表里已经注释掉。

类型转换层虽然源自 UE 插件，但可见性和目标不同。UE 内置 `PyConversion` 主要是 Private 内部工具；本插件把 `UPyConversion` 放到 Public，并加入一些项目需要的 `TBaseStructure` 特化，例如 `FVector2f`、`FVector3f`、`FTimespan`、`FARFilter` 等。这样 C++ 游戏代码、Blueprint helper 和 AutoGen wrapper 都能直接复用转换层。代价是转换 API 已经成为本插件公共 ABI，后续修改要比 UE 内置 Private 工具更谨慎。

C++/Blueprint 调 Python 的入口也不同。UE 内置插件提供的是面向编辑器脚本的 `PythonScriptLibrary`、K2 node、命令执行和文件执行能力。本插件新增 `UUPyBlueprintLibrary`，提供少量 BlueprintCallable 固定签名函数和一个 C++ variadic template `CallPythonMethod`，通过 `UPyConversion` 拼参数和转返回值。这个入口更像游戏代码调用 Python 逻辑的桥，而不是编辑器脚本平台。

Build 依赖和 docstring 策略也有差异。本插件不依赖 `Python3`，直接链接 `ThirdParty/python314`；运行时模块额外依赖 `UMG`、`InputCore`、`NetCore`、`FieldNotification` 等游戏侧模块，并在 editor build 才定义 `WITH_UPY_DOC_STRINGS=1`。UE 内置插件依赖 `Python3`、`Analytics`、`Networking`、`Json`、`DesktopPlatform`、`ContentBrowser*`、`KismetCompiler` 等编辑器自动化相关模块。这个差异解释了为什么本插件能服务打包运行时，但缺少大量编辑器工具链能力。

这些细节合起来说明：本插件不是只把 UE 插件“瘦身”，而是把很多原来挂在 `FPythonScriptPlugin`、`FPyWrapper*MetaData` 和编辑器设置里的状态，迁移到了 `UUPyManager`、`FUPyVirtualMachine`、`FUPyModuleInitializer`、`FUPyWrapperTypeRegistry`、AutoGen 描述符和项目脚本 `ue_site` 中。

## 14. 文档和维护建议

后续维护本插件时，不建议直接从 UE 内置 `PythonScriptPlugin` 整文件同步。两边的目标已经分叉：

- 需要 CPython API、GIL、类型转换、容器包装修复时，可以参考 UE 内置插件对应文件
- 涉及初始化、Build.cs、模块依赖、脚本路径、打包资源、UHT 生成、`UPy` Meta 时，应优先遵循本插件现有结构
- 从 UE 内置插件移植功能前，需要先判断它是编辑器自动化能力还是运行时脚本能力
- 新增导出类型优先通过 `Tools/GenerationSettings.json` 和 UHT 生成管线处理，不要随意扩大动态反射生成范围
- `UPyUseHelperMethod` 的实现必须留在运行时可编译代码中，不能只放在 `WITH_EDITOR`

简言之：UE 内置插件适合回答“编辑器里如何执行 Python”；本插件适合回答“打包游戏里如何嵌入 Python 并让 Python 调 UE 类型”。这个目标差异解释了大部分源码改动。
