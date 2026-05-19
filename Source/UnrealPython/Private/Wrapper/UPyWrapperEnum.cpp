// Copyright Epic Games, Inc. All Rights Reserved.

#include "Wrapper/UPyWrapperEnum.h"
#include "Wrapper/UPyWrapperTypeRegistry.h"
#include "Core/UPyConversion.h"
#include "Utils/UPyUtil.h"

#include "Containers/ArrayView.h"
#include "DynamicTypes/UPyGeneratedWrappedEnumType.h"
#include "Internationalization/Text.h"
#include "UObject/ObjectRedirector.h"

PyTypeObject UPyWrapperEnumType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"EnumBase", /* tp_name */
	sizeof(FUPyWrapperEnum), /* tp_basicsize */
};

PyTypeObject UPyWrapperEnumValueDescrType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"_EnumEntry", /* tp_name */
	sizeof(FUPyWrapperEnumValueDescrObject), /* tp_basicsize */
};

PyTypeObject UPyWrapperEnumMetaclassType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"_EnumType", /* tp_name */
	0, /* tp_basicsize */
};

PyTypeObject UPyWrapperEnumIteratorType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"_EnumIterator", /* tp_name */
	sizeof(FUPyWrapperEnumIterator), /* tp_basicsize */
};

FUPyWrapperEnum* FUPyWrapperEnum::New(PyTypeObject* InType)
{
	FUPyWrapperEnum* Self = (FUPyWrapperEnum*)FUPyWrapperBase::New(InType);
	if (Self)
	{
		Self->EntryName = nullptr;
		Self->EntryValue = nullptr;
	}
	return Self;
}

void FUPyWrapperEnum::Free(FUPyWrapperEnum* InSelf)
{
	Deinit(InSelf);

	FUPyWrapperBase::Free(InSelf);
}

int FUPyWrapperEnum::Init(FUPyWrapperEnum* InSelf)
{
	UPyUtil::SetPythonError(PyExc_Exception, InSelf, TEXT("Cannot create instances of enum types"));
	return -1;
}

int FUPyWrapperEnum::Init(FUPyWrapperEnum* InSelf, const int64 InEnumEntryValue, const char* InEnumEntryName)
{
	// if (FPyWrapperEnumMetaData::IsEnumFinalized(InSelf))
	// {
	// 	UPyUtil::SetPythonError(PyExc_Exception, InSelf, TEXT("Cannot create instances of enum types"));
	// 	return -1;
	// }

	InSelf->EntryName = PyUnicode_FromString(InEnumEntryName);
	InSelf->EntryValue = UPyConversion::Pythonize(InEnumEntryValue);
	return 0;
}

void FUPyWrapperEnum::Deinit(FUPyWrapperEnum* InSelf)
{
	Py_XDECREF(InSelf->EntryName);
	InSelf->EntryName = nullptr;

	Py_XDECREF(InSelf->EntryValue);
	InSelf->EntryValue = nullptr;
}

bool FUPyWrapperEnum::ValidateInternalState(FUPyWrapperEnum* InSelf)
{
	if (!InSelf->EntryName)
	{
		UPyUtil::SetPythonError(PyExc_Exception, Py_TYPE(InSelf), TEXT("Internal Error - EntryName is null!"));
		return false;
	}

	if (!InSelf->EntryValue)
	{
		UPyUtil::SetPythonError(PyExc_Exception, Py_TYPE(InSelf), TEXT("Internal Error - EntryValue is null!"));
		return false;
	}

	return true;
}

FUPyWrapperEnum* FUPyWrapperEnum::CastPyObject(PyObject* InPyObject, FUPyConversionResult* OutCastResult)
{
	SetOptionalUPyConversionResult(FUPyConversionResult::Failure(), OutCastResult);

	if (PyObject_IsInstance(InPyObject, (PyObject*)&UPyWrapperEnumType) == 1)
	{
		SetOptionalUPyConversionResult(FUPyConversionResult::Success(), OutCastResult);

		Py_INCREF(InPyObject);
		return (FUPyWrapperEnum*)InPyObject;
	}

	return nullptr;
}

