
#pragma once

#include "UPyCommon.h"

class UUPyCallableForDelegate;

namespace UPyDelegateUtil
{

/**
 * Tests for callable equality
 * ob_type->tp_call is the function pointer that will be called, so they need to be the same.
 * If the callable is a method, check that the self parameter is also the same.
 * 
 * Lhs & Rhs need to be callables!
 */
bool AreCallablesEqual(PyObject* Lhs, PyObject* Rhs);
	
bool PythonArgsToDelegate_ObjectAndName(PyObject* InArgs, /*const UPyGenUtil::FGeneratedWrappedFunction& InDelegateSignature,*/const UFunction* InDelegateSignature, FScriptDelegate& OutDelegate, const TCHAR* InFuncCtxt, const TCHAR* InErrorCtxt);

bool PythonArgsToPythonCallable(PyObject* InArgs, PyObject*& OutPyCallable, const TCHAR* InFuncCtxt, const TCHAR* InErrorCtxt);

UUPyCallableForDelegate* PythonCallableToDelegate(PyObject* InPyCallable, const UFunction* InDelegateSignature, /*const UPyGenUtil::FGeneratedWrappedFunction& InDelegateSignature, const UClass* InPythonCallableForDelegateClass,*/ FScriptDelegate& OutDelegate/*, const TCHAR* InErrorCtxt*/);
UUPyCallableForDelegate* PythonArgsToDelegate_Callable(PyObject* InArgs, const UFunction* InDelegateSignature, /*const UPyGenUtil::FGeneratedWrappedFunction& InDelegateSignature, const UClass* InPythonCallableForDelegateClass,*/ FScriptDelegate& OutDelegate, const TCHAR* InFuncCtxt, const TCHAR* InErrorCtxt);

} // namespace UPyDelegateUtil