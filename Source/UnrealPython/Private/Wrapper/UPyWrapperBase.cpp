// Copyright Epic Games, Inc. All Rights Reserved.

#include "Wrapper/UPyWrapperBase.h"
#include "Core/UPyGIL.h"
#include "Utils/UPyUtil.h"

PyTypeObject UPyWrapperBaseType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"WrapperBase", /* tp_name */
	sizeof(FUPyWrapperBase), /* tp_basicsize */
};

/** Python wrapper object for FUPyWrapperBaseMetaData */
struct FUPyWrapperBaseMetaDataObject
{
	/** Common Python Object */
	PyObject_HEAD

	/** Type meta-data */
	FUPyWrapperBaseMetaData* MetaData;
};

PyTypeObject UPyWrapperBaseMetaDataType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"WrapperBaseMetaData", /* tp_name */
	sizeof(FUPyWrapperBaseMetaDataObject), /* tp_basicsize */
};

FUPyWrapperBase* FUPyWrapperBase::New(PyTypeObject* InType)
{
	FUPyWrapperBase* Self = (FUPyWrapperBase*)InType->tp_alloc(InType, 0);
	// if (Self)
	// {
	// 	FPyReferenceCollector::Get().AddWrappedInstance(Self);
	// }
	return Self;
}

void FUPyWrapperBase::Free(FUPyWrapperBase* InSelf)
{
	// FPyReferenceCollector::Get().RemoveWrappedInstance(InSelf);
	Deinit(InSelf);
	Py_TYPE(InSelf)->tp_free((PyObject*)InSelf);
}

int FUPyWrapperBase::Init(FUPyWrapperBase* InSelf)
{
	Deinit(InSelf);
	return 0;
}

void FUPyWrapperBase::Deinit(FUPyWrapperBase* InSelf)
{
}

void InitializePyWrapperBaseMetaDataType()
{
	PyTypeObject* PyType = &UPyWrapperBaseMetaDataType;

	PyType->tp_new = (newfunc)&PyType_GenericNew;

	PyType->tp_flags = Py_TPFLAGS_DEFAULT;
	PyType->tp_doc = "Python wrapper object for FUPyWrapperBaseMetaData";

	PyType_Ready(PyType);
}

// ==================== Wrapper Base Type BEGIN ====================
struct FFuncs_WrapperBase
{
	static PyObject* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
	{
		return (PyObject*)FUPyWrapperBase::New(InType);
	}

	static void Dealloc(FUPyWrapperBase* InSelf)
	{
		FUPyWrapperBase::Free(InSelf);
	}

	static int Init(FUPyWrapperBase* InSelf, PyObject* InArgs, PyObject* InKwds)
	{
		return FUPyWrapperBase::Init(InSelf);
	}
};

void InitializeUPyWrapperBase(UPyGenUtil::FNativePythonModule& ModuleInfo)
{
	PyTypeObject* PyType = &UPyWrapperBaseType;

	PyType->tp_new = (newfunc)&FFuncs_WrapperBase::New;
	PyType->tp_dealloc = (destructor)&FFuncs_WrapperBase::Dealloc;
	PyType->tp_init = (initproc)&FFuncs_WrapperBase::Init;

	PyType->tp_flags = Py_TPFLAGS_DEFAULT;
	PyType->tp_doc = "Base type for all Unreal exposed types";

	if (PyType_Ready(PyType) == 0)
	{
		ModuleInfo.AddType(PyType);
	}

	InitializePyWrapperBaseMetaDataType();
}
// ==================== Wrapper Base Type END ====================

// ==================== Wrapper Base Meta Data BEGIN ====================
void FUPyWrapperBaseMetaData::SetMetaData(PyTypeObject* PyType, FUPyWrapperBaseMetaData* MetaData)
{
	if (PyType && PyType->tp_dict)
	{
		FUPyScopedGIL GIL;

		FUPyWrapperBaseMetaDataObject* PyWrapperMetaData = (FUPyWrapperBaseMetaDataObject*)PyDict_GetItemString(PyType->tp_dict, "_wrapper_meta_data");
		if (!PyWrapperMetaData)
		{
			PyWrapperMetaData = (FUPyWrapperBaseMetaDataObject*)PyObject_CallObject((PyObject*)&UPyWrapperBaseMetaDataType, nullptr);
			checkf(PyWrapperMetaData, TEXT("PyWrapperMetaData is null for PyType: %s"),
				*UPyUtil::GetFriendlyTypename(PyType));

			PyDict_SetItemString(PyType->tp_dict, "_wrapper_meta_data", (PyObject*)PyWrapperMetaData);
			Py_DECREF(PyWrapperMetaData);
		}
		PyWrapperMetaData->MetaData = MetaData;
	}
}

FUPyWrapperBaseMetaData* FUPyWrapperBaseMetaData::GetMetaData(PyTypeObject* PyType)
{
	if (PyType && PyType->tp_dict)
	{
		FUPyScopedGIL GIL;

		FUPyWrapperBaseMetaDataObject* PyWrapperMetaData = (FUPyWrapperBaseMetaDataObject*)PyDict_GetItemString(PyType->tp_dict, "_wrapper_meta_data");
		if (PyWrapperMetaData)
		{
			return PyWrapperMetaData->MetaData;
		}
	}
	return nullptr;
}

FUPyWrapperBaseMetaData* FUPyWrapperBaseMetaData::GetMetaData(FUPyWrapperBase* Instance)
{
	return GetMetaData(Py_TYPE(Instance));
}
// ==================== Wrapper Base Meta Data END ====================

UUPythonResourceOwner::UUPythonResourceOwner(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}
