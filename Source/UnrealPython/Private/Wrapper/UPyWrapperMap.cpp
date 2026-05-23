
#include "Wrapper/UPyWrapperMap.h"
#include "Wrapper/UPyWrapperTypeRegistry.h"
#include "Wrapper/UPyWrapperTypeFactory.h"
#include "Core/UPyConversion.h"
#include "UObject/UnrealType.h"
#include "UObject/Package.h"
#include "UObject/PropertyPortFlags.h"

extern PyMethodDef MapPyMethodDefs[];

/** Python type for FUPyWrapperMapItemIterator */
extern PyTypeObject PyWrapperMapItemIteratorType;

/** Python type for FPyWrapperMapKeyIterator */
extern PyTypeObject PyWrapperMapKeyIteratorType;

/** Python type for FPyWrapperMapValueIterator */
extern PyTypeObject PyWrapperMapValueIteratorType;

/** Python type for FPyWrapperMapItemView */
extern PyTypeObject PyWrapperMapItemViewType;

/** Python type for FPyWrapperMapKeyView */
extern PyTypeObject PyWrapperMapKeyViewType;

/** Python type for FPyWrapperMapValueView */
extern PyTypeObject PyWrapperMapValueViewType;

PyTypeObject UPyWrapperMapType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"Map", /* tp_name */
	sizeof(FUPyWrapperMap), /* tp_basicsize */
};

/** Helper function to make a new map iterator */
template <typename TImpl>
PyObject* MakeMapIter(FUPyWrapperMap* InSelf)
{
	typedef TUPyPtr<TImpl> FMapIteratorPtr;

	if (!FUPyWrapperMap::ValidateInternalState(InSelf))
	{
		return nullptr;
	}

	FMapIteratorPtr NewIter = FMapIteratorPtr::StealReference(TImpl::New(TImpl::GetType()));
	if (TImpl::Init(NewIter, InSelf) != 0)
	{
		return nullptr;
	}

	return (PyObject*)NewIter.Release();
}

/** Helper function to make a new Python list from a map iterator */
template <typename TImpl>
PyObject* MakeListFromMapIter(FUPyWrapperMap* InSelf)
{
	FUPyObjectPtr IterObj = FUPyObjectPtr::StealReference(MakeMapIter<TImpl>(InSelf));
	if (!IterObj)
	{
		return nullptr;
	}

	const Py_ssize_t MapLen = FUPyWrapperMap::Len(InSelf);
	FUPyObjectPtr ListObj = FUPyObjectPtr::StealReference(PyList_New(MapLen));

	for (Py_ssize_t MapIndex = 0; MapIndex < MapLen; ++MapIndex)
	{
		PyObject* MapItem = PyIter_Next(IterObj);
		if (!MapItem)
		{
			return nullptr;
		}

		PyList_SetItem(ListObj, MapIndex, MapItem); // PyList_SetItem steals the new reference returned by PyIter_Next
	}

	return ListObj.Release();
}

/** Helper function to make a new map view */
template <typename TImpl>
PyObject* MakeMapView(FUPyWrapperMap* InSelf)
{
	typedef TUPyPtr<TImpl> FMapViewPtr;

	if (!FUPyWrapperMap::ValidateInternalState(InSelf))
	{
		return nullptr;
	}

	FMapViewPtr NewView = FMapViewPtr::StealReference(TImpl::New(TImpl::GetType()));
	if (TImpl::Init(NewView, InSelf) != 0)
	{
		return nullptr;
	}

	return (PyObject*)NewView.Release();
}

/** Iterator used with maps */
template <typename TImpl>
struct TUPyWrapperMapIterator
{
	/** Common Python Object */
	PyObject_HEAD

	/** Instance being iterated over */
	FUPyWrapperMap* IterInstance;

	/** Current iteration index */
	int32 IterIndex;

	/** New this iterator instance (called via tp_new for Python, or directly in C++) */
	static TImpl* New(PyTypeObject* InType)
	{
		TImpl* Self = (TImpl*)InType->tp_alloc(InType, 0);
		if (Self)
		{
			Self->IterInstance = nullptr;
			Self->IterIndex = 0;
		}
		return Self;
	}

	/** Free this iterator instance (called via tp_dealloc for Python) */
	static void Free(TImpl* InSelf)
	{
		Deinit(InSelf);
		Py_TYPE(InSelf)->tp_free((PyObject*)InSelf);
	}

	/** Initialize this iterator instance (called via tp_init for Python, or directly in C++) */
	static int Init(TImpl* InSelf, FUPyWrapperMap* InInstance)
	{
		Deinit(InSelf);

		if (!FUPyWrapperMap::ValidateInternalState(InInstance))
		{
			return -1;
		}

		Py_INCREF(InInstance);
		InSelf->IterInstance = InInstance;
		InSelf->IterIndex = GetElementIndex(InSelf, 0);

		return 0;
	}

	/** Deinitialize this iterator instance (called via Init and Free to restore the instance to its New state) */
	static void Deinit(TImpl* InSelf)
	{
		Py_XDECREF(InSelf->IterInstance);
		InSelf->IterInstance = nullptr;
		InSelf->IterIndex = 0;
	}

	/** Called to validate the internal state of this iterator instance prior to operating on it (should be called by all functions that expect to operate on an initialized type; will set an error state on failure) */
	static bool ValidateInternalState(TImpl* InSelf)
	{
		if (!InSelf->IterInstance)
		{
			UPyUtil::SetPythonError(PyExc_Exception, Py_TYPE(InSelf), TEXT("Internal Error - IterInstance is null!"));
			return false;
		}

		return FUPyWrapperMap::ValidateInternalState(InSelf->IterInstance);
	}

	/** Given a sparse index, get the first element index from this point in the map (including the given index) */
	static int32 GetElementIndex(TImpl* InSelf, int32 InSparseIndex)
	{
		FScriptMapHelper ScriptMapHelper(InSelf->IterInstance->MapProp, InSelf->IterInstance->MapInstance);
		const int32 SparseCount = ScriptMapHelper.GetMaxIndex();

		int32 ElementIndex = InSparseIndex;
		for (; ElementIndex < SparseCount; ++ElementIndex)
		{
			if (ScriptMapHelper.IsValidIndex(ElementIndex))
			{
				break;
			}
		}

		return ElementIndex;
	}

	/** Get the iterator */
	static PyObject* GetIter(TImpl* InSelf)
	{
		Py_INCREF(InSelf);
		return (PyObject*)InSelf;
	}

	/** Return the current value (if any) and advance the iterator */
	static PyObject* IterNext(TImpl* InSelf)
	{
		if (!ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		FScriptMapHelper ScriptMapHelper(InSelf->IterInstance->MapProp, InSelf->IterInstance->MapInstance);
		const int32 SparseCount = ScriptMapHelper.GetMaxIndex();

		if (InSelf->IterIndex < SparseCount)
		{
			const int32 ElementIndex = InSelf->IterIndex;
			InSelf->IterIndex = GetElementIndex(InSelf, InSelf->IterIndex + 1);

			if (!ScriptMapHelper.IsValidIndex(ElementIndex))
			{
				UPyUtil::SetPythonError(PyExc_IndexError, InSelf, TEXT("Iterator was on an invalid element index! Was the map changed while iterating?"));
				return nullptr;
			}

			return TImpl::GetItem(InSelf, ScriptMapHelper, ElementIndex);
		}

		PyErr_SetObject(PyExc_StopIteration, Py_None);
		return nullptr;
	}
};

/** Iterator used with map items (key->value pairs) */
struct FUPyWrapperMapItemIterator : public TUPyWrapperMapIterator<FUPyWrapperMapItemIterator>
{
	static PyTypeObject* GetType()
	{
		return &PyWrapperMapItemIteratorType;
	}

