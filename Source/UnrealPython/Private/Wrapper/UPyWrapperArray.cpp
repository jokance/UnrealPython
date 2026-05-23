
#include "Wrapper/UPyWrapperArray.h"
#include "Wrapper/UPyWrapperTypeRegistry.h"
#include "Core/UPyConversion.h"
// #include "PyReferenceCollector.h"
#include "UObject/UnrealType.h"
#include "UObject/Package.h"
#include "UObject/PropertyPortFlags.h"
#include "Wrapper/UPyWrapperTypeFactory.h"

extern PyMethodDef ArrayPyMethodDefs[];

/** Python type for FUPyWrapperArrayIterator */
extern PyTypeObject UPyWrapperArrayIteratorType;

PyTypeObject UPyWrapperArrayType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"Array", /* tp_name */
	sizeof(FUPyWrapperArray), /* tp_basicsize */
};

/** Iterator used with arrays */
struct FUPyWrapperArrayIterator
{
	/** Common Python Object */
	PyObject_HEAD

	/** Instance being iterated over */
	FUPyWrapperArray* IterInstance;

	/** Current iteration index */
	int32 IterIndex;

	/** New this iterator instance (called via tp_new for Python, or directly in C++) */
	static FUPyWrapperArrayIterator* New(PyTypeObject* InType)
	{
		FUPyWrapperArrayIterator* Self = (FUPyWrapperArrayIterator*)InType->tp_alloc(InType, 0);
		if (Self)
		{
			Self->IterInstance = nullptr;
			Self->IterIndex = 0;
		}
		return Self;
	}

	/** Free this iterator instance (called via tp_dealloc for Python) */
	static void Free(FUPyWrapperArrayIterator* InSelf)
	{
		Deinit(InSelf);
		Py_TYPE(InSelf)->tp_free((PyObject*)InSelf);
	}

	/** Initialize this iterator instance (called via tp_init for Python, or directly in C++) */
	static int Init(FUPyWrapperArrayIterator* InSelf, FUPyWrapperArray* InInstance)
	{
		Deinit(InSelf);

		if (!FUPyWrapperArray::ValidateInternalState(InInstance))
		{
			return -1;
		}

		Py_INCREF(InInstance);
		InSelf->IterInstance = InInstance;
		InSelf->IterIndex = 0;

		return 0;
	}

	/** Deinitialize this iterator instance (called via Init and Free to restore the instance to its New state) */
	static void Deinit(FUPyWrapperArrayIterator* InSelf)
	{
		Py_XDECREF(InSelf->IterInstance);
		InSelf->IterInstance = nullptr;
		InSelf->IterIndex = 0;
	}

	/** Called to validate the internal state of this iterator instance prior to operating on it (should be called by all functions that expect to operate on an initialized type; will set an error state on failure) */
	static bool ValidateInternalState(FUPyWrapperArrayIterator* InSelf)
	{
		if (!InSelf->IterInstance)
		{
			UPyUtil::SetPythonError(PyExc_Exception, Py_TYPE(InSelf), TEXT("Internal Error - IterInstance is null!"));
			return false;
		}

		return FUPyWrapperArray::ValidateInternalState(InSelf->IterInstance);
	}

	/** Get the iterator */
	static FUPyWrapperArrayIterator* GetIter(FUPyWrapperArrayIterator* InSelf)
	{
		Py_INCREF(InSelf);
		return InSelf;
	}

	/** Return the current value (if any) and advance the iterator */
	static PyObject* IterNext(FUPyWrapperArrayIterator* InSelf)
	{
		if (!ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		if (InSelf->IterIndex < FUPyWrapperArray::Len(InSelf->IterInstance))
		{
			return FUPyWrapperArray::GetItem(InSelf->IterInstance, InSelf->IterIndex++);
		}

		PyErr_SetObject(PyExc_StopIteration, Py_None);
		return nullptr;
	}
};

PyTypeObject UPyWrapperArrayIteratorType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"ArrayIterator", /* tp_name */
	sizeof(FUPyWrapperArrayIterator), /* tp_basicsize */
};

FUPyWrapperArray* FUPyWrapperArray::New(PyTypeObject* InType)
{
	FUPyWrapperArray* Self = (FUPyWrapperArray*)FUPyWrapperBase::New(InType);
	if (Self)
	{
		new(&Self->OwnerContext) FUPyWrapperOwnerContext();
		new(&Self->ArrayProp) UPyUtil::FConstArrayPropOnScope();
		Self->ArrayInstance = nullptr;
	}
	return Self;
}

void FUPyWrapperArray::Free(FUPyWrapperArray* InSelf)
{
	Deinit(InSelf);

	InSelf->OwnerContext.~FUPyWrapperOwnerContext();
	InSelf->ArrayProp.~TPropOnScope();
	FUPyWrapperBase::Free(InSelf);
}

int FUPyWrapperArray::Init(FUPyWrapperArray* InSelf, const UPyUtil::FPropertyDef& InElementDef)
{
	Deinit(InSelf);

	const int BaseInit = FUPyWrapperBase::Init(InSelf);
	if (BaseInit != 0)
	{
		return BaseInit;
	}

	UPyUtil::FPropOnScope ArrayElementProp = UPyUtil::FPropOnScope::OwnedReference(UPyUtil::CreateProperty(InElementDef, 1));
	if (!ArrayElementProp)
	{
		UPyUtil::SetPythonError(PyExc_Exception, InSelf, TEXT("Array element property was null during init"));
		return -1;
	}

	UPyUtil::FArrayPropOnScope ArrayProp = UPyUtil::FArrayPropOnScope::OwnedReference(new FArrayProperty(FFieldVariant(), UPyUtil::DefaultPythonPropertyName, RF_NoFlags));
	ArrayProp->Inner = ArrayElementProp.Release();

	// Need to manually call Link to fix-up some data (such as the C++ property flags) that are only set during Link
	{
		FArchive Ar;
		ArrayProp->LinkWithoutChangingOffset(Ar);
	}

	void* ArrayValue = FMemory::Malloc(ArrayProp->GetSize(), ArrayProp->GetMinAlignment());
	ArrayProp->InitializeValue(ArrayValue);

	InSelf->ArrayProp = MoveTemp(ArrayProp);
	InSelf->ArrayInstance = ArrayValue;

	// FUPyWrapperArrayFactory::Get().MapInstance(InSelf->ArrayInstance, InSelf);
	return 0;
}

