// Copyright Epic Games, Inc. All Rights Reserved.

#include "DynamicTypes/UPyGeneratedWrappedType.h"
#include "Core/UPyGIL.h"
#include "Core/UPyPtr.h"
#include "Utils/UPyUtil.h"
#include "Wrapper/UPyWrapperBase.h"

namespace UPyGenUtil
{
	void FGeneratedWrappedFunction::SetFunction(const UFunction* InFunc, const uint32 InSetFuncFlags)
	{
		Func = InFunc;
		InputParams.Reset();
		OutputParams.Reset();
		DeprecationMessage.Reset();

		if (Func && (InSetFuncFlags & SFF_CalculateDeprecationState))
		{
			FString DeprecationMessageStr;
			if (IsDeprecatedFunction(Func, &DeprecationMessageStr))
			{
				DeprecationMessage = MoveTemp(DeprecationMessageStr);
			}
		}

		if (Func && (InSetFuncFlags & SFF_ExtractParameters))
		{
			ExtractFunctionParams(Func, InputParams, OutputParams);
		}
	}


	void FGeneratedWrappedMethod::ToPython(FUPyMethodWithClosureDef& OutPyMethod) const
	{
		OutPyMethod.MethodName = MethodName.GetData();
		OutPyMethod.MethodDoc = MethodDoc.GetData();
		OutPyMethod.MethodCallback = MethodCallback;
		OutPyMethod.MethodFlags = MethodFlags;
		OutPyMethod.MethodClosure = (void*)this;
	}


	void FGeneratedWrappedMethods::Finalize()
	{
		check(PyMethods.Num() == 0);

		PyMethods.Reserve(TypeMethods.Num() + 1);
		for (const FGeneratedWrappedMethod& TypeMethod : TypeMethods)
		{
			FUPyMethodWithClosureDef& PyMethod = PyMethods.AddZeroed_GetRef();
			TypeMethod.ToPython(PyMethod);
		}
		PyMethods.AddZeroed(); // null terminator
	}


	void FGeneratedWrappedDynamicMethodWithClosure::Finalize()
	{
		ToPython(PyMethod);
	}


	void FGeneratedWrappedDynamicMethodsMixinBase::AddDynamicMethodImpl(FGeneratedWrappedDynamicMethod&& InDynamicMethod, PyTypeObject* InPyType)
	{
		TSharedRef<FGeneratedWrappedDynamicMethodWithClosure> DynamicMethod = DynamicMethods.Add_GetRef(MakeShared<FGeneratedWrappedDynamicMethodWithClosure>());
		static_cast<FGeneratedWrappedDynamicMethod&>(*DynamicMethod) = MoveTemp(InDynamicMethod);
		DynamicMethod->Finalize();
		// Execute Python code within this block
		{
			FUPyScopedGIL GIL;
			FUPyMethodWithClosureDef::AddMethod(&DynamicMethod->PyMethod, InPyType);
		}
	}


