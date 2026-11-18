"""Hot reload for UnrealPython scripts.

In editor builds UnrealPython watches the project script directories and calls
:func:`reload_files` with the ``.py`` files that changed. Scripts can also call
:func:`reload_files` / :func:`reload_modules` directly, and the
``UnrealPython.Reload`` console command reloads on demand.

A reload re-executes the module body in the existing module namespace, then puts
back the pre-reload values of its state: every global that is not a class,
function, or module keeps its previous value unless the module lists the name in
``__reload_use_new__``. Functions, classes, and modules always take the reloaded
definitions, and everything that still refers to the previous definitions is
redirected to the reloaded ones:

* previous function objects run the new code, including closures and ``super()``;
* existing instances switch to the reloaded class;
* previous classes forward to the reloaded class, so subclasses defined in modules
  that were not reloaded inherit the new behavior;
* module globals in every loaded module that refer to a previous definition are
  rebound to the reloaded one.

Optional module-level declarations:

* ``__reload_use_new__``: names of globals that take the reloaded values instead
  of keeping their previous ones.
* ``__before_reload__()``: called on the module before it re-executes. Its return
  value is passed to ``__after_reload__``. Raising skips the module's reload.
* ``__after_reload__(state)``: called after the whole reload batch finished. If the
  module failed to reload after ``__before_reload__`` ran, it is called on the
  restored previous version so it can undo what ``__before_reload__`` tore down.

Unreal generated types (``ue.uclass``/``ue.ustruct``/``ue.uenum``) are re-instanced
by UnrealPython itself; they are rebound here but otherwise left untouched.
"""

from __future__ import annotations

import abc
import ast
import gc
import importlib
import inspect
import os
import sys
import tokenize
import traceback
import types
from dataclasses import dataclass, field
from typing import Any, Iterable

import ue


__all__ = ["ReloadReport", "reload_files", "reload_modules"]

_USE_NEW_NAME = "__reload_use_new__"
_BEFORE_RELOAD_NAME = "__before_reload__"
_AFTER_RELOAD_NAME = "__after_reload__"
_LOG_PREFIX = "[HotReload]"

# Entries that describe a class's own identity or layout. They stay on a previous
# class when it is redirected to the reloaded one.
_KEPT_CLASS_ATTRIBUTES = frozenset({
    "__dict__",
    "__weakref__",
    "__module__",
    "__qualname__",
    "__doc__",
    "__slots__",
    "__firstlineno__",
    "__static_attributes__",
    "__abstractmethods__",
    "__annotations__",
    "__annotate__",
    "__annotate_func__",
    "__annotations_cache__",
    "__type_params__",
    "_abc_impl",
})

# Value types of constants whose edits are reported when the previous value is kept.
_CONSTANT_TYPES = (bool, int, float, complex, str, bytes)

_is_reloading = False


@dataclass
class ReloadReport:
    """Summary of one reload batch."""

    reloaded: list[str] = field(default_factory=list)
    """Names of the modules that re-executed successfully, in reload order."""

    failed: dict[str, str] = field(default_factory=dict)
    """Module name to formatted error for modules that kept their previous version."""

    functions: int = 0
    classes: int = 0
    instances: int = 0
    rebound: int = 0
    kept: int = 0
    """Number of globals that kept their pre-reload values."""

    @property
    def succeeded(self) -> bool:
        return not self.failed


def reload_files(paths: Iterable[str]) -> ReloadReport:
    """Reload the loaded modules whose source files are in ``paths``.

    Files that do not belong to a loaded module are ignored; they are picked up
    by the next regular import.
    """

    requested_paths = {_normalize_path(str(path)) for path in paths}
    requested_file_names = {os.path.basename(path) for path in requested_paths}
    modules: list[types.ModuleType] = []
    for name, module in list(sys.modules.items()):
        source_path = _module_source_path(name, module)
        # Resolving every loaded module's path is slow; compare file names first.
        if source_path is None or os.path.normcase(os.path.basename(source_path)) not in requested_file_names:
            continue

        if _normalize_path(source_path) in requested_paths and module not in modules:
            modules.append(module)

    return _reload(modules)


def reload_modules(modules: Iterable[str | types.ModuleType]) -> ReloadReport:
    """Reload loaded modules given as module objects or module names."""

    resolved: list[types.ModuleType] = []
    for module_or_name in modules:
        module = sys.modules.get(module_or_name) if isinstance(module_or_name, str) else module_or_name
        if not isinstance(module, types.ModuleType):
            ue.LogWarning(f"{_LOG_PREFIX} Skipping {module_or_name!r}; it is not a loaded module.")
            continue

        if module not in resolved:
            resolved.append(module)

    return _reload(resolved)


