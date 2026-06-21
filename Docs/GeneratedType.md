[英文](GeneratedType.md) [中文](GeneratedType.zh.md)

# Python Generated Type Guide

This document explains how to define UnrealPython runtime generated types, including `ue.uclass()`, `ue.ustruct()`, `ue.uenum()`, `ue.uvalue()`, `ue.uproperty()`, `ue.ufunction()`, initialization, reflection helpers, and network semantics.

A Generated Type is a transient reflected type generated from a Python class: `ue.uclass()` generates a `UClass`, `ue.ustruct()` generates a `UScriptStruct`, and `ue.uenum()` generates a `UEnum`. These are not production Blueprint asset replacements. `Blueprintable` is currently only a metadata marker; it does not mean the editor can create a saved Blueprint subclass from a Python class.

## Basic entry points

```python
import ue


@ue.uclass()
class MyActor(ue.Actor):
    pass


@ue.ustruct()
class MyData(ue.StructBase):
    Count = ue.uproperty(int)


@ue.uenum()
class MyState(ue.EnumBase):
    Idle = ue.uvalue(0)
    Running = ue.uvalue(1)
```

`ue.uclass()` generates a `UClass`. The Python class must inherit from a UObject wrapper type, such as `ue.Object`, `ue.Actor`, or `ue.ActorComponent`.

`ue.ustruct()` generates a `UScriptStruct`. The Python class must inherit from `ue.StructBase` or an already generated struct wrapper type.

`ue.uenum()` generates a `UEnum`. The Python class must inherit from `ue.EnumBase`; enum entries are defined with `ue.uvalue()`.

## uclass parameters

```python
@ue.uclass(
    Meta={"DisplayName": "Runtime Actor"},
    BlueprintType=True,
    NotBlueprintType=False,
    Blueprintable=True,
    NotBlueprintable=False,
    Abstract=False,
)
class RuntimeActor(ue.Actor):
    pass
```

| Parameter | Description |
| --- | --- |
| `Meta={...}` | Writes metadata to the generated `UClass`. Keys and values are converted to strings. |
| `BlueprintType=True/False` | Controls whether `BlueprintType` metadata is written. The default is `True` to preserve old behavior. |
| `NotBlueprintType=True` | Writes `NotBlueprintType` metadata. It cannot be used together with `BlueprintType=True`. |
| `Blueprintable=True/False` | Writes `Blueprintable` or `NotBlueprintable` metadata for reflection, debugging, and future editor tools. |
| `NotBlueprintable=True` | Writes `NotBlueprintable` metadata. It cannot be used together with `Blueprintable=True`. |
| `Abstract=True/False` | Sets or clears the actual `CLASS_Abstract` class flag. |

`Meta` must be a `dict` or `None`; all keys and values are converted to strings before being written as metadata. Boolean parameters accept only `None` or `bool`.

`BlueprintType` means the class can be used semantically as a Blueprint variable, parameter, return value, and similar type position.

`Blueprintable` means the class declares that it can be used semantically as a Blueprint parent class. The plugin is not currently integrated with the full Blueprint asset creation, saving, compiling, and restore flow, so this does not imply that the editor can create a Blueprint subclass from a Python class.

## ustruct parameters

```python
@ue.ustruct(
    Meta={"DisplayName": "Runtime Data"},
    BlueprintType=True,
    NotBlueprintType=False,
)
class RuntimeData(ue.StructBase):
    Count = ue.uproperty(int)
```

| Parameter | Description |
| --- | --- |
| `Meta={...}` | Writes metadata to the generated `UScriptStruct`. Keys and values are converted to strings. |
| `BlueprintType=True/False` | Controls whether `BlueprintType` metadata is written. The default is `True` to preserve old behavior. |
| `NotBlueprintType=True` | Writes `NotBlueprintType` metadata. It cannot be used together with `BlueprintType=True`. |

`Meta` must be a `dict` or `None`. `BlueprintType` and `NotBlueprintType` cannot both be `True`.