	const FGeneratedWrappedOperatorSignature& FGeneratedWrappedOperatorSignature::OpTypeToSignature(const EGeneratedWrappedOperatorType InOpType)
	{
		static const FGeneratedWrappedOperatorSignature OperatorSignatures[(int32)EGeneratedWrappedOperatorType::Num] = {
			FGeneratedWrappedOperatorSignature(EGeneratedWrappedOperatorType::Bool,				TEXT("bool"),	TEXT("__bool__"),		EType::Bool,	EType::None),
			FGeneratedWrappedOperatorSignature(EGeneratedWrappedOperatorType::Equal,			TEXT("=="),		TEXT("__eq__"),			EType::Bool,	EType::Any),
			FGeneratedWrappedOperatorSignature(EGeneratedWrappedOperatorType::NotEqual,			TEXT("!="),		TEXT("__ne__"),			EType::Bool,	EType::Any),
			FGeneratedWrappedOperatorSignature(EGeneratedWrappedOperatorType::Less,				TEXT("<"),		TEXT("__lt__"),			EType::Bool,	EType::Any),
			FGeneratedWrappedOperatorSignature(EGeneratedWrappedOperatorType::LessEqual,		TEXT("<="),		TEXT("__le__"),			EType::Bool,	EType::Any),
			FGeneratedWrappedOperatorSignature(EGeneratedWrappedOperatorType::Greater,			TEXT(">"),		TEXT("__gt__"),			EType::Bool,	EType::Any),
			FGeneratedWrappedOperatorSignature(EGeneratedWrappedOperatorType::GreaterEqual,		TEXT(">="),		TEXT("__ge__"),			EType::Bool,	EType::Any),
			FGeneratedWrappedOperatorSignature(EGeneratedWrappedOperatorType::Add,				TEXT("+"),		TEXT("__add__"),		EType::Any,		EType::Any),
			FGeneratedWrappedOperatorSignature(EGeneratedWrappedOperatorType::InlineAdd,		TEXT("+="),		TEXT("__iadd__"),		EType::Struct,	EType::Any),
			FGeneratedWrappedOperatorSignature(EGeneratedWrappedOperatorType::Subtract,			TEXT("-"),		TEXT("__sub__"),		EType::Any,		EType::Any),
			FGeneratedWrappedOperatorSignature(EGeneratedWrappedOperatorType::InlineSubtract,	TEXT("-="),		TEXT("__isub__"),		EType::Struct,	EType::Any),
			FGeneratedWrappedOperatorSignature(EGeneratedWrappedOperatorType::Multiply,			TEXT("*"),		TEXT("__mul__"),		EType::Any,		EType::Any),
			FGeneratedWrappedOperatorSignature(EGeneratedWrappedOperatorType::InlineMultiply,	TEXT("*="),		TEXT("__imul__"),		EType::Struct,	EType::Any),
			FGeneratedWrappedOperatorSignature(EGeneratedWrappedOperatorType::Divide,			TEXT("/"),		TEXT("__truediv__"),	EType::Any,		EType::Any),
			FGeneratedWrappedOperatorSignature(EGeneratedWrappedOperatorType::InlineDivide,		TEXT("/="),		TEXT("__truediv__"),	EType::Struct,	EType::Any),
			FGeneratedWrappedOperatorSignature(EGeneratedWrappedOperatorType::Modulus,			TEXT("%"),		TEXT("__mod__"),		EType::Any,		EType::Any),
			FGeneratedWrappedOperatorSignature(EGeneratedWrappedOperatorType::InlineModulus,	TEXT("%="),		TEXT("__imod__"),		EType::Struct,	EType::Any),
			FGeneratedWrappedOperatorSignature(EGeneratedWrappedOperatorType::And,				TEXT("&"),		TEXT("__and__"),		EType::Any,		EType::Any),
			FGeneratedWrappedOperatorSignature(EGeneratedWrappedOperatorType::InlineAnd,		TEXT("&="),		TEXT("__iand__"),		EType::Struct,	EType::Any),
			FGeneratedWrappedOperatorSignature(EGeneratedWrappedOperatorType::Or,				TEXT("|"),		TEXT("__or__"),			EType::Any,		EType::Any),
			FGeneratedWrappedOperatorSignature(EGeneratedWrappedOperatorType::InlineOr,			TEXT("|="),		TEXT("__ior__"),		EType::Struct,	EType::Any),
			FGeneratedWrappedOperatorSignature(EGeneratedWrappedOperatorType::Xor,				TEXT("^"),		TEXT("__xor__"),		EType::Any,		EType::Any),
			FGeneratedWrappedOperatorSignature(EGeneratedWrappedOperatorType::InlineXor,		TEXT("^="),		TEXT("__ixor__"),		EType::Struct,	EType::Any),
			FGeneratedWrappedOperatorSignature(EGeneratedWrappedOperatorType::RightShift,		TEXT(">>"),		TEXT("__rshift__"),		EType::Any,		EType::Any),
			FGeneratedWrappedOperatorSignature(EGeneratedWrappedOperatorType::InlineRightShift,	TEXT(">>="),	TEXT("__irshift__"),	EType::Struct,	EType::Any),
			FGeneratedWrappedOperatorSignature(EGeneratedWrappedOperatorType::LeftShift,		TEXT("<<"),		TEXT("__lshift__"),		EType::Any,		EType::Any),
			FGeneratedWrappedOperatorSignature(EGeneratedWrappedOperatorType::InlineLeftShift,	TEXT("<<="),	TEXT("__ilshift__"),	EType::Struct,	EType::Any),
			FGeneratedWrappedOperatorSignature(EGeneratedWrappedOperatorType::Negated,			TEXT("neg"),	TEXT("__neg__"),		EType::Struct,	EType::None),
		};

		check(InOpType != EGeneratedWrappedOperatorType::Num);
		return OperatorSignatures[(int32)InOpType];
	}

