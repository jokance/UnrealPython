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

## 已验证路径

当前项目包含以下 smoke test：

- `Content/Scripts/upy_test/upy_generated_post_init_optional_repro.py`
- `Content/Scripts/upy_test/upy_generated_type_matrix_repro.py`
- `Content/Scripts/upy_test/upy_generated_actor_runtime_repro.py`
- `Content/Scripts/upy_test/upy_generated_actor_regen_repro.py`
- `Content/Scripts/upy_test/upy_generated_actor_event_repro.py`
- `Content/Scripts/upy_test/upy_world_netmode_repro.py`

这些测试已在 Editor 中以 `-DisablePython` 禁用 UE 内置 Python 后运行通过。

## 当前限制

- 生成类型是运行时 transient 类型，不是编辑器生产级 Blueprint asset 替代。
- `ue.Array`、`ue.Set`、`ue.Map` 只能作为带子类型的实例使用。
- `Name` / `Text` 的显式 generated property 类型当前没有单独 wrapper type 入口；字符串属性使用 `str`。
- 复杂 delegate、nested container、soft object/class path 等类型需要按具体 wrapper/转换路径单独验证后再进入支持矩阵。
