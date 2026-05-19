// Copyright Epic Games, Inc. All Rights Reserved.

#include "Wrapper/UPyWrapperTypeFactory.h"
#include "Core/UPyGIL.h"
#include "Wrapper/UPyWrapperObjectBase.h"
#include "Wrapper/UPyWrapperTypeRegistry.h"
#include "Wrapper/UPyWrapperStruct.h"
#include "Wrapper/UPyWrapperFieldPath.h"
#include "Wrapper/UPyWrapperOwnerContext.h"
#include "Wrapper/UPyWrapperArray.h"
#include "Wrapper/UPyWrapperFixedArray.h"
#include "Wrapper/UPyWrapperSet.h"
#include "Wrapper/UPyWrapperMap.h"
#include "Wrapper/UPyWrapperDelegate.h"
#include "UObject/UObjectGlobals.h"

#if WITH_EDITOR
void FUPyWrapperObjectFactory::OnObjectsReplaced(const TMap<UObject*, UObject*>& ReplacementMap)
{
	for (auto& Pair : ReplacementMap)
	{
		UObject* OldObject = Pair.Key;
		UObject* NewObject = Pair.Value;

		if (NewObject == nullptr || NewObject == OldObject || MappedInstances.Contains(NewObject))
		{
			continue;
		}

		if (FUPyWrapperObjectBase** WrapperPtr = MappedInstances.Find(OldObject))
		{
			FUPyWrapperObjectBase* Wrapper = *WrapperPtr;
			const PyTypeObject* OldExpectedType = FUPyWrapperTypeRegistry::Get().GetWrappedClassType(OldObject->GetClass());
			const PyTypeObject* NewExpectedType = FUPyWrapperTypeRegistry::Get().GetWrappedClassType(NewObject->GetClass());
			if (OldExpectedType && NewExpectedType && OldExpectedType != NewExpectedType)
			{
				PyTypeObject* CurrentType = Py_TYPE((PyObject*)Wrapper);
				if (CurrentType == OldExpectedType && CurrentType->tp_basicsize == NewExpectedType->tp_basicsize)
				{
					FUPyScopedGIL Gil;
					Py_SET_TYPE((PyObject*)Wrapper, const_cast<PyTypeObject*>(NewExpectedType));
				}
			}

			Wrapper->ObjectInstance = NewObject;
			MappedInstances.Remove(OldObject);
			MappedInstances.Add(NewObject, Wrapper);
		}

		RemoveOwnedPyProps(OldObject);
	}
}
#endif

void FUPyWrapperObjectFactory::MapInstance(UObject* InUnrealInstance, FUPyWrapperObjectBase* InPythonInstance)
{
	Py_INCREF(InPythonInstance);

	TUPyWrapperTypeFactory::MapInstance(InUnrealInstance, InPythonInstance);
}

void FUPyWrapperObjectFactory::UnmapInstance(UObject* InUnrealInstance)
{
	FUPyWrapperObjectBase* PyObj = FindInstance(InUnrealInstance);
	if (PyObj)
	{
		Py_DECREF(PyObj);
		TUPyWrapperTypeFactory::UnmapInstance(InUnrealInstance);
	}
}

FUPyWrapperObjectFactory& FUPyWrapperObjectFactory::Get()
{
	static FUPyWrapperObjectFactory Instance;
	return Instance;
}

FUPyWrapperObjectBase* FUPyWrapperObjectFactory::CreateInstance(UObject* InUnrealInstance)
{
	if (!InUnrealInstance)
	{
		return nullptr;
	}

	const PyTypeObject* PyType = FUPyWrapperTypeRegistry::Get().GetWrappedClassType(InUnrealInstance->GetClass());
	return CreateInstanceInternal(InUnrealInstance, PyType, [InUnrealInstance](FUPyWrapperObjectBase* InSelf)
	{
		return FUPyWrapperObjectBase::Init(InSelf, InUnrealInstance);
	});
}

