
#include "Wrapper/UPyWrapperStruct.h"
#include "Wrapper/UPyWrapperTypeRegistry.h"
#include "Wrapper/UPyWrapperTypeFactory.h"
#include "StructUtils/UserDefinedStruct.h"
#include "Utils/UPyUtil.h"

extern PyMethodDef StructPyMethodDefs[];

PyTypeObject UPyWrapperStructType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"StructBase", /* tp_name */
	sizeof(FUPyWrapperStruct), /* tp_basicsize */
};

const IUPyWrapperStructAllocationPolicy* GetPyWrapperStructAllocationPolicy(const UScriptStruct* InStruct)
{
	class FUPyWrapperStructAllocationPolicy_Heap : public IUPyWrapperStructAllocationPolicy
	{
		virtual void* AllocateStruct(const FUPyWrapperStruct* InSelf, const UScriptStruct* InStruct) const override
		{
			return FMemory::Malloc(FMath::Max(InStruct->GetStructureSize(), 1));
		}

		virtual void FreeStruct(const FUPyWrapperStruct* InSelf, void* InAlloc) const override
		{
			FMemory::Free(InAlloc);
		}
	};

	if (const IUPyWrapperInlineStructFactory* InlineStructFactory = FUPyWrapperTypeRegistry::Get().GetInlineStructFactory(InStruct->GetStructPathName()))
	{
		return InlineStructFactory->GetPythonObjectAllocationPolicy();
	}

	static const FUPyWrapperStructAllocationPolicy_Heap HeapAllocPolicy = FUPyWrapperStructAllocationPolicy_Heap();
	return &HeapAllocPolicy;
}

FUPyWrapperStruct* FUPyWrapperStruct::New(PyTypeObject* InType)
{
	FUPyWrapperStruct* Self = (FUPyWrapperStruct*)FUPyWrapperBase::New(InType);
	if (Self)
	{
		new(&Self->OwnerContext) FUPyWrapperOwnerContext();
		Self->ScriptStruct = nullptr;
		Self->StructInstance = nullptr;
		if (FUPyWrapperTypeRegistry::Get().FindStruct(InType) && Init(Self) != 0)
		{
			Free(Self);
			return nullptr;
		}
	}
	return Self;
}

void FUPyWrapperStruct::Free(FUPyWrapperStruct* InSelf)
{
	Deinit(InSelf);

	InSelf->OwnerContext.~FUPyWrapperOwnerContext();
	FUPyWrapperBase::Free(InSelf);
}

int FUPyWrapperStruct::Init(FUPyWrapperStruct* InSelf)
{
	Deinit(InSelf);

	const int BaseInit = FUPyWrapperBase::Init(InSelf);
	if (BaseInit != 0)
	{
		return BaseInit;
	}
	
	const UScriptStruct* Struct = FUPyWrapperTypeRegistry::Get().FindStruct(Py_TYPE(InSelf));
	if (!Struct)
	{
		UPyUtil::SetPythonError(PyExc_Exception, InSelf, TEXT("Struct is null"));
		return -1;
	}

	const IUPyWrapperStructAllocationPolicy* AllocPolicy = GetPyWrapperStructAllocationPolicy((UScriptStruct*)Struct);
	if (!AllocPolicy)
	{
		UPyUtil::SetPythonError(PyExc_Exception, InSelf, TEXT("AllocPolicy is null"));
		return -1;
	}

	// Deprecated structs emit a warning
	// {
	// 	FString DeprecationMessage;
	// 	if (FUPyWrapperStructMetaData::IsStructDeprecated(InSelf, &DeprecationMessage) &&
	// 		UPyUtil::SetPythonWarning(PyExc_DeprecationWarning, InSelf, *FString::Printf(TEXT("Struct '%s' is deprecated: %s"), UTF8_TO_TCHAR(Py_TYPE(InSelf)->tp_name), *DeprecationMessage)) == -1
	// 		)
	// 	{
	// 		// -1 from SetPythonWarning means the warning should be an exception
	// 		return -1;
	// 	}
	// }

	void* StructInstance = AllocPolicy->AllocateStruct(InSelf, (UScriptStruct*)Struct);
	Struct->InitializeStruct(StructInstance);

	InSelf->ScriptStruct = (UScriptStruct*)Struct;
	InSelf->StructInstance = StructInstance;

	// todo(hzn): 是否要MapInstance
	// FUPyWrapperStructFactory::Get().MapInstance(InSelf->StructInstance, InSelf);
	return 0;
}

int FUPyWrapperStruct::Init(FUPyWrapperStruct* InSelf, const FUPyWrapperOwnerContext& InOwnerContext, const UScriptStruct* InStruct, void* InValue, const EUPyConversionMethod InConversionMethod)
{
	InOwnerContext.AssertValidConversionMethod(InConversionMethod);

	Deinit(InSelf);

	const int BaseInit = FUPyWrapperBase::Init(InSelf);
	if (BaseInit != 0)
	{
		return BaseInit;
	}

	check(InValue);

	const IUPyWrapperStructAllocationPolicy* AllocPolicy = GetPyWrapperStructAllocationPolicy(InStruct);
	if (!AllocPolicy)
	{
		UPyUtil::SetPythonError(PyExc_Exception, InSelf, TEXT("AllocPolicy is null"));
		return -1;
	}

	void* StructInstanceToUse = nullptr;
	switch (InConversionMethod)
	{
	case EUPyConversionMethod::Copy:
	case EUPyConversionMethod::Steal:
		{
			StructInstanceToUse = AllocPolicy->AllocateStruct(InSelf, InStruct);
			InStruct->InitializeStruct(StructInstanceToUse);
			InStruct->CopyScriptStruct(StructInstanceToUse, InValue);
		}
		break;

	case EUPyConversionMethod::Reference:
		{
			StructInstanceToUse = InValue;

			InOwnerContext.AddOwnedPyProp(InSelf);
		}
		break;

	default:
		checkf(false, TEXT("Unknown EUPyConversionMethod"));
		break;
	}

	check(StructInstanceToUse);

	InSelf->OwnerContext = InOwnerContext;
	InSelf->ScriptStruct = InStruct;
	InSelf->StructInstance = StructInstanceToUse;

	// todo(hzn): 是否有必要所有的结构体都加入map
	if (InOwnerContext.HasOwner())
	{
		FUPyWrapperStructFactory::Get().MapInstance(InSelf->StructInstance, InSelf);
	}
	return 0;
}

