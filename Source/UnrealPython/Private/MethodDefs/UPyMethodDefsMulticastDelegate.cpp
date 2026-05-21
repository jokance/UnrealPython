
#include "UPyCommon.h"
#include "Utils/UPyUtil.h"
#include "Utils/UPyDelegateUtil.h"
#include "Wrapper/UPyWrapperDelegate.h"

struct FMethods_WrapperMulticastDelegate
{
	static PyObject* Cast(PyTypeObject* InType, PyObject* InArgs)
	{
		PyObject* PyObj = nullptr;
		if (PyArg_ParseTuple(InArgs, "O:cast", &PyObj))
		{
			PyObject* PyCastResult = (PyObject*)FUPyWrapperMulticastDelegate::CastPyObject(PyObj, InType);
			if (!PyCastResult)
			{
				UPyUtil::SetPythonError(PyExc_TypeError, InType, *FString::Printf(TEXT("Cannot cast type '%s' to '%s'"), *UPyUtil::GetFriendlyTypename(PyObj), *UPyUtil::GetFriendlyTypename(InType)));
			}
			return PyCastResult;
		}

		return nullptr;
	}

	// static PyObject* Copy(FUPyWrapperMulticastDelegate* InSelf)
	// {
	// 	if (!FUPyWrapperMulticastDelegate::ValidateInternalState(InSelf))
	// 	{
	// 		return nullptr;
	// 	}
	//
	// 	const UPyGenUtil::FGeneratedWrappedFunction& DelegateSignature = FUPyWrapperMulticastDelegateMetaData::GetDelegateSignature(InSelf);
	// 	return (PyObject*)FUPyWrapperMulticastDelegateFactory::Get().CreateInstance(DelegateSignature.Func, InSelf->DelegateInstance, FUPyWrapperOwnerContext(), EUPyConversionMethod::Copy);
	// }

