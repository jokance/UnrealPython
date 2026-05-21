
#include "Wrapper/UPyWrapperFixedArray.h"
#include "Wrapper/UPyWrapperTypeRegistry.h"
#include "Wrapper/UPyWrapperTypeFactory.h"
#include "Core/UPyConversion.h"
#include "UObject/UnrealType.h"
#include "UObject/Package.h"
#include "UObject/PropertyPortFlags.h"

extern PyMethodDef FixedArrayPyMethodDefs[];

PyTypeObject UPyWrapperFixedArrayType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"FixedArray", /* tp_name */
	sizeof(FUPyWrapperFixedArray), /* tp_basicsize */
};

/** Iterator used with fixed-arrays */
struct FUPyWrapperFixedArrayIterator
{
	/** Common Python Object */
	PyObject_HEAD

	/** Instance being iterated over */
	FUPyWrapperFixedArray* IterInstance;

	/** Current iteration index */
	int32 IterIndex;

	/** New this iterator instance (called via tp_new for Python, or directly in C++) */
	static FUPyWrapperFixedArrayIterator* New(PyTypeObject* InType)
	{
		FUPyWrapperFixedArrayIterator* Self = (FUPyWrapperFixedArrayIterator*)InType->tp_alloc(InType, 0);
		if (Self)
		{
			Self->IterInstance = nullptr;
			Self->IterIndex = 0;
		}
		return Self;
	}

	/** Free this iterator instance (called via tp_dealloc for Python) */
	static void Free(FUPyWrapperFixedArrayIterator* InSelf)
	{
		Deinit(InSelf);
		Py_TYPE(InSelf)->tp_free((PyObject*)InSelf);
	}

	/** Initialize this iterator instance (called via tp_init for Python, or directly in C++) */
	static int Init(FUPyWrapperFixedArrayIterator* InSelf, FUPyWrapperFixedArray* InInstance)
	{
		Deinit(InSelf);

		Py_INCREF(InInstance);
		InSelf->IterInstance = InInstance;
		InSelf->IterIndex = 0;

		return 0;
	}

	/** Deinitialize this iterator instance (called via Init and Free to restore the instance to its New state) */
	static void Deinit(FUPyWrapperFixedArrayIterator* InSelf)
	{
		Py_XDECREF(InSelf->IterInstance);
		InSelf->IterInstance = nullptr;
		InSelf->IterIndex = 0;
	}

	/** Called to validate the internal state of this iterator instance prior to operating on it (should be called by all functions that expect to operate on an initialized type; will set an error state on failure) */
	static bool ValidateInternalState(FUPyWrapperFixedArrayIterator* InSelf)
	{
		if (!InSelf->IterInstance)
		{
			UPyUtil::SetPythonError(PyExc_Exception, Py_TYPE(InSelf), TEXT("Internal Error - IterInstance is null!"));
			return false;
		}

		return true;
	}

	/** Get the iterator */
	static FUPyWrapperFixedArrayIterator* GetIter(FUPyWrapperFixedArrayIterator* InSelf)
	{
		Py_INCREF(InSelf);
		return InSelf;
	}

	/** Return the current value (if any) and advance the iterator */
	static PyObject* IterNext(FUPyWrapperFixedArrayIterator* InSelf)
	{
		if (!ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		if (InSelf->IterIndex < FUPyWrapperFixedArray::Len(InSelf->IterInstance))
		{
			return FUPyWrapperFixedArray::GetItem(InSelf->IterInstance, InSelf->IterIndex++);
		}

		PyErr_SetObject(PyExc_StopIteration, Py_None);
		return nullptr;
	}
};

/** Python type for FUPyWrapperFixedArrayIterator */
PyTypeObject UPyWrapperFixedArrayIteratorType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"FixedArrayIterator", /* tp_name */
	sizeof(FUPyWrapperFixedArrayIterator), /* tp_basicsize */
};

FUPyWrapperFixedArray* FUPyWrapperFixedArray::New(PyTypeObject* InType)
{
	FUPyWrapperFixedArray* Self = (FUPyWrapperFixedArray*)FUPyWrapperBase::New(InType);
	if (Self)
	{
		new(&Self->OwnerContext) FUPyWrapperOwnerContext();
		new(&Self->ArrayProp) UPyUtil::FConstPropOnScope();
		Self->ArrayInstance = nullptr;
	}
	return Self;
}