int FUPyWrapperArray::Init(FUPyWrapperArray* InSelf, const FUPyWrapperOwnerContext& InOwnerContext, const FArrayProperty* InProp, void* InValue, const EUPyConversionMethod InConversionMethod)
{
	InOwnerContext.AssertValidConversionMethod(InConversionMethod);

	Deinit(InSelf);

	const int BaseInit = FUPyWrapperBase::Init(InSelf);
	if (BaseInit != 0)
	{
		return BaseInit;
	}

	check(InProp && InValue);

	UPyUtil::FConstArrayPropOnScope PropToUse;
	void* ArrayInstanceToUse = nullptr;
	switch (InConversionMethod)
	{
	case EUPyConversionMethod::Copy:
	case EUPyConversionMethod::Steal:
		{
			UPyUtil::FPropOnScope ArrayElementProp = UPyUtil::FPropOnScope::OwnedReference(UPyUtil::CreateProperty(InProp->Inner));
			if (!ArrayElementProp)
			{
				UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to create element property from '%s' (%s)"), *InProp->Inner->GetName(), *InProp->Inner->GetClass()->GetName()));
				return -1;
			}

			UPyUtil::FArrayPropOnScope ArrayProp = UPyUtil::FArrayPropOnScope::OwnedReference(new FArrayProperty(FFieldVariant(), UPyUtil::DefaultPythonPropertyName, RF_NoFlags));
			ArrayProp->Inner = ArrayElementProp.Release();

			// Need to manually call Link to fix-up some data (such as the C++ property flags) that are only set during Link
			{
				FArchive Ar;
				ArrayProp->LinkWithoutChangingOffset(Ar);
			}

			PropToUse = MoveTemp(ArrayProp);

			ArrayInstanceToUse = FMemory::Malloc(PropToUse->GetSize(), PropToUse->GetMinAlignment());
			PropToUse->InitializeValue(ArrayInstanceToUse);
			if (InConversionMethod == EUPyConversionMethod::Steal)
			{
				FScriptArrayHelper SelfScriptArrayHelper(PropToUse, ArrayInstanceToUse);
				SelfScriptArrayHelper.MoveAssign(InValue);
			}
			else
			{
				PropToUse->CopyCompleteValue(ArrayInstanceToUse, InValue);
			}
		}
		break;

	case EUPyConversionMethod::Reference:
		{
			PropToUse = UPyUtil::FConstArrayPropOnScope::ExternalReference(InProp);
			ArrayInstanceToUse = InValue;

			InOwnerContext.AddOwnedPyProp(InSelf);
		}
		break;

	default:
		checkf(false, TEXT("Unknown EUPyConversionMethod"));
		break;
	}

	check(PropToUse && ArrayInstanceToUse);

	InSelf->OwnerContext = InOwnerContext;
	InSelf->ArrayProp = MoveTemp(PropToUse);
	InSelf->ArrayInstance = ArrayInstanceToUse;

	// todo(hzn): 是否有必要所有的结构体都加入map
	if (InOwnerContext.HasOwner())
	{
		FUPyWrapperArrayFactory::Get().MapInstance(InSelf->ArrayInstance, InSelf);
	}
	return 0;
}

void FUPyWrapperArray::Deinit(FUPyWrapperArray* InSelf)
{
	if (InSelf->ArrayInstance)
	{
		FUPyWrapperArrayFactory::Get().UnmapInstance(InSelf->ArrayInstance);
	}

	if (InSelf->OwnerContext.HasOwner())
	{
		InSelf->OwnerContext.Reset();
	}
	else if (InSelf->ArrayInstance)
	{
		if (InSelf->ArrayProp)
		{
			InSelf->ArrayProp->DestroyValue(InSelf->ArrayInstance);
		}
		FMemory::Free(InSelf->ArrayInstance);
	}
	InSelf->ArrayProp.Reset();
	InSelf->ArrayInstance = nullptr;
}

bool FUPyWrapperArray::ValidateInternalState(FUPyWrapperArray* InSelf)
{
	if (!InSelf->ArrayProp)
	{
		UPyUtil::SetPythonError(PyExc_Exception, Py_TYPE(InSelf), TEXT("Internal Error - ArrayProp is null!"));
		return false;
	}

	if (!InSelf->ArrayInstance)
	{
		UPyUtil::SetPythonError(PyExc_Exception, Py_TYPE(InSelf), TEXT("Internal Error - ArrayInstance is null!"));
		return false;
	}

	return true;
}

FUPyWrapperArray* FUPyWrapperArray::CastPyObject(PyObject* InPyObject, FUPyConversionResult* OutCastResult)
{
	if (PyObject_IsInstance(InPyObject, (PyObject*)&UPyWrapperArrayType) == 1)
	{
		Py_INCREF(InPyObject);
		return (FUPyWrapperArray*)InPyObject;
	}

	return nullptr;
}