void FUPyWrapperStruct::Deinit(FUPyWrapperStruct* InSelf)
{
	if (InSelf->StructInstance)
	{
		FUPyWrapperStructFactory::Get().UnmapInstance(InSelf->StructInstance);
	}

	if (InSelf->OwnerContext.HasOwner())
	{
		InSelf->OwnerContext.Reset();
	}
	else if (InSelf->StructInstance)
	{
		// During exit purge, we skip destroying and freeing the struct to avoid crashes due to
		// memory corruption or invalid state of dependencies (e.g. ScriptStruct).
		// The OS will reclaim the memory anyway.
		if (!GExitPurge)
		{
			if (InSelf->ScriptStruct)
			{
				InSelf->ScriptStruct->DestroyStruct(InSelf->StructInstance);
			}

			const IUPyWrapperStructAllocationPolicy* AllocPolicy = GetPyWrapperStructAllocationPolicy(InSelf->ScriptStruct);
			if (AllocPolicy)
			{
				AllocPolicy->FreeStruct(InSelf, InSelf->StructInstance);
			}
		}
	}
	InSelf->ScriptStruct = nullptr;
	InSelf->StructInstance = nullptr;
}

bool FUPyWrapperStruct::DetachFromOwner(FUPyWrapperStruct* InSelf)
{
	if (!InSelf || !InSelf->OwnerContext.HasOwner())
	{
		return true;
	}

	if (!InSelf->ScriptStruct || !InSelf->StructInstance)
	{
		return false;
	}

	const IUPyWrapperStructAllocationPolicy* AllocPolicy = GetPyWrapperStructAllocationPolicy(InSelf->ScriptStruct);
	if (!AllocPolicy)
	{
		return false;
	}

	void* OldStructInstance = InSelf->StructInstance;
	void* NewStructInstance = AllocPolicy->AllocateStruct(InSelf, InSelf->ScriptStruct);
	if (!NewStructInstance)
	{
		return false;
	}

	InSelf->ScriptStruct->InitializeStruct(NewStructInstance);
	InSelf->ScriptStruct->CopyScriptStruct(NewStructInstance, OldStructInstance);

	FUPyWrapperStructFactory::Get().UnmapInstance(OldStructInstance);
	InSelf->OwnerContext.Reset();
	InSelf->StructInstance = NewStructInstance;
	return true;
}

bool FUPyWrapperStruct::ValidateInternalState(FUPyWrapperStruct* InSelf)
{
	if (!InSelf->ScriptStruct)
	{
		UPyUtil::SetPythonError(PyExc_Exception, Py_TYPE(InSelf), TEXT("Internal Error - ScriptStruct is null!"));
		return false;
	}

	if (!InSelf->StructInstance)
	{
		UPyUtil::SetPythonError(PyExc_Exception, Py_TYPE(InSelf), TEXT("Internal Error - StructInstance is null!"));
		return false;
	}

	return true;
}

FUPyWrapperStruct* FUPyWrapperStruct::CastPyObject(PyObject* InPyObject, FUPyConversionResult* OutCastResult)
{
	SetOptionalUPyConversionResult(FUPyConversionResult::Failure(), OutCastResult);

	if (PyObject_IsInstance(InPyObject, (PyObject*)&UPyWrapperStructType) == 1)
	{
		FUPyWrapperStruct* Self = (FUPyWrapperStruct*)InPyObject;
		if (!ValidateInternalState(Self))
		{
			return nullptr;
		}

		SetOptionalUPyConversionResult(FUPyConversionResult::Success(), OutCastResult);

		Py_INCREF(InPyObject);
		return Self;
	}

	return nullptr;
}

