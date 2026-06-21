# UnrealPython 快速入门

下面的代码片段可以复制到使用该插件的项目脚本目录中。默认脚本目录是：

```text
Content/Scripts
```

项目脚本目录建议保留一个最小 `ue_site.py`，用于承接插件级生命周期回调：

```python
def on_init():
    pass


def on_tick():
    pass


def on_shutdown():
    pass
```

## 1. 主入口 game_instance.py

`UUPyGameInstance` 默认加载模块 `game_instance`，调用工厂函数 `create_game_instance(unreal_game_instance)`，然后把生命周期转发给返回的 Python 对象。

新项目可把下面内容保存为：

```text
Content/Scripts/game_instance.py
```

```python
import ue


class MyGameInstance:
    def __init__(self):
        self.unreal_game_instance = None
        self.world = None
        self.elapsed_time = 0.0

    def init(self, unreal_game_instance):
        self.unreal_game_instance = unreal_game_instance
        self.world = unreal_game_instance.GetWorld()
        ue.Log("[game_instance] init: %s" % unreal_game_instance.GetName())
        ue.Log("[game_instance] world: %s" % self.world.GetName())

    def on_start(self):
        ue.Log("[game_instance] on_start")
        actors = ue.GameplayStatics.GetAllActorsOfClass(self.unreal_game_instance, ue.Actor)
        ue.Log("[game_instance] actor count: %d" % len(actors))

    def tick(self, delta_time):
        self.elapsed_time += delta_time

    def shutdown(self):
        ue.Log("[game_instance] shutdown elapsed=%.2f" % self.elapsed_time)
        self.world = None
        self.unreal_game_instance = None


def create_game_instance(unreal_game_instance):
    return MyGameInstance()
```

配置位置：

- Game Instance Class 继承 `UUPyGameInstance`，或使用继承自它的 Blueprint/C++ 类。
- Python 模块名：`game_instance`。
- Factory 函数名：`create_game_instance`。

生命周期顺序：

```text
create_game_instance(unreal_game_instance)
init(unreal_game_instance)
on_start()
tick(delta_time)
shutdown()
```

只有返回对象上存在 `tick` 方法时，`UUPyGameInstance` 才会注册每帧 ticker。`create_game_instance()` 不能返回 `None`。

修改脚本后，最稳妥的验证方式是停止 PIE 后重新开始；如果模块已经被当前 Python VM 导入过，重启 Editor 可以确保重新导入全部脚本。

## 2. 日志

```python
import ue

ue.Log("normal")
ue.LogWarning("warning")
ue.LogError("error")
```

`sys.stdout` 会重定向到 `ue.Log`，`sys.stderr` 会重定向到 `ue.LogError`，所以普通 `print()` 也会进 Unreal 日志。

## 3. 常用值类型

```python
location = ue.Vector(100.0, 0.0, 120.0)
rotation = ue.Rotator()
scale = ue.Vector(1.0, 1.0, 1.0)
transform = ue.Transform(rotation, location, scale)
```

值类型 wrapper 可以作为函数参数、属性值和返回值参与 Unreal 反射调用。给 `Transform` 赋值时，通常按 `Rotator, Location, Scale` 的顺序构造。

也可以写成工具函数：

```python
def make_transform(x=0.0, y=0.0, z=120.0):
    location = ue.Vector(x, y, z)
    rotation = ue.Rotator()
    scale = ue.Vector(1.0, 1.0, 1.0)
    return ue.Transform(rotation, location, scale)
```

## 4. 容器

```python
numbers = ue.Array(int)
numbers.Append(3)
numbers.Append(1)
numbers.Sort()

tags = ue.Set(str)
tags.Add("runtime")

scores = ue.Map(str, int)
scores["kills"] = 2
```

注意：

- 容器类型必须传元素类型，例如 `ue.Array(int)`、`ue.Set(str)`、`ue.Map(str, int)`。
- 不要把 `ue.Array`、`ue.Set`、`ue.Map` 这些类型本身当作 property 类型。
- `Map` 支持类似 dict 的读写，也可以用 `Items()` 遍历。
- 容器 wrapper 是 Unreal 容器，不是普通 Python list/set/dict；传给反射函数时会按目标 Unreal 类型转换。