	bool FGeneratedWrappedOperatorSignature::StringToSignature(const TCHAR* InStr, FGeneratedWrappedOperatorSignature& OutSignature)
	{
		for (int32 OpTypeIndex = 0; OpTypeIndex < (int32)EGeneratedWrappedOperatorType::Num; ++OpTypeIndex)
		{
			const FGeneratedWrappedOperatorSignature& PotentialSignature = OpTypeToSignature((EGeneratedWrappedOperatorType)OpTypeIndex);
			if (FCString::Strcmp(InStr, PotentialSignature.OpTypeStr) == 0)
			{
				check(OpTypeIndex == (int32)PotentialSignature.OpType);
				OutSignature = PotentialSignature;
				return true;
			}
		}

		return false;
	}

	bool FGeneratedWrappedOperatorSignature::ValidateParam(const FGeneratedWrappedMethodParameter& InParam, const EType InType, const UScriptStruct* InStructType, FString* OutError)
	{
		switch (InType)
		{
		case EType::None:
			if (InParam.ParamProp)
			{
				if (OutError)
				{
					*OutError = TEXT("Expected None");
				}
				return false;
			}
			return true;

		case EType::Any:
			if (!InParam.ParamProp)
			{
				if (OutError)
				{
					*OutError = TEXT("Expected Any");
				}
				return false;
			}
			return true;

		case EType::Struct:
			if (!InParam.ParamProp || !InParam.ParamProp->IsA<FStructProperty>() || (InStructType && CastFieldChecked<const FStructProperty>(InParam.ParamProp)->Struct != InStructType))
			{
				if (OutError)
				{
					*OutError = FString::Printf(TEXT("Expected Struct (%s)"), *InStructType->GetName());
				}
				return false;
			}
			return true;

		case EType::Bool:
			if (!InParam.ParamProp || !InParam.ParamProp->IsA<FBoolProperty>())
			{
				if (OutError)
				{
					*OutError = TEXT("Expected Bool");
				}
				return false;
			}
			return true;

		default:
			checkf(false, TEXT("Unexpected parameter type!"));
			break;
		}

		return false;
	}

	int32 FGeneratedWrappedOperatorSignature::GetInputParamCount() const
	{
		return OtherType == EType::None ? 1 : 2;
	}

	int32 FGeneratedWrappedOperatorSignature::GetOutputParamCount() const
	{
		return ReturnType == EType::None ? 0 : 1;
	}


	bool FGeneratedWrappedOperatorFunction::SetFunction(const UFunction* InFunc, const FGeneratedWrappedOperatorSignature& InSignature, FString* OutError)
	{
		FGeneratedWrappedFunction FuncDef;
		FuncDef.SetFunction(InFunc);
		return SetFunction(FuncDef, InSignature, OutError);
	}