def _reload(modules: list[types.ModuleType]) -> ReloadReport:
    global _is_reloading

    report = ReloadReport()
    if _is_reloading:
        ue.LogWarning(f"{_LOG_PREFIX} Ignoring a nested reload request.")
        return report

    modules = [module for module in modules if _can_reload(module)]
    if not modules:
        return report

    _is_reloading = True
    try:
        _reload_batch(modules, report)
    finally:
        _is_reloading = False

    _log_report(report)
    return report


def _reload_batch(modules: list[types.ModuleType], report: ReloadReport) -> None:
    importlib.invalidate_caches()
    replacements = _Replacements()
    after_reload_states: list[tuple[types.ModuleType, Any]] = []

    for module in _sort_by_dependencies(modules):
        attempt = _reload_module(module, report)
        if attempt.old_namespace is not None:
            replacements.collect_module(module, attempt.old_namespace)

        if attempt.call_after_reload:
            after_reload_states.append((module, attempt.state))

    ue.FlushGeneratedTypeReinstancing()
    replacements.apply(report)

    for module, state in after_reload_states:
        _call_after_reload(module, state)


def _can_reload(module: types.ModuleType) -> bool:
    name = module.__name__
    if module is sys.modules.get(__name__):
        ue.LogWarning(f"{_LOG_PREFIX} {name} cannot reload itself.")
        return False

    if sys.modules.get(name) is not module:
        ue.LogWarning(f"{_LOG_PREFIX} Skipping {name}; it is not registered in sys.modules under its own name.")
        return False

    return True


@dataclass
class _ReloadAttempt:
    old_namespace: dict[str, Any] | None = None
    """The namespace before the reload; None when the module kept its previous version."""

    state: Any = None
    call_after_reload: bool = False


def _reload_module(module: types.ModuleType, report: ReloadReport) -> _ReloadAttempt:
    """Re-execute one module, restoring its previous namespace if that fails."""

    name = module.__name__
    namespace = module.__dict__
    attempt = _ReloadAttempt()

    before_reload = namespace.get(_BEFORE_RELOAD_NAME)
    if callable(before_reload):
        try:
            attempt.state = before_reload()
        except Exception:
            report.failed[name] = traceback.format_exc()
            return attempt

    # Snapshot after __before_reload__ so a restore keeps the state it left behind.
    old_namespace = dict(namespace)
    _remove_cached_bytecode(module)

    try:
        importlib.reload(module)
    except Exception:
        # Keep the previous version active: the partially executed body may have
        # replaced only some of the module's definitions.
        namespace.clear()
        namespace.update(old_namespace)
        sys.modules[name] = module
        report.failed[name] = traceback.format_exc()
        # Let the previous version undo what its __before_reload__ tore down.
        attempt.call_after_reload = callable(before_reload)
        return attempt

    report.kept += _restore_module_state(module, old_namespace)
    report.reloaded.append(name)
    attempt.old_namespace = old_namespace
    attempt.call_after_reload = True
    return attempt


def _call_after_reload(module: types.ModuleType, state: Any) -> None:
    after_reload = module.__dict__.get(_AFTER_RELOAD_NAME)
    if not callable(after_reload):
        return

    try:
        after_reload(state)
    except Exception:
        ue.LogError(f"{_LOG_PREFIX} {module.__name__}.{_AFTER_RELOAD_NAME} failed.\n{traceback.format_exc()}")


def _restore_module_state(module: types.ModuleType, old_namespace: dict[str, Any]) -> int:
    """Give the module's state globals their pre-reload values. Returns how many were kept."""

    namespace = module.__dict__
    use_new_names = _use_new_names(module)
    kept = 0
    edited_constants: list[str] = []
    for name, old_value in old_namespace.items():
        if name in use_new_names or _is_dunder(name) or _is_code(old_value) or name not in namespace:
            continue

        new_value = namespace[name]
        if new_value is old_value or _is_code(new_value):
            continue

        if _is_edited_constant(name, old_value, new_value):
            edited_constants.append(name)

        namespace[name] = old_value
        kept += 1

    if edited_constants:
        ue.LogWarning(
            f"{_LOG_PREFIX} {module.__name__} kept the previous values of {', '.join(edited_constants)}, "
            f"which the file changed; list them in {_USE_NEW_NAME} to apply the new values."
        )

    return kept


