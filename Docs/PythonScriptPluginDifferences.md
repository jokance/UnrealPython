[英文](PythonScriptPluginDifferences.md) [中文](PythonScriptPluginDifferences.zh.md)

# Source Differences Between UnrealPython and UE PythonScriptPlugin

This document compares:

- This plugin: `Plugins/UnrealPython`
- UE built-in plugin: `Engine/Plugins/Experimental/PythonScriptPlugin`; the current workspace path is `UnrealEngine/Engine/Plugins/Experimental/PythonScriptPlugin`

Conclusion first: `UnrealPython` is not a lightly renamed copy of `PythonScriptPlugin`. It keeps some ideas around CPython embedding, the GIL, UObject/UStruct/UEnum wrappers, and type conversion, but reorganizes them for runtime game scripting. UE's built-in plugin mainly serves editor automation. This plugin is designed to work in packaged games and to provide Python bindings that can be statically generated, trimmed, and completed by IDEs.

## 1. Plugin positioning and loading phase

The main module in UE's built-in `PythonScriptPlugin.uplugin` is `UncookedOnly`, with an early preload module named `PythonScriptPluginPreload`. Its description is `Python integration for the Unreal Editor`; it is disabled by default and primarily targets Editor/Program use.

`UnrealPython.uplugin` changes this to:

- `UnrealPython`: `Runtime` module, `LoadingPhase=Default`
- `UnrealPythonEditor`: separate `Editor` module
- No `PythonScriptPluginPreload`

The target scenario changed: this plugin needs to initialize Python at game runtime, not only run Python automation in the editor or command-line tools. Editor menus and code generation are moved to `UnrealPythonEditor` so the runtime module does not directly depend on large editor systems.

## 2. Python runtime source

UE's built-in plugin uses UE's bundled Python through the engine `Python3` module. It also contains editor support resources under `Content/Python`, such as pip helpers, remote execution scripts, and test scripts.

This plugin does not depend on the `Python3` module in `Source/UnrealPython/UnrealPython.Build.cs`. Instead, it directly integrates `ThirdParty/python314`:

- Win64 links `python314.lib` and copies `python314.dll`.
- Android links `libpython3.14.a` plus static dependencies such as OpenSSL, bz2, ffi, lzma, zstd, and mpdec.
- Mac links and copies `libpython3.14.dylib` and a trimmed `python3.14` standard library.
- iOS links `Python.xcframework` and packages the `python` and `Frameworks` directories from `Runtime/IOSDevice` or `Runtime/IOSSimulator` into the app bundle.

This avoids coupling to UE's built-in Python ABI, symbols, `sys.path`, and version lifecycle. The plugin currently targets Python 3.14, while UE's built-in plugin is tied to the engine version. Mixing them, especially on packaged platforms, easily causes dynamic library, standard library path, or extension module conflicts. `Docs/PythonRuntimeBuild.md` records the Python 3.14 runtime build and packaging details separately.

## 3. Initialization flow

UE's built-in plugin initializes through `FPythonScriptPlugin` and supports:

- `-EnablePython`, `-DisablePython`, `-ForceEnablePython`
- enable/disable overrides from user settings
- commandlet disable lists
- Python and PythonREPL console executors
- startup scripts, remote execution, pip install, Content Browser integration, online doc generation, and related editor features
- the `unreal` module and editor script execution context

This plugin splits initialization into smaller runtime components:

- `FUPyVirtualMachine`: handles `Py_PreInitialize`, `Py_InitializeFromConfig`, `sys.path`, `sys.argv`, and GIL main-thread save/restore.
- `FUPyModuleInitializer`: creates the `ue` module, registers built-in types, and registers auto-generated types.
- `UUPyManager`: a UObject singleton that manages lifecycle, UObject deletion notifications, output redirection, and `ue_site` lifecycle callbacks.
- `FUnrealPythonModule`: module entry point that starts/stops the manager, and in editor builds additionally registers a console executor and ToolMenus command handler.

