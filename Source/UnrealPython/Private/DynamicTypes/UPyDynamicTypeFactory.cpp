
#include "DynamicTypes/UPyDynamicTypeFactory.h"
#include "DynamicTypes/UPyGeneratedWrappedClassType.h"
#include "DynamicTypes/UPyGeneratedWrappedStructType.h"
#include "Wrapper/UPyWrapperObjectBase.h"
#include "Wrapper/UPyWrapperStruct.h"

void FUPyDynamicTypeFactory::GenerateWrappedPropertyForClass(const UClass* InClass, FUPyGeneratedWrappedClassType* GeneratedWrappedType, const FProperty* InProp)
{
	const bool bExportPropertyToScript = UPyGenUtil::ShouldExportProperty(InProp);
	// const bool bExportPropertyToEditor = UPyGenUtil::ShouldExportEditorOnlyProperty(InProp);

	if (bExportPropertyToScript /* || bExportPropertyToEditor*/)
	{
		// GatherWrappedTypesForPropertyReferences(InProp, OutGeneratedWrappedTypeReferences);
		
		// const PyGenUtil::FGeneratedWrappedPropertyDoc& GeneratedPropertyDoc = GeneratedWrappedType->PropertyDocs[GeneratedWrappedType->PropertyDocs.Emplace(InProp)];
		// PythonProperties.Add(*GeneratedPropertyDoc.PythonPropName, InProp->GetFName());

		int32 GeneratedWrappedGetSetIndex = INDEX_NONE;
		GeneratedWrappedGetSetIndex = GeneratedWrappedType->GetSets.TypeGetSets.AddDefaulted();

		// auto FindGetSetFunction = [InClass, InProp](const FName& InKey) -> const UFunction*
		// {
		// 	const FString GetSetName = InProp->GetMetaData(InKey);
		// 	if (!GetSetName.IsEmpty())
		// 	{
		// 		const UFunction* GetSetFunc = InClass->FindFunctionByName(*GetSetName);
		// 		if (!GetSetFunc)
		// 		{
		// 		
		// 			UE_LOG(LogUnrealPython, Error, TEXT("Property '%s.%s' is marked as '%s' but the function '%s' could not be found."), *InClass->GetPathName(), *InProp->GetName(), *InKey.ToString(), *GetSetName);
		// 		}
		// 		return GetSetFunc;
		// 	}
		// 	return nullptr;
		// };

		UPyGenUtil::FGeneratedWrappedGetSet& GeneratedWrappedGetSet = GeneratedWrappedType->GetSets.TypeGetSets[GeneratedWrappedGetSetIndex];
		GeneratedWrappedGetSet.GetSetName = UPyGenUtil::TCHARToUTF8Buffer(*InProp->GetName());
		// GeneratedWrappedGetSet.GetSetDoc = UPyGenUtil::TCHARToUTF8Buffer(*GeneratedPropertyDoc.DocString);
		GeneratedWrappedGetSet.Prop.SetProperty(InProp);
		// GeneratedWrappedGetSet.GetFunc.SetFunction(FindGetSetFunction(UPyGenUtil::BlueprintGetterMetaDataKey));
		// GeneratedWrappedGetSet.SetFunc.SetFunction(FindGetSetFunction(UPyGenUtil::BlueprintSetterMetaDataKey));
		GeneratedWrappedGetSet.GetCallback = (getter)&FUPyWrapperObjectBase::Getter_Impl;
		GeneratedWrappedGetSet.SetCallback = (setter)&FUPyWrapperObjectBase::Setter_Impl;
		// if (GeneratedWrappedGetSet.Prop.DeprecationMessage.IsSet())
		// {
		// 	PythonDeprecatedProperties.Add(*GeneratedPropertyDoc.PythonPropName, GeneratedWrappedGetSet.Prop.DeprecationMessage.GetValue());
		// }

		// GeneratedWrappedType->FieldTracker.RegisterPythonFieldName(GeneratedPropertyDoc.PythonPropName, InProp);

	}
}