FUPyWrapperStruct* FUPyWrapperStruct::CastPyObject(PyObject* InPyObject, PyTypeObject* InType, FUPyConversionResult* OutCastResult)
{
	if (PyObject_IsInstance(InPyObject, (PyObject*)InType) == 1 && (InType == &UPyWrapperStructType || PyObject_IsInstance(InPyObject, (PyObject*)&UPyWrapperStructType) == 1))
	{
		FUPyWrapperStruct* Self = (FUPyWrapperStruct*)InPyObject;
		if (!ValidateInternalState(Self))
		{
			SetOptionalUPyConversionResult(FUPyConversionResult::Failure(), OutCastResult);
			return nullptr;
		}

		SetOptionalUPyConversionResult(Py_TYPE(InPyObject) == InType ? FUPyConversionResult::Success() : FUPyConversionResult::SuccessWithCoercion(), OutCastResult);

		Py_INCREF(InPyObject);
		return Self;
	}
	SetOptionalUPyConversionResult(FUPyConversionResult::Failure(), OutCastResult);
	
	//
	// if (UPyUtil::HasLength(InPyObject) && !UPyUtil::IsMappingType(InPyObject))
	// {
	// 	FUPyWrapperStructPtr NewStruct = FUPyWrapperStructPtr::StealReference(FUPyWrapperStruct::New(InType));
	// 	if (FUPyWrapperStruct::Init(NewStruct) != 0)
	// 	{
	// 		return nullptr;
	// 	}
	//
	// 	FPyWrapperStructMetaData* StructMetaData = FPyWrapperStructMetaData::GetMetaData(NewStruct);
	// 	if (!StructMetaData)
	// 	{
	// 		return nullptr;
	// 	}
	//
	// 	// Don't allow conversion from sequences with more items than we have InitParams
	// 	const int32 SequenceLen = PyObject_Length(InPyObject);
	// 	if (SequenceLen > StructMetaData->InitParams.Num())
	// 	{
	// 		UPyUtil::SetPythonError(PyExc_Exception, NewStruct.Get(), *FString::Printf(TEXT("Struct has %d initialization parameters, but the given sequence had %d elements"), StructMetaData->InitParams.Num(), SequenceLen));
	// 		return nullptr;
	// 	}
	//
	// 	// Attempt to convert each entry in the sequence to the corresponding struct entry
	// 	FUPyObjectPtr PyObjIter = FUPyObjectPtr::StealReference(PyObject_GetIter(InPyObject));
	// 	if (PyObjIter)
	// 	{
	// 		for (const UPyGenUtil::FGeneratedWrappedMethodParameter& InitParam : StructMetaData->InitParams)
	// 		{
	// 			FUPyObjectPtr SequenceItem = FUPyObjectPtr::StealReference(PyIter_Next(PyObjIter));
	// 			if (!SequenceItem)
	// 			{
	// 				if (PyErr_Occurred())
	// 				{
	// 					return nullptr;
	// 				}
	// 				break;
	// 			}
	//
	// 			const TArray<void*> NoArchetypeInsts;
	// 			const int Result = UPyUtil::SetPropertyValue(NewStruct->ScriptStruct, NewStruct->StructInstance, SequenceItem, InitParam.ParamProp, InitParam.ParamName.GetData(), nullptr, 0, false, *UPyUtil::GetErrorContext(NewStruct.Get()), NoArchetypeInsts);
	// 			if (Result != 0)
	// 			{
	// 				return nullptr;
	// 			}
	// 		}
	// 	}
	//
	// 	SetOptionalUPyConversionResult(FUPyConversionResult::SuccessWithCoercion(), OutCastResult);
	// 	return NewStruct.Release();
	// }
	//
	// if (UPyUtil::IsMappingType(InPyObject))
	// {
	// 	FUPyWrapperStructPtr NewStruct = FUPyWrapperStructPtr::StealReference(FUPyWrapperStruct::New(InType));
	// 	if (FUPyWrapperStruct::Init(NewStruct) != 0)
	// 	{
	// 		return nullptr;
	// 	}
	//
	// 	FPyWrapperStructMetaData* StructMetaData = FPyWrapperStructMetaData::GetMetaData(NewStruct);
	// 	if (!StructMetaData)
	// 	{
	// 		return nullptr;
	// 	}
	//
	// 	// Don't allow conversion from dicts with more items than we have InitParams
	// 	const int32 DictLen = PyObject_Length(InPyObject);
	// 	if (DictLen > StructMetaData->InitParams.Num())
	// 	{
	// 		UPyUtil::SetPythonError(PyExc_Exception, NewStruct.Get(), *FString::Printf(TEXT("Struct has %d initialization parameters, but the given dict had %d elements"), StructMetaData->InitParams.Num(), DictLen));
	// 		return nullptr;
	// 	}
	//
	// 	// Attempt to convert each matching entry in the dict to the corresponding struct entry
	// 	for (const UPyGenUtil::FGeneratedWrappedMethodParameter& InitParam : StructMetaData->InitParams)
	// 	{
	// 		PyObject* MappingItem = PyMapping_GetItemString(InPyObject, (char*)InitParam.ParamName.GetData());
	// 		if (MappingItem)
	// 		{
	// 			TArray<void*> NoArchetypeInsts;
	// 			const int Result = UPyUtil::SetPropertyValue(NewStruct->ScriptStruct, NewStruct->StructInstance, MappingItem, InitParam.ParamProp, InitParam.ParamName.GetData(), nullptr, 0, false, *UPyUtil::GetErrorContext(NewStruct.Get()), NoArchetypeInsts);
	// 			if (Result != 0)
	// 			{
	// 				return nullptr;
	// 			}
	// 		}
	// 		else
	// 		{
	// 			// Clear the look-up error
	// 			PyErr_Clear();
	// 		}
	// 	}
	//
	// 	SetOptionalUPyConversionResult(FUPyConversionResult::SuccessWithCoercion(), OutCastResult);
	// 	return NewStruct.Release();
	// }

	return nullptr;
}

int FUPyWrapperStruct::MakeStruct(FUPyWrapperStruct* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	// if (!ValidateInternalState(InSelf))
	// {
	// 	return -1;
	// }
	//
	// FPyWrapperStructMetaData* StructMetaData = FPyWrapperStructMetaData::GetMetaData(InSelf);
	// if (!StructMetaData)
	// {
	// 	return -1;
	// }
	//
	// // If this struct has a custom make function, use that rather than the generic version
	// if (StructMetaData->MakeFunc.Func)
	// {
	// 	return CallMakeFunction_Impl(InSelf, InArgs, InKwds, StructMetaData->MakeFunc);
	// }
	//
	// // We can early out if we have no data to apply
	// if (PyTuple_Size(InArgs) == 0 && (!InKwds || PyDict_Size(InKwds) == 0))
	// {
	// 	return 0;
	// }
	//
	// // Generic implementation just tries to assign each property
	// TArray<PyObject*> Params;
	// if (!UPyGenUtil::ParseMethodParameters(InArgs, InKwds, StructMetaData->InitParams, "call", Params))
	// {
	// 	return -1;
	// }
	//
	// for (int32 ParamIndex = 0; ParamIndex < Params.Num(); ++ParamIndex)
	// {
	// 	PyObject* PyValue = Params[ParamIndex];
	// 	if (PyValue)
	// 	{
	// 		const UPyGenUtil::FGeneratedWrappedMethodParameter& InitParam = StructMetaData->InitParams[ParamIndex];
	// 		if (!UPyConversion::NativizeProperty_InContainer(PyValue, InitParam.ParamProp, InSelf->StructInstance, 0))
	// 		{
	// 			UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert type '%s' to property '%s' (%s) for attribute '%s' on '%s'"), *UPyUtil::GetFriendlyTypename(PyValue), *InitParam.ParamProp->GetName(), *InitParam.ParamProp->GetClass()->GetName(), UTF8_TO_TCHAR(InitParam.ParamName.GetData()), *InSelf->ScriptStruct->GetName()));
	// 			return -1;
	// 		}
	// 	}
	// }
	//
	// return 0;
	return -1;
}

PyObject* FUPyWrapperStruct::BreakStruct(FUPyWrapperStruct* InSelf)
{
	return nullptr;
	// if (!ValidateInternalState(InSelf))
	// {
	// 	return nullptr;
	// }
	//
	// FPyWrapperStructMetaData* StructMetaData = FPyWrapperStructMetaData::GetMetaData(InSelf);
	// if (!StructMetaData)
	// {
	// 	return nullptr;
	// }
	//
	// // If this struct has a custom break function, use that rather than use the generic version
	// if (StructMetaData->BreakFunc.Func)
	// {
	// 	return CallBreakFunction_Impl(InSelf, StructMetaData->BreakFunc);
	// }
	//
	// // Generic implementation just creates a tuple from each property
	// FUPyObjectPtr PyPropTuple = FUPyObjectPtr::StealReference(PyTuple_New(StructMetaData->InitParams.Num()));
	// for (int32 ParamIndex = 0; ParamIndex < StructMetaData->InitParams.Num(); ++ParamIndex)
	// {
	// 	const UPyGenUtil::FGeneratedWrappedMethodParameter& InitParam = StructMetaData->InitParams[ParamIndex];
	//
	// 	PyObject* PyValue = nullptr;
	// 	if (!UPyConversion::PythonizeProperty_InContainer(InitParam.ParamProp, InSelf->StructInstance, 0, PyValue))
	// 	{
	// 		UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert property '%s' (%s) for attribute '%s' on '%s'"), *InitParam.ParamProp->GetName(), *InitParam.ParamProp->GetClass()->GetName(), UTF8_TO_TCHAR(InitParam.ParamName.GetData()), *InSelf->ScriptStruct->GetName()));
	// 		return nullptr;
	// 	}
	// 	PyTuple_SetItem(PyPropTuple, ParamIndex, PyValue); // SetItem steals the reference
	// }
	//
	// return PyPropTuple.Release();
}

