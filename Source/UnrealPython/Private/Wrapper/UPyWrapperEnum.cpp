
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
	0, /* tp_basicsize */
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
	return New(InType, 0);
}

FUPyWrapperEnum* FUPyWrapperEnum::New(PyTypeObject* InType, const int64 InEnumEntryValue)
{
	FUPyObjectPtr PyValueArgs = FUPyObjectPtr::StealReference(Py_BuildValue("(L)", static_cast<long long>(InEnumEntryValue)));
	if (!PyValueArgs)
	{
		return nullptr;
	}

	return (FUPyWrapperEnum*)PyLong_Type.tp_new(InType, PyValueArgs, nullptr);
}

int FUPyWrapperEnum::Init(FUPyWrapperEnum* InSelf)
{
	UPyUtil::SetPythonError(PyExc_Exception, InSelf, TEXT("Cannot create instances of enum types"));
	return -1;
}

int FUPyWrapperEnum::Init(FUPyWrapperEnum* InSelf, const int64 InEnumEntryValue, const char* InEnumEntryName)
{
	(void)InEnumEntryName;

	if (!InSelf || !PyLong_Check((PyObject*)InSelf))
	{
		UPyUtil::SetPythonError(PyExc_Exception, InSelf ? Py_TYPE(InSelf) : &UPyWrapperEnumType, TEXT("Internal Error - enum entry is not a PyLong instance!"));
		return -1;
	}

	const int64 ActualValue = GetEnumEntryValue(InSelf);
	if (ActualValue != InEnumEntryValue)
	{
		UPyUtil::SetPythonError(PyExc_Exception, Py_TYPE(InSelf), TEXT("Internal Error - enum entry value was not initialized correctly!"));
		return -1;
	}
	return 0;
}

bool FUPyWrapperEnum::ValidateInternalState(FUPyWrapperEnum* InSelf)
{
	if (!InSelf || !PyObject_TypeCheck((PyObject*)InSelf, &UPyWrapperEnumType))
	{
		UPyUtil::SetPythonError(PyExc_Exception, InSelf ? Py_TYPE(InSelf) : &UPyWrapperEnumType, TEXT("Internal Error - object is not an enum entry!"));
		return false;
	}

	if (!PyLong_Check((PyObject*)InSelf))
	{
		UPyUtil::SetPythonError(PyExc_Exception, Py_TYPE(InSelf), TEXT("Internal Error - enum entry is not a PyLong instance!"));
		return false;
	}

	return true;
}