void FUPyDynamicTypeFactory::GenerateWrappedMethodForClass(const UClass* InClass, FUPyGeneratedWrappedClassType* GeneratedWrappedType, const UFunction* InFunc)
{
	// if (!PyGenUtil::ShouldExportFunction(InFunc))
	// {
	// 	return;
	// }

	// for (TFieldIterator<const FProperty> ParamIt(InFunc); ParamIt; ++ParamIt)
	// {
	// 	const FProperty* Param = *ParamIt;
	// 	GatherWrappedTypesForPropertyReferences(Param, OutGeneratedWrappedTypeReferences);
	// }

	// Constant functions do not export as real functions, so bail once we've generated their wrapped constant data
	// if (InFunc->HasMetaData(UPyGenUtil::ScriptConstantMetaDataKey))
	// {
	// 	GenerateWrappedConstantForClass(GeneratedWrappedType, InFunc);
	// 	return;
	// }

	const FString PythonFunctionName = InFunc->GetName();
	const bool bIsStatic = InFunc->HasAnyFunctionFlags(FUNC_Static);
	
	// PythonMethods.Add(*PythonFunctionName, InFunc->GetFName());

	UPyGenUtil::FGeneratedWrappedMethod& GeneratedWrappedMethod = GeneratedWrappedType->Methods.TypeMethods.AddDefaulted_GetRef();
	GeneratedWrappedMethod.MethodName = UPyGenUtil::TCHARToUTF8Buffer(*PythonFunctionName);
	GeneratedWrappedMethod.MethodFunc.SetFunction(InFunc);
	// if (GeneratedWrappedMethod.MethodFunc.DeprecationMessage.IsSet())
	// {
	// 	PythonDeprecatedMethods.Add(*PythonFunctionName, GeneratedWrappedMethod.MethodFunc.DeprecationMessage.GetValue());
	// }

	// GeneratedWrappedType->FieldTracker.RegisterPythonFieldName(PythonFunctionName, InFunc);

	// FString FunctionDeclDocString = PyGenUtil::BuildFunctionDocString(InFunc, PythonFunctionName, GeneratedWrappedMethod.MethodFunc.InputParams, GeneratedWrappedMethod.MethodFunc.OutputParams);
	// FunctionDeclDocString += LINE_TERMINATOR;
	// FunctionDeclDocString += PyGenUtil::PythonizeFunctionTooltip(PyGenUtil::ParseTooltip(PyGenUtil::GetFieldTooltip(InFunc)), InFunc);
	//
	// GeneratedWrappedMethod.MethodDoc = PyGenUtil::TCHARToUTF8Buffer(*FunctionDeclDocString);
	GeneratedWrappedMethod.MethodFlags = GeneratedWrappedMethod.MethodFunc.InputParams.Num() > 0 ? METH_VARARGS | METH_KEYWORDS : METH_NOARGS;
	if (bIsStatic)
	{
		GeneratedWrappedMethod.MethodFlags |= METH_CLASS;
		GeneratedWrappedMethod.MethodCallback = GeneratedWrappedMethod.MethodFunc.InputParams.Num() > 0 ? UPyCFunctionWithClosureCast(&FUPyWrapperObjectBase::CallClassMethodWithArgs_Impl) : UPyCFunctionWithClosureCast(&FUPyWrapperObjectBase::CallClassMethodNoArgs_Impl);
	}
	else
	{
		GeneratedWrappedMethod.MethodCallback = GeneratedWrappedMethod.MethodFunc.InputParams.Num() > 0 ? UPyCFunctionWithClosureCast(&FUPyWrapperObjectBase::CallMethodWithArgs_Impl) : UPyCFunctionWithClosureCast(&FUPyWrapperObjectBase::CallMethodNoArgs_Impl);
	}

	// We must create a copy here because otherwise the reference will get invalidated by 
	// subsequent modifications

	const UPyGenUtil::FGeneratedWrappedMethod GeneratedWrappedMethodCopy = GeneratedWrappedMethod;
	//
	// const TArray<TTuple<FSoftObjectPath, FString>> DeprecatedPythonFuncNames = PyGenUtil::GetDeprecatedFunctionPythonNames(InFunc);
	// for (const TTuple<FSoftObjectPath, FString>& DeprecatedPythonFuncNamePair : DeprecatedPythonFuncNames)
	// {
	// 	const FString& DeprecatedPythonFuncName = DeprecatedPythonFuncNamePair.Value;
	// 	FString DeprecationMessage = FString::Printf(TEXT("'%s' was renamed to '%s'."), *DeprecatedPythonFuncName, *PythonFunctionName);
	// 	PythonMethods.Add(*DeprecatedPythonFuncName, InFunc->GetFName());
	// 	PythonDeprecatedMethods.Add(*DeprecatedPythonFuncName, DeprecationMessage);
	//
	// 	PyGenUtil::FGeneratedWrappedMethod DeprecatedGeneratedWrappedMethod = GeneratedWrappedMethodCopy;
	// 	DeprecatedGeneratedWrappedMethod.MethodName = PyGenUtil::TCHARToUTF8Buffer(*DeprecatedPythonFuncName);
	// 	DeprecatedGeneratedWrappedMethod.MethodDoc = PyGenUtil::TCHARToUTF8Buffer(*FString::Printf(TEXT("deprecated: %s"), *DeprecationMessage));
	// 	DeprecatedGeneratedWrappedMethod.MethodFunc.DeprecationMessage = MoveTemp(DeprecationMessage);
	// 	GeneratedWrappedType->Methods.TypeMethods.Add(MoveTemp(DeprecatedGeneratedWrappedMethod));
	//
	// 	GeneratedWrappedType->FieldTracker.RegisterPythonFieldName(DeprecatedPythonFuncName, InFunc);
	// }

	// Should this function also be hoisted as a struct method or operator?
	// if (InFunc->HasMetaData(UPyGenUtil::ScriptMethodMetaDataKey))
	// {
	// 	GenerateWrappedDynamicMethod(InFunc, GeneratedWrappedMethodCopy);
	// }
	// if (InFunc->HasMetaData(UPyGenUtil::ScriptOperatorMetaDataKey))
	// {
	// 	GenerateWrappedOperator(InFunc, GeneratedWrappedMethodCopy);
	// }
}

