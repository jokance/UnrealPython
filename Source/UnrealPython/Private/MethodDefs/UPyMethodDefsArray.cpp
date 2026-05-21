
#include "UPyCommon.h"
#include "Utils/UPyUtil.h"
#include "Wrapper/UPyWrapperArray.h"
#include "Wrapper/UPyWrapperTypeFactory.h"

struct FMethods_WrapperArray
{

	// This is for type hinting, to make the class appears as a generic (https://peps.python.org/pep-0560/) and support hinting like 'a: unreal.Array[int] = unreal.Array(int)'
	static PyObject* GetClassItem(PyObject* Type, PyObject* Item)
	{
		Py_INCREF(Type);
		return Type;
	}

	static PyObject* Cast(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
	{
		PyObject* PyTypeObj = nullptr;
		PyObject* PyObj = nullptr;

		static const char *ArgsKwdList[] = { "type", "obj", nullptr };
		if (PyArg_ParseTupleAndKeywords(InArgs, InKwds, "OO:call", (char**)ArgsKwdList, &PyTypeObj, &PyObj))
		{
			UPyUtil::FPropertyDef ArrayElementDef;
			if (UPyUtil::ValidateContainerTypeParam(PyTypeObj, ArrayElementDef, "type", *UPyUtil::GetErrorContext(InType)) != 0)
			{
				return nullptr;
			}

			PyObject* PyCastResult = (PyObject*)FUPyWrapperArray::CastPyObject(PyObj, InType, ArrayElementDef);
			if (!PyCastResult)
			{
				UPyUtil::SetPythonError(PyExc_TypeError, InType, *FString::Printf(TEXT("Cannot cast type '%s' to '%s'"), *UPyUtil::GetFriendlyTypename(PyObj), *UPyUtil::GetFriendlyTypename(InType)));
			}
			return PyCastResult;
		}

		return nullptr;
	}

	static PyObject* Copy(FUPyWrapperArray* InSelf)
	{
		if (!FUPyWrapperArray::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		return (PyObject*)FUPyWrapperArrayFactory::Get().CreateInstance(InSelf->ArrayInstance, InSelf->ArrayProp, FUPyWrapperOwnerContext(), EUPyConversionMethod::Copy);
	}

	static PyObject* Append(FUPyWrapperArray* InSelf, PyObject* InArgs)
	{
		PyObject* PyObj = nullptr;
		if (!PyArg_ParseTuple(InArgs, "O:append", &PyObj))
		{
			return nullptr;
		}

		if (FUPyWrapperArray::Append(InSelf, PyObj) != 0)
		{
			return nullptr;
		}

		Py_RETURN_NONE;
	}

	static PyObject* Count(FUPyWrapperArray* InSelf, PyObject* InArgs)
	{
		PyObject* PyObj = nullptr;
		if (!PyArg_ParseTuple(InArgs, "O:count", &PyObj))
		{
			return nullptr;
		}

		const int32 ValueCount = FUPyWrapperArray::Count(InSelf, PyObj);
		if (ValueCount == -1)
		{
			return nullptr;
		}

		return UPyConversion::Pythonize(ValueCount);
	}

	static PyObject* Extend(FUPyWrapperArray* InSelf, PyObject* InArgs)
	{
		PyObject* PyObj = nullptr;
		if (!PyArg_ParseTuple(InArgs, "O:extend", &PyObj))
		{
			return nullptr;
		}

		if (FUPyWrapperArray::ConcatInplace(InSelf, PyObj) != 0)
		{
			return nullptr;
		}

		Py_RETURN_NONE;
	}

	static PyObject* Index(FUPyWrapperArray* InSelf, PyObject* InArgs, PyObject* InKwds)
	{
		PyObject* PyValueObj = nullptr;
		PyObject* PyStartObj = nullptr;
		PyObject* PyStopObj = nullptr;

		static const char *ArgsKwdList[] = { "value", "start", "stop", nullptr };
		if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "O|OO:index", (char**)ArgsKwdList, &PyValueObj, &PyStartObj, &PyStopObj))
		{
			return nullptr;
		}

		int32 StartIndex = 0;
		if (PyStartObj && !UPyConversion::Nativize(PyStartObj, StartIndex))
		{
			UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'start' (%s) to 'int32'"), *UPyUtil::GetFriendlyTypename(PyStartObj)));
			return nullptr;
		}

		int32 StopIndex = -1;
		if (PyStopObj && !UPyConversion::Nativize(PyStopObj, StopIndex))
		{
			UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'stop' (%s) to 'int32'"), *UPyUtil::GetFriendlyTypename(PyStopObj)));
			return nullptr;
		}

		const int32 ValueIndex = FUPyWrapperArray::Index(InSelf, PyValueObj, StartIndex, StopIndex);
		if (ValueIndex == -1)
		{
			return nullptr;
		}

		return UPyConversion::Pythonize(ValueIndex);
	}

