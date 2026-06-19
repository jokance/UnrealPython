
#include "Wrapper/UPyWrapperFieldPath.h"
#include "Wrapper/UPyWrapperTypeRegistry.h"
#include "Wrapper/UPyWrapperTypeFactory.h"
#include "Core/UPyConversion.h"
#include "UObject/UnrealType.h"
#include "UObject/Package.h"
#include "UObject/PropertyPortFlags.h"

PyTypeObject UPyWrapperFieldPathType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"FieldPath", /* tp_name */
	sizeof(FUPyWrapperFieldPath), /* tp_basicsize */
};

void FUPyWrapperFieldPath::InitValue(FUPyWrapperFieldPath* InSelf, const FFieldPath& InValue)
{
	Super::InitValue(InSelf, InValue);
	// FUPyWrapperFieldPathFactory::Get().MapInstance(InSelf->Value, InSelf);
}


void FUPyWrapperFieldPath::DeinitValue(FUPyWrapperFieldPath* InSelf)
{
	// FUPyWrapperFieldPathFactory::Get().UnmapInstance(InSelf->Value);
	Super::DeinitValue(InSelf);
}

FUPyWrapperFieldPath* FUPyWrapperFieldPath::CastPyObject(PyObject* InPyObject, FUPyConversionResult* OutCastResult)
{
	SetOptionalUPyConversionResult(FUPyConversionResult::Failure(), OutCastResult);

	if (PyObject_IsInstance(InPyObject, (PyObject*)&UPyWrapperFieldPathType) == 1)
	{
		SetOptionalUPyConversionResult(FUPyConversionResult::Success(), OutCastResult);

		Py_INCREF(InPyObject);
		return (FUPyWrapperFieldPath*)InPyObject;
	}

	return nullptr;
}

FUPyWrapperFieldPath* FUPyWrapperFieldPath::CastPyObject(PyObject* InPyObject, PyTypeObject* InType, FUPyConversionResult* OutCastResult)
{
	SetOptionalUPyConversionResult(FUPyConversionResult::Failure(), OutCastResult);

	if (PyObject_IsInstance(InPyObject, (PyObject*)InType) == 1 && (InType == &UPyWrapperFieldPathType || PyObject_IsInstance(InPyObject, (PyObject*)&UPyWrapperFieldPathType) == 1))
	{
		SetOptionalUPyConversionResult(Py_TYPE(InPyObject) == InType ? FUPyConversionResult::Success() : FUPyConversionResult::SuccessWithCoercion(), OutCastResult);

		Py_INCREF(InPyObject);
		return (FUPyWrapperFieldPath*)InPyObject;
	}

	{
		FFieldPath InitValue;
		if (UPyConversion::Nativize(InPyObject, InitValue, UPyConversion::ESetErrorState::No))
		{
			FUPyWrapperFieldPathPtr NewFiedPath = FUPyWrapperFieldPathPtr::StealReference(FUPyWrapperFieldPath::New(InType));
			if (NewFiedPath)
			{
				if (FUPyWrapperFieldPath::Init(NewFiedPath, InitValue) != 0)
				{
					return nullptr;
				}
			}
			SetOptionalUPyConversionResult(FUPyConversionResult::SuccessWithCoercion(), OutCastResult);
			return NewFiedPath.Release();
		}
	}

	{
		FString InitPath;
		if (UPyConversion::Nativize(InPyObject, InitPath, UPyConversion::ESetErrorState::No))
		{
			FUPyWrapperFieldPathPtr NewFiedPath = FUPyWrapperFieldPathPtr::StealReference(FUPyWrapperFieldPath::New(InType));
			if (NewFiedPath)
			{
				FFieldPath InitValue;
				InitValue.Generate(*InitPath);
				if (FUPyWrapperFieldPath::Init(NewFiedPath, InitValue) != 0)
				{
					return nullptr;
				}
			}
			SetOptionalUPyConversionResult(FUPyConversionResult::SuccessWithCoercion(), OutCastResult);
			return NewFiedPath.Release();
		}
	}

	return nullptr;
}

struct FFuncs_WrapperFieldPath
{
	static int Init(FUPyWrapperFieldPath* InSelf, PyObject* InArgs, PyObject* InKwds)
	{
		PyObject* PyObj = nullptr;
		if (!PyArg_ParseTuple(InArgs, "|O:call", &PyObj))
		{
			return -1;
		}

		FFieldPath InitValue;
		FString StrValue;
		if (PyObj && UPyConversion::Nativize(PyObj, StrValue, UPyConversion::ESetErrorState::No)) // Init from a string.
		{
			InitValue.Generate(*StrValue);
		}
		else if (PyObj && !UPyConversion::Nativize(PyObj, InitValue)) // Init from another FieldPath.
		{
			UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert init argument '%s' to 'FieldPath'"), *UPyUtil::GetFriendlyTypename(PyObj)));
			return -1;
		}
		// else: Default initialization. "f = unreal.FieldPath()".

		return FUPyWrapperFieldPath::Init(InSelf, InitValue);
	}

