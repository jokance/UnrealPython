
#include "Core/UPyConversion.h"
#include "Wrapper/UPyWrapperOwnerContext.h"
#include "Core/UPyPtr.h"
#include "Utils/UPyUtil.h"
#include "Utils/UPyGenUtil.h"

#include "Wrapper/UPyWrapperObjectBase.h"
#include "Wrapper/UPyWrapperStruct.h"
#include "Wrapper/UPyWrapperEnum.h"
#include "Wrapper/UPyWrapperDelegate.h"
// #include "Wrapper/UPyWrapperName.h"
// #include "Wrapper/UPyWrapperText.h"
#include "Wrapper/UPyWrapperArray.h"
#include "Wrapper/UPyWrapperFixedArray.h"
#include "Wrapper/UPyWrapperSet.h"
#include "Wrapper/UPyWrapperMap.h"
#include "Wrapper/UPyWrapperFieldPath.h"
#include "Wrapper/UPyWrapperTypeRegistry.h"
#include "Wrapper/UPyWrapperTypeFactory.h"

#include "DynamicTypes/UPyGeneratedWrappedEnumType.h"

#include "Templates/Casts.h"
#include "UObject/UnrealType.h"
#include "UObject/EnumProperty.h"
#include "UObject/TextProperty.h"
#include "UObject/PropertyPortFlags.h"

#include <limits>

