
#include "UPyCommon.h"
#include "Utils/UPyUtil.h"
#include "Wrapper/UPyWrapperFixedArray.h"
#include "Wrapper/UPyWrapperTypeFactory.h"

// This is for type hinting, to make the class appears as a generic (https://peps.python.org/pep-0560/) and support hinting like 'a: unreal.FixedArray[int] = unreal.FixedArray(int)'
struct FMethods_WrapperFixedArray
{
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
			UPyUtil::FPropertyDef ArrayPropDef;
			if (UPyUtil::ValidateContainerTypeParam(PyTypeObj, ArrayPropDef, "type", *UPyUtil::GetErrorContext(InType)) != 0)
			{
				return nullptr;
			}

			PyObject* PyCastResult = (PyObject*)FUPyWrapperFixedArray::CastPyObject(PyObj, InType, ArrayPropDef);
			if (!PyCastResult)
			{
				UPyUtil::SetPythonError(PyExc_TypeError, InType, *FString::Printf(TEXT("Cannot cast type '%s' to '%s'"), *UPyUtil::GetFriendlyTypename(PyObj), *UPyUtil::GetFriendlyTypename(InType)));
			}
			return PyCastResult;
		}

		return nullptr;
	}

	static PyObject* Copy(FUPyWrapperFixedArray* InSelf)
	{
		if (!FUPyWrapperFixedArray::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		return (PyObject*)FUPyWrapperFixedArrayFactory::Get().CreateInstance(InSelf->ArrayInstance, InSelf->ArrayProp, FUPyWrapperOwnerContext(), EUPyConversionMethod::Copy);
	}
};
// NOTE: _ElemType = TypeVar('_ElemType') and TYpe is defined in the typing Python module.
PyMethodDef FixedArrayPyMethodDefs[] = {
	{ "__class_getitem__", UPyCFunctionCast(FMethods_WrapperFixedArray::GetClassItem), METH_O|METH_CLASS, UPyDoc_STR("__class_getitem__(cls, item: _ElemType) -- implemented for type hinting purpose only") },
	{ "Cast", UPyCFunctionCast(&FMethods_WrapperFixedArray::Cast), METH_VARARGS | METH_KEYWORDS | METH_CLASS, UPyDoc_STR("Cast(cls, type: Type[_ElemType], obj: object) -> FixedArray[_ElemType] -- cast the given object to this Unreal fixed-array type") },
	{ "__copy__", UPyCFunctionCast(&FMethods_WrapperFixedArray::Copy), METH_NOARGS, UPyDoc_STR("__copy__(self) -> FixedArray[_ElemType] -- copy this Unreal fixed-array") },
	{ "Copy", UPyCFunctionCast(&FMethods_WrapperFixedArray::Copy), METH_NOARGS, UPyDoc_STR("Copy(self) -> FixedArray[_ElemType] -- copy this Unreal fixed-array") },
	{ nullptr, nullptr, 0, nullptr }
};
