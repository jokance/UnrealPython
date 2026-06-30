[English](GenerationSettings.md) | 中文

# GenerationSettings.json 配置说明

`Tools/GenerationSettings.json` 用于控制 UnrealPython 的 UHT 生成流程。它主要影响静态 C++ wrapper 的生成范围、成员过滤、访问方式、项目模块输出路径，以及 `ue` 模块 `.pyi` 补全文件的输出位置。

生成流程只会在设置 `GENERATE_UNREAL_PYTHON=1` 时运行。推荐使用插件脚本：

```powershell
powershell -ExecutionPolicy Bypass -File Plugins\UnrealPython\Tools\GenPy.ps1 `
  -EngineDir "D:\Games\UE_5.7\Engine"
```

Mac:

```bash
ENGINE_DIR="/Path/To/UnrealEngine/Engine" Plugins/UnrealPython/Tools/GenPy.sh
```

## 生成产物

一次生成通常会更新这些文件：

- `Tools/ReflectionData/unreal.json`：UHT 导出的 Unreal 反射元数据。
- `Tools/pystubs/ue/__init__.pyi`：`ue` 模块 IDE 补全文件，路径可由 `StubFilePath` 覆盖。
- `Source/UnrealPython/Public/Wrapper/AutoGen`：插件侧静态 C++ wrapper。
- `Source/<GameModule>/Public/UPy/Wrapper/AutoGen`：项目侧静态 C++ wrapper，路径可由 `GameExportPath` 覆盖。

`AutoGen` 目录会在生成前被清空，不要把手写代码放进这些目录。

## 顶层字段

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

| 字段 | 类型 | 说明 |
|------|------|------|
| `StubFilePath` | string | 自定义 `.pyi` 输出路径。相对于插件根目录；为空时默认输出到 `Tools/pystubs/ue/__init__.pyi`。 |
| `GameExportPath` | string | 项目侧 wrapper 输出目录。相对于项目根目录；为空时会按项目主模块推导为 `Source/<GameModule>/Public/UPy/Wrapper/AutoGen`。 |
| `ExportToGameModules` | string[] | 指定哪些 UE 模块的 wrapper 输出到项目侧 `GameExportPath`。常用于第三方插件或项目希望自己编译/注册的模块。 |
| `AutoGenClasses` | object | 要生成静态 wrapper 的 `UClass` 列表及其成员过滤配置。 |
| `AutoGenStructs` | object | 要生成静态 wrapper 的 `UScriptStruct` 列表及其成员过滤配置。 |

JSON 文件里已有的 `CommentStart`、`IncludeProperties` 等说明性字符串只是给人看的注释键，生成器不会读取它们。由于 JSON 不支持真正的注释，新增说明时也应使用这种无副作用字段，或写到本文档中。

## 类型名匹配

`AutoGenClasses` / `AutoGenStructs` 的 key 可以使用导出元数据里的这些名字：

- Python 暴露名，例如 `Actor`、`Vector`。
- UE 原始名，例如 `Actor`、`Vector`。
- C++ native type，例如 `AActor`、`FVector`。

推荐优先使用 Python 暴露名，也就是脚本里通过 `ue.Actor`、`ue.Vector` 访问到的名字。名称匹配区分大小写。如果生成时提示找不到类型，先查看 `Tools/ReflectionData/unreal.json` 中对应类型的 `ClassName`、`StructName`、`EngineName` 和 `NativeTypeName`。

## 类和结构体配置

`AutoGenClasses` 和 `AutoGenStructs` 的成员过滤字段基本一致：

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

| 字段 | 适用范围 | 说明 |
|------|----------|------|
| `IncludeProperties` | class / struct | 属性白名单。非空时只生成列表中的属性。 |
| `ExcludeProperties` | class / struct | 属性黑名单。会在 `IncludeProperties` 之后生效。 |
| `ForceReflectionProperties` | class / struct | 强制属性通过 Unreal 反射访问，而不是直接访问 C++ 成员。 |
| `IncludeMethods` | class / struct | 方法白名单。非空时只生成列表中的方法。 |
| `ExcludeMethods` | class / struct | 方法黑名单。会在 `IncludeMethods` 之后生效。 |
| `ForceReflectionMethods` | class / struct | 强制方法通过 `UFunction` / `ProcessEvent` 调用，而不是直接调用 C++ 方法。 |
| `IsInline` | struct only | 结构体 wrapper 继承 `TUPyWrapperInlineStruct<T>`，适合 `Vector`、`Rotator` 这类小型值类型。 |

成员名可以写 Python 暴露名，也可以写 UHT 元数据里的原始 C++ 名。方法过滤按方法名匹配，不区分重载签名；同名重载会一起被包含或排除。

## Include 和 Exclude 的顺序

生成静态 wrapper 时，过滤顺序是：

1. 如果 `IncludeProperties` / `IncludeMethods` 非空，先只保留白名单里的成员。
2. 再应用 `ExcludeProperties` / `ExcludeMethods` 移除黑名单成员。
3. delegate / multicast delegate 方法不会作为普通方法生成。

例如：

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

最终会生成 `GetActorLocation` 和 `K2_DestroyActor`，不会生成 `SetActorLocation`。

## ForceReflection

默认情况下，生成器会尽量为可直接访问的 public C++ 属性和方法生成直接调用代码。这样性能更好，但某些外部模块、未导出符号、特殊属性或访问限制会导致编译失败或访问不稳定。

这类成员可以放进 `ForceReflectionProperties` 或 `ForceReflectionMethods`：

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

反射路径会在运行时通过 `FindPropertyByName`、`FindFunctionByName`、`GetPropertyValue` / `SetPropertyValue` 或 `ProcessEvent` 访问成员。它通常比直接 C++ 调用慢一些，但对外部模块和访问限制更稳。

经验规则：

- 编译失败提示成员不可访问、符号不可见、API 未导出时，优先尝试 `ForceReflection*`。
- 成员本身不需要暴露给 Python 时，直接用 `Exclude*`。
- 高频调用路径优先保留直接调用；不要为了方便把大量 hot path 方法都强制反射。

## IsInline

`IsInline` 只对 `AutoGenStructs` 生效：

```json
{
  "AutoGenStructs": {
    "Vector": {
      "IsInline": true
    }
  }
}
```

开启后，结构体 wrapper 使用 `TUPyWrapperInlineStruct<T>` 保存值，适合小型、常用、值语义明确的结构体，例如 `Vector`、`Rotator`、`Quat`、`Color`。大型结构体、内部资源复杂或需要引用外部内存语义的结构体，建议保持默认 wrapper。

`IsInline` 只改变 Python wrapper 的存储方式，不负责提供 Unreal 结构体注册入口。静态结构体 wrapper 的生成代码会使用 `TBaseStructure<NativeType>::Get()` 获取 `UScriptStruct`，因此加入 `AutoGenStructs` 的结构体必须能通过 `TBaseStructure` 找到对应反射类型。

UE 自带的一些 `noexport` CoreUObject 结构体没有引擎内置的 `TBaseStructure` 特化，例如 `FVector2f`、`FVector3f`、`FVector4f`。UnrealPython 在 `Source/UnrealPython/Public/Core/UPyConversion.h` 里为这类常用结构体补了注册：

```cpp
IMPLEMENT_TBASE_STRUCTURE(FVector4f, "/Script/CoreUObject.Vector4f")
```

因此 `Vector4f` 可以这样配置：

```json
{
  "AutoGenStructs": {
    "Vector4f": {
      "IsInline": true
    }
  }
}
```

如果新增类似结构体时只改 `GenerationSettings.json`，但没有对应的 `TBaseStructure` 特化，生成阶段可能成功，编译时会因为 `TBaseStructure<...>::Get()` 不存在而失败。遇到这种情况，应先确认结构体的反射路径，再在 `UPyConversion.h` 中补 `IMPLEMENT_TBASE_STRUCTURE`，而不是简单排除依赖它的方法或属性。

## 项目模块输出

`GameExportPath` 相对于项目根目录。例如：

```json
{
  "GameExportPath": "Source/SilverGame/Public/UPy/Wrapper/AutoGen",
  "ExportToGameModules": ["AkAudio"]
}
```

生成器会把项目模块或 `ExportToGameModules` 中的模块输出到：

```text
Source/SilverGame/Public/UPy/Wrapper/AutoGen/<ModuleName>
```

并生成聚合头：

```text
Source/SilverGame/Public/UPy/Wrapper/AutoGen/UPyGameAutoGenWrapper.h
```

项目模块需要依赖 `UnrealPython`，并在模块启动时注册 `InitializeGameAutoGenWrapperTypes()`。完整接入方式见 README 中的“项目模块 C++ 类型导出 (GetOnInitializePythonWrappers)”小节。

注意：

- `GameExportPath` 指向的 `AutoGen` 目录会被清空后重建。
- `ExportToGameModules` 中的名称必须匹配 UHT 元数据里的模块名。
- 如果导出的 wrapper 引用了第三方插件类型，项目模块通常也需要在 `.Build.cs` 中依赖该插件模块。

## StubFilePath 和补全文件

`StubFilePath` 用于覆盖 `.pyi` 输出路径：

```json
{
  "StubFilePath": "Tools/pystubs/ue/__init__.pyi"
}
```

该路径相对于插件根目录。留空时使用默认路径。

`UNREAL_PYTHON_STUB_COMMENTS` 控制是否输出注释：

- 未设置或 `0`：输出无注释版本。
- `1`：输出带注释版本。
- `2`：同时输出带注释版本和 `__init__no_comments.pyi`。

当前 `.pyi` 生成基于完整反射元数据输出；`GenerationSettings.json` 中的 `ExcludeProperties` / `ExcludeMethods` 会参与隐藏部分成员，但 `IncludeProperties` / `IncludeMethods` 主要用于静态 wrapper 生成，不适合作为 stub 裁剪工具。

## 常见场景

只暴露某个类的少量方法：

```json
{
  "AutoGenClasses": {
    "Actor": {
      "IncludeMethods": ["GetActorLocation", "K2_SetActorLocation", "K2_DestroyActor"]
    }
  }
}
```

排除会导致编译问题或不需要暴露的方法：

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

为小型值类型启用 inline wrapper：

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

`Vector4f` 常见于相机相关接口，例如 `CameraComponent.AsymmetricOverscan` 和 `CameraComponent.SetAsymmetricOverscan`。如果要直接生成这些接口，需要同时具备 `Vector4f` 的 `AutoGenStructs` 配置和 `FVector4f` 的 `TBaseStructure` 注册。

把第三方模块 wrapper 输出到项目模块：

```json
{
  "GameExportPath": "Source/SilverGame/Public/UPy/Wrapper/AutoGen",
  "ExportToGameModules": ["AkAudio"],
  "AutoGenClasses": {
    "AkComponent": {}
  }
}
```

## 修改后的验证流程

1. 编辑 `Tools/GenerationSettings.json`。
2. 运行 `Tools/GenPy.ps1` 或 `Tools/GenPy.sh`。
3. 检查生成日志里是否有 `AutoGen class/struct was not found` 警告。
4. 检查 `Tools/ReflectionData/unreal.json`，确认类型名和成员名是否匹配。
5. 重新编译项目。
6. 如果编译失败，按错误类型选择 `Exclude*` 或 `ForceReflection*`。
7. 如果 IDE 补全没有变化，确认 `StubFilePath` 和 `UNREAL_PYTHON_STUB_COMMENTS` 设置。

## 注意事项

- 名称匹配区分大小写。
- `AutoGenClasses` / `AutoGenStructs` 留空时不会生成静态 wrapper。
- 过量导出会增加生成时间、编译时间和二进制体积；优先只导出脚本实际需要的类型。
- `GameExportPath` 应始终指向 `AutoGen` 目录，不要指向包含手写代码的目录。
- 修改该配置后需要重新运行生成流程，并重新编译使用生成 wrapper 的模块。
