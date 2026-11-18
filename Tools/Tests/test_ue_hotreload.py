"""Unit tests for the UnrealPython hot reload module (Content/Python/ue_hotreload.py).

The reloader is pure Python, so these tests run on a stock CPython without the
engine. A minimal fake ``ue`` module stands in for the native one.

    python -m unittest discover -s Plugins/UnrealPython/Tools/Tests -p "test_ue_hotreload.py"
"""

from __future__ import annotations

import importlib
import os
import shutil
import sys
import tempfile
import textwrap
import types
import unittest
from pathlib import Path
from typing import Any


PLUGIN_PYTHON_DIR = Path(__file__).resolve().parents[2] / "Content" / "Python"


class _FakeWrapperBase:
    pass


class _FakeEnumBase(int):
    pass


def _install_fake_ue() -> types.ModuleType:
    fake_ue = types.ModuleType("ue")
    fake_ue.Log = lambda message: None
    fake_ue.LogWarning = lambda message: None
    fake_ue.LogError = lambda message: None
    fake_ue.FlushGeneratedTypeReinstancing = lambda: None
    fake_ue.WrapperBase = _FakeWrapperBase
    fake_ue.EnumBase = _FakeEnumBase
    sys.modules["ue"] = fake_ue
    return fake_ue


_original_ue = sys.modules.get("ue")
fake_ue = _install_fake_ue()
sys.path.insert(0, str(PLUGIN_PYTHON_DIR))
import ue_hotreload  # noqa: E402

ue_hotreload.ue = fake_ue


class HotReloadTestCase(unittest.TestCase):
    def setUp(self) -> None:
        self.root = Path(tempfile.mkdtemp(prefix="ue_hotreload_test_"))
        self.module_names: list[str] = []
        self.errors: list[str] = []
        self.warnings: list[str] = []
        fake_ue.LogError = self.errors.append
        fake_ue.LogWarning = self.warnings.append
        sys.path.insert(0, str(self.root))
        self.addCleanup(self._cleanup)

    def _cleanup(self) -> None:
        sys.path.remove(str(self.root))
        for name in self.module_names:
            sys.modules.pop(name, None)
        fake_ue.LogError = lambda message: None
        fake_ue.LogWarning = lambda message: None
        shutil.rmtree(self.root, ignore_errors=True)

    def write(self, relative_path: str, source: str) -> Path:
        path = self.root / relative_path
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(textwrap.dedent(source), encoding="utf-8")
        return path

    def load(self, name: str) -> Any:
        self.module_names.append(name)
        importlib.invalidate_caches()
        return importlib.import_module(name)

    def reload_files(self, *paths: Path) -> ue_hotreload.ReloadReport:
        return ue_hotreload.reload_files([str(path) for path in paths])


class FunctionReloadTests(HotReloadTestCase):
    def test_previous_function_objects_run_new_code(self) -> None:
        path = self.write("hr_func_a.py", "def value():\n    return 1\n")
        self.write("hr_func_b.py", "from hr_func_a import value\n\ndef call():\n    return value()\n")
        module_a = self.load("hr_func_a")
        module_b = self.load("hr_func_b")
        captured = module_a.value

        self.write("hr_func_a.py", "def value():\n    return 2\n")
        report = self.reload_files(path)

        self.assertEqual(report.reloaded, ["hr_func_a"])
        self.assertEqual(captured(), 2)
        self.assertEqual(module_b.call(), 2)
        self.assertIs(module_b.value, module_a.value)

    def test_decorated_function_wrapper_calls_new_function(self) -> None:
        source = """
            import functools

            def deco(func):
                @functools.wraps(func)
                def wrapper(*args):
                    return func(*args)
                return wrapper

            @deco
            def value():
                return {result}
        """
        path = self.write("hr_deco.py", source.format(result=1))
        module = self.load("hr_deco")
        captured = module.value

        self.write("hr_deco.py", source.format(result=2))
        self.reload_files(path)

        self.assertEqual(captured(), 2)