The runtime plugin needs a more deterministic lifecycle: initialize at game startup and release on shutdown. It does not need editor enablement policy, pip tasks, remote execution services, or documentation commands. The smaller flow also makes initialization paths easier to control on mobile and packaged platforms.

## 4. `sys.path` and script entry

UE's built-in plugin organizes the editor Python environment around project setting `AdditionalPaths`, startup scripts, pip site-packages, and engine Python content paths.

This plugin explicitly sets search paths in isolated mode in `UPyVirtualMachine::ConfigureSearchPaths`:

- Adds bundled standard library paths on Mac/iOS.
- Adds `Content/Scripts` by default.
- Lets `[UnrealPython] ScriptPath` in `Game.ini` override the script directory.
- Lets `UUPySettings::AdditionalPaths` append project-relative paths.
- In non-editor builds, no longer copies `Content/Scripts` or `Content/Settings` to an external writable path; scripts and configuration must enter the package through stage/package.

Packaged content paths differ from editor source paths, and mobile platforms especially cannot assume Python can read project source directories. This plugin constrains script paths to project content and plugin runtime directories so they can work after Cook/Package.

## 5. Python module name and exposed API

UE's built-in plugin exposes the main module as `unreal`.

This plugin exposes the main module as `ue`. `FUPyModuleInitializer` creates a custom module type `UPyUEModule` and overrides `tp_getattro`: when an attribute is not in the module dictionary, it uses `StaticFindAllObjects` to find a `UClass`, `UScriptStruct`, or `UEnum` by name, then gets the matching Python type from `FUPyWrapperTypeRegistry` and caches it in the module dictionary.

The result:

- Types such as `ue.Actor` and `ue.Vector` can be resolved on demand.
- The plugin does not need to generate every reflected type at module initialization.
- Runtime scripts can use a shorter module name that fits game-side scripting.

This also means the plugin is not fully compatible with UE's built-in `unreal` API. Migration requires more than changing the import name; type, function, container, and lifecycle semantics also need review.

## 6. Binding generation strategy

This is the largest implementation difference.

UE's built-in plugin mainly generates wrapper types dynamically from reflection at runtime. Its source includes `PyWrapperTypeRegistry`, `PyGenUtil`, `PyWrapperObject/Struct/Enum`, Blueprint type generation, reinstancing, editor object cleanup, and related editor-oriented logic. The focus is broad coverage of editor-visible reflection objects and Blueprint types.

This plugin keeps a dynamic wrapper foundation, but adds a UHT generation pipeline:

- `Source/UnrealPythonUhtGenerator` is the UHT exporter.
- Generation runs only when `GENERATE_UNREAL_PYTHON` is set.
- `UnrealPythonCodeGenerator` exports reflection JSON and `.pyi` files.
- `AutoWrapperGenerator` generates C++ wrapper code from `Tools/GenerationSettings.json`.
- Generated code is written to `Public/Wrapper/AutoGen`.
- `UPyAutoGenWrapper.h` aggregates generated type initialization.
- `FUPyWrapperTypeRegistry::RegisterAutoWrappedClassCreateFunc` / `RegisterAutoWrappedStructCreateFunc` register creation functions before types are created in one pass.

The reason is runtime performance and packageability. Dynamic reflection generation is flexible, but startup cost, editor dependency, and platform behavior are harder to control. Static C++ wrappers are compiled ahead of time, do not depend on editor generation logic in packaged builds, and can be precisely trimmed by `GenerationSettings.json`. `.pyi` generation improves IDE completion and type hints.

## 7. Wrapper type system differences

Both plugins share the same high-level wrapper idea: a Python `PyTypeObject` represents a UE reflected type, and a Python wrapper instance holds the actual UObject, UScriptStruct, container, or delegate data. `PyGenUtil` / `UPyGenUtil` stores metadata for properties, functions, operators, and call paths.

