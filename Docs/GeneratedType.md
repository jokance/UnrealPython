# Python Generated Type 使用说明

本文集中说明 UnrealPython 的运行时生成类型的写法，包括 `ue.uclass()`、`ue.ustruct()`、`ue.uenum()`、`ue.uvalue()`、`ue.uproperty()`、`ue.ufunction()`、初始化、反射查询和网络语义。

Generated Type 是由 Python class 生成的 transient reflected type：`ue.uclass()` 生成 `UClass`，`ue.ustruct()` 生成 `UScriptStruct`，`ue.uenum()` 生成 `UEnum`。它们不是编辑器生产级 Blueprint asset 替代；`Blueprintable` 当前只是 metadata 语义标记，不表示编辑器已经可以基于 Python 类创建可保存的 Blueprint 子类。

## 基本入口

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

`ue.uclass()` 用于生成 `UClass`，Python 类必须继承 UObject wrapper 类型，例如 `ue.Object`、`ue.Actor`、`ue.ActorComponent`。

`ue.ustruct()` 用于生成 `UScriptStruct`，Python 类必须继承 `ue.StructBase` 或已生成的 struct wrapper 类型。

`ue.uenum()` 用于生成 `UEnum`，Python 类必须继承 `ue.EnumBase`，枚举值用 `ue.uvalue()` 定义。

## uclass 参数

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

| 参数 | 说明 |
| --- | --- |
| `Meta={...}` | 写入生成 `UClass` metadata。key/value 会转为字符串。 |
| `BlueprintType=True/False` | 控制是否写入 `BlueprintType` metadata。默认是 `True`，保持旧行为。 |
| `NotBlueprintType=True` | 写入 `NotBlueprintType` metadata。不能和 `BlueprintType=True` 同时使用。 |
| `Blueprintable=True/False` | 写入 `Blueprintable` 或 `NotBlueprintable` metadata，供反射、调试和后续 editor 工具读取。 |
| `NotBlueprintable=True` | 写入 `NotBlueprintable` metadata。不能和 `Blueprintable=True` 同时使用。 |
| `Abstract=True/False` | 设置或清除实际 `CLASS_Abstract` class flag。 |

`Meta` 必须是 `dict` 或 `None`，key/value 都会转成字符串写入 metadata。布尔参数都接受 `None` 或 `bool`。

`BlueprintType` 表示这个 class 可作为 Blueprint 变量、参数、返回值等类型语义使用。

`Blueprintable` 表示这个 class 声明自己可作为 Blueprint 父类的语义，但当前插件没有完整接入 Blueprint asset 创建、保存、编译和恢复流程，所以不能据此认为编辑器可以新建 Blueprint 继承 Python 类。

## ustruct 参数

```python
@ue.ustruct(
    Meta={"DisplayName": "Runtime Data"},
    BlueprintType=True,
    NotBlueprintType=False,
)
class RuntimeData(ue.StructBase):
    Count = ue.uproperty(int)
```

| 参数 | 说明 |
| --- | --- |
| `Meta={...}` | 写入生成 `UScriptStruct` metadata。key/value 会转为字符串。 |
| `BlueprintType=True/False` | 控制是否写入 `BlueprintType` metadata。默认是 `True`，保持旧行为。 |
| `NotBlueprintType=True` | 写入 `NotBlueprintType` metadata。不能和 `BlueprintType=True` 同时使用。 |

`Meta` 必须是 `dict` 或 `None`。`BlueprintType` 和 `NotBlueprintType` 不能同时为 `True`。

`Blueprintable` 和 `Abstract` 是 class 语义，不支持在 `ue.ustruct()` 上使用。

## uenum 和 uvalue

```python
@ue.uenum()
class DamageType(ue.EnumBase):
    Fire = ue.uvalue(0, Meta={"DisplayName": "Fire"})
    Ice = ue.uvalue(1, Meta={"DisplayName": "Ice"})
```

`ue.uvalue(Val, Meta=None)` 用于定义 enum entry。`Val` 不能是 `None`；`Meta` 会写到对应枚举值 metadata。生成时 enum entry 会按数值排序后写入 `UEnum`。

`ue.uvalue()` 只能用于 `ue.uenum()` 类型；`ue.uclass()` 和 `ue.ustruct()` 不支持 enum value。

## uproperty

```python
class MyActor(ue.Actor):
    Health = ue.uproperty(int)
    Name = ue.uproperty(str, Meta={"DisplayName": "Player Name"})
    Position = ue.uproperty(ue.Vector)
    Items = ue.uproperty(ue.Array(str))
    Scores = ue.uproperty(ue.Map(str, int))
```

完整参数：

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