def _use_new_names(module: types.ModuleType) -> frozenset[str]:
    value = module.__dict__.get(_USE_NEW_NAME)
    if value is None:
        return frozenset()

    if isinstance(value, str):
        return frozenset((value,))

    if isinstance(value, (list, tuple, set, frozenset)) and all(isinstance(item, str) for item in value):
        return frozenset(value)

    ue.LogWarning(f"{_LOG_PREFIX} {module.__name__}.{_USE_NEW_NAME} must be a string or a sequence of strings.")
    return frozenset()


def _is_dunder(name: str) -> bool:
    return name.startswith("__") and name.endswith("__")


def _is_code(value: Any) -> bool:
    return inspect.isclass(value) or inspect.isroutine(value) or inspect.ismodule(value)


def _is_edited_constant(name: str, old_value: Any, new_value: Any) -> bool:
    # Only UPPER_CASE names are treated as constants: lower-case globals are usually
    # state whose runtime value is expected to differ from the file.
    return (
        name.isupper()
        and isinstance(old_value, _CONSTANT_TYPES)
        and isinstance(new_value, _CONSTANT_TYPES)
        and old_value != new_value
    )


def _remove_cached_bytecode(module: types.ModuleType) -> None:
    # The bytecode cache is validated by source mtime (in whole seconds) and size,
    # so a quick edit that keeps the file size would otherwise load stale code.
    cached_path = module.__dict__.get("__cached__")
    if not isinstance(cached_path, str) or not os.path.isfile(cached_path):
        return

    try:
        os.remove(cached_path)
    except OSError as error:
        ue.LogWarning(f"{_LOG_PREFIX} Could not remove cached bytecode {cached_path}: {error}")


def _normalize_path(path: str) -> str:
    return os.path.normcase(os.path.realpath(path))


def _module_source_path(name: str, module: Any) -> str | None:
    if name == "__main__" or not isinstance(module, types.ModuleType):
        return None

    file_path = module.__dict__.get("__file__")
    if isinstance(file_path, str) and os.path.splitext(file_path)[1].lower() == ".py":
        return file_path

    return None


def _sort_by_dependencies(modules: list[types.ModuleType]) -> list[types.ModuleType]:
    """Order modules so that imported modules reload before the modules importing them."""

    import_order = {name: index for index, name in enumerate(sys.modules)}
    by_name = {module.__name__: module for module in modules}
    names = sorted(by_name, key=lambda name: import_order.get(name, -1), reverse=True)
    dependencies: dict[str, list[str]] = {}
    for name in names:
        imported_names = _imported_names(by_name[name])
        dependencies[name] = [other for other in names if other != name and other in imported_names]

    ordered: list[types.ModuleType] = []
    visited: set[str] = set()

    def visit(name: str) -> None:
        if name in visited:
            return

        visited.add(name)
        for dependency in dependencies[name]:
            visit(dependency)
        ordered.append(by_name[name])

    for name in names:
        visit(name)

    return ordered


def _imported_names(module: types.ModuleType) -> set[str]:
    """Collect the absolute names of the modules imported by a module's source file."""

    file_path = module.__dict__.get("__file__")
    if not isinstance(file_path, str) or not os.path.isfile(file_path):
        return set()

    try:
        with tokenize.open(file_path) as source_file:
            tree = ast.parse(source_file.read(), filename=file_path)
    except (OSError, SyntaxError, UnicodeDecodeError, ValueError):
        # The reload reports the error; the order does not matter for a module that fails.
        return set()

    package = module.__dict__.get("__package__") or ""
    names: set[str] = set()
    for node in ast.walk(tree):
        if isinstance(node, ast.Import):
            for alias in node.names:
                _add_name_and_parents(names, alias.name)
        elif isinstance(node, ast.ImportFrom):
            base = _resolve_import_base(node, package)
            if base is None:
                continue

            _add_name_and_parents(names, base)
            for alias in node.names:
                names.add(f"{base}.{alias.name}")

    return names


def _resolve_import_base(node: ast.ImportFrom, package: str) -> str | None:
    if node.level == 0:
        return node.module

    package_parts = package.split(".") if package else []
    if node.level - 1 > len(package_parts):
        return None

    base_parts = package_parts[: len(package_parts) - (node.level - 1)]
    if node.module:
        base_parts.append(node.module)

    return ".".join(base_parts) or None


def _add_name_and_parents(names: set[str], dotted_name: str) -> None:
    parts = dotted_name.split(".")
    for index in range(1, len(parts) + 1):
        names.add(".".join(parts[:index]))


