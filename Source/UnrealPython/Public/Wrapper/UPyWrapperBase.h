// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UPyCommon.h"
#include "Utils/UPyGenUtil.h"
#include "UPyWrapperBase.generated.h"

/** Python type for FUPyWrapperBase */
extern PyTypeObject UPyWrapperBaseType;

/** Initialize the PyWrapperBase types and add them to the given Python module */
void InitializeUPyWrapperBase(UPyGenUtil::FNativePythonModule& ModuleInfo);

/** Base type for all Unreal exposed instances */
struct UNREALPYTHON_API FUPyWrapperBase
{
	/** Common Python Object */
	PyObject_HEAD

	/** New this wrapper instance (called via tp_new for Python, or directly in C++) */
	static FUPyWrapperBase* New(PyTypeObject* InType);

	/** Free this wrapper instance (called via tp_dealloc for Python) */
	static void Free(FUPyWrapperBase* InSelf);

	/** Initialize this wrapper instance (called via tp_init for Python, or directly in C++) */
	static int Init(FUPyWrapperBase* InSelf);

	/** Deinitialize this wrapper instance (called via Init and Free to restore the instance to its New state) */
	static void Deinit(FUPyWrapperBase* InSelf);
};

#define UPY_OVERRIDE_GETSET_METADATA(TYPE)																						\
static void SetMetaData(PyTypeObject* PyType, TYPE* MetaData) { FUPyWrapperBaseMetaData::SetMetaData(PyType, MetaData); }	\
static TYPE* GetMetaData(PyTypeObject* PyType) { return (TYPE*)FUPyWrapperBaseMetaData::GetMetaData(PyType); }				\
static TYPE* GetMetaData(FUPyWrapperBase* Instance) { return (TYPE*)FUPyWrapperBaseMetaData::GetMetaData(Instance); }

#define UPY_METADATA_METHODS(TYPE, GUID)																							\
UPY_OVERRIDE_GETSET_METADATA(TYPE)																							\
static FGuid StaticTypeId() { return (GUID); }																				\
virtual FGuid GetTypeId() const override { return StaticTypeId(); }

/** Base meta-data for all Unreal exposed types */
struct UNREALPYTHON_API FUPyWrapperBaseMetaData
{
	/** Set the meta-data object on the given type */
	static void SetMetaData(PyTypeObject* PyType, FUPyWrapperBaseMetaData* MetaData);

	/** Get the meta-data object from the given type */
	static FUPyWrapperBaseMetaData* GetMetaData(PyTypeObject* PyType);

	/** Get the meta-data object from the type of the given instance */
	static FUPyWrapperBaseMetaData* GetMetaData(FUPyWrapperBase* Instance);

	FUPyWrapperBaseMetaData()
	{
	}

	virtual ~FUPyWrapperBaseMetaData()
	{
	}

	/** Get the ID associated with this meta-data type */
	virtual FGuid GetTypeId() const = 0;

	/** Get the reflection meta data type object associated with this wrapper type if there is one or nullptr if not. */
	virtual const UField* GetMetaType() const
	{
		return nullptr;
	}

	/** Add object references from this type meta-data to the given collector */
	virtual void AddTypeReferencedObjects(FReferenceCollector& Collector)
	{
	}

	/** Add object references from the given Python object to the given collector */
	virtual void AddInstanceReferencedObjects(FUPyWrapperBase* Instance, FReferenceCollector& Collector)
	{
	}
};

typedef TUPyPtr<FUPyWrapperBase> FUPyWrapperBasePtr;

UINTERFACE(MinimalApi)
class UUPythonResourceOwner : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class IUPythonResourceOwner
{
	GENERATED_IINTERFACE_BODY()

public:
	/**
	 * Release all Python resources owned by this object.
	 * Called during Python shutdown to free resources from lingering UObject-based instances.
	 */
	virtual void ReleasePythonResources() = 0;
};
