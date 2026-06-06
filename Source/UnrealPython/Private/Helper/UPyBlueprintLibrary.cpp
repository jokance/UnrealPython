
#include "Helper/UPyBlueprintLibrary.h"
#include "UPyCommon.h"
#include "Utils/UPyUtil.h"

FUPyObjectPtr UUPyBlueprintLibrary::CallPythonMethodInternal(const FString& ModuleName, const FString& MethodName, PyObject* PyArgs)
{
	FUPyObjectPtr PyModule = FUPyObjectPtr::StealReference(PyImport_ImportModule(TCHAR_TO_UTF8(*ModuleName)));
	if (!PyModule)
	{
		UE_LOG(LogUnrealPython, Error, TEXT("Failed to import module: %s"), *ModuleName);
		PyErr_Print();
		return FUPyObjectPtr();
	}

	FUPyObjectPtr PyMethod = FUPyObjectPtr::StealReference(PyObject_GetAttrString(PyModule, TCHAR_TO_UTF8(*MethodName)));
	if (!PyMethod)
	{
		UE_LOG(LogUnrealPython, Error, TEXT("Failed to find method: %s in module: %s"), *MethodName, *ModuleName);
		PyErr_Print();
		return FUPyObjectPtr();
	}

	if (!PyCallable_Check(PyMethod))
	{
		UE_LOG(LogUnrealPython, Error, TEXT("Attribute %s in module %s is not callable"), *MethodName, *ModuleName);
		return FUPyObjectPtr();
	}

	FUPyObjectPtr PyResult = FUPyObjectPtr::StealReference(PyObject_CallObject(PyMethod, PyArgs));
	if (!PyResult)
	{
		UE_LOG(LogUnrealPython, Error, TEXT("Failed to call method: %s in module: %s"), *MethodName, *ModuleName);
		PyErr_Print();
		return FUPyObjectPtr();
	}

	return PyResult;
}

FString UUPyBlueprintLibrary::CallPythonMethod_Str_RetStr(const FString& ModuleName, const FString& MethodName, const FString& Arg)
{
	return CallPythonMethod<FString>(ModuleName, MethodName, Arg);
}

FString UUPyBlueprintLibrary::CallPythonMethod_Int_RetStr(const FString& ModuleName, const FString& MethodName, int32 Arg)
{
	return CallPythonMethod<FString>(ModuleName, MethodName, Arg);
}

int32 UUPyBlueprintLibrary::CallPythonMethod_Int_RetInt(const FString& ModuleName, const FString& MethodName, int32 Arg)
{
	return CallPythonMethod<int32>(ModuleName, MethodName, Arg);
}

TArray<int32> UUPyBlueprintLibrary::CallPythonMethod_IntArray_RetIntArray(const FString& ModuleName, const FString& MethodName, const TArray<int32>& Arg)
{
	return CallPythonMethod<TArray<int32>>(ModuleName, MethodName, Arg);
}

TArray<FString> UUPyBlueprintLibrary::CallPythonMethod_StrArray_RetStrArray(const FString& ModuleName, const FString& MethodName, const TArray<FString>& Arg)
{
	return CallPythonMethod<TArray<FString>>(ModuleName, MethodName, Arg);
}

TArray<FString> UUPyBlueprintLibrary::CallPythonMethod_IntArray_RetStrArray(const FString& ModuleName, const FString& MethodName, const TArray<int32>& Arg)
{
	return CallPythonMethod<TArray<FString>>(ModuleName, MethodName, Arg);
}

TArray<int32> UUPyBlueprintLibrary::CallPythonMethod_StrArray_RetIntArray(const FString& ModuleName, const FString& MethodName, const TArray<FString>& Arg)
{
	return CallPythonMethod<TArray<int32>>(ModuleName, MethodName, Arg);
}

bool UUPyBlueprintLibrary::CallLoadedPythonMethod(const FString& ModuleName, const FString& MethodName)
{
	if (!Py_IsInitialized())
	{
		return false;
	}

	FUPyScopedGIL GIL;

	PyObject* Modules = PyImport_GetModuleDict();
	if (!Modules)
	{
		return false;
	}

	PyObject* PyModule = PyDict_GetItemString(Modules, TCHAR_TO_UTF8(*ModuleName));
	if (!PyModule)
	{
		UE_LOG(LogUnrealPython, Warning, TEXT("Skipping loaded Python method '%s.%s'; module is not loaded."), *ModuleName, *MethodName);
		return false;
	}

	FUPyObjectPtr PyMethod = FUPyObjectPtr::StealReference(PyObject_GetAttrString(PyModule, TCHAR_TO_UTF8(*MethodName)));
	if (!PyMethod || !PyCallable_Check(PyMethod))
	{
		if (PyErr_Occurred())
		{
			PyErr_Print();
		}
		UE_LOG(LogUnrealPython, Warning, TEXT("Skipping loaded Python method '%s.%s'; method is not callable."), *ModuleName, *MethodName);
		return false;
	}

	FUPyObjectPtr PyResult = FUPyObjectPtr::StealReference(PyObject_CallNoArgs(PyMethod));
	if (!PyResult)
	{
		if (PyErr_Occurred())
		{
			PyErr_Print();
		}
		UE_LOG(LogUnrealPython, Error, TEXT("Failed to call loaded Python method '%s.%s'."), *ModuleName, *MethodName);
		return false;
	}

	return true;
}