FUPyWrapperEnum* FUPyWrapperEnum::CastPyObject(PyObject* InPyObject, FUPyConversionResult* OutCastResult)
{
	SetOptionalUPyConversionResult(FUPyConversionResult::Failure(), OutCastResult);

	if (PyObject_TypeCheck(InPyObject, &UPyWrapperEnumType))
	{
		FUPyWrapperEnum* Self = (FUPyWrapperEnum*)InPyObject;
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

FUPyWrapperEnum* FUPyWrapperEnum::CastPyObject(PyObject* InPyObject, const PyTypeObject* InType, FUPyConversionResult* OutCastResult)
{
	SetOptionalUPyConversionResult(FUPyConversionResult::Failure(), OutCastResult);

	if (InType && PyObject_TypeCheck(InPyObject, (PyTypeObject*)InType) && (InType == &UPyWrapperEnumType || PyObject_TypeCheck(InPyObject, &UPyWrapperEnumType)))
	{
		FUPyWrapperEnum* Self = (FUPyWrapperEnum*)InPyObject;
		if (!ValidateInternalState(Self))
		{
			return nullptr;
		}

		SetOptionalUPyConversionResult(Py_TYPE(InPyObject) == InType ? FUPyConversionResult::Success() : FUPyConversionResult::SuccessWithCoercion(), OutCastResult);

		Py_INCREF(InPyObject);
		return Self;
	}

	// Allow casting from an alias enum type using the same UEnum.
	const UEnum* RequiredEnum = InType ? FUPyWrapperTypeRegistry::Get().FindEnum(InType) : nullptr;

	if (PyObject_TypeCheck(InPyObject, &UPyWrapperEnumType))
	{
		FUPyWrapperEnum* Self = (FUPyWrapperEnum*)InPyObject;
		if (!ValidateInternalState(Self))
		{
			return nullptr;
		}

		const UEnum* ActualEnum = FUPyWrapperTypeRegistry::Get().FindEnum(Py_TYPE(InPyObject));

		if (RequiredEnum == ActualEnum)
		{
			SetOptionalUPyConversionResult(FUPyConversionResult::Success(), OutCastResult);

			Py_INCREF(InPyObject);
			return Self;
		}
	}

	// Allow coerced casting from a numeric value
	int64 OtherVal = 0;
	if (InType && UPyConversion::Nativize(InPyObject, OtherVal))
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
		else if (FUPyWrapperEnum* PyEnumEntry = FindEnumEntryByValue(InType, OtherVal))
		{
			SetOptionalUPyConversionResult(FUPyConversionResult::SuccessWithCoercion(), OutCastResult);

			Py_INCREF(PyEnumEntry);
			return PyEnumEntry;
		}
	}

	return nullptr;
}

FString FUPyWrapperEnum::GetEnumEntryName(FUPyWrapperEnum* InSelf)
{
	if (!InSelf || !Py_TYPE(InSelf) || !Py_TYPE(InSelf)->tp_dict)
	{
		return FString();
	}

	PyObject* Key = nullptr;
	PyObject* Value = nullptr;
	Py_ssize_t Pos = 0;
	while (PyDict_Next(Py_TYPE(InSelf)->tp_dict, &Pos, &Key, &Value))
	{
		if (Value != (PyObject*)InSelf)
		{
			continue;
		}

		return UPyUtil::PyObjectToUEString(Key);
	}

	return FString();
}

int64 FUPyWrapperEnum::GetEnumEntryValue(FUPyWrapperEnum* InSelf)
{
	if (!InSelf)
	{
		return 0;
	}

	const int64 EnumEntryValueNum = PyLong_AsLongLong((PyObject*)InSelf);
	if (PyErr_Occurred())
	{
		PyErr_Clear();
		return 0;
	}
	return EnumEntryValueNum;
}

bool FUPyWrapperEnum::IsEnumEntryForType(PyObject* InPyObject, const PyTypeObject* InType)
{
	return InPyObject && InType && PyObject_TypeCheck(InPyObject, (PyTypeObject*)InType) && PyObject_TypeCheck(InPyObject, &UPyWrapperEnumType);
}

FUPyWrapperEnum* FUPyWrapperEnum::FindEnumEntryByValue(const PyTypeObject* InType, const int64 InEnumEntryValue)
{
	if (!InType || !InType->tp_dict)
	{
		return nullptr;
	}

	PyObject* Key = nullptr;
	PyObject* Value = nullptr;
	Py_ssize_t Pos = 0;
	while (PyDict_Next(InType->tp_dict, &Pos, &Key, &Value))
	{
		if (!IsEnumEntryForType(Value, InType))
		{
			continue;
		}

		FUPyWrapperEnum* PyEnumEntry = (FUPyWrapperEnum*)Value;
		if (GetEnumEntryValue(PyEnumEntry) == InEnumEntryValue)
		{
			return PyEnumEntry;
		}
	}

	return nullptr;
}

int32 FUPyWrapperEnum::AppendEnumEntries(PyTypeObject* InType, TArray<FUPyWrapperEnum*>& OutEnumEntries)
{
	if (!InType || !InType->tp_dict)
	{
		return 0;
	}

	const int32 InitialNum = OutEnumEntries.Num();
	PyObject* Key = nullptr;
	PyObject* Value = nullptr;
	Py_ssize_t Pos = 0;
	while (PyDict_Next(InType->tp_dict, &Pos, &Key, &Value))
	{
		if (IsEnumEntryForType(Value, InType))
		{
			OutEnumEntries.Add((FUPyWrapperEnum*)Value);
		}
	}

	return OutEnumEntries.Num() - InitialNum;
}

FUPyWrapperEnum* FUPyWrapperEnum::AddEnumEntry(PyTypeObject* InType, const int64 InEnumEntryValue, const char* InEnumEntryName)
{
	// if (!FPyWrapperEnumMetaData::IsEnumFinalized(InType))
	{
		FUPyWrapperEnumPtr PyEnumEntry = FUPyWrapperEnumPtr::StealReference(FUPyWrapperEnum::New(InType, InEnumEntryValue));
		if (PyEnumEntry && FUPyWrapperEnum::Init(PyEnumEntry, InEnumEntryValue, InEnumEntryName) == 0)
		{
			if (PyDict_SetItemString(InType->tp_dict, InEnumEntryName, (PyObject*)PyEnumEntry.GetPtr()) == 0)
			{
				return PyEnumEntry.GetPtr();
			}
		}
	}
	return nullptr;
}

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
		const int InitResult = WrappedEnumType
			? FUPyWrapperEnumIterator::Init(NewIter, EnumEntriesArray)
			: FUPyWrapperEnumIterator::Init(NewIter, InSelf);
		if (InitResult != 0)
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
		const UEnum* RequiredEnum = FUPyWrapperTypeRegistry::Get().FindEnum(InSelf);
		TSharedPtr<UPyGenUtil::FGeneratedWrappedType> WrappedType = FUPyWrapperTypeRegistry::Get().GetGeneratedWrappedType(RequiredEnum);
		TSharedPtr<FUPyGeneratedWrappedEnumType> WrappedEnumType = StaticCastSharedPtr<FUPyGeneratedWrappedEnumType>(WrappedType);
		if (WrappedEnumType)
		{
			return WrappedEnumType->PyEnumEntries.Num();
		}

		TArray<FUPyWrapperEnum*> EnumEntries;
		return FUPyWrapperEnum::AppendEnumEntries(InSelf, EnumEntries);
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

		return WrappedEnumType
			? FUPyWrapperEnumIterator::Init(InSelf, EnumEntriesArray)
			: FUPyWrapperEnumIterator::Init(InSelf, Py_TYPE(PyObj));
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

struct FGetSets_WrapperEnum
{
	static PyObject* GetName(FUPyWrapperEnum* InSelf, void* InClosure)
	{
		if (!FUPyWrapperEnum::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		return PyUnicode_FromString(TCHAR_TO_UTF8(*FUPyWrapperEnum::GetEnumEntryName(InSelf)));
	}

	static PyObject* GetValue(FUPyWrapperEnum* InSelf, void* InClosure)
	{
		if (!FUPyWrapperEnum::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		return PyLong_FromLongLong(FUPyWrapperEnum::GetEnumEntryValue(InSelf));
	}
};

static PyGetSetDef PyGetSets_WrapperEnum[] = {
	{ UPyCStrCast("name"), (getter)&FGetSets_WrapperEnum::GetName, nullptr, UPyCStrCast("The name of this enum entry"), nullptr },
	{ UPyCStrCast("value"), (getter)&FGetSets_WrapperEnum::GetValue, nullptr, UPyCStrCast("The numeric value of this enum entry"), nullptr },
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
	InitializeUPyWrapperEnumMetaclassType();
	InitializeUPyWrapperEnumIteratorType();

	PyType_Ready(&UPyWrapperEnumIteratorType);
	PyType_Ready(&UPyWrapperEnumMetaclassType);

	PyTypeObject* PyType = &UPyWrapperEnumType;

	PyType->tp_base = &PyLong_Type;
	PyType->tp_new = (newfunc)&New_WrapperEnum;
	PyType->tp_init = (initproc)&Init_WrapperEnum;
	PyType->tp_str = (reprfunc)&Str_WrapperEnum;
	PyType->tp_repr = (reprfunc)&Str_WrapperEnum;

	PyType->tp_getset = PyGetSets_WrapperEnum;
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