This plugin is not a simple `PyWrapper*` to `UPyWrapper*` rename. The wrapper layer was changed in several important ways.

### 7.1 File and type layout

UE's built-in wrappers mostly live under `PythonScriptPlugin/Private`, including `PyWrapperObject`, `PyWrapperStruct`, `PyWrapperEnum`, `PyWrapperArray`, `PyWrapperFixedArray`, `PyWrapperSet`, `PyWrapperMap`, `PyWrapperDelegate`, `PyWrapperFieldPath`, `PyWrapperName`, `PyWrapperText`, `PyWrapperMath`, and `PyWrapperTypeRegistry`.

This plugin splits wrappers into `Public/Wrapper` and `Private/Wrapper`, and adds `Public/Wrapper/AutoGen`. Core files include `UPyWrapperObjectBase`, `UPyWrapperObject`, `UPyWrapperStruct`, `UPyWrapperEnum`, container/delegate wrappers, `UPyWrapperTypeFactory`, `UPyWrapperTypeRegistry`, and generated `UPyWrapper*.cpp/.h` files.

Generated C++ wrappers and game modules need to include wrapper APIs. UE's built-in plugin hides most implementation in Private, which suits internal dynamic generation. This plugin promotes stable wrapper interfaces to Public so UHT-generated code, project-module wrappers, and helper export libraries can reuse them.

### 7.2 Object wrapper split

UE's built-in plugin uses `FPyWrapperObject` as both the UObject wrapper base and the concrete `PyWrapperObjectType` implementation.

This plugin splits it into two layers:

- `FUPyWrapperObjectBase`: stores `TObjectPtr<UObject> ObjectInstance` and implements generic property access, UFunction calls, getter/setter handling, and dynamic method calls.
- `FUPyWrapperObject`: derives from `FUPyWrapperObjectBase` and acts as the default wrapper type for regular UObjects.

Auto-generated class wrappers need a clean object base. Generated wrappers for types such as `Actor`, `Widget`, and `GameInstance` can reuse `FUPyWrapperObjectBase`, while the default `UPyWrapperObjectType` remains the fallback UObject wrapper.

### 7.3 Static AutoGen wrappers

UE's built-in `FPyWrapperTypeRegistry::GenerateWrappedClassType/StructType/EnumType` constructs `PyTypeObject`s at runtime, fills `tp_methods`, `tp_getset`, and metadata, and registers them in the module. It also supports Blueprint-generated types, reinstancing, deprecated aliases, and package reload workflows.

This plugin keeps dynamic paths such as `GenerateWrappedClassType/StructType/EnumType`, but the main path is static generation:

- UHT generates C++ types such as `UPyWrapperActor`, `UPyWrapperVector`, and `UPyWrapperUserWidget` according to `GenerationSettings.json`.
- Each generated wrapper provides a creation function such as `DoInitWrapperActor`.
- `InitializeUPyWrapperActor` registers creation functions through `RegisterAutoWrappedClassCreateFunc`.
- `FUPyWrapperTypeRegistry::StartCreateAutoWrappedTypes` creates wrapper types in inheritance order.

This reduces runtime reflection scanning and dynamic assembly cost. Generated wrappers are normal C++ code, participate in compile-time checks, and can be trimmed precisely.

### 7.4 TypeRegistry keys and responsibilities

UE's built-in `FPyWrapperTypeRegistry` often uses `FSoftObjectPath` as a registration key because it must handle Blueprint assets, rename/reload, deprecated type names, and editor package state.

This plugin's `FUPyWrapperTypeRegistry` uses reflection pointers more directly:

- `TMap<const UClass*, const PyTypeObject*> PythonWrappedClasses`
- `TMap<const UScriptStruct*, const PyTypeObject*> PythonWrappedStructs`
- `TMap<const UEnum*, const PyTypeObject*> PythonWrappedEnums`
- `TMap<const UFunction*, const PyTypeObject*> PythonWrappedDelegates`
- reverse maps from `PyTypeObject*` to `UClass/UScriptStruct/UEnum/UFunction`
- `AutoWrappedClassCreateFuncs` and `AutoWrappedStructCreateFuncs`

Runtime type sets are more stable, so direct `UClass*` / `UScriptStruct*` lookup is simpler and faster. Name lookup is handled by the `ue` module first, then by the registry.

### 7.5 Wrapper factory changes

UE's built-in plugin has wrapper factory abstractions in `PyWrapperTypeRegistry.h`, including factories for object, struct, delegate, multicast delegate, name, text, array, fixed array, set, map, and field path wrappers.

This plugin adapts that design for runtime use:

- Factories are moved to the public header `UPyWrapperTypeFactory.h`.
- `TUPyWrapperTypeFactory` simplifies `MappedInstances` from a `(WrapperKey, PyTypeObject*)` compound key to `UnrealType -> PythonType*`.
- `FUPyWrapperObjectFactory` uses `FUPyWrapperObjectBase` as the Python type, matching the object wrapper split.
- Dedicated `FName` and `FText` factories are removed; these types mainly follow string conversion semantics.
- Multicast delegate factories include property address and `FMulticastDelegateProperty` parameters for runtime delegate binding.
- `FUPyWrapperObjectFactory` tracks `MappedOwnedPyProps` to clean Python property wrappers owned by UObjects.

The goal is clearer ownership and wrapper reuse at runtime. When a UObject is deleted, `UUPyManager::NotifyUObjectDeleted` can remove mappings and clean Python-owned properties so Python does not keep accessing destroyed objects.

### 7.6 Concrete wrapper behavior

UE's built-in plugin sets dedicated metadata on base wrapper types during initialization. For example, `InitializePyWrapperObject`, `InitializePyWrapperStruct`, and `InitializePyWrapperEnum` create static metadata after `PyType_Ready`, then call functions such as `FPyWrapperObjectMetaData::SetMetaData(&PyWrapperObjectType, &MetaData)`. That metadata stores reflected type pointers, Python property/function maps, deprecation information, and reference collection logic.

This plugin keeps `FUPyWrapperBaseMetaData` and `_wrapper_meta_data` helpers, but base Object/Struct/Enum wrapper initialization generally no longer attaches dedicated metadata to the base type. Generated wrappers register directly with `FUPyWrapperTypeRegistry`, and property/function information is stored in generated descriptors such as `UPyGenUtil::FGeneratedWrappedProperty` and `FGeneratedWrappedFunction`.

Instance tracking is also different. UE's built-in `FPyWrapperBase::New/Free` registers each wrapper with `FPyReferenceCollector` for editor Python and UObject GC integration. This plugin comments out that path and relies more on `UUPyManager`, wrapper factory mappings, UObject deletion notifications, and the Python-owned object set. It also does not keep the built-in plugin's `UPythonObjectHandle`, which stores arbitrary Python objects on Python-generated UPROPERTY values.

Object wrapper organization changed:

- `UPyWrapperObjectBase.cpp`: object instance storage, property access, function calls, getter/setter handling, and validity checks.
- `UPyWrapperObject.cpp`: thin default `Object` type.
- `Subclassing/UPyGeneratedClass.cpp`: Python-generated UE class logic.
- `MethodDefs/UPyMethodDefsObject.cpp`: object methods exposed to Python.

The plugin also exposes runtime ownership/rooting helpers such as `AddPythonOwned`, `RemovePythonOwned`, `IsPythonOwned`, `AddToRoot`, and `RemoveFromRoot`. UE's built-in plugin is more editor-API oriented and commonly exposes snake_case methods such as `get_editor_property`, `set_editor_property`, and `call_method`.