void FUPyDynamicTypeFactory::GenerateWrappedConstantForClass(FUPyGeneratedWrappedClassType* GeneratedWrappedType, const UFunction* InFunc)
{
	// Only static functions can be constants
	if (!InFunc->HasAnyFunctionFlags(FUNC_Static))
	{
		UE_LOG(LogUnrealPython, Error, TEXT("Non-static function '%s.%s' is marked as 'ScriptConstant' but only static functions can be hoisted."), *InFunc->GetOwnerClass()->GetPathName(), *InFunc->GetName());
		return;
	}

	// We might want to hoist this function onto another type rather than its owner class
	const UObject* HostType = nullptr;
	// if (InFunc->HasMetaData(UPyGenUtil::ScriptConstantHostMetaDataKey))
	// {
	// 	const FString ConstantOwnerName = InFunc->GetMetaData(UPyGenUtil::ScriptConstantHostMetaDataKey);
	// 	HostType = UClass::TryFindTypeSlow<UStruct>(ConstantOwnerName);
	// 	if (HostType && !(HostType->IsA<UClass>() || HostType->IsA<UScriptStruct>()))
	// 	{
	// 		HostType = nullptr;
	// 	}
	// 	if (!HostType)
	// 	{
	// 		UE_LOG(LogUnrealPython, Error, TEXT("Function '%s.%s' is marked as 'ScriptConstantHost' but the host '%s' could not be found."), *InFunc->GetOwnerClass()->GetPathName(), *InFunc->GetName(), *ConstantOwnerName);
	// 		return;
	// 	}
	// }
	// if (const UClass* HostClass = Cast<UClass>(HostType))
	// {
	// 	if (HostClass->IsChildOf(InFunc->GetOwnerClass()))
	// 	{
	// 		UE_LOG(LogUnrealPython, Error, TEXT("Function '%s.%s' is marked as 'ScriptConstantHost' but the host type (%s) is a child of the class type of the static function. This is not allowed."), *InFunc->GetOwnerClass()->GetPathName(), *InFunc->GetName(), *HostClass->GetPathName());
	// 		return;
	// 	}
	// }

	// Verify that the function signature is valid
	UPyGenUtil::FGeneratedWrappedFunction ConstantFunc;
	ConstantFunc.SetFunction(InFunc);
	if (ConstantFunc.InputParams.Num() != 0 || ConstantFunc.OutputParams.Num() != 1)
	{
		UE_LOG(LogUnrealPython, Error, TEXT("Function '%s.%s' is marked as 'ScriptConstant' but has an invalid signature (it must return a value and take no arguments)."), *InFunc->GetOwnerClass()->GetPathName(), *InFunc->GetName());
		return;
	}

	const FString PythonConstantName = UPyGenUtil::GetScriptConstantPythonName(InFunc);
	TArray<TUniqueObj<UPyGenUtil::FGeneratedWrappedConstant>, TInlineAllocator<4>> ConstantDefs;

	// Build the constant definition
	UPyGenUtil::FGeneratedWrappedConstant& GeneratedWrappedConstant = ConstantDefs.AddDefaulted_GetRef().Get();
	GeneratedWrappedConstant.ConstantName = UPyGenUtil::TCHARToUTF8Buffer(*PythonConstantName);
	// GeneratedWrappedConstant.ConstantDoc = UPyGenUtil::TCHARToUTF8Buffer(*FString::Printf(TEXT("(%s): %s"), *PyGenUtil::GetPropertyPythonType(ConstantFunc.OutputParams[0].ParamProp), *PyGenUtil::GetFieldTooltip(InFunc)));
	GeneratedWrappedConstant.ConstantFunc = ConstantFunc;

	// Build any deprecated variants too
	// const TArray<TTuple<FSoftObjectPath, FString>> DeprecatedPythonConstantNames = PyGenUtil::GetDeprecatedScriptConstantPythonNames(InFunc);
	// for (const TTuple<FSoftObjectPath, FString>& DeprecatedPythonConstantNamePair : DeprecatedPythonConstantNames)
	// {
	// 	const FString& DeprecatedPythonConstantName = DeprecatedPythonConstantNamePair.Value;
	// 	FString DeprecationMessage = FString::Printf(TEXT("'%s' was renamed to '%s'."), *DeprecatedPythonConstantName, *PythonConstantName);
	//
	// 	PyGenUtil::FGeneratedWrappedConstant& DeprecatedGeneratedWrappedConstant = ConstantDefs.AddDefaulted_GetRef().Get();
	// 	DeprecatedGeneratedWrappedConstant = GeneratedWrappedConstant;
	// 	DeprecatedGeneratedWrappedConstant.ConstantName = PyGenUtil::TCHARToUTF8Buffer(*DeprecatedPythonConstantName);
	// 	DeprecatedGeneratedWrappedConstant.ConstantDoc = PyGenUtil::TCHARToUTF8Buffer(*FString::Printf(TEXT("deprecated: %s"), *DeprecationMessage));
	// }

	// Add the constant to either the owner type (if specified) or this class
	// if (HostType)
	// {
	// 	if (HostType->IsA<UClass>())
	// 	{
	// 		CallOnceUnlocked.Emplace(
	// 			[this, &OutGeneratedWrappedTypeReferences, &OutDirtyModules, HostType, ConstantDefs = MoveTemp(ConstantDefs), InFunc]() mutable
	// 			{
	// 				const UClass* HostClass = CastChecked<UClass>(HostType);
	//
	// 				// Ensure that we've generated a finalized Python type for this class since we'll be adding this constant to that type
	// 				if (!GenerateWrappedClassType(HostClass, OutGeneratedWrappedTypeReferences, OutDirtyModules, EPyTypeGenerationFlags::ForceShouldExport))
	// 				{
	// 					return;
	// 				}
	//
	// 				// Find the wrapped type for the class as that's what we'll actually add the constant to
	// 				TSharedPtr<PyGenUtil::FGeneratedWrappedClassType> HostedClassGeneratedWrappedType = StaticCastSharedPtr<PyGenUtil::FGeneratedWrappedClassType>(GeneratedWrappedTypes.FindRef(HostClass));
	// 				check(HostedClassGeneratedWrappedType.IsValid());
	//
	// 				UE::TUniqueLock ScopedLock(HostedClassGeneratedWrappedType->Lock);
	//
	// 				// Add the dynamic constants to the struct
	// 				for (TUniqueObj<PyGenUtil::FGeneratedWrappedConstant>& GeneratedWrappedConstantToAdd : ConstantDefs)
	// 				{
	// 					HostedClassGeneratedWrappedType->FieldTracker.RegisterPythonFieldName(UTF8_TO_TCHAR(GeneratedWrappedConstantToAdd->ConstantName.GetData()), InFunc);
	// 					HostedClassGeneratedWrappedType->AddDynamicConstant(MoveTemp(GeneratedWrappedConstantToAdd.Get()));
	// 				}
	// 			}
	// 		);
	// 	}
	// 	else if (HostType->IsA<UScriptStruct>())
	// 	{
	// 		CallOnceUnlocked.Emplace(
	// 			[this, &OutGeneratedWrappedTypeReferences, &OutDirtyModules, HostType, ConstantDefs = MoveTemp(ConstantDefs), InFunc]() mutable
	// 			{
	// 				const UScriptStruct* HostStruct = CastChecked<UScriptStruct>(HostType);
	//
	// 				// Ensure that we've generated a finalized Python type for this struct since we'll be adding this constant to that type
	// 				if (!GenerateWrappedStructType(HostStruct, OutGeneratedWrappedTypeReferences, OutDirtyModules, EPyTypeGenerationFlags::ForceShouldExport))
	// 				{
	// 					return;
	// 				}
	//
	// 				// Find the wrapped type for the struct as that's what we'll actually add the constant to
	// 				TSharedPtr<PyGenUtil::FGeneratedWrappedStructType> HostedStructGeneratedWrappedType = StaticCastSharedPtr<PyGenUtil::FGeneratedWrappedStructType>(GeneratedWrappedTypes.FindRef(HostStruct));
	// 				check(HostedStructGeneratedWrappedType.IsValid());
	//
	// 				UE::TUniqueLock ScopedLock(HostedStructGeneratedWrappedType->Lock);
	//
	// 				// Add the dynamic constants to the struct
	// 				for (TUniqueObj<PyGenUtil::FGeneratedWrappedConstant>& GeneratedWrappedConstantToAdd : ConstantDefs)
	// 				{
	// 					HostedStructGeneratedWrappedType->FieldTracker.RegisterPythonFieldName(UTF8_TO_TCHAR(GeneratedWrappedConstantToAdd->ConstantName.GetData()), InFunc);
	// 					HostedStructGeneratedWrappedType->AddDynamicConstant(MoveTemp(GeneratedWrappedConstantToAdd.Get()));
	// 				}
	// 			}
	// 		);
	// 	}
	// 	else
	// 	{
	// 		checkf(false, TEXT("Unexpected HostType type!"));
	// 	}
	// }
	// else
	{
		// Add the static constants to this type
		for (TUniqueObj<UPyGenUtil::FGeneratedWrappedConstant>& GeneratedWrappedConstantToAdd : ConstantDefs)
		{
			// GeneratedWrappedType->FieldTracker.RegisterPythonFieldName(UTF8_TO_TCHAR(GeneratedWrappedConstantToAdd->ConstantName.GetData()), InFunc);
			GeneratedWrappedType->Constants.TypeConstants.Add(MoveTemp(GeneratedWrappedConstantToAdd.Get()));
		}
	}
}