`Blueprintable` and `Abstract` are class semantics and are not supported on `ue.ustruct()`.

## uenum and uvalue

```python
@ue.uenum()
class DamageType(ue.EnumBase):
    Fire = ue.uvalue(0, Meta={"DisplayName": "Fire"})
    Ice = ue.uvalue(1, Meta={"DisplayName": "Ice"})
```

`ue.uvalue(Val, Meta=None)` defines an enum entry. `Val` cannot be `None`; `Meta` is written to the metadata for that enum value. During generation, entries are sorted by numeric value before being written to the `UEnum`.

`ue.uvalue()` can only be used in `ue.uenum()` types. `ue.uclass()` and `ue.ustruct()` do not support enum values.

## uproperty

```python
class MyActor(ue.Actor):
    Health = ue.uproperty(int)
    Name = ue.uproperty(str, Meta={"DisplayName": "Player Name"})
    Position = ue.uproperty(ue.Vector)
    Items = ue.uproperty(ue.Array(str))
    Scores = ue.uproperty(ue.Map(str, int))
```

Full parameter list:

```python
ue.uproperty(
    Type,
    Meta=None,
    Getter=None,
    Setter=None,
    Replicated=False,
    RepNotify=False,
    ReplicationCondition=None,
    RepNotifyCondition=None,
    PushBased=False,
)
```

`ue.uproperty()` returns a property definition. It participates in generation only when used as a class attribute of a `ue.uclass()` or `ue.ustruct()` type. Generated properties receive `Edit` and `BlueprintVisible` flags by default, and apply the `Meta` dictionary.

Common types:

| Python type expression | UE property |
| --- | --- |
| `bool` | `FBoolProperty` |
| `int` | `FIntProperty` |
| `float` | `FFloatProperty` |
| `str` | `FStrProperty` |
| `ue.Object` or a UObject wrapper subclass | `FObjectProperty` |
| `ue.Actor` or another UObject-derived type | `FObjectProperty`, with the corresponding `UClass` as subclass |
| `ue.Vector`, `ue.Transform`, or another struct wrapper subclass | `FStructProperty` |
| Type generated by `@ue.ustruct()` | `FStructProperty` |
| Type generated by `@ue.uenum()` | `FByteProperty` or `FEnumProperty`, depending on enum form |
| `ue.FieldPath` | `FFieldPathProperty` |
| `ue.Array(T)` | `FArrayProperty` |
| `ue.Set(T)` | `FSetProperty` |
| `ue.Map(K, V)` | `FMapProperty` |
| delegate wrapper type | `FDelegateProperty` |
| multicast delegate wrapper type | `FMulticastDelegateProperty` |

Container types must be passed as instances, such as `ue.Array(int)`. Do not pass `ue.Array`, `ue.Set`, or `ue.Map` directly.

### Getter / Setter

```python
@ue.uclass()
class MyObject(ue.Object):
    Value = ue.uproperty(int, Getter="GetValue", Setter="SetValue")

    @ue.ufunction(Ret=int, Getter=True)
    def GetValue(self):
        return 10

    @ue.ufunction(Params=[int], Setter=True)
    def SetValue(self, value):
        pass
```

`Getter` / `Setter` are property metadata links. Functions can also be marked with `Getter=True` or `Setter=True`.

When a property is bound to a getter or setter, the generator also exposes an internal property with an underscore prefix. For example, `Value` also generates `_Value`. `Value` goes through the getter/setter; `_Value` reads and writes the underlying reflected property directly, which lets getter/setter implementations access their stored value.

A `Getter=True` function must be `Pure=True` and cannot also set `Setter=True`. `Static=True` cannot be used together with `Getter=True` or `Setter=True`.

### Replication / RepNotify

Replication parameters are supported only for `ue.uclass()` generated class properties. They are not supported for `ue.ustruct()` properties.

