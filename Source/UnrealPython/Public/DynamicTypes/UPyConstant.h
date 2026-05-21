
#pragma once

#include "UPyCommon.h"

/** Python type for FUPyConstantDescrObject */
extern PyTypeObject UPyConstantDescrType;

/** Initialize the PyConstant types */
void InitializeUPyConstant();

/** Function pointer used to convert a constant value pointer into a Python object */
typedef PyObject*(*PyConstantGetter)(PyTypeObject*, const void*);

/**
 * Definition for a constant value
 * This takes a pointer to some static or otherwise persistent data, along with a function used to convert this data into a Python object when needed
 * Compared to a template, this avoids variance in the Python-type which would require a new Python-type to be registered for each instantiation
 * Compared to storing a PyObject*, this avoids returning an instance that could be mutated and affect the constant value
 */
struct FUPyConstantDef
{
	/** A pointer to the constant context */
	const void* ConstantContext;

	/** Function pointer used to convert a constant data into a Python object */
	PyConstantGetter ConstantGetter;

	/** The name of the constant value */
	const char* ConstantName;

	/** The doc string of the constant value (may be null) */
	const char* ConstantDoc;

	/** Add a singular constant to the given type */
	static bool AddConstantToType(FUPyConstantDef* InConstant, PyTypeObject* InType);

	/** Add the given null-terminated table of constants to the given type */
	static bool AddConstantsToType(FUPyConstantDef* InConstants, PyTypeObject* InType);

	/** Add a singular constant to the given module */
	static bool AddConstantToModule(FUPyConstantDef* InConstant, PyObject* InModule);

	/** Add the given null-terminated table of constants to the given module */
	static bool AddConstantsToModule(FUPyConstantDef* InConstants, PyObject* InModule);

	/** Add a singular constant to the given dict */
	static bool AddConstantToDict(FUPyConstantDef* InConstant, PyObject* InDict);

	/** Add the given null-terminated table of constants to the given dict */
	static bool AddConstantsToDict(FUPyConstantDef* InConstants, PyObject* InDict);
};

/**
 * Python object for the descriptor of an constant value
 */
struct FUPyConstantDescrObject
{
	/** Common Python Object */
	PyObject_HEAD

	/** Name of the constant being described */
	PyObject* ConstantName;

	/** Pointer to the definition of this constant */
	const FUPyConstantDef* ConstantDef;

	/** New an instance */
	static FUPyConstantDescrObject* New(const FUPyConstantDef* InConstantDef);

	/** Free this instance */
	static void Free(FUPyConstantDescrObject* InSelf);
};