	bool FGeneratedWrappedOperatorFunction::SetFunction(const FGeneratedWrappedFunction& InFuncDef, const FGeneratedWrappedOperatorSignature& InSignature, FString* OutError)
	{
		const int32 ExpectedInputParamCount = InSignature.GetInputParamCount();
		const int32 ExpectedOutputParamCount = InSignature.GetOutputParamCount();

		// Count the number of significant (non-defaulted) input parameters
		// We allow additional input parameters as long as they're defaulted and the basic signature requirements are met
		int32 SignificantInputParamCount = 0;
		for (const FGeneratedWrappedMethodParameter& InputParam : InFuncDef.InputParams)
		{
			if (!InputParam.ParamDefaultValue.IsSet())
			{
				++SignificantInputParamCount;
			}
		}

		// In some cases a required input argument may have also been defaulted, so as long as we have enough 
		// input parameters without having too many significant input parameters, still accept this function
		const bool bValidInputParamCount = SignificantInputParamCount <= ExpectedInputParamCount && InFuncDef.InputParams.Num() >= ExpectedInputParamCount;
		const bool bValidOutputParamCount = InFuncDef.OutputParams.Num() == ExpectedOutputParamCount;
		if (!bValidInputParamCount || !bValidOutputParamCount)
		{
			if (OutError)
			{
				*OutError = FString::Printf(TEXT("Incorrect number of arguments; expected %d input and %d output, but got %d input (%d default) and %d output"), ExpectedInputParamCount, ExpectedOutputParamCount, InFuncDef.InputParams.Num(), InFuncDef.InputParams.Num() - SignificantInputParamCount, InFuncDef.OutputParams.Num());
			}
			return false;
		}

		// The 'self' parameter should be the first parameter
		check(InFuncDef.InputParams.IsValidIndex(0)); // always expect a 'self' argument; ExpectedInputParamCount should have verified this
		if (InFuncDef.InputParams[0].ParamProp->IsA<FStructProperty>())
		{
			SelfParam = InFuncDef.InputParams[0];
		}
		else
		{
			if (OutError)
			{
				*OutError = TEXT("A valid struct was not found as the first argument");
			}
			return false;
		}

		// Extract and validate the 'other' parameter
		if (ExpectedInputParamCount > 1 && InFuncDef.InputParams.IsValidIndex(1))
		{
			FString OtherParamError;
			if (FGeneratedWrappedOperatorSignature::ValidateParam(InFuncDef.InputParams[1], InSignature.OtherType, CastFieldChecked<const FStructProperty>(SelfParam.ParamProp)->Struct, &OtherParamError))
			{
				OtherParam = InFuncDef.InputParams[1];
			}
			else
			{
				if (OutError)
				{
					*OutError = FString::Printf(TEXT("Other parameter was invalid (%s)"), *OtherParamError);
				}
				return false;
			}
		}

		// Extract any additional input parameters - these should all be defaulted
		for (int32 AdditionalParamIndex = ExpectedInputParamCount; AdditionalParamIndex < InFuncDef.InputParams.Num(); ++AdditionalParamIndex)
		{
			const FGeneratedWrappedMethodParameter& InputParam = InFuncDef.InputParams[AdditionalParamIndex];
			check(InputParam.ParamDefaultValue.IsSet());
			AdditionalParams.Add(InputParam);
		}

		// Extract and validate the return type
		if (InFuncDef.OutputParams.IsValidIndex(0))
		{
			FString ReturnValueError;
			if (FGeneratedWrappedOperatorSignature::ValidateParam(InFuncDef.OutputParams[0], InSignature.ReturnType, CastFieldChecked<const FStructProperty>(SelfParam.ParamProp)->Struct, &ReturnValueError))
			{
				ReturnParam = InFuncDef.OutputParams[0];
				if (InSignature.ReturnType == FGeneratedWrappedOperatorSignature::EType::Struct)
				{
					SelfReturn = InFuncDef.OutputParams[0];
				}
			}
			else
			{
				if (OutError)
				{
					*OutError = FString::Printf(TEXT("Return value was invalid (%s)"), *ReturnValueError);
				}
				return false;
			}
		}

		Func = InFuncDef.Func;

		return true;
	}


	void FGeneratedWrappedProperty::SetProperty(const FProperty* InProp, const uint32 InSetPropFlags)
	{
		Prop = InProp;
		DeprecationMessage.Reset();

		if (Prop && (InSetPropFlags & SPF_CalculateDeprecationState))
		{
			FString DeprecationMessageStr;
			if (IsDeprecatedProperty(Prop, &DeprecationMessageStr))
			{
				DeprecationMessage = MoveTemp(DeprecationMessageStr);
			}
		}
	}