void FUPyWrapperFixedArray::Free(FUPyWrapperFixedArray* InSelf)
{
	Deinit(InSelf);

	InSelf->OwnerContext.~FUPyWrapperOwnerContext();
	InSelf->ArrayProp.~TPropOnScope();
	FUPyWrapperBase::Free(InSelf);
}

int FUPyWrapperFixedArray::Init(FUPyWrapperFixedArray* InSelf, const UPyUtil::FPropertyDef& InPropDef, const int32 InLen)
{
	Deinit(InSelf);

	const int BaseInit = FUPyWrapperBase::Init(InSelf);
	if (BaseInit != 0)
	{
		return BaseInit;
	}

	UPyUtil::FConstPropOnScope ArrayProp = UPyUtil::FConstPropOnScope::OwnedReference(UPyUtil::CreateProperty(InPropDef, InLen));
	if (!ArrayProp)
	{
		UPyUtil::SetPythonError(PyExc_Exception, InSelf, TEXT("Array property was null during init"));
		return -1;
	}

	void* ArrayValue = FMemory::Malloc(ArrayProp->GetSize(), ArrayProp->GetMinAlignment());
	ArrayProp->InitializeValue(ArrayValue);

	InSelf->ArrayProp = MoveTemp(ArrayProp);
	InSelf->ArrayInstance = ArrayValue;

	// FUPyWrapperFixedArrayFactory::Get().MapInstance(InSelf->ArrayInstance, InSelf);
	return 0;
}