Struct wrappers are narrower in the base layer. UE's built-in `PyWrapperStruct` exposes many dynamic helpers such as `cast`, `static_struct`, `copy`, `assign`, `to_tuple`, `get_editor_property`, `set_editor_property`, `export_text`, and `import_text`. This plugin's base `UPyWrapperStruct` mainly keeps `StaticStruct` and `_post_init`; properties, functions, and operators usually come from AutoGen wrapper code.

Enum wrappers are also trimmed. UE's built-in `PyWrapperEnum` keeps methods such as `cast`, `static_enum`, and `get_display_name`, plus deprecated enum value warnings. This plugin mostly keeps enum entry names, values, iteration, and member access.

Container and delegate wrappers remain conceptually close to the built-in plugin, but method names and exposed sets differ:

- Array methods are PascalCase: `Append`, `Count`, `Extend`, `Index`, `Insert`, `Pop`, `Clear`, `Remove`, `Reverse`, `Sort`, `Resize`.
- Set methods are PascalCase: `Add`, `Discard`, `Remove`, `Pop`, `Clear`, `Difference`, `Update`, `IsDisJoint`, `IsSubset`, `IsSuperset`.
- Map methods are PascalCase: `Copy`, `Clear`, `FromKeys`, `Get`, `SetDefault`, `Pop`, `PopItem`, `Update`, `Items`, `Keys`, `Values`.
- Delegate methods are PascalCase: `BindFunction`, `Bind`, `Unbind`, `Execute`, `ExecuteIfBound`.
- Multicast delegate methods are PascalCase: `AddFunction`, `Add`, `Remove`, `Contains`, `Broadcast`.

The plugin does not aim for one-to-one compatibility with the built-in `unreal` module API. Migrating scripts usually requires container and delegate call-site updates, not only `import unreal` to `import ue`.

### 7.7 Struct wrappers and inline structs

UE's built-in plugin supports regular struct wrappers and special handling for built-in math/name/text types through systems such as `PyWrapperMath`, `PyWrapperName`, and `PyWrapperText`.

This plugin removes separate `UPyWrapperName`, `UPyWrapperText`, and `UPyWrapperMath` files. Instead:

- `UPyConversion` maps `FName` and `FText` mainly to Python string semantics.
- Generated wrappers such as `AutoGen/CoreUObject/Struct/UPyWrapperVector`, `UPyWrapperRotator`, and `UPyWrapperTransform` represent math types.
- `GenerationSettings.json` `IsInline` controls whether small structs use inline storage to reduce heap allocation.

After static generation, math types no longer need to be maintained in one handwritten `PyWrapperMath` file. Each exported struct can have its own generated code, properties, methods, operators, and constants.

### 7.8 MethodDefs and wrapper decoupling

UE's built-in plugin often organizes Python methods directly inside wrapper type files, with editor compatibility logic mixed into core modules.

This plugin adds `Private/MethodDefs` and splits Python method definitions into files such as:

- `UPyMethodDefsUE.cpp`
- `UPyMethodDefsObject.cpp`
- `UPyMethodDefsStruct.cpp`
- `UPyMethodDefsArray.cpp`
- `UPyMethodDefsMap.cpp`
- `UPyMethodDefsSet.cpp`
- `UPyMethodDefsDelegate.cpp`
- `UPyMethodDefsMulticastDelegate.cpp`

This separates wrapper instance storage/basic behavior from the method set exposed to Python. Static generated wrappers can reuse the base wrapper and generate different `PyMethodDef`, `PyGetSetDef`, and operator slots by type.

### 7.9 Python subclass generation remains, but is more runtime-oriented

UE's built-in plugin has `UPythonGeneratedClass/Struct/Enum` for generating UE types from Python types, serving editor scripts and Blueprint-related workflows.

This plugin keeps similar capability under `Subclassing`:

- `UUPyGeneratedClass`
- `UUPyGeneratedStruct`
- `UUPyGeneratedEnum`