```python
@ue.uclass()
class RepActor(ue.Actor):
    Count = ue.uproperty(int, RepNotify=True)
    Label = ue.uproperty(
        str,
        RepNotify="OnRepLabel",
        ReplicationCondition="OwnerOnly",
        RepNotifyCondition="Always",
        PushBased=True,
    )

    @ue.ufunction()
    def OnRep_Count(self):
        pass

    @ue.ufunction(Params=[str])
    def OnRepLabel(self, old_value):
        pass
```

`RepNotify=True` uses the default function name `OnRep_<PropertyName>`. `RepNotify="FuncName"` uses an explicit function name.

The RepNotify function must exist, must not be static, must have no return value, and can take at most one parameter. The single-parameter form receives the old value; the parameter type must match the property type.

`ReplicationCondition` supports `None`, `InitialOnly`, `OwnerOnly`, `SkipOwner`, `SimulatedOnly`, `AutonomousOnly`, `SimulatedOrPhysics`, `InitialOrOwner`, `Custom`, `ReplayOrOwner`, `ReplayOnly`, `SimulatedOnlyNoReplay`, `SimulatedOrPhysicsNoReplay`, `SkipReplay`, `Dynamic`, and `Never`; it also accepts the `COND_` prefix.

`RepNotifyCondition` supports `OnChanged` and `Always`; it also accepts the `REPNOTIFY_` prefix.

`RepNotify`, `ReplicationCondition`, and `PushBased` implicitly enable `Replicated`. `RepNotifyCondition` must be used together with `RepNotify`.

## ufunction

```python
@ue.uclass()
class MyObject(ue.Object):
    @ue.ufunction(Params=[int, str], Ret=bool)
    def Check(self, count, name):
        return count > 0 and len(name) > 0
```

Full parameter list:

```python
@ue.ufunction(
    Meta=None,
    Ret=None,
    Params=None,
    Override=False,
    Static=False,
    Pure=False,
    Getter=False,
    Setter=False,
    Server=False,
    Client=False,
    NetMulticast=False,
    Reliable=False,
    Unreliable=False,
)
```

`Params` and `Ret` reuse the same type system as `ue.uproperty()`.

Regular instance methods must keep the Python `self` parameter. `Params` should list only the parameter types after `self`. Functions with `Static=True` do not need `self`. For non-override functions, the number of Python parameters must match the number of entries in `Params`.

Function parameter defaults are converted to matching property default values and written as `CPP_Default_<ParamName>` metadata. The function docstring is written as `ToolTip` metadata.

`Ret=T` generates a regular return value. `Ret=(T1, T2, ...)` generates a `bool ReturnValue` plus multiple out params. Returning `None` from Python means `ReturnValue=False`; returning a single value or tuple means `ReturnValue=True` and writes the out params. When there are multiple out params, the returned tuple length must match.

### Override Blueprint Event

When overriding a UE Blueprint event, do not pass `Params` or `Ret`; pass only `Override=True`.

```python
@ue.uclass()
class TickActor(ue.Actor):
    @ue.ufunction(Override=True)
    def ReceiveBeginPlay(self):
        pass

    @ue.ufunction(Override=True)
    def ReceiveTick(self, delta_seconds):
        pass
```

`Override=True` can only override a Blueprint event that already exists in the parent class. Override functions cannot also set `Static=True`, `Getter=True`, `Setter=True`, `Params`, or `Ret`.

### Static / Pure

```python
@ue.uclass()
class MathLibrary(ue.Object):
    @ue.ufunction(Static=True, Pure=True, Params=[int, int], Ret=int)
    def Add(a, b):
        return a + b
```

`Pure=True` writes Blueprint pure semantics; `Static=True` writes the static function flag.

### RPC

```python
@ue.uclass()
class NetActor(ue.Actor):
    @ue.ufunction(Server=True, Reliable=True)
    def ServerDo(self):
        pass

    @ue.ufunction(Client=True, Unreliable=True)
    def ClientDo(self):
        pass

    @ue.ufunction(NetMulticast=True, Reliable=True)
    def MulticastDo(self):
        pass
```