FUPyWrapperArray* FUPyWrapperArray::CastPyObject(PyObject* InPyObject, PyTypeObject* InType, const UPyUtil::FPropertyDef& InElementDef, FUPyConversionResult* OutCastResult)
{
	SetOptionalUPyConversionResult(FUPyConversionResult::Failure(), OutCastResult);

	if (PyObject_IsInstance(InPyObject, (PyObject*)InType) == 1 && (InType == &UPyWrapperArrayType || PyObject_IsInstance(InPyObject, (PyObject*)&UPyWrapperArrayType) == 1))
	{
		FUPyWrapperArray* Self = (FUPyWrapperArray*)InPyObject;

		if (!ValidateInternalState(Self))
		{
			return nullptr;
		}

		const UPyUtil::FPropertyDef SelfElementPropDef = Self->ArrayProp->Inner;
		if (SelfElementPropDef == InElementDef)
		{
			SetOptionalUPyConversionResult(FUPyConversionResult::Success(), OutCastResult);

			Py_INCREF(Self);
			return Self;
		}

		FUPyWrapperArrayPtr NewArray = FUPyWrapperArrayPtr::StealReference(FUPyWrapperArray::New(InType));
		if (FUPyWrapperArray::Init(NewArray, InElementDef) != 0)
		{
			return nullptr;
		}

		// Attempt to convert the entries in the array to the native format of the new array
		{
			FScriptArrayHelper SelfScriptArrayHelper(Self->ArrayProp, Self->ArrayInstance);
			FScriptArrayHelper NewScriptArrayHelper(NewArray->ArrayProp, NewArray->ArrayInstance);

			const int32 ElementCount = SelfScriptArrayHelper.Num();
			NewScriptArrayHelper.Resize(ElementCount);

			FString ExportedEntry;
			for (int32 ElementIndex = 0; ElementIndex < ElementCount; ++ElementIndex)
			{
				ExportedEntry.Reset();
				if (!Self->ArrayProp->Inner->ExportText_Direct(ExportedEntry, SelfScriptArrayHelper.GetRawPtr(ElementIndex), SelfScriptArrayHelper.GetRawPtr(ElementIndex), nullptr, PPF_None))
				{
					UPyUtil::SetPythonError(PyExc_Exception, Self, *FString::Printf(TEXT("Failed to export text for element property '%s' (%s) at index %d"), *Self->ArrayProp->Inner->GetName(), *Self->ArrayProp->Inner->GetClass()->GetName(), ElementIndex));
					return nullptr;
				}

				if (!NewArray->ArrayProp->Inner->ImportText_Direct(*ExportedEntry, NewScriptArrayHelper.GetRawPtr(ElementIndex), nullptr, PPF_None))
				{
					UPyUtil::SetPythonError(PyExc_Exception, Self, *FString::Printf(TEXT("Failed to import text '%s' element for property '%s' (%s) at index %d"), *ExportedEntry, *NewArray->ArrayProp->Inner->GetName(), *NewArray->ArrayProp->Inner->GetClass()->GetName(), ElementIndex));
					return nullptr;
				}
			}
		}

		SetOptionalUPyConversionResult(FUPyConversionResult::SuccessWithCoercion(), OutCastResult);
		return NewArray.Release();
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
				FUPyWrapperArrayPtr NewArray = FUPyWrapperArrayPtr::StealReference(FUPyWrapperArray::New(InType));
				if (FUPyWrapperArray::Init(NewArray, InElementDef) != 0)
				{
					return nullptr;
				}

				FScriptArrayHelper NewScriptArrayHelper(NewArray->ArrayProp, NewArray->ArrayInstance);
				NewScriptArrayHelper.Resize(SequenceLen);

				for (Py_ssize_t SequenceIndex = 0; SequenceIndex < SequenceLen; ++SequenceIndex)
				{
					FUPyObjectPtr SequenceItem = FUPyObjectPtr::StealReference(PyIter_Next(PyObjIter));
					if (!SequenceItem)
					{
						return nullptr;
					}

					if (!UPyConversion::NativizeProperty(SequenceItem, NewArray->ArrayProp->Inner, NewScriptArrayHelper.GetRawPtr((int32)SequenceIndex)))
					{
						return nullptr;
					}
				}
				SetOptionalUPyConversionResult(FUPyConversionResult::SuccessWithCoercion(), OutCastResult);

				return NewArray.Release();
			}
		}
	}

	return nullptr;
}

Py_ssize_t FUPyWrapperArray::Len(FUPyWrapperArray* InSelf)
{
	if (!ValidateInternalState(InSelf))
	{
		return -1;
	}

	FScriptArrayHelper SelfScriptArrayHelper(InSelf->ArrayProp, InSelf->ArrayInstance);
	return SelfScriptArrayHelper.Num();
}

PyObject* FUPyWrapperArray::GetItem(FUPyWrapperArray* InSelf, Py_ssize_t InIndex)
{
	if (!ValidateInternalState(InSelf))
	{
		return nullptr;
	}

	FScriptArrayHelper SelfScriptArrayHelper(InSelf->ArrayProp, InSelf->ArrayInstance);
	const int32 ElementCount = SelfScriptArrayHelper.Num();
	const Py_ssize_t ResolvedIndex = UPyUtil::ResolveContainerIndexParam(InIndex, ElementCount);

	if (UPyUtil::ValidateContainerIndexParam(ResolvedIndex, ElementCount, InSelf->ArrayProp, *UPyUtil::GetErrorContext(InSelf)) != 0)
	{
		return nullptr;
	}

	PyObject* PyItemObj = nullptr;
	if (!UPyConversion::PythonizeProperty(InSelf->ArrayProp->Inner, SelfScriptArrayHelper.GetRawPtr(ResolvedIndex), PyItemObj))
	{
		UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert element property '%s' (%s) at index %zd"), *InSelf->ArrayProp->Inner->GetName(), *InSelf->ArrayProp->Inner->GetClass()->GetName(), ResolvedIndex));
		return nullptr;
	}
	return PyItemObj;
}

int FUPyWrapperArray::SetItem(FUPyWrapperArray* InSelf, Py_ssize_t InIndex, PyObject* InValue)
{
	if (!ValidateInternalState(InSelf))
	{
		return -1;
	}

	FScriptArrayHelper SelfScriptArrayHelper(InSelf->ArrayProp, InSelf->ArrayInstance);
	const int32 ElementCount = SelfScriptArrayHelper.Num();
	const Py_ssize_t ResolvedIndex = UPyUtil::ResolveContainerIndexParam(InIndex, ElementCount);

	const int ValidateIndexResult = UPyUtil::ValidateContainerIndexParam(ResolvedIndex, ElementCount, InSelf->ArrayProp, *UPyUtil::GetErrorContext(InSelf));
	if (ValidateIndexResult != 0)
	{
		return ValidateIndexResult;
	}

	if (!UPyConversion::NativizeProperty(InValue, InSelf->ArrayProp->Inner, SelfScriptArrayHelper.GetRawPtr(ResolvedIndex)))
	{
		UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert element property '%s' (%s) at index %zd"), *InSelf->ArrayProp->Inner->GetName(), *InSelf->ArrayProp->Inner->GetClass()->GetName(), ResolvedIndex));
		return -1;
	}

	return 0;
}

int FUPyWrapperArray::Contains(FUPyWrapperArray* InSelf, PyObject* InValue)
{
	if (!ValidateInternalState(InSelf))
	{
		return -1;
	}

	UPyUtil::FArrayElementOnScope ContainerEntryValue(InSelf->ArrayProp);
	if (!ContainerEntryValue.IsValid())
	{
		return -1;
	}

	if (!ContainerEntryValue.SetValue(InValue, *UPyUtil::GetErrorContext(InSelf)))
	{
		return -1;
	}

	FScriptArrayHelper SelfScriptArrayHelper(InSelf->ArrayProp, InSelf->ArrayInstance);
	const int32 ElementCount = SelfScriptArrayHelper.Num();

	for (int32 ElementIndex = 0; ElementIndex < ElementCount; ++ElementIndex)
	{
		if (ContainerEntryValue.GetProp()->Identical(SelfScriptArrayHelper.GetRawPtr(ElementIndex), ContainerEntryValue.GetValue()))
		{
			return 1;
		}
	}

	return 0;
}

FUPyWrapperArray* FUPyWrapperArray::Concat(FUPyWrapperArray* InSelf, PyObject* InOther)
{
	if (!ValidateInternalState(InSelf))
	{
		return nullptr;
	}

	FUPyWrapperArrayPtr NewArray = FUPyWrapperArrayPtr::StealReference(FUPyWrapperArrayFactory::Get().CreateInstance(InSelf->ArrayInstance, InSelf->ArrayProp, FUPyWrapperOwnerContext(), EUPyConversionMethod::Copy));
	if (ConcatInplace(NewArray, InOther) != 0)
	{
		return nullptr;
	}

	return NewArray.Release();
}

