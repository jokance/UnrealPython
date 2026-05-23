
#include "Wrapper/UPyWrapperSet.h"
#include "Wrapper/UPyWrapperTypeRegistry.h"
#include "Core/UPyConversion.h"
// #include "PyReferenceCollector.h"
#include "UObject/UnrealType.h"
#include "UObject/Package.h"
#include "UObject/PropertyPortFlags.h"
#include "Wrapper/UPyWrapperTypeFactory.h"

extern PyMethodDef SetPyMethodDefs[];

PyTypeObject UPyWrapperSetType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"Set", /* tp_name */
	sizeof(FUPyWrapperSet), /* tp_basicsize */
};

/** Iterator used with sets */
struct FUPyWrapperSetIterator
{
	/** Common Python Object */
	PyObject_HEAD

	/** Instance being iterated over */
	FUPyWrapperSet* IterInstance;

	/** Current iteration index */
	int32 IterIndex;

	/** New this iterator instance (called via tp_new for Python, or directly in C++) */
	static FUPyWrapperSetIterator* New(PyTypeObject* InType)
	{
		FUPyWrapperSetIterator* Self = (FUPyWrapperSetIterator*)InType->tp_alloc(InType, 0);
		if (Self)
		{
			Self->IterInstance = nullptr;
			Self->IterIndex = 0;
		}
		return Self;
	}

	/** Free this iterator instance (called via tp_dealloc for Python) */
	static void Free(FUPyWrapperSetIterator* InSelf)
	{
		Deinit(InSelf);
		Py_TYPE(InSelf)->tp_free((PyObject*)InSelf);
	}

	/** Initialize this iterator instance (called via tp_init for Python, or directly in C++) */
	static int Init(FUPyWrapperSetIterator* InSelf, FUPyWrapperSet* InInstance)
	{
		Deinit(InSelf);

		if (!FUPyWrapperSet::ValidateInternalState(InInstance))
		{
			return -1;
		}

		Py_INCREF(InInstance);
		InSelf->IterInstance = InInstance;
		InSelf->IterIndex = GetElementIndex(InSelf, 0);

		return 0;
	}

	/** Deinitialize this iterator instance (called via Init and Free to restore the instance to its New state) */
	static void Deinit(FUPyWrapperSetIterator* InSelf)
	{
		Py_XDECREF(InSelf->IterInstance);
		InSelf->IterInstance = nullptr;
		InSelf->IterIndex = 0;
	}

	/** Called to validate the internal state of this iterator instance prior to operating on it (should be called by all functions that expect to operate on an initialized type; will set an error state on failure) */
	static bool ValidateInternalState(FUPyWrapperSetIterator* InSelf)
	{
		if (!InSelf->IterInstance)
		{
			UPyUtil::SetPythonError(PyExc_Exception, Py_TYPE(InSelf), TEXT("Internal Error - IterInstance is null!"));
			return false;
		}

		return true;
	}

	/** Given a sparse index, get the first element index from this point in the set (including the given index) */
	static int32 GetElementIndex(FUPyWrapperSetIterator* InSelf, int32 InSparseIndex)
	{
		FScriptSetHelper ScriptSetHelper(InSelf->IterInstance->SetProp, InSelf->IterInstance->SetInstance);
		const int32 SparseCount = ScriptSetHelper.GetMaxIndex();

		int32 ElementIndex = InSparseIndex;
		for (; ElementIndex < SparseCount; ++ElementIndex)
		{
			if (ScriptSetHelper.IsValidIndex(ElementIndex))
			{
				break;
			}
		}

		return ElementIndex;
	}

	/** Get the iterator */
	static FUPyWrapperSetIterator* GetIter(FUPyWrapperSetIterator* InSelf)
	{
		Py_INCREF(InSelf);
		return InSelf;
	}

	/** Return the current value (if any) and advance the iterator */
	static PyObject* IterNext(FUPyWrapperSetIterator* InSelf)
	{
		if (!ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		FScriptSetHelper ScriptSetHelper(InSelf->IterInstance->SetProp, InSelf->IterInstance->SetInstance);
		const int32 SparseCount = ScriptSetHelper.GetMaxIndex();

		if (InSelf->IterIndex < SparseCount)
		{
			const int32 ElementIndex = InSelf->IterIndex;
			InSelf->IterIndex = GetElementIndex(InSelf, InSelf->IterIndex + 1);

			if (!ScriptSetHelper.IsValidIndex(ElementIndex))
			{
				UPyUtil::SetPythonError(PyExc_IndexError, InSelf, TEXT("Iterator was on an invalid element index! Was the set changed while iterating?"));
				return nullptr;
			}

			PyObject* PyItemObj = nullptr;
			if (!UPyConversion::PythonizeProperty(ScriptSetHelper.GetElementProperty(), ScriptSetHelper.GetElementPtr(ElementIndex), PyItemObj))
			{
				UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert element property '%s' (%s)"), *ScriptSetHelper.GetElementProperty()->GetName(), *ScriptSetHelper.GetElementProperty()->GetClass()->GetName()));
				return nullptr;
			}
			return PyItemObj;
		}

		PyErr_SetObject(PyExc_StopIteration, Py_None);
		return nullptr;
	}
};

/** Python type for FUPyWrapperSetIterator */
PyTypeObject UPyWrapperSetIteratorType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"SetIterator", /* tp_name */
	sizeof(FUPyWrapperSetIterator), /* tp_basicsize */
};

FUPyWrapperSet* FUPyWrapperSet::New(PyTypeObject* InType)
{
	FUPyWrapperSet* Self = (FUPyWrapperSet*)FUPyWrapperBase::New(InType);
	if (Self)
	{
		new(&Self->OwnerContext) FUPyWrapperOwnerContext();
		new(&Self->SetProp) UPyUtil::FConstSetPropOnScope();
		Self->SetInstance = nullptr;
	}
	return Self;
}

void FUPyWrapperSet::Free(FUPyWrapperSet* InSelf)
{
	Deinit(InSelf);

	InSelf->OwnerContext.~FUPyWrapperOwnerContext();
	InSelf->SetProp.~TPropOnScope();
	FUPyWrapperBase::Free(InSelf);
}

