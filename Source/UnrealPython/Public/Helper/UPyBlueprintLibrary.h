
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Core/UPyConversion.h"
#include "Core/UPyGIL.h"
#include "Core/UPyPtr.h"
#include "Utils/UPyUtil.h"
#include "UPyBlueprintLibrary.generated.h"

UCLASS()
class UNREALPYTHON_API UUPyBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "UnrealPython")
	static FString CallPythonMethod_Str_RetStr(const FString& ModuleName, const FString& MethodName, const FString& Arg);

	UFUNCTION(BlueprintCallable, Category = "UnrealPython")
	static FString CallPythonMethod_Int_RetStr(const FString& ModuleName, const FString& MethodName, int32 Arg);

	UFUNCTION(BlueprintCallable, Category = "UnrealPython")
	static int32 CallPythonMethod_Int_RetInt(const FString& ModuleName, const FString& MethodName, int32 Arg);

	UFUNCTION(BlueprintCallable, Category = "UnrealPython")
	static TArray<int32> CallPythonMethod_IntArray_RetIntArray(const FString& ModuleName, const FString& MethodName, const TArray<int32>& Arg);

	UFUNCTION(BlueprintCallable, Category = "UnrealPython")
	static TArray<FString> CallPythonMethod_StrArray_RetStrArray(const FString& ModuleName, const FString& MethodName, const TArray<FString>& Arg);

	UFUNCTION(BlueprintCallable, Category = "UnrealPython")
	static TArray<FString> CallPythonMethod_IntArray_RetStrArray(const FString& ModuleName, const FString& MethodName, const TArray<int32>& Arg);

	UFUNCTION(BlueprintCallable, Category = "UnrealPython")
	static TArray<int32> CallPythonMethod_StrArray_RetIntArray(const FString& ModuleName, const FString& MethodName, const TArray<FString>& Arg);

	UFUNCTION(BlueprintCallable, Category = "UnrealPython")
	static bool CallLoadedPythonMethod(const FString& ModuleName, const FString& MethodName);

	/**
	 * Variadic Template version for C++ usage.
	 */
	template <typename ReturnType, typename... ArgTypes>
	static ReturnType CallPythonMethod(const FString& ModuleName, const FString& MethodName, ArgTypes... Args)
	{
		if (!Py_IsInitialized())
		{
			if constexpr (std::is_same_v<ReturnType, void>)
			{
				return;
			}
			else
			{
				return ReturnType();
			}
		}

		FUPyScopedGIL GIL;

		FUPyObjectPtr PyArgs;
		if constexpr (sizeof...(Args) > 0)
		{
			PyArgs = FUPyObjectPtr::StealReference(PyTuple_New(sizeof...(Args)));
			if (!PyArgs)
			{
				UPyUtil::LogPythonError();
				if constexpr (std::is_same_v<ReturnType, void>)
				{
					return;
				}
				else
				{
					return ReturnType();
				}
			}

			int32 ArgIndex = 0;
			bool bConvertedArgs = true;

			auto ProcessArg = [&](auto&& Arg)
			{
				PyObject* PyArg = nullptr;
				if (UPyConversion::Pythonize(Arg, PyArg) && PyArg)
				{
					PyTuple_SetItem(PyArgs, ArgIndex, PyArg);
				}
				else
				{
					Py_XDECREF(PyArg);
					bConvertedArgs = false;
					Py_INCREF(Py_None);
					PyTuple_SetItem(PyArgs, ArgIndex, Py_None);
				}
				ArgIndex++;
			};

			(ProcessArg(Args), ...);
			if (!bConvertedArgs)
			{
				UPyUtil::LogPythonError();
				if constexpr (std::is_same_v<ReturnType, void>)
				{
					return;
				}
				else
				{
					return ReturnType();
				}
			}
		}

		FUPyObjectPtr PyResult = CallPythonMethodInternal(ModuleName, MethodName, PyArgs);

		if constexpr (std::is_same_v<ReturnType, void>)
		{
			return;
		}
		else
		{
			if (!PyResult || PyResult == Py_None)
			{
				return ReturnType();
			}

			ReturnType Result = ReturnType();
			if (!UPyConversion::Nativize(PyResult, Result))
			{
				UPyUtil::LogPythonError();
				return ReturnType();
			}
			return Result;
		}
	}

private:
	static FUPyObjectPtr CallPythonMethodInternal(const FString& ModuleName, const FString& MethodName, PyObject* PyArgs);
};