	static PyObject* GetItem(FUPyWrapperMapItemIterator* InSelf, FScriptMapHelper& InScriptMapHelper, const int32 InValidatedInternalIndex)
	{
		FUPyObjectPtr PyKeyObj;
		if (!UPyConversion::PythonizeProperty(InScriptMapHelper.GetKeyProperty(), InScriptMapHelper.GetKeyPtr(InValidatedInternalIndex), PyKeyObj.Get()))
		{
			UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert key property '%s' (%s)"), *InScriptMapHelper.GetKeyProperty()->GetName(), *InScriptMapHelper.GetKeyProperty()->GetClass()->GetName()));
			return nullptr;
		}

		FUPyObjectPtr PyValueObj;
		if (!UPyConversion::PythonizeProperty(InScriptMapHelper.GetValueProperty(), InScriptMapHelper.GetValuePtr(InValidatedInternalIndex), PyValueObj.Get()))
		{
			UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert value property '%s' (%s)"), *InScriptMapHelper.GetValueProperty()->GetName(), *InScriptMapHelper.GetValueProperty()->GetClass()->GetName()));
			return nullptr;
		}

		return PyTuple_Pack(2, PyKeyObj.Get(), PyValueObj.Get());
	}
};

/** Iterator used with map keys */
struct FPyWrapperMapKeyIterator : public TUPyWrapperMapIterator<FPyWrapperMapKeyIterator>
{
	static PyTypeObject* GetType()
	{
		return &PyWrapperMapKeyIteratorType;
	}

	static PyObject* GetItem(FPyWrapperMapKeyIterator* InSelf, FScriptMapHelper& InScriptMapHelper, const int32 InValidatedInternalIndex)
	{
		PyObject* PyItemObj = nullptr;
		if (!UPyConversion::PythonizeProperty(InScriptMapHelper.GetKeyProperty(), InScriptMapHelper.GetKeyPtr(InValidatedInternalIndex), PyItemObj))
		{
			UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert key property '%s' (%s)"), *InScriptMapHelper.GetKeyProperty()->GetName(), *InScriptMapHelper.GetKeyProperty()->GetClass()->GetName()));
			return nullptr;
		}
		return PyItemObj;
	}
};

/** Iterator used with map values */
struct FPyWrapperMapValueIterator : public TUPyWrapperMapIterator<FPyWrapperMapValueIterator>
{
	static PyTypeObject* GetType()
	{
		return &PyWrapperMapValueIteratorType;
	}

	static PyObject* GetItem(FPyWrapperMapValueIterator* InSelf, FScriptMapHelper& InScriptMapHelper, const int32 InValidatedInternalIndex)
	{
		PyObject* PyItemObj = nullptr;
		if (!UPyConversion::PythonizeProperty(InScriptMapHelper.GetValueProperty(), InScriptMapHelper.GetValuePtr(InValidatedInternalIndex), PyItemObj))
		{
			UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert value property '%s' (%s)"), *InScriptMapHelper.GetValueProperty()->GetName(), *InScriptMapHelper.GetValueProperty()->GetClass()->GetName()));
			return nullptr;
		}
		return PyItemObj;
	}
};

/** View used with maps */
template <typename TImpl>
struct TPyWrapperMapView
{
	/** Common Python Object */
	PyObject_HEAD

	/** Instance being viewed */
	FUPyWrapperMap* ViewInstance;

	/** New this view instance (called via tp_new for Python, or directly in C++) */
	static TImpl* New(PyTypeObject* InType)
	{
		TImpl* Self = (TImpl*)InType->tp_alloc(InType, 0);
		if (Self)
		{
			Self->ViewInstance = nullptr;
		}
		return Self;
	}

	/** Free this view instance (called via tp_dealloc for Python) */
	static void Free(TImpl* InSelf)
	{
		Deinit(InSelf);
		Py_TYPE(InSelf)->tp_free((PyObject*)InSelf);
	}

	/** Initialize this view instance (called via tp_init for Python, or directly in C++) */
	static int Init(TImpl* InSelf, FUPyWrapperMap* InInstance)
	{
		Deinit(InSelf);

		Py_INCREF(InInstance);
		InSelf->ViewInstance = InInstance;

		return 0;
	}

	/** Deinitialize this view instance (called via Init and Free to restore the instance to its New state) */
	static void Deinit(TImpl* InSelf)
	{
		Py_XDECREF(InSelf->ViewInstance);
		InSelf->ViewInstance = nullptr;
	}

	/** Called to validate the internal state of this view instance prior to operating on it (should be called by all functions that expect to operate on an initialized type; will set an error state on failure) */
	static bool ValidateInternalState(TImpl* InSelf)
	{
		if (!InSelf->ViewInstance)
		{
			UPyUtil::SetPythonError(PyExc_Exception, Py_TYPE(InSelf), TEXT("Internal Error - ViewInstance is null!"));
			return false;
		}

		return true;
	}

	/** Called to get the length of the map we're a view to */
	static Py_ssize_t Len(TImpl* InSelf)
	{
		if (!ValidateInternalState(InSelf))
		{
			return -1;
		}

		return FUPyWrapperMap::Len(InSelf->ViewInstance);
	}
};

/** View used with map items (key->value pairs) */
struct FPyWrapperMapItemView : public TPyWrapperMapView<FPyWrapperMapItemView>
{
	static PyTypeObject* GetType()
	{
		return &PyWrapperMapItemViewType;
	}

	static PyObject* GetIter(FPyWrapperMapItemView* InSelf)
	{
		if (!ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		return MakeMapIter<FUPyWrapperMapItemIterator>(InSelf->ViewInstance);
	}
};

/** View used with map keys */
struct FPyWrapperMapKeyView : public TPyWrapperMapView<FPyWrapperMapKeyView>
{
	static PyTypeObject* GetType()
	{
		return &PyWrapperMapKeyViewType;
	}

	static PyObject* GetIter(FPyWrapperMapKeyView* InSelf)
	{
		if (!ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		return MakeMapIter<FPyWrapperMapKeyIterator>(InSelf->ViewInstance);
	}
};

/** View used with map values */
struct FPyWrapperMapValueView : public TPyWrapperMapView<FPyWrapperMapValueView>
{
	static PyTypeObject* GetType()
	{
		return &PyWrapperMapValueViewType;
	}

