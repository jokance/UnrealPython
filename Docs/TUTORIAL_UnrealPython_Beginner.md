[英文](TUTORIAL_UnrealPython_Beginner.md) [中文](TUTORIAL_UnrealPython_Beginner.zh.md)

# UnrealPython Quick Start

The following snippets can be copied into the project script directory of a project that uses this plugin. The default script directory is:

```text
Content/Scripts
```

It is recommended to keep a minimal `ue_site.py` in the project script directory for plugin-level lifecycle callbacks:

```python
def on_init():
    pass


def on_tick():
    pass


def on_shutdown():
    pass
```

## 1. Main entry point: game_instance.py

`UUPyGameInstance` loads the `game_instance` module by default, calls the factory function `create_game_instance(unreal_game_instance)`, and forwards lifecycle events to the returned Python object.

For a new project, save the following content as:

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

Configuration:

- Game Instance Class should inherit from `UUPyGameInstance`, or use a Blueprint/C++ class derived from it.
- Python module name: `game_instance`.
- Factory function name: `create_game_instance`.

Lifecycle order:

```text
create_game_instance(unreal_game_instance)
init(unreal_game_instance)
on_start()
tick(delta_time)
shutdown()
```

`UUPyGameInstance` registers a per-frame ticker only when the returned object has a `tick` method. `create_game_instance()` cannot return `None`.

After changing scripts, the most reliable verification flow is to stop PIE and start it again. If the module has already been imported by the current Python VM, restart the editor to ensure all scripts are imported from scratch.

## 2. Logging

```python
import ue

ue.Log("normal")
ue.LogWarning("warning")
ue.LogError("error")
```

`sys.stdout` is redirected to `ue.Log`, and `sys.stderr` is redirected to `ue.LogError`, so regular `print()` output also goes to the Unreal log.

## 3. Common value types

```python
location = ue.Vector(100.0, 0.0, 120.0)
rotation = ue.Rotator()
scale = ue.Vector(1.0, 1.0, 1.0)
transform = ue.Transform(rotation, location, scale)
```

Value type wrappers can be used as function parameters, property values, and return values in Unreal reflection calls. When assigning a `Transform`, construct it in the usual `Rotator, Location, Scale` order.

You can also write a helper:

```python
def make_transform(x=0.0, y=0.0, z=120.0):
    location = ue.Vector(x, y, z)
    rotation = ue.Rotator()
    scale = ue.Vector(1.0, 1.0, 1.0)
    return ue.Transform(rotation, location, scale)
```

## 4. Containers

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

Notes:

- Container types must be passed element types, such as `ue.Array(int)`, `ue.Set(str)`, and `ue.Map(str, int)`.
- Do not use the `ue.Array`, `ue.Set`, or `ue.Map` type objects themselves as property types.
- `Map` supports dict-like reads and writes, and can also be iterated with `Items()`.
- Container wrappers are Unreal containers, not normal Python list/set/dict objects. When passed to reflection functions, they are converted according to the target Unreal type.

## 5. World Context and Actor queries

```python
world = unreal_game_instance.GetWorld()
ue.Log(world.GetName())
```

Many `GameplayStatics` / `KismetSystemLibrary` APIs have a parameter named World Context, but you can pass `unreal_game_instance` directly:

```python
actors = ue.GameplayStatics.GetAllActorsOfClass(unreal_game_instance, ue.Actor)
for actor in actors[:10]:
    ue.Log("[actor] %s" % actor.GetName())
```

Do not iterate all Worlds to guess the context for runtime game logic. The main entry point already provides the correct `unreal_game_instance`.

## 6. Generated Type and UObject instances

`ue.uenum()`, `ue.ustruct()`, and `ue.uclass()` can generate transient reflected types in Python. The example below defines an enum, a struct, and a UObject type:

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

Then create a runtime instance of this UObject type:

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

`AddPythonOwned()` lets Python hold a runtime-created UObject and prevents it from being collected too early by Unreal GC. Call `RemovePythonOwned()` when the lifetime ends. See [GeneratedType.md](GeneratedType.md) for the full Generated Type rules.

## 7. Reflect an Actor and spawn it

```python
class MyActor(ue.Actor):
    HitCount = ue.uproperty(int)

    @ue.ufunction(Params=[int], Ret=int)
    def AddHits(self, value):
        self.HitCount = self.HitCount + value
        return self.HitCount


actor_class = ue.uclass()(MyActor)
```

