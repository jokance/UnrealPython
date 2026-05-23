
#pragma once

#include "UPyCommon.h"
#include "Wrapper/UPyWrapperBase.h"
#include "Wrapper/UPyWrapperOwnerContext.h"
#include "Core/UPyConversion.h"
#include "UObject/Class.h"
#include "UObject/PropertyAccessUtil.h"
#include "Templates/TypeCompatibleBytes.h"
#include "Core/UPyPtr.h"
#include "Utils/UPyUtil.h"
#include "DynamicTypes/UPyGeneratedWrappedType.h"

class UObjectRedirector;

/** Python type for FUPyWrapperStruct */
extern PyTypeObject UPyWrapperStructType;

/** Initialize the PyWrapperStruct types and add them to the given Python module */
void InitializeUPyWrapperStruct(UPyGenUtil::FNativePythonModule& ModuleInfo);

/** Type for all Unreal exposed struct instances */
struct FUPyWrapperStruct : public FUPyWrapperBase
{
	/** The owner of the wrapped struct instance (if any) */
	FUPyWrapperOwnerContext OwnerContext;

	/** Struct type of this instance */
	TObjectPtr<const UScriptStruct> ScriptStruct;

	/** Wrapped struct instance */
	void* StructInstance;

	/** New this wrapper instance (called via tp_new for Python, or directly in C++) */
	static FUPyWrapperStruct* New(PyTypeObject* InType);

	/** Free this wrapper instance (called via tp_dealloc for Python) */
	static void Free(FUPyWrapperStruct* InSelf);

	/** Initialize this wrapper instance (called via tp_init for Python, or directly in C++) */
	static int Init(FUPyWrapperStruct* InSelf);

	/** Initialize this wrapper instance to the given value (called via tp_init for Python, or directly in C++) */
	static int Init(FUPyWrapperStruct* InSelf, const FUPyWrapperOwnerContext& InOwnerContext, const UScriptStruct* InStruct, void* InValue, const EUPyConversionMethod InConversionMethod);

	/** Deinitialize this wrapper instance (called via Init and Free to restore the instance to its New state) */
	static void Deinit(FUPyWrapperStruct* InSelf);

	/** Detach this wrapper from an owned property while preserving its current value as an independent copy */
	static bool DetachFromOwner(FUPyWrapperStruct* InSelf);

	/** Called to validate the internal state of this wrapper instance prior to operating on it (should be called by all functions that expect to operate on an initialized type; will set an error state on failure) */
	static bool ValidateInternalState(FUPyWrapperStruct* InSelf);

	/** Cast the given Python object to this wrapped type (returns a new reference) */
	static FUPyWrapperStruct* CastPyObject(PyObject* InPyObject, FUPyConversionResult* OutCastResult = nullptr);

	/** Cast the given Python object to this wrapped type, or attempt to convert the type into a new wrapped instance (returns a new reference) */
	static FUPyWrapperStruct* CastPyObject(PyObject* InPyObject, PyTypeObject* InType, FUPyConversionResult* OutCastResult = nullptr);

	/** "Make" this struct from the given arguments, either using a custom make function or by setting the named property values on this instance (called via generated code) */
	static int MakeStruct(FUPyWrapperStruct* InSelf, PyObject* InArgs, PyObject* InKwds);

	/** "Break" this struct into a tuple, either using a custom break function or by getting each property value on this instance (called via generated code) */
	static PyObject* BreakStruct(FUPyWrapperStruct* InSelf);

	/** Get a property value from this instance (called via generated code) */
	static PyObject* GetPropertyValue(FUPyWrapperStruct* InSelf, const UPyGenUtil::FGeneratedWrappedProperty& InPropDef, const char* InPythonAttrName);

	/** Set a property value on this instance (called via generated code) */
	static int SetPropertyValue(FUPyWrapperStruct* InSelf, PyObject* InValue, const UPyGenUtil::FGeneratedWrappedProperty& InPropDef, const char* InPythonAttrName, const EPropertyAccessChangeNotifyMode InNotifyMode = EPropertyAccessChangeNotifyMode::Never, const uint64 InReadOnlyFlags = PropertyAccessUtil::RuntimeReadOnlyFlags);