namespace UPyConversion
{

namespace Internal
{

FUPyConversionResult NativizeStructInstance(PyObject* PyObj, UScriptStruct* StructType, void* StructInstance, const ESetErrorState SetErrorState)
{
	FUPyConversionResult Result = FUPyConversionResult::Failure();

	const PyTypeObject* PyStructType = FUPyWrapperTypeRegistry::Get().GetWrappedStructType(StructType);
	FUPyWrapperStructPtr PyStruct = FUPyWrapperStructPtr::StealReference(FUPyWrapperStruct::CastPyObject(PyObj, (PyTypeObject*)PyStructType, &Result));
	if (PyStruct && ensureAlways(PyStruct->ScriptStruct->IsChildOf(StructType)))
	{
		StructType->CopyScriptStruct(StructInstance, PyStruct->StructInstance);
	}

	UPYCONVERSION_RETURN(Result, TEXT("NativizeStructInstance"), *FString::Printf(TEXT("Cannot nativize '%s' as '%s'"), *UPyUtil::GetFriendlyTypename(PyObj), *UPyUtil::GetFriendlyTypename(PyStructType)));
}

FUPyConversionResult PythonizeStructInstance(UScriptStruct* StructType, const void* StructInstance, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
{
	OutPyObj = (PyObject*)FUPyWrapperStructFactory::Get().CreateInstance(StructType, (void*)StructInstance, FUPyWrapperOwnerContext(), EUPyConversionMethod::Copy);
	return FUPyConversionResult::Success();
}

FUPyConversionResult NativizeError(PyObject* PyObj, const ESetErrorState SetErrorState, const TCHAR* InErrorType, const TCHAR* InReason = nullptr)
{
	FString ErrorMessage = FString::Printf(TEXT("Cannot nativize '%s' as '%s'"), *UPyUtil::GetFriendlyTypename(PyObj), InErrorType);
	if (InReason)
	{
		ErrorMessage += FString::Printf(TEXT(": %s"), InReason);
	}

	UPYCONVERSION_RETURN(FUPyConversionResult::Failure(), TEXT("Nativize"), *ErrorMessage);
}

PyObject* ConvertFloatToLong(PyObject* PyObj, FUPyObjectPtr& OutOwnedLong)
{
	check(PyFloat_Check(PyObj));

	const double PyDouble = PyFloat_AsDouble(PyObj);
	if (PyErr_Occurred())
	{
		return nullptr;
	}

	OutOwnedLong = FUPyObjectPtr::StealReference(PyLong_FromDouble(PyDouble));
	return OutOwnedLong.Get();
}

template <typename T>
FUPyConversionResult NativizeSigned(PyObject* PyObj, T& OutVal, const ESetErrorState SetErrorState, const TCHAR* InErrorType)
{
	// Booleans subclass integer, so exclude those explicitly
	if (!PyBool_Check(PyObj))
	{
		if (PyLong_Check(PyObj))
		{
			const long long PyVal = PyLong_AsLongLong(PyObj);
			if (PyErr_Occurred())
			{
				return NativizeError(PyObj, SetErrorState, InErrorType, TEXT("value out of range"));
			}

			if (PyVal < static_cast<long long>(std::numeric_limits<T>::lowest()) || PyVal > static_cast<long long>(std::numeric_limits<T>::max()))
			{
				return NativizeError(PyObj, SetErrorState, InErrorType, TEXT("value out of range"));
			}

			OutVal = static_cast<T>(PyVal);
			return FUPyConversionResult::Success();
		}

		if (PyFloat_Check(PyObj))
		{
			FUPyObjectPtr PyLong;
			PyObject* PyLongObj = ConvertFloatToLong(PyObj, PyLong);
			if (!PyLongObj)
			{
				return NativizeError(PyObj, SetErrorState, InErrorType, TEXT("value out of range"));
			}

			const long long PyVal = PyLong_AsLongLong(PyLongObj);
			if (PyErr_Occurred())
			{
				return NativizeError(PyObj, SetErrorState, InErrorType, TEXT("value out of range"));
			}

			if (PyVal < static_cast<long long>(std::numeric_limits<T>::lowest()) || PyVal > static_cast<long long>(std::numeric_limits<T>::max()))
			{
				return NativizeError(PyObj, SetErrorState, InErrorType, TEXT("value out of range"));
			}

			OutVal = static_cast<T>(PyVal);
			return FUPyConversionResult::SuccessWithCoercion();
		}
	}

	return NativizeError(PyObj, SetErrorState, InErrorType);
}

template <typename T>
FUPyConversionResult NativizeUnsigned(PyObject* PyObj, T& OutVal, const ESetErrorState SetErrorState, const TCHAR* InErrorType)
{
	// Booleans subclass integer, so exclude those explicitly
	if (!PyBool_Check(PyObj))
	{
		if (PyLong_Check(PyObj))
		{
			const unsigned long long PyVal = PyLong_AsUnsignedLongLong(PyObj);
			if (PyErr_Occurred())
			{
				return NativizeError(PyObj, SetErrorState, InErrorType, TEXT("value out of range"));
			}

			if (PyVal > static_cast<unsigned long long>(std::numeric_limits<T>::max()))
			{
				return NativizeError(PyObj, SetErrorState, InErrorType, TEXT("value out of range"));
			}

			OutVal = static_cast<T>(PyVal);
			return FUPyConversionResult::Success();
		}

		if (PyFloat_Check(PyObj))
		{
			FUPyObjectPtr PyLong;
			PyObject* PyLongObj = ConvertFloatToLong(PyObj, PyLong);
			if (!PyLongObj)
			{
				return NativizeError(PyObj, SetErrorState, InErrorType, TEXT("value out of range"));
			}

			const unsigned long long PyVal = PyLong_AsUnsignedLongLong(PyLongObj);
			if (PyErr_Occurred())
			{
				return NativizeError(PyObj, SetErrorState, InErrorType, TEXT("value out of range"));
			}

			if (PyVal > static_cast<unsigned long long>(std::numeric_limits<T>::max()))
			{
				return NativizeError(PyObj, SetErrorState, InErrorType, TEXT("value out of range"));
			}

			OutVal = static_cast<T>(PyVal);
			return FUPyConversionResult::SuccessWithCoercion();
		}
	}

	return NativizeError(PyObj, SetErrorState, InErrorType);
}

template <typename T>
FUPyConversionResult NativizeReal(PyObject* PyObj, T& OutVal, const ESetErrorState SetErrorState, const TCHAR* InErrorType)
{
	// Booleans subclass integer, so exclude those explicitly
	if (!PyBool_Check(PyObj))
	{
		if (PyLong_Check(PyObj))
		{
			OutVal = PyLong_AsDouble(PyObj);
			return FUPyConversionResult::SuccessWithCoercion();
		}

		if (PyFloat_Check(PyObj))
		{
			OutVal = PyFloat_AsDouble(PyObj);
			return FUPyConversionResult::Success();
		}
	}

	UPYCONVERSION_RETURN(FUPyConversionResult::Failure(), TEXT("Nativize"), *FString::Printf(TEXT("Cannot nativize '%s' as '%s'"), *UPyUtil::GetFriendlyTypename(PyObj), InErrorType));
}

template <typename T>
FUPyConversionResult PythonizeSigned(const T Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState, const TCHAR* InErrorType)
{
	OutPyObj = PyLong_FromLongLong(Val);
	return FUPyConversionResult::Success();
}

template <typename T>
FUPyConversionResult PythonizeUnsigned(const T Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState, const TCHAR* InErrorType)
{
	OutPyObj = PyLong_FromUnsignedLongLong(Val);
	return FUPyConversionResult::Success();
}

template <typename T>
FUPyConversionResult PythonizeReal(const T Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState, const TCHAR* InErrorType)
{
	OutPyObj = PyFloat_FromDouble(Val);
	return FUPyConversionResult::Success();
}

} // namespace Internal

FUPyConversionResult Nativize(PyObject* PyObj, bool& OutVal, const ESetErrorState SetErrorState)
{
	if (PyObj == Py_True)
	{
		OutVal = true;
		return FUPyConversionResult::Success();
	}

	if (PyObj == Py_False)
	{
		OutVal = false;
		return FUPyConversionResult::Success();
	}

	if (PyObj == Py_None)
	{
		OutVal = false;
		return FUPyConversionResult::Success();
	}

	if (PyLong_Check(PyObj))
	{
		const int IsTrue = PyObject_IsTrue(PyObj);
		if (IsTrue == -1)
		{
			UPYCONVERSION_RETURN(FUPyConversionResult::Failure(), TEXT("Nativize"), *FString::Printf(TEXT("Cannot nativize '%s' as 'bool'"), *UPyUtil::GetFriendlyTypename(PyObj)));
		}

		OutVal = IsTrue == 1;
		return FUPyConversionResult::SuccessWithCoercion();
	}

	UPYCONVERSION_RETURN(FUPyConversionResult::Failure(), TEXT("Nativize"), *FString::Printf(TEXT("Cannot nativize '%s' as 'bool'"), *UPyUtil::GetFriendlyTypename(PyObj)));
}

FUPyConversionResult Pythonize(const bool Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
{
	if (Val)
	{
		Py_INCREF(Py_True);
		OutPyObj = Py_True;
	}
	else
	{
		Py_INCREF(Py_False);
		OutPyObj = Py_False;
	}

	return FUPyConversionResult::Success();
}

FUPyConversionResult Nativize(PyObject* PyObj, int8& OutVal, const ESetErrorState SetErrorState)
{
	return Internal::NativizeSigned(PyObj, OutVal, SetErrorState, TEXT("int8"));
}

FUPyConversionResult Pythonize(const int8 Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
{
	return Internal::PythonizeSigned(Val, OutPyObj, SetErrorState, TEXT("int8"));
}

FUPyConversionResult Nativize(PyObject* PyObj, uint8& OutVal, const ESetErrorState SetErrorState)
{
	return Internal::NativizeUnsigned(PyObj, OutVal, SetErrorState, TEXT("uint8"));
}

FUPyConversionResult Pythonize(const uint8 Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
{
	return Internal::PythonizeUnsigned(Val, OutPyObj, SetErrorState, TEXT("uint8"));
}

FUPyConversionResult Nativize(PyObject* PyObj, int16& OutVal, const ESetErrorState SetErrorState)
{
	return Internal::NativizeSigned(PyObj, OutVal, SetErrorState, TEXT("int16"));
}

FUPyConversionResult Pythonize(const int16 Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
{
	return Internal::PythonizeSigned(Val, OutPyObj, SetErrorState, TEXT("int16"));
}

FUPyConversionResult Nativize(PyObject* PyObj, uint16& OutVal, const ESetErrorState SetErrorState)
{
	return Internal::NativizeUnsigned(PyObj, OutVal, SetErrorState, TEXT("uint16"));
}

FUPyConversionResult Pythonize(const uint16 Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
{
	return Internal::PythonizeUnsigned(Val, OutPyObj, SetErrorState, TEXT("uint16"));
}

FUPyConversionResult Nativize(PyObject* PyObj, int32& OutVal, const ESetErrorState SetErrorState)
{
	return Internal::NativizeSigned(PyObj, OutVal, SetErrorState, TEXT("int32"));
}

FUPyConversionResult Pythonize(const int32 Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
{
	return Internal::PythonizeSigned(Val, OutPyObj, SetErrorState, TEXT("int32"));
}

FUPyConversionResult Nativize(PyObject* PyObj, uint32& OutVal, const ESetErrorState SetErrorState)
{
	return Internal::NativizeUnsigned(PyObj, OutVal, SetErrorState, TEXT("uint32"));
}

FUPyConversionResult Pythonize(const uint32 Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
{
	return Internal::PythonizeUnsigned(Val, OutPyObj, SetErrorState, TEXT("uint32"));
}

FUPyConversionResult Nativize(PyObject* PyObj, int64& OutVal, const ESetErrorState SetErrorState)
{
	return Internal::NativizeSigned(PyObj, OutVal, SetErrorState, TEXT("int64"));
}

FUPyConversionResult Pythonize(const int64 Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
{
	return Internal::PythonizeSigned(Val, OutPyObj, SetErrorState, TEXT("int64"));
}

FUPyConversionResult Nativize(PyObject* PyObj, uint64& OutVal, const ESetErrorState SetErrorState)
{
	return Internal::NativizeUnsigned(PyObj, OutVal, SetErrorState, TEXT("uint64"));
}

FUPyConversionResult Pythonize(const uint64 Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
{
	return Internal::PythonizeUnsigned(Val, OutPyObj, SetErrorState, TEXT("uint64"));
}

FUPyConversionResult Nativize(PyObject* PyObj, float& OutVal, const ESetErrorState SetErrorState)
{
	return Internal::NativizeReal(PyObj, OutVal, SetErrorState, TEXT("float"));
}

FUPyConversionResult Pythonize(const float Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
{
	return Internal::PythonizeReal(Val, OutPyObj, SetErrorState, TEXT("float"));
}

FUPyConversionResult Nativize(PyObject* PyObj, double& OutVal, const ESetErrorState SetErrorState)
{
	return Internal::NativizeReal(PyObj, OutVal, SetErrorState, TEXT("double"));
}

FUPyConversionResult Pythonize(const double Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
{
	return Internal::PythonizeReal(Val, OutPyObj, SetErrorState, TEXT("double"));
}

FUPyConversionResult Nativize(PyObject* PyObj, const char*& OutVal, const ESetErrorState SetErrorState)
{
	if (PyUnicode_Check(PyObj))
	{
		OutVal = PyUnicode_AsUTF8(PyObj);
		return FUPyConversionResult::Success();
	}
	if (PyBytes_Check(PyObj))
	{
		OutVal = PyBytes_AsString(PyObj);
		return FUPyConversionResult::Success();
	}
	UPYCONVERSION_RETURN(FUPyConversionResult::Failure(), TEXT("Nativize"), *FString::Printf(TEXT("Cannot nativize '%s' as 'const char*'"), *UPyUtil::GetFriendlyTypename(PyObj)));
}
	
FUPyConversionResult Pythonize(const char* Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
{
	if (Val)
	{
		OutPyObj = PyUnicode_FromString(Val);
	}
	else
	{
		Py_INCREF(Py_None);
		OutPyObj = Py_None;
	}
	return FUPyConversionResult::Success();
}
	
FUPyConversionResult Nativize(PyObject* PyObj, FString& OutVal, const ESetErrorState SetErrorState)
{
	const char* TempChar;
	if (Nativize(PyObj, TempChar))
	{
		OutVal = UTF8_TO_TCHAR(TempChar);
		return FUPyConversionResult::Success();
	}
	
	// if (PyUnicode_Check(PyObj))
	// {
	// 	FUPyObjectPtr PyBytesObj = FUPyObjectPtr::StealReference(PyUnicode_AsUTF8String(PyObj));
	// 	if (PyBytesObj)
	// 	{
	// 		const char* PyUtf8Buffer = PyBytes_AsString(PyBytesObj);
	// 		OutVal = UTF8_TO_TCHAR(PyUtf8Buffer);
	// 		return FUPyConversionResult::Success();
	// 	}
	// }

	// if (PyObject_IsInstance(PyObj, (PyObject*)&PyWrapperNameType) == 1)
	// {
	// 	FPyWrapperName* PyWrappedName = (FPyWrapperName*)PyObj;
	// 	OutVal = PyWrappedName->Value.ToString();
	// 	return FUPyConversionResult::SuccessWithCoercion();
	// }

	UPYCONVERSION_RETURN(FUPyConversionResult::Failure(), TEXT("Nativize"), *FString::Printf(TEXT("Cannot nativize '%s' as 'String'"), *UPyUtil::GetFriendlyTypename(PyObj)));
}

FUPyConversionResult Pythonize(const FString& Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
{
	OutPyObj = PyUnicode_FromString(TCHAR_TO_UTF8(*Val));
	return FUPyConversionResult::Success();
}

FUPyConversionResult Nativize(PyObject* PyObj, FName& OutVal, const ESetErrorState SetErrorState)
{
	// if (PyObject_IsInstance(PyObj, (PyObject*)&PyWrapperNameType) == 1)
	// {
	// 	FPyWrapperName* PyWrappedName = (FPyWrapperName*)PyObj;
	// 	OutVal = PyWrappedName->Value;
	// 	return FUPyConversionResult::Success();
	// }

	FString NameStr;
	if (Nativize(PyObj, NameStr, ESetErrorState::No))
	{
		if (NameStr.Len() >= NAME_SIZE)
		{
			UPYCONVERSION_RETURN(FUPyConversionResult::Failure(), TEXT("Nativize"), *FString::Printf(TEXT("Cannot nativize '%s' as 'Name' (lenth: %d, max length: %d)"), *UPyUtil::GetFriendlyTypename(PyObj), NameStr.Len(), NAME_SIZE - 1));
		}

		OutVal = *NameStr;
		return FUPyConversionResult::SuccessWithCoercion();
	}

	UPYCONVERSION_RETURN(FUPyConversionResult::Failure(), TEXT("Nativize"), *FString::Printf(TEXT("Cannot nativize '%s' as 'Name'"), *UPyUtil::GetFriendlyTypename(PyObj)));
}

FUPyConversionResult Pythonize(const FName& Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
{
	return Pythonize(Val.ToString(), OutPyObj);
	// OutPyObj = (PyObject*)FPyWrapperNameFactory::Get().CreateInstance(Val);
	// return FUPyConversionResult::Success();
}

FUPyConversionResult Nativize(PyObject* PyObj, FText& OutVal, const ESetErrorState SetErrorState)
{
	// if (PyObject_IsInstance(PyObj, (PyObject*)&PyWrapperTextType) == 1)
	// {
	// 	FPyWrapperText* PyWrappedText = (FPyWrapperText*)PyObj;
	// 	OutVal = PyWrappedText->Value;
	// 	return FUPyConversionResult::Success();
	// }

	FString TextStr;
	if (Nativize(PyObj, TextStr, ESetErrorState::No))
	{
		OutVal = FText::FromString(TextStr);
		return FUPyConversionResult::SuccessWithCoercion();
	}

	UPYCONVERSION_RETURN(FUPyConversionResult::Failure(), TEXT("Nativize"), *FString::Printf(TEXT("Cannot nativize '%s' as 'Text'"), *UPyUtil::GetFriendlyTypename(PyObj)));
}

FUPyConversionResult Pythonize(const FText& Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
{
	return Pythonize(Val.ToString(), OutPyObj);
	// OutPyObj = (PyObject*)FPyWrapperTextFactory::Get().CreateInstance(Val);
	// return FUPyConversionResult::Success();
}

FUPyConversionResult Nativize(PyObject* PyObj, FFieldPath& OutVal, const ESetErrorState SetErrorState)
{
	if (PyObject_IsInstance(PyObj, (PyObject*)&UPyWrapperFieldPathType) == 1)
	{
		FUPyWrapperFieldPath* PyWrappedFieldPath = (FUPyWrapperFieldPath*)PyObj;
		OutVal = PyWrappedFieldPath->Value;
		return FUPyConversionResult::Success();
	}

	UPYCONVERSION_RETURN(FUPyConversionResult::Failure(), TEXT("Nativize"), *FString::Printf(TEXT("Cannot nativize '%s' as 'FieldPath'"), *UPyUtil::GetFriendlyTypename(PyObj)));
}

FUPyConversionResult Pythonize(const FFieldPath& Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
{
	OutPyObj = (PyObject*)FUPyWrapperFieldPathFactory::Get().CreateInstance(Val);
	return FUPyConversionResult::Success();
}


FUPyConversionResult Nativize(PyObject* PyObj, void*& OutVal, const ESetErrorState SetErrorState)
{
	// CObject was removed in Python 3.2
#if !(PY_MAJOR_VERSION >= 3 && PY_MINOR_VERSION >= 2)
	if (PyCObject_Check(PyObj))
	{
		OutVal = PyCObject_AsVoidPtr(PyObj);
		return FUPyConversionResult::Success();
	}
#endif	// !(PY_MAJOR_VERSION >= 3 && PY_MINOR_VERSION >= 2)

	// Capsule was added in Python 2.7 and 3.1
#if PY_MAJOR_VERSION >= 3 && PY_MINOR_VERSION >= 1
	if (PyCapsule_CheckExact(PyObj))
	{
		OutVal = PyCapsule_GetPointer(PyObj, PyCapsule_GetName(PyObj));
		return FUPyConversionResult::Success();
	}
#endif	// PY_MAJOR_VERSION >= 3 && PY_MINOR_VERSION >= 1

	if (PyObj == Py_None)
	{
		OutVal = nullptr;
		return FUPyConversionResult::Success();
	}

	{
		uint64 PtrValue = 0;
		if (UPyConversion::Nativize(PyObj, PtrValue, ESetErrorState::No))
		{
			OutVal = (void*)PtrValue;
			return FUPyConversionResult::SuccessWithCoercion();
		}
	}

	UPYCONVERSION_RETURN(FUPyConversionResult::Failure(), TEXT("Nativize"), *FString::Printf(TEXT("Cannot nativize '%s' as 'void*'"), *UPyUtil::GetFriendlyTypename(PyObj)));
}

FUPyConversionResult Pythonize(void* Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
{
	if (Val)
	{
		// Use Capsule for Python 3.1+, and CObject for older versions
#if PY_MAJOR_VERSION >= 3 && PY_MINOR_VERSION >= 1
		OutPyObj = PyCapsule_New(Val, nullptr, nullptr);
#else	// PY_MAJOR_VERSION >= 3 && PY_MINOR_VERSION >= 1
		OutPyObj = PyCObject_FromVoidPtr(Val, nullptr);
#endif	// PY_MAJOR_VERSION >= 3 && PY_MINOR_VERSION >= 1
	}
	else
	{
		Py_INCREF(Py_None);
		OutPyObj = Py_None;
	}

	return FUPyConversionResult::Success();
}

FUPyConversionResult Nativize(PyObject* PyObj, UObject*& OutVal, const ESetErrorState SetErrorState)
{
	return NativizeObject(PyObj, OutVal, UObject::StaticClass(), SetErrorState);
}

FUPyConversionResult Pythonize(UObject* Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
{
	return PythonizeObject(Val, OutPyObj, SetErrorState);
}

FUPyConversionResult NativizeObject(PyObject* PyObj, UObject*& OutVal, UClass* ExpectedType, const ESetErrorState SetErrorState)
{
	auto IsObjectExpectedType = [ExpectedType](const UObject* ObjectInstance) -> bool
	{
		if (!ExpectedType)
		{
			return true;
		}

		return ExpectedType->HasAnyClassFlags(CLASS_Interface)
			? ObjectInstance->GetClass()->ImplementsInterface(ExpectedType)
			: ObjectInstance->IsA(ExpectedType);
	};

	if (PyObject_IsInstance(PyObj, (PyObject*)&UPyWrapperObjectBaseType) == 1)
	{
		FUPyWrapperObjectBase* PyWrappedObj = (FUPyWrapperObjectBase*)PyObj;
		if (FUPyWrapperObjectBase::ValidateInternalState(PyWrappedObj) && IsObjectExpectedType(PyWrappedObj->ObjectInstance))
		{
			OutVal = PyWrappedObj->ObjectInstance;
			return FUPyConversionResult::Success();
		}
	}

	if (PyObj == Py_None)
	{
		OutVal = nullptr;
		return FUPyConversionResult::Success();
	}

	UPYCONVERSION_RETURN(FUPyConversionResult::Failure(), TEXT("NativizeObject"), *FString::Printf(TEXT("Cannot nativize '%s' as 'Object' (allowed Class type: '%s')"), *UPyUtil::GetFriendlyTypename(PyObj), ExpectedType ? *ExpectedType->GetName() : TEXT("<any>")));
}

FUPyConversionResult PythonizeObject(UObject* Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
{
	if (Val)
	{
		OutPyObj = (PyObject*)FUPyWrapperObjectFactory::Get().CreateInstance(Val);
	}
	else
	{
		Py_INCREF(Py_None);
		OutPyObj = Py_None;
	}

	return FUPyConversionResult::Success();
}

PyObject* PythonizeObject(UObject* Val, const ESetErrorState SetErrorState)
{
	PyObject* Obj = nullptr;
	PythonizeObject(Val, Obj, SetErrorState);
	return Obj;
}

FUPyConversionResult NativizeClass(PyObject* PyObj, UClass*& OutVal, UClass* ExpectedType, const ESetErrorState SetErrorState)
{
	const UClass* Class = nullptr;

	if (PyType_Check(PyObj) && PyType_IsSubtype((PyTypeObject*)PyObj, &UPyWrapperObjectBaseType))
	{
		Class = FUPyWrapperTypeRegistry::Get().FindClass((PyTypeObject*)PyObj);
	}

	if (Class || NativizeObject(PyObj, (UObject*&)Class, UClass::StaticClass(), SetErrorState))
	{
		if (!Class || !ExpectedType || Class->IsChildOf(ExpectedType))
		{
			OutVal = (UClass*)Class;
			return FUPyConversionResult::Success();
		}
	}

	UPYCONVERSION_RETURN(FUPyConversionResult::Failure(), TEXT("NativizeClass"), *FString::Printf(TEXT("Cannot nativize '%s' as 'Class' (allowed Class type: '%s')"), *UPyUtil::GetFriendlyTypename(PyObj), ExpectedType ? *ExpectedType->GetName() : TEXT("<any>")));
}

FUPyConversionResult PythonizeClass(UClass* Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
{
	return PythonizeObject(Val, OutPyObj, SetErrorState);
}

PyObject* PythonizeClass(UClass* Val, const ESetErrorState SetErrorState)
{
	PyObject* Obj = nullptr;
	PythonizeClass(Val, Obj, SetErrorState);
	return Obj;
}

UClass* NativizeClass(PyObject* PyObj, UClass* ExpectedType, const ESetErrorState SetErrorState)
{
	UClass* Class = nullptr;
	NativizeClass(PyObj, Class, ExpectedType, SetErrorState);
	return Class;
}

FUPyConversionResult NativizeStruct(PyObject* PyObj, UScriptStruct*& OutVal, UScriptStruct* ExpectedType, const ESetErrorState SetErrorState)
{
	const UScriptStruct* Struct = nullptr;

	if (PyType_Check(PyObj) && PyType_IsSubtype((PyTypeObject*)PyObj, &UPyWrapperStructType))
	{
		Struct = FUPyWrapperTypeRegistry::Get().FindStruct((PyTypeObject*)PyObj);
	}

	if (Struct || NativizeObject(PyObj, (UObject*&)Struct, UScriptStruct::StaticClass(), SetErrorState))
	{
		if (!Struct || !ExpectedType || Struct->IsChildOf(ExpectedType))
		{
			OutVal = (UScriptStruct*)Struct;
			return FUPyConversionResult::Success();
		}
	}

	UPYCONVERSION_RETURN(FUPyConversionResult::Failure(), TEXT("NativizeStruct"), *FString::Printf(TEXT("Cannot nativize '%s' as 'Struct' (allowed Struct type: '%s')"), *UPyUtil::GetFriendlyTypename(PyObj), ExpectedType ? *ExpectedType->GetName() : TEXT("<any>")));
}

FUPyConversionResult PythonizeStruct(UScriptStruct* Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
{
	return PythonizeObject(Val, OutPyObj, SetErrorState);
}

PyObject* PythonizeStruct(UScriptStruct* Val, const ESetErrorState SetErrorState)
{
	PyObject* Obj = nullptr;
	PythonizeStruct(Val, Obj, SetErrorState);
	return Obj;
}

void EnsureWrappedBlueprintEnumType(const UEnum* EnumType)
{
	if (UPyGenUtil::IsBlueprintGeneratedEnum(EnumType))
	{
		// FUPyWrapperTypeRegistry& PyWrapperTypeRegistry = FUPyWrapperTypeRegistry::Get();

		// FUPyWrapperTypeRegistry::FGeneratedWrappedTypeReferences GeneratedWrappedTypeReferences;
		// TSet<FName> DirtyModules;
		//
		// PyWrapperTypeRegistry.GenerateWrappedTypeForObject(EnumType, GeneratedWrappedTypeReferences, DirtyModules, EPyTypeGenerationFlags::IncludeBlueprintGeneratedTypes);
		//
		// PyWrapperTypeRegistry.GenerateWrappedTypesForReferences(GeneratedWrappedTypeReferences, DirtyModules);
		// PyWrapperTypeRegistry.NotifyModulesDirtied(DirtyModules);
	}
}

FUPyConversionResult NativizeEnumEntry(PyObject* PyObj, const UEnum* EnumType, int64& OutVal, const ESetErrorState SetErrorState)
{
	FUPyConversionResult Result = FUPyConversionResult::Failure();

	EnsureWrappedBlueprintEnumType(EnumType);

	const PyTypeObject* PyEnumType = FUPyWrapperTypeRegistry::Get().GetWrappedEnumType(EnumType);
	FUPyWrapperEnumPtr PyEnum = FUPyWrapperEnumPtr::StealReference(FUPyWrapperEnum::CastPyObject(PyObj, PyEnumType, &Result));
	if (PyEnum)
	{
		OutVal = FUPyWrapperEnum::GetEnumEntryValue(PyEnum);
	}

	UPYCONVERSION_RETURN(Result, TEXT("NativizeEnumEntry"), *FString::Printf(TEXT("Cannot nativize '%s' as '%s'"), *UPyUtil::GetFriendlyTypename(PyObj), *UPyUtil::GetFriendlyTypename(PyEnumType)));
}

FUPyConversionResult PythonizeEnumEntry(const int64 Val, const UEnum* EnumType, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
{
	EnsureWrappedBlueprintEnumType(EnumType);

	const PyTypeObject* PyEnumType = FUPyWrapperTypeRegistry::Get().GetWrappedEnumType(EnumType);
	TSharedPtr<UPyGenUtil::FGeneratedWrappedType> WrappedType = FUPyWrapperTypeRegistry::Get().GetGeneratedWrappedType(EnumType);
	TSharedPtr<FUPyGeneratedWrappedEnumType> WrappedEnumType = StaticCastSharedPtr<FUPyGeneratedWrappedEnumType>(WrappedType);
		
	if (WrappedEnumType)
	{
		// Find an enum entry using this value
		for (FUPyWrapperEnum* PyEnumEntry : WrappedEnumType->PyEnumEntries)
		{
			const int64 EnumEntryVal = FUPyWrapperEnum::GetEnumEntryValue(PyEnumEntry);
			if (EnumEntryVal == Val)
			{
				Py_INCREF(PyEnumEntry);
				OutPyObj = (PyObject*)PyEnumEntry;
				return FUPyConversionResult::Success();
			}
		}
	}
	else if (PyEnumType)
	{
		PyObject* Key;
		PyObject* Value;
		Py_ssize_t Pos = 0;

		while (PyDict_Next(PyEnumType->tp_dict, &Pos, &Key, &Value))
		{
			if (Py_TYPE(Value) == &UPyWrapperEnumValueDescrType)
			{
				FUPyWrapperEnumValueDescrObject* PyEnumDescr = (FUPyWrapperEnumValueDescrObject*)Value;
				FUPyWrapperEnum* PyEnumEntry = PyEnumDescr->EnumEntry;
				const int64 EnumEntryVal = FUPyWrapperEnum::GetEnumEntryValue(PyEnumEntry);
				if (EnumEntryVal == Val)
				{
					Py_INCREF(PyEnumEntry);
					OutPyObj = (PyObject*)PyEnumEntry;
					return FUPyConversionResult::Success();
				}
			}
		}
	}

	UPYCONVERSION_RETURN(FUPyConversionResult::Failure(), TEXT("PythonizeEnumEntry"), *FString::Printf(TEXT("Cannot pythonize '%" INT64_FMT "' (int64) as '%s'"), Val, *UPyUtil::GetFriendlyTypename(PyEnumType)));
}

PyObject* PythonizeEnumEntry(const int64 Val, const UEnum* EnumType, const ESetErrorState SetErrorState)
{
	PyObject* Obj = nullptr;
	PythonizeEnumEntry(Val, EnumType, Obj, SetErrorState);
	return Obj;
}

FUPyConversionResult NativizeProperty(PyObject* PyObj, const FProperty* Prop, void* ValueAddr, const TConstArrayView<void*>& InArchetypeInstValueAddrs, const FPropertyAccessChangeNotify* InChangeNotify, const ESetErrorState SetErrorState)
{
#define UPYCONVERSION_PROPERTY_RETURN(RESULT) \
	UPYCONVERSION_RETURN(RESULT, TEXT("NativizeProperty"), *FString::Printf(TEXT("Cannot nativize '%s' as '%s' (%s)"), *UPyUtil::GetFriendlyTypename(PyObj), *Prop->GetName(), *Prop->GetClass()->GetName()))

	if (Prop->ArrayDim > 1)
	{
		FUPyWrapperFixedArrayPtr PyFixedArray = FUPyWrapperFixedArrayPtr::StealReference(FUPyWrapperFixedArray::CastPyObject(PyObj, &UPyWrapperFixedArrayType, Prop));
		if (PyFixedArray)
		{
			EmitPropertyChangeNotifications(InChangeNotify, /*bIdenticalValue*/false, [&]()
			{
				const int32 ArrSize = FMath::Min(Prop->ArrayDim, PyFixedArray->ArrayProp->ArrayDim);
				for (int32 ArrIndex = 0; ArrIndex < ArrSize; ++ArrIndex)
				{
					for (void* ArchInstValueAddr : InArchetypeInstValueAddrs)
					{
						Prop->CopySingleValue(static_cast<uint8*>(ArchInstValueAddr) + (Prop->GetElementSize() * ArrIndex), FUPyWrapperFixedArray::GetItemPtr(PyFixedArray, ArrIndex));
					}
					Prop->CopySingleValue(static_cast<uint8*>(ValueAddr) + (Prop->GetElementSize() * ArrIndex), FUPyWrapperFixedArray::GetItemPtr(PyFixedArray, ArrIndex));
				}
			});
			return FUPyConversionResult::Success();
		}

		UPYCONVERSION_PROPERTY_RETURN(FUPyConversionResult::Failure());
	}

	return NativizeProperty_Direct(PyObj, Prop, ValueAddr, InArchetypeInstValueAddrs, InChangeNotify, SetErrorState);

#undef UPYCONVERSION_PROPERTY_RETURN
}

FUPyConversionResult PythonizeProperty(const FProperty* Prop, const void* ValueAddr, PyObject*& OutPyObj, const EUPyConversionMethod ConversionMethod, PyObject* OwnerPyObj, const ESetErrorState SetErrorState)
{
	if (Prop->ArrayDim > 1)
	{
		OutPyObj = (PyObject*)FUPyWrapperFixedArrayFactory::Get().CreateInstance((void*)ValueAddr, Prop, FUPyWrapperOwnerContext(OwnerPyObj, OwnerPyObj ? Prop : nullptr), ConversionMethod);
		return FUPyConversionResult::Success();
	}

	return PythonizeProperty_Direct(Prop, ValueAddr, OutPyObj, ConversionMethod, OwnerPyObj, SetErrorState);
}

FUPyConversionResult NativizeProperty_Direct(PyObject* PyObj, const FProperty* Prop, void* ValueAddr, const TConstArrayView<void*>& InArchetypeInstValueAddrs, const FPropertyAccessChangeNotify* InChangeNotify, const ESetErrorState SetErrorState)
{
#define UPYCONVERSION_PROPERTY_RETURN(RESULT) \
	UPYCONVERSION_RETURN(RESULT, TEXT("NativizeProperty"), *FString::Printf(TEXT("Cannot nativize '%s' as '%s' (%s)"), *UPyUtil::GetFriendlyTypename(PyObj), *Prop->GetName(), *Prop->GetClass()->GetName()))

#define NATIVIZE_SETTER_PROPERTY(PROPTYPE)											\
	if (const PROPTYPE* CastProp = CastField<PROPTYPE>(Prop))								\
	{																				\
		PROPTYPE::TCppType NewValue;												\
		const FUPyConversionResult Result = Nativize(PyObj, NewValue, SetErrorState);\
		if (Result)																	\
		{																			\
			auto OldValue = CastProp->GetPropertyValue(ValueAddr);					\
			EmitPropertyChangeNotifications(InChangeNotify,							\
				OldValue == NewValue, [&]()											\
			{																		\
				for (void* ArchInstValueAddr : InArchetypeInstValueAddrs)			\
				{																	\
					CastProp->SetPropertyValue(ArchInstValueAddr, NewValue);		\
				}																	\
				CastProp->SetPropertyValue(ValueAddr, NewValue);					\
			});																		\
		}																			\
		UPYCONVERSION_PROPERTY_RETURN(Result);										\
	}

#define NATIVIZE_INLINE_PROPERTY(PROPTYPE)											\
	if (const PROPTYPE* CastProp = CastField<PROPTYPE>(Prop))								\
	{																				\
		PROPTYPE::TCppType NewValue;												\
		const FUPyConversionResult Result = Nativize(PyObj, NewValue, SetErrorState);\
		if (Result)																	\
		{																			\
			auto* ValuePtr = static_cast<PROPTYPE::TCppType*>(ValueAddr);			\
			EmitPropertyChangeNotifications(InChangeNotify,							\
				CastProp->Identical(ValuePtr, &NewValue, PPF_None), [&]()			\
			{																		\
				for (void* ArchInstValueAddr : InArchetypeInstValueAddrs)			\
				{																	\
					auto* ArchInstValuePtr = static_cast<PROPTYPE::TCppType*>(ArchInstValueAddr);	\
					*ArchInstValuePtr = NewValue;									\
				}																	\
				*ValuePtr = MoveTemp(NewValue);										\
			});																		\
		}																			\
		UPYCONVERSION_PROPERTY_RETURN(Result);										\
	}

	NATIVIZE_SETTER_PROPERTY(FBoolProperty);
	NATIVIZE_INLINE_PROPERTY(FInt8Property);
	NATIVIZE_INLINE_PROPERTY(FInt16Property);
	NATIVIZE_INLINE_PROPERTY(FUInt16Property);
	NATIVIZE_INLINE_PROPERTY(FIntProperty);
	NATIVIZE_INLINE_PROPERTY(FUInt32Property);
	NATIVIZE_INLINE_PROPERTY(FInt64Property);
	NATIVIZE_INLINE_PROPERTY(FUInt64Property);
	NATIVIZE_INLINE_PROPERTY(FFloatProperty);
	NATIVIZE_INLINE_PROPERTY(FDoubleProperty);
	NATIVIZE_INLINE_PROPERTY(FStrProperty);
	NATIVIZE_INLINE_PROPERTY(FNameProperty);
	NATIVIZE_INLINE_PROPERTY(FTextProperty);
	NATIVIZE_INLINE_PROPERTY(FFieldPathProperty);

	if (const FByteProperty* CastProp = CastField<FByteProperty>(Prop))
	{
		uint8 NewValue = 0;
		FUPyConversionResult Result = FUPyConversionResult::Failure();

		if (CastProp->Enum)
		{
			int64 EnumVal = 0;
			Result = NativizeEnumEntry(PyObj, CastProp->Enum, EnumVal, SetErrorState);
			if (Result.GetState() == EUPyConversionResultState::SuccessWithCoercion)
			{
				// Don't allow implicit conversion on enum properties
				Result.SetState(EUPyConversionResultState::Failure);
			}
			if (Result)
			{
				NewValue = (uint8)EnumVal;
			}
		}
		else
		{
			Result = Nativize(PyObj, NewValue, SetErrorState);
		}

		if (Result)
		{
			auto* ValuePtr = static_cast<uint8*>(ValueAddr);
			EmitPropertyChangeNotifications(InChangeNotify, *ValuePtr == NewValue, [&]()
			{
				for (void* ArchInstValueAddr : InArchetypeInstValueAddrs)
				{
					auto* InstValuePtr = static_cast<uint8*>(ArchInstValueAddr);
					*InstValuePtr = NewValue;
				}
				*ValuePtr = NewValue;
			});
		}
		UPYCONVERSION_PROPERTY_RETURN(Result);
	}

	if (const FEnumProperty* CastProp = CastField<FEnumProperty>(Prop))
	{
		FUPyConversionResult Result = FUPyConversionResult::Failure();

		FNumericProperty* EnumInternalProp = CastProp->GetUnderlyingProperty();
		if (EnumInternalProp)
		{
			int64 NewValue = 0;

			Result = NativizeEnumEntry(PyObj, CastProp->GetEnum(), NewValue, SetErrorState);
			if (Result.GetState() == EUPyConversionResultState::SuccessWithCoercion)
			{
				// Don't allow implicit conversion on enum properties
				Result.SetState(EUPyConversionResultState::Failure);
			}

			if (Result)
			{
				const int64 OldValue = EnumInternalProp->GetSignedIntPropertyValue(ValueAddr);
				EmitPropertyChangeNotifications(InChangeNotify, OldValue == NewValue, [&]()
				{
					for (void* ArchInstValueAddr : InArchetypeInstValueAddrs)
					{
						EnumInternalProp->SetIntPropertyValue(ArchInstValueAddr, NewValue);
					}
					EnumInternalProp->SetIntPropertyValue(ValueAddr, NewValue);
				});
			}
		}

		UPYCONVERSION_PROPERTY_RETURN(Result);
	}

	if (const FClassProperty* CastProp = CastField<FClassProperty>(Prop))
	{
		UClass* NewValue = nullptr;
		const FUPyConversionResult Result = NativizeClass(PyObj, NewValue, CastProp->MetaClass, SetErrorState);
		if (Result)
		{
			UObject* OldValue = CastProp->GetObjectPropertyValue(ValueAddr);
			EmitPropertyChangeNotifications(InChangeNotify, OldValue == NewValue, [&]()
			{
				for (void* ArchInstValueAddr : InArchetypeInstValueAddrs)
				{
					CastProp->SetPropertyValue(ArchInstValueAddr, NewValue);
				}
				CastProp->SetObjectPropertyValue(ValueAddr, NewValue);
			});
		}
		UPYCONVERSION_PROPERTY_RETURN(Result);
	}

	if (const FSoftClassProperty* CastProp = CastField<FSoftClassProperty>(Prop))
	{
		UClass* NewValue = nullptr;
		const FUPyConversionResult Result = NativizeClass(PyObj, NewValue, CastProp->MetaClass, SetErrorState);
		if (Result)
		{
			UObject* OldValue = CastProp->GetObjectPropertyValue(ValueAddr);
			EmitPropertyChangeNotifications(InChangeNotify, OldValue == NewValue, [&]()
			{
				for (void* ArchInstValueAddr : InArchetypeInstValueAddrs)
				{
					CastProp->SetObjectPropertyValue(ArchInstValueAddr, NewValue);
				}
				CastProp->SetObjectPropertyValue(ValueAddr, NewValue);
			});
		}
		UPYCONVERSION_PROPERTY_RETURN(Result);
	}

	if (const FObjectPropertyBase* CastProp = CastField<FObjectPropertyBase>(Prop))
	{
		UObject* NewValue = nullptr;
		const FUPyConversionResult Result = NativizeObject(PyObj, NewValue, CastProp->PropertyClass, SetErrorState);
		if (Result)
		{
			UObject* OldValue = CastProp->GetObjectPropertyValue(ValueAddr);
			EmitPropertyChangeNotifications(InChangeNotify, OldValue == NewValue, [&]()
			{
				for (void* ArchInstValueAddr : InArchetypeInstValueAddrs)
				{
					CastProp->SetObjectPropertyValue(ArchInstValueAddr, NewValue);
				}
				CastProp->SetObjectPropertyValue(ValueAddr, NewValue);
			});
		}
		UPYCONVERSION_PROPERTY_RETURN(Result);
	}

	if (const FInterfaceProperty* CastProp = CastField<FInterfaceProperty>(Prop))
	{
		UObject* NewValue = nullptr;
		const FUPyConversionResult Result = NativizeObject(PyObj, NewValue, CastProp->InterfaceClass, SetErrorState);
		if (Result)
		{
			UObject* OldValue = CastProp->GetPropertyValue(ValueAddr).GetObject();
			EmitPropertyChangeNotifications(InChangeNotify, OldValue == NewValue, [&]()
			{
				const FScriptInterface ScriptInterface = FScriptInterface(NewValue, NewValue ? NewValue->GetInterfaceAddress(CastProp->InterfaceClass) : nullptr);
				for (void* ArchInstValueAddr : InArchetypeInstValueAddrs)
				{
					CastProp->SetPropertyValue(ArchInstValueAddr, ScriptInterface);
				}
				CastProp->SetPropertyValue(ValueAddr, ScriptInterface);
			});
		}
		UPYCONVERSION_PROPERTY_RETURN(Result);
	}

	if (const FStructProperty* CastProp = CastField<FStructProperty>(Prop))
	{
		FUPyConversionResult Result = FUPyConversionResult::Failure();
		PyTypeObject* PyStructType = (PyTypeObject*)FUPyWrapperTypeRegistry::Get().GetWrappedStructType(CastProp->Struct);
		FUPyWrapperStructPtr PyStruct = FUPyWrapperStructPtr::StealReference(FUPyWrapperStruct::CastPyObject(PyObj, PyStructType, &Result));
		if (PyStruct && ensureAlways(PyStruct->ScriptStruct->IsChildOf(CastProp->Struct)))
		{
			EmitPropertyChangeNotifications(InChangeNotify, CastProp->Identical(ValueAddr, PyStruct->StructInstance, PPF_None), [&]()
			{
				for (void* ArchInstValueAddr : InArchetypeInstValueAddrs)
				{
					CastProp->Struct->CopyScriptStruct(ArchInstValueAddr, PyStruct->StructInstance);
				}
				CastProp->Struct->CopyScriptStruct(ValueAddr, PyStruct->StructInstance);
			});
		}
		else
		{
			// Extra function scope as we don't want UPYCONVERSION_RETURN to early return and skip the UPYCONVERSION_PROPERTY_RETURN below
			[&]()
			{
				UPYCONVERSION_RETURN(Result, TEXT("NativizeStructInstance"), *FString::Printf(TEXT("Cannot nativize '%s' as '%s'"), *UPyUtil::GetFriendlyTypename(PyObj), *UPyUtil::GetFriendlyTypename(PyStructType)));
			}();
		}
		UPYCONVERSION_PROPERTY_RETURN(Result);
	}

	if (const FDelegateProperty* CastProp = CastField<FDelegateProperty>(Prop))
	{
		FUPyConversionResult Result = FUPyConversionResult::Failure();
		const PyTypeObject* PyDelegateType = FUPyWrapperTypeRegistry::Get().GetWrappedDelegateType(CastProp->SignatureFunction);
		FUPyWrapperDelegatePtr PyDelegate = FUPyWrapperDelegatePtr::StealReference(FUPyWrapperDelegate::CastPyObject(PyObj, PyDelegateType, &Result));
		if (PyDelegate)
		{
			EmitPropertyChangeNotifications(InChangeNotify, CastProp->Identical(ValueAddr, PyDelegate->DelegateInstance, PPF_None), [&]()
			{
				for (void* ArchInstValueAddr : InArchetypeInstValueAddrs)
				{
					CastProp->SetPropertyValue(ArchInstValueAddr, *PyDelegate->DelegateInstance);
				}
				CastProp->SetPropertyValue(ValueAddr, *PyDelegate->DelegateInstance);
			});
		}
		UPYCONVERSION_PROPERTY_RETURN(Result);
	}

	if (const FMulticastDelegateProperty* CastProp = CastField<FMulticastDelegateProperty>(Prop))
	{
		FUPyConversionResult Result = FUPyConversionResult::Failure();
		const PyTypeObject* PyDelegateType = FUPyWrapperTypeRegistry::Get().GetWrappedDelegateType(CastProp->SignatureFunction);
		FUPyWrapperMulticastDelegatePtr PyDelegate = FUPyWrapperMulticastDelegatePtr::StealReference(FUPyWrapperMulticastDelegate::CastPyObject(PyObj, PyDelegateType, &Result));
		if (PyDelegate)
		{
			EmitPropertyChangeNotifications(InChangeNotify, CastProp->Identical(ValueAddr, PyDelegate->DelegateInstance, PPF_None), [&]()
			{
				for (void* ArchInstValueAddr : InArchetypeInstValueAddrs)
				{
					CastProp->SetMulticastDelegate(ArchInstValueAddr, *PyDelegate->DelegateInstance);
				}
				CastProp->SetMulticastDelegate(ValueAddr, *PyDelegate->DelegateInstance);
			});
		}
		UPYCONVERSION_PROPERTY_RETURN(Result);
	}

	if (const FArrayProperty* CastProp = CastField<FArrayProperty>(Prop))
	{
		FUPyConversionResult Result = FUPyConversionResult::Failure();
		FUPyWrapperArrayPtr PyArray = FUPyWrapperArrayPtr::StealReference(FUPyWrapperArray::CastPyObject(PyObj, &UPyWrapperArrayType, CastProp->Inner, &Result));
		if (PyArray)
		{
			EmitPropertyChangeNotifications(InChangeNotify, CastProp->Identical(ValueAddr, PyArray->ArrayInstance, PPF_None), [&]()
			{
				for (void* ArchInstValueAddr : InArchetypeInstValueAddrs)
				{
					CastProp->CopySingleValue(ArchInstValueAddr, PyArray->ArrayInstance);
				}
				CastProp->CopySingleValue(ValueAddr, PyArray->ArrayInstance);
			});
		}
		UPYCONVERSION_PROPERTY_RETURN(Result);
	}

	if (const FSetProperty* CastProp = CastField<FSetProperty>(Prop))
	{
		FUPyConversionResult Result = FUPyConversionResult::Failure();
		FUPyWrapperSetPtr PySet = FUPyWrapperSetPtr::StealReference(FUPyWrapperSet::CastPyObject(PyObj, &UPyWrapperSetType, CastProp->ElementProp, &Result));
		if (PySet)
		{
			EmitPropertyChangeNotifications(InChangeNotify, CastProp->Identical(ValueAddr, PySet->SetInstance, PPF_None), [&]()
			{
				for (void* ArchInstValueAddr : InArchetypeInstValueAddrs)
				{
					CastProp->CopySingleValue(ArchInstValueAddr, PySet->SetInstance);
				}
				CastProp->CopySingleValue(ValueAddr, PySet->SetInstance);
			});
		}
		UPYCONVERSION_PROPERTY_RETURN(Result);
	}

	if (const FMapProperty* CastProp = CastField<FMapProperty>(Prop))
	{
		FUPyConversionResult Result = FUPyConversionResult::Failure();
		FUPyWrapperMapPtr PyMap = FUPyWrapperMapPtr::StealReference(FUPyWrapperMap::CastPyObject(PyObj, &UPyWrapperMapType, CastProp->KeyProp, CastProp->ValueProp, &Result));
		if (PyMap)
		{
			EmitPropertyChangeNotifications(InChangeNotify, CastProp->Identical(ValueAddr, PyMap->MapInstance, PPF_None), [&]()
			{
				for (void* ArchInstValueAddr : InArchetypeInstValueAddrs)
				{
					CastProp->CopySingleValue(ArchInstValueAddr, PyMap->MapInstance);
				}
				CastProp->CopySingleValue(ValueAddr, PyMap->MapInstance);
			});
		}
		UPYCONVERSION_PROPERTY_RETURN(Result);
	}

#undef NATIVIZE_SETTER_PROPERTY
#undef NATIVIZE_INLINE_PROPERTY

	UPYCONVERSION_RETURN(FUPyConversionResult::Failure(), TEXT("NativizeProperty"), *FString::Printf(TEXT("Cannot nativize '%s' as '%s' (%s). %s conversion not implemented!"), *UPyUtil::GetFriendlyTypename(PyObj), *Prop->GetName(), *Prop->GetClass()->GetName(), *Prop->GetClass()->GetName()));

#undef UPYCONVERSION_PROPERTY_RETURN
}

FUPyConversionResult PythonizeProperty_Direct(const FProperty* Prop, const void* ValueAddr, PyObject*& OutPyObj, const EUPyConversionMethod ConversionMethod, PyObject* OwnerPyObj, const ESetErrorState SetErrorState)
{
#define UPYCONVERSION_PROPERTY_RETURN(RESULT) \
	UPYCONVERSION_RETURN(RESULT, TEXT("PythonizeProperty"), *FString::Printf(TEXT("Cannot pythonize '%s' (%s)"), *Prop->GetName(), *Prop->GetClass()->GetName()))

	const FUPyWrapperOwnerContext OwnerContext(OwnerPyObj, OwnerPyObj ? Prop : nullptr);
	OwnerContext.AssertValidConversionMethod(ConversionMethod);

#define PYTHONIZE_GETTER_PROPERTY(PROPTYPE)											\
	if (const PROPTYPE* CastProp = CastField<PROPTYPE>(Prop))										\
	{																				\
		auto&& Value = CastProp->GetPropertyValue(ValueAddr);						\
		UPYCONVERSION_PROPERTY_RETURN(Pythonize(Value, OutPyObj, SetErrorState));	\
	}

	PYTHONIZE_GETTER_PROPERTY(FBoolProperty);
	PYTHONIZE_GETTER_PROPERTY(FInt8Property);
	PYTHONIZE_GETTER_PROPERTY(FInt16Property);
	PYTHONIZE_GETTER_PROPERTY(FUInt16Property);
	PYTHONIZE_GETTER_PROPERTY(FIntProperty);
	PYTHONIZE_GETTER_PROPERTY(FUInt32Property);
	PYTHONIZE_GETTER_PROPERTY(FInt64Property);
	PYTHONIZE_GETTER_PROPERTY(FUInt64Property);
	PYTHONIZE_GETTER_PROPERTY(FFloatProperty);
	PYTHONIZE_GETTER_PROPERTY(FDoubleProperty);
	PYTHONIZE_GETTER_PROPERTY(FStrProperty);
	PYTHONIZE_GETTER_PROPERTY(FNameProperty);
	PYTHONIZE_GETTER_PROPERTY(FTextProperty);
	PYTHONIZE_GETTER_PROPERTY(FFieldPathProperty);

	if (const FByteProperty* CastProp = CastField<FByteProperty>(Prop))
	{
		const uint8 Value = CastProp->GetPropertyValue(ValueAddr);
		if (CastProp->Enum)
		{
			UPYCONVERSION_PROPERTY_RETURN(PythonizeEnumEntry((int64)Value, CastProp->Enum, OutPyObj, SetErrorState));
		}
		else
		{
			UPYCONVERSION_PROPERTY_RETURN(Pythonize(Value, OutPyObj, SetErrorState));
		}
	}

	if (const FEnumProperty* CastProp = CastField<FEnumProperty>(Prop))
	{
		FNumericProperty* EnumInternalProp = CastProp->GetUnderlyingProperty();
		UPYCONVERSION_PROPERTY_RETURN(PythonizeEnumEntry(EnumInternalProp ? EnumInternalProp->GetSignedIntPropertyValue(ValueAddr) : 0, CastProp->GetEnum(), OutPyObj, SetErrorState));
	}

	if (const FClassProperty* CastProp = CastField<FClassProperty>(Prop))
	{
		UClass* Value = Cast<UClass>(CastProp->LoadObjectPropertyValue(ValueAddr));
		UPYCONVERSION_PROPERTY_RETURN(PythonizeClass(Value, OutPyObj, SetErrorState));
	}

	if (const FSoftClassProperty* CastProp = CastField<FSoftClassProperty>(Prop))
	{
		UClass* Value = Cast<UClass>(CastProp->LoadObjectPropertyValue(ValueAddr));
		UPYCONVERSION_PROPERTY_RETURN(PythonizeClass(Value, OutPyObj, SetErrorState));
	}

	if (const FObjectPropertyBase* CastProp = CastField<FObjectPropertyBase>(Prop))
	{
		UObject* Value = CastProp->LoadObjectPropertyValue(ValueAddr);
		UPYCONVERSION_PROPERTY_RETURN(Pythonize(Value, OutPyObj, SetErrorState));
	}

	if (const FInterfaceProperty* CastProp = CastField<FInterfaceProperty>(Prop))
	{
		UObject* Value = CastProp->GetPropertyValue(ValueAddr).GetObject();
		if (Value)
		{
			OutPyObj = (PyObject*)FUPyWrapperObjectFactory::Get().CreateInstance(CastProp->InterfaceClass, Value);
		}
		else
		{
			Py_INCREF(Py_None);
			OutPyObj = Py_None;
		}
		return FUPyConversionResult::Success();
	}

	if (const FStructProperty* CastProp = CastField<FStructProperty>(Prop))
	{
		OutPyObj = (PyObject*)FUPyWrapperStructFactory::Get().CreateInstance(CastProp->Struct, (void*)ValueAddr, OwnerContext, ConversionMethod);
		return FUPyConversionResult::Success();
	}

	if (const FDelegateProperty* CastProp = CastField<FDelegateProperty>(Prop))
	{
		const FScriptDelegate* Value = CastProp->GetPropertyValuePtr(ValueAddr);
		OutPyObj = (PyObject*)FUPyWrapperDelegateFactory::Get().CreateInstance(CastProp->SignatureFunction, (FScriptDelegate*)Value, CastProp, OwnerContext, ConversionMethod);
		return FUPyConversionResult::Success();
	}

	if (const FMulticastDelegateProperty* CastProp = CastField<FMulticastDelegateProperty>(Prop))
	{
		const FMulticastScriptDelegate* Value = CastProp->GetMulticastDelegate(ValueAddr);
		OutPyObj = (PyObject*)FUPyWrapperMulticastDelegateFactory::Get().CreateInstance(CastProp->SignatureFunction, (FMulticastScriptDelegate*)Value, (void*)ValueAddr, CastProp, OwnerContext, ConversionMethod);
		return FUPyConversionResult::Success();
	}

	// TODO: We're just not handling sparse delegates at this time
	/*if (const FMulticastSparseDelegateProperty* CastProp = CastField<FMulticastSparseDelegateProperty>(Prop))
	{
		const FMulticastScriptDelegate* Value = CastProp->GetMulticastDelegate(ValueAddr);
		OutPyObj = (PyObject*)FPyWrapperMulticastDelegateFactory::Get().CreateInstance(CastProp->SignatureFunction, (FMulticastScriptDelegate*)Value, OwnerContext, ConversionMethod);
		return FUPyConversionResult::Success();
	}*/

	if (const FArrayProperty* CastProp = CastField<FArrayProperty>(Prop))
	{
		OutPyObj = (PyObject*)FUPyWrapperArrayFactory::Get().CreateInstance((void*)ValueAddr, CastProp, OwnerContext, ConversionMethod);
		return FUPyConversionResult::Success();
	}

	if (const FSetProperty* CastProp = CastField<FSetProperty>(Prop))
	{
		OutPyObj = (PyObject*)FUPyWrapperSetFactory::Get().CreateInstance((void*)ValueAddr, CastProp, OwnerContext, ConversionMethod);
		return FUPyConversionResult::Success();
	}

	if (const FMapProperty* CastProp = CastField<FMapProperty>(Prop))
	{
		OutPyObj = (PyObject*)FUPyWrapperMapFactory::Get().CreateInstance((void*)ValueAddr, CastProp, OwnerContext, ConversionMethod);
		return FUPyConversionResult::Success();
	}

#undef PYTHONIZE_GETTER_PROPERTY

	UPYCONVERSION_RETURN(FUPyConversionResult::Failure(), TEXT("PythonizeProperty"), *FString::Printf(TEXT("Cannot pythonize '%s' (%s). %s conversion not implemented!"), *Prop->GetName(), *Prop->GetClass()->GetName(), *Prop->GetClass()->GetName()));

#undef UPYCONVERSION_PROPERTY_RETURN
}

FUPyConversionResult NativizeProperty_InContainer(PyObject* PyObj, const FProperty* Prop, void* BaseAddr, const int32 ArrayIndex, const TConstArrayView<void*>& InArchetypeInstBaseAddrs, const FPropertyAccessChangeNotify* InChangeNotify, const ESetErrorState SetErrorState)
{
	TArray<void*> ArchetypeInstValueAddrs;
	ArchetypeInstValueAddrs.Reserve(InArchetypeInstBaseAddrs.Num());
	for (void* ContainerAddr : InArchetypeInstBaseAddrs)
	{
		ArchetypeInstValueAddrs.Add(Prop->ContainerPtrToValuePtr<void>(ContainerAddr, ArrayIndex));
	}

	check(ArrayIndex < Prop->ArrayDim);
	return NativizeProperty(PyObj, Prop, Prop->ContainerPtrToValuePtr<void>(BaseAddr, ArrayIndex), ArchetypeInstValueAddrs, InChangeNotify, SetErrorState);
}

FUPyConversionResult PythonizeProperty_InContainer(const FProperty* Prop, const void* BaseAddr, const int32 ArrayIndex, PyObject*& OutPyObj, const EUPyConversionMethod ConversionMethod, PyObject* OwnerPyObj, const ESetErrorState SetErrorState)
{
	check(ArrayIndex < Prop->ArrayDim);
	return PythonizeProperty(Prop, Prop->ContainerPtrToValuePtr<void>(BaseAddr, ArrayIndex), OutPyObj, ConversionMethod, OwnerPyObj, SetErrorState);
}

void EmitPropertyChangeNotifications(const FPropertyAccessChangeNotify* InChangeNotify, const bool bIdenticalValue, const TFunctionRef<void()>& InDoChangeFunc)
{
	Py_BEGIN_ALLOW_THREADS
	PropertyAccessUtil::EmitPreChangeNotify(InChangeNotify, bIdenticalValue);
	if (!bIdenticalValue)
	{
		InDoChangeFunc();
	}
	PropertyAccessUtil::EmitPostChangeNotify(InChangeNotify, bIdenticalValue);
	Py_END_ALLOW_THREADS
}

}

#undef UPYCONVERSION_RETURN
