# Unreal Python: Runtime Python integration for Unreal Engine 5

English | [中文](README.zh.md)

UnrealPython is a plugin for embedding CPython into Unreal Engine at runtime. It bundles its own Python runtime and provides the `ue` module, generated Unreal Engine type wrappers, and Blueprint/C++ interop entry points so projects can access engine objects and write runtime logic in Python.

**Python version**: 3.14.5 (the bundled runtime directory is `ThirdParty/python314`)

**Supported Unreal Engine versions**: 5.7, 5.8

**Runtime supported platforms**: Win64, Android, Mac, iOS

**Editor tool supported platforms**: Win64, Mac

---

To speed up game development, try these companion plugins:

- [WidgetSemanticBridge](https://www.fab.com/listings/92270793-0b09-406a-81b9-d6f9f307044f): Use AI to quickly generate and iterate UMG, with plain-text DSL files that describe widget structure. Combined with UnrealPython and DSL workflows, you can hand all UI logic development to AI.
- [MaterialSemanticBridge](https://www.fab.com/listings/cb2fcfbe-a4db-4dee-a9cf-8bbe62823418): Use AI to quickly generate and iterate materials, turning AI into your dedicated technical artist.


## Contents

- [Installation](#installation)
- [Configuration](#configuration)
- [Handling Conflicts With UE's Built-in Python](#handling-conflicts-with-ues-built-in-python)
- [Binding Generation and IDE Completion](#binding-generation-and-ide-completion)
- [Usage](#usage)
  - [ue Module Basics](#ue-module-basics)
  - [Game Instance Integration](#game-instance-integration)
  - [Blueprint Calls](#blueprint-calls)
  - [C++ Calls](#c-calls)
  - [C++ Helper Export Meta Tags](#c-helper-export-meta-tags)
  - [Exporting Project Module C++ Types (GetOnInitializePythonWrappers)](#exporting-project-module-c-types-getoninitializepythonwrappers)
  - [ue_site Module](#ue_site-module)
  - [Containers, Object Lifetime, and Delegates](#containers-object-lifetime-and-delegates)
  - [Generated Type](#generated-type)
  - [Packaged / Dedicated Server](#packaged--dedicated-server)
  - [Timer Manager](#timer-manager)
  - [Editor Tools (UnrealPythonEditor)](#editor-tools-unrealpythoneditor)
- [Notes](#notes)
- [Related Documentation](#related-documentation)

## Installation

1. Place the `UnrealPython` folder under your project's `Plugins` folder.
2. Regenerate project files by right-clicking the `.uproject` file and choosing **Generate Visual Studio project files**.
3. Generate Python wrappers, reflection data, and `.pyi` completion files.

   On Windows, use the plugin script:

   ```powershell
   powershell -ExecutionPolicy Bypass -File Plugins\UnrealPython\Tools\GenPy.ps1 `
     -EngineDir "D:\Games\UE_5.7\Engine"
   ```

   On Mac, use:

   ```bash
   ENGINE_DIR="/Path/To/UnrealEngine/Engine" Plugins/UnrealPython/Tools/GenPy.sh
   ```

   Both scripts set `GENERATE_UNREAL_PYTHON=1`, then run the Editor target with `-ForceHeaderGeneration -SkipBuild`. If you run UBT directly, this environment variable must also be set; otherwise the UHT exporter skips generation.

4. Build and launch the project.
5. Enable the plugin in the editor. `UnrealPython.uplugin` currently has `EnabledByDefault=true`, so it is usually enabled automatically.

## Configuration

The Python VM is initialized with an isolated configuration and does not read the system Python environment. The default script entry directory is:

```text
Content/Scripts
```

You can override the main script directory in `Config/DefaultGame.ini`:

```ini
[UnrealPython]
ScriptPath=Content/Scripts
```

You can also add search paths in Project Settings:

1. Open **Project Settings**.
2. Find the **Python** section under the Plugins category.
3. Add your Python script directories to **Additional Paths**. Paths are relative to the project directory.

For packaging, it is recommended to stage the project script directory as Non-UFS files:

```ini
[/Script/UnrealEd.ProjectPackagingSettings]
+DirectoriesToAlwaysStageAsNonUFS=(Path="Scripts")
```

On Win64, `python314.dll` and `python314.zip` are staged next to the executable. On Mac, `libpython3.14.dylib` and the standard library are staged. On iOS, `Python.xcframework` and runtime bundle resources are placed in the app. On Android, the build creates `Intermediate/Android/unrealpython_bootstrap.zip`, packages it into the APK asset through `UnrealPython_UPL.xml`, extracts it to `Saved/UnrealPythonBootstrap` at runtime, and adds it to `sys.path`.

For platform-specific runtime build, trimming, packaging, and verification commands, see [Docs/PythonRuntimeBuild.md](Docs/PythonRuntimeBuild.md).

## Handling Conflicts With UE's Built-in Python

This plugin bundles and initializes Python 3.14.5. It does not use the Python runtime from UE's built-in `PythonScriptPlugin`. UE's built-in Python is commonly used for editor automation and may be enabled indirectly by dependencies from engine plugins such as Niagara, ControlRig, or Sequencer. Once it initializes in the same Editor process, it can conflict with this plugin's Python 3.14.5 through ABI differences, symbols, `sys.path`, extension module loading, and lifecycle management.

If you need to use this plugin's Python 3.14.5 in the Editor or PIE, explicitly disabling UE's built-in Python initialization is recommended.

For a temporary Editor launch, use:

```bash
-DisablePython
```

For project-level configuration, add this to `Config/DefaultEngine.ini`:

```ini
[ConsoleVariables]
Engine.Python.IsEnabledByDefault=0
```

- If you need to keep the official UE Python editor features, avoid initializing this plugin's Python in the same Editor/PIE process. Use Standalone Game, Launch, or a packaged build to test runtime Python instead.

## Binding Generation and IDE Completion

The plugin's UHT exporter generates three groups of files:

- `Tools/ReflectionData/unreal.json`: exported UE reflection data.
- `Tools/pystubs/ue/__init__.pyi`: IDE completion and type hint stubs.
- `Source/UnrealPython/Public/Wrapper/AutoGen`: static C++ wrappers.

`Tools/GenerationSettings.json` controls the static wrapper export scope, exclusion lists, inline structs, `ExportToGameModules`, `GameExportPath`, and custom stub output paths. See [Docs/GenerationSettings.md](Docs/GenerationSettings.md) for the full field reference. The current default export covers commonly used types from CoreUObject, Engine, InputCore, SlateCore, and UMG, and supports outputting project module wrappers to `Source/<GameModule>/Public/UPy/Wrapper/AutoGen`.

The generation flow requires `GENERATE_UNREAL_PYTHON=1`. Prefer using `Tools/GenPy.ps1` or `Tools/GenPy.sh` from the installation steps. Stub comment output can be controlled through `UNREAL_PYTHON_STUB_COMMENTS`; by default `__init__.pyi` is generated, while setting the value to `2` generates both `__init__.pyi` and `__init__no_comments.pyi`.

The **Tools -> Unreal Python -> Generate Python** menu exports Native Python module reflection data to `Tools/ReflectionData/native_module.json`. This file is read by the next UHT generation pass and is used to complete C API functions on the `ue` module.

1. **VS Code configuration**:
   - Open `.vscode/settings.json`.
   - Add `python.analysis.extraPaths`:
     ```json
     "python.analysis.extraPaths": [
         "${workspaceFolder}/Plugins/UnrealPython/Tools/pystubs"
     ]
     ```

2. **PyCharm configuration**:
   - In the project view, find `Plugins/UnrealPython/Tools/pystubs`.
   - Right-click the directory and choose **Mark Directory as -> Sources Root**.

## Usage

### ue Module Basics

Use `import ue` in Python scripts to access the Unreal API exposed by this plugin. The `ue` module lazily resolves native `UClass`, `UScriptStruct`, and `UEnum` objects on attribute access, such as `ue.Actor`, `ue.Vector`, and `ue.UserWidget`.

Common module functions include:

- Logging: `ue.Log()`, `ue.LogWarning()`, `ue.LogError()`, `ue.LogDisplay()`, `ue.LogVerbose()`, `ue.LogFlush()`.
- Objects and assets: `ue.NewObject()`, `ue.FindObject()`, `ue.LoadObject()`, `ue.LoadClass()`, `ue.FindAsset()`, `ue.LoadAsset()`, `ue.FindPackage()`, `ue.LoadPackage()`, `ue.GetDefaultObject()`.
- Type queries: `ue.GetTypeFromClass()`, `ue.GetTypeFromStruct()`, `ue.GetStructFromType()`, `ue.GetTypeFromEnum()`.
- Generated types: `ue.uclass()`, `ue.ustruct()`, `ue.uenum()`, `ue.uvalue()`, `ue.uproperty()`, `ue.ufunction()`.
- Debugging and paths: `ue.CollectGarbage()`, `ue.IsEditor()`, `ue.GetContentDir()`, `ue.GetGameSavedDir()`.

`sys.stdout` is redirected to `ue.Log`, and `sys.stderr` is redirected to `ue.LogError`, so ordinary `print()` output appears in the Unreal log.

### Game Instance Integration

The plugin provides `UUPyGameInstance`, derived from `UPlatformGameInstance`, as a Game Instance base class for managing Python module lifetimes.

1. **Create a Game Instance**:
   - In the UE editor, create a Blueprint class derived from `UPyGameInstance`.
   - Or derive your C++ Game Instance from `UUPyGameInstance`.
   - Set it in **Project Settings -> Maps & Modes -> Game Instance Class**.

2. **Configure the Python module**:
   - Open the Game Instance Blueprint.
   - Find the **Python** category in the Details panel.
   - Set **Game Instance Module Name** to the Python module you want to load. The default is `game_instance`; omit the `.py` suffix.
   - **Game Instance Factory Function Name** defaults to `create_game_instance`, which creates the Python callback object for the current UE GameInstance.

3. **Write the Python script**:
   Create a Python file, for example `game_instance.py`, under a configured Python path. Provide a factory function that returns your game object. Each `UUPyGameInstance` holds its own Python object and calls lifecycle methods on it. The per-frame ticker is only registered when the object has a `tick` method.

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

### Blueprint Calls

You can call Python functions directly through Blueprint nodes provided by `UnrealPython`.

| Node | Description |
|------|-------------|
| `CallPythonMethod_Str_RetStr` | Takes an `FString`, returns an `FString` |
| `CallPythonMethod_Int_RetStr` | Takes an `int32`, returns an `FString` |
| `CallPythonMethod_Int_RetInt` | Takes an `int32`, returns an `int32` |
| `CallPythonMethod_IntArray_RetIntArray` | Takes `TArray<int32>`, returns `TArray<int32>` |
| `CallPythonMethod_StrArray_RetStrArray` | Takes `TArray<FString>`, returns `TArray<FString>` |
| `CallPythonMethod_IntArray_RetStrArray` | Takes `TArray<int32>`, returns `TArray<FString>` |
| `CallPythonMethod_StrArray_RetIntArray` | Takes `TArray<FString>`, returns `TArray<int32>` |
| `CallLoadedPythonMethod` | Calls a no-argument Python function only if the module is already loaded; returns `true` on success and does not import the module |

**Parameters**:
- `Module Name`: Python module name, matching the file name.
- `Method Name`: Function name.
- `Arg`: The single argument used by fixed-signature nodes.

Normal `CallPythonMethod_*` nodes import the specified module before looking up and calling the function. `CallLoadedPythonMethod` is useful for optional hooks in shutdown or callback paths where importing a not-yet-loaded module would be undesirable.

### C++ Calls

In C++, use the `UUPyBlueprintLibrary::CallPythonMethod` template function to call Python methods:

```cpp
#include "Helper/UPyBlueprintLibrary.h"

// Call my_func in my_module.py with an int and string argument and no return value.
UUPyBlueprintLibrary::CallPythonMethod<void>("my_module", "my_func", 123, FString("Hello"));

// Call a Python function and read the return value.
int32 Result = UUPyBlueprintLibrary::CallPythonMethod<int32>("my_module", "calculate", 10, 20);

// Call a no-argument hook only when the module is already loaded.
bool bCalled = UUPyBlueprintLibrary::CallLoadedPythonMethod(TEXT("my_module"), TEXT("on_ready"));
```

### C++ Helper Export Meta Tags

Specific `meta` tags on C++ `UFUNCTION`s control how functions are exposed to Python. These tags are commonly used in helper export libraries such as `UUPyEditorScriptExportHelperLibrary` or `UPyRuntimeScriptExportHelperLibrary`.

**Common meta tags**:

- **UPyScriptConstant**: Exposes the function as a property or constant on the host type, which is the type of the first parameter.
  - Example: `meta=(UPyScriptConstant = "ZeroVector")`
  - Python result: `ue.Vector.ZeroVector`

- **UPyScriptMethod**: Exposes the function as a method on the host type. The first parameter must be the host type.
  - Example: `meta=(UPyScriptMethod = "Cross")`
  - Python result: `vector_instance.Cross(other)`

- **UPyScriptHost**: Explicitly chooses the host type. This is useful when the helper function has no host-type parameter or when the host type must be specified by path.
  - Example: `meta=(UPyScriptConstant="ConstantValue", UPyScriptHost="/Script/UnrealPython.UPyTestStruct")`
  - Result: exports the function as a constant or method on the specified type.

- **UPyScriptStatic**: Marks a function as a static method. It is usually used together with `UPyScriptMethod`.
  - Example: `meta=(UPyScriptMethod = "CrossProduct", UPyScriptStatic)`
  - Python result: `ue.Vector.CrossProduct(vec_a, vec_b)`

- **UPyScriptOperator**: Maps the function to Python operators such as `+`, `-`, `*`, or `==`. Multiple operators can be separated with semicolons.
  - Example: `meta=(UPyScriptOperator = "+;+=")`
  - Result: supports Python operations such as `a + b` and `a += b`.

- **UPyScriptMethodSelfReturn**: Marks the helper method's return-value semantics as the host instance itself. This is commonly used for chaining or in-place operations. Verify generated code for the specific type before relying on the behavior.

- **UPyScriptMethodMutable**: Marks the helper method as mutating the host instance. This is commonly used for mutable struct helper semantics. Verify whether the generated code writes back by reference before relying on it.

- **UPyUseHelperMethod**: Marks a method as being logically owned by the first parameter's type even though it is implemented in a helper library.
  - Example: `meta=(UPyScriptMethod = "SetRotation", UPyUseHelperMethod)`

> **Note about WITH_EDITOR**
>
> Most helper export functions that only provide generation metadata, such as functions tagged with `UPyScriptConstant`, `UPyScriptMethod`, or `UPyScriptOperator`, are normally only needed by UnrealHeaderTool in the editor. Wrapping those functions in `#if WITH_EDITOR` is recommended to avoid unnecessary runtime overhead.
>
> **Exception**: functions tagged with `UPyUseHelperMethod` are actual runtime implementations. Do not wrap them in `#if WITH_EDITOR`; packaged games will fail at runtime if the implementation cannot be found.

**Examples**:

```cpp
// 1. Expose FVector.ZeroVector.
UFUNCTION(BlueprintPure, meta = (UPyScriptConstant = "ZeroVector"))
static FVector Vector_ZeroVector(const FVector& Host);

// 2. Expose FVector.Cross and support the ^ operator.
UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Cross", UPyScriptOperator = "^"))
static FVector Vector_Cross(const FVector& Host, const FVector& V);

// 3. Expose a static CrossProduct method.
UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "CrossProduct", UPyScriptStatic))
static FVector Vector_CrossProduct(const FVector& Host, const FVector& A, const FVector& B);

// 4. Extend Transform with a helper implementation of SetRotation.
UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "SetRotation", UPyUseHelperMethod))
static void Transform_SetRotation(FTransform& InTrans, const FRotator& InRot);
```

### Exporting Project Module C++ Types (GetOnInitializePythonWrappers)

If you define C++ types in your game module or other project modules and want to expose them to Python through tags such as `UPyScriptMethod`, manually register an initialization callback so the generated wrappers are loaded when the Python environment initializes.

This is usually done in the module's `StartupModule` and `ShutdownModule`.

If `Tools/GenerationSettings.json` configures project module types, or if `ExportToGameModules` specifies output to a project-side module, the generator writes wrappers to `Source/<GameModule>/Public/UPy/Wrapper/AutoGen` by default and creates an aggregate `UPyGameAutoGenWrapper.h` header in the same directory. The project module's `.Build.cs` must depend on `UnrealPython` so it can include `Utils/UPyGenUtil.h`, wrapper types, and `FUnrealPythonModule`.

**Example**:

Assume your module is named `SampleGame`.

1. **Header (.h)**:
   Declare a `FDelegateHandle` to store the registration handle.

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

2. **Source (.cpp)**:
   Register a `GetOnInitializePythonWrappers` callback in `StartupModule` and remove it in `ShutdownModule`.

   ```cpp
   #include "SampleGame.h"
   #include "UnrealPython.h"
   // Include the generated aggregate project wrapper header.
   // You can also include "UPy/Wrapper/AutoGen/SampleGame/UPySampleGameModuleWrapper.h" directly.
   #include "UPy/Wrapper/AutoGen/UPyGameAutoGenWrapper.h"

   void FSampleGameModule::StartupModule()
   {
       PythonInitHandle = FUnrealPythonModule::Get().GetOnInitializePythonWrappers().AddLambda([]()
       {
           UPyGenUtil::FNativePythonModule ModuleInfo;
           ModuleInfo.PyModule = (PyObject*)FUnrealPythonModule::Get().GetPyUEModule();
           
           InitializeGameAutoGenWrapperTypes(ModuleInfo);
       });
   }

   void FSampleGameModule::ShutdownModule()
   {
       FUnrealPythonModule::Get().GetOnInitializePythonWrappers().Remove(PythonInitHandle);
   }
   ```

### ue_site Module

`ue_site` is the project-level global entry module. After the Runtime module initializes the Python VM outside commandlets, it imports `ue_site` and calls lifecycle functions. It calls `on_shutdown()` before shutting Python down, and calls `on_tick()` every frame through a ticker.

Your project script directory should provide `ue_site.py`. Even if it has no logic yet, keeping empty functions is recommended to avoid import or attribute errors during startup and tick.

**Supported callbacks**:

- `on_init()`: called when the plugin initializes in `StartupModule`.
- `on_shutdown()`: called when the plugin shuts down in `ShutdownModule`.
- `on_tick()`: called every frame after plugin initialization. This callback is registered in Editor, PIE, Standalone, packaged game, dedicated server, and listen server runtime contexts.

**Example**:

Create `ue_site.py` under your Python path:

```python
import ue

def on_init():
    print("UnrealPython Plugin Initialized")
    # Put global initialization logic here.

def on_shutdown():
    print("UnrealPython Plugin Shutdown")
    # Put cleanup logic here.

def on_tick():
    pass
```

### Containers, Object Lifetime, and Delegates

The wrapper API exposed by this plugin uses PascalCase method names. It is not fully compatible with the snake_case API of UE's built-in `unreal` module. When migrating scripts, do not simply replace `import unreal` with `import ue`.

Common container methods:

- `Array`: `Append`, `Count`, `Extend`, `Index`, `Insert`, `Pop`, `Clear`, `Remove`, `Reverse`, `Sort`, `Resize`, `Copy`.
- `Set`: `Add`, `Discard`, `Remove`, `Pop`, `Clear`, `Difference`, `DifferenceUpdate`, `Intersection`, `IntersectionUpdate`, `Union`, `Update`, `IsDisjoint`, `IsSubset`, `IsSuperset`.
- `Map`: `Copy`, `Clear`, `FromKeys`, `Get`, `SetDefault`, `Pop`, `PopItem`, `Update`, `Items`, `Keys`, `Values`.

UObject wrappers provide methods such as `IsValid()`, `StaticClass()`, `GetClass()`, `GetWorld()`, `AddPythonOwned()`, `RemovePythonOwned()`, `IsPythonOwned()`, `AddToRoot()`, and `RemoveFromRoot()`. If Python keeps UObject references created by scripts or used across frames, prefer `AddPythonOwned()` / `RemovePythonOwned()` for lifetime management. Check `IsValid()` before accessing properties when the object may have been destroyed by Unreal.

Delegate wrappers support:

- Single-cast delegates: `IsBound()`, `BindFunction(obj, name)`, `Bind(callable)`, `Unbind()`, `Execute(*args)`, `ExecuteIfBound(*args)`.
- Multicast delegates: `AddFunction`, `Add`, `AddFunctionUnique`, `AddUnique`, `RemoveFunction`, `Remove`, `RemoveObject`, `ContainsFunction`, `Contains`, `Clear`, `Broadcast`.

Generated property getters and setters validate the internal UObject state before access. Sparse delegates support lazy resolution. If the UObject has been destroyed by Unreal, the wrapper is marked invalid.

### Generated Type

See [Docs/GeneratedType.md](Docs/GeneratedType.md) for usage instructions and the current support surface for runtime-generated `uclass`, `ustruct`, `uenum`, `uproperty`, and `ufunction`.

For performance-sensitive projects, avoid heavy use of Generated Type, especially in high-frequency tick, bulk object creation, or core combat/simulation paths. Generated Type creates transient reflection types at runtime and routes property/function access through the Python/C++ boundary, reflection calls, argument conversion, the GIL, and Python object lifetime management. This is more expensive than native C++ types, pregenerated wrappers, or stable Blueprint/C++ types. It is best suited for prototyping, small runtime glue types, test helpers, and tooling. Long-lived and high-frequency types should be implemented in C++/Blueprint and exposed through the normal wrapper or export mechanisms.

The Python definition types behind these decorators support CPython cycle GC. `ue.ufunction()` currently supports keyword arguments such as `Params=[...]` and `Ret=...`, as well as the positional decorator argument paths supported by the generator. See [Docs/GeneratedType.md](Docs/GeneratedType.md) for exact parameters and limitations.

### Packaged / Dedicated Server

The UnrealPython Runtime module initializes the Python VM outside commandlets, so packaged games, listen servers, and dedicated servers can all run the `ue_site` entry point and project-side Python scripts. Your project must stage the script directory into the package, for example:

```ini
[/Script/UnrealEd.ProjectPackagingSettings]
+DirectoriesToAlwaysStageAsNonUFS=(Path="Scripts")

[UnrealPython]
ScriptPath=Content/Scripts
```

Dedicated/listen server scripts should distinguish context in Python through `World.GetNetModeName()`, `World.GetNetMode()`, `GameplayStatics.GetGameInstance(world)`, or `world.OwningGameInstance`.

Platform notes:

- Win64 packaged builds require `python314.dll` and `python314.zip` next to the executable.
- Android builds package the Python stdlib, `_android_support.py`, and the current `Content/Scripts` into the APK asset `unrealpython_bootstrap.zip`. At runtime, it is extracted by hash/version. The runtime also attempts to read the packaged script path and scripts inside OBB.
- Mac/iOS runtime dependencies, standard libraries, and bundle resources are collected by `Build.cs`.
- Commandlets skip Python VM initialization to avoid accidentally starting runtime Python during cook or generation flows.

Verified in this project:

- Editor dedicated server: under `UnrealEditor <Project>.uproject <Map> -server -DisablePython`, UnrealPython `on_init()` and `on_tick()` both fire, and `World.GetNetModeName()` returns `DedicatedServer`.
- Mac packaged game: the Game target build phase passes, and the build flow collects UnrealPython's `libpython3.14.dylib` and Python standard library runtime dependencies.
- Packaged dedicated server executable: requires engine distribution support for Server targets. The current UE_5.7 environment reports Server targets as unsupported when building `TargetType.Server`, so a true packaged dedicated server executable could not be verified in that environment.

### Timer Manager

The plugin provides `UUPyTimerManager` for managing timer callbacks from Python.

**Blueprint nodes**:

| Node | Description | Parameters | Return value |
|------|-------------|------------|--------------|
| `SetTimer` | Sets a looping timer | `Rate`: interval in seconds | Timer Handle |
| `SetOnceTimer` | Sets a one-shot timer | `Delay`: delay in seconds | Timer Handle |
| `ClearTimer` | Clears a specific timer | `Handle`: timer handle | - |
| `ClearAllTimers` | Clears all timers | - | - |

**Python example**:

Create `unreal_timer.py` under a Python search path and define `on_timer(handle)`. `UUPyTimerManager` lazily loads and caches `unreal_timer.on_timer`, and releases the cache after all timers are cleared.

```python
def on_timer(handle):
    """Timer callback; receives the timer handle."""
    print(f"Timer {handle} triggered!")

# Set timers from Blueprint or C++ through UUPyTimerManager.
```

**C++ example**:

```cpp
#include "PyGameFramework/UPyTimerManager.h"

// Set a looping timer that fires every 1 second.
int32 TimerHandle = UUPyTimerManager::SetTimer(1.0f);

// Set a one-shot timer that fires after 5 seconds.
int32 OnceTimerHandle = UUPyTimerManager::SetOnceTimer(5.0f);

// Clear a timer.
UUPyTimerManager::ClearTimer(TimerHandle);

// Clear all timers.
UUPyTimerManager::ClearAllTimers();
```

### Editor Tools (UnrealPythonEditor)

The plugin provides helper tools in the Unreal Engine editor.

**Generate Python reflection data**:
- **Menu path**: `Tools` -> `Unreal Python` -> `Generate Python`
- **Function**: scans all registered Native Python modules and methods and exports reflection information, including module names, method names, and documentation, to a JSON file.
- **Output path**: `Plugins/UnrealPython/Tools/ReflectionData/native_module.json`
- **Purpose**: this data is read by the next `GENERATE_UNREAL_PYTHON=1` UHT generation pass and is used to complete Native C API functions on the `ue` module. Full static wrappers and `.pyi` files still require `Tools/GenPy.ps1` or `Tools/GenPy.sh`.

## Notes

- Do not initialize UE's built-in `PythonScriptPlugin` and this plugin's Python runtime in the same process.
- Project scripts should live under `Content/Scripts` and should provide `ue_site.py`; provide `unreal_timer.py` when timer callbacks are needed.
- After changing `Tools/GenerationSettings.json`, `UPyScript*` meta tags, or helper exports, rerun `Tools/GenPy.*` and rebuild.
- The plugin automatically handles conversions for basic types, UObject, UStruct, UEnum, containers, delegates, and FieldPath. For complex object lifetimes, prefer wrapper `IsValid()` checks and Python-owned management.
- Generated wrappers validate object state before property access. If Python stores UObject wrappers across map transitions or object destruction cycles, still check validity before use.

## Related Documentation

- [UnrealPython Quick Start](Docs/TUTORIAL_UnrealPython_Beginner.md): starts from `ue_site.py`, lifecycle callbacks, object access, timers, and cleanup habits.
- [Python Generated Type Guide](Docs/GeneratedType.md): explains `ue.uclass()`, `ue.ustruct()`, `ue.uenum()`, `ue.uproperty()`, and `ue.ufunction()` usage and limitations.
- [GenerationSettings.json Guide](Docs/GenerationSettings.md): documents static wrapper generation scope, member filtering, `ForceReflection*`, `IsInline`, project module output, and verification flow.
- [Python 3.14 Runtime Build and Packaging Flow](Docs/PythonRuntimeBuild.md): records the source, platform directories, build process, and packaging verification for the bundled Python runtime.
- [Differences Between UnrealPython and UE PythonScriptPlugin](Docs/PythonScriptPluginDifferences.md): compares this plugin with UE's built-in `PythonScriptPlugin` in positioning, module structure, and runtime behavior.