## 5. World Context 和 Actor 查询

```python
world = unreal_game_instance.GetWorld()
ue.Log(world.GetName())
```

很多 `GameplayStatics` / `KismetSystemLibrary` API 参数名虽然叫 World Context，但可以直接传 `unreal_game_instance`：

```python
actors = ue.GameplayStatics.GetAllActorsOfClass(unreal_game_instance, ue.Actor)
for actor in actors[:10]:
    ue.Log("[actor] %s" % actor.GetName())
```

不要为了游戏运行时逻辑遍历所有 World 来猜上下文。主入口已经给了准确的 `unreal_game_instance`。


## 6. Generated Type 和 UObject 实例

`ue.uenum()`、`ue.ustruct()`、`ue.uclass()` 可以在 Python 里生成 transient reflected type。下面例子先定义 enum、struct 和 UObject 类型：

```python
@ue.uenum()
class MyState(ue.EnumBase):
    Idle = ue.uvalue(0)
    Active = ue.uvalue(1)


@ue.ustruct()
class MyStats(ue.StructBase):
    Health = ue.uproperty(int)
    Location = ue.uproperty(ue.Vector)


@ue.uclass()
class MyData(ue.Object):
    Label = ue.uproperty(str)
    State = ue.uproperty(MyState)
    Stats = ue.uproperty(MyStats)

    @ue.ufunction(Params=[int], Ret=int)
    def Double(self, value):
        return value * 2
```

然后创建这个 UObject 类型的运行时实例：

```python
obj = ue.NewObject(Type=MyData, Name="MyDataRuntime")
obj.AddPythonOwned()
obj.Label = "demo"
obj.State = MyState.Active

stats = MyStats()
stats.Health = 100
stats.Location = ue.Vector(0.0, 0.0, 120.0)
obj.Stats = stats

result = obj.CallMethod("Double", (21,))
```

`AddPythonOwned()` 用于让 Python 持有运行时创建的 UObject，避免对象过早被 Unreal GC 清理。生命周期结束后可调用 `RemovePythonOwned()`。Generated Type 的完整规则见 [GeneratedType.md](GeneratedType.md)。

## 7. 反射 Actor 并生成

```python
class MyActor(ue.Actor):
    HitCount = ue.uproperty(int)

    @ue.ufunction(Params=[int], Ret=int)
    def AddHits(self, value):
        self.HitCount = self.HitCount + value
        return self.HitCount


actor_class = ue.uclass()(MyActor)
```

下面片段适合放在 `MyGameInstance` 的方法里：

```python
world_context = self.unreal_game_instance
transform = make_transform(0.0, 0.0, 120.0)

actor = ue.GameplayStatics.BeginDeferredActorSpawnFromClass(world_context, actor_class, transform)
actor = ue.GameplayStatics.FinishSpawningActor(actor, transform)
```

`ue.uclass()` 生成的是 transient reflected type，不是可保存的 Blueprint asset。类定义变更后，如果当前 Python VM 已经导入过模块，重新开始 PIE 或重启 Editor 再验证最稳。

## 8. 生命周期和清理

Python 变量只是持有 Unreal 对象 wrapper，不等于延长 Unreal 对象本身的生命周期。运行时创建的 UObject、生成出来的 Actor，以及从场景中查到的 Actor，清理方式略有不同。

### Python 创建的 UObject

`ue.NewObject()` 创建的是 Unreal UObject。对象如果只被 Python 局部变量引用，离开作用域后很容易丢失管理入口；如果对象还需要在后续 tick 或回调里使用，创建后建议加入 Python owned 集合：

```python
obj = ue.NewObject(Type=MyData, Name="RuntimeData")
obj.AddPythonOwned()

self.runtime_data = obj
```

生命周期结束时，把它从 Python owned 集合移除，并清掉自己的引用：

```python
if self.runtime_data:
    self.runtime_data.RemovePythonOwned()
    self.runtime_data = None
```