int FUPyWrapperArray::ConcatInplace(FUPyWrapperArray* InSelf, PyObject* InOther)
{
	if (!ValidateInternalState(InSelf))
	{
		return -1;
	}

	const UPyUtil::FPropertyDef SelfElementDef = InSelf->ArrayProp->Inner;
	FUPyWrapperArrayPtr Other = FUPyWrapperArrayPtr::StealReference(FUPyWrapperArray::CastPyObject(InOther, &UPyWrapperArrayType, SelfElementDef));
	if (!Other)
	{
		UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Cannot concatenate types '%s' and '%s' together"), *UPyUtil::GetFriendlyTypename(InSelf), *UPyUtil::GetFriendlyTypename(InOther)));
		return -1;
	}

	FScriptArrayHelper SelfScriptArrayHelper(InSelf->ArrayProp, InSelf->ArrayInstance);
	FScriptArrayHelper OtherScriptArrayHelper(Other->ArrayProp, Other->ArrayInstance);

	const int32 SelfElementCount = SelfScriptArrayHelper.Num();
	const int32 OtherElementCount = OtherScriptArrayHelper.Num();

	SelfScriptArrayHelper.AddValues(OtherElementCount);
	for (int32 ElementIndex = 0; ElementIndex < OtherElementCount; ++ElementIndex)
	{
		InSelf->ArrayProp->Inner->CopyCompleteValue(SelfScriptArrayHelper.GetRawPtr(SelfElementCount + ElementIndex), OtherScriptArrayHelper.GetRawPtr(ElementIndex));
	}

	return 0;
}

FUPyWrapperArray* FUPyWrapperArray::Repeat(FUPyWrapperArray* InSelf, Py_ssize_t InMultiplier)
{
	if (!ValidateInternalState(InSelf))
	{
		return nullptr;
	}

	FUPyWrapperArrayPtr NewArray = FUPyWrapperArrayPtr::StealReference(FUPyWrapperArrayFactory::Get().CreateInstance(InSelf->ArrayInstance, InSelf->ArrayProp, FUPyWrapperOwnerContext(), EUPyConversionMethod::Copy));
	if (RepeatInplace(NewArray, InMultiplier) != 0)
	{
		return nullptr;
	}

	return NewArray.Release();
}

int FUPyWrapperArray::RepeatInplace(FUPyWrapperArray* InSelf, Py_ssize_t InMultiplier)
{
	if (!ValidateInternalState(InSelf))
	{
		return -1;
	}

	InMultiplier = FMath::Max(InMultiplier, (Py_ssize_t)0);

	FScriptArrayHelper SelfScriptArrayHelper(InSelf->ArrayProp, InSelf->ArrayInstance);
	const int32 SelfElementCount = SelfScriptArrayHelper.Num();

	SelfScriptArrayHelper.Resize(SelfElementCount * InMultiplier);

	int32 NewElementIndex = SelfElementCount;
	for (int32 MultipleIndex = 1; MultipleIndex < (int32)InMultiplier; ++MultipleIndex)
	{
		for (int32 ElementIndex = 0; ElementIndex < SelfElementCount; ++ElementIndex, ++NewElementIndex)
		{
			InSelf->ArrayProp->Inner->CopyCompleteValue(SelfScriptArrayHelper.GetRawPtr(NewElementIndex), SelfScriptArrayHelper.GetRawPtr(ElementIndex));
		}
	}

	return 0;
}

int FUPyWrapperArray::Append(FUPyWrapperArray* InSelf, PyObject* InValue)
{
	if (!ValidateInternalState(InSelf))
	{
		return -1;
	}

	FScriptArrayHelper SelfScriptArrayHelper(InSelf->ArrayProp, InSelf->ArrayInstance);
	const int32 NewValueIndex = SelfScriptArrayHelper.AddValue();

	if (!UPyConversion::NativizeProperty(InValue, InSelf->ArrayProp->Inner, SelfScriptArrayHelper.GetRawPtr(NewValueIndex)))
	{
		SelfScriptArrayHelper.RemoveValues(NewValueIndex);
		UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert '%s' to an element of '%s' for insertion"), *UPyUtil::GetFriendlyTypename(InValue), *UPyUtil::GetFriendlyTypename(InSelf)));
		return -1;
	}

	return 0;
}

Py_ssize_t FUPyWrapperArray::Count(FUPyWrapperArray* InSelf, PyObject* InValue)
{
	if (!ValidateInternalState(InSelf))
	{
		return -1;
	}

	UPyUtil::FArrayElementOnScope ContainerEntryValue(InSelf->ArrayProp);
	if (!ContainerEntryValue.IsValid())
	{
		return -1;
	}

	if (!ContainerEntryValue.SetValue(InValue, *UPyUtil::GetErrorContext(InSelf)))
	{
		return -1;
	}

	FScriptArrayHelper SelfScriptArrayHelper(InSelf->ArrayProp, InSelf->ArrayInstance);
	const int32 ElementCount = SelfScriptArrayHelper.Num();

	int32 ValueCount = 0;
	for (int32 ElementIndex = 0; ElementIndex < ElementCount; ++ElementIndex)
	{
		if (ContainerEntryValue.GetProp()->Identical(SelfScriptArrayHelper.GetRawPtr(ElementIndex), ContainerEntryValue.GetValue()))
		{
			++ValueCount;
		}
	}

	return (Py_ssize_t)ValueCount;
}

Py_ssize_t FUPyWrapperArray::Index(FUPyWrapperArray* InSelf, PyObject* InValue, Py_ssize_t InStartIndex, Py_ssize_t InStopIndex)
{
	if (!ValidateInternalState(InSelf))
	{
		return -1;
	}

	UPyUtil::FArrayElementOnScope ContainerEntryValue(InSelf->ArrayProp);
	if (!ContainerEntryValue.IsValid())
	{
		return -1;
	}

	if (!ContainerEntryValue.SetValue(InValue, *UPyUtil::GetErrorContext(InSelf)))
	{
		return -1;
	}

	FScriptArrayHelper SelfScriptArrayHelper(InSelf->ArrayProp, InSelf->ArrayInstance);
	const int32 ElementCount = SelfScriptArrayHelper.Num();
	const Py_ssize_t ResolvedStartIndex = UPyUtil::ResolveContainerIndexParam(InStartIndex, ElementCount);
	const Py_ssize_t ResolvedStopIndex = UPyUtil::ResolveContainerIndexParam(InStopIndex, ElementCount);

	const int32 StartIndex = FMath::Min((int32)ResolvedStartIndex, ElementCount);
	const int32 StopIndex = FMath::Max((int32)ResolvedStopIndex, ElementCount);

	int32 ReturnIndex = INDEX_NONE;
	for (int32 ElementIndex = StartIndex; ElementIndex < StopIndex; ++ElementIndex)
	{
		if (ContainerEntryValue.GetProp()->Identical(SelfScriptArrayHelper.GetRawPtr(ElementIndex), ContainerEntryValue.GetValue()))
		{
			ReturnIndex = ElementIndex;
			break;
		}
	}

	if (ReturnIndex == INDEX_NONE)
	{
		UPyUtil::SetPythonError(PyExc_ValueError, InSelf, TEXT("The given value was not found in the array"));
		return -1;
	}

	return ReturnIndex;
}