int FUPyWrapperSet::Init(FUPyWrapperSet* InSelf, const UPyUtil::FPropertyDef& InElementDef)
{
	Deinit(InSelf);

	const int BaseInit = FUPyWrapperBase::Init(InSelf);
	if (BaseInit != 0)
	{
		return BaseInit;
	}

	UPyUtil::FPropOnScope SetElementProp = UPyUtil::FPropOnScope::OwnedReference(UPyUtil::CreateProperty(InElementDef, 1));
	if (!SetElementProp)
	{
		UPyUtil::SetPythonError(PyExc_Exception, InSelf, TEXT("Set element property was null during init"));
		return -1;
	}

	UPyUtil::FSetPropOnScope SetProp = UPyUtil::FSetPropOnScope::OwnedReference(new FSetProperty(FFieldVariant(), UPyUtil::DefaultPythonPropertyName, RF_NoFlags));
	SetProp->ElementProp = SetElementProp.Release();

	// Need to manually call Link to fix-up some data (such as the C++ property flags and the set layout) that are only set during Link
	{
		FArchive Ar;
		SetProp->LinkWithoutChangingOffset(Ar);
	}

	void* SetValue = FMemory::Malloc(SetProp->GetSize(), SetProp->GetMinAlignment());
	SetProp->InitializeValue(SetValue);

	InSelf->SetProp = MoveTemp(SetProp);
	InSelf->SetInstance = SetValue;

	if (!InSelf->SetProp->ElementProp->HasAnyPropertyFlags(CPF_HasGetValueTypeHash))
	{
		UPyUtil::SetPythonError(PyExc_Exception, Py_TYPE(InSelf), *FString::Printf(TEXT("Set element type must be hashable: %s (%s)"), *InSelf->SetProp->ElementProp->GetName(), *InSelf->SetProp->ElementProp->GetClass()->GetName()));
		return -1;
	}

	// FUPyWrapperSetFactory::Get().MapInstance(InSelf->SetInstance, InSelf);
	return 0;
}

int FUPyWrapperSet::Init(FUPyWrapperSet* InSelf, const FUPyWrapperOwnerContext& InOwnerContext, const FSetProperty* InProp, void* InValue, const EUPyConversionMethod InConversionMethod)
{
	InOwnerContext.AssertValidConversionMethod(InConversionMethod);

	Deinit(InSelf);

	const int BaseInit = FUPyWrapperBase::Init(InSelf);
	if (BaseInit != 0)
	{
		return BaseInit;
	}

	check(InProp && InValue);

	UPyUtil::FConstSetPropOnScope PropToUse;
	void* SetInstanceToUse = nullptr;
	switch (InConversionMethod)
	{
	case EUPyConversionMethod::Copy:
	case EUPyConversionMethod::Steal:
		{
			UPyUtil::FPropOnScope SetElementProp = UPyUtil::FPropOnScope::OwnedReference(UPyUtil::CreateProperty(InProp->ElementProp));
			if (!SetElementProp)
			{
				UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to create element property from '%s' (%s)"), *InProp->ElementProp->GetName(), *InProp->ElementProp->GetClass()->GetName()));
				return -1;
			}

			UPyUtil::FSetPropOnScope SetProp = UPyUtil::FSetPropOnScope::OwnedReference(new FSetProperty(FFieldVariant(), UPyUtil::DefaultPythonPropertyName, RF_NoFlags));
			SetProp->ElementProp = SetElementProp.Release();

			// Need to manually call Link to fix-up some data (such as the C++ property flags and the set layout) that are only set during Link
			{
				FArchive Ar;
				SetProp->LinkWithoutChangingOffset(Ar);
			}

			PropToUse = MoveTemp(SetProp);

			SetInstanceToUse = FMemory::Malloc(PropToUse->GetSize(), PropToUse->GetMinAlignment());
			PropToUse->InitializeValue(SetInstanceToUse);
			if (InConversionMethod == EUPyConversionMethod::Steal)
			{
				FScriptSetHelper SelfScriptSetHelper(PropToUse, SetInstanceToUse);
				SelfScriptSetHelper.MoveAssign(InValue);
			}
			else
			{
				PropToUse->CopyCompleteValue(SetInstanceToUse, InValue);
			}
		}
		break;

	case EUPyConversionMethod::Reference:
		{
			PropToUse = UPyUtil::FConstSetPropOnScope::ExternalReference(InProp);
			SetInstanceToUse = InValue;

			InOwnerContext.AddOwnedPyProp(InSelf);
		}
		break;

	default:
		checkf(false, TEXT("Unknown EUPyConversionMethod"));
		break;
	}

	check(PropToUse && SetInstanceToUse);

	InSelf->OwnerContext = InOwnerContext;
	InSelf->SetProp = MoveTemp(PropToUse);
	InSelf->SetInstance = SetInstanceToUse;

	if (!InSelf->SetProp->ElementProp->HasAnyPropertyFlags(CPF_HasGetValueTypeHash))
	{
		UPyUtil::SetPythonError(PyExc_Exception, Py_TYPE(InSelf), *FString::Printf(TEXT("Set element type must be hashable: %s (%s)"), *InSelf->SetProp->ElementProp->GetName(), *InSelf->SetProp->ElementProp->GetClass()->GetName()));
		return -1;
	}

	// todo(hzn): 是否有必要所有的结构体都加入map
	if (InOwnerContext.HasOwner())
	{
		FUPyWrapperSetFactory::Get().MapInstance(InSelf->SetInstance, InSelf);
	}
	return 0;
}

void FUPyWrapperSet::Deinit(FUPyWrapperSet* InSelf)
{
	if (InSelf->SetInstance)
	{
		FUPyWrapperSetFactory::Get().UnmapInstance(InSelf->SetInstance);
	}

	if (InSelf->OwnerContext.HasOwner())
	{
		InSelf->OwnerContext.Reset();
	}
	else if (InSelf->SetInstance)
	{
		if (InSelf->SetProp)
		{
			InSelf->SetProp->DestroyValue(InSelf->SetInstance);
		}
		FMemory::Free(InSelf->SetInstance);
	}
	InSelf->SetProp.Reset();
	InSelf->SetInstance = nullptr;
}

bool FUPyWrapperSet::ValidateInternalState(FUPyWrapperSet* InSelf)
{
	if (!InSelf->SetProp)
	{
		UPyUtil::SetPythonError(PyExc_Exception, Py_TYPE(InSelf), TEXT("Internal Error - SetProp is null!"));
		return false;
	}

	if (!InSelf->SetInstance)
	{
		UPyUtil::SetPythonError(PyExc_Exception, Py_TYPE(InSelf), TEXT("Internal Error - SetInstance is null!"));
		return false;
	}

	return true;
}