FUPyWrapperEnum* FUPyWrapperEnum::CastPyObject(PyObject* InPyObject, const PyTypeObject* InType, FUPyConversionResult* OutCastResult)
{
	SetOptionalUPyConversionResult(FUPyConversionResult::Failure(), OutCastResult);

	if (PyObject_IsInstance(InPyObject, (PyObject*)InType) == 1 && (InType == &UPyWrapperEnumType || PyObject_IsInstance(InPyObject, (PyObject*)&UPyWrapperEnumType) == 1))
	{
		SetOptionalUPyConversionResult(Py_TYPE(InPyObject) == InType ? FUPyConversionResult::Success() : FUPyConversionResult::SuccessWithCoercion(), OutCastResult);

		Py_INCREF(InPyObject);
		return (FUPyWrapperEnum*)InPyObject;
	}

	// Allow casting from a different enum type using the same UEnum (for deprecation)
	const UEnum* RequiredEnum = FUPyWrapperTypeRegistry::Get().FindEnum(InType);

	if (PyObject_IsInstance(InPyObject, (PyObject*)&UPyWrapperEnumType) == 1)
	{
		const UEnum* ActualEnum = FUPyWrapperTypeRegistry::Get().FindEnum(Py_TYPE(InPyObject));

		if (RequiredEnum == ActualEnum)
		{
			SetOptionalUPyConversionResult(FUPyConversionResult::Success(), OutCastResult);

			Py_INCREF(InPyObject);
			return (FUPyWrapperEnum*)InPyObject;
		}
	}

	// Allow coerced casting from a numeric value
	int64 OtherVal = 0;
	if (UPyConversion::Nativize(InPyObject, OtherVal))
	{
		TSharedPtr<UPyGenUtil::FGeneratedWrappedType> WrappedType = FUPyWrapperTypeRegistry::Get().GetGeneratedWrappedType(RequiredEnum);
		TSharedPtr<FUPyGeneratedWrappedEnumType> WrappedEnumType = StaticCastSharedPtr<FUPyGeneratedWrappedEnumType>(WrappedType);
		if (WrappedEnumType)
		{
			// Find an enum entry using this value
			for (FUPyWrapperEnum* PyEnumEntry : WrappedEnumType->PyEnumEntries)
			{
				const int64 EnumEntryVal = FUPyWrapperEnum::GetEnumEntryValue(PyEnumEntry);
				if (EnumEntryVal == OtherVal)
				{
					SetOptionalUPyConversionResult(FUPyConversionResult::SuccessWithCoercion(), OutCastResult);

					Py_INCREF(PyEnumEntry);
					return (FUPyWrapperEnum*)PyEnumEntry;
				}
			}
		}
		else if (InType)
		{
			PyObject* Key;
			PyObject* Value;
			Py_ssize_t Pos = 0;

			while (PyDict_Next(InType->tp_dict, &Pos, &Key, &Value))
			{
				if (Py_TYPE(Value) == &UPyWrapperEnumValueDescrType)
				{
					FUPyWrapperEnumValueDescrObject* PyEnumDescr = (FUPyWrapperEnumValueDescrObject*)Value;
					FUPyWrapperEnum* PyEnumEntry = PyEnumDescr->EnumEntry;
					const int64 EnumEntryVal = FUPyWrapperEnum::GetEnumEntryValue(PyEnumEntry);
					if (EnumEntryVal == OtherVal)
					{
						SetOptionalUPyConversionResult(FUPyConversionResult::SuccessWithCoercion(), OutCastResult);

						Py_INCREF(PyEnumEntry);
						return (FUPyWrapperEnum*)PyEnumEntry;
					}
				}
			}
		}
	}

	return nullptr;
}

FString FUPyWrapperEnum::GetEnumEntryName(FUPyWrapperEnum* InSelf)
{
	FString EnumEntryNameStr;
	if (InSelf->EntryName)
	{
		UPyConversion::Nativize(InSelf->EntryName, EnumEntryNameStr);
	}
	return EnumEntryNameStr;
}

int64 FUPyWrapperEnum::GetEnumEntryValue(FUPyWrapperEnum* InSelf)
{
	int64 EnumEntryValueNum = 0;
	if (InSelf->EntryValue)
	{
		UPyConversion::Nativize(InSelf->EntryValue, EnumEntryValueNum);
	}
	return EnumEntryValueNum;
}

