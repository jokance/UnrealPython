# Generated Type 支持矩阵

本文记录 `ue.uclass()`、`ue.ustruct()`、`ue.uenum()`、`ue.uproperty()`、`ue.ufunction()` 当前可用的运行时生成类型范围。

## 装饰器入口

```python
@ue.uclass()
class MyObject(ue.Object):
    pass

@ue.ustruct()
class MyStruct(ue.StructBase):
    pass

@ue.uenum()
class MyEnum(ue.EnumBase):
    Value = ue.uvalue(0)
```

`_post_init(self)` 是可选的。只有类型自身显式定义 `_post_init` 时才会调用；继承自 wrapper 基类的 `_post_init` 不会被当作用户初始化函数。

## `ue.uproperty(Type)` 支持

| Python type 表达式 | UE property |
| --- | --- |
| `bool` | `FBoolProperty` |
| `int` | `FIntProperty` |
| `float` | `FFloatProperty` |
| `str` | `FStrProperty` |
| `ue.Object` 或任意 UObject wrapper 子类 | `FObjectProperty` |
| `ue.Actor` 等 UObject 派生类型 | `FObjectProperty`，subclass 为对应 `UClass` |
| `ue.Vector`、`ue.Transform` 等 UStruct wrapper 子类 | `FStructProperty` |
| `@ue.ustruct()` 生成的 struct | `FStructProperty` |
| `@ue.uenum()` 生成的 enum | `FByteProperty` 或 `FEnumProperty`，取决于 enum form |
| `ue.FieldPath` | `FFieldPathProperty` |
| `ue.Array(T)` | `FArrayProperty` |
| `ue.Set(T)` | `FSetProperty` |
| `ue.Map(K, V)` | `FMapProperty` |
| delegate wrapper type | `FDelegateProperty` |
| multicast delegate wrapper type | `FMulticastDelegateProperty` |

容器必须传实例，例如 `ue.Array(int)`；不能直接传 `ue.Array`、`ue.Set`、`ue.Map`。

`ue.uproperty(..., Replicated=True)` 会把生成 class property 标记为 `CPF_Net`，进入 UE replication 的 `ClassReps`。`Replicated` / `RepNotify` / replication strategy 参数只支持 `ue.uclass()` 生成的 class property，不支持 `ue.ustruct()` property。

RepNotify 可传 `True` 使用默认函数名 `OnRep_<PropertyName>`，也可传字符串指定函数名：

```python
@ue.uclass()
class MyActor(ue.Actor):
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

`RepNotify`、`ReplicationCondition`、`PushBased` 会隐式打开 `Replicated`。RepNotify 函数必须存在、不能是 static、不能有返回值，参数最多一个；单参数形式用于接收 old value，参数类型必须与属性类型一致。

`ReplicationCondition` 支持字符串：`None`、`InitialOnly`、`OwnerOnly`、`SkipOwner`、`SimulatedOnly`、`AutonomousOnly`、`SimulatedOrPhysics`、`InitialOrOwner`、`Custom`、`ReplayOrOwner`、`ReplayOnly`、`SimulatedOnlyNoReplay`、`SimulatedOrPhysicsNoReplay`、`SkipReplay`、`Dynamic`、`Never`，也接受带 `COND_` 前缀的名字。`RepNotifyCondition` 支持 `OnChanged` 和 `Always`，也接受 `REPNOTIFY_` 前缀。

## `ue.ufunction(Params=[...], Ret=...)` 支持

`Params` 和 `Ret` 复用 `uproperty` 的类型系统。常用可用组合：

```python
@ue.ufunction(Params=[int, float, bool, str], Ret=int)
def Compute(self, count, scale, enabled, name):
    return count

@ue.ufunction(Params=[ue.Vector, ue.Array(int), ue.Map(str, int)], Ret=int)
def UseContainers(self, location, values, lookup):
    return len(values) + len(lookup)
```

Override UE Blueprint event 时不要传 `Params` 或 `Ret`，直接使用 `Override=True`：

```python
@ue.ufunction(Override=True)
def ReceiveTick(self, delta_seconds):
    pass
```

网络 RPC 反射 flags 可通过 `Server=True`、`Client=True`、`NetMulticast=True`、`Reliable=True`、`Unreliable=True` 指定：

```python
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

`Server`、`Client`、`NetMulticast` 三者只能选一个；`Reliable` 和 `Unreliable` 不能同时使用，也不能脱离网络目标单独使用。RPC 调度语义仍依赖 Actor ownership、NetDriver、连接状态等 UE 网络条件。

在 Editor `-game` listen server/client 实测中，`ue.uclass()` 生成 Actor 已验证可跨进程复制 Actor、复制 `RepNotify` 属性，并执行 `NetMulticast` RPC。Python 调 Unreal net function 时不会套用 editor script execution guard，避免 `AActor::GetFunctionCallspace()` 被强制短路为本地调用。

## 已验证路径

当前项目包含以下 smoke test：

- `Content/Scripts/upy_test/upy_generated_post_init_optional_repro.py`
- `Content/Scripts/upy_test/upy_generated_type_matrix_repro.py`
- `Content/Scripts/upy_test/upy_generated_actor_runtime_repro.py`
- `Content/Scripts/upy_test/upy_generated_actor_regen_repro.py`
- `Content/Scripts/upy_test/upy_generated_actor_event_repro.py`
- `Content/Scripts/upy_test/upy_world_netmode_repro.py`
- `Content/Scripts/upy_test/upy_reload_workflow_repro.py`
- `Content/Scripts/upy_test/upy_network_semantics_repro.py`
- `Content/Scripts/upy_test/upy_network_runtime_repro.py`

这些测试已在 Editor 中以 `-DisablePython` 禁用 UE 内置 Python 后运行通过。

## 当前限制

- 生成类型是运行时 transient 类型，不是编辑器生产级 Blueprint asset 替代。
- `ue.Array`、`ue.Set`、`ue.Map` 只能作为带子类型的实例使用。
- `Name` / `Text` 的显式 generated property 类型当前没有单独 wrapper type 入口；字符串属性使用 `str`。
- 复杂 delegate、nested container、soft object/class path 等类型需要按具体 wrapper/转换路径单独验证后再进入支持矩阵。
- 网络语义扩展当前覆盖 `Replicated=True`、RepNotify metadata、lifetime condition、RepNotify condition、PushBased、RPC target、reliability，并已验证 listen server/client 下的 Actor replication、RepNotify 和 `NetMulticast` RPC。单播 `Server` / `Client` RPC 仍需按 Actor ownership、NetDriver、连接状态等 UE 网络条件单独验证；主动 dirty 标记 API 和多 world 行为 smoke 还未补齐。