class ClassReloadTests(HotReloadTestCase):
    def test_existing_instance_and_bound_method_use_reloaded_class(self) -> None:
        source = "class Foo:\n    def value(self):\n        return {result}\n"
        path = self.write("hr_cls.py", source.format(result=1))
        module = self.load("hr_cls")
        old_class = module.Foo
        instance = module.Foo()
        bound = instance.value

        self.write("hr_cls.py", source.format(result=2))
        report = self.reload_files(path)

        self.assertIs(type(instance), module.Foo)
        self.assertEqual(instance.value(), 2)
        self.assertEqual(bound(), 2)
        self.assertEqual(old_class().value(), 2)
        self.assertGreaterEqual(report.instances, 1)

    def test_abstract_base_subclass_in_other_module_inherits_reloaded_base(self) -> None:
        source = """
            import abc

            class Base(abc.ABC):
                @abc.abstractmethod
                def kind(self): ...

                def describe(self):
                    return "{prefix}:" + self.kind()
        """
        path = self.write("hr_abc.py", source.format(prefix="v1"))
        self.write("hr_abc_impl.py", "from hr_abc import Base\n\nclass Impl(Base):\n    def kind(self):\n        return 'impl'\n")
        module = self.load("hr_abc")
        impl_module = self.load("hr_abc_impl")
        impl = impl_module.Impl()

        self.write("hr_abc.py", source.format(prefix="v2"))
        self.reload_files(path)

        self.assertEqual(impl.describe(), "v2:impl")
        self.assertIsInstance(impl, module.Base)
        self.assertEqual(impl_module.Impl().describe(), "v2:impl")

    def test_subclass_in_other_module_inherits_reloaded_base(self) -> None:
        source = "class Base:\n    def value(self):\n        return {result}\n"
        path = self.write("hr_base.py", source.format(result=1))
        self.write("hr_child.py", "from hr_base import Base\n\nclass Child(Base):\n    pass\n")
        module_base = self.load("hr_base")
        module_child = self.load("hr_child")
        child = module_child.Child()

        self.write("hr_base.py", source.format(result=2))
        self.reload_files(path)

        self.assertEqual(child.value(), 2)
        self.assertIsInstance(child, module_base.Base)
        self.assertTrue(issubclass(module_child.Child, module_base.Base))

    def test_super_calls_work_for_old_instances_methods_and_classes(self) -> None:
        source = """
            class Base:
                def name(self):
                    return "base"

            class Child(Base):
                def name(self):
                    return "{prefix}-" + super().name()
        """
        path = self.write("hr_super.py", source.format(prefix="child"))
        module = self.load("hr_super")
        old_child_class = module.Child
        instance = module.Child()
        bound = instance.name

        self.write("hr_super.py", source.format(prefix="kid"))
        self.reload_files(path)

        self.assertEqual(instance.name(), "kid-base")
        self.assertEqual(bound(), "kid-base")
        self.assertEqual(old_child_class().name(), "kid-base")

    def test_frozen_dataclass_instance_is_reclassed(self) -> None:
        source = """
            from dataclasses import dataclass

            @dataclass(frozen=True)
            class Item:
                count: int

                def doubled(self):
                    return self.count * {factor}
        """
        path = self.write("hr_dataclass.py", source.format(factor=2))
        module = self.load("hr_dataclass")
        item = module.Item(3)

        self.write("hr_dataclass.py", source.format(factor=3))
        self.reload_files(path)

        self.assertIs(type(item), module.Item)
        self.assertEqual(item.doubled(), 9)
        self.assertEqual(item, module.Item(3))

    def test_slotted_class_falls_back_to_in_place_update(self) -> None:
        source = """
            class Point:
                __slots__ = ("x",)

                def __init__(self):
                    self.x = 1

                def value(self):
                    return self.x + {offset}
        """
        path = self.write("hr_slots.py", source.format(offset=0))
        module = self.load("hr_slots")
        old_class = module.Point
        point = module.Point()

        self.write("hr_slots.py", source.format(offset=10))
        self.reload_files(path)

        self.assertEqual(point.value(), 11)
        self.assertEqual(old_class().value(), 11)

    def test_unreal_backed_class_is_rebound_but_not_forwarded(self) -> None:
        source = "import ue\n\nclass Actorish(ue.WrapperBase):\n    def value(self):\n        return {result}\n"
        path = self.write("hr_unreal.py", source.format(result=1))
        self.write("hr_unreal_user.py", "from hr_unreal import Actorish\n")
        module = self.load("hr_unreal")
        user = self.load("hr_unreal_user")
        old_class = module.Actorish
        instance = module.Actorish()

        self.write("hr_unreal.py", source.format(result=2))
        self.reload_files(path)

        self.assertEqual(old_class.__bases__, (fake_ue.WrapperBase,))
        self.assertIs(type(instance), old_class)
        self.assertIs(user.Actorish, module.Actorish)
        self.assertEqual(instance.value(), 2)

    def test_enum_members_are_not_patched(self) -> None:
        source = "import enum\n\nclass Mode(enum.Enum):\n    IDLE = {value}\n"
        path = self.write("hr_enum.py", source.format(value=1))
        module = self.load("hr_enum")
        old_class = module.Mode

        self.write("hr_enum.py", source.format(value=2))
        report = self.reload_files(path)

        self.assertEqual(report.failed, {})
        self.assertEqual(module.Mode.IDLE.value, 2)
        self.assertEqual(old_class.IDLE.value, 1)