	static PyObject* Insert(FUPyWrapperArray* InSelf, PyObject* InArgs, PyObject* InKwds)
	{
		PyObject* PyIndexObj = nullptr;
		PyObject* PyValueObj = nullptr;

		static const char *ArgsKwdList[] = { "index", "value", nullptr };
		if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "OO:insert", (char**)ArgsKwdList, &PyIndexObj, &PyValueObj))
		{
			return nullptr;
		}

		int32 InsertIndex = 0;
		if (!UPyConversion::Nativize(PyIndexObj, InsertIndex))
		{
			UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'index' (%s) to 'int32'"), *UPyUtil::GetFriendlyTypename(PyIndexObj)));
			return nullptr;
		}

		if (FUPyWrapperArray::Insert(InSelf, InsertIndex, PyValueObj) != 0)
		{
			return nullptr;
		}

		Py_RETURN_NONE;
	}

	static PyObject* Pop(FUPyWrapperArray* InSelf, PyObject* InArgs)
	{
		PyObject* PyObj = nullptr;
		if (!PyArg_ParseTuple(InArgs, "|O:pop", &PyObj))
		{
			return nullptr;
		}

		int32 ValueIndex = INDEX_NONE;
		if (PyObj && !UPyConversion::Nativize(PyObj, ValueIndex))
		{
			UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'index' (%s) to 'int32'"), *UPyUtil::GetFriendlyTypename(PyObj)));
			return nullptr;
		}

		return FUPyWrapperArray::Pop(InSelf, ValueIndex);
	}

	static PyObject* Clear(FUPyWrapperArray* InSelf)
	{
		if (FUPyWrapperArray::Clear(InSelf) != 0)
		{
			return nullptr;
		}

		Py_RETURN_NONE;
	}

	static PyObject* Remove(FUPyWrapperArray* InSelf, PyObject* InArgs)
	{
		PyObject* PyObj = nullptr;
		if (!PyArg_ParseTuple(InArgs, "O:remove", &PyObj))
		{
			return nullptr;
		}

		if (FUPyWrapperArray::Remove(InSelf, PyObj) != 0)
		{
			return nullptr;
		}

		Py_RETURN_NONE;
	}

	static PyObject* Reverse(FUPyWrapperArray* InSelf)
	{
		if (FUPyWrapperArray::Reverse(InSelf) != 0)
		{
			return nullptr;
		}

		Py_RETURN_NONE;
	}

	static PyObject* Sort(FUPyWrapperArray* InSelf, PyObject* InArgs, PyObject* InKwds)
	{
		PyObject* PyKeyObj = nullptr;
		PyObject* PyReverseObj = nullptr;

		static const char *ArgsKwdList[] = { "key", "reverse", nullptr };
		if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "|OO:sort", (char**)ArgsKwdList, &PyKeyObj, &PyReverseObj))
		{
			return nullptr;
		}

		bool bReverse = false;
		if (PyReverseObj && !UPyConversion::Nativize(PyReverseObj, bReverse))
		{
			UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'reverse' (%s) to 'bool'"), *UPyUtil::GetFriendlyTypename(PyReverseObj)));
			return nullptr;
		}

		if (FUPyWrapperArray::Sort(InSelf, PyKeyObj, bReverse) != 0)
		{
			return nullptr;
		}

		Py_RETURN_NONE;
	}

	static PyObject* Resize(FUPyWrapperArray* InSelf, PyObject* InArgs)
	{
		PyObject* PyObj = nullptr;
		if (!PyArg_ParseTuple(InArgs, "O:resize", &PyObj))
		{
			return nullptr;
		}

		int32 Len = 0;
		if (UPyUtil::ValidateContainerLenParam(PyObj, Len, "len", *UPyUtil::GetErrorContext(InSelf)) != 0)
		{
			return nullptr;
		}

		if (FUPyWrapperArray::Resize(InSelf, Len) != 0)
		{
			return nullptr;
		}

		Py_RETURN_NONE;
	}
};