PyObject* FUPyWrapperStruct::GetPropertyValue(FUPyWrapperStruct* InSelf, const UPyGenUtil::FGeneratedWrappedProperty& InPropDef, const char* InPythonAttrName)
{
	if (!ValidateInternalState(InSelf))
	{
		return nullptr;
	}

	return UPyGenUtil::GetPropertyValue(InSelf->ScriptStruct, InSelf->StructInstance, InPropDef, InPythonAttrName, (PyObject*)InSelf, *UPyUtil::GetErrorContext(InSelf));
}

int FUPyWrapperStruct::SetPropertyValue(FUPyWrapperStruct* InSelf, PyObject* InValue, const UPyGenUtil::FGeneratedWrappedProperty& InPropDef, const char* InPythonAttrName, const EPropertyAccessChangeNotifyMode InNotifyMode, const uint64 InReadOnlyFlags)
{
	if (!ValidateInternalState(InSelf))
	{
		return -1;
	}

	// Structs are not a template by default (for standalone structs)
	bool OwnerIsTemplate = false;
	if (const UObject* OwnerObject = UPyUtil::GetOwnerObject((PyObject*)InSelf))
	{
		OwnerIsTemplate = PropertyAccessUtil::IsObjectTemplate(OwnerObject);
	}

	const TUniquePtr<FPropertyAccessChangeNotify> ChangeNotify = FUPyWrapperOwnerContext((PyObject*)InSelf, InPropDef.Prop).BuildChangeNotify(InNotifyMode);
	const TArray<void*> NoArchetypeInsts;
	return UPyGenUtil::SetPropertyValue(InSelf->ScriptStruct, InSelf->StructInstance, InValue, InPropDef, InPythonAttrName, ChangeNotify.Get(), InReadOnlyFlags, OwnerIsTemplate, *UPyUtil::GetErrorContext(InSelf), NoArchetypeInsts);
}

int FUPyWrapperStruct::CallMakeFunction_Impl(FUPyWrapperStruct* InSelf, PyObject* InArgs, PyObject* InKwds, const UPyGenUtil::FGeneratedWrappedFunction& InFuncDef)
{
	TArray<PyObject*> Params;
	if (!UPyGenUtil::ParseMethodParameters(InArgs, InKwds, InFuncDef.InputParams, "call", Params))
	{
		return -1;
	}

	if (ensureAlways(InFuncDef.Func))
	{
		UClass* Class = InFuncDef.Func->GetOwnerClass();
		UObject* Obj = Class->GetDefaultObject();

		UPY_UFUNCTION_STACK(FuncParams, InFuncDef.Func);
		UPyGenUtil::ApplyParamDefaults(FuncParams.GetMemory(), InFuncDef.InputParams);
		for (int32 ParamIndex = 0; ParamIndex < Params.Num(); ++ParamIndex)
		{
			const UPyGenUtil::FGeneratedWrappedMethodParameter& ParamDef = InFuncDef.InputParams[ParamIndex];

			PyObject* PyValue = Params[ParamIndex];
			if (PyValue)
			{
				if (!UPyConversion::NativizeProperty_InContainer(PyValue, ParamDef.ParamProp, FuncParams.GetMemory(), 0))
				{
					UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert parameter '%s' when calling function '%s.%s' on '%s'"), UTF8_TO_TCHAR(ParamDef.ParamName.GetData()), *Class->GetName(), *InFuncDef.Func->GetName(), *Obj->GetName()));
					return -1;
				}
			}
		}
		if (!UPyUtil::InvokeFunctionCall(Obj, InFuncDef.Func, FuncParams.GetMemory(), *UPyUtil::GetErrorContext(InSelf)))
		{
			return -1;
		}
		if (ensureAlways(InFuncDef.OutputParams.Num() == 1 && CastField<FStructProperty>(InFuncDef.OutputParams[0].ParamProp) && CastFieldChecked<const FStructProperty>(InFuncDef.OutputParams[0].ParamProp)->Struct->IsChildOf(InSelf->ScriptStruct)))
		{
			// Copy the result back onto ourself
			const UPyGenUtil::FGeneratedWrappedMethodParameter& ReturnParam = InFuncDef.OutputParams[0];
			const void* ReturnArgInstance = ReturnParam.ParamProp->ContainerPtrToValuePtr<void>(FuncParams.GetMemory());
			InSelf->ScriptStruct->CopyScriptStruct(InSelf->StructInstance, ReturnArgInstance);
		}
	}

	return 0;
}

PyObject* FUPyWrapperStruct::CallBreakFunction_Impl(FUPyWrapperStruct* InSelf, const UPyGenUtil::FGeneratedWrappedFunction& InFuncDef)
{
	if (ensureAlways(InFuncDef.Func))
	{
		UClass* Class = InFuncDef.Func->GetOwnerClass();
		UObject* Obj = Class->GetDefaultObject();

		UPY_UFUNCTION_STACK(FuncParams, InFuncDef.Func);
		if (ensureAlways(InFuncDef.InputParams.Num() == 1 && CastField<FStructProperty>(InFuncDef.InputParams[0].ParamProp) && InSelf->ScriptStruct->IsChildOf(CastFieldChecked<const FStructProperty>(InFuncDef.InputParams[0].ParamProp)->Struct)))
		{
			// Copy us as the 'self' argument
			const UPyGenUtil::FGeneratedWrappedMethodParameter& SelfParam = InFuncDef.InputParams[0];
			void* SelfArgInstance = SelfParam.ParamProp->ContainerPtrToValuePtr<void>(FuncParams.GetMemory());
			CastFieldChecked<const FStructProperty>(SelfParam.ParamProp)->Struct->CopyScriptStruct(SelfArgInstance, InSelf->StructInstance);
		}
		if (!UPyUtil::InvokeFunctionCall(Obj, InFuncDef.Func, FuncParams.GetMemory(), *UPyUtil::GetErrorContext(InSelf)))
		{
			return nullptr;
		}
		FUPyObjectPtr PyPropTuple = FUPyObjectPtr::StealReference(PyTuple_New(InFuncDef.OutputParams.Num()));
		for (int32 ParamIndex = 0; ParamIndex < InFuncDef.OutputParams.Num(); ++ParamIndex)
		{
			const UPyGenUtil::FGeneratedWrappedMethodParameter& ParamDef = InFuncDef.OutputParams[ParamIndex];

			PyObject* PyValue = nullptr;
			if (!UPyConversion::PythonizeProperty_InContainer(ParamDef.ParamProp, FuncParams.GetMemory(), 0, PyValue, EUPyConversionMethod::Steal))
			{
				UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert return property '%s' when calling function '%s.%s' on '%s'"), UTF8_TO_TCHAR(ParamDef.ParamName.GetData()), *Class->GetName(), *InFuncDef.Func->GetName(), *Obj->GetName()));
				return nullptr;
			}
			PyTuple_SetItem(PyPropTuple, ParamIndex, PyValue); // SetItem steals the reference
		}
		return PyPropTuple.Release();
	}

	Py_RETURN_NONE;
}

