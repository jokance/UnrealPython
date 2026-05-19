// Copyright Epic Games, Inc. All Rights Reserved.

#include "UPyCodeGenerator.h"
#include "ProfilingDebugging/ScopedTimers.h"
#include "Utils/UPyGenUtil.h"
#include "Utils/UPyUtil.h"
#include "UObject/UnrealType.h"
#include "UPyCommon.h"
#include "DynamicTypes/UPyGeneratedWrappedType.h"
#include "Algo/Sort.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "HAL/FileManager.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Policies/PrettyJsonPrintPolicy.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Wrapper/UPyWrapperTypeRegistry.h"

#define LOCTEXT_NAMESPACE "UPyCodeGenerator"

FUPyCodeGenerator& FUPyCodeGenerator::Get()
{
	static FUPyCodeGenerator Instance;
	return Instance;
}

FUPyCodeGenerator::FUPyCodeGenerator()
{
}

void FUPyCodeGenerator::GenerateAllCode()
{
	NativeModulesJson.Reset();

	double GenerateDuration = 0.0;
	{
		FScopedDurationTimer GenerateDurationTimer(GenerateDuration);

		SaveJsonToFile();
	}

	UE_LOG(LogUnrealPython, Verbose, TEXT("Took %f seconds to generate and initialize Python wrapped types for the initial load."), GenerateDuration);
}

void FUPyCodeGenerator::SaveJsonToFile() const
{
	TSharedRef<FJsonObject> RootJson = MakeShared<FJsonObject>();

	TArray<FNativeModuleMethodJsonInfo> UnrealModuleMethodJsonArray;
	const TArray<UPyGenUtil::FNativePythonModule>& NativeModules = FUPyWrapperTypeRegistry::Get().GetNativePythonModules();
	for (const UPyGenUtil::FNativePythonModule& NativeModule : NativeModules)
	{
		for (PyTypeObject* Type : NativeModule.PyModuleTypes)
		{
			if (!Type || !Type->tp_name)
			{
				continue;
			}

			const FString ModuleName = UTF8_TO_TCHAR(Type->tp_name);
			FNativeModuleJsonInfo& ModuleInfo = NativeModulesJson.AddDefaulted_GetRef();
			ModuleInfo.ModuleName = ModuleName;

			if (Type->tp_methods)
			{
				for (const PyMethodDef* Method = Type->tp_methods; Method && Method->ml_name; ++Method)
				{
					FNativeModuleMethodJsonInfo MethodInfo;
					MethodInfo.MethodName = UTF8_TO_TCHAR(Method->ml_name);
					MethodInfo.PyMethodFlags = Method->ml_flags;
					if (Method->ml_doc)
					{
						MethodInfo.MethodDoc = UTF8_TO_TCHAR(Method->ml_doc);
					}
					ModuleInfo.NativeMethods.Add(MoveTemp(MethodInfo));
				}
			}
		}

		for (const PyMethodDef* Method = NativeModule.PyModuleMethods; Method && Method->ml_name; ++Method)
		{
			FNativeModuleMethodJsonInfo MethodInfo;
			MethodInfo.MethodName = UTF8_TO_TCHAR(Method->ml_name);
			MethodInfo.PyMethodFlags = Method->ml_flags;
			if (Method->ml_doc)
			{
				MethodInfo.MethodDoc = UTF8_TO_TCHAR(Method->ml_doc);
			}
			UnrealModuleMethodJsonArray.Add(MoveTemp(MethodInfo));
		}
	}

	FNativeModuleJsonInfo UnrealModuleJsonInfo;
	UnrealModuleJsonInfo.ModuleName = "ue";
	UnrealModuleJsonInfo.NativeMethods = MoveTemp(UnrealModuleMethodJsonArray);
	NativeModulesJson.Insert(UnrealModuleJsonInfo, 0);
	
	for (FNativeModuleJsonInfo& ModuleJsonInfo : NativeModulesJson)
	{
		TSharedPtr<FJsonObject> ModuleJsonObject = MakeShared<FJsonObject>();

		TArray<TSharedPtr<FJsonValue>> NativeMethodsArray;
		for (const FNativeModuleMethodJsonInfo& MethodInfo : ModuleJsonInfo.NativeMethods)
		{
			TSharedPtr<FJsonObject> MethodObject = MakeShared<FJsonObject>();
			MethodObject->SetStringField(TEXT("MethodName"), MethodInfo.MethodName);
			MethodObject->SetStringField(TEXT("MethodDoc"), MethodInfo.MethodDoc);
			MethodObject->SetNumberField(TEXT("PyMethodFlags"), MethodInfo.PyMethodFlags);
			NativeMethodsArray.Add(MakeShared<FJsonValueObject>(MethodObject));
		}
		ModuleJsonObject->SetArrayField(TEXT("NativeMethods"), NativeMethodsArray);

		RootJson->SetObjectField(ModuleJsonInfo.ModuleName, ModuleJsonObject);
	}

	FString OutputString;
	TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> JsonWriter = TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&OutputString);
	const bool bSerialised = FJsonSerializer::Serialize(RootJson, JsonWriter);

	if (bSerialised)
	{
		const FString OutputDir = FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("UnrealPython"), TEXT("Tools"), TEXT("ReflectionData"));
		IFileManager::Get().MakeDirectory(*OutputDir, true);

		const FString OutputPath = FPaths::Combine(OutputDir, TEXT("native_module.json"));
		if (FFileHelper::SaveStringToFile(OutputString, *OutputPath))
		{
			UE_LOG(LogUnrealPython, Log, TEXT("Saved generated Python metadata to: %s"), *OutputPath);
		}
		else
		{
			UE_LOG(LogUnrealPython, Error, TEXT("Failed to write generated Python metadata to: %s"), *OutputPath);
		}
	}
	else
	{
		UE_LOG(LogUnrealPython, Error, TEXT("Failed to serialise generated Python metadata to JSON."));
	}
}

#undef LOCTEXT_NAMESPACE
