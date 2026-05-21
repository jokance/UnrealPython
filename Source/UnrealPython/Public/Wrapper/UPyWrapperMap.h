
#pragma once

#include "Wrapper//UPyWrapperBase.h"
#include "Wrapper/UPyWrapperOwnerContext.h"
#include "Utils/UPyUtil.h"


/** Python type for FUPyWrapperMap */
extern PyTypeObject UPyWrapperMapType;

/** Initialize the PyWrapperMap types and add them to the given Python module */
void InitializeUPyWrapperMap(UPyGenUtil::FNativePythonModule& ModuleInfo);

/** Type for all Unreal exposed map instances */
struct FUPyWrapperMap : public FUPyWrapperBase
{
	/** The owner of the wrapped map instance (if any) */
	FUPyWrapperOwnerContext OwnerContext;

	/** Property describing the map */
	UPyUtil::FConstMapPropOnScope MapProp;

	/** Wrapped map instance */
	void* MapInstance;

	/** New this wrapper instance (called via tp_new for Python, or directly in C++) */
	static FUPyWrapperMap* New(PyTypeObject* InType);

	/** Free this wrapper instance (called via tp_dealloc for Python) */
	static void Free(FUPyWrapperMap* InSelf);

	/** Initialize this wrapper (called via tp_init for Python, or directly in C++) */
	static int Init(FUPyWrapperMap* InSelf, const UPyUtil::FPropertyDef& InKeyDef, const UPyUtil::FPropertyDef& InValueDef);

	/** Initialize this wrapper instance to the given value (called via tp_init for Python, or directly in C++) */
	static int Init(FUPyWrapperMap* InSelf, const FUPyWrapperOwnerContext& InOwnerContext, const FMapProperty* InProp, void* InValue, const EUPyConversionMethod InConversionMethod);

	/** Deinitialize this wrapper instance (called via Init and Free to restore the instance to its New state) */
	static void Deinit(FUPyWrapperMap* InSelf);

	/** Called to validate the internal state of this wrapper instance prior to operating on it (should be called by all functions that expect to operate on an initialized type; will set an error state on failure) */
	static bool ValidateInternalState(FUPyWrapperMap* InSelf);

	/** Cast the given Python object to this wrapped type (returns a new reference) */
	static FUPyWrapperMap* CastPyObject(PyObject* InPyObject, FUPyConversionResult* OutCastResult = nullptr);

	/** Cast the given Python object to this wrapped type, or attempt to convert the type into a new wrapped instance (returns a new reference) */
	static FUPyWrapperMap* CastPyObject(PyObject* InPyObject, PyTypeObject* InType, const UPyUtil::FPropertyDef& InKeyDef, const UPyUtil::FPropertyDef& InValueDef, FUPyConversionResult* OutCastResult = nullptr);

	/** Get the length of this container (equivalent to 'len(x)' in Python) */
	static Py_ssize_t Len(FUPyWrapperMap* InSelf);

	/** Get the item with key K (equivalent to 'x[K]' in Python, returns new reference) */
	static PyObject* GetItem(FUPyWrapperMap* InSelf, PyObject* InKey);

	/** Get the item with key K (equivalent to 'x.get(K, D)' in Python, returns new reference) */
	static PyObject* GetItem(FUPyWrapperMap* InSelf, PyObject* InKey, PyObject* InDefault);

	/** Set the item with key K (equivalent to 'x[K] = v' in Python) */
	static int SetItem(FUPyWrapperMap* InSelf, PyObject* InKey, PyObject* InValue);

	/** Set the item with key K if K isn't in the map and return the value of K (equivalent to 'x.setdefault(K, v)' in Python) */
	static PyObject* SetDefault(FUPyWrapperMap* InSelf, PyObject* InKey, PyObject* InValue);

	/** Does this container have an entry with the given value? (equivalent to 'v in x' in Python) */
	static int Contains(FUPyWrapperMap* InSelf, PyObject* InKey);

	/** Remove all values from this container (equivalent to 'x.clear()' in Python) */
	static int Clear(FUPyWrapperMap* InSelf);

	/** Remove and return the value for key K if present, otherwise return the default, or raise KeyError if no default is given (equivalent to 'x.popitem()' in Python, returns new reference) */
	static PyObject* Pop(FUPyWrapperMap* InSelf, PyObject* InKey, PyObject* InDefault);

	/** Remove and return an arbitrary pair from this map (equivalent to 'x.popitem()' in Python, returns new reference) */
	static PyObject* PopItem(FUPyWrapperMap* InSelf);

	/** Update this map from another (equivalent to 'x.update(o)' in Python) */
	static int Update(FUPyWrapperMap* InSelf, PyObject* InOther);

	/** Get a Python list containing the items from this map as key->value pairs */
	static PyObject* Items(FUPyWrapperMap* InSelf);

	/** Get a Python list containing the keys from this map */
	static PyObject* Keys(FUPyWrapperMap* InSelf);

	/** Get a Python list containing the values from this map */
	static PyObject* Values(FUPyWrapperMap* InSelf);

	/** Create a new map of keys from the sequence using the given value (types are inferred) */
	static FUPyWrapperMap* FromKeys(PyObject* InSequence, PyObject* InValue, PyTypeObject* InType);
};


typedef TUPyPtr<FUPyWrapperMap> FUPyWrapperMapPtr;

PyObject* GetUPyWrapperMapItems(FUPyWrapperMap* InSelf);
PyObject* GetUPyWrapperMapKeys(FUPyWrapperMap* InSelf);
PyObject* GetUPyWrapperMapValues(FUPyWrapperMap* InSelf);