PyObject* FUPyWrapperStruct::CallDynamicFunction_Impl(FUPyWrapperStruct* InSelf, PyObject* InArgs, PyObject* InKwds, const UPyGenUtil::FGeneratedWrappedFunction& InFuncDef, const UPyGenUtil::FGeneratedWrappedMethodParameter& InSelfParam, const UPyGenUtil::FGeneratedWrappedMethodParameter& InSelfReturn, const char* InPythonFuncName)
{
	TArray<PyObject*> Params;
	if ((InArgs || InKwds) && !UPyGenUtil::ParseMethodParameters(InArgs, InKwds, InFuncDef.InputParams, InPythonFuncName, Params))
	{
		return nullptr;
	}

	if (ensureAlways(InFuncDef.Func))
	{
		UClass* Class = InFuncDef.Func->GetOwnerClass();
		UObject* Obj = Class->GetDefaultObject();

		// Deprecated functions emit a warning
		if (InFuncDef.DeprecationMessage.IsSet())
		{
			if (UPyUtil::SetPythonWarning(PyExc_DeprecationWarning, InSelf, *FString::Printf(TEXT("Function '%s' on '%s' is deprecated: %s"), UTF8_TO_TCHAR(InPythonFuncName), *Class->GetName(), *InFuncDef.DeprecationMessage.GetValue())) == -1)
			{
				// -1 from SetPythonWarning means the warning should be an exception
				return nullptr;
			}
		}

		UPY_UFUNCTION_STACK(FuncParams, InFuncDef.Func);
		UPyGenUtil::ApplyParamDefaults(FuncParams.GetMemory(), InFuncDef.InputParams);
		if (ensureAlways(CastField<FStructProperty>(InSelfParam.ParamProp) && InSelf->ScriptStruct->IsChildOf(CastFieldChecked<const FStructProperty>(InSelfParam.ParamProp)->Struct)))
		{
			void* SelfArgInstance = InSelfParam.ParamProp->ContainerPtrToValuePtr<void>(FuncParams.GetMemory());
			CastFieldChecked<const FStructProperty>(InSelfParam.ParamProp)->Struct->CopyScriptStruct(SelfArgInstance, InSelf->StructInstance);
		}
		for (int32 ParamIndex = 0; ParamIndex < Params.Num(); ++ParamIndex)
		{
			const UPyGenUtil::FGeneratedWrappedMethodParameter& ParamDef = InFuncDef.InputParams[ParamIndex];

			PyObject* PyValue = Params[ParamIndex];
			if (PyValue)
			{
				if (!UPyConversion::NativizeProperty_InContainer(PyValue, ParamDef.ParamProp, FuncParams.GetMemory(), 0))
				{
					UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert parameter '%s' when calling function '%s.%s' on '%s'"), UTF8_TO_TCHAR(ParamDef.ParamName.GetData()), *Class->GetName(), *InFuncDef.Func->GetName(), *Obj->GetName()));
					return nullptr;
				}
			}
		}
		const FString ErrorCtxt = UPyUtil::GetErrorContext(InSelf);
		if (!UPyUtil::InvokeFunctionCall(Obj, InFuncDef.Func, FuncParams.GetMemory(), *ErrorCtxt))
		{
			return nullptr;
		}
		if (InSelfReturn.ParamProp && ensureAlways(CastField<FStructProperty>(InSelfReturn.ParamProp) && CastFieldChecked<const FStructProperty>(InSelfReturn.ParamProp)->Struct->IsChildOf(InSelf->ScriptStruct)))
		{
			// Copy the 'self' return value back onto ourself
			const void* SelfReturnInstance = InSelfReturn.ParamProp->ContainerPtrToValuePtr<void>(FuncParams.GetMemory());
			InSelf->ScriptStruct->CopyScriptStruct(InSelf->StructInstance, SelfReturnInstance);
		}
		return UPyGenUtil::PackReturnValues(FuncParams.GetMemory(), InFuncDef.OutputParams, *ErrorCtxt, *FString::Printf(TEXT("function '%s.%s' on '%s'"), *Class->GetName(), *InFuncDef.Func->GetName(), *Obj->GetName()));
	}

	Py_RETURN_NONE;
}

PyObject* FUPyWrapperStruct::CallDynamicMethodNoArgs_Impl(FUPyWrapperStruct* InSelf, void* InClosure)
{
	if (!ValidateInternalState(InSelf))
	{
		return nullptr;
	}

	const UPyGenUtil::FGeneratedWrappedDynamicMethod* Closure = (UPyGenUtil::FGeneratedWrappedDynamicMethod*)InClosure;
	return CallDynamicFunction_Impl(InSelf, nullptr, nullptr, Closure->MethodFunc, Closure->SelfParam, Closure->SelfReturn, Closure->MethodName.GetData());
}

PyObject* FUPyWrapperStruct::CallDynamicMethodWithArgs_Impl(FUPyWrapperStruct* InSelf, PyObject* InArgs, PyObject* InKwds, void* InClosure)
{
	if (!ValidateInternalState(InSelf))
	{
		return nullptr;
	}

	const UPyGenUtil::FGeneratedWrappedDynamicMethod* Closure = (UPyGenUtil::FGeneratedWrappedDynamicMethod*)InClosure;
	return CallDynamicFunction_Impl(InSelf, InArgs, InKwds, Closure->MethodFunc, Closure->SelfParam, Closure->SelfReturn, Closure->MethodName.GetData());
}

