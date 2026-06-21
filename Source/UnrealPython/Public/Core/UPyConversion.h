
#pragma once

#include "UPyCommon.h"
#include "AssetRegistry/ARFilter.h"
#include "Core/UPyPtr.h"
#include "Core/UPyConversionResult.h"
#include "UObject/Class.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Templates/EnableIf.h"
#include "Templates/UnrealTemplate.h"
#include "Templates/PointerIsConvertibleFromTo.h"
#include "UObject/PropertyAccessUtil.h"
#include "Misc/CoreMiscDefines.h"

#define IMPLEMENT_TBASE_STRUCTURE(T, Path) \
template<> \
struct TBaseStructure<T> \
{ \
	static UScriptStruct* Get() \
	{ \
		/* Static local variable ensures FindObject is called only once. Subsequent calls are instant. */ \
		static UScriptStruct* Struct = FindObject<UScriptStruct>(nullptr, TEXT(Path)); \
		return Struct; \
	} \
};

IMPLEMENT_TBASE_STRUCTURE(FVector2f, "/Script/CoreUObject.Vector2f")
IMPLEMENT_TBASE_STRUCTURE(FVector3f, "/Script/CoreUObject.Vector3f")
IMPLEMENT_TBASE_STRUCTURE(FVector4f, "/Script/CoreUObject.Vector4f")
IMPLEMENT_TBASE_STRUCTURE(FTimespan, "/Script/CoreUObject.Timespan")
IMPLEMENT_TBASE_STRUCTURE(FBox, "/Script/CoreUObject.Box")
IMPLEMENT_TBASE_STRUCTURE(FBoxSphereBounds, "/Script/CoreUObject.BoxSphereBounds")
IMPLEMENT_TBASE_STRUCTURE(FQualifiedFrameTime, "/Script/CoreUObject.QualifiedFrameTime")
IMPLEMENT_TBASE_STRUCTURE(FIntVector2, "/Script/CoreUObject.IntVector2")
IMPLEMENT_TBASE_STRUCTURE(FOrientedBox, "/Script/CoreUObject.OrientedBox")
IMPLEMENT_TBASE_STRUCTURE(FPlatformUserId, "/Script/CoreUObject.PlatformUserId")
IMPLEMENT_TBASE_STRUCTURE(FARFilter, "/Script/CoreUObject.ARFilter")

#define UPYCONVERSION_RETURN(RESULT, ERROR_CTX, ERROR_MSG)							\
    {																				\
        const FUPyConversionResult PyConversionReturnResult_Internal = (RESULT);	\
        if (!PyConversionReturnResult_Internal)										\
        {																			\
            if (SetErrorState == ESetErrorState::Yes)								\
            {																		\
                UPyUtil::SetPythonError(PyExc_TypeError, (ERROR_CTX), (ERROR_MSG));	\
            }																		\
            else																	\
            {																		\
                PyErr_Clear();														\
            }																		\
        }																			\
        return PyConversionReturnResult_Internal;									\
    }


enum class EUPyConversionMethod : uint8
{
	/** Copy the value */
	Copy,
	/** Steal the value (or fallback to Copy) */
	Steal,
	/** Reference the value from the given owner (or fallback to Copy) */
	Reference,
};

FORCEINLINE void AssertValidUPyConversionOwner(PyObject* InPyOwner, const EUPyConversionMethod InMethod)
{
	checkf(InPyOwner || InMethod != EUPyConversionMethod::Reference, TEXT("EUPyConversionMethod::Reference requires a valid owner object"));
}

/**
 * Conversion between native and Python types.
 * @note These functions may set error state when using ESetErrorState::Yes.
 */
namespace UPyConversion
{
	enum class ESetErrorState : uint8 { No, Yes };