	void FGeneratedWrappedGetSet::ToPython(PyGetSetDef& OutPyGetSet) const
	{
		OutPyGetSet.name = (char*)GetSetName.GetData();
		OutPyGetSet.doc = (char*)GetSetDoc.GetData();
		OutPyGetSet.get = GetCallback;
		OutPyGetSet.set = SetCallback;
		OutPyGetSet.closure = (void*)this;
	}


	void FGeneratedWrappedGetSets::Finalize()
	{
		check(PyGetSets.Num() == 0);

		PyGetSets.Reserve(TypeGetSets.Num() + 1);
		for (const FGeneratedWrappedGetSet& TypeGetSet : TypeGetSets)
		{
			PyGetSetDef& PyGetSet = PyGetSets.AddZeroed_GetRef();
			TypeGetSet.ToPython(PyGetSet);
		}
		PyGetSets.AddZeroed(); // null terminator
	}


	void FGeneratedWrappedConstant::ToPython(FUPyConstantDef& OutPyConstant) const
	{
		auto ConstantGetterImpl = [](PyTypeObject* InType, const void* InClosure) -> PyObject*
		{
			const FGeneratedWrappedConstant* This = (FGeneratedWrappedConstant*)InClosure;
		
			if (ensureAlways(This->ConstantFunc.Func))
			{
				const FString ErrorCtxt = UPyUtil::GetErrorContext(InType);

				UClass* Class = This->ConstantFunc.Func->GetOwnerClass();
				UObject* Obj = Class->GetDefaultObject();
	
				// Deprecated functions emit a warning
				if (This->ConstantFunc.DeprecationMessage.IsSet())
				{
					if (UPyUtil::SetPythonWarning(PyExc_DeprecationWarning, *ErrorCtxt, *FString::Printf(TEXT("Constant '%s' on '%s' is deprecated: %s"), UTF8_TO_TCHAR(This->ConstantName.GetData()), *UPyUtil::GetCleanTypename(InType), *This->ConstantFunc.DeprecationMessage.GetValue())) == -1)
					{
						// -1 from SetPythonWarning means the warning should be an exception
						return nullptr;
					}
				}
	
				// Return value requires that we create a params struct to hold the result
				UPY_UFUNCTION_STACK(FuncParams, This->ConstantFunc.Func);
				if (!UPyUtil::InvokeFunctionCall(Obj, This->ConstantFunc.Func, FuncParams.GetMemory(), *ErrorCtxt))
				{
					return nullptr;
				}
				return UPyGenUtil::PackReturnValues(FuncParams.GetMemory(), This->ConstantFunc.OutputParams, *ErrorCtxt, *FString::Printf(TEXT("constant '%s' on '%s'"), UTF8_TO_TCHAR(This->ConstantName.GetData()), *UPyUtil::GetCleanTypename(InType)));
			}
	
			Py_RETURN_NONE;
		};

		OutPyConstant.ConstantContext = this;
		OutPyConstant.ConstantGetter = ConstantGetterImpl;
		OutPyConstant.ConstantName = ConstantName.GetData();
		OutPyConstant.ConstantDoc = ConstantDoc.GetData();
	}


	void FGeneratedWrappedConstants::Finalize()
	{
		check(PyConstants.Num() == 0);

		PyConstants.Reserve(TypeConstants.Num() + 1);
		for (const FGeneratedWrappedConstant& TypeConstant : TypeConstants)
		{
			FUPyConstantDef& PyConstant = PyConstants.AddZeroed_GetRef();
			TypeConstant.ToPython(PyConstant);
		}
		PyConstants.AddZeroed(); // null terminator
	}


	void FGeneratedWrappedDynamicConstantWithClosure::Finalize()
	{
		ToPython(PyConstant);
	}