FUPyWrapperObjectBase* FUPyWrapperObjectFactory::CreateInstance(const PyTypeObject* InPyType, UObject* InUnrealInstance)
{
	if (!InUnrealInstance || !InPyType)
	{
		return nullptr;
	}

	return CreateInstanceInternal(InUnrealInstance, InPyType, [InUnrealInstance](FUPyWrapperObjectBase* InSelf)
	{
		return FUPyWrapperObjectBase::Init(InSelf, InUnrealInstance);
	});
}

FUPyWrapperObjectBase* FUPyWrapperObjectFactory::CreateInstance(UClass* InInterfaceClass, UObject* InUnrealInstance)
{
	if (!InInterfaceClass || !InUnrealInstance)
	{
		return nullptr;
	}

	const PyTypeObject* PyType = FUPyWrapperTypeRegistry::Get().GetWrappedClassType(InInterfaceClass);
	return CreateInstanceInternal(InUnrealInstance, PyType, [InUnrealInstance](FUPyWrapperObjectBase* InSelf)
	{
		return FUPyWrapperObjectBase::Init(InSelf, InUnrealInstance);
	});
}

void FUPyWrapperObjectFactory::AddOwnedPyProp(UObject* InUnrealInstance, FUPyWrapperBase* PyProp)
{
	if (!ContainsInstance(InUnrealInstance))
	{
		return;
	}

	auto& PyProps = MappedOwnedPyProps.FindOrAdd(InUnrealInstance);
	if (PyProps.Contains(PyProp))
	{
		return;
	}

	Py_INCREF(PyProp);
	PyProps.Add(PyProp);
}

void FUPyWrapperObjectFactory::RemoveOwnedPyProps(const UObject* InUnrealInstance)
{
	FUPyScopedGIL Gil;
	if (auto* PyProps = MappedOwnedPyProps.Find(InUnrealInstance))
	{
		for (auto& PyProp : *PyProps)
		{
			Py_XDECREF(PyProp);
			// if (PyObject_TypeCheck(PyProp, &UPyWrapperArrayType) == 1)
			// {
			// 	FUPyWrapperArray::Deinit(reinterpret_cast<FUPyWrapperArray*>(PyProp));
			// }
			// else if (PyObject_TypeCheck(PyProp, &UPyWrapperStructType) == 1)
			// {
			// 	FUPyWrapperStruct::Deinit(reinterpret_cast<FUPyWrapperStruct*>(PyProp));
			// }
		}

		MappedOwnedPyProps.Remove(InUnrealInstance);
	}
}

FUPyWrapperStructFactory& FUPyWrapperStructFactory::Get()
{
	static FUPyWrapperStructFactory Instance;
	return Instance;
}

FUPyWrapperStruct* FUPyWrapperStructFactory::CreateInstance(const UScriptStruct* InStruct, void* InUnrealInstance, const FUPyWrapperOwnerContext& InOwnerContext, const EUPyConversionMethod InConversionMethod)
{
	if (!InStruct || !InUnrealInstance)
	{
		return nullptr;
	}

	const PyTypeObject* PyType = FUPyWrapperTypeRegistry::Get().GetWrappedStructType(InStruct);
	return CreateInstanceInternal(InUnrealInstance, PyType, [InStruct, InUnrealInstance, &InOwnerContext, InConversionMethod](FUPyWrapperStruct* InSelf)
	{
		return FUPyWrapperStruct::Init(InSelf, InOwnerContext, InStruct, InUnrealInstance, InConversionMethod);
	}, InConversionMethod == EUPyConversionMethod::Copy || InConversionMethod == EUPyConversionMethod::Steal);
}

FUPyWrapperStruct* FUPyWrapperStructFactory::CreateInstance(const PyTypeObject* InPyType, const UScriptStruct* InStruct, void* InUnrealInstance, const FUPyWrapperOwnerContext& InOwnerContext, const EUPyConversionMethod InConversionMethod)
{
	if (!InPyType || !InUnrealInstance || !InStruct)
	{
		return nullptr;
	}
	return CreateInstanceInternal(InUnrealInstance, InPyType, [InStruct, InUnrealInstance, &InOwnerContext, InConversionMethod](FUPyWrapperStruct* InSelf)
	{
		return FUPyWrapperStruct::Init(InSelf, InOwnerContext, InStruct, InUnrealInstance, InConversionMethod);
	}, InConversionMethod == EUPyConversionMethod::Copy || InConversionMethod == EUPyConversionMethod::Steal);
}

