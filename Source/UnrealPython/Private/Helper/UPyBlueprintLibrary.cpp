// Copyright Epic Games, Inc. All Rights Reserved.

#include "Helper/UPyBlueprintLibrary.h"
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
