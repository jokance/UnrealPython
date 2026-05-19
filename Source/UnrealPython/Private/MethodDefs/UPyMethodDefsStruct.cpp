// Copyright Epic Games, Inc. All Rights Reserved.

#include "UPyCommon.h"
#include  "Core/UPyConversion.h"
#include "Wrapper/UPyWrapperTypeRegistry.h"
#include "Utils/UPyGenUtil.h"

static PyObject* PostInit(PyObject* InSelf)
{
	Py_RETURN_NONE;
}

static PyObject* StaticStruct2Py(PyTypeObject* InType)
{
	const UScriptStruct* Struct = FUPyWrapperTypeRegistry::Get().FindStruct(InType);
	return UPyConversion::Pythonize(Struct);
}

PyMethodDef StructPyMethodDefs[] = {
	{ "StaticStruct", (PyCFunction)(&StaticStruct2Py), METH_NOARGS | METH_CLASS, UPyDoc_STR("StaticStruct(cls) -> ScriptStruct -- get the Unreal struct of this type") },
	{ UPyGenUtil::PostInitFuncName, (PyCFunction)(&PostInit), METH_NOARGS, UPyDoc_STR("_post_init(self) -> None -- called during Unreal struct initialization") },
{nullptr}
};