FUPyWrapperEnum* FUPyWrapperEnum::AddEnumEntry(PyTypeObject* InType, const int64 InEnumEntryValue, const char* InEnumEntryName, const char* InEnumEntryDoc)
{
	// if (!FPyWrapperEnumMetaData::IsEnumFinalized(InType))
	{
		FUPyWrapperEnumValueDescrObjectPtr PyEnumValueDescr = FUPyWrapperEnumValueDescrObjectPtr::StealReference(FUPyWrapperEnumValueDescrObject::New(InType, InEnumEntryValue, InEnumEntryName, InEnumEntryDoc));
		if (PyEnumValueDescr)
		{
			PyDict_SetItemString(InType->tp_dict, InEnumEntryName, (PyObject*)PyEnumValueDescr.GetPtr());
			return PyEnumValueDescr->EnumEntry;
		}
	}
	return nullptr;
}

FUPyWrapperEnum* FUPyWrapperEnum::AddDeprecatedEnumEntry(PyTypeObject* InType, FUPyWrapperEnum* InEnumEntry, const char* InEnumEntryName, const char* InEnumEntryDoc)
{
	// if (!FPyWrapperEnumMetaData::IsEnumFinalized(InType))
	{
		FUPyWrapperEnumValueDescrObjectPtr PyEnumValueDescr = FUPyWrapperEnumValueDescrObjectPtr::StealReference(FUPyWrapperEnumValueDescrObject::NewDeprecated(InEnumEntry, InEnumEntryName, InEnumEntryDoc));
		if (PyEnumValueDescr)
		{
			PyDict_SetItemString(InType->tp_dict, InEnumEntryName, (PyObject*)PyEnumValueDescr.GetPtr());
			return PyEnumValueDescr->EnumEntry;
		}
	}
	return nullptr;
}

// ==================== Wrapper EnumValueDescr Type BEGIN ====================
static void Dealloc_EnumValueDescr(FUPyWrapperEnumValueDescrObject* InSelf)
{
	FUPyWrapperEnumValueDescrObject::Free(InSelf);
}

static PyObject* Str_EnumValueDescr(FUPyWrapperEnumValueDescrObject* InSelf)
{
	return PyUnicode_FromString("<built-in enum value>");
}

static PyObject* DescrGet_EnumValueDescr(FUPyWrapperEnumValueDescrObject* InSelf, PyObject* InObj, PyObject* InType)
{
	if (!FUPyWrapperEnum::ValidateInternalState(InSelf->EnumEntry))
	{
		return nullptr;
	}

	// Deprecated enums emit a warning
	// {
	// 	FString DeprecationMessage;
	// 	if (FPyWrapperEnumMetaData::IsEnumDeprecated(InSelf->EnumEntry, &DeprecationMessage) &&
	// 		UPyUtil::SetPythonWarning(PyExc_DeprecationWarning, InSelf->EnumEntry, *FString::Printf(TEXT("Enum '%s' is deprecated: %s"), UTF8_TO_TCHAR(Py_TYPE(InSelf->EnumEntry)->tp_name), *DeprecationMessage)) == -1
	// 		)
	// 	{
	// 		// -1 from SetPythonWarning means the warning should be an exception
	// 		return nullptr;
	// 	}
	// }

	// Deprecated enum entries emit a warning
	if (InSelf->DeprecationMessage.IsSet() && UPyUtil::SetPythonWarning(PyExc_DeprecationWarning, InSelf->EnumEntry, *InSelf->DeprecationMessage.GetValue()) == -1)
	{
		// -1 from SetPythonWarning means the warning should be an exception
		return nullptr;
	}

	Py_INCREF(InSelf->EnumEntry);
	return (PyObject*)InSelf->EnumEntry;
}

static int DescrSet_EnumValueDescr(FUPyWrapperEnumValueDescrObject* InSelf, PyObject* InObj, PyObject* InValue)
{
	PyErr_SetString(PyExc_Exception, "Enum values are read-only");
	return -1;
}

struct FGetSets_EnumValueDescr
{
	static PyObject* GetName(FUPyWrapperEnumValueDescrObject* InSelf, void* InClosure)
	{
		if (InSelf->EnumEntry && InSelf->EnumEntry->EntryName)
		{
			Py_INCREF(InSelf->EnumEntry->EntryName);
			return InSelf->EnumEntry->EntryName;
		}

		Py_RETURN_NONE;
	}