`ue.uproperty()` 返回 property definition，只有作为 `ue.uclass()` 或 `ue.ustruct()` 的 class attribute 时才会参与生成。生成后的属性默认带 `Edit` 和 `BlueprintVisible` flags，并应用 `Meta` 字典。

常用类型：

| Python type 表达式 | UE property |
| --- | --- |
| `bool` | `FBoolProperty` |
| `int` | `FIntProperty` |
| `float` | `FFloatProperty` |
| `str` | `FStrProperty` |
| `ue.Object` 或 UObject wrapper 子类 | `FObjectProperty` |
| `ue.Actor` 等 UObject 派生类型 | `FObjectProperty`，subclass 为对应 `UClass` |
| `ue.Vector`、`ue.Transform` 等 struct wrapper 子类 | `FStructProperty` |
| `@ue.ustruct()` 生成类型 | `FStructProperty` |
| `@ue.uenum()` 生成类型 | `FByteProperty` 或 `FEnumProperty`，取决于 enum form |
| `ue.FieldPath` | `FFieldPathProperty` |
| `ue.Array(T)` | `FArrayProperty` |
| `ue.Set(T)` | `FSetProperty` |
| `ue.Map(K, V)` | `FMapProperty` |
| delegate wrapper type | `FDelegateProperty` |
| multicast delegate wrapper type | `FMulticastDelegateProperty` |

容器类型必须传实例，例如 `ue.Array(int)`，不能直接传 `ue.Array`、`ue.Set`、`ue.Map`。

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

`Getter` / `Setter` 是 property metadata 连接。函数侧也可以用 `Getter=True`、`Setter=True` 标记函数语义。

当属性绑定 getter 或 setter 时，生成器还会额外暴露一个带下划线前缀的内部属性，例如 `Value` 会同时生成 `_Value`。`Value` 走 getter/setter；`_Value` 直接读写底层 reflected property，方便 getter/setter 自己访问存储值。

`Getter=True` 的函数必须是 `Pure=True`，不能同时设置 `Setter=True`。`Static=True` 不能和 `Getter=True` 或 `Setter=True` 同时使用。

### Replication / RepNotify

Replication 参数只支持 `ue.uclass()` 生成的 class property，不支持 `ue.ustruct()` property。

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

`RepNotify=True` 使用默认函数名 `OnRep_<PropertyName>`。`RepNotify="FuncName"` 使用显式函数名。

RepNotify 函数必须存在、不能是 static、不能有返回值，参数最多一个；单参数形式用于接收 old value，参数类型必须与属性类型一致。

`ReplicationCondition` 支持 `None`、`InitialOnly`、`OwnerOnly`、`SkipOwner`、`SimulatedOnly`、`AutonomousOnly`、`SimulatedOrPhysics`、`InitialOrOwner`、`Custom`、`ReplayOrOwner`、`ReplayOnly`、`SimulatedOnlyNoReplay`、`SimulatedOrPhysicsNoReplay`、`SkipReplay`、`Dynamic`、`Never`，也接受 `COND_` 前缀。

`RepNotifyCondition` 支持 `OnChanged`、`Always`，也接受 `REPNOTIFY_` 前缀。

`RepNotify`、`ReplicationCondition`、`PushBased` 会隐式打开 `Replicated`。`RepNotifyCondition` 必须和 `RepNotify` 一起使用。

## ufunction

```python
@ue.uclass()
class MyObject(ue.Object):
    @ue.ufunction(Params=[int, str], Ret=bool)
    def Check(self, count, name):
        return count > 0 and len(name) > 0
```

完整参数：

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

`Params` 和 `Ret` 复用 `ue.uproperty()` 的类型系统。

普通实例方法需要保留 Python 的 `self` 参数，`Params` 只填写 `self` 之后的参数类型；`Static=True` 函数不需要 `self`。非 override 函数要求 Python 参数数量和 `Params` 数量一致。

函数参数默认值会被转换成对应 property 默认值并写入 `CPP_Default_<ParamName>` metadata；函数 docstring 会写入 `ToolTip` metadata。

`Ret=T` 生成普通返回值。`Ret=(T1, T2, ...)` 会生成一个 `bool ReturnValue` 和多个 out params：Python 返回 `None` 表示 `ReturnValue=False`；返回单个值或 tuple 会表示 `ReturnValue=True` 并写入 out params。多个 out params 时必须返回长度匹配的 tuple。

### Override Blueprint Event

Override UE Blueprint event 时不要传 `Params` 或 `Ret`，只传 `Override=True`。

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

`Override=True` 只能覆盖父类里已有的 Blueprint event。override 函数不能同时设置 `Static=True`、`Getter=True`、`Setter=True`、`Params` 或 `Ret`。

