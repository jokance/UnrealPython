
#pragma once

#include "Wrapper/UPyWrapperBase.h"
#include "Wrapper/UPyWrapperOwnerContext.h"
#include "Utils/UPyUtil.h"
#include "Core/UPyPtr.h"
#include "Utils/UPyGenUtil.h"

/** Python type for FUPyWrapperArray */
extern PyTypeObject UPyWrapperArrayType;

/** Initialize the PyWrapperArray types and add them to the given Python module */
void InitializeUPyWrapperArray(UPyGenUtil::FNativePythonModule& ModuleInfo);

/** Type for all Unreal exposed array instances */
struct FUPyWrapperArray : public FUPyWrapperBase
{
	/** The owner of the wrapped array instance (if any) */
	FUPyWrapperOwnerContext OwnerContext;

	/** Property describing the array */
	UPyUtil::FConstArrayPropOnScope ArrayProp;

	/** Wrapped array instance */
	void* ArrayInstance;

	/** New this wrapper instance (called via tp_new for Python, or directly in C++) */
	static FUPyWrapperArray* New(PyTypeObject* InType);

	/** Free this wrapper instance (called via tp_dealloc for Python) */
	static void Free(FUPyWrapperArray* InSelf);

	/** Initialize this wrapper (called via tp_init for Python, or directly in C++) */
	static int Init(FUPyWrapperArray* InSelf, const UPyUtil::FPropertyDef& InElementDef);

	/** Initialize this wrapper instance to the given value (called via tp_init for Python, or directly in C++) */
	static int Init(FUPyWrapperArray* InSelf, const FUPyWrapperOwnerContext& InOwnerContext, const FArrayProperty* InProp, void* InValue, const EUPyConversionMethod InConversionMethod);

	/** Deinitialize this wrapper instance (called via Init and Free to restore the instance to its New state) */
	static void Deinit(FUPyWrapperArray* InSelf);

	/** Called to validate the internal state of this wrapper instance prior to operating on it (should be called by all functions that expect to operate on an initialized type; will set an error state on failure) */
	static bool ValidateInternalState(FUPyWrapperArray* InSelf);

	/** Cast the given Python object to this wrapped type (returns a new reference) */
	static FUPyWrapperArray* CastPyObject(PyObject* InPyObject, FUPyConversionResult* OutCastResult = nullptr);

	/** Cast the given Python object to this wrapped type, or attempt to convert the type into a new wrapped instance (returns a new reference) */
	static FUPyWrapperArray* CastPyObject(PyObject* InPyObject, PyTypeObject* InType, const UPyUtil::FPropertyDef& InElementDef, FUPyConversionResult* OutCastResult = nullptr);

	/** Get the length of this container (equivalent to 'len(x)' in Python) */
	static Py_ssize_t Len(FUPyWrapperArray* InSelf);

	/** Get the item at index N (equivalent to 'x[N]' in Python, returns new reference) */
	static PyObject* GetItem(FUPyWrapperArray* InSelf, Py_ssize_t InIndex);

	/** Set the item at index N (equivalent to 'x[N] = v' in Python) */
	static int SetItem(FUPyWrapperArray* InSelf, Py_ssize_t InIndex, PyObject* InValue);

	/** Does this container have an entry with the given value? (equivalent to 'v in x' in Python) */
	static int Contains(FUPyWrapperArray* InSelf, PyObject* InValue);

	/** Concatenate the other object to this one, returning a new container (equivalent to 'x + o' in Python, returns new reference) */
	static FUPyWrapperArray* Concat(FUPyWrapperArray* InSelf, PyObject* InOther);

	/** Concatenate the other object to this one in-place (equivalent to 'x += o' in Python) */
	static int ConcatInplace(FUPyWrapperArray* InSelf, PyObject* InOther);

	/** Repeat this container by N, returning a new container (equivalent to 'x * N' in Python, returns new reference) */
	static FUPyWrapperArray* Repeat(FUPyWrapperArray* InSelf, Py_ssize_t InMultiplier);

	/** Repeat this container by N in-place (equivalent to 'x *= N' in Python) */
	static int RepeatInplace(FUPyWrapperArray* InSelf, Py_ssize_t InMultiplier);

	/** Append the given value to the end this container (equivalent to 'x.append(v)' in Python) */
	static int Append(FUPyWrapperArray* InSelf, PyObject* InValue);

	/** Count the number of times that the given value appears in this container (equivalent to 'x.count(v)' in Python) */
	static Py_ssize_t Count(FUPyWrapperArray* InSelf, PyObject* InValue);

	/** Get the index of the first the given value appears in this container (equivalent to 'x.index(v)' in Python) */
	static Py_ssize_t Index(FUPyWrapperArray* InSelf, PyObject* InValue, Py_ssize_t InStartIndex = 0, Py_ssize_t InStopIndex = INDEX_NONE);

	/** Insert the given value into this container at the given index (equivalent to 'x.insert(i, v)' in Python) */
	static int Insert(FUPyWrapperArray* InSelf, Py_ssize_t InIndex, PyObject* InValue);

	/** Pop and return the value at the given index (or the end) of this container (equivalent to 'x.pop()' in Python) */
	static PyObject* Pop(FUPyWrapperArray* InSelf, Py_ssize_t InIndex = INDEX_NONE);

	/** Remove the given value from this container (equivalent to 'x.remove(v)' in Python) */
	static int Remove(FUPyWrapperArray* InSelf, PyObject* InValue);

	/** Remove all values from this container (equivalent to 'x.clear()' in Python) */
	static int Clear(FUPyWrapperArray* InSelf);

	/** Reverse this container (equivalent to 'x.reverse()' in Python) */
	static int Reverse(FUPyWrapperArray* InSelf);

	/** Sort this container (equivalent to 'x.sort()' in Python) */
	static int Sort(FUPyWrapperArray* InSelf, PyObject* InKey, bool InReverse);

	/** Resize this container (equivalent to 'x.resize(l)' in Python) */
	static int Resize(FUPyWrapperArray* InSelf, Py_ssize_t InLen);
};

/** Meta-data for all Unreal exposed array types */
// struct FUPyWrapperArrayMetaData : public FUPyWrapperBaseMetaData
// {
// 	PY_METADATA_METHODS(FUPyWrapperArrayMetaData, FGuid(0xFCD8DF06, 0x7AD6426F, 0x97482D82, 0x6E100C39))
//
// 	FUPyWrapperArrayMetaData()
// 	{
// 	}
//
// 	/** Add object references from the given Python object to the given collector */
// 	virtual void AddInstanceReferencedObjects(FUPyWrapperBase* Instance, FReferenceCollector& Collector) override;
// };

typedef TUPyPtr<FUPyWrapperArray> FUPyWrapperArrayPtr;