	static PyObject* GetDoc(FUPyWrapperEnumValueDescrObject* InSelf, void* InClosure)
	{
		if (InSelf->EnumEntryDoc)
		{
			Py_INCREF(InSelf->EnumEntryDoc);
			return InSelf->EnumEntryDoc;
		}

		Py_RETURN_NONE;
	}
};

static PyGetSetDef PyGetSets_EnumValueDescr[] = {
	{ UPyCStrCast("__name__"), (getter)&FGetSets_EnumValueDescr::GetName, nullptr, nullptr, nullptr },
	{ UPyCStrCast("__doc__"), (getter)&FGetSets_EnumValueDescr::GetDoc, nullptr, nullptr, nullptr },
	{ nullptr, nullptr, nullptr, nullptr, nullptr }
};

PyTypeObject* InitializeUPyWrapperEnumValueDescrType()
{
	PyTypeObject* PyType = &UPyWrapperEnumValueDescrType;

	PyType->tp_dealloc = (destructor)&Dealloc_EnumValueDescr;
	PyType->tp_str = (reprfunc)&Str_EnumValueDescr;
	PyType->tp_repr = (reprfunc)&Str_EnumValueDescr;
	PyType->tp_descr_get = (descrgetfunc)&DescrGet_EnumValueDescr;
	PyType->tp_descr_set = (descrsetfunc)&DescrSet_EnumValueDescr;
	PyType->tp_getattro = (getattrofunc)&PyObject_GenericGetAttr;

	PyType->tp_getset = PyGetSets_EnumValueDescr;

	PyType->tp_flags = Py_TPFLAGS_DEFAULT;

	return PyType;
}
// ==================== Wrapper EnumValueDescr Type END ====================

// ==================== Wrapper EnumMetaclass Type BEGIN ====================
struct FFuncs_EnumMetaclass
{
	static PyObject* GetIter(PyTypeObject* InSelf)
	{
		typedef TUPyPtr<FUPyWrapperEnumIterator> FPyWrapperEnumIteratorPtr;

		const UEnum* RequiredEnum = FUPyWrapperTypeRegistry::Get().FindEnum(InSelf);
		TSharedPtr<UPyGenUtil::FGeneratedWrappedType> WrappedType = FUPyWrapperTypeRegistry::Get().GetGeneratedWrappedType(RequiredEnum);
		TSharedPtr<FUPyGeneratedWrappedEnumType> WrappedEnumType = StaticCastSharedPtr<FUPyGeneratedWrappedEnumType>(WrappedType);

		FPyWrapperEnumArrayView EnumEntriesArray;
		if (WrappedEnumType)
		{
			EnumEntriesArray = WrappedEnumType->PyEnumEntries;
		}

		FPyWrapperEnumIteratorPtr NewIter = FPyWrapperEnumIteratorPtr::StealReference(FUPyWrapperEnumIterator::New(&UPyWrapperEnumIteratorType));
		if (FUPyWrapperEnumIterator::Init(NewIter, EnumEntriesArray) != 0)
		{
			return nullptr;
		}

		return (PyObject*)NewIter.Release();
	}
};

struct FSequenceFuncs_EnumMetaclass
{
	static Py_ssize_t Len(PyTypeObject* InSelf)
	{
		// if (const FPyWrapperEnumMetaData* EnumMetaData = FPyWrapperEnumMetaData::GetMetaData(InSelf))
		// {
		// 	return EnumMetaData->EnumEntries.Num();
		// }
		return 0;
	}
};

PyTypeObject* InitializeUPyWrapperEnumMetaclassType()
{
	PyTypeObject* PyType = &UPyWrapperEnumMetaclassType;
	PyType->tp_base = &PyType_Type;
	PyType->tp_iter = (getiterfunc)&FFuncs_EnumMetaclass::GetIter;

	PyType->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
	PyType->tp_doc = "Metaclass type for all Unreal exposed enum instances";

	static PySequenceMethods PySequence;
	PySequence.sq_length = (lenfunc)&FSequenceFuncs_EnumMetaclass::Len;

	PyType->tp_as_sequence = &PySequence;

	return PyType;
}