class _Replacements:
    """Pairs of previous and reloaded definitions collected from reloaded modules."""

    def __init__(self) -> None:
        self.functions: list[tuple[types.FunctionType, types.FunctionType]] = []
        self.classes: list[tuple[type, type]] = []
        # id(previous) -> (previous, reloaded); keeps the previous objects alive for the id lookups.
        self.by_id: dict[int, tuple[Any, Any]] = {}

    def collect_module(self, module: types.ModuleType, old_namespace: dict[str, Any]) -> None:
        name = module.__name__
        namespace = module.__dict__
        for key, old_value in old_namespace.items():
            new_value = namespace.get(key)
            if new_value is None or new_value is old_value:
                continue

            if _is_function_pair(old_value, new_value) and old_value.__module__ == name:
                self._add_function(old_value, new_value)
            elif _is_class_pair(old_value, new_value) and old_value.__module__ == name:
                self._add_class(old_value, new_value)

    def _add_function(self, old: types.FunctionType, new: types.FunctionType) -> None:
        if id(old) in self.by_id:
            return

        self.by_id[id(old)] = (old, new)
        self.functions.append((old, new))

    def _add_class(self, old: type, new: type) -> None:
        if id(old) in self.by_id:
            return

        self.by_id[id(old)] = (old, new)
        self.classes.append((old, new))

        new_members = vars(new)
        for key, old_member in vars(old).items():
            new_member = new_members.get(key)
            if new_member is None or new_member is old_member:
                continue

            for old_function, new_function in _member_function_pairs(old_member, new_member):
                self._add_function(old_function, new_function)

            if _is_class_pair(old_member, new_member) and old_member.__module__ == old.__module__:
                self._add_class(old_member, new_member)

    def apply(self, report: ReloadReport) -> None:
        for old, new in self.functions:
            if _update_function(old, new):
                report.functions += 1

        updated_classes = [(old, new) for old, new in self.classes if _supports_class_update(old, new)]
        replaced_class_ids = {id(old) for old, _ in updated_classes}
        for old, new in updated_classes:
            _rebase_subclasses(old, new, replaced_class_ids)

        for old, new in updated_classes:
            _retarget_class_cells(old, new)
            if not _forward_class(old, new):
                _update_class_in_place(old, new)

        report.classes = len(updated_classes)
        report.instances = _reclass_instances(updated_classes)
        report.rebound = _rebind_module_globals(self.by_id)


def _is_function_pair(old: Any, new: Any) -> bool:
    return (
        isinstance(old, types.FunctionType)
        and isinstance(new, types.FunctionType)
        and old.__qualname__ == new.__qualname__
    )


def _is_class_pair(old: Any, new: Any) -> bool:
    return isinstance(old, type) and isinstance(new, type) and old.__qualname__ == new.__qualname__


def _member_function_pairs(old: Any, new: Any) -> list[tuple[types.FunctionType, types.FunctionType]]:
    if _is_function_pair(old, new):
        return [(old, new)]

    if (isinstance(old, staticmethod) and isinstance(new, staticmethod)) or (
        isinstance(old, classmethod) and isinstance(new, classmethod)
    ):
        return [(old.__func__, new.__func__)] if _is_function_pair(old.__func__, new.__func__) else []

    if isinstance(old, property) and isinstance(new, property):
        accessors = ((old.fget, new.fget), (old.fset, new.fset), (old.fdel, new.fdel))
        return [(old_accessor, new_accessor) for old_accessor, new_accessor in accessors if _is_function_pair(old_accessor, new_accessor)]

    return []


def _member_functions(member: Any) -> list[types.FunctionType]:
    if isinstance(member, types.FunctionType):
        return [member]

    if isinstance(member, (staticmethod, classmethod)) and isinstance(member.__func__, types.FunctionType):
        return [member.__func__]

    if isinstance(member, property):
        return [accessor for accessor in (member.fget, member.fset, member.fdel) if isinstance(accessor, types.FunctionType)]

    return []


def _update_function(old: types.FunctionType, new: types.FunctionType) -> bool:
    """Make a previous function object run the reloaded function's code."""

    if old.__code__.co_freevars != new.__code__.co_freevars:
        # The closure layout changed; the previous function cannot take the new code.
        return False

    old.__code__ = new.__code__
    old.__defaults__ = new.__defaults__
    old.__kwdefaults__ = new.__kwdefaults__
    old.__doc__ = new.__doc__
    old.__dict__.update(new.__dict__)
    for old_cell, new_cell in zip(old.__closure__ or (), new.__closure__ or ()):
        if old_cell is not new_cell and _cell_is_set(new_cell):
            old_cell.cell_contents = new_cell.cell_contents

    return True


