
#include "UPyCommon.h"
#include "Utils/UPyUtil.h"
#include "Wrapper/UPyWrapperMap.h"
#include "Wrapper/UPyWrapperTypeFactory.h"

// This is for type hinting, to make the class appears as a generic (https://peps.python.org/pep-0560/) and support hinting like 'm: unreal.Map[int, str] = unreal.Map(int, str)'
struct FMethods_WrapperMap
{
	static PyObject* GetClassItem(PyObject* Type, PyObject* Item)
	{
		Py_INCREF(Type);
		return Type;
	}

	static PyObject* Cast(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
	{
		PyObject* PyKeyObj = nullptr;
		PyObject* PyValueObj = nullptr;
		PyObject* PyObj = nullptr;

		static const char *ArgsKwdList[] = { "key", "value", "obj", nullptr };
		if (PyArg_ParseTupleAndKeywords(InArgs, InKwds, "OOO:call", (char**)ArgsKwdList, &PyKeyObj, &PyValueObj, &PyObj))
		{
			UPyUtil::FPropertyDef MapKeyDef;
			if (UPyUtil::ValidateContainerTypeParam(PyKeyObj, MapKeyDef, "key", *UPyUtil::GetErrorContext(InType)) != 0)
			{
				return nullptr;
			}

			UPyUtil::FPropertyDef MapValueDef;
			if (UPyUtil::ValidateContainerTypeParam(PyValueObj, MapValueDef, "value", *UPyUtil::GetErrorContext(InType)) != 0)
			{
				return nullptr;
			}

			PyObject* PyCastResult = (PyObject*)FUPyWrapperMap::CastPyObject(PyObj, InType, MapKeyDef, MapValueDef);
			if (!PyCastResult)
			{
				UPyUtil::SetPythonError(PyExc_TypeError, InType, *FString::Printf(TEXT("Cannot cast type '%s' to '%s'"), *UPyUtil::GetFriendlyTypename(PyObj), *UPyUtil::GetFriendlyTypename(InType)));
			}
			return PyCastResult;
		}

		return nullptr;
	}

	static PyObject* Copy(FUPyWrapperMap* InSelf)
	{
		if (!FUPyWrapperMap::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		return (PyObject*)FUPyWrapperMapFactory::Get().CreateInstance(InSelf->MapInstance, InSelf->MapProp, FUPyWrapperOwnerContext(), EUPyConversionMethod::Copy);
	}

	static PyObject* Clear(FUPyWrapperMap* InSelf)
	{
		if (FUPyWrapperMap::Clear(InSelf) != 0)
		{
			return nullptr;
		}

		Py_RETURN_NONE;
	}

	static PyObject* FromKeys(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
	{
		PyObject* PySequenceObj = nullptr;
		PyObject* PyValueObj = nullptr;

		static const char *ArgsKwdList[] = { "sequence", "value", nullptr };
		if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "O|O:fromkeys", (char**)ArgsKwdList, &PySequenceObj, &PyValueObj))
		{
			return nullptr;
		}

		return (PyObject*)FUPyWrapperMap::FromKeys(PySequenceObj, PyValueObj ? PyValueObj : Py_None, InType);
	}

	static PyObject* Get(FUPyWrapperMap* InSelf, PyObject* InArgs, PyObject* InKwds)
	{
		PyObject* PyKeyObj = nullptr;
		PyObject* PyDefaultObj = nullptr;

		static const char *ArgsKwdList[] = { "key", "default", nullptr };
		if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "O|O:get", (char**)ArgsKwdList, &PyKeyObj, &PyDefaultObj))
		{
			return nullptr;
		}

		return FUPyWrapperMap::GetItem(InSelf, PyKeyObj, PyDefaultObj ? PyDefaultObj : Py_None);
	}

	static PyObject* SetDefault(FUPyWrapperMap* InSelf, PyObject* InArgs, PyObject* InKwds)
	{
		PyObject* PyKeyObj = nullptr;
		PyObject* PyDefaultObj = nullptr;

		static const char *ArgsKwdList[] = { "key", "default", nullptr };
		if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "O|O:setdefault", (char**)ArgsKwdList, &PyKeyObj, &PyDefaultObj))
		{
			return nullptr;
		}

		return FUPyWrapperMap::SetDefault(InSelf, PyKeyObj, PyDefaultObj);
	}

	static PyObject* Pop(FUPyWrapperMap* InSelf, PyObject* InArgs, PyObject* InKwds)
	{
		PyObject* PyKeyObj = nullptr;
		PyObject* PyDefaultObj = nullptr;

		static const char *ArgsKwdList[] = { "key", "default", nullptr };
		if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "O|O:pop", (char**)ArgsKwdList, &PyKeyObj, &PyDefaultObj))
		{
			return nullptr;
		}

		return FUPyWrapperMap::Pop(InSelf, PyKeyObj, PyDefaultObj);
	}

	static PyObject* PopItem(FUPyWrapperMap* InSelf)
	{
		return FUPyWrapperMap::PopItem(InSelf);
	}

	static PyObject* Update(FUPyWrapperMap* InSelf, PyObject* InArgs, PyObject* InKwds)
	{
		PyObject* PyObj = nullptr;
		if (!PyArg_ParseTuple(InArgs, "|O:update", &PyObj))
		{
			return nullptr;
		}

		if (PyObj && FUPyWrapperMap::Update(InSelf, PyObj) != 0)
		{
			return nullptr;
		}

		if (InKwds && FUPyWrapperMap::Update(InSelf, InKwds) != 0)
		{
			return nullptr;
		}

		Py_RETURN_NONE;
	}

	static PyObject* Items(FUPyWrapperMap* InSelf)
	{
		return GetUPyWrapperMapItems(InSelf);
	}

	static PyObject* Keys(FUPyWrapperMap* InSelf)
	{
		return GetUPyWrapperMapKeys(InSelf);
	}

	static PyObject* Values(FUPyWrapperMap* InSelf)
	{
		return GetUPyWrapperMapValues(InSelf);
	}
};

