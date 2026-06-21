# UnrealPython 插件使用说明文档

UnrealPython 是一个用于在 Unreal Engine 运行时嵌入 CPython 的插件。它使用插件自带的 Python 运行时，暴露 `ue` 模块、自动生成的 UE 类型包装、Blueprint/C++ 调 Python 入口，以及面向 packaged game 的脚本加载和生命周期回调。

**Python 版本**: 3.14.5（插件目录名为 `ThirdParty/python314`）

**Runtime 支持平台**: Win64, Android, Mac, iOS

**Editor 工具支持平台**: Win64, Mac

## 目录

- [UnrealPython 插件使用说明文档](#unrealpython-插件使用说明文档)
  - [目录](#目录)
  - [安装](#安装)
  - [配置](#配置)
  - [UE 内置 Python 冲突处理](#ue-内置-python-冲突处理)
  - [绑定生成与 IDE 自动补全](#绑定生成与-ide-自动补全)
  - [使用方法](#使用方法)
    - [ue 模块基础](#ue-模块基础)
    - [Game Instance 集成](#game-instance-集成)
    - [Blueprint 调用](#blueprint-调用)
    - [C++ 调用](#c-调用)
    - [C++ 辅助导出 Meta 标记](#c-辅助导出-meta-标记)
    - [项目模块 C++ 类型导出 (GetOnInitializePythonWrappers)](#项目模块-c-类型导出-getoninitializepythonwrappers)
    - [ue\_site 模块](#ue_site-模块)
    - [容器、对象生命周期与 Delegate](#容器对象生命周期与-delegate)
    - [Generated Type](#generated-type)
    - [Packaged / Dedicated Server](#packaged--dedicated-server)
    - [Timer Manager](#timer-manager)
    - [编辑器工具 (UnrealPythonEditor)](#编辑器工具-unrealpythoneditor)
  - [注意事项](#注意事项)

## 安装

1. 将 `UnrealPython` 文件夹放到项目目录的 `Plugins` 文件夹下。
2. 重新生成项目文件（右键 `.uproject` -> **Generate Visual Studio project files**）。
3. 生成 Python wrapper、反射数据和 `.pyi` 补全文件。

   Windows 推荐使用插件脚本：

   ```powershell
   powershell -ExecutionPolicy Bypass -File Plugins\UnrealPython\Tools\GenPy.ps1 `
     -EngineDir "D:\Games\UE_5.7\Engine"
   ```

   Mac 推荐使用：

   ```bash
   ENGINE_DIR="/Path/To/UnrealEngine/Engine" Plugins/UnrealPython/Tools/GenPy.sh
   ```

   这两个脚本会设置 `GENERATE_UNREAL_PYTHON=1`，然后以 `-ForceHeaderGeneration -SkipBuild` 运行 Editor target。直接运行 UBT 时也必须设置这个环境变量，否则 UHT exporter 会跳过生成。

4. 编译并启动项目。
5. 在编辑器中启用插件（`UnrealPython.uplugin` 当前为 `EnabledByDefault=true`，通常会自动启用）。

## 配置

Python VM 使用隔离配置初始化，不读取系统 Python 环境。默认脚本入口目录为：

```text
Content/Scripts
```

可以用 `Config/DefaultGame.ini` 覆盖主脚本目录：

```ini
[UnrealPython]
ScriptPath=Content/Scripts
```

也可以在项目设置中追加搜索路径：

1. 打开 **Project Settings** (项目设置)。
2. 找到 **Python** 栏目 (在 Plugins 分类下)。
3. 在 **Additional Paths** 中添加你的 Python 脚本所在的目录路径（相对于项目目录）。

打包时建议把项目脚本目录作为 Non-UFS 文件 staged 进包体：

```ini
[/Script/UnrealEd.ProjectPackagingSettings]
+DirectoriesToAlwaysStageAsNonUFS=(Path="Scripts")
```

Win64 会把 `python314.dll` 和 `python314.zip` staged 到可执行文件目录。Mac 会 staged `libpython3.14.dylib` 和标准库。iOS 会把 `Python.xcframework` 与 runtime bundle 资源放进 app。Android 会在构建时生成 `Intermediate/Android/unrealpython_bootstrap.zip`，并通过 `UnrealPython_UPL.xml` 打进 APK asset，运行时解压到 `Saved/UnrealPythonBootstrap` 后加入 `sys.path`。

各平台 runtime 构建、裁剪、打包和验证命令见 [Docs/PythonRuntimeBuild.md](Docs/PythonRuntimeBuild.md)。

## UE 内置 Python 冲突处理

本插件自带并初始化 Python 3.14.5，不使用 UE 内置 `PythonScriptPlugin` 的 Python 运行时。UE 内置 Python 通常用于编辑器自动化，并且可能被 Niagara、ControlRig、Sequencer 等引擎插件依赖链间接启用；一旦它在同一个 Editor 进程中初始化，就可能和本插件的 Python 3.14.5 产生 ABI、符号、`sys.path`、扩展模块加载和生命周期冲突。

如果需要在 Editor 或 PIE 中使用本插件的 Python 3.14.5，推荐显式禁用 UE 内置 Python 初始化。

临时启动 Editor 时可以使用：

```bash
-DisablePython
```

项目级配置可以在 `Config/DefaultEngine.ini` 中加入：

```ini
[ConsoleVariables]
Engine.Python.IsEnabledByDefault=0
```

- 如果需要保留 UE 官方 Python 的编辑器功能，建议不要在同一个 Editor/PIE 进程中初始化本插件 Python；改用 Standalone Game、Launch 或 packaged build 测试运行时 Python。

## 绑定生成与 IDE 自动补全

插件的 UHT exporter 负责生成三类内容：

- `Tools/ReflectionData/unreal.json`：UE 反射导出数据。
- `Tools/pystubs/ue/__init__.pyi`：IDE 补全和类型提示文件。
- `Source/UnrealPython/Public/Wrapper/AutoGen`：静态 C++ wrapper。

`Tools/GenerationSettings.json` 控制导出范围、排除列表、inline struct、`ExportToGameModules`、`GameExportPath` 和自定义 stub 输出路径。当前主要导出了 CoreUObject、Engine、InputCore、SlateCore、UMG 的常用类型，并支持把项目模块 wrapper 输出到 `Source/<GameModule>/Public/UPy/Wrapper/AutoGen`。

生成流程必须设置 `GENERATE_UNREAL_PYTHON=1`。推荐直接使用安装步骤里的 `Tools/GenPy.ps1` 或 `Tools/GenPy.sh`。如果需要控制 stub 注释输出，可以通过 `UNREAL_PYTHON_STUB_COMMENTS` 设置；默认会生成 `__init__.pyi` 和 `__init__no_comments.pyi`。

`Tools -> Unreal Python -> Generate Python` 菜单只导出 Native Python module 反射信息到 `Tools/ReflectionData/native_module.json`，它会被下一次 UHT 生成流程读取，用于补全 `ue` 模块上的 C API 函数。

1. **配置 VS Code**:
   - 打开 `.vscode/settings.json`。
   - 添加 `python.analysis.extraPaths` 配置：
     ```json
     "python.analysis.extraPaths": [
         "${workspaceFolder}/Plugins/UnrealPython/Tools/pystubs"
     ]
     ```

2. **配置 PyCharm**:
   - 在项目视图中，找到 `Plugins/UnrealPython/Tools/pystubs` 目录。
   - 右键点击该目录，选择 **Mark Directory as -> Sources Root**。

## 使用方法

### ue 模块基础

Python 脚本中使用 `import ue` 访问插件暴露的 Unreal API。`ue` 模块会在属性访问时按需解析 native `UClass`、`UScriptStruct` 和 `UEnum`，例如 `ue.Actor`、`ue.Vector`、`ue.UserWidget`。

常用模块函数包括：

- 日志：`ue.Log()`、`ue.LogWarning()`、`ue.LogError()`、`ue.LogDisplay()`、`ue.LogVerbose()`、`ue.LogFlush()`。
- 对象和资产：`ue.NewObject()`、`ue.FindObject()`、`ue.LoadObject()`、`ue.LoadClass()`、`ue.FindAsset()`、`ue.LoadAsset()`、`ue.FindPackage()`、`ue.LoadPackage()`、`ue.GetDefaultObject()`。
- 类型查询：`ue.GetTypeFromClass()`、`ue.GetTypeFromStruct()`、`ue.GetStructFromType()`、`ue.GetTypeFromEnum()`。
- 生成类型：`ue.uclass()`、`ue.ustruct()`、`ue.uenum()`、`ue.uvalue()`、`ue.uproperty()`、`ue.ufunction()`。
- 调试和路径：`ue.CollectGarbage()`、`ue.IsEditor()`、`ue.GetContentDir()`、`ue.GetGameSavedDir()`。

`sys.stdout` 会重定向到 `ue.Log`，`sys.stderr` 会重定向到 `ue.LogError`，因此普通 `print()` 会进入 Unreal 日志。

### Game Instance 集成

插件提供了一个 `UUPyGameInstance` 类（继承自 `UPlatformGameInstance`），可以作为项目的 Game Instance 基类，用于管理 Python 模块的生命周期。

1. **创建 Game Instance**:
   - 在 UE 编辑器中，创建一个新的 Blueprint 类，继承自 `UPyGameInstance`。
   - 或者在你的 C++ Game Instance 中继承 `UUPyGameInstance`。
   - 在 **Project Settings -> Maps & Modes -> Game Instance Class** 中设置为你创建的类。

2. **配置 Python 模块**:
   - 打开你创建的 Game Instance Blueprint。
   - 在 Details 面板中找到 **Python** 分类。
   - 设置 **Game Instance Module Name** 为你想要加载的 Python 模块名称（默认 `game_instance`，无需 `.py` 后缀）。
   - **Game Instance Factory Function Name** 默认 `create_game_instance`，用于为当前 UE GameInstance 创建 Python 回调对象。

3. **编写 Python 脚本**:
   在你配置的 Python 路径下创建一个 Python 文件（例如 `game_instance.py`），并提供 factory 函数创建业务对象。每个 `UUPyGameInstance` 会持有自己的 Python 对象，并调用该对象上的生命周期方法。只有当对象上存在 `tick` 方法时，插件才会注册每帧 ticker。

   ```python
   class MyGameInstance:
       def init(self, game_instance):
           print("Python GameInstance: Init")

       def on_start(self):
           print("Python GameInstance: OnStart")

       def tick(self, delta_time):
           pass

       def shutdown(self):
           print("Python GameInstance: Shutdown")


   def create_game_instance(ue_game_instance):
       return MyGameInstance()
   ```

### Blueprint 调用

你可以使用 `UnrealPython` 提供的 Blueprint 节点直接调用 Python 函数。

| 节点 | 说明 |
|------|------|
| `CallPythonMethod_Str_RetStr` | 传入 `FString`，返回 `FString` |
| `CallPythonMethod_Int_RetStr` | 传入 `int32`，返回 `FString` |
| `CallPythonMethod_Int_RetInt` | 传入 `int32`，返回 `int32` |
| `CallPythonMethod_IntArray_RetIntArray` | 传入 `TArray<int32>`，返回 `TArray<int32>` |
| `CallPythonMethod_StrArray_RetStrArray` | 传入 `TArray<FString>`，返回 `TArray<FString>` |
| `CallPythonMethod_IntArray_RetStrArray` | 传入 `TArray<int32>`，返回 `TArray<FString>` |
| `CallPythonMethod_StrArray_RetIntArray` | 传入 `TArray<FString>`，返回 `TArray<int32>` |
| `CallLoadedPythonMethod` | 只在模块已加载时调用无参 Python 函数，成功返回 `true`；不会主动 import 模块 |

**参数说明**:
- `Module Name`: Python 模块名（文件名）。
- `Method Name`: 函数名。
- `Arg`: 固定签名节点的单个参数。

普通 `CallPythonMethod_*` 节点会先 import 指定模块，再查找并调用函数。`CallLoadedPythonMethod` 适合在关闭或回调路径中调用可选 hook，避免因为模块尚未加载而产生新的 import。

### C++ 调用

在 C++ 中，可以使用 `UUPyBlueprintLibrary::CallPythonMethod` 模板函数来调用 Python 方法：

```cpp
#include "Helper/UPyBlueprintLibrary.h"

// 示例：调用 my_module.py 中的 my_func，传入 int 和 string，不返回值
UUPyBlueprintLibrary::CallPythonMethod<void>("my_module", "my_func", 123, FString("Hello"));

// 示例：调用并获取返回值
int32 Result = UUPyBlueprintLibrary::CallPythonMethod<int32>("my_module", "calculate", 10, 20);

// 示例：只在模块已经加载时调用无参 hook
bool bCalled = UUPyBlueprintLibrary::CallLoadedPythonMethod(TEXT("my_module"), TEXT("on_ready"));
```

### C++ 辅助导出 Meta 标记

通过在 C++ 的 `UFUNCTION` 中添加特定的 `meta` 标记，可以控制 C++ 函数如何暴露给 Python。这些标记通常在辅助导出库中使用（如 `UUPyEditorScriptExportHelperLibrary` 或 `UPyRuntimeScriptExportHelperLibrary`）。

**常用 Meta 标记说明**:

- **UPyScriptConstant**: 将函数暴露为宿主类型（第一个参数的类型）的属性或常量。
  - 示例: `meta=(UPyScriptConstant = "ZeroVector")`
  - 效果: 在 Python 中可以通过 `ue.Vector.ZeroVector` 访问。

- **UPyScriptMethod**: 将函数暴露为宿主类型的方法。第一个参数必须是宿主类型。
  - 示例: `meta=(UPyScriptMethod = "Cross")`
  - 效果: 在 Python 中可以通过 `vector_instance.Cross(other)` 调用。

- **UPyScriptHost**: 显式指定函数应该挂到哪个宿主类型上，适合 helper 函数没有宿主类型参数，或宿主类型需要通过路径指定的情况。
  - 示例: `meta=(UPyScriptConstant="ConstantValue", UPyScriptHost="/Script/UnrealPython.UPyTestStruct")`
  - 效果: 将函数导出为指定类型上的常量或方法。

- **UPyScriptStatic**: 将函数标记为静态方法。通常与 `UPyScriptMethod` 结合使用。
  - 示例: `meta=(UPyScriptMethod = "CrossProduct", UPyScriptStatic)`
  - 效果: 在 Python 中可以通过 `ue.Vector.CrossProduct(vec_a, vec_b)` 调用。

- **UPyScriptOperator**: 将函数映射为 Python 操作符（如 `+`, `-`, `*`, `==` 等）。支持分号分隔多个操作符。
  - 示例: `meta=(UPyScriptOperator = "+;+=") `
  - 效果: 支持 Python 中的 `a + b` 和 `a += b` 操作。

- **UPyScriptMethodSelfReturn**: 标记 helper 方法的返回值语义为宿主类型自身，常用于链式或 in-place 操作对应的生成数据。使用前应结合当前生成代码验证具体类型的行为。

- **UPyScriptMethodMutable**: 标记 helper 方法会修改宿主实例，常用于 struct helper 的可变操作语义。使用前应结合当前生成代码验证是否会按引用写回。

- **UPyUseHelperMethod**: 标记该方法虽然定义在辅助库中，但逻辑上属于第一个参数的类型。这通常用于扩展现有类型的 Python 绑定。
  - 示例: `meta=(UPyScriptMethod = "SetRotation", UPyUseHelperMethod)`

> **注意：关于 WITH_EDITOR 宏的使用**
>
> 大部分辅助导出的函数（如仅用于标记 `UPyScriptConstant`, `UPyScriptMethod`, `UPyScriptOperator` 等生成信息的函数）通常只在编辑器下用于辅助 UnrealHeaderTool 导出封装代码，**建议使用 `#if WITH_EDITOR` 宏包裹**，以避免不必要的运行时开销。
>
> **例外**：被标记为 `UPyUseHelperMethod` 的函数是运行时实际会被调用的实现函数，**严禁**使用 `#if WITH_EDITOR` 包裹，否则会导致打包后的游戏在运行时因找不到该函数实现而报错。

**代码示例**:

```cpp
// 1. 暴露 FVector 的 ZeroVector 属性
UFUNCTION(BlueprintPure, meta = (UPyScriptConstant = "ZeroVector"))
static FVector Vector_ZeroVector(const FVector& Host);

// 2. 暴露 FVector 的 Cross 方法，同时支持 ^ 操作符
UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Cross", UPyScriptOperator = "^"))
static FVector Vector_Cross(const FVector& Host, const FVector& V);

// 3. 暴露静态方法 CrossProduct
UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "CrossProduct", UPyScriptStatic))
static FVector Vector_CrossProduct(const FVector& Host, const FVector& A, const FVector& B);

// 4. 使用 Helper 方法扩展 Transform 的 SetRotation
UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "SetRotation", UPyUseHelperMethod))
static void Transform_SetRotation(FTransform& InTrans, const FRotator& InRot);
```

### 项目模块 C++ 类型导出 (GetOnInitializePythonWrappers)

如果你在游戏主模块或其他模块中定义了需要暴露给 Python 的 C++ 类型（使用了 `UPyScriptMethod` 等标记），你需要手动注册初始化回调，以确保生成的包装代码在 Python 环境初始化时被正确加载。

这通常在模块的 `StartupModule` 和 `ShutdownModule` 中完成。

如果 `Tools/GenerationSettings.json` 中把项目模块列入 `ExportToGameModules`，生成器会默认把项目模块 wrapper 输出到 `Source/<GameModule>/Public/UPy/Wrapper/AutoGen`，并在同目录生成 `UPyGameAutoGenWrapper.h` 聚合头。项目模块的 `.Build.cs` 需要依赖 `UnrealPython`，以便包含 `Utils/UPyGenUtil.h`、wrapper 类型和 `FUnrealPythonModule`。

**示例代码**:

假设你的模块名为 `SampleGame`。

1. **头文件 (.h)**:
   声明 `FDelegateHandle` 用于保存注册句柄。

   ```cpp
   class FSampleGameModule : public FDefaultGameModuleImpl
   {
   public:
       virtual void StartupModule() override;
       virtual void ShutdownModule() override;
   private:
       FDelegateHandle PythonInitHandle;
   };
   ```

2. **源文件 (.cpp)**:
   在 `StartupModule` 中注册 `GetOnInitializePythonWrappers` 回调，并在 `ShutdownModule` 中清理。

   ```cpp
   #include "SampleGame.h"
   #include "UnrealPython.h"
   // 引入自动生成的项目 wrapper 聚合头。
   // 也可以直接包含 "UPy/Wrapper/AutoGen/SampleGame/UPySampleGameModuleWrapper.h"
   #include "UPy/Wrapper/AutoGen/UPyGameAutoGenWrapper.h"

   void FSampleGameModule::StartupModule()
   {
       // 注册 Python 包装器初始化回调
       PythonInitHandle = FUnrealPythonModule::Get().GetOnInitializePythonWrappers().AddLambda([]()
       {
           UPyGenUtil::FNativePythonModule ModuleInfo;
           // 获取 ue 模块作为父模块
           ModuleInfo.PyModule = (PyObject*)FUnrealPythonModule::Get().GetPyUEModule();
           
           // 初始化项目模块生成的 Wrapper 类型。
           InitializeGameAutoGenWrapperTypes(ModuleInfo);
       });
   }

   void FSampleGameModule::ShutdownModule()
   {
       // 移除回调
       FUnrealPythonModule::Get().GetOnInitializePythonWrappers().Remove(PythonInitHandle);
   }
   ```

### ue_site 模块

`ue_site` 是项目侧全局入口模块。Runtime 模块在非 commandlet 运行时初始化 Python VM 后，会导入 `ue_site` 并调用生命周期函数；关闭 Python 前会调用 `on_shutdown()`；每帧 ticker 会调用 `on_tick()`。

项目脚本目录中应提供 `ue_site.py`，即使暂时没有逻辑，也建议保留空函数，避免启动或 Tick 时出现导入/属性错误。

**支持的回调函数**:

- `on_init()`: 插件初始化时调用 (StartupModule)。
- `on_shutdown()`: 插件关闭时调用 (ShutdownModule)。
- `on_tick()`: 插件初始化后每帧调用。该回调在 Editor、PIE、Standalone、packaged game、dedicated/listen server 运行时都会注册。

**示例**:

在你的 Python 路径下创建 `ue_site.py`:

```python
import ue

def on_init():
    print("UnrealPython Plugin Initialized")
    # 在这里执行全局初始化逻辑

def on_shutdown():
    print("UnrealPython Plugin Shutdown")
    # 在这里执行清理逻辑

def on_tick():
    pass
```

### 容器、对象生命周期与 Delegate

本插件暴露的 wrapper API 使用 PascalCase 方法名，不与 UE 内置 `unreal` 模块的 snake_case API 完全兼容。迁移脚本时不要只替换 `import unreal` 为 `import ue`。

常用容器方法：

- `Array`: `Append`、`Count`、`Extend`、`Index`、`Insert`、`Pop`、`Clear`、`Remove`、`Reverse`、`Sort`、`Resize`、`Copy`。
- `Set`: `Add`、`Discard`、`Remove`、`Pop`、`Clear`、`Difference`、`DifferenceUpdate`、`Intersection`、`IntersectionUpdate`、`Union`、`Update`、`IsDisjoint`、`IsSubset`、`IsSuperset`。
- `Map`: `Copy`、`Clear`、`FromKeys`、`Get`、`SetDefault`、`Pop`、`PopItem`、`Update`、`Items`、`Keys`、`Values`。

UObject wrapper 提供 `IsValid()`、`StaticClass()`、`GetClass()`、`GetWorld()`、`AddPythonOwned()`、`RemovePythonOwned()`、`IsPythonOwned()`、`AddToRoot()`、`RemoveFromRoot()` 等方法。Python 长期持有由脚本创建或跨帧使用的 UObject 时，优先使用 `AddPythonOwned()` / `RemovePythonOwned()` 管理生命周期；访问属性前建议用 `IsValid()` 判断对象是否仍然有效。

Delegate wrapper 支持：

- 单播 delegate: `IsBound()`、`BindFunction(obj, name)`、`Bind(callable)`、`Unbind()`、`Execute(*args)`、`ExecuteIfBound(*args)`。
- Multicast delegate: `AddFunction`、`Add`、`AddFunctionUnique`、`AddUnique`、`RemoveFunction`、`Remove`、`RemoveObject`、`ContainsFunction`、`Contains`、`Clear`、`Broadcast`。

当前 wrapper 会在生成的属性 getter/setter 前校验内部 UObject 状态，并支持 sparse delegate 的延迟解析；如果对象已被 UE 销毁，对应 wrapper 会被标记为无效。

### Generated Type

运行时生成 `uclass`、`ustruct`、`uenum`、`uproperty`、`ufunction` 的使用说明和当前支持范围见 [Docs/GeneratedType.md](Docs/GeneratedType.md)。

对于性能敏感的项目，不推荐大量使用 Generated Type，尤其不要把它放在高频 tick、批量对象生成或核心战斗/模拟路径上。Generated Type 会在运行时创建 transient 反射类型，并通过 Python/C++ 边界、反射调用、参数转换、GIL 和 Python 对象生命周期管理完成属性与函数访问；这些成本高于原生 C++ 类型、预生成 wrapper 或稳定的 Blueprint/C++ 类型。它更适合原型验证、少量运行时胶水类型、测试辅助和工具化场景；长期稳定且高频使用的类型建议落到 C++/Blueprint，并通过常规 wrapper 或导出机制暴露给 Python。

这些 decorator 对应的 Python definition type 已启用 CPython cycle GC 支持。`ue.ufunction()` 当前支持关键字参数形式（如 `Params=[...]`, `Ret=...`）以及生成器支持的 positional decorator 参数路径；具体参数和限制以 [Docs/GeneratedType.md](Docs/GeneratedType.md) 为准。

### Packaged / Dedicated Server

UnrealPython 的 Runtime 模块会在非 commandlet 运行时初始化 Python VM，因此 packaged game、listen server 和 dedicated server 都可以运行 `ue_site` 入口及游戏侧 Python 脚本。项目需要把脚本目录 staged 到包体，例如：

```ini
[/Script/UnrealEd.ProjectPackagingSettings]
+DirectoriesToAlwaysStageAsNonUFS=(Path="Scripts")

[UnrealPython]
ScriptPath=Content/Scripts
```

Dedicated/listen server 脚本应在 Python 里按 `World.GetNetModeName()`、`World.GetNetMode()`、`GameplayStatics.GetGameInstance(world)` 或 `world.OwningGameInstance` 区分上下文。

平台差异：

- Win64 packaged build 需要 `python314.dll` 和 `python314.zip` 位于可执行文件目录。
- Android 构建会把 Python stdlib、`_android_support.py` 和构建时存在的 `Content/Scripts` 打成 APK asset `unrealpython_bootstrap.zip`，运行时按 hash/version 解压；同时仍会尝试读取 packaged script path 和 OBB 内脚本路径。
- Mac/iOS runtime 由 `Build.cs` 负责收集动态库、标准库或 bundle resource。
- Commandlet 下会跳过 Python VM 初始化，避免 Cook/生成流程误启动运行时 Python。

本项目已验证：

- Editor dedicated server: `UnrealEditor <Project>.uproject <Map> -server -DisablePython` 下，UnrealPython 的 `on_init()` 和 `on_tick()` 均触发，`World.GetNetModeName()` 返回 `DedicatedServer`。
- Mac packaged game: Game target 构建阶段通过，并确认 UnrealPython 的 `libpython3.14.dylib` 与 Python 标准库 runtime dependencies 被构建流程收集。
- Packaged dedicated server executable: 需要引擎分发支持 Server target。当前 UE_5.7 环境构建 `TargetType.Server` 时返回不支持 Server targets，因此不能在该环境完成真正的 packaged dedicated server 可执行文件验证。

### Timer Manager

插件提供了 `UUPyTimerManager` 类，用于在 Python 中管理定时器回调。

**Blueprint 节点**:

| 节点 | 说明 | 参数 | 返回值 |
|------|------|------|--------|
| `SetTimer` | 设置循环定时器 | `Rate`: 间隔时间（秒） | Timer Handle |
| `SetOnceTimer` | 设置单次定时器 | `Delay`: 延迟时间（秒） | Timer Handle |
| `ClearTimer` | 清除指定定时器 | `Handle`: 定时器句柄 | - |
| `ClearAllTimers` | 清除所有定时器 | - | - |

**Python 使用示例**:

在 Python 中，需要在 Python 搜索路径下创建 `unreal_timer.py`，并定义 `on_timer(handle)` 回调函数。`UUPyTimerManager` 会懒加载并缓存 `unreal_timer.on_timer`，当所有 timer 清空后释放缓存。

```python
def on_timer(handle):
    """定时器回调函数，接收定时器句柄作为参数"""
    print(f"Timer {handle} triggered!")

# 设置定时器需要在 Blueprint 或 C++ 中调用 UUPyTimerManager 的方法
```

**C++ 调用示例**:

```cpp
#include "PyGameFramework/UPyTimerManager.h"

// 设置一个每 1 秒触发的循环定时器
int32 TimerHandle = UUPyTimerManager::SetTimer(1.0f);

// 设置一个 5 秒后触发的单次定时器
int32 OnceTimerHandle = UUPyTimerManager::SetOnceTimer(5.0f);

// 清除定时器
UUPyTimerManager::ClearTimer(TimerHandle);

// 清除所有定时器
UUPyTimerManager::ClearAllTimers();
```

### 编辑器工具 (UnrealPythonEditor)

插件在 Unreal Engine 编辑器中提供了一些辅助工具。

**生成 Python 反射数据**:
- **菜单路径**: `Tools` -> `Unreal Python` -> `Generate Python`
- **功能**: 扫描所有注册的 Native Python 模块和方法，将其反射信息（模块名、方法名、文档等）导出为 JSON 文件。
- **输出路径**: `Plugins/UnrealPython/Tools/ReflectionData/native_module.json`
- **用途**: 该数据会被下一次 `GENERATE_UNREAL_PYTHON=1` 的 UHT 生成流程读取，用于补全 `ue` 模块上的 Native C API。完整静态 wrapper 和 `.pyi` 仍然需要运行 `Tools/GenPy.ps1` 或 `Tools/GenPy.sh`。

## 注意事项

- 不要在同一个进程中同时初始化 UE 内置 `PythonScriptPlugin` 和本插件 Python 运行时。
- 项目脚本建议放在 `Content/Scripts`，并提供 `ue_site.py` 与按需提供 `unreal_timer.py`。
- 修改 `Tools/GenerationSettings.json`、`UPyScript*` meta 或 helper export 后，需要重新运行 `Tools/GenPy.*` 并重新编译。
- 插件会自动处理基本类型、UObject、UStruct、UEnum、容器、delegate 和 FieldPath 的转换；复杂对象生命周期请优先检查 wrapper 的 `IsValid()` 和 Python-owned 管理。
- 生成 wrapper 已在属性访问前校验对象状态；如果 Python 保存了跨关卡或跨销毁周期的 UObject wrapper，仍应在使用前判断有效性。