FUPyWrapperSet* FUPyWrapperSet::CastPyObject(PyObject* InPyObject, FUPyConversionResult* OutCastResult)
{
	SetOptionalUPyConversionResult(FUPyConversionResult::Failure(), OutCastResult);

	if (PyObject_IsInstance(InPyObject, (PyObject*)&UPyWrapperSetType) == 1)
	{
		SetOptionalUPyConversionResult(FUPyConversionResult::Success(), OutCastResult);

		Py_INCREF(InPyObject);
		return (FUPyWrapperSet*)InPyObject;
	}

	return nullptr;
}

FUPyWrapperSet* FUPyWrapperSet::CastPyObject(PyObject* InPyObject, PyTypeObject* InType, const UPyUtil::FPropertyDef& InElementDef, FUPyConversionResult* OutCastResult)
{
	SetOptionalUPyConversionResult(FUPyConversionResult::Failure(), OutCastResult);

	if (PyObject_IsInstance(InPyObject, (PyObject*)InType) == 1 && (InType == &UPyWrapperSetType || PyObject_IsInstance(InPyObject, (PyObject*)&UPyWrapperSetType) == 1))
	{
		FUPyWrapperSet* Self = (FUPyWrapperSet*)InPyObject;

		if (!ValidateInternalState(Self))
		{
			return nullptr;
		}

		const UPyUtil::FPropertyDef SelfElementPropDef = Self->SetProp->ElementProp;
		if (SelfElementPropDef == InElementDef)
		{
			SetOptionalUPyConversionResult(FUPyConversionResult::Success(), OutCastResult);

			Py_INCREF(Self);
			return Self;
		}

		FUPyWrapperSetPtr NewSet = FUPyWrapperSetPtr::StealReference(FUPyWrapperSet::New(InType));
		if (FUPyWrapperSet::Init(NewSet, InElementDef) != 0)
		{
			return nullptr;
		}

		// Attempt to convert the entries in the set to the native format of the new set
		{
			FScriptSetHelper SelfScriptSetHelper(Self->SetProp, Self->SetInstance);
			FScriptSetHelper NewScriptSetHelper(NewSet->SetProp, NewSet->SetInstance);

			FString ExportedEntry;
			for (FScriptSetHelper::FIterator It(SelfScriptSetHelper); It; ++It)
			{
				ExportedEntry.Reset();
				if (!SelfScriptSetHelper.GetElementProperty()->ExportText_Direct(ExportedEntry, SelfScriptSetHelper.GetElementPtr(It), SelfScriptSetHelper.GetElementPtr(It), nullptr, PPF_None))
				{
					UPyUtil::SetPythonError(PyExc_Exception, Self, *FString::Printf(TEXT("Failed to export text for element property '%s' (%s) at index %d"), *SelfScriptSetHelper.GetElementProperty()->GetName(), *SelfScriptSetHelper.GetElementProperty()->GetClass()->GetName(), It.GetLogicalIndex()));
					return nullptr;
				}

				const int32 NewElementIndex = NewScriptSetHelper.AddDefaultValue_Invalid_NeedsRehash();
				if (!NewScriptSetHelper.GetElementProperty()->ImportText_Direct(*ExportedEntry, NewScriptSetHelper.GetElementPtr(NewElementIndex), nullptr, PPF_None))
				{
					UPyUtil::SetPythonError(PyExc_Exception, Self, *FString::Printf(TEXT("Failed to import text '%s' element for property '%s' (%s) at index %d"), *ExportedEntry, *NewScriptSetHelper.GetElementProperty()->GetName(), *NewScriptSetHelper.GetElementProperty()->GetClass()->GetName(), NewElementIndex));
					return nullptr;
				}
			}

			NewScriptSetHelper.Rehash();
		}

		SetOptionalUPyConversionResult(FUPyConversionResult::SuccessWithCoercion(), OutCastResult);
		return NewSet.Release();
	}

	// Attempt conversion from any iterable sequence type that has a defined length
	if (!UPyUtil::IsMappingType(InPyObject))
	{
		const Py_ssize_t SequenceLen = PyObject_Length(InPyObject);
		if (SequenceLen != -1)
		{
			FUPyObjectPtr PyObjIter = FUPyObjectPtr::StealReference(PyObject_GetIter(InPyObject));
			if (PyObjIter)
			{
				FUPyWrapperSetPtr NewSet = FUPyWrapperSetPtr::StealReference(FUPyWrapperSet::New(InType));
				if (FUPyWrapperSet::Init(NewSet, InElementDef) != 0)
				{
					return nullptr;
				}

				FScriptSetHelper NewScriptSetHelper(NewSet->SetProp, NewSet->SetInstance);
				for (Py_ssize_t SequenceIndex = 0; SequenceIndex < SequenceLen; ++SequenceIndex)
				{
					FUPyObjectPtr SequenceItem = FUPyObjectPtr::StealReference(PyIter_Next(PyObjIter));
					if (!SequenceItem)
					{
						return nullptr;
					}

					const int32 NewElementIndex = NewScriptSetHelper.AddDefaultValue_Invalid_NeedsRehash();
					if (!UPyConversion::NativizeProperty(SequenceItem, NewScriptSetHelper.GetElementProperty(), NewScriptSetHelper.GetElementPtr(NewElementIndex)))
					{
						return nullptr;
					}
				}

				NewScriptSetHelper.Rehash();

				SetOptionalUPyConversionResult(FUPyConversionResult::SuccessWithCoercion(), OutCastResult);
				return NewSet.Release();
			}
		}
	}

	return nullptr;
}

Py_ssize_t FUPyWrapperSet::Len(FUPyWrapperSet* InSelf)
{
	if (!ValidateInternalState(InSelf))
	{
		return -1;
	}

	FScriptSetHelper SelfScriptSetHelper(InSelf->SetProp, InSelf->SetInstance);
	return SelfScriptSetHelper.Num();
}

int FUPyWrapperSet::Contains(FUPyWrapperSet* InSelf, PyObject* InValue)
{
	if (!ValidateInternalState(InSelf))
	{
		return -1;
	}

	UPyUtil::FSetElementOnScope ContainerEntryValue(InSelf->SetProp);
	if (!ContainerEntryValue.IsValid())
	{
		return -1;
	}

	if (!ContainerEntryValue.SetValue(InValue, *UPyUtil::GetErrorContext(InSelf)))
	{
		return -1;
	}

	FScriptSetHelper SelfScriptSetHelper(InSelf->SetProp, InSelf->SetInstance);
	return SelfScriptSetHelper.FindElementIndexFromHash(ContainerEntryValue.GetValue()) != INDEX_NONE;
}