// template <typename PyStructType>
// PyStructType* FUPyWrapperStructFactory::CreateInstance(void* InUnrealInstance, const FUPyWrapperOwnerContext& InOwnerContext, const EUPyConversionMethod InConversionMethod)
// {
// 	if (!InUnrealInstance)
// 	{
// 		return nullptr;
// 	}
//
// 	const PyTypeObject* PyType = PyStructType::GetPyType();
//
// 	PyStructType* PyInstance = (PyStructType*)CreateInstanceInternal(InUnrealInstance, PyType, InConversionMethod == EUPyConversionMethod::Copy || InConversionMethod == EUPyConversionMethod::Steal);
// 	if (PyInstance)
// 	{
// 		PyStructType::Init(PyInstance, InUnrealInstance);
// 	}
// 	return PyInstance;
// }

FUPyWrapperDelegateFactory& FUPyWrapperDelegateFactory::Get()
{
	static FUPyWrapperDelegateFactory Instance;
	return Instance;
}

FUPyWrapperDelegate* FUPyWrapperDelegateFactory::CreateInstance(const UFunction* InDelegateSignature, FScriptDelegate* InUnrealInstance, const FDelegateProperty* InProp, const FUPyWrapperOwnerContext& InOwnerContext, const EUPyConversionMethod InConversionMethod)
{
	if (!InDelegateSignature || !InUnrealInstance)
	{
		return nullptr;
	}

	const PyTypeObject* PyType = FUPyWrapperTypeRegistry::Get().GetWrappedDelegateType(InDelegateSignature);
	FUPyWrapperDelegate* Result = CreateInstanceInternal(InUnrealInstance, PyType, [InUnrealInstance, &InOwnerContext, InConversionMethod, InProp](FUPyWrapperDelegate* InSelf)
	{
		return FUPyWrapperDelegate::Init(InSelf, InOwnerContext, InUnrealInstance, InProp, InConversionMethod);
	}, InConversionMethod == EUPyConversionMethod::Copy || InConversionMethod == EUPyConversionMethod::Steal);

	if (Result && PyErr_Occurred())
	{
		PyErr_Clear();
	}

	return Result;
}


FUPyWrapperMulticastDelegateFactory& FUPyWrapperMulticastDelegateFactory::Get()
{
	static FUPyWrapperMulticastDelegateFactory Instance;
	return Instance;
}

FUPyWrapperMulticastDelegate* FUPyWrapperMulticastDelegateFactory::CreateInstance(const UFunction* InDelegateSignature, FMulticastScriptDelegate* InUnrealInstance, void* InPropAddr, const FMulticastDelegateProperty* InProp, const FUPyWrapperOwnerContext& InOwnerContext, const EUPyConversionMethod InConversionMethod)
{
	if (!InDelegateSignature)
	{
		return nullptr;
	}

	const PyTypeObject* PyType = FUPyWrapperTypeRegistry::Get().GetWrappedDelegateType(InDelegateSignature);
	FUPyWrapperMulticastDelegate* Result = CreateInstanceInternal(InUnrealInstance, PyType, [InUnrealInstance, InPropAddr, &InOwnerContext, InConversionMethod, InProp](FUPyWrapperMulticastDelegate* InSelf)
	{
		return FUPyWrapperMulticastDelegate::Init(InSelf, InOwnerContext, InUnrealInstance, InPropAddr, InProp, InConversionMethod);
	}, InConversionMethod == EUPyConversionMethod::Copy || InConversionMethod == EUPyConversionMethod::Steal);

	if (Result && PyErr_Occurred())
	{
		PyErr_Clear();
	}

	return Result;
}

FUPyWrapperArrayFactory& FUPyWrapperArrayFactory::Get()
{
	static FUPyWrapperArrayFactory Instance;
	return Instance;
}

