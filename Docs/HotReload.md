English | [中文](HotReload.zh.md)

# Python Hot Reload

UnrealPython reloads modified Python scripts while the editor, PIE, or a `-game` session started with editor binaries is running. Save a `.py` file and the loaded module that came from it re-executes; code that already holds the previous functions, classes, or instances picks up the new definitions.

Hot reload is an editor-build feature. Packaged builds do not watch script files and do not include the `ue_hotreload` module.

## Triggering a reload

- **Automatic**: UnrealPython watches the script directory (`Content/Scripts`, or the `[UnrealPython] ScriptPath` override) and every entry of **Project Settings -> Plugins -> Unreal Python -> Additional Paths**. When `.py` files change, it waits until no file has changed for 0.3 seconds and then reloads the affected modules on the game thread.
- **Console command**: `UnrealPython.Reload` reloads the modified scripts collected so far. `UnrealPython.Reload game.player game.inventory` reloads the named loaded modules, whether or not their files changed.
- **Python**:

  ```python
  import ue_hotreload

  ue_hotreload.reload_modules(["game.player"])
  ue_hotreload.reload_files([r"D:/MyProject/Content/Scripts/game/player.py"])
  ```

  Both return a `ReloadReport` with the `reloaded` module names, the `failed` modules and their errors, and update counters.

Turn off **Auto Reload Scripts** in the same settings section to reload only on demand. Changes are still collected for `UnrealPython.Reload`.

Only modules that are already imported are reloaded. A new file, or one that has not been imported yet, is picked up by the next regular `import`. Deleting a file does not unload its module.

For IDE completion of `ue_hotreload`, add `Plugins/UnrealPython/Content/Python` to the analysis paths next to `Tools/pystubs`.

## What a reload does

The module body re-executes in the existing module namespace. Afterwards, every module-level variable that holds state keeps its pre-reload value, while functions, classes, and imported modules take the reloaded definitions. See [Keeping state across reloads](#keeping-state-across-reloads).

After the changed modules re-executed, UnrealPython redirects everything that still refers to the previous definitions:

| Previous object | After the reload |
| --- | --- |
| Function objects held elsewhere: `from m import f`, stored callbacks, bound methods, decorator wrappers | Run the new code |
| Instances of a reloaded class | Switch to the reloaded class, so `type(obj) is m.Foo` |
| Subclasses defined in modules that were not reloaded | Derive from the reloaded class |
| The previous class object | Forwards to, or is updated from, the reloaded class |
| Module globals in any loaded module, such as `from m import Foo` | Rebound to the reloaded object |
| Objects of `@ue.uclass` types | Re-instanced; their Python wrappers switch to the reloaded class |
| `@ue.ustruct` / `@ue.uenum` types | Regenerated as with `ue.ReloadModule`; values created before the reload keep the previous type |

When several files change together, imported modules reload before the modules that import them.

If a module raises while it re-executes (a syntax error, or an exception at import time), its previous namespace is restored, the error is logged, and the previous version stays active. Fix the file and save it again.

## Keeping state across reloads

A global keeps its pre-reload value when it is not a class, a function, or a module. This covers registries, caches, counters, singletons, and constants such as `MAX_HP = 100`. Names starting and ending with double underscores, such as `__all__`, always take the reloaded values. A kept instance of a reloaded class still switches to the reloaded class, so it keeps its data and runs the new methods.

To apply new values from the file, list the names in `__reload_use_new__`:

```python
MAX_HP = 120
SPAWN_TABLE = {"slime": 3, "bat": 1}
_timers: dict[int, Timer] = {}

# Take the values defined in the file on every reload; everything else keeps its value.
__reload_use_new__ = ("MAX_HP", "SPAWN_TABLE")
```

`__reload_use_new__` accepts a single name or a sequence of names. When the file changes the value of an UPPER_CASE constant that is not listed, the reload logs a warning naming it, so an edit that did not take effect is visible in the log.

When keeping state needs code, define hooks:

```python
def __before_reload__():
    # Runs on the previous version, before the module re-executes.
    # Raising cancels this module's reload.
    unregister_console_commands()
    return {"selection": current_selection()}


def __after_reload__(state):
    # Runs on the reloaded module after the whole reload batch finished.
    register_console_commands()
    restore_selection(state["selection"])
```

`__after_reload__` receives `None` when the module has no `__before_reload__`. If the reload fails after `__before_reload__` ran, the restored previous version's `__after_reload__(state)` still runs, so it can undo the teardown.

## Limitations

- Existing instances keep their `__dict__`. Attributes that a changed `__init__` now creates are missing on objects created before the reload; give them class-level defaults or set them in `__after_reload__`.
- Enum classes are rebound but not patched. Members stored before the reload do not compare equal to members of the reloaded enum.
- When a class changes its `__slots__`, existing instances cannot switch to the reloaded class. They keep the previous class, which is updated in place.
- A function whose closure changed (it now captures different variables) keeps its previous code wherever the previous function object is still referenced.
- A class object stored before the reload outside the module globals, for example in a registry of a module that was not reloaded, still creates instances of the previous class. Those instances run the new methods, but `super()` calls in them can fail. Register classes again in `__after_reload__`, or look them up through the module.
- Callable objects that are not plain functions, such as instances returned by a decorator, count as state and keep their previous value. List them in `__reload_use_new__` when their behavior should change.
- Values such as `from m import LIMIT` keep the previous value in the importing module, even when `m` lists `LIMIT` in `__reload_use_new__`; only functions and classes are rebound across modules.
- `ue_site.on_init()` is not called again when `ue_site.py` reloads; use `__after_reload__`.
- Updating instances walks every Python object once, which can take a noticeable fraction of a second in large sessions.

## Relation to `ue.ReloadModule`

`ue.ReloadModule(module)` still performs a plain `importlib.reload` followed by generated type re-instancing. It does not keep module state or redirect previous functions, classes, or instances. Hot reload uses the same generated type re-instancing.

## Tests

`Tools/Tests/test_ue_hotreload.py` covers the reload behavior with a stock CPython and a stand-in `ue` module:

```bash
python -m unittest discover -s Plugins/UnrealPython/Tools/Tests -p "test_ue_hotreload.py"
```