	/** Call a make function on this instance (MakeStruct internal use only) */
	static int CallMakeFunction_Impl(FUPyWrapperStruct* InSelf, PyObject* InArgs, PyObject* InKwds, const UPyGenUtil::FGeneratedWrappedFunction& InFuncDef);

	/** Call a break function on this instance (BreakStruct internal use only) */
	static PyObject* CallBreakFunction_Impl(FUPyWrapperStruct* InSelf, const UPyGenUtil::FGeneratedWrappedFunction& InFuncDef);

	/** Call a dynamic function on this instance (CallFunction internal use only) */
	static PyObject* CallDynamicFunction_Impl(FUPyWrapperStruct* InSelf, PyObject* InArgs, PyObject* InKwds, const UPyGenUtil::FGeneratedWrappedFunction& InFuncDef, const UPyGenUtil::FGeneratedWrappedMethodParameter& InSelfParam, const UPyGenUtil::FGeneratedWrappedMethodParameter& InSelfReturn, const char* InPythonFuncName);

	/** Implementation of the "call" logic for a dynamic Python method with no arguments (internal Python bindings use only) */
	static PyObject* CallDynamicMethodNoArgs_Impl(FUPyWrapperStruct* InSelf, void* InClosure);

	/** Implementation of the "call" logic for a dynamic Python method with arguments (internal Python bindings use only) */
	static PyObject* CallDynamicMethodWithArgs_Impl(FUPyWrapperStruct* InSelf, PyObject* InArgs, PyObject* InKwds, void* InClosure);

	/** Implementation of the "call" logic for a Python operator function (internal CallOperator use only) */
	static PyObject* CallOperatorFunction_Impl(FUPyWrapperStruct* InSelf, PyObject* InRHS, const UPyGenUtil::FGeneratedWrappedOperatorFunction& InOpFunc, const TOptional<EUPyConversionResultState> InRequiredConversionResult = TOptional<EUPyConversionResultState>(), FUPyConversionResult* OutRHSConversionResult = nullptr);

	/** Implementation of the "call" logic for a Python operator function (internal Python bindings use only) */
	static PyObject* CallOperator_Impl(FUPyWrapperStruct* InSelf, PyObject* InRHS, const UPyGenUtil::EGeneratedWrappedOperatorType InOpType);

	/** Implementation of the "getter" logic for a Python descriptor reading from an struct property (internal Python bindings use only) */
	static PyObject* Getter_Impl(FUPyWrapperStruct* InSelf, void* InClosure);

	/** Implementation of the "setter" logic for a Python descriptor writing to an struct property (internal Python bindings use only) */
	static int Setter_Impl(FUPyWrapperStruct* InSelf, PyObject* InValue, void* InClosure);

	/** Get a pointer to the typed struct instance this wrapper represents */
	template <typename StructType>
	static StructType* GetTypedStructPtr(FUPyWrapperStruct* InSelf)
	{
		return (StructType*)InSelf->StructInstance;
	}

	/** Get a reference to the typed struct instance this wrapper represents */
	template <typename StructType>
	static StructType& GetTypedStruct(FUPyWrapperStruct* InSelf)
	{
		return *GetTypedStructPtr<StructType>(InSelf);
	}
};

/** Specialized version of FUPyWrapperStruct that can store its struct payload inline (requires a known type) */
template <typename InlineType>
struct TUPyWrapperInlineStruct : public FUPyWrapperStruct
{
	typedef InlineType WrappedType;

	/** Inline struct instance (do not use directly, StructInstance will be set to point to this when appropriate via FUPyWrapperStructAllocationPolicy_Inline) */
	TTypeCompatibleBytes<WrappedType> InlineStructInstance;