// ==================== Wrapper EnumEnumIterator Type END ====================
struct FFuncs_EnumIterator
{
	static PyObject* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
	{
		return (PyObject*)FUPyWrapperEnumIterator::New(InType);
	}

	static void Dealloc(FUPyWrapperEnumIterator* InSelf)
	{
		FUPyWrapperEnumIterator::Free(InSelf);
	}

	static int Init(FUPyWrapperEnumIterator* InSelf, PyObject* InArgs, PyObject* InKwds)
	{
		PyObject* PyObj = nullptr;
		if (!PyArg_ParseTuple(InArgs, "O:call", &PyObj))
		{
			return -1;
		}

		if (PyObject_IsInstance(PyObj, (PyObject*)&UPyWrapperEnumType) != 1)
		{
			UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Cannot initialize '%s' with an instance of '%s'"), *UPyUtil::GetFriendlyTypename(InSelf), *UPyUtil::GetFriendlyTypename(PyObj)));
			return -1;
		}

		const UEnum* RequiredEnum = FUPyWrapperTypeRegistry::Get().FindEnum(Py_TYPE(PyObj));
		TSharedPtr<UPyGenUtil::FGeneratedWrappedType> WrappedType = FUPyWrapperTypeRegistry::Get().GetGeneratedWrappedType(RequiredEnum);
		TSharedPtr<FUPyGeneratedWrappedEnumType> WrappedEnumType = StaticCastSharedPtr<FUPyGeneratedWrappedEnumType>(WrappedType);

		FPyWrapperEnumArrayView EnumEntriesArray;
		if (WrappedEnumType)
		{
			EnumEntriesArray = WrappedEnumType->PyEnumEntries;
		}

		return FUPyWrapperEnumIterator::Init(InSelf, EnumEntriesArray);
	}

	static FUPyWrapperEnumIterator* GetIter(FUPyWrapperEnumIterator* InSelf)
	{
		return FUPyWrapperEnumIterator::GetIter(InSelf);
	}

	static PyObject* IterNext(FUPyWrapperEnumIterator* InSelf)
	{
		return FUPyWrapperEnumIterator::IterNext(InSelf);
	}
};

PyTypeObject* InitializeUPyWrapperEnumIteratorType()
{
	PyTypeObject* PyType = &UPyWrapperEnumIteratorType;
	PyType->tp_new = (newfunc)&FFuncs_EnumIterator::New;
	PyType->tp_dealloc = (destructor)&FFuncs_EnumIterator::Dealloc;
	PyType->tp_init = (initproc)&FFuncs_EnumIterator::Init;
	PyType->tp_iter = (getiterfunc)&FFuncs_EnumIterator::GetIter;
	PyType->tp_iternext = (iternextfunc)&FFuncs_EnumIterator::IterNext;

	PyType->tp_flags = Py_TPFLAGS_DEFAULT;
	PyType->tp_doc = "Type for all Unreal exposed enum iterators";

	return PyType;
}
// ==================== Wrapper EnumEnumIterator Type END ====================

// ==================== Wrapper EnumEnum Type BEGIN ====================