	static PyObject* GetIter(FPyWrapperMapValueView* InSelf)
	{
		if (!ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		return MakeMapIter<FPyWrapperMapValueIterator>(InSelf->ViewInstance);
	}
};

FUPyWrapperMap* FUPyWrapperMap::New(PyTypeObject* InType)
{
	FUPyWrapperMap* Self = (FUPyWrapperMap*)FUPyWrapperBase::New(InType);
	if (Self)
	{
		new(&Self->OwnerContext) FUPyWrapperOwnerContext();
		new(&Self->MapProp) UPyUtil::FConstMapPropOnScope();
		Self->MapInstance = nullptr;
	}
	return Self;
}

void FUPyWrapperMap::Free(FUPyWrapperMap* InSelf)
{
	Deinit(InSelf);

	InSelf->OwnerContext.~FUPyWrapperOwnerContext();
	InSelf->MapProp.~TPropOnScope();
	FUPyWrapperBase::Free(InSelf);
}

int FUPyWrapperMap::Init(FUPyWrapperMap* InSelf, const UPyUtil::FPropertyDef& InKeyDef, const UPyUtil::FPropertyDef& InValueDef)
{
	Deinit(InSelf);

	const int BaseInit = FUPyWrapperBase::Init(InSelf);
	if (BaseInit != 0)
	{
		return BaseInit;
	}

	UPyUtil::FPropOnScope MapKeyProp = UPyUtil::FPropOnScope::OwnedReference(UPyUtil::CreateProperty(InKeyDef, 1));
	if (!MapKeyProp)
	{
		UPyUtil::SetPythonError(PyExc_Exception, InSelf, TEXT("Map key property was null during init"));
		return -1;
	}

	UPyUtil::FPropOnScope MapValueProp = UPyUtil::FPropOnScope::OwnedReference(UPyUtil::CreateProperty(InValueDef, 1));
	if (!MapValueProp)
	{
		UPyUtil::SetPythonError(PyExc_Exception, InSelf, TEXT("Map value property was null during init"));
		return -1;
	}

	UPyUtil::FMapPropOnScope MapProp = UPyUtil::FMapPropOnScope::OwnedReference(new FMapProperty(FFieldVariant(), UPyUtil::DefaultPythonPropertyName, RF_NoFlags));
	MapProp->KeyProp = MapKeyProp.Release();
	MapProp->ValueProp = MapValueProp.Release();

	// Need to manually call Link to fix-up some data (such as the C++ property flags and the map layout) that are only set during Link
	{
		FArchive Ar;
		MapProp->LinkWithoutChangingOffset(Ar);
	}

	void* MapValue = FMemory::Malloc(MapProp->GetSize(), MapProp->GetMinAlignment());
	MapProp->InitializeValue(MapValue);

	InSelf->MapProp = MoveTemp(MapProp);
	InSelf->MapInstance = MapValue;

	// FUPyWrapperMapFactory::Get().MapInstance(InSelf->MapInstance, InSelf);
	return 0;
}

int FUPyWrapperMap::Init(FUPyWrapperMap* InSelf, const FUPyWrapperOwnerContext& InOwnerContext, const FMapProperty* InProp, void* InValue, const EUPyConversionMethod InConversionMethod)
{
	InOwnerContext.AssertValidConversionMethod(InConversionMethod);

	Deinit(InSelf);

	const int BaseInit = FUPyWrapperBase::Init(InSelf);
	if (BaseInit != 0)
	{
		return BaseInit;
	}

	check(InProp && InValue);

	UPyUtil::FConstMapPropOnScope PropToUse;
	void* MapInstanceToUse = nullptr;
	switch (InConversionMethod)
	{
	case EUPyConversionMethod::Copy:
	case EUPyConversionMethod::Steal:
		{
			UPyUtil::FPropOnScope MapKeyProp = UPyUtil::FPropOnScope::OwnedReference(UPyUtil::CreateProperty(InProp->KeyProp, 1));
			if (!MapKeyProp)
			{
				UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to create key property from '%s' (%s)"), *InProp->KeyProp->GetName(), *InProp->KeyProp->GetClass()->GetName()));
				return -1;
			}

			UPyUtil::FPropOnScope MapValueProp = UPyUtil::FPropOnScope::OwnedReference(UPyUtil::CreateProperty(InProp->ValueProp, 1));
			if (!MapValueProp)
			{
				UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to create value property from '%s' (%s)"), *InProp->ValueProp->GetName(), *InProp->ValueProp->GetClass()->GetName()));
				return -1;
			}

			UPyUtil::FMapPropOnScope MapProp = UPyUtil::FMapPropOnScope::OwnedReference(new FMapProperty(FFieldVariant(), UPyUtil::DefaultPythonPropertyName, RF_NoFlags));
			MapProp->KeyProp = MapKeyProp.Release();
			MapProp->ValueProp = MapValueProp.Release();

			// Need to manually call Link to fix-up some data (such as the C++ property flags and the map layout) that are only set during Link
			{
				FArchive Ar;
				MapProp->LinkWithoutChangingOffset(Ar);
			}

			PropToUse = MoveTemp(MapProp);

			MapInstanceToUse = FMemory::Malloc(PropToUse->GetSize(), PropToUse->GetMinAlignment());
			PropToUse->InitializeValue(MapInstanceToUse);
			if (InConversionMethod == EUPyConversionMethod::Steal)
			{
				FScriptMapHelper SelfScriptMapHelper(PropToUse, MapInstanceToUse);
				SelfScriptMapHelper.MoveAssign(InValue);
			}
			else
			{
				PropToUse->CopyCompleteValue(MapInstanceToUse, InValue);
			}
		}
		break;

	case EUPyConversionMethod::Reference:
		{
			PropToUse = UPyUtil::FConstMapPropOnScope::ExternalReference(InProp);
			MapInstanceToUse = InValue;
			InOwnerContext.AddOwnedPyProp(InSelf);
		}
		break;

	default:
		checkf(false, TEXT("Unknown EUPyConversionMethod"));
		break;
	}

	check(PropToUse && MapInstanceToUse);

	InSelf->OwnerContext = InOwnerContext;
	InSelf->MapProp = MoveTemp(PropToUse);
	InSelf->MapInstance = MapInstanceToUse;

	// todo(hzn): 是否有必要所有的结构体都加入map
	if (InOwnerContext.HasOwner())
	{
		FUPyWrapperMapFactory::Get().MapInstance(InSelf->MapInstance, InSelf);
	}
	return 0;
}

void FUPyWrapperMap::Deinit(FUPyWrapperMap* InSelf)
{
	if (InSelf->MapInstance)
	{
		FUPyWrapperMapFactory::Get().UnmapInstance(InSelf->MapInstance);
	}

	if (InSelf->OwnerContext.HasOwner())
	{
		InSelf->OwnerContext.Reset();
	}
	else if (InSelf->MapInstance)
	{
		if (InSelf->MapProp)
		{
			InSelf->MapProp->DestroyValue(InSelf->MapInstance);
		}
		FMemory::Free(InSelf->MapInstance);
	}
	InSelf->MapProp.Reset();
	InSelf->MapInstance = nullptr;
}

bool FUPyWrapperMap::ValidateInternalState(FUPyWrapperMap* InSelf)
{
	if (!InSelf->MapProp)
	{
		UPyUtil::SetPythonError(PyExc_Exception, Py_TYPE(InSelf), TEXT("Internal Error - MapProp is null!"));
		return false;
	}

	if (!InSelf->MapInstance)
	{
		UPyUtil::SetPythonError(PyExc_Exception, Py_TYPE(InSelf), TEXT("Internal Error - MapInstance is null!"));
		return false;
	}

	return true;
}

FUPyWrapperMap* FUPyWrapperMap::CastPyObject(PyObject* InPyObject, FUPyConversionResult* OutCastResult)
{
	SetOptionalUPyConversionResult(FUPyConversionResult::Failure(), OutCastResult);

	if (PyObject_TypeCheck(InPyObject, &UPyWrapperMapType))
	{
		SetOptionalUPyConversionResult(FUPyConversionResult::Success(), OutCastResult);

		Py_INCREF(InPyObject);
		return (FUPyWrapperMap*)InPyObject;
	}

	return nullptr;
}

FUPyWrapperMap* FUPyWrapperMap::CastPyObject(PyObject* InPyObject, PyTypeObject* InType, const UPyUtil::FPropertyDef& InKeyDef, const UPyUtil::FPropertyDef& InValueDef, FUPyConversionResult* OutCastResult)
{
	SetOptionalUPyConversionResult(FUPyConversionResult::Failure(), OutCastResult);

	if (PyObject_TypeCheck(InPyObject, InType) && (InType == &UPyWrapperMapType || PyObject_TypeCheck(InPyObject, &UPyWrapperMapType)))
	{
		FUPyWrapperMap* Self = (FUPyWrapperMap*)InPyObject;

		if (!ValidateInternalState(Self))
		{
			return nullptr;
		}

		const UPyUtil::FPropertyDef SelfKeyPropDef = Self->MapProp->KeyProp;
		const UPyUtil::FPropertyDef SelfValuePropDef = Self->MapProp->ValueProp;
		if (SelfKeyPropDef == InKeyDef && SelfValuePropDef == InValueDef)
		{
			SetOptionalUPyConversionResult(FUPyConversionResult::Success(), OutCastResult);

			Py_INCREF(Self);
			return Self;
		}

		FUPyWrapperMapPtr NewMap = FUPyWrapperMapPtr::StealReference(FUPyWrapperMap::New(InType));
		if (FUPyWrapperMap::Init(NewMap, InKeyDef, InValueDef) != 0)
		{
			return nullptr;
		}

		// Attempt to convert the entries in the map to the native format of the new map
		{
			const FScriptMapHelper SelfScriptMapHelper(Self->MapProp, Self->MapInstance);
			FScriptMapHelper NewScriptMapHelper(NewMap->MapProp, NewMap->MapInstance);

			FString ExportedKey;
			FString ExportedValue;
			for (FScriptMapHelper::FIterator It(SelfScriptMapHelper); It; ++It)
			{
				ExportedKey.Reset();
				if (!SelfScriptMapHelper.GetKeyProperty()->ExportText_Direct(ExportedKey, SelfScriptMapHelper.GetKeyPtr(It), SelfScriptMapHelper.GetKeyPtr(It), nullptr, PPF_None))
				{
					UPyUtil::SetPythonError(PyExc_Exception, Self, *FString::Printf(TEXT("Failed to export text for key property '%s' (%s) at index %d"), *SelfScriptMapHelper.GetKeyProperty()->GetName(), *SelfScriptMapHelper.GetKeyProperty()->GetClass()->GetName(), It.GetLogicalIndex()));
					return nullptr;
				}

				ExportedValue.Reset();
				if (!SelfScriptMapHelper.GetValueProperty()->ExportText_Direct(ExportedValue, SelfScriptMapHelper.GetValuePtr(It), SelfScriptMapHelper.GetValuePtr(It), nullptr, PPF_None))
				{
					UPyUtil::SetPythonError(PyExc_Exception, Self, *FString::Printf(TEXT("Failed to export text for value property '%s' (%s) at index %d"), *SelfScriptMapHelper.GetValueProperty()->GetName(), *SelfScriptMapHelper.GetValueProperty()->GetClass()->GetName(), It.GetLogicalIndex()));
					return nullptr;
				}

				const int32 NewElementIndex = NewScriptMapHelper.AddDefaultValue_Invalid_NeedsRehash();
				if (!NewScriptMapHelper.GetKeyProperty()->ImportText_Direct(*ExportedKey, NewScriptMapHelper.GetKeyPtr(NewElementIndex), nullptr, PPF_None))
				{
					UPyUtil::SetPythonError(PyExc_Exception, Self, *FString::Printf(TEXT("Failed to import text '%s' key for property '%s' (%s) at index %d"), *ExportedKey, *NewScriptMapHelper.GetKeyProperty()->GetName(), *NewScriptMapHelper.GetKeyProperty()->GetClass()->GetName(), NewElementIndex));
					return nullptr;
				}
				if (!NewScriptMapHelper.GetValueProperty()->ImportText_Direct(*ExportedValue, NewScriptMapHelper.GetValuePtr(NewElementIndex), nullptr, PPF_None))
				{
					UPyUtil::SetPythonError(PyExc_Exception, Self, *FString::Printf(TEXT("Failed to import text '%s' value for property '%s' (%s) at index %d"), *ExportedValue, *NewScriptMapHelper.GetValueProperty()->GetName(), *NewScriptMapHelper.GetValueProperty()->GetClass()->GetName(), NewElementIndex));
					return nullptr;
				}
			}

			NewScriptMapHelper.Rehash();
		}

		SetOptionalUPyConversionResult(FUPyConversionResult::SuccessWithCoercion(), OutCastResult);
		return NewMap.Release();
	}

	// Attempt conversion from any iterable type that has a defined length
	if (UPyUtil::HasLength(InPyObject))
	{
		const Py_ssize_t SequenceLen = PyObject_Length(InPyObject);
		int32 ElementCount = 0;
		if (UPyUtil::ValidateContainerLenValue(SequenceLen, ElementCount, *UPyUtil::GetErrorContext(InType)) == 0)
		{
			FUPyObjectPtr PyObjIter = FUPyObjectPtr::StealReference(PyObject_GetIter(InPyObject));
			if (PyObjIter)
			{
				FUPyWrapperMapPtr NewMap = FUPyWrapperMapPtr::StealReference(FUPyWrapperMap::New(InType));
				if (FUPyWrapperMap::Init(NewMap, InKeyDef, InValueDef) != 0)
				{
					return nullptr;
				}

				FScriptMapHelper NewScriptMapHelper(NewMap->MapProp, NewMap->MapInstance);

				if (UPyUtil::IsMappingType(InPyObject))
				{
					// Conversion from a mapping type
						for (int32 SequenceIndex = 0; SequenceIndex < ElementCount; ++SequenceIndex)
						{
							FUPyObjectPtr KeyItem = FUPyObjectPtr::StealReference(PyIter_Next(PyObjIter));
							if (!KeyItem)
							{
								if (!PyErr_Occurred())
								{
									UPyUtil::SetPythonError(PyExc_RuntimeError, InType, *FString::Printf(TEXT("Mapping iterator ended after %d of %d keys"), SequenceIndex, ElementCount));
								}
								return nullptr;
							}

						FUPyObjectPtr ValueItem = FUPyObjectPtr::StealReference(PyObject_GetItem(InPyObject, KeyItem));
						if (!ValueItem)
						{
							return nullptr;
						}

						const int32 NewElementIndex = NewScriptMapHelper.AddDefaultValue_Invalid_NeedsRehash();
						if (!UPyConversion::NativizeProperty(KeyItem, NewScriptMapHelper.GetKeyProperty(), NewScriptMapHelper.GetKeyPtr(NewElementIndex)))
						{
							return nullptr;
						}
						if (!UPyConversion::NativizeProperty(ValueItem, NewScriptMapHelper.GetValueProperty(), NewScriptMapHelper.GetValuePtr(NewElementIndex)))
						{
							return nullptr;
						}
					}
				}
				else
				{
					// Conversion from a sequence of pairs
						for (int32 SequenceIndex = 0; SequenceIndex < ElementCount; ++SequenceIndex)
						{
							FUPyObjectPtr PairItem = FUPyObjectPtr::StealReference(PyIter_Next(PyObjIter));
							if (!PairItem)
							{
								if (!PyErr_Occurred())
								{
									UPyUtil::SetPythonError(PyExc_RuntimeError, InType, *FString::Printf(TEXT("Iterable ended after %d of %d items"), SequenceIndex, ElementCount));
								}
								return nullptr;
							}

						FUPyObjectPtr PairSequence = FUPyObjectPtr::StealReference(PySequence_Fast(PairItem, ""));
						if (!PairSequence)
						{
							UPyUtil::SetPythonError(PyExc_TypeError, NewMap.Get(), *FString::Printf(TEXT("Failed to convert item at index %zd to a sequence"), SequenceIndex));
							return nullptr;
						}

						const Py_ssize_t PairLen = PySequence_Fast_GET_SIZE(PairSequence.Get());
						if (PairLen != 2)
						{
							UPyUtil::SetPythonError(PyExc_TypeError, NewMap.Get(), *FString::Printf(TEXT("Failed to convert item at index %zd as it was not a pair (len != 2)"), SequenceIndex));
							return nullptr;
						}

						const int32 NewElementIndex = NewScriptMapHelper.AddDefaultValue_Invalid_NeedsRehash();
						if (!UPyConversion::NativizeProperty(PySequence_Fast_GET_ITEM(PairSequence.Get(), 0), NewScriptMapHelper.GetKeyProperty(), NewScriptMapHelper.GetKeyPtr(NewElementIndex)))
						{
							return nullptr;
						}
						if (!UPyConversion::NativizeProperty(PySequence_Fast_GET_ITEM(PairSequence.Get(), 1), NewScriptMapHelper.GetValueProperty(), NewScriptMapHelper.GetValuePtr(NewElementIndex)))
						{
							return nullptr;
						}
					}
				}

				NewScriptMapHelper.Rehash();

				SetOptionalUPyConversionResult(FUPyConversionResult::SuccessWithCoercion(), OutCastResult);
				return NewMap.Release();
			}
		}
	}

	return nullptr;
}

Py_ssize_t FUPyWrapperMap::Len(FUPyWrapperMap* InSelf)
{
	if (!ValidateInternalState(InSelf))
	{
		return -1;
	}

	FScriptMapHelper SelfScriptMapHelper(InSelf->MapProp, InSelf->MapInstance);
	return SelfScriptMapHelper.Num();
}

PyObject* FUPyWrapperMap::GetItem(FUPyWrapperMap* InSelf, PyObject* InKey)
{
	return GetItem(InSelf, InKey, nullptr);
}

PyObject* FUPyWrapperMap::GetItem(FUPyWrapperMap* InSelf, PyObject* InKey, PyObject* InDefault)
{
	if (!ValidateInternalState(InSelf))
	{
		return nullptr;
	}

	UPyUtil::FMapKeyOnScope MapKey(InSelf->MapProp);
	if (!MapKey.IsValid())
	{
		return nullptr;
	}

	if (!MapKey.SetValue(InKey, *UPyUtil::GetErrorContext(InSelf)))
	{
		return nullptr;
	}

	FScriptMapHelper SelfScriptMapHelper(InSelf->MapProp, InSelf->MapInstance);

	const void* ValuePtr = SelfScriptMapHelper.FindValueFromHash(MapKey.GetValue());
	if (!ValuePtr)
	{
		if (!InDefault)
		{
			UPyUtil::SetPythonError(PyExc_KeyError, InSelf, *FString::Printf(TEXT("Key '%s' was not found in the map"), *UPyUtil::PyObjectToUEString(InKey)));
			return nullptr;
		}

		Py_INCREF(InDefault);
		return InDefault;
	}

	PyObject* PyItemObj = nullptr;
	if (!UPyConversion::PythonizeProperty(SelfScriptMapHelper.GetValueProperty(), ValuePtr, PyItemObj))
	{
		UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert value property '%s' (%s) for key '%s'"), *SelfScriptMapHelper.GetValueProperty()->GetName(), *SelfScriptMapHelper.GetValueProperty()->GetClass()->GetName(), *UPyUtil::PyObjectToUEString(InKey)));
		return nullptr;
	}
	return PyItemObj;
}

int FUPyWrapperMap::SetItem(FUPyWrapperMap* InSelf, PyObject* InKey, PyObject* InValue)
{
	if (!ValidateInternalState(InSelf))
	{
		return -1;
	}

	UPyUtil::FMapKeyOnScope MapKey(InSelf->MapProp);
	if (!MapKey.IsValid())
	{
		return -1;
	}

	if (!MapKey.SetValue(InKey, *UPyUtil::GetErrorContext(InSelf)))
	{
		return -1;
	}

	FScriptMapHelper SelfScriptMapHelper(InSelf->MapProp, InSelf->MapInstance);
	if (InValue)
	{
		UPyUtil::FMapValueOnScope MapValue(InSelf->MapProp);
		if (!MapValue.IsValid())
		{
			return -1;
		}

		if (!MapValue.SetValue(InValue, *UPyUtil::GetErrorContext(InSelf)))
		{
			return -1;
		}

		SelfScriptMapHelper.AddPair(MapKey.GetValue(), MapValue.GetValue());
	}
	else
	{
		SelfScriptMapHelper.RemovePair(MapKey.GetValue());
	}

	return 0;
}

PyObject* FUPyWrapperMap::SetDefault(FUPyWrapperMap* InSelf, PyObject* InKey, PyObject* InValue)
{
	if (!ValidateInternalState(InSelf))
	{
		return nullptr;
	}

	UPyUtil::FMapKeyOnScope MapKey(InSelf->MapProp);
	if (!MapKey.IsValid())
	{
		return nullptr;
	}

	if (!MapKey.SetValue(InKey, *UPyUtil::GetErrorContext(InSelf)))
	{
		return nullptr;
	}

	FScriptMapHelper SelfScriptMapHelper(InSelf->MapProp, InSelf->MapInstance);

	const void* ValuePtr = SelfScriptMapHelper.FindValueFromHash(MapKey.GetValue());
	if (!ValuePtr)
	{
		UPyUtil::FMapValueOnScope MapValue(InSelf->MapProp);
		if (!MapValue.IsValid())
		{
			return nullptr;
		}

		if (InValue && !MapValue.SetValue(InValue, *UPyUtil::GetErrorContext(InSelf)))
		{
			return nullptr;
		}

		SelfScriptMapHelper.AddPair(MapKey.GetValue(), MapValue.GetValue());

		ValuePtr = SelfScriptMapHelper.FindValueFromHash(MapKey.GetValue());
		check(ValuePtr);
	}

	PyObject* PyItemObj = nullptr;
	if (!UPyConversion::PythonizeProperty(SelfScriptMapHelper.GetValueProperty(), ValuePtr, PyItemObj))
	{
		UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert value property '%s' (%s) for key '%s'"), *SelfScriptMapHelper.GetValueProperty()->GetName(), *SelfScriptMapHelper.GetValueProperty()->GetClass()->GetName(), *UPyUtil::PyObjectToUEString(InKey)));
		return nullptr;
	}
	return PyItemObj;
}

int FUPyWrapperMap::Contains(FUPyWrapperMap* InSelf, PyObject* InKey)
{
	if (!ValidateInternalState(InSelf))
	{
		return -1;
	}

	UPyUtil::FMapKeyOnScope MapKey(InSelf->MapProp);
	if (!MapKey.IsValid())
	{
		return -1;
	}

	if (!MapKey.SetValue(InKey, *UPyUtil::GetErrorContext(InSelf)))
	{
		return -1;
	}

	FScriptMapHelper SelfScriptMapHelper(InSelf->MapProp, InSelf->MapInstance);
	return SelfScriptMapHelper.FindValueFromHash(MapKey.GetValue()) ? 1 : 0;
}

int FUPyWrapperMap::Clear(FUPyWrapperMap* InSelf)
{
	if (!ValidateInternalState(InSelf))
	{
		return -1;
	}

	FScriptMapHelper SelfScriptMapHelper(InSelf->MapProp, InSelf->MapInstance);
	SelfScriptMapHelper.EmptyValues();

	return 0;
}

PyObject* FUPyWrapperMap::Pop(FUPyWrapperMap* InSelf, PyObject* InKey, PyObject* InDefault)
{
	if (!ValidateInternalState(InSelf))
	{
		return nullptr;
	}

	UPyUtil::FMapKeyOnScope MapKey(InSelf->MapProp);
	if (!MapKey.IsValid())
	{
		return nullptr;
	}

	if (!MapKey.SetValue(InKey, *UPyUtil::GetErrorContext(InSelf)))
	{
		return nullptr;
	}

	FScriptMapHelper SelfScriptMapHelper(InSelf->MapProp, InSelf->MapInstance);

	const void* ValuePtr = SelfScriptMapHelper.FindValueFromHash(MapKey.GetValue());
	if (!ValuePtr)
	{
		if (!InDefault)
		{
			UPyUtil::SetPythonError(PyExc_KeyError, InSelf, *FString::Printf(TEXT("Key '%s' was not found in the map"), *UPyUtil::PyObjectToUEString(InKey)));
			return nullptr;
		}

		Py_INCREF(InDefault);
		return InDefault;
	}

	PyObject* PyItemObj = nullptr;
	if (!UPyConversion::PythonizeProperty(SelfScriptMapHelper.GetValueProperty(), ValuePtr, PyItemObj))
	{
		UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert value property '%s' (%s) for key '%s'"), *SelfScriptMapHelper.GetValueProperty()->GetName(), *SelfScriptMapHelper.GetValueProperty()->GetClass()->GetName(), *UPyUtil::PyObjectToUEString(InKey)));
		return nullptr;
	}

	SelfScriptMapHelper.RemovePair(MapKey.GetValue());

	return PyItemObj;
}

PyObject* FUPyWrapperMap::PopItem(FUPyWrapperMap* InSelf)
{
	if (!ValidateInternalState(InSelf))
	{
		return nullptr;
	}

	FScriptMapHelper SelfScriptMapHelper(InSelf->MapProp, InSelf->MapInstance);
	const FScriptMapHelper::FIterator It(SelfScriptMapHelper);
	if (It)
	{
		FUPyObjectPtr PyReturnKey;
		if (!UPyConversion::PythonizeProperty(SelfScriptMapHelper.GetKeyProperty(), SelfScriptMapHelper.GetKeyPtr(It), PyReturnKey.Get()))
		{
			UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert key property '%s' (%s) at index 0"), *SelfScriptMapHelper.GetKeyProperty()->GetName(), *SelfScriptMapHelper.GetKeyProperty()->GetClass()->GetName()));
			return nullptr;
		}

		FUPyObjectPtr PyReturnValue;
		if (!UPyConversion::PythonizeProperty(SelfScriptMapHelper.GetValueProperty(), SelfScriptMapHelper.GetValuePtr(It), PyReturnValue.Get()))
		{
			UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert value property '%s' (%s) at index 0"), *SelfScriptMapHelper.GetValueProperty()->GetName(), *SelfScriptMapHelper.GetValueProperty()->GetClass()->GetName()));
			return nullptr;
		}

		SelfScriptMapHelper.RemoveAt(It.GetInternalIndex());

		return PyTuple_Pack(2, PyReturnKey.Get(), PyReturnValue.Get());
	}

	UPyUtil::SetPythonError(PyExc_KeyError, InSelf, TEXT("Cannot pop from an empty map"));
	return nullptr;
}

int FUPyWrapperMap::Update(FUPyWrapperMap* InSelf, PyObject* InOther)
{
	if (!ValidateInternalState(InSelf))
	{
		return -1;
	}

	const UPyUtil::FPropertyDef SelfKeyDef = InSelf->MapProp->KeyProp;
	const UPyUtil::FPropertyDef SelfValueDef = InSelf->MapProp->ValueProp;
	FUPyWrapperMapPtr Other = FUPyWrapperMapPtr::StealReference(FUPyWrapperMap::CastPyObject(InOther, &UPyWrapperMapType, SelfKeyDef, SelfValueDef));
	if (!Other)
	{
		UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Cannot convert argument (%s) to '%s'"), *UPyUtil::GetFriendlyTypename(InOther), *UPyUtil::GetFriendlyTypename(InSelf)));
		return -1;
	}

	FScriptMapHelper SelfScriptMapHelper(InSelf->MapProp, InSelf->MapInstance);
	const FScriptMapHelper OtherScriptMapHelper(Other->MapProp, Other->MapInstance);

	for (FScriptMapHelper::FIterator It(OtherScriptMapHelper); It; ++It)
	{
		SelfScriptMapHelper.AddPair(OtherScriptMapHelper.GetKeyPtr(It), OtherScriptMapHelper.GetValuePtr(It));
	}

	return 0;
}

PyObject* FUPyWrapperMap::Items(FUPyWrapperMap* InSelf)
{
	return MakeListFromMapIter<FUPyWrapperMapItemIterator>(InSelf);
}

PyObject* FUPyWrapperMap::Keys(FUPyWrapperMap* InSelf)
{
	return MakeListFromMapIter<FPyWrapperMapKeyIterator>(InSelf);
}

PyObject* FUPyWrapperMap::Values(FUPyWrapperMap* InSelf)
{
	return MakeListFromMapIter<FPyWrapperMapValueIterator>(InSelf);
}

FUPyWrapperMap* FUPyWrapperMap::FromKeys(PyObject* InSequence, PyObject* InValue, PyTypeObject* InType)
{
	const Py_ssize_t SequenceLen = PyObject_Length(InSequence);
	int32 ElementCount = 0;
	if (UPyUtil::ValidateContainerLenValue(SequenceLen, ElementCount, *UPyUtil::GetErrorContext(InType)) != 0)
	{
		return nullptr;
	}

	if (ElementCount == 0)
	{
		UPyUtil::SetPythonError(PyExc_Exception, InType, *FString::Printf(TEXT("'sequence' (%s) cannot be empty"), *UPyUtil::GetFriendlyTypename(InSequence)));
		return nullptr;
	}

	FUPyObjectPtr PySequenceIter = FUPyObjectPtr::StealReference(PyObject_GetIter(InSequence));
	if (!PySequenceIter)
	{
		UPyUtil::SetPythonError(PyExc_Exception, InType, *FString::Printf(TEXT("'sequence' (%s) must be iterable"), *UPyUtil::GetFriendlyTypename(InSequence)));
		return nullptr;
	}

	UPyUtil::FPropertyDef MapKeyDef;
	UPyUtil::FPropertyDef MapValueDef;
	if (UPyUtil::ValidateContainerTypeParam((PyObject*)Py_TYPE(InValue), MapValueDef, "value", *UPyUtil::GetErrorContext(InType)) != 0)
	{
		return nullptr;
	}

	FUPyWrapperMapPtr NewMap;
	for (int32 SequenceIndex = 0; SequenceIndex < ElementCount; ++SequenceIndex)
	{
		FUPyObjectPtr PyKeyItem = FUPyObjectPtr::StealReference(PyIter_Next(PySequenceIter));
		if (!PyKeyItem)
		{
			if (!PyErr_Occurred())
			{
				UPyUtil::SetPythonError(PyExc_RuntimeError, InType, *FString::Printf(TEXT("'sequence' iterator ended after %d of %d keys"), SequenceIndex, ElementCount));
			}
			return nullptr;
		}

		if (!NewMap)
		{
			if (UPyUtil::ValidateContainerTypeParam((PyObject*)Py_TYPE(PyKeyItem), MapKeyDef, "sequence[0]", *UPyUtil::GetErrorContext(InType)) != 0)
			{
				return nullptr;
			}

			NewMap = FUPyWrapperMapPtr::StealReference(FUPyWrapperMap::New(InType));
			if (FUPyWrapperMap::Init(NewMap, MapKeyDef, MapValueDef) != 0)
			{
				return nullptr;
			}
		}

		FScriptMapHelper NewScriptMapHelper(NewMap->MapProp, NewMap->MapInstance);

		const int32 NewElementIndex = NewScriptMapHelper.AddDefaultValue_Invalid_NeedsRehash();
		if (!UPyConversion::NativizeProperty(PyKeyItem, NewScriptMapHelper.GetKeyProperty(), NewScriptMapHelper.GetKeyPtr(NewElementIndex)))
		{
			return nullptr;
		}
		if (!UPyConversion::NativizeProperty(InValue, NewScriptMapHelper.GetValueProperty(), NewScriptMapHelper.GetValuePtr(NewElementIndex)))
		{
			return nullptr;
		}
	}

	FScriptMapHelper NewScriptMapHelper(NewMap->MapProp, NewMap->MapInstance);
	NewScriptMapHelper.Rehash();

	return NewMap.Release();
}

// ==================== Wrapper Map type BEGIN ====================

struct FFuncs_WrapperMap
{
	static PyObject* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
	{
		return (PyObject*)FUPyWrapperMap::New(InType);
	}

