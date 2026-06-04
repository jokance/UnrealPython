# UnrealPython 插件使用说明文档

UnrealPython 是一个用于在 Unreal Engine 运行时集成 Python 的插件。它允许开发者使用 Python 编写游戏逻辑，并通过 Blueprint 或 C++ 与 Python 代码进行交互。

**Python 版本**: 3.14

**支持平台**: Win64, Android, Mac, iOS

## 目录

- [UnrealPython 插件使用说明文档](#unrealpython-插件使用说明文档)
  - [目录](#目录)
  - [安装](#安装)
  - [配置](#配置)
  - [UE 内置 Python 冲突处理](#ue-内置-python-冲突处理)
  - [IDE 自动补全](#ide-自动补全)
  - [使用方法](#使用方法)
    - [Game Instance 集成](#game-instance-集成)
    - [Blueprint 调用](#blueprint-调用)
    - [C++ 调用](#c-调用)
    - [C++ 辅助导出 Meta 标记](#c-辅助导出-meta-标记)
    - [项目模块 C++ 类型导出 (GetOnInitializePythonWrappers)](#项目模块-c-类型导出-getoninitializepythonwrappers)
    - [ue\_site 模块](#ue_site-模块)
    - [Timer Manager](#timer-manager)
    - [编辑器工具 (UnrealPythonEditor)](#编辑器工具-unrealpythoneditor)
  - [注意事项](#注意事项)

## 安装

1. 将 `UnrealPython` 文件夹复制到你的项目目录下的 `Plugins` 文件夹中（如果不存在该文件夹请手动创建）。
2. 重新生成项目文件（右键 .uproject -> Generate Visual Studio project files）。
3. 运行以下命令生成Python封装文件（请根据实际路径替换 `%UE_ENGINE_DIR%` 和 `YourProjectPath`）：
   ```cmd
   "%UE_ENGINE_DIR%\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" -Project="YourProjectPath\YourProject.uproject" YourProjectEditor Win64 Development -FromMsBuild -ForceHeaderGeneration -SkipBuild
   ```
4. 编译并启动项目。
5. 在编辑器中启用插件（通常会自动启用）。

## 配置

可以在项目设置中配置 Python 搜索路径：

1. 打开 **Project Settings** (项目设置)。
2. 找到 **Python** 栏目 (在 Plugins 分类下)。
3. 在 **Additional Paths** 中添加你的 Python 脚本所在的目录路径（相对于项目目录）。

Windows、Android 和 iOS 打包时都需要把项目脚本目录作为 Non-UFS 文件 staged 进包体，并确保对应平台的 Python runtime 一起进入包内。SampleGame 当前使用的配置和打包命令记录在 [Docs/PythonRuntimeBuild.md](Docs/PythonRuntimeBuild.md)。

## UE 内置 Python 冲突处理

本插件自带并初始化 Python 3.14，不使用 UE 内置 `PythonScriptPlugin` 的 Python 运行时。UE 内置 Python 通常用于编辑器自动化，并且可能被 Niagara、ControlRig、Sequencer 等引擎插件依赖链间接启用；一旦它在同一个 Editor 进程中初始化，就可能和本插件的 Python 3.14 产生 ABI、符号、`sys.path`、扩展模块加载和生命周期冲突。

如果需要在 Editor 或 PIE 中使用本插件的 Python 3.14，推荐显式禁用 UE 内置 Python 初始化。

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

## IDE 自动补全

插件支持生成 Python 接口定义文件 (.pyi)，用于 IDE（如 VS Code, PyCharm）的代码补全和类型提示。

1. **生成接口文件**:
   - 运行安装步骤中提到的 `UnrealBuildTool` 命令（带有 `-ForceHeaderGeneration` 参数）会自动生成接口文件。
   - 生成的文件位于：`Plugins/UnrealPython/Tools/pystubs`。

2. **配置 VS Code**:
   - 打开 `.vscode/settings.json`。
   - 添加 `python.analysis.extraPaths` 配置：
     ```json
     "python.analysis.extraPaths": [
         "${workspaceFolder}/Plugins/UnrealPython/Tools/pystubs"
     ]
     ```

3. **配置 PyCharm**:
   - 在项目视图中，找到 `Plugins/UnrealPython/Tools/pystubs` 目录。
   - 右键点击该目录，选择 **Mark Directory as -> Sources Root**。

## 使用方法

### Game Instance 集成

插件提供了一个 `UUPyGameInstance` 类，可以作为项目的 Game Instance 基类，用于管理 Python 模块的生命周期。

1. **创建 Game Instance**:
   - 在 UE 编辑器中，创建一个新的 Blueprint 类，继承自 `UPyGameInstance`。
   - 或者在你的 C++ Game Instance 中继承 `UUPyGameInstance`。
   - 在 **Project Settings -> Maps & Modes -> Game Instance Class** 中设置为你创建的类。

2. **配置 Python 模块**:
   - 打开你创建的 Game Instance Blueprint。
   - 在 Details 面板中找到 **Python** 分类。
   - 设置 **Game Instance Module Name** 为你想要加载的 Python 模块名称（默认 `game_instance`，无需 `.py` 后缀）。

3. **编写 Python 脚本**:
   在你配置的 Python 路径下创建一个 Python 文件（例如 `game_instance.py`），并实现以下生命周期函数：

   ```python
   import ue

   def init():
       print("Python GameInstance: Init")

   def on_start():
       print("Python GameInstance: OnStart")

   def tick(delta_time):
       # print(f"Python GameInstance: Tick {delta_time}")
       pass

   def shutdown():
       print("Python GameInstance: Shutdown")
   ```

4. **手动触发垃圾回收**:
   - 在游戏运行时，可以通过控制台命令 `PyGC` 手动触发 Python 垃圾回收。
   - 这对于调试内存问题或手动清理循环引用对象非常有用。

### Blueprint 调用

你可以使用 `UnrealPython` 提供的 Blueprint 节点直接调用 Python 函数。

- **CallPythonMethod_Str_RetStr**: 调用返回字符串的 Python 函数。
- **CallPythonMethod_Int_RetStr**: 传入整数参数并返回字符串。
- **CallPythonMethod_Int_RetInt**: 传入整数参数并返回整数。

**参数说明**:
- `Module Name`: Python 模块名（文件名）。
- `Method Name`: 函数名。
- `Arg`: 传递给函数的参数。

### C++ 调用

在 C++ 中，可以使用 `UUPyBlueprintLibrary::CallPythonMethod` 模板函数来调用 Python 方法：

```cpp
#include "Helper/UPyBlueprintLibrary.h"

// 示例：调用 my_module.py 中的 my_func，传入 int 和 string，不返回值
UUPyBlueprintLibrary::CallPythonMethod<void>("my_module", "my_func", 123, FString("Hello"));

// 示例：调用并获取返回值
int32 Result = UUPyBlueprintLibrary::CallPythonMethod<int32>("my_module", "calculate", 10, 20);
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

- **UPyScriptStatic**: 将函数标记为静态方法。通常与 `UPyScriptMethod` 结合使用。
  - 示例: `meta=(UPyScriptMethod = "CrossProduct", UPyScriptStatic)`
  - 效果: 在 Python 中可以通过 `ue.Vector.CrossProduct(vec_a, vec_b)` 调用。

- **UPyScriptOperator**: 将函数映射为 Python 操作符（如 `+`, `-`, `*`, `==` 等）。支持分号分隔多个操作符。
  - 示例: `meta=(UPyScriptOperator = "+;+=") `
  - 效果: 支持 Python 中的 `a + b` 和 `a += b` 操作。

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
   // 引入自动生成的 Wrapper 头文件
   // 路径通常为 PyAutoGenWrapper/UPy{ModuleName}Wrapper.h
   #include "UPy/Wrapper/AutoGen/UPySampleGameModuleWrapper.h"

   void FSampleGameModule::StartupModule()
   {
       // 注册 Python 包装器初始化回调
       PythonInitHandle = FUnrealPythonModule::Get().GetOnInitializePythonWrappers().AddLambda([]()
       {
           UPyGenUtil::FNativePythonModule ModuleInfo;
           // 获取 ue 模块作为父模块
           ModuleInfo.PyModule = (PyObject*)FUnrealPythonModule::Get().GetPyUEModule();
           
           // 初始化本模块生成的 Wrapper 类型
           // 函数名为 Initialize{ModuleName}WrapperTypes
           InitializeSampleGameModuleWrapperTypes(ModuleInfo);
       });
   }

   void FSampleGameModule::ShutdownModule()
   {
       // 移除回调
       FUnrealPythonModule::Get().GetOnInitializePythonWrappers().Remove(PythonInitHandle);
   }
   ```

### ue_site 模块

`ue_site` 是一个特殊的 Python 模块，如果存在于 Python 搜索路径中，插件会自动加载并执行其中的特定回调函数。这通常用于全局的初始化和清理工作。

**支持的回调函数**:

- `on_init()`: 插件初始化时调用 (StartupModule)。
- `on_shutdown()`: 插件关闭时调用 (ShutdownModule)。

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
```

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

在 Python 中，需要在 `ue_site.py` 或其他已加载模块中定义 `on_timer` 回调函数：

```python
import ue

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
- **用途**: 该数据通常用于辅助生成 IDE 的智能感知文件 (.pyi) 或供其他外部工具分析使用。

## 注意事项

- 请确保你的 Python 环境已正确设置，且脚本路径已添加到 **Additional Paths** 中。
- 插件会自动处理基本的类型转换，但复杂的对象传递可能需要查阅源码中的 `Wrapper` 实现。

## 模块依赖

| 模块 | 类型 | 说明 |
|------|------|------|
| UnrealPython | Runtime | 核心 Python 集成模块 |
| UnrealPythonEditor | Editor | 编辑器工具模块 |

**核心依赖**:
- Core, CoreUObject, Engine
- Slate, SlateCore
- DeveloperSettings, Projects, InputCore
- NetCore, UMG, PhysicsCore, FieldNotification