// NOTE: _KeyType = TypeVar('_KeyType'), _ValueType = TypeVar('_ValueType'), Type/Iterable/Tuple/Union/List/ItemsView/KeysView/ValuesView are defines in the Python typing module.
PyMethodDef MapPyMethodDefs[] = {
	{ "__class_getitem__", UPyCFunctionCast(FMethods_WrapperMap::GetClassItem), METH_O|METH_CLASS, UPyDoc_STR("__class_getitem__(cls, item:_KeyType) -- implemented for type hinting purpose only") },
	{ "Cast", UPyCFunctionCast(&FMethods_WrapperMap::Cast), METH_VARARGS | METH_KEYWORDS | METH_CLASS, UPyDoc_STR("Cast(cls, key: Type[_KeyType], value: Type[_ValueType], obj: object) -> Map[_KeyType, _ValueType] -- cast the given object to this Unreal map type") },
	{ "__copy__", UPyCFunctionCast(&FMethods_WrapperMap::Copy), METH_NOARGS, UPyDoc_STR("__copy__(self) -> Map[_KeyType, _ValueType] -- copy this Unreal map") },
	{ "Copy", UPyCFunctionCast(&FMethods_WrapperMap::Copy), METH_NOARGS, UPyDoc_STR("Copy(self) -> Map[_KeyType, _ValueType] -- copy this Unreal map") },
	{ "Clear", UPyCFunctionCast(&FMethods_WrapperMap::Clear), METH_NOARGS, UPyDoc_STR("Clear(self) -> None -- remove all items from this Unreal map") },
	{ "FromKeys", UPyCFunctionCast(&FMethods_WrapperMap::FromKeys), METH_VARARGS | METH_KEYWORDS | METH_CLASS, UPyDoc_STR("FromKeys(cls, sequence: Iterable[_KeyType], value: _ValueType) -> Map[_KeyType, _ValueType] -- returns a new Unreal map of keys from the sequence using the given value (types are inferred)") },
	{ "Get", UPyCFunctionCast(&FMethods_WrapperMap::Get), METH_VARARGS | METH_KEYWORDS, UPyDoc_STR("Get(self, key: _KeyType, default: _ValueType=...) -> _ValueType -- x[key] if key in x, otherwise default") },
	{ "SetDefault", UPyCFunctionCast(&FMethods_WrapperMap::SetDefault), METH_VARARGS | METH_KEYWORDS, UPyDoc_STR("SetDefault(self, key: _KeyType, default: _ValueType=...) -> _ValueType -- set key to default if key not in x and return the value of key") },
	{ "Pop", UPyCFunctionCast(&FMethods_WrapperMap::Pop), METH_VARARGS | METH_KEYWORDS, UPyDoc_STR("Pop(self, key: _KeyType, default: _ValueType=...) -> _ValueType -- remove key and return its value, or default if key not present, or raise KeyError if no default") },
	{ "PopItem", UPyCFunctionCast(&FMethods_WrapperMap::PopItem), METH_NOARGS, UPyDoc_STR("PopItem(self) -> tuple[_KeyType, _ValueType] -- remove and return an arbitrary pair from this Unreal map, or raise KeyError if the map is empty") },
	{ "Update", UPyCFunctionCast(&FMethods_WrapperMap::Update), METH_VARARGS | METH_KEYWORDS, UPyDoc_STR("Update(self, pairs: Union[dict[_KeyType, _ValueType], Iterable[tuple[_KeyType, _ValueType]], Iterable[list[Union[_KeyType, _ValueType]]]]) -> None -- update this Unreal map from the given mapping or sequence pairs type or key->value arguments") },
	{ "Items", UPyCFunctionCast(&FMethods_WrapperMap::Items), METH_NOARGS, UPyDoc_STR("Items(self) -> ItemsView[_KeyType, _ValueType] -- a set-like view of the key->value pairs of this Unreal map") },
	{ "Keys", UPyCFunctionCast(&FMethods_WrapperMap::Keys), METH_NOARGS, UPyDoc_STR("Keys(self) -> KeysView[_KeyType] -- a set-like view of the keys of this Unreal map") },
	{ "Values", UPyCFunctionCast(&FMethods_WrapperMap::Values), METH_NOARGS, UPyDoc_STR("Values(self) -> ValuesView[_ValueType] -- a view of the values of this Unreal map") },
	{ nullptr, nullptr, 0, nullptr }
};