FUPyWrapperArray* FUPyWrapperArrayFactory::CreateInstance(void* InUnrealInstance, const FArrayProperty* InProp, const FUPyWrapperOwnerContext& InOwnerContext, const EUPyConversionMethod InConversionMethod)
{
	if (!InUnrealInstance)
	{
		return nullptr;
	}

	return CreateInstanceInternal(InUnrealInstance, &UPyWrapperArrayType, [InUnrealInstance, InProp, &InOwnerContext, InConversionMethod](FUPyWrapperArray* InSelf)
	{
		return FUPyWrapperArray::Init(InSelf, InOwnerContext, InProp, InUnrealInstance, InConversionMethod);
	}, InConversionMethod == EUPyConversionMethod::Copy || InConversionMethod == EUPyConversionMethod::Steal);
}

FUPyWrapperFixedArrayFactory& FUPyWrapperFixedArrayFactory::Get()
{
	static FUPyWrapperFixedArrayFactory Instance;
	return Instance;
}

FUPyWrapperFixedArray* FUPyWrapperFixedArrayFactory::CreateInstance(void* InUnrealInstance, const FProperty* InProp, const FUPyWrapperOwnerContext& InOwnerContext, const EUPyConversionMethod InConversionMethod)
{
	if (!InUnrealInstance)
	{
		return nullptr;
	}

	return CreateInstanceInternal(InUnrealInstance, &UPyWrapperFixedArrayType, [InUnrealInstance, InProp, &InOwnerContext, InConversionMethod](FUPyWrapperFixedArray* InSelf)
	{
		return FUPyWrapperFixedArray::Init(InSelf, InOwnerContext, InProp, InUnrealInstance, InConversionMethod);
	}, InConversionMethod == EUPyConversionMethod::Copy || InConversionMethod == EUPyConversionMethod::Steal);
}

FUPyWrapperSetFactory& FUPyWrapperSetFactory::Get()
{
	static FUPyWrapperSetFactory Instance;
	return Instance;
}

FUPyWrapperSet* FUPyWrapperSetFactory::CreateInstance(void* InUnrealInstance, const FSetProperty* InProp, const FUPyWrapperOwnerContext& InOwnerContext, const EUPyConversionMethod InConversionMethod)
{
	if (!InUnrealInstance)
	{
		return nullptr;
	}

	return CreateInstanceInternal(InUnrealInstance, &UPyWrapperSetType, [InUnrealInstance, InProp, &InOwnerContext, InConversionMethod](FUPyWrapperSet* InSelf)
	{
		return FUPyWrapperSet::Init(InSelf, InOwnerContext, InProp, InUnrealInstance, InConversionMethod);
	}, InConversionMethod == EUPyConversionMethod::Copy || InConversionMethod == EUPyConversionMethod::Steal);
}

FUPyWrapperMapFactory& FUPyWrapperMapFactory::Get()
{
	static FUPyWrapperMapFactory Instance;
	return Instance;
}

FUPyWrapperMap* FUPyWrapperMapFactory::CreateInstance(void* InUnrealInstance, const FMapProperty* InProp, const FUPyWrapperOwnerContext& InOwnerContext, const EUPyConversionMethod InConversionMethod)
{
	if (!InUnrealInstance)
	{
		return nullptr;
	}

	return CreateInstanceInternal(InUnrealInstance, &UPyWrapperMapType, [InUnrealInstance, InProp, &InOwnerContext, InConversionMethod](FUPyWrapperMap* InSelf)
	{
		return FUPyWrapperMap::Init(InSelf, InOwnerContext, InProp, InUnrealInstance, InConversionMethod);
	}, InConversionMethod == EUPyConversionMethod::Copy || InConversionMethod == EUPyConversionMethod::Steal);
}

FUPyWrapperFieldPathFactory& FUPyWrapperFieldPathFactory::Get()
{
	static FUPyWrapperFieldPathFactory Instance;
	return Instance;
}

FUPyWrapperFieldPath* FUPyWrapperFieldPathFactory::CreateInstance(FFieldPath InUnrealInstance)
{
	// 默认都创建新的
	return CreateInstanceInternal(InUnrealInstance, &UPyWrapperFieldPathType, [InUnrealInstance](FUPyWrapperFieldPath* InSelf)
	{
		return FUPyWrapperFieldPath::Init(InSelf, InUnrealInstance);
	}, true);
}