	static PyObject* Str(FUPyWrapperFieldPath* InSelf)
	{
		return PyUnicode_FromString(TCHAR_TO_UTF8(*InSelf->Value.ToString()));
	}

	static PyObject* Repr(FUPyWrapperFieldPath* InSelf)
	{
		return PyUnicode_FromString(TCHAR_TO_UTF8(*FString::Printf(TEXT("FieldPath(\"%s\")"), *InSelf->Value.ToString())));
	}

	static PyObject* RichCmp(FUPyWrapperFieldPath* InSelf, PyObject* InOther, int InOp)
	{
		FFieldPath Other;
		if (!UPyConversion::Nativize(InOther, Other))
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

		bool bIdentical = InSelf->Value == Other;
		return PyBool_FromLong(InOp == Py_EQ ? bIdentical : !bIdentical);
	}

	static Py_hash_t Hash(FUPyWrapperFieldPath* InSelf)
	{
		uint32 FieldPathHash = GetTypeHash(InSelf->Value);
		return FieldPathHash != -1 ? FieldPathHash : 0;
	}
};


struct FMethods_WrapperFieldPath
{
	static PyObject* Cast(PyTypeObject* InType, PyObject* InArgs)
	{
		PyObject* PyObj = nullptr;
		if (PyArg_ParseTuple(InArgs, "O:call", &PyObj))
		{
			PyObject* PyCastResult = (PyObject*)FUPyWrapperFieldPath::CastPyObject(PyObj, InType);
			if (!PyCastResult)
			{
				UPyUtil::SetPythonError(PyExc_TypeError, InType, *FString::Printf(TEXT("Cannot cast type '%s' to '%s'"), *UPyUtil::GetFriendlyTypename(PyObj), *UPyUtil::GetFriendlyTypename(InType)));
			}
			return PyCastResult;
		}

		return nullptr;
	}

	static PyObject* Copy(FUPyWrapperFieldPath* InSelf)
	{
		return UPyConversion::Pythonize(InSelf->Value);
	}

	static PyObject* IsValid(FUPyWrapperFieldPath* InSelf)
	{
		return UPyConversion::Pythonize(InSelf->Value.TryToResolvePath(nullptr) != nullptr);
	}
};

PyMethodDef FieldPathPyMethods[] = {
	{ "Cast", UPyCFunctionCast(&FMethods_WrapperFieldPath::Cast), METH_VARARGS | METH_CLASS, UPyDoc_STR("Cast(cls, obj: object) -> FieldPath -- cast the given object to this Unreal field path type") },
	{ "__copy__", UPyCFunctionCast(&FMethods_WrapperFieldPath::Copy), METH_NOARGS, UPyDoc_STR("__copy__(self) -> FieldPath -- copy this Unreal field path") },
	{ "Copy", UPyCFunctionCast(&FMethods_WrapperFieldPath::Copy), METH_NOARGS, UPyDoc_STR("Copy(self) -> FieldPath -- copy this Unreal field path") },
	{ "IsValid", UPyCFunctionCast(&FMethods_WrapperFieldPath::IsValid), METH_NOARGS, UPyDoc_STR("IsValid(self) -> bool -- whether this Unreal field path refers to an existing Unreal field") },
	{ nullptr, nullptr, 0, nullptr }
};

void InitializeUPyWrapperFieldPath(UPyGenUtil::FNativePythonModule& ModuleInfo)
{
	PyTypeObject* PyType = &UPyWrapperFieldPathType;
	InitializeUPyWrapperBasicType<FUPyWrapperFieldPath>(PyType, "Type for all Unreal exposed FieldPath instances");

	PyType->tp_init = (initproc)&FFuncs_WrapperFieldPath::Init;
	PyType->tp_str = (reprfunc)&FFuncs_WrapperFieldPath::Str;
	PyType->tp_repr = (reprfunc)&FFuncs_WrapperFieldPath::Repr;
	PyType->tp_richcompare = (richcmpfunc)&FFuncs_WrapperFieldPath::RichCmp;
	PyType->tp_hash = (hashfunc)&FFuncs_WrapperFieldPath::Hash;

	PyType->tp_methods = FieldPathPyMethods;

	if (PyType_Ready(PyType) == 0)
	{
		ModuleInfo.AddType(PyType, true);
	}
}

