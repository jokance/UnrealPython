// Copyright Epic Games, Inc. All Rights Reserved.

#include "UPyCommon.h"
#include "Utils/UPyUtil.h"
#include "Utils/UPyDelegateUtil.h"
#include "Wrapper/UPyWrapperDelegate.h"

struct FMethods_WrapperDelegate
{
	static PyObject* Cast(PyTypeObject* InType, PyObject* InArgs)
	{
		PyObject* PyObj = nullptr;
		if (PyArg_ParseTuple(InArgs, "O:cast", &PyObj))
		{
			PyObject* PyCastResult = (PyObject*)FUPyWrapperDelegate::CastPyObject(PyObj, InType);
			if (!PyCastResult)
			{
				UPyUtil::SetPythonError(PyExc_TypeError, InType, *FString::Printf(TEXT("Cannot cast type '%s' to '%s'"), *UPyUtil::GetFriendlyTypename(PyObj), *UPyUtil::GetFriendlyTypename(InType)));
			}
			return PyCastResult;
		}

		return nullptr;
	}

	// static PyObject* Copy(FUPyWrapperDelegate* InSelf)
	// {
	// 	if (!FUPyWrapperDelegate::ValidateInternalState(InSelf))
	// 	{
	// 		return nullptr;
	// 	}
	//
	// 	const UPyGenUtil::FGeneratedWrappedFunction& DelegateSignature = FUPyWrapperDelegateMetaData::GetDelegateSignature(InSelf);
	// 	return (PyObject*)FUPyWrapperDelegateFactory::Get().CreateInstance(DelegateSignature.Func, InSelf->DelegateInstance, FUPyWrapperOwnerContext(), EUPyConversionMethod::Copy);
	// }