	/** bool overload */
	UNREALPYTHON_API FUPyConversionResult Nativize(PyObject* PyObj, bool& OutVal, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	UNREALPYTHON_API FUPyConversionResult Pythonize(const bool Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/** int8 overload */
	UNREALPYTHON_API FUPyConversionResult Nativize(PyObject* PyObj, int8& OutVal, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	UNREALPYTHON_API FUPyConversionResult Pythonize(const int8 Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/** uint8 overload */
	UNREALPYTHON_API FUPyConversionResult Nativize(PyObject* PyObj, uint8& OutVal, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	UNREALPYTHON_API FUPyConversionResult Pythonize(const uint8 Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/** int16 overload */
	UNREALPYTHON_API FUPyConversionResult Nativize(PyObject* PyObj, int16& OutVal, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	UNREALPYTHON_API FUPyConversionResult Pythonize(const int16 Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/** uint16 overload */
	UNREALPYTHON_API FUPyConversionResult Nativize(PyObject* PyObj, uint16& OutVal, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	UNREALPYTHON_API FUPyConversionResult Pythonize(const uint16 Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/** int32 overload */
	UNREALPYTHON_API FUPyConversionResult Nativize(PyObject* PyObj, int32& OutVal, const ESetErrorState ErrorState = ESetErrorState::Yes);
	UNREALPYTHON_API FUPyConversionResult Pythonize(const int32 Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/** uint32 overload */
	UNREALPYTHON_API FUPyConversionResult Nativize(PyObject* PyObj, uint32& OutVal, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	UNREALPYTHON_API FUPyConversionResult Pythonize(const uint32 Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/** int64 overload */
	UNREALPYTHON_API FUPyConversionResult Nativize(PyObject* PyObj, int64& OutVal, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	UNREALPYTHON_API FUPyConversionResult Pythonize(const int64 Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/** uint64 overload */
	UNREALPYTHON_API FUPyConversionResult Nativize(PyObject* PyObj, uint64& OutVal, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	UNREALPYTHON_API FUPyConversionResult Pythonize(const uint64 Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/** float overload */
	UNREALPYTHON_API FUPyConversionResult Nativize(PyObject* PyObj, float& OutVal, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	UNREALPYTHON_API FUPyConversionResult Pythonize(const float Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/** double overload */
	UNREALPYTHON_API FUPyConversionResult Nativize(PyObject* PyObj, double& OutVal, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	UNREALPYTHON_API FUPyConversionResult Pythonize(const double Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/** char overload */
	UNREALPYTHON_API FUPyConversionResult Nativize(PyObject* PyObj, const char*& OutVal, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	UNREALPYTHON_API FUPyConversionResult Pythonize(const char* Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/** FString overload */
	UNREALPYTHON_API FUPyConversionResult Nativize(PyObject* PyObj, FString& OutVal, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	UNREALPYTHON_API FUPyConversionResult Pythonize(const FString& Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/** FName overload */
	UNREALPYTHON_API FUPyConversionResult Nativize(PyObject* PyObj, FName& OutVal, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	UNREALPYTHON_API FUPyConversionResult Pythonize(const FName& Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/** FText overload */
	UNREALPYTHON_API FUPyConversionResult Nativize(PyObject* PyObj, FText& OutVal, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	UNREALPYTHON_API FUPyConversionResult Pythonize(const FText& Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/** FFieldPath overload */
	UNREALPYTHON_API FUPyConversionResult Nativize(PyObject* PyObj, FFieldPath& OutVal, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	UNREALPYTHON_API FUPyConversionResult Pythonize(const FFieldPath& Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/** void* overload */
	UNREALPYTHON_API FUPyConversionResult Nativize(PyObject* PyObj, void*& OutVal, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	UNREALPYTHON_API FUPyConversionResult Pythonize(void* Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/** UObject overload */
	UNREALPYTHON_API FUPyConversionResult Nativize(PyObject* PyObj, UObject*& OutVal, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	UNREALPYTHON_API FUPyConversionResult Pythonize(UObject* Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/** Conversion for object types, including optional type checking */
	UNREALPYTHON_API FUPyConversionResult NativizeObject(PyObject* PyObj, UObject*& OutVal, UClass* ExpectedType, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	UNREALPYTHON_API FUPyConversionResult PythonizeObject(UObject* Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	UNREALPYTHON_API PyObject* PythonizeObject(UObject* Val, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/** Conversion for class types, including optional type checking */
	UNREALPYTHON_API FUPyConversionResult NativizeClass(PyObject* PyObj, UClass*& OutVal, UClass* ExpectedType, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	UNREALPYTHON_API FUPyConversionResult PythonizeClass(UClass* Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	UNREALPYTHON_API PyObject* PythonizeClass(UClass* Val, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	UNREALPYTHON_API UClass* NativizeClass(PyObject* PyObj, UClass* ExpectedType = nullptr, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/** Conversion for struct types, including optional type checking */
	UNREALPYTHON_API FUPyConversionResult NativizeStruct(PyObject* PyObj, UScriptStruct*& OutVal, UScriptStruct* ExpectedType, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	UNREALPYTHON_API FUPyConversionResult PythonizeStruct(UScriptStruct* Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	UNREALPYTHON_API PyObject* PythonizeStruct(UScriptStruct* Val, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/** Conversion for enum entries */
	UNREALPYTHON_API FUPyConversionResult NativizeEnumEntry(PyObject* PyObj, const UEnum* EnumType, int64& OutVal, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	UNREALPYTHON_API FUPyConversionResult PythonizeEnumEntry(const int64 Val, const UEnum* EnumType, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	UNREALPYTHON_API PyObject* PythonizeEnumEntry(const int64 Val, const UEnum* EnumType, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	namespace Internal
	{
		/** Internal version of NativizeStructInstance/PythonizeStructInstance that work on the type-erased data */
		UNREALPYTHON_API FUPyConversionResult NativizeStructInstance(PyObject* PyObj, UScriptStruct* StructType, void* StructInstance, const ESetErrorState SetErrorState);
		UNREALPYTHON_API FUPyConversionResult PythonizeStructInstance(UScriptStruct* StructType, const void* StructInstance, PyObject*& OutPyObj, const ESetErrorState SetErrorState);

		/** Dummy catch-all for type conversions that aren't yet implemented */
		template <typename T, typename Spec = void>
		struct FTypeConv
		{
			static FUPyConversionResult Nativize(PyObject* PyObj, T& OutVal, const ESetErrorState SetErrorState)
			{
				ensureAlwaysMsgf(false, TEXT("Nativize not implemented for type"));
				return FUPyConversionResult::Failure();
			}

			static FUPyConversionResult Pythonize(const T& Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
			{
				ensureAlwaysMsgf(false, TEXT("Pythonize not implemented for type"));
				return FUPyConversionResult::Failure();
			}
		};

		/** Override the catch-all for UObject types */
		template <typename T>
		struct FTypeConv<T, typename TEnableIf<TPointerIsConvertibleFromTo<typename TRemovePointer<T>::Type, UObject>::Value>::Type>
		{
			static FUPyConversionResult Nativize(PyObject* PyObj, T& OutVal, const ESetErrorState SetErrorState)
			{
				return UPyConversion::NativizeObject(PyObj, (UObject*&)OutVal, TRemovePointer<T>::Type::StaticClass(), SetErrorState);
			}

			static FUPyConversionResult Pythonize(const T& Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
			{
				return UPyConversion::PythonizeObject((UObject*)Val, OutPyObj, SetErrorState);
			}
		};

#if ENGINE_MAJOR_VERSION >= 5
		template<typename ...> using UPy_void_t = void;

		template<typename T, typename = void>
		struct TIsComplete
		{
			enum { Value = false };
		};

		template<typename T>
		struct TIsComplete<T, UPy_void_t<decltype(sizeof(T))>>
		{
			enum { Value = true };
		};

		/** Override the catch-all for TObjectPtr types */
		template <typename T>
		struct FTypeConv<TObjectPtr<T>, typename TEnableIf<TIsComplete<T>::Value>::Type>
		{
			static FUPyConversionResult Nativize(PyObject* PyObj, TObjectPtr<T>& OutVal, const ESetErrorState SetErrorState)
			{
				T* Obj = nullptr;
				FUPyConversionResult Result = UPyConversion::NativizeObject(PyObj, (UObject*&)Obj, T::StaticClass(), SetErrorState);
				if (Result)
				{
					OutVal = Obj;
				}
				return Result;
			}

			static FUPyConversionResult Pythonize(const TObjectPtr<T>& Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
			{
				return UPyConversion::PythonizeObject(Val.Get(), OutPyObj, SetErrorState);
			}
		};
#endif
	}

	/**
	 * Generic version of Nativize used if there is no matching overload.
	 * Used to allow conversion from object and struct types that don't match a specific override (see FTypeConv).
	 */
	template <typename T>
	FUPyConversionResult Nativize(PyObject* PyObj, T& OutVal, const ESetErrorState SetErrorState = ESetErrorState::Yes)
	{
		return Internal::FTypeConv<T>::Nativize(PyObj, OutVal, SetErrorState);
	}

	/**
	 * Generic version of Pythonize used if there is no matching overload.
	 * Used to allow conversion from object and struct types that don't match a specific override (see FTypeConv).
	 */
	template <typename T>
	FUPyConversionResult Pythonize(const T& Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes)
	{
		return Internal::FTypeConv<T>::Pythonize(Val, OutPyObj, SetErrorState);
	}

	/** Generic version of Pythonize that returns a PyObject rather than a bool */
	template <typename T>
	PyObject* Pythonize(const T& Val, const ESetErrorState SetErrorState = ESetErrorState::Yes)
	{
		PyObject* Obj = nullptr;
		Pythonize(Val, Obj, SetErrorState);
		return Obj;
	}

	/** Conversion for TArray values used by C++ template call helpers. */
	template <typename ElementType>
	FUPyConversionResult Nativize(PyObject* PyObj, TArray<ElementType>& OutVal, const ESetErrorState SetErrorState = ESetErrorState::Yes)
	{
		if (PyUnicode_Check(PyObj) || PyBytes_Check(PyObj))
		{
			if (SetErrorState == ESetErrorState::Yes)
			{
				PyErr_SetString(PyExc_TypeError, "Cannot nativize string/bytes as TArray");
			}
			else
			{
				PyErr_Clear();
			}
			return FUPyConversionResult::Failure();
		}

		FUPyObjectPtr PySequence = FUPyObjectPtr::StealReference(PySequence_Fast(PyObj, "Cannot nativize object as TArray"));
		if (!PySequence)
		{
			if (SetErrorState == ESetErrorState::No)
			{
				PyErr_Clear();
			}
			return FUPyConversionResult::Failure();
		}

		const Py_ssize_t SequenceSize = PySequence_Fast_GET_SIZE(PySequence.Get());
		if (SequenceSize > TNumericLimits<int32>::Max())
		{
			if (SetErrorState == ESetErrorState::Yes)
			{
				PyErr_SetString(PyExc_OverflowError, "Cannot nativize Python sequence larger than int32 max as TArray");
			}
			else
			{
				PyErr_Clear();
			}
			return FUPyConversionResult::Failure();
		}

		TArray<ElementType> TempVal;
		TempVal.Reserve(static_cast<int32>(SequenceSize));

		FUPyConversionResult ArrayResult = FUPyConversionResult::Success();
		PyObject** SequenceItems = PySequence_Fast_ITEMS(PySequence.Get());
		for (Py_ssize_t SequenceIndex = 0; SequenceIndex < SequenceSize; ++SequenceIndex)
		{
			ElementType Element = ElementType();
			const FUPyConversionResult ElementResult = Nativize(SequenceItems[SequenceIndex], Element, SetErrorState);
			if (!ElementResult)
			{
				return FUPyConversionResult::Failure();
			}

			if (ElementResult.GetState() == EUPyConversionResultState::SuccessWithCoercion)
			{
				ArrayResult = FUPyConversionResult::SuccessWithCoercion();
			}
			TempVal.Add(MoveTemp(Element));
		}

		OutVal = MoveTemp(TempVal);
		return ArrayResult;
	}

	template <typename ElementType>
	FUPyConversionResult Pythonize(const TArray<ElementType>& Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes)
	{
		FUPyObjectPtr PyList = FUPyObjectPtr::StealReference(PyList_New(Val.Num()));
		if (!PyList)
		{
			if (SetErrorState == ESetErrorState::No)
			{
				PyErr_Clear();
			}
			return FUPyConversionResult::Failure();
		}

		FUPyConversionResult ArrayResult = FUPyConversionResult::Success();
		for (int32 ElementIndex = 0; ElementIndex < Val.Num(); ++ElementIndex)
		{
			PyObject* PyItem = nullptr;
			const FUPyConversionResult ElementResult = Pythonize(Val[ElementIndex], PyItem, SetErrorState);
			if (!ElementResult)
			{
				Py_XDECREF(PyItem);
				return FUPyConversionResult::Failure();
			}

			if (ElementResult.GetState() == EUPyConversionResultState::SuccessWithCoercion)
			{
				ArrayResult = FUPyConversionResult::SuccessWithCoercion();
			}
			PyList_SetItem(PyList, ElementIndex, PyItem);
		}

		OutPyObj = PyList.Release();
		return ArrayResult;
	}

	/** Conversion for known struct types */
	template <typename T>
	FUPyConversionResult NativizeStructInstance(PyObject* PyObj, T& OutVal, const ESetErrorState SetErrorState = ESetErrorState::Yes)
	{
		return Internal::NativizeStructInstance(PyObj, TBaseStructure<T>::Get(), &OutVal, SetErrorState);
	}

	/** Conversion for known struct types */
	template <typename T>
	FUPyConversionResult PythonizeStructInstance(const T& Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes)
	{
		return Internal::PythonizeStructInstance(TBaseStructure<T>::Get(), &Val, OutPyObj, SetErrorState);
	}

	/** Conversion for known struct types that returns a PyObject rather than a bool */
	template <typename T>
	PyObject* PythonizeStructInstance(const T& Val, const ESetErrorState SetErrorState = ESetErrorState::Yes)
	{
		PyObject* Obj = nullptr;
		PythonizeStructInstance(Val, Obj, SetErrorState);
		return Obj;
	}

	template <>
	inline FUPyConversionResult PythonizeStructInstance<FPlatformUserId>(const FPlatformUserId& Val, PyObject*& OutPyObj, const ESetErrorState SetErrorState)
	{
		OutPyObj = PyLong_FromLong(Val.GetInternalId());
		return FUPyConversionResult::Success();
	}

	/** Conversion for known enum types */
	template <typename T>
	FUPyConversionResult NativizeEnumEntry(PyObject* PyObj, const UEnum* EnumType, T& OutVal, const ESetErrorState SetErrorState = ESetErrorState::Yes)
	{
		int64 OutTmpVal = 0;
		FUPyConversionResult Result = NativizeEnumEntry(PyObj, EnumType, OutTmpVal, SetErrorState);
		if (Result)
		{
			OutVal = (T)OutTmpVal;
		}
		return Result;
	}

	/** Conversion for TEnumAsByte types */
	template <typename T>
	FUPyConversionResult NativizeEnumEntry(PyObject* PyObj, const UEnum* EnumType, TEnumAsByte<T>& OutVal, const ESetErrorState SetErrorState = ESetErrorState::Yes)
	{
		int64 OutTmpVal = 0;
		FUPyConversionResult Result = NativizeEnumEntry(PyObj, EnumType, OutTmpVal, SetErrorState);
		if (Result)
		{
			OutVal = (T)OutTmpVal;
		}
		return Result;
	}

	/** Conversion for known enum types */
	template <typename T>
	FUPyConversionResult PythonizeEnumEntry(const T& Val, const UEnum* EnumType, PyObject*& OutPyObj, const ESetErrorState SetErrorState = ESetErrorState::Yes)
	{
		const int64 TmpVal = (int64)Val;
		return PythonizeEnumEntry(TmpVal, EnumType, OutPyObj, SetErrorState);
	}

	/** Conversion for property instances (including fixed arrays) - ValueAddr should point to the property data */
	FUPyConversionResult NativizeProperty(PyObject* PyObj, const FProperty* Prop, void* ValueAddr, const TConstArrayView<void*>& ArchetypeInstValueAddrs = TArray<void*>(), const FPropertyAccessChangeNotify* InChangeNotify = nullptr, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	FUPyConversionResult PythonizeProperty(const FProperty* Prop, const void* ValueAddr, PyObject*& OutPyObj, const EUPyConversionMethod ConversionMethod = EUPyConversionMethod::Copy, PyObject* OwnerPyObj = nullptr, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/** Conversion for single property instances - ValueAddr should point to the property data */
	FUPyConversionResult NativizeProperty_Direct(PyObject* PyObj, const FProperty* Prop, void* ValueAddr, const TConstArrayView<void*>& ArchetypeInstValueAddrs = TArray<void*>(), const FPropertyAccessChangeNotify* InChangeNotify = nullptr, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	FUPyConversionResult PythonizeProperty_Direct(const FProperty* Prop, const void* ValueAddr, PyObject*& OutPyObj, const EUPyConversionMethod ConversionMethod = EUPyConversionMethod::Copy, PyObject* OwnerPyObj = nullptr, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/** Conversion for property instances within a structure (including fixed arrays) - BaseAddr should point to the structure data */
	FUPyConversionResult NativizeProperty_InContainer(PyObject* PyObj, const FProperty* Prop, void* BaseAddr, const int32 ArrayIndex, const TConstArrayView<void*>& ArchetypeInstBaseAddrs = TArray<void*>(), const FPropertyAccessChangeNotify* InChangeNotify = nullptr, const ESetErrorState SetErrorState = ESetErrorState::Yes);
	FUPyConversionResult PythonizeProperty_InContainer(const FProperty* Prop, const void* BaseAddr, const int32 ArrayIndex, PyObject*& OutPyObj, const EUPyConversionMethod ConversionMethod = EUPyConversionMethod::Copy, PyObject* OwnerPyObj = nullptr, const ESetErrorState SetErrorState = ESetErrorState::Yes);

	/**
	 * Helper function used to emit property change notifications as value changes are made
	 * This function should be called when you know the value will actually change (or know you want to emit the notifications for it changing) and will do
	 * the pre-change notify, invoke the passed delegate to perform the change, then do the post-change notify
	 */
	void EmitPropertyChangeNotifications(const FPropertyAccessChangeNotify* InChangeNotify, const bool bIdenticalValue, const TFunctionRef<void()>& InDoChangeFunc);
}