// NOTE: _ElemType = TypeVar('_ElemType') and Type/Optional/Iterable/Callable are defines by the Python typing module.
PyMethodDef ArrayPyMethodDefs[] = {
	{ "__class_getitem__", UPyCFunctionCast(FMethods_WrapperArray::GetClassItem), METH_O | METH_CLASS, UPyDoc_STR("__class_getitem__(cls, item: _ElemType) -- implemented for type hinting purpose only") },
	{ "Cast", UPyCFunctionCast(&FMethods_WrapperArray::Cast), METH_VARARGS | METH_KEYWORDS | METH_CLASS, UPyDoc_STR("Cast(cls, type: Type[_ElemType], obj: object) -> Array[_ElemType] -- cast the given object to this Unreal array type") },
	{ "__copy__", UPyCFunctionCast(&FMethods_WrapperArray::Copy), METH_NOARGS, UPyDoc_STR("__copy__(self) -> Array[_ElemType] -- copy this Unreal array") },
	{ "Copy", UPyCFunctionCast(&FMethods_WrapperArray::Copy), METH_NOARGS, UPyDoc_STR("Copy(self) -> Array[_ElemType] -- copy this Unreal array") },
	{ "Append", UPyCFunctionCast(&FMethods_WrapperArray::Append), METH_VARARGS, UPyDoc_STR("Append(self, value: _ElemType) -> None -- append the given value to the end of this Unreal array (equivalent to TArray::Add in C++)") },
	{ "Count", UPyCFunctionCast(&FMethods_WrapperArray::Count), METH_VARARGS, UPyDoc_STR("Count(self, value: _ElemType) -> int -- return the number of times that value appears in this this Unreal array") },
	{ "Extend", UPyCFunctionCast(&FMethods_WrapperArray::Extend), METH_VARARGS, UPyDoc_STR("Extend(self, values: Iterable[_ElemType]) -> None -- extend this Unreal array by appending elements from the given iterable (equivalent to TArray::Append in C++)") },
	{ "Index", UPyCFunctionCast(&FMethods_WrapperArray::Index), METH_VARARGS | METH_KEYWORDS, UPyDoc_STR("Index(self, value: _ElemType, start: int = 0, stop: int = -1) -> int -- get the index of the first matching value in this Unreal array, or raise ValueError if missing (equivalent to TArray::IndexOfByKey in C++)") },
	{ "Insert", UPyCFunctionCast(&FMethods_WrapperArray::Insert), METH_VARARGS | METH_KEYWORDS, UPyDoc_STR("Insert(self, index: int, value: _ElemType) -> None -- insert the given value at the given index in this Unreal array") },
	{ "Pop", UPyCFunctionCast(&FMethods_WrapperArray::Pop), METH_VARARGS, UPyDoc_STR("Pop(self, index: int = -1) -> _ElemType -- remove and return the value at the given index in this Unreal array, or raise IndexError if the index is out-of-bounds") },
	{ "Clear", UPyCFunctionCast(&FMethods_WrapperArray::Clear), METH_NOARGS, UPyDoc_STR("Clear(self) -> None -- remove all values from this Unreal array") },
	{ "Remove", UPyCFunctionCast(&FMethods_WrapperArray::Remove), METH_VARARGS, UPyDoc_STR("Remove(self, value: _ElemType) -> None -- remove the first matching value in this Unreal array, or raise ValueError if missing") },
	{ "Reverse", UPyCFunctionCast(&FMethods_WrapperArray::Reverse), METH_NOARGS, UPyDoc_STR("Reverse(self) -> None -- reverse this Unreal array in-place") },
	{ "Sort", UPyCFunctionCast(&FMethods_WrapperArray::Sort), METH_VARARGS | METH_KEYWORDS, UPyDoc_STR("Sort(self, key: Optional[Callable[[_ElemType], object]]=None, reverse: bool=False) -> None -- stable sort this Unreal array in-place") },
	{ "Resize", UPyCFunctionCast(&FMethods_WrapperArray::Resize), METH_VARARGS, UPyDoc_STR("Resize(self, len: int) -> None -- resize this Unreal array to hold the given number of elements") },
	{ nullptr, nullptr, 0, nullptr }
};