	void FGeneratedWrappedDynamicConstantsMixinBase::AddDynamicConstantImpl(FGeneratedWrappedConstant&& InDynamicConstant, PyTypeObject* InPyType)
	{
		TSharedRef<FGeneratedWrappedDynamicConstantWithClosure> DynamicConstant = DynamicConstants.Add_GetRef(MakeShared<FGeneratedWrappedDynamicConstantWithClosure>());
		static_cast<FGeneratedWrappedConstant&>(*DynamicConstant) = MoveTemp(InDynamicConstant);
		DynamicConstant->Finalize();
		// Execute Python code within this block
		{
			FUPyScopedGIL GIL;
			FUPyConstantDef::AddConstantToType(&DynamicConstant->PyConstant, InPyType);
		}
	}


	FGeneratedWrappedPropertyDoc::FGeneratedWrappedPropertyDoc(const FProperty* InProp)
	{
		PythonPropName = GetPropertyPythonName(InProp);

		const FString PropTooltip = GetFieldTooltip(InProp);
		const FParsedTooltip ParsedPropTooltip = ParseTooltip(PropTooltip);
		DocString = PythonizePropertyTooltip(ParsedPropTooltip, InProp, PropertyAccessUtil::RuntimeReadOnlyFlags);
		EditorDocString = PythonizePropertyTooltip(ParsedPropTooltip, InProp, PropertyAccessUtil::EditorReadOnlyFlags);
	}

	bool FGeneratedWrappedPropertyDoc::SortPredicate(const FGeneratedWrappedPropertyDoc& InOne, const FGeneratedWrappedPropertyDoc& InTwo)
	{
		return InOne.PythonPropName < InTwo.PythonPropName;
	}

	FString FGeneratedWrappedPropertyDoc::BuildDocString(const TArray<FGeneratedWrappedPropertyDoc>& InDocs)
	{
		FString Str;
		AppendDocString(InDocs, Str);
		return Str;
	}

	void FGeneratedWrappedPropertyDoc::AppendDocString(const TArray<FGeneratedWrappedPropertyDoc>& InDocs, FString& OutStr)
	{
		if (!InDocs.Num())
		{
			return;
		}

		TStringBuilder<4096> Buffer;
		if (!OutStr.IsEmpty())
		{
			if (OutStr[OutStr.Len() - 1] != TEXT('\n'))
			{
				Buffer += LINE_TERMINATOR;
			}
		}

		Buffer += LINE_TERMINATOR TEXT("**Editor Properties:** (see get_editor_property/set_editor_property)") LINE_TERMINATOR;
		for (const FGeneratedWrappedPropertyDoc& Doc : InDocs)
		{
			Buffer += LINE_TERMINATOR TEXT("- ``");  // add as a list and code style
			Buffer += Doc.PythonPropName;
			Buffer += TEXT("`` ");

			bool bMultipleLines = false;

			const FStringView EditorDocString = Doc.EditorDocString;
			int32 EditorDocStringParseIndex = 0;

			while (EditorDocStringParseIndex < EditorDocString.Len())
			{
				if (bMultipleLines)
				{
					Buffer += LINE_TERMINATOR TEXT("  ");
				}
				bMultipleLines = true;

				const int32 LineStartIndex = EditorDocStringParseIndex;
				while (EditorDocStringParseIndex < EditorDocString.Len() && !FChar::IsLinebreak(EditorDocString[EditorDocStringParseIndex]))
				{
					++EditorDocStringParseIndex;
				}

				Buffer += EditorDocString.Mid(LineStartIndex, EditorDocStringParseIndex - LineStartIndex);

				if (EditorDocStringParseIndex + 1 < EditorDocString.Len() && EditorDocString[EditorDocStringParseIndex] == TEXT('\r') && EditorDocString[EditorDocStringParseIndex + 1] == TEXT('\n'))
				{
					EditorDocStringParseIndex += 2;
				}
				else if (EditorDocStringParseIndex < EditorDocString.Len() && FChar::IsLinebreak(EditorDocString[EditorDocStringParseIndex]))
				{
					++EditorDocStringParseIndex;
				}
			}
		}

		OutStr += Buffer.ToString();
	}