PyObject* FUPyWrapperStruct::CallOperatorFunction_Impl(FUPyWrapperStruct* InSelf, PyObject* InRHS, const UPyGenUtil::FGeneratedWrappedOperatorFunction& InOpFunc, const TOptional<EUPyConversionResultState> InRequiredConversionResult, FUPyConversionResult* OutRHSConversionResult)
{
	SetOptionalUPyConversionResult(FUPyConversionResult::Failure(), OutRHSConversionResult);

	if (ensureAlways(InOpFunc.Func))
	{
		UClass* Class = InOpFunc.Func->GetOwnerClass();
		UObject* Obj = Class->GetDefaultObject();

		// Build the input arguments (failures here aren't fatal as we may have multiple functions to evaluate on the stack, only one of which may accept the RHS parameter)
		UPY_UFUNCTION_STACK(FuncParams, InOpFunc.Func);
		UPyGenUtil::ApplyParamDefaults(FuncParams.GetMemory(), InOpFunc.AdditionalParams);
		if (InOpFunc.OtherParam.ParamProp)
		{
			TArray<void*> NoArchetypeInsts;
			const FUPyConversionResult RHSResult = UPyConversion::NativizeProperty_InContainer(InRHS, InOpFunc.OtherParam.ParamProp, FuncParams.GetMemory(), 0, NoArchetypeInsts, nullptr, UPyConversion::ESetErrorState::No);
			SetOptionalUPyConversionResult(RHSResult, OutRHSConversionResult);

			if (!RHSResult)
			{
				return nullptr;
			}

			if (InRequiredConversionResult.IsSet() && RHSResult.GetState() != InRequiredConversionResult.GetValue())
			{
				return nullptr;
			}
		}
		if (ensureAlways(CastField<FStructProperty>(InOpFunc.SelfParam.ParamProp) && InSelf->ScriptStruct->IsChildOf(CastFieldChecked<const FStructProperty>(InOpFunc.SelfParam.ParamProp)->Struct)))
		{
			void* StructArgInstance = InOpFunc.SelfParam.ParamProp->ContainerPtrToValuePtr<void>(FuncParams.GetMemory());
			CastFieldChecked<const FStructProperty>(InOpFunc.SelfParam.ParamProp)->Struct->CopyScriptStruct(StructArgInstance, InSelf->StructInstance);
		}
		if (!UPyUtil::InvokeFunctionCall(Obj, InOpFunc.Func, FuncParams.GetMemory(), *UPyUtil::GetErrorContext(InSelf)))
		{
			return nullptr;
		}

		PyObject* ReturnPyObj = nullptr;
		if (InOpFunc.SelfReturn.ParamProp)
		{
			if (ensureAlways(CastField<FStructProperty>(InOpFunc.SelfReturn.ParamProp) && CastFieldChecked<const FStructProperty>(InOpFunc.SelfReturn.ParamProp)->Struct->IsChildOf(InSelf->ScriptStruct)))
			{
				// Copy the 'self' return value back onto ourself
				const void* SelfReturnInstance = InOpFunc.SelfReturn.ParamProp->ContainerPtrToValuePtr<void>(FuncParams.GetMemory());
				InSelf->ScriptStruct->CopyScriptStruct(InSelf->StructInstance, SelfReturnInstance);
			}

			Py_INCREF(InSelf);
			ReturnPyObj = (PyObject*)InSelf;
		}
		else if (InOpFunc.ReturnParam.ParamProp)
		{
			if (!UPyConversion::PythonizeProperty_InContainer(InOpFunc.ReturnParam.ParamProp, FuncParams.GetMemory(), 0, ReturnPyObj, EUPyConversionMethod::Steal))
			{
				UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert return property '%s' (%s) when calling function '%s' on '%s'"), *InOpFunc.ReturnParam.ParamProp->GetName(), *InOpFunc.ReturnParam.ParamProp->GetClass()->GetName(), *InOpFunc.Func->GetName(), *Obj->GetName()));
				return nullptr;
			}
		}
		else
		{
			Py_INCREF(Py_None);
			ReturnPyObj = Py_None;
		}

		return ReturnPyObj;
	}

	return nullptr;
}

PyObject* FUPyWrapperStruct::CallOperator_Impl(FUPyWrapperStruct* InSelf, PyObject* InRHS, const UPyGenUtil::EGeneratedWrappedOperatorType InOpType)
{
	// if (!ValidateInternalState(InSelf))
	// {
	// 	return nullptr;
	// }
	//
	// // Walk up the inheritance chain to find the correct op functions to use
	// // We take the first one with any functions set, so that overrides on a derived type hide those from the base type
	// TArray<UPyGenUtil::FGeneratedWrappedOperatorFunction>* OpFuncsPtr = nullptr;
	// {
	// 	PyTypeObject* PyType = Py_TYPE(InSelf);
	// 	do
	// 	{
	// 		PyTypeObject* NextPyType = nullptr;
	// 		if (FPyWrapperStructMetaData* PyWrapperMetaData = FPyWrapperStructMetaData::GetMetaData(PyType))
	// 		{
	// 			if (PyWrapperMetaData->OpStacks[(int32)InOpType].Funcs.Num() > 0)
	// 			{
	// 				OpFuncsPtr = &PyWrapperMetaData->OpStacks[(int32)InOpType].Funcs;
	// 				break;
	// 			}
	//
	// 			if (const UScriptStruct* SuperStruct = PyWrapperMetaData->Struct ? Cast<UScriptStruct>(PyWrapperMetaData->Struct->GetSuperStruct()) : nullptr)
	// 			{
	// 				NextPyType = FUPyWrapperTypeRegistry::Get().GetWrappedStructType(SuperStruct);
	// 			}
	// 		}
	// 		PyType = NextPyType;
	// 	}
	// 	while (PyType);
	// }
	//
	// if (OpFuncsPtr)
	// {
	// 	// We process the operator stack in two passes:
	// 	//	- The first pass looks for a signature that exactly matches the given argument
	// 	//	- The second pass allows type coercion to occur when calling the signature
	// 	// We use the first pass to find a function that may be called for the second pass
	// 	const UPyGenUtil::FGeneratedWrappedOperatorFunction* CoercedOpFunc = nullptr;
	// 	for (const UPyGenUtil::FGeneratedWrappedOperatorFunction& OpFunc : *OpFuncsPtr)
	// 	{
	// 		FUPyConversionResult RHSConversionResult = FUPyConversionResult::Failure();
	// 		PyObject* PyResult = CallOperatorFunction_Impl(InSelf, InRHS, OpFunc, EUPyConversionResultState::Success, &RHSConversionResult);
	// 		if (PyResult)
	// 		{
	// 			return PyResult;
	// 		}
	// 		else if (!CoercedOpFunc && RHSConversionResult.GetState() == EUPyConversionResultState::SuccessWithCoercion)
	// 		{
	// 			CoercedOpFunc = &OpFunc;
	// 		}
	// 	}
	// 	if (CoercedOpFunc)
	// 	{
	// 		PyObject* PyResult = CallOperatorFunction_Impl(InSelf, InRHS, *CoercedOpFunc);
	// 		if (PyResult)
	// 		{
	// 			return PyResult;
	// 		}
	// 	}
	// }

	Py_INCREF(Py_NotImplemented);
	return Py_NotImplemented;
}