int FUPyWrapperArray::Insert(FUPyWrapperArray* InSelf, Py_ssize_t InIndex, PyObject* InValue)
{
	if (!ValidateInternalState(InSelf))
	{
		return -1;
	}

	FScriptArrayHelper SelfScriptArrayHelper(InSelf->ArrayProp, InSelf->ArrayInstance);
	const int32 ElementCount = SelfScriptArrayHelper.Num();
	const Py_ssize_t ResolvedIndex = UPyUtil::ResolveContainerIndexParam(InIndex, ElementCount);

	const int32 InsertIndex = FMath::Min((int32)ResolvedIndex, ElementCount);
	SelfScriptArrayHelper.InsertValues(InsertIndex);

	if (!UPyConversion::NativizeProperty(InValue, InSelf->ArrayProp->Inner, SelfScriptArrayHelper.GetRawPtr(InsertIndex)))
	{
		SelfScriptArrayHelper.RemoveValues(InsertIndex);
		UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert '%s' to an element of '%s' for insertion"), *UPyUtil::GetFriendlyTypename(InValue), *UPyUtil::GetFriendlyTypename(InSelf)));
		return -1;
	}

	return 0;
}

PyObject* FUPyWrapperArray::Pop(FUPyWrapperArray* InSelf, Py_ssize_t InIndex)
{
	if (!ValidateInternalState(InSelf))
	{
		return nullptr;
	}

	FScriptArrayHelper SelfScriptArrayHelper(InSelf->ArrayProp, InSelf->ArrayInstance);
	const int32 ElementCount = SelfScriptArrayHelper.Num();
	const Py_ssize_t ResolvedIndex = UPyUtil::ResolveContainerIndexParam(InIndex, ElementCount);

	if (UPyUtil::ValidateContainerIndexParam(ResolvedIndex, ElementCount, InSelf->ArrayProp, *UPyUtil::GetErrorContext(InSelf)) != 0)
	{
		return nullptr;
	}

	PyObject* PyReturnValue = nullptr;
	if (!UPyConversion::PythonizeProperty(InSelf->ArrayProp->Inner, SelfScriptArrayHelper.GetRawPtr(ResolvedIndex), PyReturnValue))
	{
		UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert element property '%s' (%s) at index %zd"), *InSelf->ArrayProp->Inner->GetName(), *InSelf->ArrayProp->Inner->GetClass()->GetName(), ResolvedIndex));
		return nullptr;
	}

	SelfScriptArrayHelper.RemoveValues(ResolvedIndex);

	return PyReturnValue;
}

int FUPyWrapperArray::Remove(FUPyWrapperArray* InSelf, PyObject* InValue)
{
	const int32 ValueIndex = Index(InSelf, InValue);
	if (ValueIndex == INDEX_NONE)
	{
		return -1;
	}

	FScriptArrayHelper SelfScriptArrayHelper(InSelf->ArrayProp, InSelf->ArrayInstance);
	SelfScriptArrayHelper.RemoveValues(ValueIndex);

	return 0;
}

int FUPyWrapperArray::Clear(FUPyWrapperArray* InSelf)
{
	if (!ValidateInternalState(InSelf))
	{
		return -1;
	}

	FScriptArrayHelper SelfScriptArrayHelper(InSelf->ArrayProp, InSelf->ArrayInstance);
	SelfScriptArrayHelper.EmptyValues();

	return 0;
}

int FUPyWrapperArray::Reverse(FUPyWrapperArray* InSelf)
{
	if (!ValidateInternalState(InSelf))
	{
		return -1;
	}

	FScriptArrayHelper SelfScriptArrayHelper(InSelf->ArrayProp, InSelf->ArrayInstance);
	const int32 ElementCount = SelfScriptArrayHelper.Num();

	// Taken from Algo::Reverse
	for (int32 i = 0, i2 = ElementCount - 1; i < ElementCount / 2 /*rounding down*/; ++i, --i2)
	{
		SelfScriptArrayHelper.SwapValues(i, i2);
	}

	return 0;
}

int FUPyWrapperArray::Sort(FUPyWrapperArray* InSelf, PyObject* InKey, bool InReverse)
{
	if (!ValidateInternalState(InSelf))
	{
		return -1;
	}

	FScriptArrayHelper SelfScriptArrayHelper(InSelf->ArrayProp, InSelf->ArrayInstance);
	const int32 ElementCount = SelfScriptArrayHelper.Num();

	// This isn't ideal, but we have no sorting algorithms that take untyped data, and it's the simplest way to deal with the key (and cmp) arguments that need processing in Python.
	FUPyObjectPtr PyList = FUPyObjectPtr::StealReference(PyList_New(ElementCount));
	if (!PyList)
	{
		return -1;
	}

	for (int32 ElementIndex = 0; ElementIndex < ElementCount; ++ElementIndex)
	{
		PyObject* PyItemObj = nullptr;
		if (!UPyConversion::PythonizeProperty(InSelf->ArrayProp->Inner, SelfScriptArrayHelper.GetRawPtr(ElementIndex), PyItemObj))
		{
			UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert element property '%s' (%s) at index %d"), *InSelf->ArrayProp->Inner->GetName(), *InSelf->ArrayProp->Inner->GetClass()->GetName(), ElementIndex));
			return -1;
		}
		PyList_SetItem(PyList, ElementIndex, PyItemObj); // PyList_SetItem steals this reference
	}

	// We need to call 'sort' directly since PyList_Sort doesn't expose the version that takes arguments
	FUPyObjectPtr PyListSortFunc = FUPyObjectPtr::StealReference(PyObject_GetAttrString(PyList, "sort"));
	FUPyObjectPtr PyListSortArgs = FUPyObjectPtr::StealReference(PyTuple_New(0));
	FUPyObjectPtr PyListSortKwds = FUPyObjectPtr::StealReference(PyDict_New());

	PyDict_SetItemString(PyListSortKwds, "key", InKey ? InKey : Py_None);
	PyDict_SetItemString(PyListSortKwds, "reverse", InReverse ? Py_True : Py_False);

	FUPyObjectPtr PyListSortResult = FUPyObjectPtr::StealReference(PyObject_Call(PyListSortFunc, PyListSortArgs, PyListSortKwds));
	if (!PyListSortResult)
	{
		return -1;
	}

	for (int32 ElementIndex = 0; ElementIndex < ElementCount; ++ElementIndex)
	{
		PyObject* PyItemObj = PyList_GetItem(PyList, ElementIndex);
		UPyConversion::NativizeProperty(PyItemObj, InSelf->ArrayProp->Inner, SelfScriptArrayHelper.GetRawPtr(ElementIndex));
	}

	return 0;
}