class ModuleStateTests(HotReloadTestCase):
    def test_state_globals_keep_previous_values(self) -> None:
        source = "LIMIT = {limit}\n_cache = {{}}\n_counter = 0\n"
        path = self.write("hr_state.py", source.format(limit=1))
        module = self.load("hr_state")
        module._cache["key"] = "kept"
        module._counter = 5

        self.write("hr_state.py", source.format(limit=2))
        report = self.reload_files(path)

        self.assertEqual(module.LIMIT, 1)
        self.assertEqual(module._cache, {"key": "kept"})
        self.assertEqual(module._counter, 5)
        self.assertEqual(report.kept, 3)
        self.assertEqual(len(self.warnings), 1)
        self.assertIn("LIMIT", self.warnings[0])
        self.assertNotIn("_counter", self.warnings[0])

    def test_use_new_names_take_reloaded_values(self) -> None:
        source = 'LIMIT = {limit}\nNAME = "{name}"\n__reload_use_new__ = {use_new}\n'
        path = self.write("hr_use_new.py", source.format(limit=1, name="a", use_new='("LIMIT",)'))
        module = self.load("hr_use_new")

        self.write("hr_use_new.py", source.format(limit=2, name="b", use_new='("LIMIT",)'))
        self.reload_files(path)

        self.assertEqual(module.LIMIT, 2)
        self.assertEqual(module.NAME, "a")

        self.write("hr_use_new.py", source.format(limit=3, name="c", use_new='"NAME"'))
        self.reload_files(path)

        self.assertEqual(module.LIMIT, 2)
        self.assertEqual(module.NAME, "c")

    def test_kept_instance_global_uses_reloaded_class(self) -> None:
        source = "class Service:\n    def value(self):\n        return {result}\n\n_service = Service()\n"
        path = self.write("hr_service.py", source.format(result=1))
        module = self.load("hr_service")
        service = module._service

        self.write("hr_service.py", source.format(result=2))
        self.reload_files(path)

        self.assertIs(module._service, service)
        self.assertIs(type(service), module.Service)
        self.assertEqual(service.value(), 2)

    def test_reload_hooks_pass_state_from_old_to_new_module(self) -> None:
        source = """
            VERSION = {version}
            RESTORED = None

            def __before_reload__():
                return {{"version": VERSION}}

            def __after_reload__(state):
                global RESTORED
                RESTORED = state
        """
        path = self.write("hr_hooks.py", source.format(version=1))
        module = self.load("hr_hooks")

        self.write("hr_hooks.py", source.format(version=2))
        self.reload_files(path)

        self.assertEqual(module.VERSION, 1)
        self.assertEqual(module.RESTORED, {"version": 1})

    def test_failing_before_reload_hook_skips_module(self) -> None:
        source = "VALUE = {value}\n\ndef __before_reload__():\n    raise RuntimeError('not now')\n"
        path = self.write("hr_veto.py", source.format(value=1))
        module = self.load("hr_veto")

        self.write("hr_veto.py", source.format(value=2))
        report = self.reload_files(path)

        self.assertIn("not now", report.failed["hr_veto"])
        self.assertEqual(module.VALUE, 1)

    def test_failed_reload_lets_previous_version_undo_before_reload(self) -> None:
        source = """
            registered = True

            def __before_reload__():
                global registered
                registered = False
                return "teardown"

            def __after_reload__(state):
                global registered
                registered = state == "teardown"
        """
        path = self.write("hr_balance.py", source)
        module = self.load("hr_balance")

        self.write("hr_balance.py", source + "\nraise RuntimeError('boom')\n")
        report = self.reload_files(path)

        self.assertIn("hr_balance", report.failed)
        self.assertTrue(module.registered)

    def test_failed_reload_keeps_previous_module(self) -> None:
        path = self.write("hr_fail.py", "def value():\n    return 1\n")
        module = self.load("hr_fail")
        captured = module.value

        self.write("hr_fail.py", "def value():\n    return 2\n\nraise RuntimeError('boom')\n")
        report = self.reload_files(path)

        self.assertEqual(report.reloaded, [])
        self.assertIn("hr_fail", report.failed)
        self.assertIn("boom", report.failed["hr_fail"])
        self.assertEqual(len(self.errors), 1)
        self.assertIs(module.value, captured)
        self.assertEqual(module.value(), 1)

        self.write("hr_fail.py", "def value():\n    return 3\n")
        report = self.reload_files(path)

        self.assertEqual(report.reloaded, ["hr_fail"])
        self.assertEqual(captured(), 3)

    def test_syntax_error_keeps_previous_module(self) -> None:
        path = self.write("hr_syntax.py", "def value():\n    return 1\n")
        module = self.load("hr_syntax")

        self.write("hr_syntax.py", "def value(:\n    return 2\n")
        report = self.reload_files(path)

        self.assertIn("hr_syntax", report.failed)
        self.assertEqual(module.value(), 1)