PyObject* FUPyWrapperStruct::Getter_Impl(FUPyWrapperStruct* InSelf, void* InClosure)
{
	const UPyGenUtil::FGeneratedWrappedGetSet* Closure = (UPyGenUtil::FGeneratedWrappedGetSet*)InClosure;
	return GetPropertyValue(InSelf, Closure->Prop, Closure->GetSetName.GetData());
}

int FUPyWrapperStruct::Setter_Impl(FUPyWrapperStruct* InSelf, PyObject* InValue, void* InClosure)
{
	const UPyGenUtil::FGeneratedWrappedGetSet* Closure = (UPyGenUtil::FGeneratedWrappedGetSet*)InClosure;
	return SetPropertyValue(InSelf, InValue, Closure->Prop, Closure->GetSetName.GetData());
}

// ==================== Wrapper Struct Type BEGIN ====================

struct FFuncs_WrapperStruct
{
	static PyObject* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
	{
		return (PyObject*)FUPyWrapperStruct::New(InType);
	}

	static void Dealloc(FUPyWrapperStruct* InSelf)
	{
		FUPyWrapperStruct::Free(InSelf);
	}

	static int Init(FUPyWrapperStruct* InSelf, PyObject* InArgs, PyObject* InKwds)
	{
		// PyTypeObject* InType = Py_TYPE(InSelf);
		// PyErr_Format(PyExc_RuntimeError, "Direct StructBase initialize of %s failed", InType->tp_name);
		// return -1;
		return FUPyWrapperStruct::Init(InSelf);
	}