def _cell_is_set(cell: types.CellType) -> bool:
    try:
        cell.cell_contents
    except ValueError:
        return False

    return True


def _retarget_class_cells(old: type, new: type) -> None:
    """Point the ``__class__`` cell used by ``super()`` in the previous methods at the reloaded class."""

    for member in vars(old).values():
        for function in _member_functions(member):
            freevars = function.__code__.co_freevars
            if "__class__" not in freevars:
                continue

            cell = function.__closure__[freevars.index("__class__")]
            if _cell_is_set(cell) and cell.cell_contents is old:
                cell.cell_contents = new


def _supports_class_update(old: type, new: type) -> bool:
    return type(old) is type(new) and _is_plain_python_class(old) and _is_plain_python_class(new)


def _is_plain_python_class(cls: type) -> bool:
    # Enums and other custom metaclasses keep per-class state (members, registries)
    # that cannot be redirected safely; they are only rebound.
    metaclass = type(cls)
    if metaclass is not type and not issubclass(metaclass, abc.ABCMeta):
        return False

    return not issubclass(cls, (ue.WrapperBase, ue.EnumBase))


def _rebase_subclasses(old: type, new: type, replaced_class_ids: set[int]) -> None:
    """Make subclasses that were not reloaded derive from the reloaded class."""

    for subclass in old.__subclasses__():
        if id(subclass) in replaced_class_ids or not _is_plain_python_class(subclass):
            continue

        try:
            subclass.__bases__ = tuple(new if base is old else base for base in subclass.__bases__)
        except TypeError:
            # Incompatible layout or MRO; the subclass keeps the previous class, which is
            # forwarded to or updated from the reloaded class.
            continue


def _forward_class(old: type, new: type) -> bool:
    """Turn the previous class into an empty subclass of the reloaded class."""

    try:
        old.__bases__ = (new,)
    except TypeError:
        # Python only allows this when the previous base has a compatible layout,
        # which excludes classes deriving directly from ``object`` and ``__slots__`` changes.
        return False

    for key in list(vars(old)):
        if key not in _KEPT_CLASS_ATTRIBUTES:
            _delete_class_attribute(old, key)

    return True


def _update_class_in_place(old: type, new: type) -> None:
    """Copy the reloaded class's members onto the previous class."""

    new_members = vars(new)
    for key, value in list(vars(old).items()):
        if key not in new_members and not _is_layout_attribute(key, value):
            _delete_class_attribute(old, key)

    for key, value in new_members.items():
        if _is_layout_attribute(key, value) or vars(old).get(key) is value:
            continue

        try:
            setattr(old, key, value)
        except (AttributeError, TypeError):
            continue


def _is_layout_attribute(key: str, value: Any) -> bool:
    return key in _KEPT_CLASS_ATTRIBUTES or isinstance(value, types.MemberDescriptorType)


def _delete_class_attribute(cls: type, key: str) -> None:
    try:
        delattr(cls, key)
    except (AttributeError, TypeError):
        # Read-only special attributes differ between Python versions.
        return


def _reclass_instances(class_pairs: list[tuple[type, type]]) -> int:
    if not class_pairs:
        return 0

    new_class_by_old_id = {id(old): new for old, new in class_pairs}
    count = 0
    for obj in gc.get_objects():
        new_class = new_class_by_old_id.get(id(type(obj)))
        if new_class is None:
            continue

        try:
            object.__setattr__(obj, "__class__", new_class)
        except TypeError:
            # The instance layout differs; the instance keeps the previous class, which
            # forwards to or was updated from the reloaded class.
            continue

        count += 1

    return count


def _rebind_module_globals(replacements: dict[int, tuple[Any, Any]]) -> int:
    count = 0
    for module in list(sys.modules.values()):
        if not isinstance(module, types.ModuleType):
            continue

        namespace = module.__dict__
        for key, value in list(namespace.items()):
            replacement = replacements.get(id(value))
            if replacement is not None and replacement[0] is value:
                namespace[key] = replacement[1]
                count += 1

    return count


def _log_report(report: ReloadReport) -> None:
    if report.reloaded:
        ue.Log(
            f"{_LOG_PREFIX} Reloaded {len(report.reloaded)} module(s): {', '.join(report.reloaded)} "
            f"({report.functions} functions, {report.classes} classes, {report.instances} instances updated; "
            f"{report.rebound} references rebound; {report.kept} globals kept)"
        )

    for name, error in report.failed.items():
        ue.LogError(f"{_LOG_PREFIX} Failed to reload {name}; the previous version stays active.\n{error}")