	static PyObject* IsBound(FUPyWrapperDelegate* InSelf)
	{
		if (!FUPyWrapperDelegate::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		if (InSelf->DelegateInstance->IsBound())
		{
			Py_RETURN_TRUE;
		}

		Py_RETURN_FALSE;
	}

	static PyObject* BindDelegate(FUPyWrapperDelegate* InSelf, PyObject* InArgs)
	{
		if (!FUPyWrapperDelegate::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		PyObject* PyObj = nullptr;
		if (!PyArg_ParseTuple(InArgs, "O:bind_delegate", &PyObj))
		{
			return nullptr;
		}

		FUPyWrapperDelegate* PyOtherDelegate = FUPyWrapperDelegate::CastPyObject(PyObj, Py_TYPE(InSelf));
		if (!PyOtherDelegate)
		{
			UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert argument 0 (%s) to '%s'"), *UPyUtil::GetFriendlyTypename(PyObj), *UPyUtil::GetFriendlyTypename(InSelf)));
			return nullptr;
		}

		*InSelf->DelegateInstance = *PyOtherDelegate->DelegateInstance;

		Py_RETURN_NONE;
	}

	static PyObject* BindFunction(FUPyWrapperDelegate* InSelf, PyObject* InArgs)
	{
		if (!FUPyWrapperDelegate::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		// Unbind previous binding to ensure cleanup of UUPyCallableForDelegate if any
		FUPyWrapperDelegate::Unbind(InSelf);

		// const UPyGenUtil::FGeneratedWrappedFunction& DelegateSignature = FUPyWrapperDelegateMetaData::GetDelegateSignature(InSelf);
		if (!UPyDelegateUtil::PythonArgsToDelegate_ObjectAndName(InArgs, InSelf->DelegateProp->SignatureFunction, *InSelf->DelegateInstance, TEXT("bind_function"), *UPyUtil::GetErrorContext(InSelf)))
		{
			return nullptr;
		}

		Py_RETURN_NONE;
	}

	static PyObject* BindCallable(FUPyWrapperDelegate* InSelf, PyObject* InArgs)
	{
		if (!FUPyWrapperDelegate::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		// Unbind previous binding to ensure cleanup of UUPyCallableForDelegate if any
		FUPyWrapperDelegate::Unbind(InSelf);

		// const UPyGenUtil::FGeneratedWrappedFunction& DelegateSignature = FUPyWrapperDelegateMetaData::GetDelegateSignature(InSelf);
		// const UClass* PythonCallableForDelegateClass = FUPyWrapperDelegateMetaData::GetPythonCallableForDelegateClass(InSelf);

		UUPyCallableForDelegate* PythonCallableForDelegate = UPyDelegateUtil::PythonArgsToDelegate_Callable(InArgs, InSelf->DelegateProp->SignatureFunction, *InSelf->DelegateInstance, TEXT("bind_callable"), *UPyUtil::GetErrorContext(InSelf));
		if (!PythonCallableForDelegate)
		{
			return nullptr;
		}

		Py_RETURN_NONE;
	}

	static PyObject* Unbind(FUPyWrapperDelegate* InSelf)
	{
		if (!FUPyWrapperDelegate::Unbind(InSelf))
		{
			return nullptr;
		}
		Py_RETURN_NONE;
	}

	static PyObject* Execute(FUPyWrapperDelegate* InSelf, PyObject* InArgs)
	{
		return FUPyWrapperDelegate::CallDelegate(InSelf, InArgs);
	}

	static PyObject* ExecuteIfBound(FUPyWrapperDelegate* InSelf, PyObject* InArgs)
	{
		if (!FUPyWrapperDelegate::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		if (InSelf->DelegateInstance->IsBound())
		{
			return FUPyWrapperDelegate::CallDelegate(InSelf, InArgs);
		}

		Py_RETURN_NONE;
	}
};

// NOTE: The delegate couldn't be strictly type-hinted. The exact return/param types of the delegate are unknown here. In Python 3.11, we should be able
//       to use Self for return type of __copy__() and copy() methods.
//       _T = TypeVar('_T') and Type/Any/Callable/Union are defines by the Python typing module.
PyMethodDef DelegatePyMethodDefs[] = {
	{ "Cast", UPyCFunctionCast(&FMethods_WrapperDelegate::Cast), METH_VARARGS | METH_CLASS, UPyDoc_STR("Cast(cls: Type[_T], object: object) -> _T -- cast the given object to this Unreal delegate type") },
	//{ "__copy__", UPyCFunctionCast(&Copy), METH_NOARGS, UPyDoc_STR("__copy__(self) -> Any -- copy this Unreal delegate") },
	//{ "copy", UPyCFunctionCast(&Copy), METH_NOARGS, UPyDoc_STR("copy(self) -> Any -- copy this Unreal delegate") },
	{ "IsBound", UPyCFunctionCast(&FMethods_WrapperDelegate::IsBound), METH_NOARGS, UPyDoc_STR("IsBound(self) -> bool -- is this Unreal delegate bound to something?") },
	// { "BindDelegate", UPyCFunctionCast(&FMethods_WrapperDelegate::BindDelegate), METH_VARARGS, UPyDoc_STR("BindDelegate(self, delegate: DelegateBase) -> None -- bind this Unreal delegate to the same object and function as another delegate") },
	{ "BindFunction", UPyCFunctionCast(&FMethods_WrapperDelegate::BindFunction), METH_VARARGS, UPyDoc_STR("BindFunction(self, obj: Object, name: str) -> None -- bind this Unreal delegate to a named Unreal function on the given object instance") },
	{ "Bind", UPyCFunctionCast(&FMethods_WrapperDelegate::BindCallable), METH_VARARGS, UPyDoc_STR("Bind(self, callable: Callable[..., Any]) -> None -- bind this Unreal delegate to a Python callable") },
	{ "Unbind", UPyCFunctionCast(&FMethods_WrapperDelegate::Unbind), METH_NOARGS, UPyDoc_STR("Unbind(self) -> None -- unbind this Unreal delegate") },
	{ "Execute", UPyCFunctionCast(&FMethods_WrapperDelegate::Execute), METH_VARARGS, UPyDoc_STR("Execute(self, *args: Any) -> Any -- call this Unreal delegate, but error if it's unbound") },
	{ "ExecuteIfBound", UPyCFunctionCast(&FMethods_WrapperDelegate::ExecuteIfBound), METH_VARARGS, UPyDoc_STR("ExecuteIfBound(self, *args: Any) -> Any -- call this Unreal delegate, but only if it's bound to something") },

	{ nullptr, nullptr, 0, nullptr }
};