int FUPyWrapperSet::Add(FUPyWrapperSet* InSelf, PyObject* InValue)
{
	if (!ValidateInternalState(InSelf))
	{
		return -1;
	}

	UPyUtil::FSetElementOnScope ContainerEntryValue(InSelf->SetProp);
	if (!ContainerEntryValue.IsValid())
	{
		return -1;
	}

	if (!ContainerEntryValue.SetValue(InValue, *UPyUtil::GetErrorContext(InSelf)))
	{
		return -1;
	}

	FScriptSetHelper SelfScriptSetHelper(InSelf->SetProp, InSelf->SetInstance);
	SelfScriptSetHelper.AddElement(ContainerEntryValue.GetValue());

	return 0;
}

int FUPyWrapperSet::Discard(FUPyWrapperSet* InSelf, PyObject* InValue)
{
	if (!ValidateInternalState(InSelf))
	{
		return -1;
	}

	UPyUtil::FSetElementOnScope ContainerEntryValue(InSelf->SetProp);
	if (!ContainerEntryValue.IsValid())
	{
		return -1;
	}

	if (!ContainerEntryValue.SetValue(InValue, *UPyUtil::GetErrorContext(InSelf)))
	{
		return -1;
	}

	FScriptSetHelper SelfScriptSetHelper(InSelf->SetProp, InSelf->SetInstance);
	const bool bWasRemoved = SelfScriptSetHelper.RemoveElement(ContainerEntryValue.GetValue());

	return bWasRemoved ? 1 : 0;
}

int FUPyWrapperSet::Remove(FUPyWrapperSet* InSelf, PyObject* InValue)
{
	const int DiscardResult = Discard(InSelf, InValue);
	if (DiscardResult == -1)
	{
		return -1;
	}

	if (DiscardResult == 0)
	{
		UPyUtil::SetPythonError(PyExc_KeyError, InSelf, TEXT("The given value was not found in the set"));
		return -1;
	}

	return 0;
}

PyObject* FUPyWrapperSet::Pop(FUPyWrapperSet* InSelf)
{
	if (!ValidateInternalState(InSelf))
	{
		return nullptr;
	}

	FScriptSetHelper SelfScriptSetHelper(InSelf->SetProp, InSelf->SetInstance);
	const FScriptSetHelper::FIterator It(SelfScriptSetHelper);
	if (It)
	{
		PyObject* PyReturnValue = nullptr;
		if (!UPyConversion::PythonizeProperty(SelfScriptSetHelper.GetElementProperty(), SelfScriptSetHelper.GetElementPtr(It), PyReturnValue))
		{
			UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert element property '%s' (%s) at index 0"), *SelfScriptSetHelper.GetElementProperty()->GetName(), *SelfScriptSetHelper.GetElementProperty()->GetClass()->GetName()));
			return nullptr;
		}

		SelfScriptSetHelper.RemoveAt(It.GetInternalIndex());

		return PyReturnValue;
	}

	UPyUtil::SetPythonError(PyExc_KeyError, InSelf, TEXT("Cannot pop from an empty set"));
	return nullptr;
}

int FUPyWrapperSet::Clear(FUPyWrapperSet* InSelf)
{
	if (!ValidateInternalState(InSelf))
	{
		return -1;
	}

	FScriptSetHelper SelfScriptSetHelper(InSelf->SetProp, InSelf->SetInstance);
	SelfScriptSetHelper.EmptyElements();

	return 0;
}

FUPyWrapperSet* FUPyWrapperSet::Difference(FUPyWrapperSet* InSelf, PyObject* InOthers)
{
	if (!ValidateInternalState(InSelf))
	{
		return nullptr;
	}

	FUPyWrapperSetPtr NewSet = FUPyWrapperSetPtr::StealReference(FUPyWrapperSetFactory::Get().CreateInstance(InSelf->SetInstance, InSelf->SetProp, FUPyWrapperOwnerContext(), EUPyConversionMethod::Copy));
	if (DifferenceUpdate(NewSet, InOthers) != 0)
	{
		return nullptr;
	}

	return NewSet.Release();
}

int FUPyWrapperSet::DifferenceUpdate(FUPyWrapperSet* InSelf, PyObject* InOthers)
{
	if (!ValidateInternalState(InSelf))
	{
		return -1;
	}

	const UPyUtil::FPropertyDef SelfElementDef = InSelf->SetProp->ElementProp;
	FScriptSetHelper SelfScriptSetHelper(InSelf->SetProp, InSelf->SetInstance);

	auto ProcessOtherObject = [&InSelf, &SelfScriptSetHelper, &SelfElementDef](PyObject* InOtherObj, const int32 InObjIndex)
	{
		FUPyWrapperSetPtr Other = FUPyWrapperSetPtr::StealReference(FUPyWrapperSet::CastPyObject(InOtherObj, &UPyWrapperSetType, SelfElementDef));
		if (!Other)
		{
			UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Cannot convert argument %d (%s) to '%s'"), InObjIndex, *UPyUtil::GetFriendlyTypename(InOtherObj), *UPyUtil::GetFriendlyTypename(InSelf)));
			return false;
		}

		const FScriptSetHelper OtherScriptSetHelper(Other->SetProp, Other->SetInstance);
		for (FScriptSetHelper::FIterator It(OtherScriptSetHelper); It; ++It)
		{
			const void* OtherElementPtr = OtherScriptSetHelper.GetElementPtr(It);
			SelfScriptSetHelper.RemoveElement(OtherElementPtr);
		}

		return true;
	};

	if (PyTuple_Check(InOthers))
	{
		// We need to go through each argument we were passed and remove any of its values that are present in NewSet
		const Py_ssize_t TupleLen = PyTuple_Size(InOthers);
		for (Py_ssize_t TupleIndex = 0; TupleIndex < TupleLen; ++TupleIndex)
		{
			PyObject* OtherObj = PyTuple_GetItem(InOthers, TupleIndex);
			if (!ProcessOtherObject(OtherObj, TupleIndex))
			{
				return -1;
			}
		}
	}
	else
	{
		// Assume a single value
		if (!ProcessOtherObject(InOthers, 0))
		{
			return -1;
		}
	}

	return 0;
}