	static PyObject* IsBound(FUPyWrapperMulticastDelegate* InSelf)
	{
		if (!FUPyWrapperMulticastDelegate::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		if (InSelf->DelegateInstance && InSelf->DelegateInstance->IsBound())
		{
			Py_RETURN_TRUE;
		}
		
		if (const FMulticastDelegateProperty* MulticastProp = CastField<FMulticastDelegateProperty>(InSelf->DelegateProp))
		{
			if (const FMulticastScriptDelegate* ScriptDelegate = MulticastProp->GetMulticastDelegate(InSelf->PropAddr))
			{
				if (ScriptDelegate->IsBound())
				{
					Py_RETURN_TRUE;
				}
			}
			Py_RETURN_FALSE;
		}

		Py_RETURN_FALSE;
	}

	static PyObject* AddFunction(FUPyWrapperMulticastDelegate* InSelf, PyObject* InArgs)
	{
		if (!FUPyWrapperMulticastDelegate::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		// const UPyGenUtil::FGeneratedWrappedFunction& DelegateSignature = FUPyWrapperMulticastDelegateMetaData::GetDelegateSignature(InSelf);

		FScriptDelegate Delegate;
		if (!UPyDelegateUtil::PythonArgsToDelegate_ObjectAndName(InArgs, InSelf->DelegateProp->SignatureFunction, Delegate, TEXT("add_function"), *UPyUtil::GetErrorContext(InSelf)))
		{
			return nullptr;
		}

		if (InSelf->DelegateInstance)
		{
			InSelf->DelegateInstance->Add(Delegate);
			Py_RETURN_NONE;
		}

		if (const FMulticastDelegateProperty* MulticastProp = CastField<FMulticastDelegateProperty>(InSelf->DelegateProp))
		{
			MulticastProp->AddDelegate(MoveTemp(Delegate), nullptr, InSelf->PropAddr);
			Py_RETURN_NONE;
		}

		Py_RETURN_NONE;
	}

	static PyObject* AddCallable(FUPyWrapperMulticastDelegate* InSelf, PyObject* InArgs)
	{
		if (!FUPyWrapperMulticastDelegate::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		// const UPyGenUtil::FGeneratedWrappedFunction& DelegateSignature = FUPyWrapperMulticastDelegateMetaData::GetDelegateSignature(InSelf);
		// const UClass* PythonCallableForDelegateClass = FUPyWrapperMulticastDelegateMetaData::GetPythonCallableForDelegateClass(InSelf);

		FScriptDelegate Delegate;
		UUPyCallableForDelegate* PythonCallableForDelegate = UPyDelegateUtil::PythonArgsToDelegate_Callable(InArgs, InSelf->DelegateProp->SignatureFunction, Delegate, TEXT("add_callable"), *UPyUtil::GetErrorContext(InSelf));
		if (!PythonCallableForDelegate)
		{
			return nullptr;
		}

		if (InSelf->DelegateInstance)
		{
			InSelf->DelegateInstance->Add(Delegate);
			Py_RETURN_NONE;
		}

		if (const FMulticastDelegateProperty* MulticastProp = CastField<FMulticastDelegateProperty>(InSelf->DelegateProp))
		{
			MulticastProp->AddDelegate(MoveTemp(Delegate), nullptr, InSelf->PropAddr);
			Py_RETURN_NONE;
		}

		Py_RETURN_NONE;
	}

	static PyObject* AddFunctionUnique(FUPyWrapperMulticastDelegate* InSelf, PyObject* InArgs)
	{
		if (!FUPyWrapperMulticastDelegate::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		// const UPyGenUtil::FGeneratedWrappedFunction& DelegateSignature = FUPyWrapperMulticastDelegateMetaData::GetDelegateSignature(InSelf);

		FScriptDelegate Delegate;
		if (!UPyDelegateUtil::PythonArgsToDelegate_ObjectAndName(InArgs, InSelf->DelegateProp->SignatureFunction, Delegate, TEXT("add_function_unique"), *UPyUtil::GetErrorContext(InSelf)))
		{
			return nullptr;
		}

		if (InSelf->DelegateInstance)
		{
			InSelf->DelegateInstance->AddUnique(Delegate);
			Py_RETURN_NONE;
		}

		if (const FMulticastDelegateProperty* MulticastProp = CastField<FMulticastDelegateProperty>(InSelf->DelegateProp))
		{
			// Try to use script delegate API if possible to add unique
			// For property API, AddDelegate just adds.
			FMulticastScriptDelegate* ScriptDelegate = const_cast<FMulticastScriptDelegate*>(MulticastProp->GetMulticastDelegate(InSelf->PropAddr));
			if (ScriptDelegate)
			{
				ScriptDelegate->AddUnique(Delegate);
			}
			else
			{
				MulticastProp->AddDelegate(MoveTemp(Delegate), nullptr, InSelf->PropAddr);
			}
			Py_RETURN_NONE;
		}

		Py_RETURN_NONE;
	}

	static PyObject* AddCallableUnique(FUPyWrapperMulticastDelegate* InSelf, PyObject* InArgs)
	{
		if (!FUPyWrapperMulticastDelegate::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		// We need to search for an entry using the current callable rather than use the normal AddUnique function,
		// as that only checks the object and function name and each Python callable proxy is its own instance
		PyObject* PyCallable = nullptr;
		if (!UPyDelegateUtil::PythonArgsToPythonCallable(InArgs, PyCallable, TEXT("add_callable_unique"), *UPyUtil::GetErrorContext(InSelf)))
		{
			return nullptr;
		}

		// Check internal instance first if available via property or direct
		bool bAddDelegate = true;
		const FMulticastScriptDelegate* ScriptDelegateToCheck = InSelf->DelegateInstance;

		if (!ScriptDelegateToCheck)
		{
			if (const FMulticastDelegateProperty* MulticastProp = CastField<FMulticastDelegateProperty>(InSelf->DelegateProp))
			{
				ScriptDelegateToCheck = MulticastProp->GetMulticastDelegate(InSelf->PropAddr);
			}
		}

		if (ScriptDelegateToCheck)
		{
			for (const UObject* DelegateObj : ScriptDelegateToCheck->GetAllObjects())
			{
				if (const UUPyCallableForDelegate* PythonCallableForDelegate = ::Cast<UUPyCallableForDelegate>(DelegateObj))
				{
					if (UPyDelegateUtil::AreCallablesEqual(PythonCallableForDelegate->GetCallable(), PyCallable))
					{
						bAddDelegate = false;
						break;
					}
				}
			}
		}

		if (bAddDelegate)
		{
			// const UPyGenUtil::FGeneratedWrappedFunction& DelegateSignature = FUPyWrapperMulticastDelegateMetaData::GetDelegateSignature(InSelf);
			// const UClass* PythonCallableForDelegateClass = FUPyWrapperMulticastDelegateMetaData::GetPythonCallableForDelegateClass(InSelf);

			FScriptDelegate Delegate;
			if (!UPyDelegateUtil::PythonCallableToDelegate(PyCallable, InSelf->DelegateProp->SignatureFunction, Delegate))
			{
				return nullptr;
			}

			if (InSelf->DelegateInstance)
			{
				InSelf->DelegateInstance->Add(Delegate);
				Py_RETURN_NONE;
			}

			if (const FMulticastDelegateProperty* MulticastProp = CastField<FMulticastDelegateProperty>(InSelf->DelegateProp))
			{
				MulticastProp->AddDelegate(MoveTemp(Delegate), nullptr, InSelf->PropAddr);
				Py_RETURN_NONE;
			}
		}

		Py_RETURN_NONE;
	}

	static PyObject* RemoveFunction(FUPyWrapperMulticastDelegate* InSelf, PyObject* InArgs)
	{
		if (!FUPyWrapperMulticastDelegate::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		// const UPyGenUtil::FGeneratedWrappedFunction& DelegateSignature = FUPyWrapperMulticastDelegateMetaData::GetDelegateSignature(InSelf);

		FScriptDelegate Delegate;
		if (!UPyDelegateUtil::PythonArgsToDelegate_ObjectAndName(InArgs, InSelf->DelegateProp->SignatureFunction, Delegate, TEXT("remove_function"), *UPyUtil::GetErrorContext(InSelf)))
		{
			return nullptr;
		}

		if (InSelf->DelegateInstance)
		{
			InSelf->DelegateInstance->Remove(Delegate);
			Py_RETURN_NONE;
		}

		if (const FMulticastDelegateProperty* MulticastProp = CastField<FMulticastDelegateProperty>(InSelf->DelegateProp))
		{
			MulticastProp->RemoveDelegate(Delegate, nullptr, InSelf->PropAddr);
			Py_RETURN_NONE;
		}

		Py_RETURN_NONE;
	}

	static PyObject* RemoveCallable(FUPyWrapperMulticastDelegate* InSelf, PyObject* InArgs)
	{
		if (!FUPyWrapperMulticastDelegate::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		// We need to search for an entry using the current callable rather than use the normal Remove function,
		// as that only checks the object and function name and each Python callable proxy is its own instance
		PyObject* PyCallable = nullptr;
		if (!UPyDelegateUtil::PythonArgsToPythonCallable(InArgs, PyCallable, TEXT("remove_callable"), *UPyUtil::GetErrorContext(InSelf)))
		{
			return nullptr;
		}

		const FMulticastScriptDelegate* ScriptDelegateToCheck = InSelf->DelegateInstance;
		if (!ScriptDelegateToCheck)
		{
			if (const FMulticastDelegateProperty* MulticastProp = CastField<FMulticastDelegateProperty>(InSelf->DelegateProp))
			{
				ScriptDelegateToCheck = MulticastProp->GetMulticastDelegate(InSelf->PropAddr);
			}
		}

		if (ScriptDelegateToCheck)
		{
			UUPyCallableForDelegate* PythonCallableForDelegateToRemove = nullptr;
			for (UObject* DelegateObj : ScriptDelegateToCheck->GetAllObjects())
			{
				if (UUPyCallableForDelegate* PythonCallableForDelegate = ::Cast<UUPyCallableForDelegate>(DelegateObj))
				{
					if (UPyDelegateUtil::AreCallablesEqual(PythonCallableForDelegate->GetCallable(), PyCallable))
					{
						PythonCallableForDelegateToRemove = PythonCallableForDelegate;
						break;
					}
				}
			}

			if (PythonCallableForDelegateToRemove)
			{
				// We need a non-const pointer to remove
				if (InSelf->DelegateInstance)
				{
					InSelf->DelegateInstance->RemoveAll(PythonCallableForDelegateToRemove);
				}
				else if (const FMulticastDelegateProperty* MulticastProp = CastField<FMulticastDelegateProperty>(InSelf->DelegateProp))
				{
					if (FMulticastScriptDelegate* MutableDelegate = const_cast<FMulticastScriptDelegate*>(MulticastProp->GetMulticastDelegate(InSelf->PropAddr)))
					{
						MutableDelegate->RemoveAll(PythonCallableForDelegateToRemove);
					}
				}
				PythonCallableForDelegateToRemove->Deinit();
			}
		}

		Py_RETURN_NONE;
	}

	static PyObject* RemoveObject(FUPyWrapperMulticastDelegate* InSelf, PyObject* InArgs)
	{
		if (!FUPyWrapperMulticastDelegate::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		PyObject* PyObj = nullptr;
		if (!PyArg_ParseTuple(InArgs, "O:remove_object", &PyObj))
		{
			return nullptr;
		}

		UObject* Obj = nullptr;
		if (!UPyConversion::Nativize(PyObj, Obj))
		{
			return nullptr;
		}

		if (InSelf->DelegateInstance)
		{
			InSelf->DelegateInstance->RemoveAll(Obj);
			Py_RETURN_NONE;
		}

		if (const FMulticastDelegateProperty* MulticastProp = CastField<FMulticastDelegateProperty>(InSelf->DelegateProp))
		{
			if (FMulticastScriptDelegate* ScriptDelegate = const_cast<FMulticastScriptDelegate*>(MulticastProp->GetMulticastDelegate(InSelf->PropAddr)))
			{
				ScriptDelegate->RemoveAll(Obj);
			}
			Py_RETURN_NONE;
		}

		Py_RETURN_NONE;
	}

	static PyObject* ContainsFunction(FUPyWrapperMulticastDelegate* InSelf, PyObject* InArgs)
	{
		if (!FUPyWrapperMulticastDelegate::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		// const UPyGenUtil::FGeneratedWrappedFunction& DelegateSignature = FUPyWrapperMulticastDelegateMetaData::GetDelegateSignature(InSelf);

		FScriptDelegate Delegate;
		if (!UPyDelegateUtil::PythonArgsToDelegate_ObjectAndName(InArgs, InSelf->DelegateProp->SignatureFunction, Delegate, TEXT("contains_function"), *UPyUtil::GetErrorContext(InSelf)))
		{
			return nullptr;
		}

		if (InSelf->DelegateInstance && InSelf->DelegateInstance->Contains(Delegate))
		{
			Py_RETURN_TRUE;
		}

		if (const FMulticastDelegateProperty* MulticastProp = CastField<FMulticastDelegateProperty>(InSelf->DelegateProp))
		{
			if (const FMulticastScriptDelegate* ScriptDelegate = MulticastProp->GetMulticastDelegate(InSelf->PropAddr))
			{
				if (ScriptDelegate->Contains(Delegate))
				{
					Py_RETURN_TRUE;
				}
			}
			Py_RETURN_FALSE;
		}

		Py_RETURN_FALSE;
	}

	static PyObject* ContainsCallable(FUPyWrapperMulticastDelegate* InSelf, PyObject* InArgs)
	{
		if (!FUPyWrapperMulticastDelegate::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		// We need to search for an entry using the current callable rather than use the normal Contains function,
		// as that only checks the object and function name and each Python callable proxy is its own instance
		PyObject* PyCallable = nullptr;
		if (!UPyDelegateUtil::PythonArgsToPythonCallable(InArgs, PyCallable, TEXT("contains_callable"), *UPyUtil::GetErrorContext(InSelf)))
		{
			return nullptr;
		}

		const FMulticastScriptDelegate* ScriptDelegateToCheck = InSelf->DelegateInstance;
		if (!ScriptDelegateToCheck)
		{
			if (const FMulticastDelegateProperty* MulticastProp = CastField<FMulticastDelegateProperty>(InSelf->DelegateProp))
			{
				ScriptDelegateToCheck = MulticastProp->GetMulticastDelegate(InSelf->PropAddr);
			}
		}

		if (ScriptDelegateToCheck)
		{
			for (const UObject* DelegateObj : ScriptDelegateToCheck->GetAllObjects())
			{
				if (const UUPyCallableForDelegate* PythonCallableForDelegate = ::Cast<UUPyCallableForDelegate>(DelegateObj))
				{
					if (UPyDelegateUtil::AreCallablesEqual(PythonCallableForDelegate->GetCallable(), PyCallable))
					{
						Py_RETURN_TRUE; // Changed from bContainsCallable = true; break;
					}
				}
			}
		}

		Py_RETURN_FALSE;
	}

	static PyObject* Clear(FUPyWrapperMulticastDelegate* InSelf)
	{
		if (!FUPyWrapperMulticastDelegate::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		if (InSelf->DelegateInstance)
		{
			for (UObject* DelegateObj : InSelf->DelegateInstance->GetAllObjects())
			{
				if (UUPyCallableForDelegate* PythonCallableForDelegate = ::Cast<UUPyCallableForDelegate>(DelegateObj))
				{
					PythonCallableForDelegate->Deinit();
				}
			}

			InSelf->DelegateInstance->Clear();
			Py_RETURN_NONE;
		}

		if (const FMulticastDelegateProperty* MulticastProp = CastField<FMulticastDelegateProperty>(InSelf->DelegateProp))
		{
			// We still need to iterate and deinit python callables if possible?
			// Accessing the delegate list before clearing
			if (const FMulticastScriptDelegate* ScriptDelegate = MulticastProp->GetMulticastDelegate(InSelf->PropAddr))
			{
				for (UObject* DelegateObj : ScriptDelegate->GetAllObjects())
				{
					if (UUPyCallableForDelegate* PythonCallableForDelegate = ::Cast<UUPyCallableForDelegate>(DelegateObj))
					{
						PythonCallableForDelegate->Deinit();
					}
				}
			}
			MulticastProp->ClearDelegate(nullptr, InSelf->PropAddr);
			Py_RETURN_NONE;
		}

		Py_RETURN_NONE;
	}

	static PyObject* Broadcast(FUPyWrapperMulticastDelegate* InSelf, PyObject* InArgs)
	{
		if (InSelf->DelegateInstance)
		{
			return FUPyWrapperMulticastDelegate::CallDelegate(InSelf, InArgs);
		}

		if (const FMulticastDelegateProperty* MulticastProp = CastField<FMulticastDelegateProperty>(InSelf->DelegateProp))
		{
			if (const FMulticastScriptDelegate* ScriptDelegate = MulticastProp->GetMulticastDelegate(InSelf->PropAddr))
			{
				InSelf->DelegateInstance = const_cast<FMulticastScriptDelegate*>(ScriptDelegate);
				return FUPyWrapperMulticastDelegate::CallDelegate(InSelf, InArgs);
			}
			// If unbound/empty sparse delegate, nothing to broadcast.
			Py_RETURN_NONE;
		}

		// If invalid instance and sparse delegate returns null (unbound), just no-op
		Py_RETURN_NONE;
	}

};

// NOTE: The multicast delegate couldn't be strictly type-hinted. The exact param types of the delegate are unknown here. In Python 3.11, we should be able
//       to use Self for return type of __copy__() and copy() methods.
//       _T = TypeVar('_T') and Type/Any/Callable/Union are defines by the Python typing module.
PyMethodDef MulticastDelegatePyMethodDefs[] = {
	{ "Cast", UPyCFunctionCast(&FMethods_WrapperMulticastDelegate::Cast), METH_VARARGS | METH_CLASS, UPyDoc_STR("Cast(cls: Type[_T], object: object) -> _T -- cast the given object to this Unreal delegate type") },
	// { "__copy__", UPyCFunctionCast(&Copy), METH_NOARGS, UPyDoc_STR("__copy__(self) -> Any -- copy this Unreal delegate") },
	// { "copy", UPyCFunctionCast(&Copy), METH_NOARGS, UPyDoc_STR("copy(self) -> Any -- copy this Unreal delegate") },
	{ "IsBound", UPyCFunctionCast(&FMethods_WrapperMulticastDelegate::IsBound), METH_NOARGS, UPyDoc_STR("IsBound(self) -> bool -- is this Unreal delegate bound to something?") },
	{ "AddFunction", UPyCFunctionCast(&FMethods_WrapperMulticastDelegate::AddFunction), METH_VARARGS, UPyDoc_STR("AddFunction(self, obj: Object, name: str) -> None -- add a binding to a named Unreal function on the given object instance to the invocation list of this Unreal delegate") },
	{ "Add", UPyCFunctionCast(&FMethods_WrapperMulticastDelegate::AddCallable), METH_VARARGS, UPyDoc_STR("Add(self, callable: Callable[..., Any]) -> None -- add a binding to a Python callable to the invocation list of this Unreal delegate") },
	{ "AddFunctionUnique", UPyCFunctionCast(&FMethods_WrapperMulticastDelegate::AddFunctionUnique), METH_VARARGS, UPyDoc_STR("AddFunctionUnique(self, obj: Object, name: str) -> None -- add a unique binding to a named Unreal function on the given object instance to the invocation list of this Unreal delegate") },
	{ "AddUnique", UPyCFunctionCast(&FMethods_WrapperMulticastDelegate::AddCallableUnique), METH_VARARGS, UPyDoc_STR("AddUnique(self, callable: Callable[..., Any]) -> None -- add a unique binding to a Python callable to the invocation list of this Unreal delegate") },
	{ "RemoveFunction", UPyCFunctionCast(&FMethods_WrapperMulticastDelegate::RemoveFunction), METH_VARARGS, UPyDoc_STR("RemoveFunction(self, obj: Object, name: str) -> None -- remove a binding to a named Unreal function on the given object instance from the invocation list of this Unreal delegate") },
	{ "Remove", UPyCFunctionCast(&FMethods_WrapperMulticastDelegate::RemoveCallable), METH_VARARGS, UPyDoc_STR("Remove(self, callable: Callable[..., Any]) -> None -- remove a binding to a Python callable from the invocation list of this Unreal delegate") },
	{ "RemoveObject", UPyCFunctionCast(&FMethods_WrapperMulticastDelegate::RemoveObject), METH_VARARGS, UPyDoc_STR("RemoveObject(self, obj: Object) -> None -- remove all bindings for the given object instance from the invocation list of this Unreal delegate") },
	{ "ContainsFunction", UPyCFunctionCast(&FMethods_WrapperMulticastDelegate::ContainsFunction), METH_VARARGS, UPyDoc_STR("ContainsFunction(self, obj: Object, name: str) -> bool -- does the invocation list of this Unreal delegate contain a binding to a named Unreal function on the given object instance") },
	{ "Contains", UPyCFunctionCast(&FMethods_WrapperMulticastDelegate::ContainsCallable), METH_VARARGS, UPyDoc_STR("Contains(self, callable: Callable[..., Any]) -> bool -- does the invocation list of this Unreal delegate contain a binding to a Python callable") },
	{ "Clear", UPyCFunctionCast(&FMethods_WrapperMulticastDelegate::Clear), METH_NOARGS, UPyDoc_STR("x.Clear() -> None -- clear the invocation list of this Unreal delegate") },
	{ "Broadcast", UPyCFunctionCast(&FMethods_WrapperMulticastDelegate::Broadcast), METH_VARARGS, UPyDoc_STR("x.Broadcast(*args: Any) -> None -- invoke this Unreal multicast delegate") },
	{ nullptr, nullptr, 0, nullptr }
};