	static PyObject* Str(FUPyWrapperStruct* InSelf)
	{
		if (!FUPyWrapperStruct::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		const FString ExportedStruct = UPyUtil::GetFriendlyStructValue(InSelf->ScriptStruct, InSelf->StructInstance, PPF_IncludeTransient);
		return PyUnicode_FromFormat("<Struct '%s' (%p) %s>", TCHAR_TO_UTF8(*InSelf->ScriptStruct->GetName()), InSelf->StructInstance, TCHAR_TO_UTF8(*ExportedStruct));
	}

	static PyObject* RichCmp(FUPyWrapperStruct* InSelf, PyObject* InOther, int InOp)
	{
		if (!FUPyWrapperStruct::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		auto PythonCmpOpToWrapperOp = [InOp]() -> UPyGenUtil::EGeneratedWrappedOperatorType
		{
			switch (InOp)
			{
			case Py_EQ:
				return UPyGenUtil::EGeneratedWrappedOperatorType::Equal;
			case Py_NE:
				return UPyGenUtil::EGeneratedWrappedOperatorType::NotEqual;
			case Py_LT:
				return UPyGenUtil::EGeneratedWrappedOperatorType::Less;
			case Py_LE:
				return UPyGenUtil::EGeneratedWrappedOperatorType::LessEqual;
			case Py_GT:
				return UPyGenUtil::EGeneratedWrappedOperatorType::Greater;
			case Py_GE:
				return UPyGenUtil::EGeneratedWrappedOperatorType::GreaterEqual;
			default:
				checkf(false, TEXT("Unknown Python comparison type!"));
				break;
			}
			return UPyGenUtil::EGeneratedWrappedOperatorType::Equal;
		};

		return FUPyWrapperStruct::CallOperator_Impl(InSelf, InOther, PythonCmpOpToWrapperOp());
	}

	static UPyUtil::FPyHashType Hash(FUPyWrapperStruct* InSelf)
	{
		if (!FUPyWrapperStruct::ValidateInternalState(InSelf))
		{
			return -1;
		}

		// UserDefinedStruct overrides GetStructTypeHash to work without valid CppStructOps
		if (InSelf->ScriptStruct->IsA<UUserDefinedStruct>() || (InSelf->ScriptStruct->GetCppStructOps() && InSelf->ScriptStruct->GetCppStructOps()->HasGetTypeHash()))
		{
			const UPyUtil::FPyHashType PyHash = (UPyUtil::FPyHashType)InSelf->ScriptStruct->GetStructTypeHash(InSelf->StructInstance);
			return PyHash != -1 ? PyHash : 0;
		}

		UPyUtil::SetPythonError(PyExc_Exception, InSelf, TEXT("Type cannot be hashed"));
		return -1;
	}
};

#define DEFINE_INQUIRY_OPERATOR_FUNC(OP, NOT_IMPLEMENTED_VALUE)																	\
	static int OP(FUPyWrapperStruct* InLHS)																						\
	{																															\
		PyObject* PyResult = FUPyWrapperStruct::CallOperator_Impl(InLHS, nullptr, UPyGenUtil::EGeneratedWrappedOperatorType::OP);	\
		const int Result = PyObjectResultToInt(PyResult, (NOT_IMPLEMENTED_VALUE));												\
		Py_XDECREF(PyResult);																									\
		return Result;																											\
	}
#define DEFINE_UNARY_OPERATOR_FUNC(OP)																							\
	static PyObject* OP(FUPyWrapperStruct* InLHS)																				\
	{																															\
		return FUPyWrapperStruct::CallOperator_Impl(InLHS, nullptr, UPyGenUtil::EGeneratedWrappedOperatorType::OP);				\
	}
#define DEFINE_BINARY_OPERATOR_FUNC(OP)																							\
	static PyObject* OP(FUPyWrapperStruct* InLHS, PyObject* InRHS)																\
	{																															\
		return FUPyWrapperStruct::CallOperator_Impl(InLHS, InRHS, UPyGenUtil::EGeneratedWrappedOperatorType::OP);					\
	}

struct FNumberFuncs_WrapperStruct
{
	static int PyObjectResultToInt(PyObject* PyResult, int NotImplementedValue)
	{
		int Result = -1;
		if (PyResult)
		{
			if (PyResult == Py_NotImplemented)
			{
				Result = NotImplementedValue;
			}
			else if (PyBool_Check(PyResult))
			{
				Result = (PyResult == Py_True) ? 1 : 0;
			}
			else
			{
				UPyConversion::Nativize(PyResult, Result);
			}
		}
		return Result;
	}

	DEFINE_INQUIRY_OPERATOR_FUNC(Bool, 1)
	DEFINE_BINARY_OPERATOR_FUNC(Add)
	DEFINE_BINARY_OPERATOR_FUNC(InlineAdd)
	DEFINE_BINARY_OPERATOR_FUNC(Subtract)
	DEFINE_BINARY_OPERATOR_FUNC(InlineSubtract)
	DEFINE_BINARY_OPERATOR_FUNC(Multiply)
	DEFINE_BINARY_OPERATOR_FUNC(InlineMultiply)
	DEFINE_BINARY_OPERATOR_FUNC(Divide)
	DEFINE_BINARY_OPERATOR_FUNC(InlineDivide)
	DEFINE_BINARY_OPERATOR_FUNC(Modulus)
	DEFINE_BINARY_OPERATOR_FUNC(InlineModulus)
	DEFINE_BINARY_OPERATOR_FUNC(And)
	DEFINE_BINARY_OPERATOR_FUNC(InlineAnd)
	DEFINE_BINARY_OPERATOR_FUNC(Or)
	DEFINE_BINARY_OPERATOR_FUNC(InlineOr)
	DEFINE_BINARY_OPERATOR_FUNC(Xor)
	DEFINE_BINARY_OPERATOR_FUNC(InlineXor)
	DEFINE_BINARY_OPERATOR_FUNC(RightShift)
	DEFINE_BINARY_OPERATOR_FUNC(InlineRightShift)
	DEFINE_BINARY_OPERATOR_FUNC(LeftShift)
	DEFINE_BINARY_OPERATOR_FUNC(InlineLeftShift)
	DEFINE_UNARY_OPERATOR_FUNC(Negated)
};
#undef DEFINE_INQUIRY_OPERATOR_FUNC
#undef DEFINE_UNARY_OPERATOR_FUNC
#undef DEFINE_BINARY_OPERATOR_FUNC

void InitializeUPyWrapperStruct(UPyGenUtil::FNativePythonModule& ModuleInfo)
{
	PyTypeObject* PyType = &UPyWrapperStructType;

	PyType->tp_base = &UPyWrapperBaseType;
	PyType->tp_new = (newfunc)&FFuncs_WrapperStruct::New;
	PyType->tp_dealloc = (destructor)&FFuncs_WrapperStruct::Dealloc;
	PyType->tp_init = (initproc)&FFuncs_WrapperStruct::Init;
	PyType->tp_str = (reprfunc)&FFuncs_WrapperStruct::Str;
	PyType->tp_repr = (reprfunc)&FFuncs_WrapperStruct::Str;
	PyType->tp_richcompare = (richcmpfunc)&FFuncs_WrapperStruct::RichCmp;
	PyType->tp_hash = (hashfunc)&FFuncs_WrapperStruct::Hash;
	
	PyType->tp_methods = StructPyMethodDefs;

	PyType->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
	PyType->tp_doc = "Type for all Unreal exposed struct instances";

	static PyNumberMethods PyNumber;
	PyNumber.nb_bool = (inquiry)&FNumberFuncs_WrapperStruct::Bool;
	PyNumber.nb_add = (binaryfunc)&FNumberFuncs_WrapperStruct::Add;
	PyNumber.nb_inplace_add = (binaryfunc)&FNumberFuncs_WrapperStruct::InlineAdd;
	PyNumber.nb_subtract = (binaryfunc)&FNumberFuncs_WrapperStruct::Subtract;
	PyNumber.nb_inplace_subtract = (binaryfunc)&FNumberFuncs_WrapperStruct::InlineSubtract;
	PyNumber.nb_multiply = (binaryfunc)&FNumberFuncs_WrapperStruct::Multiply;
	PyNumber.nb_inplace_multiply = (binaryfunc)&FNumberFuncs_WrapperStruct::InlineMultiply;
	PyNumber.nb_true_divide = (binaryfunc)&FNumberFuncs_WrapperStruct::Divide;
	PyNumber.nb_inplace_true_divide = (binaryfunc)&FNumberFuncs_WrapperStruct::InlineDivide;
	PyNumber.nb_remainder = (binaryfunc)&FNumberFuncs_WrapperStruct::Modulus;
	PyNumber.nb_inplace_remainder = (binaryfunc)&FNumberFuncs_WrapperStruct::InlineModulus;
	PyNumber.nb_and = (binaryfunc)&FNumberFuncs_WrapperStruct::And;
	PyNumber.nb_inplace_and = (binaryfunc)&FNumberFuncs_WrapperStruct::InlineAnd;
	PyNumber.nb_or = (binaryfunc)&FNumberFuncs_WrapperStruct::Or;
	PyNumber.nb_inplace_or = (binaryfunc)&FNumberFuncs_WrapperStruct::InlineOr;
	PyNumber.nb_xor = (binaryfunc)&FNumberFuncs_WrapperStruct::Xor;
	PyNumber.nb_inplace_xor = (binaryfunc)&FNumberFuncs_WrapperStruct::InlineXor;
	PyNumber.nb_rshift = (binaryfunc)&FNumberFuncs_WrapperStruct::RightShift;
	PyNumber.nb_inplace_rshift = (binaryfunc)&FNumberFuncs_WrapperStruct::InlineRightShift;
	PyNumber.nb_lshift = (binaryfunc)&FNumberFuncs_WrapperStruct::LeftShift;
	PyNumber.nb_inplace_lshift = (binaryfunc)&FNumberFuncs_WrapperStruct::InlineLeftShift;
	PyNumber.nb_negative = (unaryfunc)&FNumberFuncs_WrapperStruct::Negated;

	PyType->tp_as_number = &PyNumber;
	
	if (PyType_Ready(PyType) == 0)
	{
		ModuleInfo.AddType(PyType, true);
	}
}
// ==================== Wrapper Struct Type END ====================