static PyObject* New_WrapperEnum(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
{
	return (PyObject*)FUPyWrapperEnum::New(InType);
}

static void Dealloc_WrapperEnum(FUPyWrapperEnum* InSelf)
{
	FUPyWrapperEnum::Free(InSelf);
}

static int Init_WrapperEnum(FUPyWrapperEnum* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	return FUPyWrapperEnum::Init(InSelf);
}

static PyObject* Str_WrapperEnum(FUPyWrapperEnum* InSelf)
{
	if (!FUPyWrapperEnum::ValidateInternalState(InSelf))
	{
		return nullptr;
	}

	return PyUnicode_FromFormat("<%s.%s: %d>", Py_TYPE(InSelf)->tp_name, TCHAR_TO_UTF8(*FUPyWrapperEnum::GetEnumEntryName(InSelf)), FUPyWrapperEnum::GetEnumEntryValue(InSelf));
}

static PyObject* RichCmp_WrapperEnum(FUPyWrapperEnum* InSelf, PyObject* InOther, int InOp)
{
	if (!FUPyWrapperEnum::ValidateInternalState(InSelf))
	{
		return nullptr;
	}

	FUPyWrapperEnumPtr Other = FUPyWrapperEnumPtr::StealReference(FUPyWrapperEnum::CastPyObject(InOther, Py_TYPE(InSelf)));
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

	// Compare the objects if both enums are the same type, otherwise compare the values (as cast must have returned a deprecated enum and the entry objects won't match)
	const bool bIsIdentical = (Py_TYPE(InSelf) == Py_TYPE(Other.GetPtr())) ? InSelf->EntryValue == Other->EntryValue : FUPyWrapperEnum::GetEnumEntryValue(InSelf) == FUPyWrapperEnum::GetEnumEntryValue(Other);
	return PyBool_FromLong(InOp == Py_EQ ? bIsIdentical : !bIsIdentical);
}

static Py_hash_t Hash_WrapperEnum(FUPyWrapperEnum* InSelf)
{
	if (!FUPyWrapperEnum::ValidateInternalState(InSelf))
	{
		return -1;
	}

	return PyObject_Hash(InSelf->EntryValue);
}

static PyMemberDef PyMembers[] = {
	{ UPyCStrCast("name"), T_OBJECT, STRUCT_OFFSET(FUPyWrapperEnum, EntryName), READONLY, UPyCStrCast("The name of this enum entry") },
	{ UPyCStrCast("value"), T_OBJECT, STRUCT_OFFSET(FUPyWrapperEnum, EntryValue), READONLY, UPyCStrCast("The numeric value of this enum entry") },
	{ nullptr, 0, 0, 0, nullptr }
};

// NOTE: _T is defines as: _T = TypeVar('_T') and Type is defined by the Python typing module.
static PyMethodDef PyMethods[] = {
	// { "cast", UPyCFunctionCast(&FMethods::Cast), METH_VARARGS | METH_CLASS, "cast(cls: Type[_T], object: object) -> _T -- cast the given object to this Unreal enum type" },
	// { "static_enum", UPyCFunctionCast(&FMethods::StaticEnum), METH_NOARGS | METH_CLASS, "static_enum(cls) -> Enum -- get the Unreal enum of this type" },
	// { "get_display_name", UPyCFunctionCast(&FMethods::GetDisplayName), METH_NOARGS, "get_display_name(self) -> Text -- get the UMETA display name of this type in the current culture" },
	{ nullptr, nullptr, 0, nullptr }
};


void InitializeUPyWrapperEnum(UPyGenUtil::FNativePythonModule& ModuleInfo)
{
	InitializeUPyWrapperEnumValueDescrType();
	InitializeUPyWrapperEnumMetaclassType();
	InitializeUPyWrapperEnumIteratorType();

	PyType_Ready(&UPyWrapperEnumIteratorType);
	PyType_Ready(&UPyWrapperEnumMetaclassType);

	if (PyType_Ready(&UPyWrapperEnumValueDescrType) == 0)
	{
		ModuleInfo.AddType(&UPyWrapperEnumValueDescrType, true);
	}

	PyTypeObject* PyType = &UPyWrapperEnumType;

	PyType->tp_base = &UPyWrapperBaseType;
	PyType->tp_new = (newfunc)&New_WrapperEnum;
	PyType->tp_dealloc = (destructor)&Dealloc_WrapperEnum;
	PyType->tp_init = (initproc)&Init_WrapperEnum;
	PyType->tp_str = (reprfunc)&Str_WrapperEnum;
	PyType->tp_repr = (reprfunc)&Str_WrapperEnum;
	PyType->tp_richcompare = (richcmpfunc)&RichCmp_WrapperEnum;
	PyType->tp_hash = (hashfunc)&Hash_WrapperEnum;

	PyType->tp_members = PyMembers;
	PyType->tp_methods = PyMethods;

	PyType->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
	PyType->tp_doc = "Type for all Unreal exposed enum instances";

	// Set the metaclass on the enum type
	Py_SET_TYPE(PyType, &UPyWrapperEnumMetaclassType);

	if (PyType_Ready(PyType) == 0)
	{
		ModuleInfo.AddType(PyType, true);
	}
}
// ==================== Wrapper EnumEnum Type END ====================
