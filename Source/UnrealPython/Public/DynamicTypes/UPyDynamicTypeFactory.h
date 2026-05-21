
#pragma once

#include "UPyCommon.h"

struct FUPyGeneratedWrappedClassType;
struct FUPyGeneratedWrappedStructType;

class FUPyDynamicTypeFactory
{
public:
	static void GenerateWrappedPropertyForClass(const UClass* InClass, FUPyGeneratedWrappedClassType* GeneratedWrappedType, const FProperty* InProp);

	static void GenerateWrappedMethodForClass(const UClass* InClass, FUPyGeneratedWrappedClassType* GeneratedWrappedType, const UFunction* InFunc);

	static void GenerateWrappedConstantForClass(FUPyGeneratedWrappedClassType* GeneratedWrappedType, const UFunction* InFunc);

	static void GenerateWrappedPropertyForStruct(const UScriptStruct* InStruct, FUPyGeneratedWrappedStructType* GeneratedWrappedType, const FProperty* InProp);
	
};