FUPyWrapperSet* FUPyWrapperSet::Intersection(FUPyWrapperSet* InSelf, PyObject* InOthers)
{
	if (!ValidateInternalState(InSelf))
	{
		return nullptr;
	}

	FUPyWrapperSetPtr NewSet = FUPyWrapperSetPtr::StealReference(FUPyWrapperSetFactory::Get().CreateInstance(InSelf->SetInstance, InSelf->SetProp, FUPyWrapperOwnerContext(), EUPyConversionMethod::Copy));
	if (IntersectionUpdate(NewSet, InOthers) != 0)
	{
		return nullptr;
	}

	return NewSet.Release();
}

int FUPyWrapperSet::IntersectionUpdate(FUPyWrapperSet* InSelf, PyObject* InOthers)
{
	if (!ValidateInternalState(InSelf))
	{
		return -1;
	}

	const UPyUtil::FPropertyDef SelfElementDef = InSelf->SetProp->ElementProp;
	FScriptSetHelper SelfScriptSetHelper(InSelf->SetProp, InSelf->SetInstance);

	auto ProcessOtherObject = [&InSelf, &SelfScriptSetHelper, &SelfElementDef](PyObject* InOtherObj, const int32 InObjIndex)
	{
		FUPyWrapperSetPtr Other = FUPyWrapperSetPtr::StealReference(FUPyWrapperSet::CastPyObject(InOtherObj, &UPyWrapperSetType, SelfElementDef));
		if (!Other)
		{
			UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Cannot convert argument %d (%s) to '%s'"), InObjIndex, *UPyUtil::GetFriendlyTypename(InOtherObj), *UPyUtil::GetFriendlyTypename(InSelf)));
			return false;
		}

		FScriptSetHelper OtherScriptSetHelper(Other->SetProp, Other->SetInstance);

		const int32 SelfSparseCount = SelfScriptSetHelper.GetMaxIndex();
		for (int32 SelfSparseIndex = 0; SelfSparseIndex < SelfSparseCount; ++SelfSparseIndex)
		{
			if (!SelfScriptSetHelper.IsValidIndex(SelfSparseIndex))
			{
				continue;
			}

			const void* SelfElementPtr = SelfScriptSetHelper.GetElementPtr(SelfSparseIndex);
			if (OtherScriptSetHelper.FindElementIndexFromHash(SelfElementPtr) == INDEX_NONE)
			{
				// This is safe as long as the set isn't compacted as we're iterating it
				SelfScriptSetHelper.RemoveAt(SelfSparseIndex);
			}
		}

		return true;
	};

	if (PyTuple_Check(InOthers))
	{
		// We need to go through each argument we were passed and remove any values from NewSet that aren't present in the others
		const Py_ssize_t TupleLen = PyTuple_Size(InOthers);
		for (Py_ssize_t TupleIndex = 0; TupleIndex < TupleLen; ++TupleIndex)
		{
			PyObject* OtherObj = PyTuple_GetItem(InOthers, TupleIndex);
			if (!ProcessOtherObject(OtherObj, TupleIndex))
			{
				return -1;
			}
		}
	}
	else
	{
		// Assume a single value
		if (!ProcessOtherObject(InOthers, 0))
		{
			return -1;
		}
	}

	return 0;
}

FUPyWrapperSet* FUPyWrapperSet::SymmetricDifference(FUPyWrapperSet* InSelf, PyObject* InOther)
{
	if (!ValidateInternalState(InSelf))
	{
		return nullptr;
	}

	FUPyWrapperSetPtr NewSet = FUPyWrapperSetPtr::StealReference(FUPyWrapperSetFactory::Get().CreateInstance(InSelf->SetInstance, InSelf->SetProp, FUPyWrapperOwnerContext(), EUPyConversionMethod::Copy));
	if (SymmetricDifferenceUpdate(NewSet, InOther) != 0)
	{
		return nullptr;
	}

	return NewSet.Release();
}

int FUPyWrapperSet::SymmetricDifferenceUpdate(FUPyWrapperSet* InSelf, PyObject* InOther)
{
	if (!ValidateInternalState(InSelf))
	{
		return -1;
	}

	const UPyUtil::FPropertyDef SelfElementDef = InSelf->SetProp->ElementProp;
	FScriptSetHelper SelfScriptSetHelper(InSelf->SetProp, InSelf->SetInstance);

	FUPyWrapperSetPtr Other = FUPyWrapperSetPtr::StealReference(FUPyWrapperSet::CastPyObject(InOther, &UPyWrapperSetType, SelfElementDef));
	if (!Other)
	{
		UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Cannot convert argument (%s) to '%s'"), *UPyUtil::GetFriendlyTypename(InOther), *UPyUtil::GetFriendlyTypename(InSelf)));
		return -1;
	}

	const FScriptSetHelper OtherScriptSetHelper(Other->SetProp, Other->SetInstance);

	// We need to go through the other set and remove any values from Self that are present in Other, and add any values from Other that aren't present in Self
	for (FScriptSetHelper::FIterator It(OtherScriptSetHelper); It; ++It)
	{
		const void* OtherElementPtr = OtherScriptSetHelper.GetElementPtr(It);
		if (SelfScriptSetHelper.FindElementIndexFromHash(OtherElementPtr) == INDEX_NONE)
		{
			SelfScriptSetHelper.AddElement(OtherElementPtr);
		}
		else
		{
			SelfScriptSetHelper.RemoveElement(OtherElementPtr);
		}
	}

	return 0;
}

FUPyWrapperSet* FUPyWrapperSet::Union(FUPyWrapperSet* InSelf, PyObject* InOthers)
{
	if (!ValidateInternalState(InSelf))
	{
		return nullptr;
	}

	FUPyWrapperSetPtr NewSet = FUPyWrapperSetPtr::StealReference(FUPyWrapperSetFactory::Get().CreateInstance(InSelf->SetInstance, InSelf->SetProp, FUPyWrapperOwnerContext(), EUPyConversionMethod::Copy));
	if (Update(NewSet, InOthers) != 0)
	{
		return nullptr;
	}

	return NewSet.Release();
}

