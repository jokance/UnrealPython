# Python Subclassing 使用说明

本文集中说明 UnrealPython 运行时生成类型的写法，包括 `ue.uclass()`、`ue.ustruct()`、`ue.uenum()`、`ue.uproperty()`、`ue.ufunction()`、初始化、热更新和网络语义。

这些类型是运行时 transient reflected type，适合游戏运行时脚本、PIE、standalone、listen server、dedicated server 和 packaged runtime。它们不是编辑器生产级 Blueprint asset 替代；`Blueprintable` 当前只是 metadata 语义标记，不表示编辑器已经可以基于 Python 类创建可保存的 Blueprint 子类。

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

`Blueprintable` 和 `Abstract` 是 class 语义，不支持在 `ue.ustruct()` 上使用。

## uenum 和 uvalue

```python
@ue.uenum()
class DamageType(ue.EnumBase):
    Fire = ue.uvalue(0, Meta={"DisplayName": "Fire"})
    Ice = ue.uvalue(1, Meta={"DisplayName": "Ice"})
```

`ue.uvalue(Val, Meta=None)` 用于定义 enum entry。`Meta` 会写到对应枚举值 metadata。

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

常用类型：

| Python type 表达式 | UE property |
| --- | --- |
| `bool` | `FBoolProperty` |
| `int` | `FIntProperty` |
| `float` | `FFloatProperty` |
| `str` | `FStrProperty` |
| `ue.Object` 或 UObject wrapper 子类 | `FObjectProperty` |
| `ue.Vector`、`ue.Transform` 等 struct wrapper 子类 | `FStructProperty` |
| `@ue.ustruct()` 生成类型 | `FStructProperty` |
| `@ue.uenum()` 生成类型 | enum property |
| `ue.Array(T)` | `FArrayProperty` |
| `ue.Set(T)` | `FSetProperty` |
| `ue.Map(K, V)` | `FMapProperty` |

容器类型必须传实例，例如 `ue.Array(int)`，不能直接传 `ue.Array`。

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

`ReplicationCondition` 支持 `None`、`InitialOnly`、`OwnerOnly`、`SkipOwner`、`SimulatedOnly`、`AutonomousOnly`、`SimulatedOrPhysics`、`InitialOrOwner`、`Custom`、`ReplayOrOwner`、`ReplayOnly`、`SimulatedOnlyNoReplay`、`SimulatedOrPhysicsNoReplay`、`SkipReplay`、`Dynamic`、`Never`，也接受 `COND_` 前缀。

`RepNotifyCondition` 支持 `OnChanged`、`Always`，也接受 `REPNOTIFY_` 前缀。

`RepNotify`、`ReplicationCondition`、`PushBased` 会隐式打开 `Replicated`。

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

`_post_init(self)` 是可选的。只有类型自身显式定义 `_post_init` 时才会调用；继承自 wrapper 基类的 `_post_init` 不会被当作用户初始化函数。

```python
@ue.uclass()
class MyObject(ue.Object):
    Value = ue.uproperty(int)

    def _post_init(self):
        self.Value = 10
```

`uclass` 的 `_post_init` 对应对象初始化后的 Python 回调。`ustruct` 的 `_post_init` 会在 struct 实例初始化时调用。

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
| `ue.HasTypeMetaData(Type, Key)` | 判断生成 Python 类型对应的 reflected field 是否有 metadata。 |
| `ue.GetTypeMetaData(Type, Key)` | 读取生成 Python 类型对应的 reflected field metadata。 |
| `ue.GetTypeFromClass(Class)` | 从 `UClass` 找回 Python type。 |
| `ue.GetTypeFromStruct(Struct)` | 从 `UScriptStruct` 找回 Python type。 |
| `ue.GetTypeFromEnum(Enum)` | 从 `UEnum` 找回 Python type。 |

## Reload / 热更新

推荐使用 `ue.ReloadModule(module_or_name)`：

```python
import my_gameplay
import ue

my_gameplay = ue.ReloadModule(my_gameplay)
# 或：
my_gameplay = ue.ReloadModule("my_gameplay")
```

`ue.ReloadModule()` 内部会调用 `importlib.reload()`，并自动 flush generated type reinstancing。

如果手动使用 `importlib.reload()` 或动态生成多个同名 `uclass` / `ustruct`，可以调用：

```python
ue.FlushGeneratedTypeReinstancing()
```

## Runtime / PIE / Dedicated Server

UnrealPython 的 runtime 模块会在非 commandlet 运行时初始化 Python VM。Editor、PIE、Standalone、packaged game、listen server、dedicated server 都可以执行 `ue_site` 入口和游戏侧 Python 脚本。

Dedicated server 和 listen server 下，Python 脚本需要按 UE 运行时语义区分：

```python
world = self.GetWorld()
net_mode = world.GetNetMode()
game_instance = world.GetGameInstance()
```

不要把同一个 Python 脚本假设为只运行在 editor world 或单 world。PIE、多 client、dedicated server 都可能存在多个 world/context。

## 当前限制

- 生成类型是 runtime transient type，不是可保存的 Blueprint asset。
- `Blueprintable=True` 当前只是 metadata 语义标记，不能让编辑器直接新建 Blueprint 继承 Python 类。
- `ue.ustruct()` 不支持函数，只支持 property。
- `ue.ustruct()` property 不支持 replication 参数。
- `ue.Array`、`ue.Set`、`ue.Map` 必须传带子类型的实例，且不支持直接嵌套容器。
- `Name` / `Text` 当前没有单独 generated property 类型入口；字符串属性使用 `str`。
- 复杂 delegate、nested container、soft object/class path 等类型需要按具体 wrapper/转换路径单独验证。

## 更多验证信息

当前支持矩阵、已验证 smoke test 和类型覆盖范围见 [GeneratedTypeSupportMatrix.md](GeneratedTypeSupportMatrix.md)。
