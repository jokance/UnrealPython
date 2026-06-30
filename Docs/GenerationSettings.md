English | [中文](GenerationSettings.zh.md)

# GenerationSettings.json Configuration Guide

`Tools/GenerationSettings.json` controls the UnrealPython UHT generation flow. It mainly affects the static C++ wrapper generation scope, member filtering, access mode, project module output path, and the output location of the `ue` module `.pyi` completion file.

The generation flow runs only when `GENERATE_UNREAL_PYTHON=1` is set. The recommended entry point is the plugin script:

```powershell
powershell -ExecutionPolicy Bypass -File Plugins\UnrealPython\Tools\GenPy.ps1 `
  -EngineDir "D:\Games\UE_5.7\Engine"
```

Mac:

```bash
ENGINE_DIR="/Path/To/UnrealEngine/Engine" Plugins/UnrealPython/Tools/GenPy.sh
```

## Generated outputs

A generation pass usually updates these files:

- `Tools/ReflectionData/unreal.json`: Unreal reflection metadata exported by UHT.
- `Tools/pystubs/ue/__init__.pyi`: IDE completion file for the `ue` module; the path can be overridden by `StubFilePath`.
- `Source/UnrealPython/Public/Wrapper/AutoGen`: static C++ wrappers on the plugin side.
- `Source/<GameModule>/Public/UPy/Wrapper/AutoGen`: static C++ wrappers on the project side; the path can be overridden by `GameExportPath`.

The `AutoGen` directory is cleared before generation. Do not put handwritten code in these directories.

## Top-level fields

```json
{
  "StubFilePath": "",
  "GameExportPath": "",
  "ExportToGameModules": ["AkAudio"],
  "AutoGenClasses": {
    "Actor": {}
  },
  "AutoGenStructs": {
    "Vector": {
      "IsInline": true
    }
  }
}
```

| Field | Type | Description |
|------|------|------|
| `StubFilePath` | string | Custom `.pyi` output path. It is relative to the plugin root. When empty, the default is `Tools/pystubs/ue/__init__.pyi`. |
| `GameExportPath` | string | Project-side wrapper output directory. It is relative to the project root. When empty, it is inferred from the main project module as `Source/<GameModule>/Public/UPy/Wrapper/AutoGen`. |
| `ExportToGameModules` | string[] | UE modules whose wrappers should be emitted to the project-side `GameExportPath`. This is commonly used for third-party plugins or modules that the project wants to compile/register itself. |
| `AutoGenClasses` | object | List of `UClass` types to generate static wrappers for, plus their member filtering configuration. |
| `AutoGenStructs` | object | List of `UScriptStruct` types to generate static wrappers for, plus their member filtering configuration. |

Existing explanatory string keys such as `CommentStart` and `IncludeProperties` in the JSON file are human-readable comment keys only; the generator does not read them. Since JSON does not support real comments, add new notes using this kind of no-op field or put them in this document.

## Type name matching

Keys in `AutoGenClasses` / `AutoGenStructs` can use these names from exported metadata:

- Python exposed names, such as `Actor` and `Vector`.
- Original UE names, such as `Actor` and `Vector`.
- C++ native types, such as `AActor` and `FVector`.

Prefer the Python exposed name, meaning the name used from scripts as `ue.Actor` or `ue.Vector`. Name matching is case-sensitive. If generation reports that a type cannot be found, first inspect the type's `ClassName`, `StructName`, `EngineName`, and `NativeTypeName` in `Tools/ReflectionData/unreal.json`.

## Class and struct configuration

`AutoGenClasses` and `AutoGenStructs` use mostly the same member filtering fields:

```json
{
  "AutoGenClasses": {
    "Pawn": {
      "ExcludeMethods": ["IsControlled"],
      "ExcludeProperties": ["RemoteViewPitch"]
    }
  },
  "AutoGenStructs": {
    "Transform": {
      "IsInline": true,
      "ExcludeProperties": ["Rotation", "Translation", "Scale3D"]
    }
  }
}
```

| Field | Scope | Description |
|------|----------|------|
| `IncludeProperties` | class / struct | Property allowlist. When non-empty, only listed properties are generated. |
| `ExcludeProperties` | class / struct | Property denylist. Applied after `IncludeProperties`. |
| `ForceReflectionProperties` | class / struct | Forces properties to be accessed through Unreal reflection instead of direct C++ member access. |
| `IncludeMethods` | class / struct | Method allowlist. When non-empty, only listed methods are generated. |
| `ExcludeMethods` | class / struct | Method denylist. Applied after `IncludeMethods`. |
| `ForceReflectionMethods` | class / struct | Forces methods to be called through `UFunction` / `ProcessEvent` instead of direct C++ calls. |
| `IsInline` | struct only | Makes the struct wrapper inherit from `TUPyWrapperInlineStruct<T>`, which is suitable for small value types such as `Vector` and `Rotator`. |

Member names can be Python exposed names or the original C++ names in UHT metadata. Method filters match by method name, not overload signature; overloads with the same name are included or excluded together.

## Include and Exclude order

When generating static wrappers, filters are applied in this order:

1. If `IncludeProperties` / `IncludeMethods` are non-empty, keep only members in the allowlist.
2. Apply `ExcludeProperties` / `ExcludeMethods` to remove denylisted members.
3. Delegate / multicast delegate methods are not generated as regular methods.

For example:

```json
{
  "AutoGenClasses": {
    "Actor": {
      "IncludeMethods": ["GetActorLocation", "SetActorLocation", "K2_DestroyActor"],
      "ExcludeMethods": ["SetActorLocation"]
    }
  }
}
```

The final output generates `GetActorLocation` and `K2_DestroyActor`, but not `SetActorLocation`.

## ForceReflection

By default, the generator prefers direct C++ access for public properties and methods that can be accessed directly. This is faster, but some external modules, non-exported symbols, special properties, or access restrictions can cause compilation failures or unstable access.

Put these members in `ForceReflectionProperties` or `ForceReflectionMethods`:

```json
{
  "AutoGenClasses": {
    "SkeletalMeshComponent": {
      "ForceReflectionProperties": ["GlobalAnimRateScale", "ClothTeleportMode"]
    },
    "GameplayStatics": {
      "ForceReflectionMethods": ["BlueprintSuggestProjectileVelocity"]
    }
  }
}
```

The reflection path accesses members at runtime through `FindPropertyByName`, `FindFunctionByName`, `GetPropertyValue` / `SetPropertyValue`, or `ProcessEvent`. It is usually slower than direct C++ calls, but more stable for external modules and access restrictions.

Rules of thumb:

- If compilation fails because a member is inaccessible, a symbol is invisible, or an API is not exported, try `ForceReflection*` first.
- If the member does not need to be exposed to Python, use `Exclude*`.
- Keep direct calls for hot paths; do not force a large number of hot path methods through reflection for convenience.

## IsInline

`IsInline` applies only to `AutoGenStructs`:

```json
{
  "AutoGenStructs": {
    "Vector": {
      "IsInline": true
    }
  }
}
```

When enabled, the struct wrapper stores the value with `TUPyWrapperInlineStruct<T>`. This is suitable for small, common, clearly value-semantic structs such as `Vector`, `Rotator`, `Quat`, and `Color`. Large structs, structs with complex internal resources, or structs that need external-memory reference semantics should keep the default wrapper.

`IsInline` changes only the Python wrapper storage mode. It does not provide an Unreal struct registration entry point. Static struct wrapper generated code uses `TBaseStructure<NativeType>::Get()` to obtain the `UScriptStruct`, so a struct added to `AutoGenStructs` must be discoverable through `TBaseStructure`.

Some built-in UE `noexport` CoreUObject structs do not have engine-provided `TBaseStructure` specializations, such as `FVector2f`, `FVector3f`, and `FVector4f`. UnrealPython adds registrations for common structs like these in `Source/UnrealPython/Public/Core/UPyConversion.h`:

```cpp
IMPLEMENT_TBASE_STRUCTURE(FVector4f, "/Script/CoreUObject.Vector4f")
```

Therefore `Vector4f` can be configured like this:

```json
{
  "AutoGenStructs": {
    "Vector4f": {
      "IsInline": true
    }
  }
}
```

If a similar struct is added only to `GenerationSettings.json` without a matching `TBaseStructure` specialization, generation may succeed but compilation can fail because `TBaseStructure<...>::Get()` does not exist. In that case, first confirm the struct reflection path and add `IMPLEMENT_TBASE_STRUCTURE` in `UPyConversion.h` instead of simply excluding methods or properties that depend on it.

## Project module output

`GameExportPath` is relative to the project root. Example:

```json
{
  "GameExportPath": "Source/SilverGame/Public/UPy/Wrapper/AutoGen",
  "ExportToGameModules": ["AkAudio"]
}
```

The generator emits project modules or modules in `ExportToGameModules` to:

```text
Source/SilverGame/Public/UPy/Wrapper/AutoGen/<ModuleName>
```

It also generates the aggregate header:

```text
Source/SilverGame/Public/UPy/Wrapper/AutoGen/UPyGameAutoGenWrapper.h
```

Project modules must depend on `UnrealPython` and register `InitializeGameAutoGenWrapperTypes()` when the module starts. For the complete integration flow, see the "Project module C++ type export (GetOnInitializePythonWrappers)" section in the README.

Notes:

- The `AutoGen` directory pointed to by `GameExportPath` is cleared and recreated.
- Names in `ExportToGameModules` must match module names in UHT metadata.
- If exported wrappers reference third-party plugin types, the project module usually also needs a dependency on that plugin module in `.Build.cs`.

## StubFilePath and completion file

`StubFilePath` overrides the `.pyi` output path:

```json
{
  "StubFilePath": "Tools/pystubs/ue/__init__.pyi"
}
```

The path is relative to the plugin root. When empty, the default path is used.

`UNREAL_PYTHON_STUB_COMMENTS` controls whether comments are emitted:

- Unset or `0`: emit the no-comment version.
- `1`: emit the commented version.
- `2`: emit both the commented version and `__init__no_comments.pyi`.

Current `.pyi` generation is based on complete reflection metadata. `ExcludeProperties` / `ExcludeMethods` from `GenerationSettings.json` participate in hiding some members, but `IncludeProperties` / `IncludeMethods` are mainly for static wrapper generation and are not suitable as a stub trimming tool.

## Common scenarios

Expose only a few methods from a class:

```json
{
  "AutoGenClasses": {
    "Actor": {
      "IncludeMethods": ["GetActorLocation", "K2_SetActorLocation", "K2_DestroyActor"]
    }
  }
}
```

Exclude methods that cause compilation problems or do not need to be exposed:

```json
{
  "AutoGenClasses": {
    "Character": {
      "ExcludeMethods": [
        "ServerMove",
        "ServerMoveDual",
        "ServerMoveOld"
      ]
    }
  }
}
```

Enable inline wrappers for small value types:

```json
{
  "AutoGenStructs": {
    "Vector": {
      "IsInline": true
    },
    "Rotator": {
      "IsInline": true
    },
    "Vector4f": {
      "IsInline": true
    }
  }
}
```

`Vector4f` commonly appears in camera-related APIs, such as `CameraComponent.AsymmetricOverscan` and `CameraComponent.SetAsymmetricOverscan`. To generate these APIs directly, both the `Vector4f` `AutoGenStructs` entry and the `FVector4f` `TBaseStructure` registration are required.

Emit third-party module wrappers to the project module:

```json
{
  "GameExportPath": "Source/SilverGame/Public/UPy/Wrapper/AutoGen",
  "ExportToGameModules": ["AkAudio"],
  "AutoGenClasses": {
    "AkComponent": {}
  }
}
```

## Validation after changes

1. Edit `Tools/GenerationSettings.json`.
2. Run `Tools/GenPy.ps1` or `Tools/GenPy.sh`.
3. Check the generation log for `AutoGen class/struct was not found` warnings.
4. Inspect `Tools/ReflectionData/unreal.json` and confirm that type names and member names match.
5. Rebuild the project.
6. If compilation fails, choose `Exclude*` or `ForceReflection*` based on the error.
7. If IDE completion does not change, check `StubFilePath` and `UNREAL_PYTHON_STUB_COMMENTS`.

## Notes

- Name matching is case-sensitive.
- When `AutoGenClasses` / `AutoGenStructs` are empty, no static wrapper is generated.
- Excessive export increases generation time, compile time, and binary size; prefer exporting only the types actually needed by scripts.
- `GameExportPath` should always point to an `AutoGen` directory, not a directory that contains handwritten code.
- After changing this configuration, rerun the generation flow and rebuild every module that uses the generated wrappers.