	/** Cast the given Python object to this wrapped type (returns a new reference) */
	static TUPyWrapperInlineStruct* CastPyObject(PyObject* InPyObject, FUPyConversionResult* OutCastResult = nullptr)
	{
		return (TUPyWrapperInlineStruct*)FUPyWrapperStruct::CastPyObject(InPyObject, OutCastResult);
	}

	/** Cast the given Python object to this wrapped type, or attempt to convert the type into a new wrapped instance (returns a new reference) */
	static TUPyWrapperInlineStruct* CastPyObject(PyObject* InPyObject, PyTypeObject* InType, FUPyConversionResult* OutCastResult = nullptr)
	{
		return (TUPyWrapperInlineStruct*)FUPyWrapperStruct::CastPyObject(InPyObject, InType, OutCastResult);
	}

	/** Get a pointer to the typed struct instance this wrapper represents */
	static WrappedType* GetTypedStructPtr(TUPyWrapperInlineStruct* InSelf)
	{
		return FUPyWrapperStruct::GetTypedStructPtr<WrappedType>(InSelf);
	}

	/** Get a reference to the typed struct instance this wrapper represents */
	static WrappedType& GetTypedStruct(TUPyWrapperInlineStruct* InSelf)
	{
		return FUPyWrapperStruct::GetTypedStruct<WrappedType>(InSelf);
	}

	/** Getter function for intrinsic field access (for use with PyGetSetDef) */
	template <typename FieldType, FieldType WrappedType::*FieldPtr>
	static PyObject* IntrinsicFieldGetter(TUPyWrapperInlineStruct* InSelf, void* InClosure)
	{
		return UPyConversion::Pythonize(GetTypedStruct(InSelf).*FieldPtr);
	}

	/** Setter function for intrinsic field access (for use with PyGetSetDef) */
	template <typename FieldType, FieldType WrappedType::*FieldPtr>
	static int IntrinsicFieldSetter(TUPyWrapperInlineStruct* InSelf, PyObject* InValue, void* InClosure)
	{
		if (!InValue)
		{
			UPyUtil::SetPythonError(PyExc_TypeError, InSelf, TEXT("Cannot delete attribute from type"));
			return -1;
		}

		if (!UPyConversion::Nativize(InValue, GetTypedStruct(InSelf).*FieldPtr))
		{
			return -1;
		}

		return 0;
	}

	/** Getter function for struct field access (for use with PyGetSetDef) */
	template <typename FieldType, FieldType WrappedType::*FieldPtr>
	static PyObject* StructFieldGetter(TUPyWrapperInlineStruct* InSelf, void* InClosure)
	{
		return UPyConversion::PythonizeStructInstance(GetTypedStruct(InSelf).*FieldPtr);
	}

	/** Setter function for struct field access (for use with PyGetSetDef) */
	template <typename FieldType, FieldType WrappedType::*FieldPtr>
	static int StructFieldSetter(TUPyWrapperInlineStruct* InSelf, PyObject* InValue, void* InClosure)
	{
		if (!InValue)
		{
			UPyUtil::SetPythonError(PyExc_TypeError, InSelf, TEXT("Cannot delete attribute from type"));
			return -1;
		}

		if (!UPyConversion::NativizeStructInstance(InValue, GetTypedStruct(InSelf).*FieldPtr))
		{
			return -1;
		}

		return 0;
	}

	/** Call a function with no parameters and no return value (for use with PyMethodDef) */
	template <void(WrappedType::*FuncPtr)()>
	static PyObject* CallFunc_NoParams_NoReturn(TUPyWrapperInlineStruct* InSelf)
	{
		(GetTypedStruct(InSelf).*FuncPtr)();
		Py_RETURN_NONE;
	}

	/** Call a const function with no parameters and no return value (for use with PyMethodDef) */
	template <void(WrappedType::*FuncPtr)() const>
	static PyObject* CallConstFunc_NoParams_NoReturn(TUPyWrapperInlineStruct* InSelf)
	{
		(GetTypedStruct(InSelf).*FuncPtr)();
		Py_RETURN_NONE;
	}