These still use `UPyWrapperObjectBase`, `UPyWrapperStruct`, and `UPyWrapperEnum` to inspect Python types and generate UE types. The difference is that the main export path is now "static wrappers for native UE types plus Python subclass generation when needed", rather than editor-time dynamic wrapper generation as the primary route.

### 7.10 Script API impact

Wrapper differences show up in Python scripts:

- The exposed module is `ue`, not `unreal`.
- The stable default surface is the generated types selected by `GenerationSettings.json` plus a small number of dynamically found types.
- Math types, UMG, Engine, and common CoreUObject members come from generated wrappers and `UPyScript*` helpers; they are not identical to UE's `unreal.Vector` or `unreal.Object` behavior.
- `FName` and `FText` are closer to Python string conversion than independent wrapper types.
- UObject lifetime matters more at runtime; Python-held objects should use `AddPythonOwned`, `RemovePythonOwned`, `IsValid`, and related helpers carefully.

Therefore wrapper compatibility is one of the most likely migration pitfalls. Do not only replace `import unreal` with `import ue`; also check type construction, property names, method names, operators, container return values, and object lifetimes.

## 8. Type export metadata

UE's built-in plugin uses Epic's script export metadata conventions, such as `ScriptMethod`, `ScriptConstant`, `ScriptOperator`, and `ScriptName`.

This plugin's UHT generator uses additional `UPy`-prefixed metadata:

- `UPyScriptMethod`
- `UPyScriptMethodSelfReturn`
- `UPyScriptMethodMutable`
- `UPyScriptOperator`
- `UPyScriptConstant`
- `UPyScriptHost`
- `UPyScriptStatic`
- `UPyUseHelperMethod`

`UPyEditorScriptExportHelperLibrary` contains many editor-only helper functions used for generation. `UPyRuntimeScriptExportHelperLibrary` contains `UPyUseHelperMethod` implementations that are actually called at runtime.

The prefix keeps this plugin's export semantics separate from the engine Python plugin's metadata. `UPyUseHelperMethod` also clearly distinguishes generation-only helper functions from runtime helper functions, which matters for packaging: runtime helpers cannot be compiled out under `WITH_EDITOR`.

## 9. Wrapper coverage

UE's built-in plugin has broader editor-script coverage, including `PyEditor`, `PySlate`, online docs, command menus, Content Browser Python file execution, and similar features.

This plugin focuses on game runtime and adds or strengthens:

- `PyGameFramework/UPyGameInstance`: creates a Python object for each UE GameInstance from a configurable factory function and calls `init`, `on_start`, `tick`, and `shutdown`.
- `PyGameFramework/UPyTimerManager`.
- `Helper/UPyBlueprintLibrary`: Blueprint/C++ calls into Python functions.
- Auto-generated wrappers for UMG, Slate, SlateCore, InputCore, Engine, CoreUObject, and related runtime types.
- `UPyRuntimeScriptExportHelperLibrary`: runtime helpers for APIs that are common but not suitable for direct native export.
- Python-owned object sets, UObject deletion listeners, and editor object replacement handling in `UUPyManager`.

The goal is to let Python participate directly in game logic and UI logic, not only execute editor commands. `UPyGameInstance` provides a simple game-side entry point, and `BlueprintLibrary` lowers the cost of calling Python from Blueprint or C++.

## 10. Removed or reduced built-in capabilities

This plugin does not keep, or does not fully keep, these built-in plugin capabilities:

- `IPythonScriptPlugin` public interface
- `PythonScriptCommandlet`
- `PythonOnlineDocsCommandlet`
- `PipInstall` and pip requirements pipeline
- `PythonScriptRemoteExecution`
- `EditorPythonScriptingLibrary`
- Content Browser Python file integration
- content resources such as `debugpy_unreal.py`, `remote_execution.py`, and test scripts
- full result capture/evaluation logic of the Python REPL executor