	static void Dealloc(FUPyWrapperMap* InSelf)
	{
		FUPyWrapperMap::Free(InSelf);
	}

	static int Init(FUPyWrapperMap* InSelf, PyObject* InArgs, PyObject* InKwds)
	{
		PyObject* PyKeyObj = nullptr;
		PyObject* PyValueObj = nullptr;

		static const char *ArgsKwdList[] = { "key", "value", nullptr };
		if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "OO:call", (char**)ArgsKwdList, &PyKeyObj, &PyValueObj))
		{
			return -1;
		}

		UPyUtil::FPropertyDef MapKeyDef;
		const int ValidateKeyResult = UPyUtil::ValidateContainerTypeParam(PyKeyObj, MapKeyDef, "key", *UPyUtil::GetErrorContext(Py_TYPE(InSelf)));
		if (ValidateKeyResult != 0)
		{
			return ValidateKeyResult;
		}

		UPyUtil::FPropertyDef MapValueDef;
		const int ValidateValueResult = UPyUtil::ValidateContainerTypeParam(PyValueObj, MapValueDef, "value", *UPyUtil::GetErrorContext(Py_TYPE(InSelf)));
		if (ValidateValueResult != 0)
		{
			return ValidateValueResult;
		}

		return FUPyWrapperMap::Init(InSelf, MapKeyDef, MapValueDef);
	}

