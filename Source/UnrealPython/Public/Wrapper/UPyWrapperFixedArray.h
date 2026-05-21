
#pragma once

#include "UPyCommon.h"
#include "Wrapper/UPyWrapperOwnerContext.h"
#include "Utils/UPyUtil.h"
#include "Utils/UPyGenUtil.h"
#include "Wrapper/UPyWrapperBase.h"

/** Python type for FUPyWrapperFixedArray */
extern PyTypeObject UPyWrapperFixedArrayType;

/** Initialize the PyWrapperFixedArray types and add them to the given Python module */
void InitializeUPyWrapperFixedArray(UPyGenUtil::FNativePythonModule& ModuleInfo);

/** Type for all Unreal exposed fixed-array instances */
struct FUPyWrapperFixedArray : public FUPyWrapperBase
{
	/** The owner of the wrapped fixed-array instance (if any) */
	FUPyWrapperOwnerContext OwnerContext;

	/** Property describing the fixed-array */
	UPyUtil::FConstPropOnScope ArrayProp;

	/** Wrapped fixed-array instance */
	void* ArrayInstance;

	/** New this wrapper instance (called via tp_new for Python, or directly in C++) */
	static FUPyWrapperFixedArray* New(PyTypeObject* InType);

	/** Free this wrapper instance (called via tp_dealloc for Python) */
	static void Free(FUPyWrapperFixedArray* InSelf);

	/** Initialize this wrapper (called via tp_init for Python, or directly in C++) */
	static int Init(FUPyWrapperFixedArray* InSelf, const UPyUtil::FPropertyDef& InPropDef, const int32 InLen);

	/** Initialize this wrapper instance to the given value (called via tp_init for Python, or directly in C++) */
	static int Init(FUPyWrapperFixedArray* InSelf, const FUPyWrapperOwnerContext& InOwnerContext, const FProperty* InProp, void* InValue, const EUPyConversionMethod InConversionMethod);

	/** Deinitialize this wrapper instance (called via Init and Free to restore the instance to its New state) */
	static void Deinit(FUPyWrapperFixedArray* InSelf);

	/** Called to validate the internal state of this wrapper instance prior to operating on it (should be called by all functions that expect to operate on an initialized type; will set an error state on failure) */
	static bool ValidateInternalState(FUPyWrapperFixedArray* InSelf);

	/** Cast the given Python object to this wrapped type (returns a new reference) */
	static FUPyWrapperFixedArray* CastPyObject(PyObject* InPyObject, FUPyConversionResult* OutCastResult = nullptr);

	/** Cast the given Python object to this wrapped type, or attempt to convert the type into a new wrapped instance (returns a new reference) */
	static FUPyWrapperFixedArray* CastPyObject(PyObject* InPyObject, PyTypeObject* InType, const UPyUtil::FPropertyDef& InPropDef, FUPyConversionResult* OutCastResult = nullptr);

	/** Get the raw pointer to the element at index N (negative indexing not supported) */
	static void* GetItemPtr(FUPyWrapperFixedArray* InSelf, Py_ssize_t InIndex);

	/** Get the length of this container (equivalent to 'len(x)' in Python) */
	static Py_ssize_t Len(FUPyWrapperFixedArray* InSelf);

	/** Get the item at index N (equivalent to 'x[N]' in Python, returns new reference) */
	static PyObject* GetItem(FUPyWrapperFixedArray* InSelf, Py_ssize_t InIndex);

	/** Set the item at index N (equivalent to 'x[N] = v' in Python) */
	static int SetItem(FUPyWrapperFixedArray* InSelf, Py_ssize_t InIndex, PyObject* InValue);

	/** Does this container have an entry with the given value? (equivalent to 'v in x' in Python) */
	static int Contains(FUPyWrapperFixedArray* InSelf, PyObject* InValue);

	/** Concatenate the other object to this one, returning a new container (equivalent to 'x + o' in Python, returns new reference) */
	static FUPyWrapperFixedArray* Concat(FUPyWrapperFixedArray* InSelf, PyObject* InOther);

	/** Repeat this container by N, returning a new container (equivalent to 'x * N' in Python, returns new reference) */
	static FUPyWrapperFixedArray* Repeat(FUPyWrapperFixedArray* InSelf, Py_ssize_t InMultiplier);
};

typedef TUPyPtr<FUPyWrapperFixedArray> FUPyWrapperFixedArrayPtr;