class FileSelectionTests(HotReloadTestCase):
    def test_unloaded_and_unknown_files_are_ignored(self) -> None:
        path = self.write("hr_unloaded.py", "VALUE = 1\n")
        self.module_names.append("hr_unloaded")

        report = self.reload_files(path, self.root / "missing.py")

        self.assertEqual(report.reloaded, [])
        self.assertEqual(report.failed, {})
        self.assertNotIn("hr_unloaded", sys.modules)

    def test_package_init_and_submodule_are_matched(self) -> None:
        init_path = self.write("hr_pkg/__init__.py", "from .sub import value\n")
        sub_path = self.write("hr_pkg/sub.py", "def value():\n    return 1\n")
        self.module_names.append("hr_pkg.sub")
        package = self.load("hr_pkg")

        self.write("hr_pkg/sub.py", "def value():\n    return 2\n")
        report = self.reload_files(init_path, sub_path)

        self.assertEqual(report.reloaded, ["hr_pkg.sub", "hr_pkg"])
        self.assertEqual(package.value(), 2)

    def test_dependencies_reload_before_importers(self) -> None:
        source_a = 'LIMIT = {limit}\n__reload_use_new__ = "LIMIT"\n'
        path_a = self.write("hr_dep_a.py", source_a.format(limit=1))
        path_b = self.write(
            "hr_dep_b.py",
            'from hr_dep_a import LIMIT\nDOUBLE = LIMIT * 2\n__reload_use_new__ = ("LIMIT", "DOUBLE")\n',
        )
        self.load("hr_dep_a")
        module_b = self.load("hr_dep_b")

        self.write("hr_dep_a.py", source_a.format(limit=5))
        report = self.reload_files(path_b, path_a)

        self.assertEqual(report.reloaded, ["hr_dep_a", "hr_dep_b"])
        self.assertEqual(module_b.DOUBLE, 10)

    def test_stale_bytecode_with_same_size_and_mtime_is_not_used(self) -> None:
        path = self.write("hr_pyc.py", "def value():\n    return 1\n")
        stat = path.stat()
        module = self.load("hr_pyc")
        self.assertTrue(Path(module.__cached__).is_file())

        self.write("hr_pyc.py", "def value():\n    return 2\n")
        os.utime(path, ns=(stat.st_atime_ns, stat.st_mtime_ns))
        self.reload_files(path)

        self.assertEqual(module.value(), 2)

    def test_reload_modules_by_name(self) -> None:
        self.write("hr_named.py", "def value():\n    return 1\n")
        module = self.load("hr_named")

        self.write("hr_named.py", "def value():\n    return 2\n")
        report = ue_hotreload.reload_modules(["hr_named", "hr_not_loaded"])

        self.assertEqual(report.reloaded, ["hr_named"])
        self.assertEqual(module.value(), 2)


def tearDownModule() -> None:
    sys.path.remove(str(PLUGIN_PYTHON_DIR))
    sys.modules.pop("ue_hotreload", None)
    if _original_ue is None:
        sys.modules.pop("ue", None)
    else:
        sys.modules["ue"] = _original_ue


if __name__ == "__main__":
    unittest.main()
