
#pragma once

#include "Wrapper/UPyWrapperBasic.h"
#include "Core/UPyPtr.h"
#include "Core/UPyConversionResult.h"

/** Python type for FUPyWrapperFieldPath */
extern PyTypeObject UPyWrapperFieldPathType;

/** Initialize the PyWrapperFieldPath types and add them to the given Python module */
void InitializeUPyWrapperFieldPath(UPyGenUtil::FNativePythonModule& ModuleInfo);

/** Type for all Unreal exposed FUPyWrapperFieldPath instances */
struct FUPyWrapperFieldPath : public TUPyWrapperBasic<FFieldPath, FUPyWrapperFieldPath>
{
	typedef TUPyWrapperBasic<FFieldPath, FUPyWrapperFieldPath> Super;

	/** Cast the given Python object to this wrapped type (returns a new reference) */
	static FUPyWrapperFieldPath* CastPyObject(PyObject* InPyObject, FUPyConversionResult* OutCastResult = nullptr);

	/** Cast the given Python object to this wrapped type, or attempt to convert the type into a new wrapped instance (returns a new reference) */
	static FUPyWrapperFieldPath* CastPyObject(PyObject* InPyObject, PyTypeObject* InType, FUPyConversionResult* OutCastResult = nullptr);

	/** Initialize the value of this wrapper instance (internal) */
	static void InitValue(FUPyWrapperFieldPath* InSelf, const FFieldPath& InValue);

	/** Deinitialize this wrapper instance (called via Init and Free to restore the instance to its New state) */
	static void DeinitValue(FUPyWrapperFieldPath* InSelf);
};

typedef TUPyPtr<FUPyWrapperFieldPath> FUPyWrapperFieldPathPtr;