int FUPyWrapperArray::Resize(FUPyWrapperArray* InSelf, Py_ssize_t InLen)
{
	if (!ValidateInternalState(InSelf))
	{
		return -1;
	}

	FScriptArrayHelper SelfScriptArrayHelper(InSelf->ArrayProp, InSelf->ArrayInstance);
	SelfScriptArrayHelper.Resize(FMath::Max(0, (int32)InLen));
	return 0;
}

// ==================== Wrapper Array iterator type BEGIN ====================
struct FFuncs_WrapperArrayIterator
{
	static PyObject* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
	{
		return (PyObject*)FUPyWrapperArrayIterator::New(InType);
	}

	static void Dealloc(FUPyWrapperArrayIterator* InSelf)
	{
		FUPyWrapperArrayIterator::Free(InSelf);
	}

	static int Init(FUPyWrapperArrayIterator* InSelf, PyObject* InArgs, PyObject* InKwds)
	{
		PyObject* PyObj = nullptr;
		if (!PyArg_ParseTuple(InArgs, "O:call", &PyObj))
		{
			return -1;
		}

		if (PyObject_IsInstance(PyObj, (PyObject*)&UPyWrapperArrayType) != 1)
		{
			UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Cannot initialize '%s' with an instance of '%s'"), *UPyUtil::GetFriendlyTypename(InSelf), *UPyUtil::GetFriendlyTypename(PyObj)));
			return -1;
		}

		return FUPyWrapperArrayIterator::Init(InSelf, (FUPyWrapperArray*)PyObj);
	}

	static FUPyWrapperArrayIterator* GetIter(FUPyWrapperArrayIterator* InSelf)
	{
		return FUPyWrapperArrayIterator::GetIter(InSelf);
	}

	static PyObject* IterNext(FUPyWrapperArrayIterator* InSelf)
	{
		return FUPyWrapperArrayIterator::IterNext(InSelf);
	}
};

void InitializeUPyWrapperArrayIteratorType()
{
	PyTypeObject* PyType = &UPyWrapperArrayIteratorType;

	PyType->tp_new = (newfunc)&FFuncs_WrapperArrayIterator::New;
	PyType->tp_dealloc = (destructor)&FFuncs_WrapperArrayIterator::Dealloc;
	PyType->tp_init = (initproc)&FFuncs_WrapperArrayIterator::Init;
	PyType->tp_iter = (getiterfunc)&FFuncs_WrapperArrayIterator::GetIter;
	PyType->tp_iternext = (iternextfunc)&FFuncs_WrapperArrayIterator::IterNext;

	PyType->tp_flags = Py_TPFLAGS_DEFAULT;
	PyType->tp_doc = "Type for all Unreal exposed array iterators";

	PyType_Ready(PyType);
}
// ==================== Wrapper Array iterator type END ====================

// ==================== Wrapper Array type BEGIN ====================
struct FFuncs_WrapperArray
{
	static PyObject* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
	{
		return (PyObject*)FUPyWrapperArray::New(InType);
	}

	static void Dealloc(FUPyWrapperArray* InSelf)
	{
		FUPyWrapperArray::Free(InSelf);
	}