### Static / Pure

```python
@ue.uclass()
class MathLibrary(ue.Object):
    @ue.ufunction(Static=True, Pure=True, Params=[int, int], Ret=int)
    def Add(a, b):
        return a + b
```

`Pure=True` 写入 Blueprint pure 语义；`Static=True` 写入 static function flag。

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

`Server`、`Client`、`NetMulticast` 三者只能选一个。

`Reliable` 和 `Unreliable` 不能同时使用，也不能脱离网络目标单独使用。

RPC 是否实际远程发送仍依赖 UE 网络规则，例如 Actor ownership、NetDriver、连接状态、World 和 NetMode。

## 初始化

`_post_init(self)` 是可选的。只有类型自身显式定义 `_post_init` 时才会调用；继承自 wrapper 基类的 `_post_init` 不会被当作用户初始化函数。`_post_init` 必须可调用。

```python
@ue.uclass()
class MyObject(ue.Object):
    Value = ue.uproperty(int)

    def _post_init(self):
        self.Value = 10
```

`uclass` 的 `_post_init` 对应 UObject `PostInitInstance` 之后的 Python 回调。`ustruct` 的 `_post_init` 会在 struct 实例初始化时调用。

## 创建对象和使用 struct

```python
obj = ue.NewObject(Type=MyObject, Name="MyRuntimeObject")
obj.AddPythonOwned()

data = MyData()
data.Count = 7
obj.SomeStructProperty = data
```

需要让 Python 长期持有 UObject 时，可以调用 `AddPythonOwned()`，避免对象被 UE GC 回收。生命周期结束后可调用 `RemovePythonOwned()`。

## 反射辅助

```python
cls = MyActor.StaticClass()
flags = ue.GetClassFlags(MyActor)

if ue.HasTypeMetaData(MyActor, "BlueprintType"):
    display_name = ue.GetTypeMetaData(MyActor, "DisplayName")
```

当前常用 helper：

| 函数 | 说明 |
| --- | --- |
| `Type.StaticClass()` | 获取 class 对应 `UClass`。 |
| `ue.GetClassFlags(TypeOrClass)` | 获取 `UClass::ClassFlags`。 |
| `ue.GetFunctionFlags(TypeOrClass, Name)` | 获取 `UFunction::FunctionFlags`。 |
| `ue.GetPropertyFlags(TypeOrClass, Name)` | 获取 `FProperty::PropertyFlags`。 |
| `ue.HasTypeMetaData(Type, Key)` | 判断生成 Python 类型对应的 reflected field 是否有 metadata。 |
| `ue.GetTypeMetaData(Type, Key)` | 读取生成 Python 类型对应的 reflected field metadata。 |
| `ue.GetTypeFromClass(Class)` | 从 `UClass` 找回 Python type。 |
| `ue.GetTypeFromStruct(Struct)` | 从 `UScriptStruct` 找回 Python type。 |
| `ue.GetStructFromType(Type)` | 从 Python struct type 找到对应 `UScriptStruct`。 |
| `ue.GetTypeFromEnum(Enum)` | 从 `UEnum` 找回 Python type。 |
| `ue.GetPropertyRepNotifyName(TypeOrClass, Name)` | 获取 property 上记录的 RepNotify 函数名。 |
| `ue.GetPropertyReplicationInfo(TypeOrClass, Name)` | 获取 Generated Class 记录的复制配置，返回 `ReplicationCondition`、`RepNotifyCondition`、`PushBased`。 |
| `ue.GetPropertyLifetimeReplicationInfo(TypeOrClass, Name)` | 从 CDO 的 lifetime replication 数据中查询最终复制配置。 |

## 当前限制

- 生成类型是 runtime transient type，不是可保存的 Blueprint asset。
- `Blueprintable=True` 当前只是 metadata 语义标记，不能让编辑器直接新建 Blueprint 继承 Python 类。
- `ue.ustruct()` 不支持函数，只支持 property。
- `ue.ustruct()` property 不支持 replication 参数。
- `ue.uclass()` 和 `ue.ustruct()` 的 property 不能覆盖父类型中已有同名 property。
- `ue.uclass()` 的 method 覆盖父类型方法时必须显式使用 `Override=True`，且只能覆盖 Blueprint event。
- `ue.Array`、`ue.Set`、`ue.Map` 必须传带子类型的实例，且不支持直接嵌套容器。
- `Name` / `Text` 当前没有单独 generated property 类型入口；字符串属性使用 `str`。
- 复杂 delegate、nested container、soft object/class path 等类型需要按具体 wrapper/转换路径单独验证。