Only one of `Server`, `Client`, and `NetMulticast` can be selected.

`Reliable` and `Unreliable` cannot both be used, and neither can be used without a network target.

Whether an RPC is actually sent remotely still depends on UE networking rules, such as Actor ownership, NetDriver, connection state, World, and NetMode.

## Initialization

`_post_init(self)` is optional. It is called only when the type itself explicitly defines `_post_init`; an inherited `_post_init` from a wrapper base class is not treated as user initialization. `_post_init` must be callable.

```python
@ue.uclass()
class MyObject(ue.Object):
    Value = ue.uproperty(int)

    def _post_init(self):
        self.Value = 10
```

For `uclass`, `_post_init` is the Python callback after UObject `PostInitInstance`. For `ustruct`, `_post_init` is called when the struct instance is initialized.

## Creating objects and using structs

```python
obj = ue.NewObject(Type=MyObject, Name="MyRuntimeObject")
obj.AddPythonOwned()

data = MyData()
data.Count = 7
obj.SomeStructProperty = data
```

When Python needs to hold a UObject for a long time, call `AddPythonOwned()` to prevent the object from being collected by UE GC. Call `RemovePythonOwned()` when the lifetime ends.

## Reflection helpers

```python
cls = MyActor.StaticClass()
flags = ue.GetClassFlags(MyActor)

if ue.HasTypeMetaData(MyActor, "BlueprintType"):
    display_name = ue.GetTypeMetaData(MyActor, "DisplayName")
```

Common helpers:

| Function | Description |
| --- | --- |
| `Type.StaticClass()` | Gets the `UClass` for a class type. |
| `ue.GetClassFlags(TypeOrClass)` | Gets `UClass::ClassFlags`. |
| `ue.GetFunctionFlags(TypeOrClass, Name)` | Gets `UFunction::FunctionFlags`. |
| `ue.GetPropertyFlags(TypeOrClass, Name)` | Gets `FProperty::PropertyFlags`. |
| `ue.HasTypeMetaData(Type, Key)` | Checks whether the reflected field for a generated Python type has metadata. |
| `ue.GetTypeMetaData(Type, Key)` | Reads metadata from the reflected field for a generated Python type. |
| `ue.GetTypeFromClass(Class)` | Gets the Python type from a `UClass`. |
| `ue.GetTypeFromStruct(Struct)` | Gets the Python type from a `UScriptStruct`. |
| `ue.GetStructFromType(Type)` | Gets the `UScriptStruct` for a Python struct type. |
| `ue.GetTypeFromEnum(Enum)` | Gets the Python type from a `UEnum`. |
| `ue.GetPropertyRepNotifyName(TypeOrClass, Name)` | Gets the RepNotify function name recorded on a property. |
| `ue.GetPropertyReplicationInfo(TypeOrClass, Name)` | Gets generated class replication settings and returns `ReplicationCondition`, `RepNotifyCondition`, and `PushBased`. |
| `ue.GetPropertyLifetimeReplicationInfo(TypeOrClass, Name)` | Queries the final replication configuration from the CDO lifetime replication data. |

## Current limitations

- Generated types are runtime transient types, not saved Blueprint assets.
- `Blueprintable=True` is currently only a metadata marker and does not let the editor directly create a Blueprint subclass from a Python class.
- `ue.ustruct()` does not support functions, only properties.
- `ue.ustruct()` properties do not support replication parameters.
- `ue.uclass()` and `ue.ustruct()` properties cannot override properties with the same name from a parent type.
- A `ue.uclass()` method that overrides a parent method must explicitly use `Override=True`, and can only override Blueprint events.
- `ue.Array`, `ue.Set`, and `ue.Map` must be passed as typed instances, and nested containers are not supported directly.
- `Name` / `Text` currently do not have separate generated property type entry points; string properties use `str`.
- Complex delegate, nested container, soft object/class path, and similar types should be verified against their specific wrapper/conversion paths.