`AddPythonOwned()` 适合“这个 UObject 是 Python 运行时创建，并且需要由 Python 保留一段时间”的场景。临时对象不一定需要加入；加入之后记得在 `shutdown()` 或不再使用时移除。

### 场景里的 Actor

Actor 的生命周期由 World 管理。Python 持有 Actor wrapper 不代表 Actor 还活着；Actor 被关卡卸载、被游戏逻辑销毁或 PIE 结束后，wrapper 仍可能留在 Python 变量里。

```python
if self.spawned_actor and self.spawned_actor.IsValid():
    self.spawned_actor.K2_DestroyActor()

self.spawned_actor = None
```

访问缓存的 Actor 前先判断有效性：

```python
if self.spawned_actor and self.spawned_actor.IsValid():
    self.spawned_actor.CallMethod("AddHits", (1,))
```

经验规则：

- `UObject`：Python 创建且要长期保留时，用 `AddPythonOwned()`；结束时 `RemovePythonOwned()` 并清掉引用。
- `Actor`：用 `K2_DestroyActor()` 请求销毁；销毁后清掉 Python 引用。
- 缓存的 UObject/Actor：使用前先判断对象还是否有效；Actor 至少要检查 `IsValid()`。
- `shutdown()` 里不要再启动新异步逻辑，也不要继续访问已经清理掉的 World/Actor。

## 9. Actor 操作

```python
actor.GetName()
actor.IsValid()
actor.K2_SetActorTransform(transform, False, None, True)
actor.K2_DestroyActor()
actor.CallMethod("AddHits", (3,))
```

`CallMethod()` 可以调用 Python、Blueprint 或 C++ 暴露给反射的方法，参数用 tuple 传入。单参数也要写成 `(3,)`。

Destroy 后的 Actor wrapper 可能仍被 Python 变量持有，访问前先判断：

```python
if actor and actor.IsValid():
    actor.K2_DestroyActor()
```

## 10. 加载 Blueprint 类

```python
enemy_class = ue.LoadClass("/Game/Blueprints/BP_Enemy.BP_Enemy_C")
```

路径拿不准时，在 Content Browser 里右键资源复制引用，再确认它是类路径而不是资源对象路径。Blueprint Actor 类路径通常以 `_C` 结尾。

常见区别：

```python
asset = ue.LoadAsset("/Game/Blueprints/BP_Enemy.BP_Enemy")
klass = ue.LoadClass("/Game/Blueprints/BP_Enemy.BP_Enemy_C")
```

生成 Actor 时需要 class，通常用 `LoadClass()`。

## 11. 控制台命令

```python
ue.KismetSystemLibrary.ExecuteConsoleCommand(unreal_game_instance, "stat fps", None)
```

## 12. 一次跑通

下面片段适合放在 `init()` 或 `on_start()` 中，因为它需要 `unreal_game_instance`：

使用前请把第 3、6、7 节里的 `make_transform`、`MyData`、`MyActor` 定义放在同一个模块中，或改成你自己的类型。

```python
ue.Log("[demo] hello")

numbers = ue.Array(int)
numbers.Append(1)
numbers.Append(2)
ue.Log("[demo] numbers: %s" % [value for value in numbers])

actors = ue.GameplayStatics.GetAllActorsOfClass(unreal_game_instance, ue.Actor)
ue.Log("[demo] actor count: %d" % len(actors))

obj = ue.NewObject(Type=MyData, Name="MyDataRuntime")
obj.AddPythonOwned()
obj.Label = "demo"
ue.Log("[demo] object: %s" % obj.GetName())

actor_class = ue.uclass()(MyActor)
transform = make_transform()
actor = ue.GameplayStatics.BeginDeferredActorSpawnFromClass(unreal_game_instance, actor_class, transform)
actor = ue.GameplayStatics.FinishSpawningActor(actor, transform)
ue.Log("[demo] spawned actor: %s" % actor.GetName())

ue.KismetSystemLibrary.ExecuteConsoleCommand(unreal_game_instance, "stat fps", None)
```

清理生成的 Actor：

```python
if actor and actor.IsValid():
    actor.K2_DestroyActor()

if obj:
    obj.RemovePythonOwned()
```