int FUPyWrapperSet::Update(FUPyWrapperSet* InSelf, PyObject* InOthers)
{
	if (!ValidateInternalState(InSelf))
	{
		return -1;
	}

	const UPyUtil::FPropertyDef SelfElementDef = InSelf->SetProp->ElementProp;
	FScriptSetHelper SelfScriptSetHelper(InSelf->SetProp, InSelf->SetInstance);

	auto ProcessOtherObject = [&InSelf, &SelfScriptSetHelper, &SelfElementDef](PyObject* InOtherObj, const int32 InObjIndex)
	{
		FUPyWrapperSetPtr Other = FUPyWrapperSetPtr::StealReference(FUPyWrapperSet::CastPyObject(InOtherObj, &UPyWrapperSetType, SelfElementDef));
		if (!Other)
		{
			UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Cannot convert argument %d (%s) to '%s'"), InObjIndex, *UPyUtil::GetFriendlyTypename(InOtherObj), *UPyUtil::GetFriendlyTypename(InSelf)));
			return false;
		}

		const FScriptSetHelper OtherScriptSetHelper(Other->SetProp, Other->SetInstance);
		for (FScriptSetHelper::FIterator It(OtherScriptSetHelper); It; ++It)
		{
			const void* OtherElementPtr = OtherScriptSetHelper.GetElementPtr(It);
			SelfScriptSetHelper.AddElement(OtherElementPtr);
		}

		return true;
	};

	if (PyTuple_Check(InOthers))
	{
		// We need to go through each argument we were passed and remove any values from NewSet that aren't present in the others
		const Py_ssize_t TupleLen = PyTuple_Size(InOthers);
		for (Py_ssize_t TupleIndex = 0; TupleIndex < TupleLen; ++TupleIndex)
		{
			PyObject* OtherObj = PyTuple_GetItem(InOthers, TupleIndex);
			if (!ProcessOtherObject(OtherObj, TupleIndex))
			{
				return -1;
			}
		}
	}
	else
	{
		// Assume a single value
		if (!ProcessOtherObject(InOthers, 0))
		{
			return -1;
		}
	}

	return 0;
}

int FUPyWrapperSet::IsDisjoint(FUPyWrapperSet* InSelf, PyObject* InOther)
{
	if (!ValidateInternalState(InSelf))
	{
		return -1;
	}

	FUPyWrapperSetPtr IntersectionSet = FUPyWrapperSetPtr::StealReference(FUPyWrapperSet::Intersection(InSelf, InOther));
	if (!IntersectionSet)
	{
		return -1;
	}

	FScriptSetHelper IntersectionScriptSetHelper(IntersectionSet->SetProp, IntersectionSet->SetInstance);
	if (IntersectionScriptSetHelper.Num() == 0)
	{
		return 1;
	}

	return 0;
}

int FUPyWrapperSet::IsSubset(FUPyWrapperSet* InSelf, PyObject* InOther)
{
	if (!ValidateInternalState(InSelf))
	{
		return -1;
	}

	const UPyUtil::FPropertyDef SelfElementDef = InSelf->SetProp->ElementProp;
	FUPyWrapperSetPtr Other = FUPyWrapperSetPtr::StealReference(FUPyWrapperSet::CastPyObject(InOther, &UPyWrapperSetType, SelfElementDef));
	if (!Other)
	{
		UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Cannot convert argument (%s) to '%s'"), *UPyUtil::GetFriendlyTypename(InOther), *UPyUtil::GetFriendlyTypename(InSelf)));
		return -1;
	}

	const FScriptSetHelper SelfScriptSetHelper(InSelf->SetProp, InSelf->SetInstance);
	const FScriptSetHelper OtherScriptSetHelper(Other->SetProp, Other->SetInstance);

	for (FScriptSetHelper::FIterator It(SelfScriptSetHelper); It; ++It)
	{
		const void* SelfElementPtr = SelfScriptSetHelper.GetElementPtr(It);
		if (OtherScriptSetHelper.FindElementIndexFromHash(SelfElementPtr) == INDEX_NONE)
		{
			return 0;
		}
	}

	return 1;
}

int FUPyWrapperSet::IsSuperset(FUPyWrapperSet* InSelf, PyObject* InOther)
{
	if (!ValidateInternalState(InSelf))
	{
		return -1;
	}

	const UPyUtil::FPropertyDef SelfElementDef = InSelf->SetProp->ElementProp;
	FUPyWrapperSetPtr Other = FUPyWrapperSetPtr::StealReference(FUPyWrapperSet::CastPyObject(InOther, &UPyWrapperSetType, SelfElementDef));
	if (!Other)
	{
		UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Cannot convert argument (%s) to '%s'"), *UPyUtil::GetFriendlyTypename(InOther), *UPyUtil::GetFriendlyTypename(InSelf)));
		return -1;
	}

	const FScriptSetHelper SelfScriptSetHelper(InSelf->SetProp, InSelf->SetInstance);
	const FScriptSetHelper OtherScriptSetHelper(Other->SetProp, Other->SetInstance);

	for (FScriptSetHelper::FIterator It(OtherScriptSetHelper); It; ++It)
	{
		const void* OtherElementPtr = OtherScriptSetHelper.GetElementPtr(It);
		if (SelfScriptSetHelper.FindElementIndexFromHash(OtherElementPtr) == INDEX_NONE)
		{
			return 0;
		}
	}

	return 1;
}

// ==================== Wrapper Set Iterator type BEGIN ====================
struct FFuncs_WrapperSetIterator
{
	static PyObject* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
	{
		return (PyObject*)FUPyWrapperSetIterator::New(InType);
	}

	static void Dealloc(FUPyWrapperSetIterator* InSelf)
	{
		FUPyWrapperSetIterator::Free(InSelf);
	}

	static int Init(FUPyWrapperSetIterator* InSelf, PyObject* InArgs, PyObject* InKwds)
	{
		PyObject* PyObj = nullptr;
		if (!PyArg_ParseTuple(InArgs, "O:call", &PyObj))
		{
			return -1;
		}

		if (PyObject_IsInstance(PyObj, (PyObject*)&UPyWrapperSetType) != 1)
		{
			UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Cannot initialize '%s' with an instance of '%s'"), *UPyUtil::GetFriendlyTypename(InSelf), *UPyUtil::GetFriendlyTypename(PyObj)));
			return -1;
		}

		return FUPyWrapperSetIterator::Init(InSelf, (FUPyWrapperSet*)PyObj);
	}

	static FUPyWrapperSetIterator* GetIter(FUPyWrapperSetIterator* InSelf)
	{
		return FUPyWrapperSetIterator::GetIter(InSelf);
	}

	static PyObject* IterNext(FUPyWrapperSetIterator* InSelf)
	{
		return FUPyWrapperSetIterator::IterNext(InSelf);
	}
};

