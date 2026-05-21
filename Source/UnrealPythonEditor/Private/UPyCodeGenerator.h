
#pragma once

#include "CoreMinimal.h"
#include "UPyCodeGenerator.generated.h"

enum class EUPyTypeGenerationFlags : uint8
{
	/** No behavior */
	None = 0,
	/** Generate the Python wrapper for this type, even if it fails the PyGenUtil::ShouldExportX check */
	ForceShouldExport = 1<<0,
	/** Generate the Python wrapper for this type, even if it passes the PyGenUtil::IsBlueprintGeneratedX check */
	IncludeBlueprintGeneratedTypes = 1<<1,
	/** Generate the Python wrapper for this type, re-using the existing type if it already exists */
	OverwriteExisting = 1<<2,
};
ENUM_CLASS_FLAGS(EUPyTypeGenerationFlags);

USTRUCT(BlueprintType)
struct FNativeModuleMethodJsonInfo
{
	GENERATED_BODY()

	UPROPERTY()
	FString MethodName;

	UPROPERTY()
	FString MethodDoc;

	UPROPERTY()
	int64 PyMethodFlags = 0;	
};

USTRUCT(BlueprintType)
struct FNativeModuleJsonInfo
{
	GENERATED_BODY()

	UPROPERTY()
	FString ModuleName;
	
	UPROPERTY()
	TArray<FNativeModuleMethodJsonInfo> NativeMethods;
};

class FUPyCodeGenerator
{
public:
	static FUPyCodeGenerator& Get();
	
	void GenerateAllCode();

private:
	FUPyCodeGenerator();

	void SaveJsonToFile() const;
	
	mutable TArray<FNativeModuleJsonInfo> NativeModulesJson;
};
