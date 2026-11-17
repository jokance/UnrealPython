"""Regression: an empty delegate proxy must be callable without the runtime VM.

Run in a host project with UnrealPython and PythonScriptPlugin enabled:
    UnrealEditor-Cmd <project.uproject> -run=pythonscript \
        -script=<absolute/path/to/this/file.py> -NullRHI -Unattended

UnrealPython deliberately skips VM initialization in commandlets. Before the
fix, the reflected call below crashed in PyGILState_Ensure instead of returning.
The editor-only ``unreal`` module uses a separate interpreter to drive the test.
"""

import unreal


def run():
    proxy_class = unreal.load_class(
        None, "/Script/UnrealPython.UPyCallableForDelegate"
    )
    if proxy_class is None:
        raise AssertionError("UnrealPython delegate proxy class is unavailable")

    proxy = unreal.new_object(proxy_class)
    result = proxy.call_method("CallPython")
    if result is not None:
        raise AssertionError("An empty delegate proxy should return no value")

    unreal.log("UPY_DELEGATE_UNINITIALIZED_VM_PASS")


run()
