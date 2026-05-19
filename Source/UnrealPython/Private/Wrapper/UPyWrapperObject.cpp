// Copyright Epic Games, Inc. All Rights Reserved.

#include "Wrapper/UPyWrapperObject.h"
#include "Wrapper/UPyWrapperTypeRegistry.h"

PyTypeObject UPyWrapperObjectType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"Object", /* tp_name */
	sizeof(FUPyWrapperObject), /* tp_basicsize */
};

struct FMethods_WrapperObject
{
	
};

PyMethodDef ObjectPyMethodDefs[] = {
	{nullptr}
};

void InitializeUPyWrapperObject(UPyGenUtil::FNativePythonModule& ModuleInfo)
{
	PyTypeObject* PyType = &UPyWrapperObjectType;

	PyType->tp_base = &UPyWrapperObjectBaseType;
	PyType->tp_methods = ObjectPyMethodDefs;
	PyType->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;

	if (PyType_Ready(PyType) == 0)
	{
		ModuleInfo.AddType(PyType);
		FUPyWrapperTypeRegistry::Get().RegisterWrappedClassType(UObject::StaticClass(), PyType);
	}
}

FUPyWrapperObject* UPyIsObject(PyObject* InSelf)
{
	if (InSelf && PyObject_IsInstance(InSelf, (PyObject*)&UPyWrapperObjectType))
	{
		return (FUPyWrapperObject*)InSelf;
	}
	return nullptr;
}