void FUPyDynamicTypeFactory::GenerateWrappedPropertyForStruct(const UScriptStruct* InStruct,
	FUPyGeneratedWrappedStructType* GeneratedWrappedType, const FProperty* InProp)
{
	// const bool bExportPropertyToScript = PyGenUtil::ShouldExportProperty(InProp);
	// const bool bExportPropertyToEditor = PyGenUtil::ShouldExportEditorOnlyProperty(InProp);

	// if (bExportPropertyToScript || bExportPropertyToEditor)
	{
		// GatherWrappedTypesForPropertyReferences(InProp, OutGeneratedWrappedTypeReferences);

		// const PyGenUtil::FGeneratedWrappedPropertyDoc& GeneratedPropertyDoc = GeneratedWrappedType->PropertyDocs[GeneratedWrappedType->PropertyDocs.Emplace(InProp)];
		// PythonProperties.Add(*GeneratedPropertyDoc.PythonPropName, InProp->GetFName());

		int32 GeneratedWrappedGetSetIndex = INDEX_NONE;
		// if (bExportPropertyToScript)
		{
			GeneratedWrappedGetSetIndex = GeneratedWrappedType->GetSets.TypeGetSets.AddDefaulted();

			UPyGenUtil::FGeneratedWrappedGetSet& GeneratedWrappedGetSet = GeneratedWrappedType->GetSets.TypeGetSets[GeneratedWrappedGetSetIndex];
			GeneratedWrappedGetSet.GetSetName = UPyGenUtil::TCHARToUTF8Buffer(*InProp->GetName());
			// GeneratedWrappedGetSet.GetSetDoc = UPyGenUtil::TCHARToUTF8Buffer(*GeneratedPropertyDoc.DocString);
			GeneratedWrappedGetSet.Prop.SetProperty(InProp);
			GeneratedWrappedGetSet.GetCallback = (getter)&FUPyWrapperStruct::Getter_Impl;
			GeneratedWrappedGetSet.SetCallback = (setter)&FUPyWrapperStruct::Setter_Impl;
			// if (GeneratedWrappedGetSet.Prop.DeprecationMessage.IsSet())
			// {
			// 	PythonDeprecatedProperties.Add(*GeneratedPropertyDoc.PythonPropName, GeneratedWrappedGetSet.Prop.DeprecationMessage.GetValue());
			// }

			//GeneratedWrappedType->FieldTracker.RegisterPythonFieldName(GeneratedPropertyDoc.PythonPropName, InProp);
		}

		// const TArray<TTuple<FSoftObjectPath, FString>> DeprecatedPythonPropNames = PyGenUtil::GetDeprecatedPropertyPythonNames(InProp);
		// for (const TTuple<FSoftObjectPath, FString>& DeprecatedPythonPropNamePair : DeprecatedPythonPropNames)
		// {
		// 	const FString& DeprecatedPythonPropName = DeprecatedPythonPropNamePair.Value;
		// 	FString DeprecationMessage = FString::Printf(TEXT("'%s' was renamed to '%s'."), *DeprecatedPythonPropName, *GeneratedPropertyDoc.PythonPropName);
		// 	PythonProperties.Add(*DeprecatedPythonPropName, InProp->GetFName());
		// 	PythonDeprecatedProperties.Add(*DeprecatedPythonPropName, DeprecationMessage);
		//
		// 	if (GeneratedWrappedGetSetIndex != INDEX_NONE)
		// 	{
		// 		PyGenUtil::UPyGenUtil::FGeneratedWrappedGetSet DeprecatedGeneratedWrappedGetSet = GeneratedWrappedType->GetSets.TypeGetSets[GeneratedWrappedGetSetIndex];
		// 		DeprecatedGeneratedWrappedGetSet.GetSetName = PyGenUtil::TCHARToUTF8Buffer(*DeprecatedPythonPropName);
		// 		DeprecatedGeneratedWrappedGetSet.GetSetDoc = PyGenUtil::TCHARToUTF8Buffer(*FString::Printf(TEXT("deprecated: %s"), *DeprecationMessage));
		// 		DeprecatedGeneratedWrappedGetSet.Prop.DeprecationMessage = MoveTemp(DeprecationMessage);
		// 		GeneratedWrappedType->GetSets.TypeGetSets.Add(MoveTemp(DeprecatedGeneratedWrappedGetSet));
		//
		// 		GeneratedWrappedType->FieldTracker.RegisterPythonFieldName(DeprecatedPythonPropName, InProp);
		// 	}
		// }
	}
}