void InitializePyWrapperSetIteratorType()
{
	PyTypeObject* PyType = &UPyWrapperSetIteratorType;

	PyType->tp_new = (newfunc)&FFuncs_WrapperSetIterator::New;
	PyType->tp_dealloc = (destructor)&FFuncs_WrapperSetIterator::Dealloc;
	PyType->tp_init = (initproc)&FFuncs_WrapperSetIterator::Init;
	PyType->tp_iter = (getiterfunc)&FFuncs_WrapperSetIterator::GetIter;
	PyType->tp_iternext = (iternextfunc)&FFuncs_WrapperSetIterator::IterNext;

	PyType->tp_flags = Py_TPFLAGS_DEFAULT;
	PyType->tp_doc = "Type for all Unreal exposed set iterators";

	PyType_Ready(PyType);
}
// ==================== Wrapper Set Iterator type END ====================

// ==================== Wrapper Set type BEGIN ====================
struct FFuncs_WrapperSet
{
	static PyObject* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
	{
		return (PyObject*)FUPyWrapperSet::New(InType);
	}

	static void Dealloc(FUPyWrapperSet* InSelf)
	{
		FUPyWrapperSet::Free(InSelf);
	}

	static int Init(FUPyWrapperSet* InSelf, PyObject* InArgs, PyObject* InKwds)
	{
		PyObject* PyTypeObj = nullptr;

		static const char *ArgsKwdList[] = { "type", nullptr };
		if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "O:call", (char**)ArgsKwdList, &PyTypeObj))
		{
			return -1;
		}

		UPyUtil::FPropertyDef SetElementDef;
		const int ValidateTypeResult = UPyUtil::ValidateContainerTypeParam(PyTypeObj, SetElementDef, "type", *UPyUtil::GetErrorContext(Py_TYPE(InSelf)));
		if (ValidateTypeResult != 0)
		{
			return ValidateTypeResult;
		}

		return FUPyWrapperSet::Init(InSelf, SetElementDef);
	}

	static PyObject* Str(FUPyWrapperSet* InSelf)
	{
		if (!FUPyWrapperSet::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		const FScriptSetHelper SelfScriptSetHelper(InSelf->SetProp, InSelf->SetInstance);

		FString ExportedSet;
		for (FScriptSetHelper::FIterator It(SelfScriptSetHelper); It; ++It)
		{
			if (It.GetLogicalIndex() > 0)
			{
				ExportedSet += TEXT(", ");
			}
			ExportedSet += UPyUtil::GetFriendlyPropertyValue(SelfScriptSetHelper.GetElementProperty(), SelfScriptSetHelper.GetElementPtr(It), PPF_Delimited | PPF_IncludeTransient);
		}
		return PyUnicode_FromFormat("set([%s])", TCHAR_TO_UTF8(*ExportedSet));
	}

	static PyObject* RichCmp(FUPyWrapperSet* InSelf, PyObject* InOther, int InOp)
	{
		if (!FUPyWrapperSet::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		const UPyUtil::FPropertyDef SelfElementDef = InSelf->SetProp->ElementProp;
		FUPyWrapperSetPtr Other = FUPyWrapperSetPtr::StealReference(FUPyWrapperSet::CastPyObject(InOther, &UPyWrapperSetType, SelfElementDef));
		if (!Other)
		{
			Py_INCREF(Py_NotImplemented);
			return Py_NotImplemented;
		}

		if (InOp != Py_EQ && InOp != Py_NE)
		{
			UPyUtil::SetPythonError(PyExc_TypeError, InSelf, TEXT("Only == and != comparison is supported"));
			return nullptr;
		}

		const bool bIsIdentical = InSelf->SetProp->Identical(InSelf->SetInstance, Other->SetInstance, PPF_None);
		return PyBool_FromLong(InOp == Py_EQ ? bIsIdentical : !bIsIdentical);
	}

	static Py_hash_t Hash(FUPyWrapperSet* InSelf)
	{
		if (!FUPyWrapperSet::ValidateInternalState(InSelf))
		{
			return -1;
		}

		if (InSelf->SetProp->HasAnyPropertyFlags(CPF_HasGetValueTypeHash))
		{
			const uint32 SetHash = InSelf->SetProp->GetValueTypeHash(InSelf->SetInstance);
			return (Py_hash_t)(SetHash != -1 ? SetHash : 0);
		}

		UPyUtil::SetPythonError(PyExc_Exception, InSelf, TEXT("Type cannot be hashed"));
		return -1;
	}

	static PyObject* GetIter(FUPyWrapperSet* InSelf)
	{
		typedef TUPyPtr<FUPyWrapperSetIterator> FPyWrapperSetIteratorPtr;

		if (!FUPyWrapperSet::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		FPyWrapperSetIteratorPtr NewIter = FPyWrapperSetIteratorPtr::StealReference(FUPyWrapperSetIterator::New(&UPyWrapperSetIteratorType));
		if (FUPyWrapperSetIterator::Init(NewIter, InSelf) != 0)
		{
			return nullptr;
		}

		return (PyObject*)NewIter.Release();
	}
};

struct FNumberFuncs_WrapperSet
{
	static PyObject* Sub(PyObject* InLHS, PyObject* InRHS)
	{
		FUPyWrapperSetPtr LHS = FUPyWrapperSetPtr::StealReference(FUPyWrapperSet::CastPyObject(InLHS));
		FUPyWrapperSetPtr RHS = FUPyWrapperSetPtr::StealReference(FUPyWrapperSet::CastPyObject(InRHS));
		if (!LHS && !RHS)
		{
			UPyUtil::SetPythonError(PyExc_TypeError, *UPyUtil::GetErrorContext(&UPyWrapperSetType), *FString::Printf(TEXT("One of LHS or RHS must be a '%s'"), *UPyUtil::GetFriendlyTypename(&UPyWrapperSetType)));
			return nullptr;
		}

		return (PyObject*)(LHS ? FUPyWrapperSet::Difference(LHS, InRHS) : FUPyWrapperSet::Difference(RHS, InLHS));
	}

	static PyObject* SubInplace(FUPyWrapperSet* InSelf, PyObject* InOther)
	{
		if (FUPyWrapperSet::DifferenceUpdate(InSelf, InOther) != 0)
		{
			return nullptr;
		}

		Py_RETURN_NONE;
	}

	static PyObject* And(PyObject* InLHS, PyObject* InRHS)
	{
		FUPyWrapperSetPtr LHS = FUPyWrapperSetPtr::StealReference(FUPyWrapperSet::CastPyObject(InLHS));
		FUPyWrapperSetPtr RHS = FUPyWrapperSetPtr::StealReference(FUPyWrapperSet::CastPyObject(InRHS));
		if (!LHS && !RHS)
		{
			UPyUtil::SetPythonError(PyExc_TypeError, *UPyUtil::GetErrorContext(&UPyWrapperSetType), *FString::Printf(TEXT("One of LHS or RHS must be a '%s'"), *UPyUtil::GetFriendlyTypename(&UPyWrapperSetType)));
			return nullptr;
		}

		return (PyObject*)(LHS ? FUPyWrapperSet::Intersection(LHS, InRHS) : FUPyWrapperSet::Intersection(RHS, InLHS));
	}

	static PyObject* AndInplace(FUPyWrapperSet* InSelf, PyObject* InOther)
	{
		if (FUPyWrapperSet::IntersectionUpdate(InSelf, InOther) != 0)
		{
			return nullptr;
		}

		Py_RETURN_NONE;
	}

	static PyObject* Xor(PyObject* InLHS, PyObject* InRHS)
	{
		FUPyWrapperSetPtr LHS = FUPyWrapperSetPtr::StealReference(FUPyWrapperSet::CastPyObject(InLHS));
		FUPyWrapperSetPtr RHS = FUPyWrapperSetPtr::StealReference(FUPyWrapperSet::CastPyObject(InRHS));
		if (!LHS && !RHS)
		{
			UPyUtil::SetPythonError(PyExc_TypeError, *UPyUtil::GetErrorContext(&UPyWrapperSetType), *FString::Printf(TEXT("One of LHS or RHS must be a '%s'"), *UPyUtil::GetFriendlyTypename(&UPyWrapperSetType)));
			return nullptr;
		}

		return (PyObject*)(LHS ? FUPyWrapperSet::SymmetricDifference(LHS, InRHS) : FUPyWrapperSet::SymmetricDifference(RHS, InLHS));
	}

	static PyObject* XorInplace(FUPyWrapperSet* InSelf, PyObject* InOther)
	{
		if (FUPyWrapperSet::SymmetricDifferenceUpdate(InSelf, InOther) != 0)
		{
			return nullptr;
		}

		Py_RETURN_NONE;
	}

	static PyObject* Or(PyObject* InLHS, PyObject* InRHS)
	{
		FUPyWrapperSetPtr LHS = FUPyWrapperSetPtr::StealReference(FUPyWrapperSet::CastPyObject(InLHS));
		FUPyWrapperSetPtr RHS = FUPyWrapperSetPtr::StealReference(FUPyWrapperSet::CastPyObject(InRHS));
		if (!LHS && !RHS)
		{
			UPyUtil::SetPythonError(PyExc_TypeError, *UPyUtil::GetErrorContext(&UPyWrapperSetType), *FString::Printf(TEXT("One of LHS or RHS must be a '%s'"), *UPyUtil::GetFriendlyTypename(&UPyWrapperSetType)));
			return nullptr;
		}

		return (PyObject*)(LHS ? FUPyWrapperSet::Union(LHS, InRHS) : FUPyWrapperSet::Union(RHS, InLHS));
	}

	static PyObject* OrInplace(FUPyWrapperSet* InSelf, PyObject* InOther)
	{
		if (FUPyWrapperSet::Update(InSelf, InOther) != 0)
		{
			return nullptr;
		}

		Py_RETURN_NONE;
	}
};

struct FSequenceFuncs_WrapperSet
{
	static Py_ssize_t Len(FUPyWrapperSet* InSelf)
	{
		return FUPyWrapperSet::Len(InSelf);
	}

	static int Contains(FUPyWrapperSet* InSelf, PyObject* InValue)
	{
		return FUPyWrapperSet::Contains(InSelf, InValue);
	}
};

void InitializeUPyWrapperSet(UPyGenUtil::FNativePythonModule& ModuleInfo)
{
	PyTypeObject* PyType = &UPyWrapperSetType;

	PyType->tp_base = &UPyWrapperBaseType;
	PyType->tp_new = (newfunc)&FFuncs_WrapperSet::New;
	PyType->tp_dealloc = (destructor)&FFuncs_WrapperSet::Dealloc;
	PyType->tp_init = (initproc)&FFuncs_WrapperSet::Init;
	PyType->tp_str = (reprfunc)&FFuncs_WrapperSet::Str;
	PyType->tp_richcompare = (richcmpfunc)&FFuncs_WrapperSet::RichCmp;
	PyType->tp_hash = (hashfunc)&FFuncs_WrapperSet::Hash;
	PyType->tp_iter = (getiterfunc)&FFuncs_WrapperSet::GetIter;

	PyType->tp_methods = SetPyMethodDefs;

	PyType->tp_flags = Py_TPFLAGS_DEFAULT;
	PyType->tp_doc = "Type for all Unreal exposed set instances";

	static PyNumberMethods PyNumber;
	PyNumber.nb_subtract = (binaryfunc)&FNumberFuncs_WrapperSet::Sub;
	PyNumber.nb_inplace_subtract = (binaryfunc)&FNumberFuncs_WrapperSet::SubInplace;
	PyNumber.nb_and = (binaryfunc)&FNumberFuncs_WrapperSet::And;
	PyNumber.nb_inplace_and = (binaryfunc)&FNumberFuncs_WrapperSet::AndInplace;
	PyNumber.nb_xor = (binaryfunc)&FNumberFuncs_WrapperSet::Xor;
	PyNumber.nb_inplace_xor = (binaryfunc)&FNumberFuncs_WrapperSet::XorInplace;
	PyNumber.nb_or = (binaryfunc)&FNumberFuncs_WrapperSet::Or;
	PyNumber.nb_inplace_or = (binaryfunc)&FNumberFuncs_WrapperSet::OrInplace;

	static PySequenceMethods PySequence;
	PySequence.sq_length = (lenfunc)&FSequenceFuncs_WrapperSet::Len;
	PySequence.sq_contains = (objobjproc)&FSequenceFuncs_WrapperSet::Contains;

	PyType->tp_as_number = &PyNumber;
	PyType->tp_as_sequence = &PySequence;

	if (PyType_Ready(PyType) == 0)
	{
		ModuleInfo.AddType(PyType, true);
	}

	InitializePyWrapperSetIteratorType();
}

// ==================== Wrapper Set type END ====================