	void FGeneratedWrappedFieldTracker::RegisterPythonFieldName(const FString& InPythonFieldName, const FFieldVariant& InUnrealField)
	{
		FFieldVariant ExistingUnrealField;
		const UField* ExistingUField = nullptr;
		const FField* ExistingFField = nullptr;

		if (InUnrealField.IsUObject())
		{
			ExistingUField = PythonWrappedFieldNameToUnrealField.FindRef(InPythonFieldName).Get();
			if (!ExistingUField)
			{
				PythonWrappedFieldNameToUnrealField.Add(InPythonFieldName, InUnrealField.Get<UField>());
			}
			else
			{
				ExistingUnrealField = ExistingUField;
			}
		}
		else
		{
			ExistingFField = PythonWrappedFieldNameToFField.FindRef(InPythonFieldName).Get();
			if (!ExistingFField)
			{
				PythonWrappedFieldNameToFField.Add(InPythonFieldName, InUnrealField.ToField());
			}
			else
			{
				ExistingUnrealField = ExistingFField;
			}
		}
		if (ExistingUnrealField.IsValid())
		{
			auto GetScopedFieldName = [](const FFieldVariant& InField) -> FString
			{
				// Note: We don't use GetOwnerStruct here, as UFunctions are UStructs so it
				// doesn't work correctly for them as it includes 'this' in the look-up chain
				FFieldVariant OwnerStruct = InField.GetOwnerVariant();
				while (OwnerStruct.IsValid() && !OwnerStruct.IsA<UStruct>())
				{
					OwnerStruct = OwnerStruct.GetOwnerVariant();
				}
				return OwnerStruct.IsValid() ? FString::Printf(TEXT("%s.%s"), *OwnerStruct.GetName(), *InField.GetName()) : InField.GetName();
			};

			REPORT_UNREAL_PYTHON_GENERATION_ISSUE(Warning, TEXT("'%s' and '%s' have the same name (%s) when exposed to Python. Rename one of them using 'ScriptName' meta-data (or 'ScriptMethod' or 'ScriptConstant' for extension functions)."), *GetScopedFieldName(ExistingUnrealField), *GetScopedFieldName(InUnrealField), *InPythonFieldName);
		}
	}


	bool FGeneratedWrappedType::Finalize()
	{
		Finalize_PreReady();

		bool bSuccess = false;
		// Execute Python code within this block
		if (FinalizedState != EFinalizedState::Finalized)
		{
			FUPyScopedGIL GIL;
			if (FinalizedState == EFinalizedState::Initial)
			{
				bSuccess = PyType_Ready(&PyType) == 0;
			}
			else if (FinalizedState == EFinalizedState::Reset)
			{
				PyType_Modified(&PyType);

				// PyType_Modified doesn't update __doc__
				FUPyObjectPtr PyDocString = FUPyObjectPtr::StealReference(PyUnicode_FromString(PyType.tp_doc ? PyType.tp_doc : ""));
				PyDict_SetItemString(PyType.tp_dict, "__doc__", PyDocString);

				bSuccess = true;
			}
			FinalizedState = EFinalizedState::Finalized;

			// Run these under the same GIL as above since we're going to need it in both functions below
			// and it will avoid 2 more locks in rapid successions.
			if (bSuccess)
			{
				Finalize_PostReady();
                // todo(hzn): meta data
				// FUPyWrapperBaseMetaData::SetMetaData(&PyType, MetaData.Get());
				return true;
			}
		}

		return false;
	}

	void FGeneratedWrappedType::Reset()
	{
		Reset_CleansePyType();
		Reset_CleanseSelf();
	}

	void FGeneratedWrappedType::Finalize_PreReady()
	{
		PyType.tp_name = TypeName.GetData();
		PyType.tp_doc = TypeDoc.GetData();
	}

	void FGeneratedWrappedType::Finalize_PostReady()
	{
	}

	void FGeneratedWrappedType::Reset_CleansePyType()
	{
		PyType.tp_name = nullptr;
		PyType.tp_doc = nullptr;

		FUPyWrapperBaseMetaData::SetMetaData(&PyType, nullptr);
	}

	void FGeneratedWrappedType::Reset_CleanseSelf()
	{
		TypeName.Reset();
		TypeDoc.Reset();
		MetaData.Reset();
		FinalizedState = EFinalizedState::Reset;
	}
}