int FUPyWrapperFixedArray::Init(FUPyWrapperFixedArray* InSelf, const FUPyWrapperOwnerContext& InOwnerContext, const FProperty* InProp, void* InValue, const EUPyConversionMethod InConversionMethod)
{
	InOwnerContext.AssertValidConversionMethod(InConversionMethod);

	Deinit(InSelf);

	const int BaseInit = FUPyWrapperBase::Init(InSelf);
	if (BaseInit != 0)
	{
		return BaseInit;
	}

	check(InProp && InValue);

	UPyUtil::FConstPropOnScope PropToUse;
	void* ArrayInstanceToUse = nullptr;
	switch (InConversionMethod)
	{
	case EUPyConversionMethod::Copy:
	case EUPyConversionMethod::Steal:
		{
			PropToUse = UPyUtil::FConstPropOnScope::OwnedReference(UPyUtil::CreateProperty(InProp));
			if (!PropToUse)
			{
				UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to create property from '%s' (%s)"), *InProp->GetName(), *InProp->GetClass()->GetName()));
				return -1;
			}

			ArrayInstanceToUse = FMemory::Malloc(PropToUse->GetSize(), PropToUse->GetMinAlignment());
			PropToUse->InitializeValue(ArrayInstanceToUse);
			PropToUse->CopyCompleteValue(ArrayInstanceToUse, InValue);
		}
		break;

	case EUPyConversionMethod::Reference:
		{
			PropToUse = UPyUtil::FConstPropOnScope::ExternalReference(InProp);
			ArrayInstanceToUse = InValue;
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
		FUPyWrapperFixedArrayFactory::Get().MapInstance(InSelf->ArrayInstance, InSelf);
	}
	return 0;
}

void FUPyWrapperFixedArray::Deinit(FUPyWrapperFixedArray* InSelf)
{
	if (InSelf->ArrayInstance)
	{
		FUPyWrapperFixedArrayFactory::Get().UnmapInstance(InSelf->ArrayInstance);
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
	InSelf->ArrayProp.Release();
	InSelf->ArrayInstance = nullptr;
}

bool FUPyWrapperFixedArray::ValidateInternalState(FUPyWrapperFixedArray* InSelf)
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

FUPyWrapperFixedArray* FUPyWrapperFixedArray::CastPyObject(PyObject* InPyObject, FUPyConversionResult* OutCastResult)
{
	SetOptionalUPyConversionResult(FUPyConversionResult::Failure(), OutCastResult);

	if (PyObject_IsInstance(InPyObject, (PyObject*)&UPyWrapperFixedArrayType) == 1)
	{
		SetOptionalUPyConversionResult(FUPyConversionResult::Success(), OutCastResult);

		Py_INCREF(InPyObject);
		return (FUPyWrapperFixedArray*)InPyObject;
	}

	return nullptr;
}

FUPyWrapperFixedArray* FUPyWrapperFixedArray::CastPyObject(PyObject* InPyObject, PyTypeObject* InType, const UPyUtil::FPropertyDef& InPropDef, FUPyConversionResult* OutCastResult)
{
	SetOptionalUPyConversionResult(FUPyConversionResult::Failure(), OutCastResult);

	if (PyObject_IsInstance(InPyObject, (PyObject*)InType) == 1 && (InType == &UPyWrapperFixedArrayType || PyObject_IsInstance(InPyObject, (PyObject*)&UPyWrapperFixedArrayType) == 1))
	{
		FUPyWrapperFixedArray* Self = (FUPyWrapperFixedArray*)InPyObject;
		if (!ValidateInternalState(Self))
		{
			return nullptr;
		}

		const UPyUtil::FPropertyDef SelfPropDef = Self->ArrayProp.Get();
		if (SelfPropDef == InPropDef)
		{
			SetOptionalUPyConversionResult(FUPyConversionResult::Success(), OutCastResult);

			Py_INCREF(Self);
			return Self;
		}

		FUPyWrapperFixedArrayPtr NewArray = FUPyWrapperFixedArrayPtr::StealReference(FUPyWrapperFixedArray::New(InType));
		if (FUPyWrapperFixedArray::Init(NewArray, InPropDef, Self->ArrayProp->ArrayDim) != 0)
		{
			return nullptr;
		}

		// Attempt to convert the entries in the array to the native format of the new array
		FString ExportedEntry;
		for (int32 ArrIndex = 0; ArrIndex < NewArray->ArrayProp->ArrayDim; ++ArrIndex)
		{
			ExportedEntry.Reset();
			if (!Self->ArrayProp->ExportText_Direct(ExportedEntry, GetItemPtr(Self, ArrIndex), GetItemPtr(Self, ArrIndex), nullptr, PPF_None))
			{
				UPyUtil::SetPythonError(PyExc_Exception, Self, *FString::Printf(TEXT("Failed to export text for property '%s' (%s) at index %d"), *Self->ArrayProp->GetName(), *Self->ArrayProp->GetClass()->GetName(), ArrIndex));
				return nullptr;
			}

			if (!NewArray->ArrayProp->ImportText_Direct(*ExportedEntry, GetItemPtr(NewArray, ArrIndex), nullptr, PPF_None))
			{
				UPyUtil::SetPythonError(PyExc_Exception, Self, *FString::Printf(TEXT("Failed to import text '%s' for property '%s' (%s) at index %d"), *ExportedEntry, *NewArray->ArrayProp->GetName(), *NewArray->ArrayProp->GetClass()->GetName(), ArrIndex));
				return nullptr;
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
				FUPyWrapperFixedArrayPtr NewArray = FUPyWrapperFixedArrayPtr::StealReference(FUPyWrapperFixedArray::New(InType));
				if (FUPyWrapperFixedArray::Init(NewArray, InPropDef, SequenceLen) != 0)
				{
					return nullptr;
				}

				for (Py_ssize_t SequenceIndex = 0; SequenceIndex < SequenceLen; ++SequenceIndex)
				{
					FUPyObjectPtr SequenceItem = FUPyObjectPtr::StealReference(PyIter_Next(PyObjIter));
					if (!SequenceItem)
					{
						return nullptr;
					}

					if (!UPyConversion::NativizeProperty_Direct(SequenceItem, NewArray->ArrayProp, GetItemPtr(NewArray, SequenceIndex)))
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

void* FUPyWrapperFixedArray::GetItemPtr(FUPyWrapperFixedArray* InSelf, Py_ssize_t InIndex)
{
	check(ValidateInternalState(InSelf));
	check(InIndex >= 0 && InIndex < InSelf->ArrayProp->ArrayDim);

	// This doesn't use ContainerPtrToValuePtr as the ArrayInstance has already been adjusted to
	// point to the first element in the array and we just need to adjust it for the element size
	return static_cast<uint8*>(InSelf->ArrayInstance) + (InSelf->ArrayProp->GetElementSize() * InIndex);
}

Py_ssize_t FUPyWrapperFixedArray::Len(FUPyWrapperFixedArray* InSelf)
{
	if (!ValidateInternalState(InSelf))
	{
		return -1;
	}

	return InSelf->ArrayProp->ArrayDim;
}

PyObject* FUPyWrapperFixedArray::GetItem(FUPyWrapperFixedArray* InSelf, Py_ssize_t InIndex)
{
	if (!ValidateInternalState(InSelf))
	{
		return nullptr;
	}

	const Py_ssize_t ResolvedIndex = UPyUtil::ResolveContainerIndexParam(InIndex, InSelf->ArrayProp->ArrayDim);
	if (UPyUtil::ValidateContainerIndexParam(ResolvedIndex, InSelf->ArrayProp->ArrayDim, InSelf->ArrayProp, *UPyUtil::GetErrorContext(InSelf)) != 0)
	{
		return nullptr;
	}

	PyObject* PyItemObj = nullptr;
	if (!UPyConversion::PythonizeProperty_Direct(InSelf->ArrayProp, GetItemPtr(InSelf, ResolvedIndex), PyItemObj))
	{
		UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert property '%s' (%s) at index %zd"), *InSelf->ArrayProp->GetName(), *InSelf->ArrayProp->GetClass()->GetName(), ResolvedIndex));
		return nullptr;
	}
	return PyItemObj;
}

int FUPyWrapperFixedArray::SetItem(FUPyWrapperFixedArray* InSelf, Py_ssize_t InIndex, PyObject* InValue)
{
	if (!ValidateInternalState(InSelf))
	{
		return -1;
	}

	const Py_ssize_t ResolvedIndex = UPyUtil::ResolveContainerIndexParam(InIndex, InSelf->ArrayProp->ArrayDim);
	const int ValidateIndexResult = UPyUtil::ValidateContainerIndexParam(ResolvedIndex, InSelf->ArrayProp->ArrayDim, InSelf->ArrayProp, *UPyUtil::GetErrorContext(InSelf));
	if (ValidateIndexResult != 0)
	{
		return ValidateIndexResult;
	}

	if (!UPyConversion::NativizeProperty_Direct(InValue, InSelf->ArrayProp, GetItemPtr(InSelf, ResolvedIndex)))
	{
		UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert property '%s' (%s) at index %zd"), *InSelf->ArrayProp->GetName(), *InSelf->ArrayProp->GetClass()->GetName(), ResolvedIndex));
		return -1;
	}

	return 0;
}

int FUPyWrapperFixedArray::Contains(FUPyWrapperFixedArray* InSelf, PyObject* InValue)
{
	if (!ValidateInternalState(InSelf))
	{
		return -1;
	}

	UPyUtil::FFixedArrayElementOnScope ContainerEntryValue(InSelf->ArrayProp);
	if (!ContainerEntryValue.IsValid())
	{
		return -1;
	}

	if (!ContainerEntryValue.SetValue(InValue, *UPyUtil::GetErrorContext(InSelf)))
	{
		return -1;
	}

	for (int32 ArrIndex = 0; ArrIndex < InSelf->ArrayProp->ArrayDim; ++ArrIndex)
	{
		if (ContainerEntryValue.GetProp()->Identical(GetItemPtr(InSelf, ArrIndex), ContainerEntryValue.GetValue()))
		{
			return 1;
		}
	}

	return 0;
}

FUPyWrapperFixedArray* FUPyWrapperFixedArray::Concat(FUPyWrapperFixedArray* InSelf, PyObject* InOther)
{
	if (!ValidateInternalState(InSelf))
	{
		return nullptr;
	}

	const UPyUtil::FPropertyDef SelfPropDef = InSelf->ArrayProp.Get();
	FUPyWrapperFixedArrayPtr Other = FUPyWrapperFixedArrayPtr::StealReference(FUPyWrapperFixedArray::CastPyObject(InOther, &UPyWrapperFixedArrayType, SelfPropDef));
	if (!Other)
	{
		UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Cannot concatenate types '%s' and '%s' together"), *UPyUtil::GetFriendlyTypename(InSelf), *UPyUtil::GetFriendlyTypename(InOther)));
		return nullptr;
	}

	const int32 NewArrayLen = InSelf->ArrayProp->ArrayDim + Other->ArrayProp->ArrayDim;
	FUPyWrapperFixedArrayPtr NewArray = FUPyWrapperFixedArrayPtr::StealReference(FUPyWrapperFixedArray::New(Py_TYPE(InSelf)));
	if (FUPyWrapperFixedArray::Init(NewArray, SelfPropDef, NewArrayLen) != 0)
	{
		return nullptr;
	}

	int32 NewArrayIndex = 0;
	for (int32 ArrIndex = 0; ArrIndex < InSelf->ArrayProp->ArrayDim; ++ArrIndex, ++NewArrayIndex)
	{
		InSelf->ArrayProp->CopySingleValue(GetItemPtr(NewArray, NewArrayIndex), GetItemPtr(InSelf, ArrIndex));
	}
	for (int32 ArrIndex = 0; ArrIndex < Other->ArrayProp->ArrayDim; ++ArrIndex, ++NewArrayIndex)
	{
		Other->ArrayProp->CopySingleValue(GetItemPtr(NewArray, NewArrayIndex), GetItemPtr(Other, ArrIndex));
	}
	return NewArray.Release();
}

FUPyWrapperFixedArray* FUPyWrapperFixedArray::Repeat(FUPyWrapperFixedArray* InSelf, Py_ssize_t InMultiplier)
{
	if (!ValidateInternalState(InSelf))
	{
		return nullptr;
	}

	if (InMultiplier <= 0)
	{
		UPyUtil::SetPythonError(PyExc_Exception, InSelf, TEXT("Multiplier must be greater than zero"));
		return nullptr;
	}

	const UPyUtil::FPropertyDef SelfPropDef = InSelf->ArrayProp.Get();
	const int32 NewArrayLen = InSelf->ArrayProp->ArrayDim * InMultiplier;
	FUPyWrapperFixedArrayPtr NewArray = FUPyWrapperFixedArrayPtr::StealReference(FUPyWrapperFixedArray::New(Py_TYPE(InSelf)));
	if (FUPyWrapperFixedArray::Init(NewArray, SelfPropDef, NewArrayLen) != 0)
	{
		return nullptr;
	}

	int32 NewArrayIndex = 0;
	for (int32 MultipleIndex = 0; MultipleIndex < (int32)InMultiplier; ++MultipleIndex)
	{
		for (int32 ArrIndex = 0; ArrIndex < InSelf->ArrayProp->ArrayDim; ++ArrIndex, ++NewArrayIndex)
		{
			InSelf->ArrayProp->CopySingleValue(GetItemPtr(NewArray, NewArrayIndex), GetItemPtr(InSelf, ArrIndex));
		}
	}
	return NewArray.Release();
}

// ==================== Wrapper FixedArray iterator type BEGIN ====================
struct FFuncs_WrapperFixedArrayIterator
{
	static PyObject* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
	{
		return (PyObject*)FUPyWrapperFixedArrayIterator::New(InType);
	}

	static void Dealloc(FUPyWrapperFixedArrayIterator* InSelf)
	{
		FUPyWrapperFixedArrayIterator::Free(InSelf);
	}

	static int Init(FUPyWrapperFixedArrayIterator* InSelf, PyObject* InArgs, PyObject* InKwds)
	{
		PyObject* PyObj = nullptr;
		if (!PyArg_ParseTuple(InArgs, "O:call", &PyObj))
		{
			return -1;
		}

		if (PyObject_IsInstance(PyObj, (PyObject*)&UPyWrapperFixedArrayType) != 1)
		{
			UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Cannot initialize '%s' with an instance of '%s'"), *UPyUtil::GetFriendlyTypename(InSelf), *UPyUtil::GetFriendlyTypename(PyObj)));
			return -1;
		}

		return FUPyWrapperFixedArrayIterator::Init(InSelf, (FUPyWrapperFixedArray*)PyObj);
	}

	static FUPyWrapperFixedArrayIterator* GetIter(FUPyWrapperFixedArrayIterator* InSelf)
	{
		return FUPyWrapperFixedArrayIterator::GetIter(InSelf);
	}

	static PyObject* IterNext(FUPyWrapperFixedArrayIterator* InSelf)
	{
		return FUPyWrapperFixedArrayIterator::IterNext(InSelf);
	}
};

void InitializeUPyWrapperFixedArrayIteratorType()
{
	PyTypeObject* PyType = &UPyWrapperFixedArrayIteratorType;

	PyType->tp_new = (newfunc)&FFuncs_WrapperFixedArrayIterator::New;
	PyType->tp_dealloc = (destructor)&FFuncs_WrapperFixedArrayIterator::Dealloc;
	PyType->tp_init = (initproc)&FFuncs_WrapperFixedArrayIterator::Init;
	PyType->tp_iter = (getiterfunc)&FFuncs_WrapperFixedArrayIterator::GetIter;
	PyType->tp_iternext = (iternextfunc)&FFuncs_WrapperFixedArrayIterator::IterNext;

	PyType->tp_flags = Py_TPFLAGS_DEFAULT;
	PyType->tp_doc = "Type for all Unreal exposed fixed-array iterators";

	PyType_Ready(PyType);
}
// ==================== Wrapper FixedArray iterator type END ====================

// ==================== Wrapper FixedArray type BEGIN ====================

struct FFuncs_WrapperFixedArray
{
	static PyObject* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
	{
		return (PyObject*)FUPyWrapperFixedArray::New(InType);
	}

	static void Dealloc(FUPyWrapperFixedArray* InSelf)
	{
		FUPyWrapperFixedArray::Free(InSelf);
	}

	static int Init(FUPyWrapperFixedArray* InSelf, PyObject* InArgs, PyObject* InKwds)
	{
		PyObject* PyTypeObj = nullptr;
		PyObject* PyLenObj = nullptr;

		static const char *ArgsKwdList[] = { "type", "len", nullptr };
		if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "OO:call", (char**)ArgsKwdList, &PyTypeObj, &PyLenObj))
		{
			return -1;
		}

		UPyUtil::FPropertyDef ArrayPropDef;
		const int ValidateTypeResult = UPyUtil::ValidateContainerTypeParam(PyTypeObj, ArrayPropDef, "type", *UPyUtil::GetErrorContext(Py_TYPE(InSelf)));
		if (ValidateTypeResult != 0)
		{
			return ValidateTypeResult;
		}

		int32 ArrayLen = 0;
		const int ValidateLenResult = UPyUtil::ValidateContainerLenParam(PyLenObj, ArrayLen, "len", *UPyUtil::GetErrorContext(Py_TYPE(InSelf)));
		if (ValidateLenResult != 0)
		{
			return ValidateLenResult;
		}

		ArrayLen = FMath::Max(ArrayLen, 1);
		return FUPyWrapperFixedArray::Init(InSelf, ArrayPropDef, ArrayLen);
	}

	static PyObject* Str(FUPyWrapperFixedArray* InSelf)
	{
		if (!FUPyWrapperFixedArray::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		FString ExportedArray;
		for (int32 ArrayIndex = 0; ArrayIndex < InSelf->ArrayProp->ArrayDim; ++ArrayIndex)
		{
			if (ArrayIndex > 0)
			{
				ExportedArray += TEXT(", ");
			}
			ExportedArray += UPyUtil::GetFriendlyPropertyValue(InSelf->ArrayProp, FUPyWrapperFixedArray::GetItemPtr(InSelf, ArrayIndex), PPF_Delimited | PPF_IncludeTransient);
		}
		return PyUnicode_FromFormat("[%s]", TCHAR_TO_UTF8(*ExportedArray));
	}

	static PyObject* RichCmp(FUPyWrapperFixedArray* InSelf, PyObject* InOther, int InOp)
	{
		if (!FUPyWrapperFixedArray::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		const UPyUtil::FPropertyDef SelfPropDef = InSelf->ArrayProp.Get();
		FUPyWrapperFixedArrayPtr Other = FUPyWrapperFixedArrayPtr::StealReference(FUPyWrapperFixedArray::CastPyObject(InOther, &UPyWrapperFixedArrayType, SelfPropDef));
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

		bool bIsIdentical = InSelf->ArrayProp->ArrayDim == Other->ArrayProp->ArrayDim;
		for (int32 ArrayIndex = 0; bIsIdentical && ArrayIndex < InSelf->ArrayProp->ArrayDim; ++ArrayIndex)
		{
			bIsIdentical &= InSelf->ArrayProp->Identical_InContainer(InSelf->ArrayInstance, Other->ArrayInstance, ArrayIndex, PPF_None);
		}

		return PyBool_FromLong(InOp == Py_EQ ? bIsIdentical : !bIsIdentical);
	}

	static UPyUtil::FPyHashType Hash(FUPyWrapperFixedArray* InSelf)
	{
		if (!FUPyWrapperFixedArray::ValidateInternalState(InSelf))
		{
			return -1;
		}

		if (InSelf->ArrayProp->HasAnyPropertyFlags(CPF_HasGetValueTypeHash))
		{
			uint32 ArrayHash = 0;
			for (int32 ArrayIndex = 0; ArrayIndex < InSelf->ArrayProp->ArrayDim; ++ArrayIndex)
			{
				ArrayHash = HashCombine(ArrayHash, InSelf->ArrayProp->GetValueTypeHash(FUPyWrapperFixedArray::GetItemPtr(InSelf, ArrayIndex)));
			}
			return (UPyUtil::FPyHashType)(ArrayHash != -1 ? ArrayHash : 0);
		}

		UPyUtil::SetPythonError(PyExc_Exception, InSelf, TEXT("Type cannot be hashed"));
		return -1;
	}

	static PyObject* GetIter(FUPyWrapperFixedArray* InSelf)
	{
		typedef TUPyPtr<FUPyWrapperFixedArrayIterator> FPyWrapperFixedArrayIteratorPtr;

		if (!FUPyWrapperFixedArray::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		FPyWrapperFixedArrayIteratorPtr NewIter = FPyWrapperFixedArrayIteratorPtr::StealReference(FUPyWrapperFixedArrayIterator::New(&UPyWrapperFixedArrayIteratorType));
		if (FUPyWrapperFixedArrayIterator::Init(NewIter, InSelf) != 0)
		{
			return nullptr;
		}

		return (PyObject*)NewIter.Release();
	}
};

struct FSequenceFuncs_WrapperFixedArray
{
	static Py_ssize_t Len(FUPyWrapperFixedArray* InSelf)
	{
		return FUPyWrapperFixedArray::Len(InSelf);
	}

	static PyObject* GetItem(FUPyWrapperFixedArray* InSelf, Py_ssize_t InIndex)
	{
		return FUPyWrapperFixedArray::GetItem(InSelf, InIndex);
	}

	static int SetItem(FUPyWrapperFixedArray* InSelf, Py_ssize_t InIndex, PyObject* InValue)
	{
		return FUPyWrapperFixedArray::SetItem(InSelf, InIndex, InValue);
	}

	static int Contains(FUPyWrapperFixedArray* InSelf, PyObject* InValue)
	{
		return FUPyWrapperFixedArray::Contains(InSelf, InValue);
	}

	static PyObject* Concat(FUPyWrapperFixedArray* InSelf, PyObject* InOther)
	{
		return (PyObject*)FUPyWrapperFixedArray::Concat(InSelf, InOther);
	}

	static PyObject* Repeat(FUPyWrapperFixedArray* InSelf, Py_ssize_t InMultiplier)
	{
		return (PyObject*)FUPyWrapperFixedArray::Repeat(InSelf, InMultiplier);
	}

	static PyObject* GetSlice(FUPyWrapperFixedArray* InSelf, Py_ssize_t InSliceStart, Py_ssize_t InSliceStop)
	{
		if (!FUPyWrapperFixedArray::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		const Py_ssize_t ResolvedSliceStart = UPyUtil::ResolveContainerIndexParam(InSliceStart, InSelf->ArrayProp->ArrayDim);
		const Py_ssize_t ResolvedSliceStop = UPyUtil::ResolveContainerIndexParam(InSliceStop, InSelf->ArrayProp->ArrayDim);
		const Py_ssize_t SliceLen = ResolvedSliceStop - ResolvedSliceStart;
		if (SliceLen <= 0)
		{
			UPyUtil::SetPythonError(PyExc_Exception, InSelf, TEXT("Slice length must be greater than zero"));
			return nullptr;
		}

		const UPyUtil::FPropertyDef SelfPropDef = InSelf->ArrayProp.Get();
		FUPyWrapperFixedArrayPtr NewArray = FUPyWrapperFixedArrayPtr::StealReference(FUPyWrapperFixedArray::New(Py_TYPE(InSelf)));
		if (FUPyWrapperFixedArray::Init(NewArray, SelfPropDef, SliceLen) != 0)
		{
			return nullptr;
		}

		for (Py_ssize_t ArrIndex = ResolvedSliceStart; ArrIndex < ResolvedSliceStop; ++ArrIndex)
		{
			InSelf->ArrayProp->CopySingleValue(FUPyWrapperFixedArray::GetItemPtr(NewArray, ArrIndex - ResolvedSliceStart), FUPyWrapperFixedArray::GetItemPtr(InSelf, ArrIndex));
		}
		return (PyObject*)NewArray.Release();
	}
};

struct FMappingFuncs_WrapperFixedArray
{
	static PyObject* GetItem(FUPyWrapperFixedArray* InSelf, PyObject* InIndexer)
	{
		if (!FUPyWrapperFixedArray::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		// Array types support numeric and slice based indexing
		int32 Index = 0;
		if (UPyConversion::Nativize(InIndexer, Index, UPyConversion::ESetErrorState::No))
		{
			return FUPyWrapperFixedArray::GetItem(InSelf, (Py_ssize_t)Index);
		}

		if (PySlice_Check(InIndexer))
		{
			Py_ssize_t SliceStart = 0;
			Py_ssize_t SliceStop = 0;
			Py_ssize_t SliceStep = 0;
			Py_ssize_t SliceLen = 0;
			if (PySlice_GetIndicesEx((UPyUtil::FPySliceType*)InIndexer, FUPyWrapperFixedArray::Len(InSelf), &SliceStart, &SliceStop, &SliceStep, &SliceLen) != 0)
			{
				return nullptr;
			}
			if (SliceStep == 1)
			{
				return FSequenceFuncs_WrapperFixedArray::GetSlice(InSelf, SliceStart, SliceStop);
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
};

void InitializeUPyWrapperFixedArray(UPyGenUtil::FNativePythonModule& ModuleInfo)
{
	PyTypeObject* PyType = &UPyWrapperFixedArrayType;

	PyType->tp_base = &UPyWrapperBaseType;
	PyType->tp_new = (newfunc)&FFuncs_WrapperFixedArray::New;
	PyType->tp_dealloc = (destructor)&FFuncs_WrapperFixedArray::Dealloc;
	PyType->tp_init = (initproc)&FFuncs_WrapperFixedArray::Init;
	PyType->tp_str = (reprfunc)&FFuncs_WrapperFixedArray::Str;
	PyType->tp_richcompare = (richcmpfunc)&FFuncs_WrapperFixedArray::RichCmp;
	PyType->tp_hash = (hashfunc)&FFuncs_WrapperFixedArray::Hash;
	PyType->tp_iter = (getiterfunc)&FFuncs_WrapperFixedArray::GetIter;

	PyType->tp_methods = FixedArrayPyMethodDefs;

	PyType->tp_flags = Py_TPFLAGS_DEFAULT;
	PyType->tp_doc = "Type for all Unreal exposed fixed-array instances";

	static PySequenceMethods PySequence;
	PySequence.sq_length = (lenfunc)&FSequenceFuncs_WrapperFixedArray::Len;
	PySequence.sq_item = (ssizeargfunc)&FSequenceFuncs_WrapperFixedArray::GetItem;
	PySequence.sq_ass_item = (ssizeobjargproc)&FSequenceFuncs_WrapperFixedArray::SetItem;
	PySequence.sq_contains = (objobjproc)&FSequenceFuncs_WrapperFixedArray::Contains;
	PySequence.sq_concat = (binaryfunc)&FSequenceFuncs_WrapperFixedArray::Concat;
	PySequence.sq_repeat = (ssizeargfunc)&FSequenceFuncs_WrapperFixedArray::Repeat;

	static PyMappingMethods PyMapping;
	PyMapping.mp_subscript = (binaryfunc)&FMappingFuncs_WrapperFixedArray::GetItem;

	PyType->tp_as_sequence = &PySequence;
	PyType->tp_as_mapping = &PyMapping;

	if (PyType_Ready(PyType) == 0)
	{
		ModuleInfo.AddType(PyType, true);
	}

	InitializeUPyWrapperFixedArrayIteratorType();
}
// ==================== Wrapper FixedArray type END ====================