The following snippet is suitable inside a `MyGameInstance` method:

```python
world_context = self.unreal_game_instance
transform = make_transform(0.0, 0.0, 120.0)

actor = ue.GameplayStatics.BeginDeferredActorSpawnFromClass(world_context, actor_class, transform)
actor = ue.GameplayStatics.FinishSpawningActor(actor, transform)
```

`ue.uclass()` generates a transient reflected type, not a saved Blueprint asset. After changing the class definition, if the current Python VM has already imported the module, restart PIE or the editor for the cleanest verification.

## 8. Lifecycle and cleanup

A Python variable holds an Unreal object wrapper; it does not necessarily extend the lifetime of the Unreal object itself. Runtime-created UObjects, generated Actors, and Actors found in a level have slightly different cleanup rules.

### UObjects created from Python

`ue.NewObject()` creates an Unreal UObject. If the object is referenced only by a Python local variable, you can easily lose the management entry point after the scope exits. If the object will be used later in ticks or callbacks, add it to the Python-owned set after creation:

```python
obj = ue.NewObject(Type=MyData, Name="RuntimeData")
obj.AddPythonOwned()

self.runtime_data = obj
```

When the lifetime ends, remove it from the Python-owned set and clear your own reference:

```python
if self.runtime_data:
    self.runtime_data.RemovePythonOwned()
    self.runtime_data = None
```

`AddPythonOwned()` is for the case where a UObject is created by Python at runtime and Python needs to keep it for some time. Temporary objects do not always need to be added. If you add one, remember to remove it in `shutdown()` or when it is no longer used.

### Actors in the scene

Actor lifetime is managed by the World. Holding an Actor wrapper in Python does not mean the Actor is still alive. After a level unload, game-logic destroy, or PIE end, a wrapper may still remain in a Python variable.

```python
if self.spawned_actor and self.spawned_actor.IsValid():
    self.spawned_actor.K2_DestroyActor()

self.spawned_actor = None
```

Check validity before accessing a cached Actor:

```python
if self.spawned_actor and self.spawned_actor.IsValid():
    self.spawned_actor.CallMethod("AddHits", (1,))
```

Rules of thumb:

- `UObject`: when Python creates it and needs to keep it, use `AddPythonOwned()`; when done, call `RemovePythonOwned()` and clear references.
- `Actor`: use `K2_DestroyActor()` to request destruction; clear the Python reference afterwards.
- Cached UObject/Actor: check whether the object is still valid before use; for Actors, at least check `IsValid()`.
- In `shutdown()`, do not start new asynchronous work or keep accessing Worlds/Actors that have already been cleaned up.

## 9. Actor operations

```python
actor.GetName()
actor.IsValid()
actor.K2_SetActorTransform(transform, False, None, True)
actor.K2_DestroyActor()
actor.CallMethod("AddHits", (3,))
```

`CallMethod()` can call Python, Blueprint, or C++ methods exposed to reflection. Pass arguments as a tuple. A single argument still needs tuple syntax, such as `(3,)`.

An Actor wrapper may still be held by a Python variable after destruction. Check before access:

```python
if actor and actor.IsValid():
    actor.K2_DestroyActor()
```

## 10. Loading Blueprint classes

```python
enemy_class = ue.LoadClass("/Game/Blueprints/BP_Enemy.BP_Enemy_C")
```

If you are unsure about the path, right-click the asset in the Content Browser, copy the reference, and confirm that it is a class path rather than an asset object path. Blueprint Actor class paths usually end with `_C`.

Common distinction:

```python
asset = ue.LoadAsset("/Game/Blueprints/BP_Enemy.BP_Enemy")
klass = ue.LoadClass("/Game/Blueprints/BP_Enemy.BP_Enemy_C")
```

Spawning an Actor requires a class, so `LoadClass()` is usually the correct API.

## 11. Console commands

```python
ue.KismetSystemLibrary.ExecuteConsoleCommand(unreal_game_instance, "stat fps", None)
```

## 12. Run-through demo

The following snippet is suitable in `init()` or `on_start()` because it needs `unreal_game_instance`.

Before using it, put the `make_transform`, `MyData`, and `MyActor` definitions from sections 3, 6, and 7 in the same module, or replace them with your own types.

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

Clean up the generated Actor:

```python
if actor and actor.IsValid():
    actor.K2_DestroyActor()

if obj:
    obj.RemovePythonOwned()
```
