
#include "Utils/UPyDelegateUtil.h"
#include "Utils/UPyUtil.h"
#include "Core/UPyConversion.h"
#include "Wrapper/UPyWrapperDelegate.h"

namespace UPyDelegateUtil
{

/**
 * Tests for callable equality
 * ob_type->tp_call is the function pointer that will be called, so they need to be the same.
 * If the callable is a method, check that the self parameter is also the same.
 * 
 * Lhs & Rhs need to be callables!
 */
bool AreCallablesEqual(PyObject* Lhs, PyObject* Rhs)
{
	check(PyCallable_Check(Lhs) && PyCallable_Check(Rhs));

	bool bAreCallablesEqual = (Lhs->ob_type == Rhs->ob_type); // Must be of the same type
	bAreCallablesEqual = bAreCallablesEqual && (Lhs->ob_type->tp_call == Rhs->ob_type->tp_call); // Must be calling the same function/method

	if (bAreCallablesEqual && PyMethod_Check(Lhs))
	{
		bAreCallablesEqual = (PyMethod_Self(Lhs) == PyMethod_Self(Rhs)); // Must have the same self
	}

	return bAreCallablesEqual;
}

bool PythonArgsToDelegate_ObjectAndName(PyObject* InArgs, const UFunction* InDelegateSignature, FScriptDelegate& OutDelegate, const TCHAR* InFuncCtxt, const TCHAR* InErrorCtxt)
{
	PyObject* PyObj = nullptr;
	PyObject* PyFuncNameObj = nullptr;
	if (!PyArg_ParseTuple(InArgs, TCHAR_TO_UTF8(*FString::Printf(TEXT("OO:%s"), InFuncCtxt)), &PyObj, &PyFuncNameObj))
	{
		return false;
	}

	UObject* Obj = nullptr;
	if (!UPyConversion::Nativize(PyObj, Obj))
	{
		UPyUtil::SetPythonError(PyExc_TypeError, InErrorCtxt, *FString::Printf(TEXT("Failed to convert argument 0 (%s) to 'Object'"), *UPyUtil::GetFriendlyTypename(PyObj)));
		return false;
	}

	FName FuncName;
	if (!UPyConversion::Nativize(PyFuncNameObj, FuncName))
	{
		UPyUtil::SetPythonError(PyExc_TypeError, InErrorCtxt, *FString::Printf(TEXT("Failed to convert argument 1 (%s) to 'Name'"), *UPyUtil::GetFriendlyTypename(PyFuncNameObj)));
		return false;
	}

	if (Obj)
	{
		check(PyObj);

		// Is the function name we've been given a Python alias? If so, we need to resolve that now
		// FuncName = FPyWrapperObjectMetaData::ResolveFunctionName(Py_TYPE(PyObj), FuncName);
		const UFunction* Func = Obj->FindFunction(FuncName);

		// Valid signature?
		if (Func && !InDelegateSignature->IsSignatureCompatibleWith(Func))
		{
			UPyUtil::SetPythonError(PyExc_TypeError, InErrorCtxt, *FString::Printf(TEXT("Function '%s' on '%s' does not match the signature required by the delegate '%s'"), *Func->GetName(), *Obj->GetName(), *InDelegateSignature->GetName()));
			return false;
		}
	}

	OutDelegate.BindUFunction(Obj, FuncName);
	return true;
}

bool PythonArgsToPythonCallable(PyObject* InArgs, PyObject*& OutPyCallable, const TCHAR* InFuncCtxt, const TCHAR* InErrorCtxt)
{
	PyObject* PyObj = nullptr;
	if (!PyArg_ParseTuple(InArgs, TCHAR_TO_UTF8(*FString::Printf(TEXT("O:%s"), InFuncCtxt)), &PyObj))
	{
		return false;
	}

	if (!PyCallable_Check(PyObj))
	{
		UPyUtil::SetPythonError(PyExc_TypeError, InErrorCtxt, *FString::Printf(TEXT("Given argument is of type '%s' which isn't callable"), *UPyUtil::GetFriendlyTypename(PyObj)));
		return false;
	}

	OutPyCallable = PyObj;
	return true;
}

UUPyCallableForDelegate* PythonCallableToDelegate(PyObject* InPyCallable, const UFunction* InDelegateSignature,/*const UPyGenUtil::FGeneratedWrappedFunction& InDelegateSignature, const UClass* InPythonCallableForDelegateClass,*/ FScriptDelegate& OutDelegate/*, const TCHAR* InErrorCtxt*/)
{
	// if (!InPythonCallableForDelegateClass)
	// {
	// 	UPyUtil::SetPythonError(PyExc_Exception, InErrorCtxt, TEXT("Delegate wrapper proxy class is null! Cannot create binding"));
	// 	return false;
	// }

	// Inspect the arguments from the Python callable
	// If this fails, don't error as it may be a C++ wrapped function we were passed (and inspect doesn't work with those)
	// todo(hzn): metadata
	// TArray<FString> CallableArgNames;
	// if (UPyUtil::InspectFunctionArgs(InPyCallable, CallableArgNames))
	// {
	// 	// If the callable is a method with a bound "self", remove the first argument
	// 	const bool bHasSelf = PyMethod_Check(InPyCallable) && PyMethod_GET_SELF(InPyCallable);
	// 	if (bHasSelf && CallableArgNames.Num() > 0)
	// 	{
	// 		CallableArgNames.RemoveAt(0, EAllowShrinking::No);
	// 	}
	//
	// 	if (InDelegateSignature.InputParams.Num() != CallableArgNames.Num())
	// 	{
	// 		UPyUtil::SetPythonError(PyExc_Exception, InErrorCtxt, *FString::Printf(TEXT("Callable has the incorrect number of arguments (expected %d, got %d)"), InDelegateSignature.InputParams.Num(), CallableArgNames.Num()));
	// 		return false;
	// 	}
	// }

	// Note: -----------------------------------------------------------------------------------------------------------------------------------------------------------
	//  Delegates only hold a weak reference to the object. Wrapped delegates will attempt to keep the proxy object alive as long as it is referenced in Python, 
	//  but once Python is no longer referencing it, there is no guarantee that the proxy won't be GC'd unless C++ code explicitly keeps the delegate object alive.
	//  This is a known and accepted state of delegates as they currently stand. In the future we may revisit this and attempt to improve the lifetime management.
	UUPyCallableForDelegate* PythonCallableForDelegate = NewObject<UUPyCallableForDelegate>(GetTransientPackage()/*, (UClass*)InPythonCallableForDelegateClass*/);
	PythonCallableForDelegate->SetCallable(InPyCallable, InDelegateSignature);
	OutDelegate.BindUFunction(PythonCallableForDelegate, UUPyCallableForDelegate::GeneratedFuncName);
	return PythonCallableForDelegate;
}

UUPyCallableForDelegate* PythonArgsToDelegate_Callable(PyObject* InArgs, const UFunction* InDelegateSignature, /* const UPyGenUtil::FGeneratedWrappedFunction& InDelegateSignature, const UClass* InPythonCallableForDelegateClass,*/  FScriptDelegate& OutDelegate,const TCHAR* InFuncCtxt, const TCHAR* InErrorCtxt)
{
	PyObject* PyCallable = nullptr;
	if (!PythonArgsToPythonCallable(InArgs, PyCallable, InFuncCtxt, InErrorCtxt))
	{
		return nullptr;
	}
	return PythonCallableToDelegate(PyCallable, InDelegateSignature, OutDelegate);
}

} // namespace UPyDelegateUtil
