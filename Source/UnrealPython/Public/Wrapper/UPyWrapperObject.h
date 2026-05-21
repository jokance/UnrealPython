
#pragma once

#include "Wrapper/UPyWrapperObjectBase.h"

/** Python type for FUPyWrapperObject */
extern UNREALPYTHON_API PyTypeObject UPyWrapperObjectType;

/** Initialize the PyWrapperObject types and add them to the given Python module */
void InitializeUPyWrapperObject(UPyGenUtil::FNativePythonModule& ModuleInfo);

class UNREALPYTHON_API FUPyWrapperObject: public FUPyWrapperObjectBase
{
public:

	static PyTypeObject* GetPyType()
	{
		return &UPyWrapperObjectType;
	}

	UObject* ValuePtr()
	{
		return ObjectInstance;
	}
};

UNREALPYTHON_API FUPyWrapperObject* UPyIsObject(PyObject* InSelf);