These features are valuable for editor automation, but they bring dependencies such as `Analytics`, `AssetRegistry`, `DesktopPlatform`, `Networking`, `ContentBrowser*`, `KismetCompiler`, and pip tooling. For a game runtime plugin, they increase packaging complexity and do not fit mobile platforms well.

## 11. Editor feature changes

UE's built-in plugin has a full editor feature set: menus, console, REPL, file execution, remote execution, Content Browser integration, startup script progress reporting, and more.

`UnrealPythonEditor` currently has one core job: add `Unreal Python -> Generate Python` under `LevelEditor.MainMenu.Tools`, and call `FUPyCodeGenerator::GenerateAllCode()` to export `Tools/ReflectionData/native_module.json`.

In editor builds, the runtime module also registers an `UnrealPython` console executor and ToolMenus string command handler, but the implementation is lightweight and directly calls `PyRun_SimpleString`.

The editor is treated as a generation and debugging entry point, not as a full Python automation platform.

## 12. Runtime objects and GC handling

UE's built-in plugin has `PyReferenceCollector`, editor object cleanup, package reload handling, Blueprint reinstancing, and other complex logic for an editor where objects and Blueprints reload frequently.

This plugin keeps the object wrapper and some reinstancing foundation, but emphasizes runtime UObject lifetime:

- `UUPyManager` registers with `GUObjectArray` deletion notifications.
- When a UObject is deleted, Python wrapper mappings are removed.
- `PythonOwnedObjects` prevents Python-held objects from being collected too early by UE GC.

At game runtime, objects are often destroyed by level transitions, Actor/Widget lifecycles, or game logic. If a Python wrapper keeps referencing a destroyed UObject, it becomes a dangling reference. Runtime management therefore focuses strongly on object ownership.

## 13. Additional implementation details that are easy to miss

Console executor registration moved. UE's built-in plugin registers two executors in the `PythonScriptPlugin` module: `Python` and `PythonREPL`. `Python` supports script/file execution; `PythonREPL` supports single-statement evaluation and result display. This plugin's `FUnrealPythonModule` is a runtime module, but console executor code is wrapped in `WITH_EDITOR`; it registers only one `UnrealPython` executor in editor builds. It has no REPL executor, file execution, completion, or captured evaluation result, and simply runs `PyRun_SimpleString` while holding the GIL.

The `ue` module initialization order was rearranged. `FUPyModuleInitializer::InitializeModules` initializes `__main__`, creates the custom `ue` module, then registers core, wrapper, and AutoGen types. The built-in plugin initializes around the `unreal` module and has responsibilities split across `PyCore`, `PyEngine`, `PyEditor`, `PySlate`, and related systems. This plugin's `ue` module also writes `BUILD_SHIPPING` and `GIsEditor`, and lazily finds native `UClass/UScriptStruct/UEnum` through `tp_getattro`, explicitly skipping `BlueprintGeneratedClass`, `UserDefinedStruct`, and `UserDefinedEnum` types.

The Python VM configuration is stricter. The built-in plugin's isolated mode, AdditionalPaths, StartupScripts, pip, remote execution, developer mode, and related settings come from `UPythonScriptPluginSettings` and user settings. This plugin's `FUPyVirtualMachine` currently hard-codes an isolated interpreter, `parse_argv=0`, `utf8_mode=1`, `use_environment=0`, and `module_search_paths_set=1`; it then manually adds plugin runtime paths, `Content/Scripts`, and `UUPySettings::AdditionalPaths`. It fills `sys.argv` from the raw UE command line but does not keep the built-in plugin's startup script queue.

Settings are heavily trimmed. UE's built-in `UPythonScriptPluginSettings` is `config=Engine` and has per-user settings for enablement, developer mode, type hints, pip, remote execution, Content Browser integration, and more. This plugin's `UUPySettings` is `config=Game` and currently keeps only `AdditionalPaths`; the main script path is read separately from `[UnrealPython] ScriptPath` in `Game.ini`.