	/** Call a function with no parameters and an intrinsic return value (for use with PyMethodDef) */
	template <typename ReturnType, ReturnType(WrappedType::*FuncPtr)()>
	static PyObject* CallFunc_NoParams_IntrinsicReturn(TUPyWrapperInlineStruct* InSelf)
	{
		return UPyConversion::Pythonize((GetTypedStruct(InSelf).*FuncPtr)());
	}

	/** Call a const function with no parameters and an intrinsic return value (for use with PyMethodDef) */
	template <typename ReturnType, ReturnType(WrappedType::*FuncPtr)() const>
	static PyObject* CallConstFunc_NoParams_IntrinsicReturn(TUPyWrapperInlineStruct* InSelf)
	{
		return UPyConversion::Pythonize((GetTypedStruct(InSelf).*FuncPtr)());
	}

	/** Call a function with no parameters and a struct return value (for use with PyMethodDef) */
	template <typename ReturnType, ReturnType(WrappedType::*FuncPtr)()>
	static PyObject* CallFunc_NoParams_StructReturn(TUPyWrapperInlineStruct* InSelf)
	{
		return UPyConversion::PythonizeStructInstance((GetTypedStruct(InSelf).*FuncPtr)());
	}

	/** Call a const function with no parameters and a struct return value (for use with PyMethodDef) */
	template <typename ReturnType, ReturnType(WrappedType::*FuncPtr)() const>
	static PyObject* CallConstFunc_NoParams_StructReturn(TUPyWrapperInlineStruct* InSelf)
	{
		return UPyConversion::PythonizeStructInstance((GetTypedStruct(InSelf).*FuncPtr)());
	}
};

/** Struct allocation policy interface */
class IUPyWrapperStructAllocationPolicy
{
public:
	/** Allocate memory to hold an instance of the given struct and return the result */
	virtual void* AllocateStruct(const FUPyWrapperStruct* InSelf, const UScriptStruct* InStruct) const = 0;

	/** Free memory previously allocated with AllocateStruct */
	virtual void FreeStruct(const FUPyWrapperStruct* InSelf, void* InAlloc) const = 0;
};

/** Inline struct factory interface */
class IUPyWrapperInlineStructFactory
{
public:
	/** Get the name of the Unreal struct this factory is for */
	virtual FTopLevelAssetPath GetStructName() const = 0;

	/** Get the size of the Python object that should be constructed (in bytes) */
	virtual int32 GetPythonObjectSizeBytes() const = 0;

	/** Get the allocation policy of the Python object */
	virtual const IUPyWrapperStructAllocationPolicy* GetPythonObjectAllocationPolicy() const = 0;
};

/** Concrete implementation of an inline struct factory for the given type */
template <typename InlineType>
class TUPyWrapperInlineStructFactory : public IUPyWrapperInlineStructFactory
{
private:
	class FUPyWrapperStructAllocationPolicy_Inline : public IUPyWrapperStructAllocationPolicy
	{
	public:
		virtual void* AllocateStruct(const FUPyWrapperStruct* InSelf, const UScriptStruct* InStruct) const override
		{
			return (void*)&static_cast<const TUPyWrapperInlineStruct<InlineType>*>(InSelf)->InlineStructInstance;
		}

		virtual void FreeStruct(const FUPyWrapperStruct* InSelf, void* InAlloc) const override
		{
		}
	};

public:
	virtual FTopLevelAssetPath GetStructName() const override
	{
		return TBaseStructure<InlineType>::Get()->GetStructPathName();
	}

	virtual int32 GetPythonObjectSizeBytes() const override
	{
		return sizeof(TUPyWrapperInlineStruct<InlineType>);
	}

	virtual const IUPyWrapperStructAllocationPolicy* GetPythonObjectAllocationPolicy() const override
	{
		static const FUPyWrapperStructAllocationPolicy_Inline InlineAllocPolicy = FUPyWrapperStructAllocationPolicy_Inline();
		return &InlineAllocPolicy;
	}
};

typedef TUPyPtr<FUPyWrapperStruct> FUPyWrapperStructPtr;