	static int Init(FUPyWrapperArray* InSelf, PyObject* InArgs, PyObject* InKwds)
	{
		PyObject* PyTypeObj = nullptr;

		static const char *ArgsKwdList[] = { "type", nullptr };
		if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "O:call", (char**)ArgsKwdList, &PyTypeObj))
		{
			return -1;
		}

		UPyUtil::FPropertyDef ArrayElementDef;
		const int ValidateTypeResult = UPyUtil::ValidateContainerTypeParam(PyTypeObj, ArrayElementDef, "type", *UPyUtil::GetErrorContext(Py_TYPE(InSelf)));
		if (ValidateTypeResult != 0)
		{
			return ValidateTypeResult;
		}

		return FUPyWrapperArray::Init(InSelf, ArrayElementDef);
	}

	static PyObject* Str(FUPyWrapperArray* InSelf)
	{
		if (!FUPyWrapperArray::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		FScriptArrayHelper SelfScriptArrayHelper(InSelf->ArrayProp, InSelf->ArrayInstance);
		const int32 ElementCount = SelfScriptArrayHelper.Num();

		FString ExportedArray;
		for (int32 ElementIndex = 0; ElementIndex < ElementCount; ++ElementIndex)
		{
			if (ElementIndex > 0)
			{
				ExportedArray += TEXT(", ");
			}
			ExportedArray += UPyUtil::GetFriendlyPropertyValue(InSelf->ArrayProp->Inner, SelfScriptArrayHelper.GetRawPtr(ElementIndex), PPF_Delimited | PPF_IncludeTransient);
		}
		return PyUnicode_FromFormat("[%s]", TCHAR_TO_UTF8(*ExportedArray));
	}

	static PyObject* RichCmp(FUPyWrapperArray* InSelf, PyObject* InOther, int InOp)
	{
		if (!FUPyWrapperArray::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		const UPyUtil::FPropertyDef SelfElementDef = InSelf->ArrayProp->Inner;
		FUPyWrapperArrayPtr Other = FUPyWrapperArrayPtr::StealReference(FUPyWrapperArray::CastPyObject(InOther, &UPyWrapperArrayType, SelfElementDef));
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

		const bool bIsIdentical = InSelf->ArrayProp->Identical(InSelf->ArrayInstance, Other->ArrayInstance, PPF_None);
		return PyBool_FromLong(InOp == Py_EQ ? bIsIdentical : !bIsIdentical);
	}

	static Py_hash_t Hash(FUPyWrapperArray* InSelf)
	{
		if (!FUPyWrapperArray::ValidateInternalState(InSelf))
		{
			return -1;
		}

		if (InSelf->ArrayProp->HasAnyPropertyFlags(CPF_HasGetValueTypeHash))
		{
			const uint32 ArrayHash = InSelf->ArrayProp->GetValueTypeHash(InSelf->ArrayInstance);
			return (Py_hash_t)(ArrayHash != -1 ? ArrayHash : 0);
		}

		UPyUtil::SetPythonError(PyExc_Exception, InSelf, TEXT("Type cannot be hashed"));
		return -1;
	}

	static PyObject* GetIter(FUPyWrapperArray* InSelf)
	{
		typedef TUPyPtr<FUPyWrapperArrayIterator> FPyWrapperArrayIteratorPtr;

		if (!FUPyWrapperArray::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		FPyWrapperArrayIteratorPtr NewIter = FPyWrapperArrayIteratorPtr::StealReference(FUPyWrapperArrayIterator::New(&UPyWrapperArrayIteratorType));
		if (FUPyWrapperArrayIterator::Init(NewIter, InSelf) != 0)
		{
			return nullptr;
		}

		return (PyObject*)NewIter.Release();
	}
};

struct FSequenceFuncs_WrapperArray
{
	static Py_ssize_t Len(FUPyWrapperArray* InSelf)
	{
		return FUPyWrapperArray::Len(InSelf);
	}

	static PyObject* GetItem(FUPyWrapperArray* InSelf, Py_ssize_t InIndex)
	{
		return FUPyWrapperArray::GetItem(InSelf, InIndex);
	}

	static int SetItem(FUPyWrapperArray* InSelf, Py_ssize_t InIndex, PyObject* InValue)
	{
		return FUPyWrapperArray::SetItem(InSelf, InIndex, InValue);
	}

	static int Contains(FUPyWrapperArray* InSelf, PyObject* InValue)
	{
		return FUPyWrapperArray::Contains(InSelf, InValue);
	}

	static PyObject* Concat(FUPyWrapperArray* InSelf, PyObject* InOther)
	{
		return (PyObject*)FUPyWrapperArray::Concat(InSelf, InOther);
	}

	static PyObject* ConcatInplace(FUPyWrapperArray* InSelf, PyObject* InOther)
	{
		if (FUPyWrapperArray::ConcatInplace(InSelf, InOther) != 0)
		{
			return nullptr;
		}
		Py_RETURN_NONE;
	}

	static PyObject* Repeat(FUPyWrapperArray* InSelf, Py_ssize_t InMultiplier)
	{
		return (PyObject*)FUPyWrapperArray::Repeat(InSelf, InMultiplier);
	}

	static PyObject* RepeatInplace(FUPyWrapperArray* InSelf, Py_ssize_t InMultiplier)
	{
		if (FUPyWrapperArray::RepeatInplace(InSelf, InMultiplier) != 0)
		{
			return nullptr;
		}

		Py_RETURN_NONE;
	}

	static PyObject* GetSlice(FUPyWrapperArray* InSelf, Py_ssize_t InSliceStart, Py_ssize_t InSliceStop)
	{
		if (!FUPyWrapperArray::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		FScriptArrayHelper SelfScriptArrayHelper(InSelf->ArrayProp, InSelf->ArrayInstance);
		const int32 SelfElementCount = SelfScriptArrayHelper.Num();
		const Py_ssize_t ResolvedSliceStart = UPyUtil::ResolveContainerIndexParam(InSliceStart, SelfElementCount);
		const Py_ssize_t ResolvedSliceStop = UPyUtil::ResolveContainerIndexParam(InSliceStop, SelfElementCount);

		const Py_ssize_t SliceLen = FMath::Max(ResolvedSliceStop - ResolvedSliceStart, (Py_ssize_t)0);

		const UPyUtil::FPropertyDef SelfElementDef = InSelf->ArrayProp->Inner;
		FUPyWrapperArrayPtr NewArray = FUPyWrapperArrayPtr::StealReference(FUPyWrapperArray::New(Py_TYPE(InSelf)));
		if (FUPyWrapperArray::Init(NewArray, SelfElementDef) != 0)
		{
			return nullptr;
		}

		FScriptArrayHelper NewScriptArrayHelper(NewArray->ArrayProp, NewArray->ArrayInstance);
		NewScriptArrayHelper.Resize(SliceLen);

		for (Py_ssize_t ElementIndex = ResolvedSliceStart; ElementIndex < ResolvedSliceStop; ++ElementIndex)
		{
			InSelf->ArrayProp->Inner->CopyCompleteValue(NewScriptArrayHelper.GetRawPtr((int32)(ElementIndex - ResolvedSliceStart)), SelfScriptArrayHelper.GetRawPtr((int32)ElementIndex));
		}
		return (PyObject*)NewArray.Release();
	}

	static int SetSlice(FUPyWrapperArray* InSelf, Py_ssize_t InSliceStart, Py_ssize_t InSliceStop, PyObject* InValue)
	{
		if (!FUPyWrapperArray::ValidateInternalState(InSelf))
		{
			return -1;
		}

		FScriptArrayHelper SelfScriptArrayHelper(InSelf->ArrayProp, InSelf->ArrayInstance);
		const int32 SelfElementCount = SelfScriptArrayHelper.Num();
		const Py_ssize_t ResolvedSliceStart = UPyUtil::ResolveContainerIndexParam(InSliceStart, SelfElementCount);
		const Py_ssize_t ResolvedSliceStop = UPyUtil::ResolveContainerIndexParam(InSliceStop, SelfElementCount);

		const Py_ssize_t SliceLen = FMath::Max(ResolvedSliceStop - ResolvedSliceStart, (Py_ssize_t)0);

		// Value will be null when performing a slice delete
		FUPyWrapperArrayPtr Value;
		if (InValue)
		{
			const UPyUtil::FPropertyDef SelfElementDef = InSelf->ArrayProp->Inner;
			Value = FUPyWrapperArrayPtr::StealReference(FUPyWrapperArray::CastPyObject(InValue, &UPyWrapperArrayType, SelfElementDef));
			if (!Value)
			{
				UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Cannot assign type '%s' to type '%s' during a slice"), *UPyUtil::GetFriendlyTypename(InSelf), *UPyUtil::GetFriendlyTypename(InValue)));
				return -1;
			}
		}

		SelfScriptArrayHelper.RemoveValues((int32)ResolvedSliceStart, (int32)SliceLen);

		if (Value)
		{
			FScriptArrayHelper ValueScriptArrayHelper(Value->ArrayProp, Value->ArrayInstance);
			const int32 ValueElementCount = ValueScriptArrayHelper.Num();

			SelfScriptArrayHelper.InsertValues((int32)ResolvedSliceStart, ValueElementCount);
			for (int32 ElementIndex = 0; ElementIndex < ValueElementCount; ++ElementIndex)
			{
				InSelf->ArrayProp->Inner->CopyCompleteValue(SelfScriptArrayHelper.GetRawPtr(ResolvedSliceStart + ElementIndex), ValueScriptArrayHelper.GetRawPtr(ElementIndex));
			}
		}

		return 0;
	}
};

struct FMappingFuncs_WrapperArray
{
	static PyObject* GetItem(FUPyWrapperArray* InSelf, PyObject* InIndexer)
	{
		if (!FUPyWrapperArray::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		// Array types support numeric and slice based indexing
		int32 Index = 0;
		if (UPyConversion::Nativize(InIndexer, Index))
		{
			return FUPyWrapperArray::GetItem(InSelf, (Py_ssize_t)Index);
		}

		if (PySlice_Check(InIndexer))
		{
			Py_ssize_t SliceStart = 0;
			Py_ssize_t SliceStop = 0;
			Py_ssize_t SliceStep = 0;
			Py_ssize_t SliceLen = 0;
			if (PySlice_GetIndicesEx((UPyUtil::FPySliceType*)InIndexer, FUPyWrapperArray::Len(InSelf), &SliceStart, &SliceStop, &SliceStep, &SliceLen) != 0)
			{
				return nullptr;
			}
			if (SliceStep == 1)
			{
				return FSequenceFuncs_WrapperArray::GetSlice(InSelf, SliceStart, SliceStop);
			}
			else
			{
				UPyUtil::SetPythonError(PyExc_Exception, InSelf, TEXT("Slice step must be 1"));
				return nullptr;
			}
		}

		UPyUtil::SetPythonError(PyExc_Exception, InSelf, *FString::Printf(TEXT("Indexer '%s' was invalid for '%s' (%s)"), *UPyUtil::GetFriendlyTypename(InIndexer), *InSelf->ArrayProp->GetName(), *InSelf->ArrayProp->GetClass()->GetName()));
		return nullptr;
	}

	static int SetItem(FUPyWrapperArray* InSelf, PyObject* InIndexer, PyObject* InValue)
	{
		if (!FUPyWrapperArray::ValidateInternalState(InSelf))
		{
			return -1;
		}

		// Array types support numeric and slice based indexing
		int32 Index = 0;
		if (UPyConversion::Nativize(InIndexer, Index))
		{
			return FUPyWrapperArray::SetItem(InSelf, (Py_ssize_t)Index, InValue);
		}

		if (PySlice_Check(InIndexer))
		{
			Py_ssize_t SliceStart = 0;
			Py_ssize_t SliceStop = 0;
			Py_ssize_t SliceStep = 0;
			Py_ssize_t SliceLen = 0;
			if (PySlice_GetIndicesEx((UPyUtil::FPySliceType*)InIndexer, FUPyWrapperArray::Len(InSelf), &SliceStart, &SliceStop, &SliceStep, &SliceLen) != 0)
			{
				return -1;
			}
			if (SliceStep == 1)
			{
				return FSequenceFuncs_WrapperArray::SetSlice(InSelf, SliceStart, SliceStop, InValue);
			}
			else
			{
				UPyUtil::SetPythonError(PyExc_Exception, InSelf, TEXT("Slice step must be 1"));
				return -1;
			}
		}

		UPyUtil::SetPythonError(PyExc_Exception, InSelf, *FString::Printf(TEXT("Indexer '%s' was invalid for '%s' (%s)"), *UPyUtil::GetFriendlyTypename(InIndexer), *InSelf->ArrayProp->GetName(), *InSelf->ArrayProp->GetClass()->GetName()));
		return -1;
	}
};

void InitializeUPyWrapperArray(UPyGenUtil::FNativePythonModule& ModuleInfo)
{
	PyTypeObject* PyType = &UPyWrapperArrayType;

	PyType->tp_base = &UPyWrapperBaseType;
	PyType->tp_new = (newfunc)&FFuncs_WrapperArray::New;
	PyType->tp_dealloc = (destructor)&FFuncs_WrapperArray::Dealloc;
	PyType->tp_init = (initproc)&FFuncs_WrapperArray::Init;
	PyType->tp_str = (reprfunc)&FFuncs_WrapperArray::Str;
	PyType->tp_richcompare = (richcmpfunc)&FFuncs_WrapperArray::RichCmp;
	PyType->tp_hash = (hashfunc)&FFuncs_WrapperArray::Hash;
	PyType->tp_iter = (getiterfunc)&FFuncs_WrapperArray::GetIter;

	PyType->tp_methods = ArrayPyMethodDefs;

	PyType->tp_flags = Py_TPFLAGS_DEFAULT;
	PyType->tp_doc = "Type for all Unreal exposed array instances";

	static PySequenceMethods PySequence;
	PySequence.sq_length = (lenfunc)&FSequenceFuncs_WrapperArray::Len;
	PySequence.sq_item = (ssizeargfunc)&FSequenceFuncs_WrapperArray::GetItem;
	PySequence.sq_ass_item = (ssizeobjargproc)&FSequenceFuncs_WrapperArray::SetItem;
	PySequence.sq_contains = (objobjproc)&FSequenceFuncs_WrapperArray::Contains;
	PySequence.sq_concat = (binaryfunc)&FSequenceFuncs_WrapperArray::Concat;
	PySequence.sq_inplace_concat = (binaryfunc)&FSequenceFuncs_WrapperArray::ConcatInplace;
	PySequence.sq_repeat = (ssizeargfunc)&FSequenceFuncs_WrapperArray::Repeat;
	PySequence.sq_inplace_repeat = (ssizeargfunc)&FSequenceFuncs_WrapperArray::RepeatInplace;

	static PyMappingMethods PyMapping;
	PyMapping.mp_subscript = (binaryfunc)&FMappingFuncs_WrapperArray::GetItem;
	PyMapping.mp_ass_subscript = (objobjargproc)&FMappingFuncs_WrapperArray::SetItem;

	PyType->tp_as_sequence = &PySequence;
	PyType->tp_as_mapping = &PyMapping;

	if (PyType_Ready(PyType) == 0)
	{
		ModuleInfo.AddType(PyType, true);
	}

	InitializeUPyWrapperArrayIteratorType();
}
// ==================== Wrapper Array type END ====================

// void FPyWrapperArrayMetaData::AddInstanceReferencedObjects(FUPyWrapperBase* Instance, FReferenceCollector& Collector)
// {
// 	FUPyWrapperArray* Self = static_cast<FUPyWrapperArray*>(Instance);
// 	if (Self->ArrayProp && Self->ArrayInstance && !Self->OwnerContext.HasOwner())
// 	{
// 		Self->ArrayProp.AddReferencedObjects(Collector);
// 		FPyReferenceCollector::AddReferencedObjectsFromProperty(Collector, Self->ArrayProp, Self->ArrayInstance);
// 	}
// }
