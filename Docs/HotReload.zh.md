[English](HotReload.md) | 中文

# Python 热重载

在编辑器、PIE，或使用编辑器二进制启动的 `-game` 会话运行期间，UnrealPython 会重新加载被修改的 Python 脚本。保存 `.py` 文件后，对应的已加载模块会重新执行；已经持有旧函数、旧类或旧实例的代码也会使用新的定义。

热重载是编辑器构建专用功能。打包版本不会监听脚本文件，也不包含 `ue_hotreload` 模块。

## 触发重载

- **自动**：UnrealPython 监听脚本目录（`Content/Scripts`，或 `[UnrealPython] ScriptPath` 覆盖的目录）以及 **Project Settings -> Plugins -> Unreal Python -> Additional Paths** 中的每一项。`.py` 文件变化后，会等待 0.3 秒内没有新的文件变化，再在游戏线程上重载受影响的模块。
- **控制台命令**：`UnrealPython.Reload` 重载目前已收集到的修改脚本。`UnrealPython.Reload game.player game.inventory` 重载指定的已加载模块，无论其文件是否修改。
- **Python**：

  ```python
  import ue_hotreload

  ue_hotreload.reload_modules(["game.player"])
  ue_hotreload.reload_files([r"D:/MyProject/Content/Scripts/game/player.py"])
  ```

  两者都返回 `ReloadReport`，包含成功重载的模块名 `reloaded`、失败模块及错误 `failed`，以及更新计数。

在同一设置分区关闭 **Auto Reload Scripts** 后只会按需重载，文件变化仍会被收集，供 `UnrealPython.Reload` 使用。

只有已导入的模块会被重载。新文件或尚未导入的文件会在下一次正常 `import` 时加载。删除文件不会卸载对应模块。

如需 IDE 补全 `ue_hotreload`，把 `Plugins/UnrealPython/Content/Python` 与 `Tools/pystubs` 一起加入分析路径。

## 重载做了什么

模块代码在原有模块命名空间中重新执行。执行后，保存状态的模块级变量保留重载前的值，函数、类和导入的模块则使用新的定义。详见 [跨重载保留状态](#跨重载保留状态)。

被修改的模块重新执行后，UnrealPython 会把仍然引用旧定义的地方重定向到新定义：

| 旧对象 | 重载后 |
| --- | --- |
| 其他地方持有的函数对象：`from m import f`、保存的回调、绑定方法、装饰器 wrapper | 执行新代码 |
| 被重载类的实例 | 切换到新类，即 `type(obj) is m.Foo` |
| 定义在未重载模块中的子类 | 继承新类 |
| 旧类对象本身 | 转发到新类，或用新类的内容原地更新 |
| 任意已加载模块中的全局变量，例如 `from m import Foo` | 重新绑定到新对象 |
| `@ue.uclass` 类型的对象 | 重新实例化，其 Python wrapper 切换到新类 |
| `@ue.ustruct` / `@ue.uenum` 类型 | 与 `ue.ReloadModule` 相同地重新生成；重载前创建的值保留旧类型 |

多个文件同时变化时，被导入的模块会先于导入它的模块重载。

如果模块在重新执行时抛出异常（语法错误或导入时异常），会恢复它原来的命名空间并记录错误，旧版本继续生效。修复文件后再次保存即可。

## 跨重载保留状态

不是类、函数或模块的全局变量会保留重载前的值，包括注册表、缓存、计数器、单例，以及 `MAX_HP = 100` 这类常量。以双下划线开头和结尾的名称（例如 `__all__`）总是使用新值。被保留的实例如果属于被重载的类，仍会切换到新类，因此保留数据的同时执行新方法。

如需应用文件中的新值，把名称列入 `__reload_use_new__`：

```python
MAX_HP = 120
SPAWN_TABLE = {"slime": 3, "bat": 1}
_timers: dict[int, Timer] = {}

# 每次重载都使用文件中定义的值；其余变量保留原值。
__reload_use_new__ = ("MAX_HP", "SPAWN_TABLE")
```

`__reload_use_new__` 可以是单个名称或名称序列。如果文件修改了未列出的 UPPER_CASE 常量的值，重载时会输出一条警告并列出这些名称，便于在日志中发现未生效的修改。

需要代码处理状态时，定义钩子函数：

```python
def __before_reload__():
    # 在旧版本上、模块重新执行之前调用。
    # 抛出异常会取消本模块的重载。
    unregister_console_commands()
    return {"selection": current_selection()}


def __after_reload__(state):
    # 整批重载完成后，在新模块上调用。
    register_console_commands()
    restore_selection(state["selection"])
```

模块没有定义 `__before_reload__` 时，`__after_reload__` 收到 `None`。如果 `__before_reload__` 已执行但重载失败，恢复后的旧版本仍会调用 `__after_reload__(state)`，以便撤销之前的清理。

## 限制

- 已有实例保留自己的 `__dict__`。修改后的 `__init__` 新增的属性不会出现在重载前创建的对象上；请为其提供类级默认值，或在 `__after_reload__` 中设置。
- Enum 类只重新绑定，不做修补。重载前保存的成员与新 enum 的成员比较不相等。
- 类修改了 `__slots__` 时，已有实例无法切换到新类，会保留旧类，旧类被原地更新。
- 闭包发生变化的函数（捕获了不同的变量）在旧函数对象仍被引用的地方保留旧代码。
- 重载前保存在模块全局变量之外的类对象，例如未重载模块中的注册表，仍会创建旧类的实例。这些实例执行新方法，但其中的 `super()` 调用可能失败。请在 `__after_reload__` 中重新注册类，或通过模块查找类。
- 不是普通函数的可调用对象（例如装饰器返回的实例）视为状态，会保留旧值。需要更新其行为时，把它列入 `__reload_use_new__`。
- `from m import LIMIT` 这类值在导入它的模块中保留旧值，即使 `m` 把 `LIMIT` 列入了 `__reload_use_new__`；跨模块只会重新绑定函数和类。
- `ue_site.py` 重载时不会再次调用 `ue_site.on_init()`；请使用 `__after_reload__`。
- 更新实例时会遍历一次所有 Python 对象，在大型会话中可能需要明显的零点几秒。

## 与 `ue.ReloadModule` 的关系

`ue.ReloadModule(module)` 仍然只执行普通的 `importlib.reload`，随后进行生成类型的重新实例化，不会保留模块状态，也不会重定向旧函数、旧类或旧实例。热重载使用相同的生成类型重新实例化流程。

## 测试

`Tools/Tests/test_ue_hotreload.py` 使用标准 CPython 和替身 `ue` 模块覆盖重载行为：

```bash
python -m unittest discover -s Plugins/UnrealPython/Tools/Tests -p "test_ue_hotreload.py"
```