Output and lifecycle hooks changed. After initialization, this plugin replaces `sys.stdout` and `sys.stderr` with redirectors that call `ue.Log` and `ue.LogError`, then calls `ue_site.on_init()`. On shutdown it calls `ue_site.on_shutdown()`. In the editor ticker it calls `ue_site.on_tick()` every frame. The built-in plugin relies more on its execution context, log capture, startup scripts, and editor callbacks.

Module function naming and coverage differ. This plugin's `ue` module functions use PascalCase, such as `Log`, `NewObject`, `FindObject`, `LoadObject`, `LoadClass`, `FindAsset`, `CollectGarbage`, `GetTypeFromClass`, and `GetContentDir`. The built-in `unreal` module exposes many snake_case functions and wrapper methods. This plugin keeps decorator entry points such as `uclass`, `ustruct`, `uenum`, `uproperty`, and `ufunction`, but does not preserve the built-in plugin's module loading, object reference cleanup, localized text helpers, shutdown callbacks, and related APIs as compatibility shims.

The type conversion layer has a different visibility and target. UE's built-in `PyConversion` is primarily a Private internal tool. This plugin puts `UPyConversion` in Public and adds project-needed `TBaseStructure` specializations such as `FVector2f`, `FVector3f`, `FVector4f`, `FTimespan`, and `FARFilter`. C++ game code, Blueprint helpers, and AutoGen wrappers can reuse it directly, but that also makes conversion APIs part of this plugin's public ABI.

C++/Blueprint calls into Python also differ. UE's built-in plugin exposes editor-script-oriented `PythonScriptLibrary`, K2 nodes, command execution, and file execution. This plugin adds `UUPyBlueprintLibrary`, with a small number of BlueprintCallable fixed-signature functions and a C++ variadic template `CallPythonMethod` that uses `UPyConversion` for parameters and return values. This is a bridge from game code to Python logic, not an editor scripting platform.

Build dependencies and docstring policy differ. This plugin does not depend on `Python3` and links `ThirdParty/python314` directly. Its runtime module also depends on game-side modules such as `UMG`, `InputCore`, `NetCore`, and `FieldNotification`, and defines `WITH_UPY_DOC_STRINGS=1` only in editor builds. UE's built-in plugin depends on `Python3`, `Analytics`, `Networking`, `Json`, `DesktopPlatform`, `ContentBrowser*`, `KismetCompiler`, and other editor automation modules.

Taken together, these details show that the plugin is not only a trimmed UE plugin. Much of the state that lived in `FPythonScriptPlugin`, `FPyWrapper*MetaData`, and editor settings moved into `UUPyManager`, `FUPyVirtualMachine`, `FUPyModuleInitializer`, `FUPyWrapperTypeRegistry`, AutoGen descriptors, and project script `ue_site`.

## 14. Documentation and maintenance advice

For future maintenance, do not sync entire files directly from UE's built-in `PythonScriptPlugin`. The goals have diverged:

- For CPython API, GIL, type conversion, and container wrapper fixes, the corresponding built-in plugin files are useful references.
- For initialization, `.Build.cs`, UE module references, script paths, packaged resources, UHT generation, and `UPy` metadata, follow this plugin's current structure first.
- Before porting a built-in plugin feature, decide whether it is an editor automation feature or a runtime scripting feature.
- Prefer `Tools/GenerationSettings.json` and the UHT generation pipeline when adding exported types. Do not casually expand dynamic reflection generation scope.
- `UPyUseHelperMethod` implementations must stay in runtime-compilable code and cannot live only under `WITH_EDITOR`.

In short: UE's built-in plugin answers "how to run Python in the editor"; this plugin answers "how to embed Python in a packaged game and let Python call UE types." That goal difference explains most of the source changes.