	static PyObject* Str(FUPyWrapperMap* InSelf)
	{
		if (!FUPyWrapperMap::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		const FScriptMapHelper SelfScriptMapHelper(InSelf->MapProp, InSelf->MapInstance);

		FString ExportedMap;
		for (FScriptMapHelper::FIterator It(SelfScriptMapHelper); It; ++It)
		{
			if (It.GetLogicalIndex() > 0)
			{
				ExportedMap += TEXT(", ");
			}
			ExportedMap += UPyUtil::GetFriendlyPropertyValue(SelfScriptMapHelper.GetKeyProperty(), SelfScriptMapHelper.GetKeyPtr(It), PPF_Delimited | PPF_IncludeTransient);
			ExportedMap += TEXT(": ");
			ExportedMap += UPyUtil::GetFriendlyPropertyValue(SelfScriptMapHelper.GetValueProperty(), SelfScriptMapHelper.GetValuePtr(It), PPF_Delimited | PPF_IncludeTransient);
		}
		return PyUnicode_FromFormat("{%s}", TCHAR_TO_UTF8(*ExportedMap));
	}

	static PyObject* RichCmp(FUPyWrapperMap* InSelf, PyObject* InOther, int InOp)
	{
		if (!FUPyWrapperMap::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		const UPyUtil::FPropertyDef SelfKeyDef = InSelf->MapProp->KeyProp;
		const UPyUtil::FPropertyDef SelfValueDef = InSelf->MapProp->ValueProp;
		FUPyWrapperMapPtr Other = FUPyWrapperMapPtr::StealReference(FUPyWrapperMap::CastPyObject(InOther, &UPyWrapperMapType, SelfKeyDef, SelfValueDef));
		if (!Other)
		{
			PyErr_Clear();
			Py_INCREF(Py_NotImplemented);
			return Py_NotImplemented;
		}

		if (InOp != Py_EQ && InOp != Py_NE)
		{
			UPyUtil::SetPythonError(PyExc_TypeError, InSelf, TEXT("Only == and != comparison is supported"));
			return nullptr;
		}

		const bool bIsIdentical = InSelf->MapProp->Identical(InSelf->MapInstance, Other->MapInstance, PPF_None);
		return PyBool_FromLong(InOp == Py_EQ ? bIsIdentical : !bIsIdentical);
	}

	static UPyUtil::FPyHashType Hash(FUPyWrapperMap* InSelf)
	{
		if (!FUPyWrapperMap::ValidateInternalState(InSelf))
		{
			return -1;
		}

		if (InSelf->MapProp->HasAnyPropertyFlags(CPF_HasGetValueTypeHash))
		{
			const uint32 MapHash = InSelf->MapProp->GetValueTypeHash(InSelf->MapInstance);
			return (UPyUtil::FPyHashType)(MapHash != -1 ? MapHash : 0);
		}

		UPyUtil::SetPythonError(PyExc_Exception, InSelf, TEXT("Type cannot be hashed"));
		return -1;
	}

	static PyObject* GetIter(FUPyWrapperMap* InSelf)
	{
		return MakeMapIter<FPyWrapperMapKeyIterator>(InSelf);
	}
};

struct FSequenceFuncs_WrapperMap
{
	static int Contains(FUPyWrapperMap* InSelf, PyObject* InValue)
	{
		return FUPyWrapperMap::Contains(InSelf, InValue);
	}
};

struct FMappingFuncs_WrapperMap
{
	static Py_ssize_t Len(FUPyWrapperMap* InSelf)
	{
		return FUPyWrapperMap::Len(InSelf);
	}

	static PyObject* GetItem(FUPyWrapperMap* InSelf, PyObject* InKey)
	{
		return FUPyWrapperMap::GetItem(InSelf, InKey);
	}

	static int SetItem(FUPyWrapperMap* InSelf, PyObject* InKey, PyObject* InValue)
	{
		return FUPyWrapperMap::SetItem(InSelf, InKey, InValue);
	}
};

void InitializeUPyWrapperMapType(UPyGenUtil::FNativePythonModule& ModuleInfo)
{
	PyTypeObject* PyType = &UPyWrapperMapType;

	PyType->tp_base = &UPyWrapperBaseType;
	PyType->tp_new = (newfunc)&FFuncs_WrapperMap::New;
	PyType->tp_dealloc = (destructor)&FFuncs_WrapperMap::Dealloc;
	PyType->tp_init = (initproc)&FFuncs_WrapperMap::Init;
	PyType->tp_str = (reprfunc)&FFuncs_WrapperMap::Str;
	PyType->tp_richcompare = (richcmpfunc)&FFuncs_WrapperMap::RichCmp;
	PyType->tp_hash = (hashfunc)&FFuncs_WrapperMap::Hash;
	PyType->tp_iter = (getiterfunc)&FFuncs_WrapperMap::GetIter;

	PyType->tp_methods = MapPyMethodDefs;

	PyType->tp_flags = Py_TPFLAGS_DEFAULT;
	PyType->tp_doc = "Type for all Unreal exposed map instances";

	static PySequenceMethods PySequence;
	PySequence.sq_contains = (objobjproc)&FSequenceFuncs_WrapperMap::Contains;

	static PyMappingMethods PyMapping;
	PyMapping.mp_length = (lenfunc)&FMappingFuncs_WrapperMap::Len;
	PyMapping.mp_subscript = (binaryfunc)&FMappingFuncs_WrapperMap::GetItem;
	PyMapping.mp_ass_subscript = (objobjargproc)&FMappingFuncs_WrapperMap::SetItem;

	PyType->tp_as_sequence = &PySequence;
	PyType->tp_as_mapping = &PyMapping;

	if (PyType_Ready(&UPyWrapperMapType) == 0)
	{
		// static FPyWrapperMapMetaData MetaData;
		// FPyWrapperMapMetaData::SetMetaData(&UPyWrapperMapType, &MetaData);
		ModuleInfo.AddType(&UPyWrapperMapType, true);
	}
}

void InitializeUPyWrapperMap(UPyGenUtil::FNativePythonModule& ModuleInfo)
{
	InitializeUPyWrapperMapType(ModuleInfo);

	PyType_Ready(&PyWrapperMapItemIteratorType);
	PyType_Ready(&PyWrapperMapKeyIteratorType);
	PyType_Ready(&PyWrapperMapValueIteratorType);

	PyType_Ready(&PyWrapperMapItemViewType);
	PyType_Ready(&PyWrapperMapKeyViewType);
	PyType_Ready(&PyWrapperMapValueViewType);
}
// ==================== Wrapper Map type END ====================

template <typename TImpl>
PyTypeObject InitializePyWrapperMapIteratorType(const char* InName)
{
	struct FFuncs
	{
		static PyObject* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
		{
			return (PyObject*)TImpl::New(InType);
		}

		static void Dealloc(TImpl* InSelf)
		{
			TImpl::Free(InSelf);
		}

		static int Init(TImpl* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			PyObject* PyObj = nullptr;
			if (!PyArg_ParseTuple(InArgs, "O:call", &PyObj))
			{
				return -1;
			}

			if (PyObject_IsInstance(PyObj, (PyObject*)&UPyWrapperMapType) != 1)
			{
				UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Cannot initialize '%s' with an instance of '%s'"), *UPyUtil::GetFriendlyTypename(InSelf), *UPyUtil::GetFriendlyTypename(PyObj)));
				return -1;
			}

			return TImpl::Init(InSelf, (FUPyWrapperMap*)PyObj);
		}

		static PyObject* GetIter(TImpl* InSelf)
		{
			return TImpl::GetIter(InSelf);
		}

		static PyObject* IterNext(TImpl* InSelf)
		{
			return TImpl::IterNext(InSelf);
		}
	};

	PyTypeObject PyType = {
		PyVarObject_HEAD_INIT(nullptr, 0)
		InName, /* tp_name */
		sizeof(TImpl), /* tp_basicsize */
	};

	PyType.tp_new = (newfunc)&FFuncs::New;
	PyType.tp_dealloc = (destructor)&FFuncs::Dealloc;
	PyType.tp_init = (initproc)&FFuncs::Init;
	PyType.tp_iter = (getiterfunc)&FFuncs::GetIter;
	PyType.tp_iternext = (iternextfunc)&FFuncs::IterNext;

	PyType.tp_flags = Py_TPFLAGS_DEFAULT;
	PyType.tp_doc = "Type for all Unreal exposed map iterators";

	return PyType;
}

template <typename TImpl>
PyTypeObject InitializePyWrapperMapViewType(const char* InName)
{
	struct FFuncs
	{
		static PyObject* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
		{
			return (PyObject*)TImpl::New(InType);
		}

		static void Dealloc(TImpl* InSelf)
		{
			TImpl::Free(InSelf);
		}

		static int Init(TImpl* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			PyObject* PyObj = nullptr;
			if (!PyArg_ParseTuple(InArgs, "O:call", &PyObj))
			{
				return -1;
			}

			if (PyObject_IsInstance(PyObj, (PyObject*)&UPyWrapperMapType) != 1)
			{
				UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Cannot initialize '%s' with an instance of '%s'"), *UPyUtil::GetFriendlyTypename(InSelf), *UPyUtil::GetFriendlyTypename(PyObj)));
				return -1;
			}

			return TImpl::Init(InSelf, (FUPyWrapperMap*)PyObj);
		}

		static PyObject* GetIter(TImpl* InSelf)
		{
			return TImpl::GetIter(InSelf);
		}
	};

	struct FSequenceFuncs
	{
		static Py_ssize_t Len(TImpl* InSelf)
		{
			return TImpl::Len(InSelf);
		}

		static int Contains(TImpl* InSelf, PyObject* InValue)
		{
			FUPyObjectPtr PySet = FUPyObjectPtr::StealReference(PySet_New((PyObject*)InSelf));
			if (!PySet)
			{
				return -1;
			}

			return PySet_Contains(PySet, InValue);
		}
	};

	PyTypeObject PyType = {
		PyVarObject_HEAD_INIT(nullptr, 0)
		InName, /* tp_name */
		sizeof(TImpl), /* tp_basicsize */
	};

	PyType.tp_new = (newfunc)&FFuncs::New;
	PyType.tp_dealloc = (destructor)&FFuncs::Dealloc;
	PyType.tp_init = (initproc)&FFuncs::Init;
	PyType.tp_iter = (getiterfunc)&FFuncs::GetIter;

	PyType.tp_flags = Py_TPFLAGS_DEFAULT;
	PyType.tp_doc = "Type for all Unreal exposed map views";

	static PySequenceMethods PySequence;
	PySequence.sq_length = (lenfunc)&FSequenceFuncs::Len;
	PySequence.sq_contains = (objobjproc)&FSequenceFuncs::Contains;

	PyType.tp_as_sequence = &PySequence;

	return PyType;
}

template <typename TImpl>
PyTypeObject InitializePyWrapperMapSetViewType(const char* InName)
{
	struct FNumberFuncs
	{
		static PyObject* Sub(PyObject* InLHS, PyObject* InRHS)
		{
			FUPyObjectPtr LHS = FUPyObjectPtr::StealReference(PySet_New(InLHS));
			FUPyObjectPtr RHS = FUPyObjectPtr::StealReference(PySet_New(InRHS));
			if (!LHS || !RHS)
			{
				return nullptr;
			}

			return PyNumber_Subtract(LHS, RHS);
		}

		static PyObject* And(PyObject* InLHS, PyObject* InRHS)
		{
			FUPyObjectPtr LHS = FUPyObjectPtr::StealReference(PySet_New(InLHS));
			FUPyObjectPtr RHS = FUPyObjectPtr::StealReference(PySet_New(InRHS));
			if (!LHS || !RHS)
			{
				return nullptr;
			}

			return PyNumber_And(LHS, RHS);
		}

		static PyObject* Xor(PyObject* InLHS, PyObject* InRHS)
		{
			FUPyObjectPtr LHS = FUPyObjectPtr::StealReference(PySet_New(InLHS));
			FUPyObjectPtr RHS = FUPyObjectPtr::StealReference(PySet_New(InRHS));
			if (!LHS || !RHS)
			{
				return nullptr;
			}

			return PyNumber_Xor(LHS, RHS);
		}

		static PyObject* Or(PyObject* InLHS, PyObject* InRHS)
		{
			FUPyObjectPtr LHS = FUPyObjectPtr::StealReference(PySet_New(InLHS));
			FUPyObjectPtr RHS = FUPyObjectPtr::StealReference(PySet_New(InRHS));
			if (!LHS || !RHS)
			{
				return nullptr;
			}

			return PyNumber_Or(LHS, RHS);
		}
	};

	PyTypeObject PyType = InitializePyWrapperMapViewType<TImpl>(InName);

	static PyNumberMethods PyNumber;
	PyNumber.nb_subtract = (binaryfunc)&FNumberFuncs::Sub;
	PyNumber.nb_and = (binaryfunc)&FNumberFuncs::And;
	PyNumber.nb_xor = (binaryfunc)&FNumberFuncs::Xor;
	PyNumber.nb_or = (binaryfunc)&FNumberFuncs::Or;

	PyType.tp_as_number = &PyNumber;

	return PyType;
}

PyTypeObject PyWrapperMapItemIteratorType = InitializePyWrapperMapIteratorType<FUPyWrapperMapItemIterator>("MapItemIterator");
PyTypeObject PyWrapperMapKeyIteratorType = InitializePyWrapperMapIteratorType<FPyWrapperMapKeyIterator>("MapKeyIterator");
PyTypeObject PyWrapperMapValueIteratorType = InitializePyWrapperMapIteratorType<FPyWrapperMapValueIterator>("MapValueIterator");
PyTypeObject PyWrapperMapItemViewType = InitializePyWrapperMapSetViewType<FPyWrapperMapItemView>("MapItemView");
PyTypeObject PyWrapperMapKeyViewType = InitializePyWrapperMapSetViewType<FPyWrapperMapKeyView>("MapKeyView");
PyTypeObject PyWrapperMapValueViewType = InitializePyWrapperMapViewType<FPyWrapperMapValueView>("MapValueView");

PyObject* GetUPyWrapperMapItems(FUPyWrapperMap* InSelf)
{
	return MakeMapView<FPyWrapperMapItemView>(InSelf);
}

PyObject* GetUPyWrapperMapKeys(FUPyWrapperMap* InSelf)
{
	return MakeMapView<FPyWrapperMapKeyView>(InSelf);
}

PyObject* GetUPyWrapperMapValues(FUPyWrapperMap* InSelf)
{
	return MakeMapView<FPyWrapperMapValueView>(InSelf);
}
