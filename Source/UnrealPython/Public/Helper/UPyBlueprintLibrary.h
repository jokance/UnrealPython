
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Core/UPyConversion.h"
#include "Core/UPyGIL.h"
#include "Core/UPyPtr.h"
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

	/**
	 * Variadic Template version for C++ usage.
	 */
	template <typename ReturnType, typename... ArgTypes>
	static ReturnType CallPythonMethod(const FString& ModuleName, const FString& MethodName, ArgTypes... Args)
	{
		FUPyScopedGIL GIL;

		FUPyObjectPtr PyArgs;
		if constexpr (sizeof...(Args) > 0)
		{
			PyArgs = FUPyObjectPtr::StealReference(PyTuple_New(sizeof...(Args)));
			int32 ArgIndex = 0;

			auto ProcessArg = [&](auto&& Arg)
			{
				PyObject* PyArg = nullptr;
				if (UPyConversion::Pythonize(Arg, PyArg))
				{
					PyTuple_SetItem(PyArgs, ArgIndex, PyArg);
				}
				else
				{
					Py_INCREF(Py_None);
					PyTuple_SetItem(PyArgs, ArgIndex, Py_None);
				}
				ArgIndex++;
			};

			(ProcessArg(Args), ...);
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
			UPyConversion::Nativize(PyResult, Result);
			return Result;
		}
	}

private:
	static FUPyObjectPtr CallPythonMethodInternal(const FString& ModuleName, const FString& MethodName, PyObject* PyArgs);
};
