#include "Wrapper/UPyReferenceCollector.h"

#include "Core/UPyGIL.h"
#include "Wrapper/UPyWrapperArray.h"
#include "Wrapper/UPyWrapperDelegate.h"
#include "Wrapper/UPyWrapperFixedArray.h"
#include "Wrapper/UPyWrapperMap.h"
#include "Wrapper/UPyWrapperSet.h"
#include "Wrapper/UPyWrapperStruct.h"
#include "UObject/PropertyOptional.h"
#include "UObject/UnrealType.h"

FUPyReferenceCollector& FUPyReferenceCollector::Get()
{
	static FUPyReferenceCollector Instance;
	return Instance;
}

void FUPyReferenceCollector::AddWrappedInstance(FUPyWrapperBase* InInstance)
{
	PythonWrappedInstances.Add(InInstance);
}

void FUPyReferenceCollector::RemoveWrappedInstance(FUPyWrapperBase* InInstance)
{
	PythonWrappedInstances.Remove(InInstance);
}

void FUPyReferenceCollector::Shutdown()
{
	PythonWrappedInstances.Reset();
}

void FUPyReferenceCollector::AddReferencedObjects(FReferenceCollector& InCollector)
{
	if (!Py_IsInitialized() || PythonWrappedInstances.IsEmpty())
	{
		return;
	}

	FUPyScopedGIL GIL;
	for (FUPyWrapperBase* WrappedInstance : PythonWrappedInstances)
	{
		PyObject* PyInstance = reinterpret_cast<PyObject*>(WrappedInstance);
		if (PyObject_TypeCheck(PyInstance, &UPyWrapperStructType))
		{
			FUPyWrapperStruct* Self = reinterpret_cast<FUPyWrapperStruct*>(WrappedInstance);
			TObjectPtr<const UScriptStruct> ScriptStruct = Self->ScriptStruct;
			InCollector.AddReferencedObject(ScriptStruct);
			Self->ScriptStruct = ScriptStruct;
			if (Self->ScriptStruct && Self->StructInstance && !Self->OwnerContext.HasOwner())
			{
				AddReferencedObjectsFromStruct(InCollector, Self->ScriptStruct, Self->StructInstance);
			}
		}
		else if (PyObject_TypeCheck(PyInstance, &UPyWrapperArrayType))
		{
			FUPyWrapperArray* Self = reinterpret_cast<FUPyWrapperArray*>(WrappedInstance);
			if (Self->ArrayProp && Self->ArrayInstance && !Self->OwnerContext.HasOwner())
			{
				Self->ArrayProp.AddReferencedObjects(InCollector);
				AddReferencedObjectsFromProperty(InCollector, Self->ArrayProp.Get(), Self->ArrayInstance);
			}
		}
		else if (PyObject_TypeCheck(PyInstance, &UPyWrapperFixedArrayType))
		{
			FUPyWrapperFixedArray* Self = reinterpret_cast<FUPyWrapperFixedArray*>(WrappedInstance);
			if (Self->ArrayProp && Self->ArrayInstance && !Self->OwnerContext.HasOwner())
			{
				Self->ArrayProp.AddReferencedObjects(InCollector);
				AddReferencedObjectsFromProperty(InCollector, Self->ArrayProp.Get(), Self->ArrayInstance);
			}
		}
		else if (PyObject_TypeCheck(PyInstance, &UPyWrapperSetType))
		{
			FUPyWrapperSet* Self = reinterpret_cast<FUPyWrapperSet*>(WrappedInstance);
			if (Self->SetProp && Self->SetInstance && !Self->OwnerContext.HasOwner())
			{
				Self->SetProp.AddReferencedObjects(InCollector);
				AddReferencedObjectsFromProperty(InCollector, Self->SetProp.Get(), Self->SetInstance);
			}
		}
		else if (PyObject_TypeCheck(PyInstance, &UPyWrapperMapType))
		{
			FUPyWrapperMap* Self = reinterpret_cast<FUPyWrapperMap*>(WrappedInstance);
			if (Self->MapProp && Self->MapInstance && !Self->OwnerContext.HasOwner())
			{
				Self->MapProp.AddReferencedObjects(InCollector);
				AddReferencedObjectsFromProperty(InCollector, Self->MapProp.Get(), Self->MapInstance);
			}
		}
	}
}

FString FUPyReferenceCollector::GetReferencerName() const
{
	return TEXT("FUPyReferenceCollector");
}

void FUPyReferenceCollector::AddReferencedObjectsFromStruct(FReferenceCollector& InCollector, const UStruct* InStruct, void* InStructAddr)
{
	bool bUnused = false;
	AddReferencedObjectsFromStructInternal(InCollector, InStruct, InStructAddr, bUnused);
}

void FUPyReferenceCollector::AddReferencedObjectsFromProperty(FReferenceCollector& InCollector, const FProperty* InProp, void* InBaseAddr)
{
	bool bUnused = false;
	AddReferencedObjectsFromPropertyInternal(InCollector, InProp, InBaseAddr, bUnused);
}

void FUPyReferenceCollector::AddReferencedObjectsFromStructInternal(FReferenceCollector& InCollector, const UStruct* InStruct, void* InStructAddr, bool& OutValueChanged)
{
	for (TFieldIterator<const FProperty> PropIt(InStruct); PropIt; ++PropIt)
	{
		AddReferencedObjectsFromPropertyInternal(InCollector, *PropIt, InStructAddr, OutValueChanged);
	}
}

void FUPyReferenceCollector::AddReferencedObjectsFromPropertyInternal(FReferenceCollector& InCollector, const FProperty* InProp, void* InBaseAddr, bool& OutValueChanged)
{
	if (const FObjectProperty* ObjectProp = CastField<FObjectProperty>(InProp))
	{
		for (int32 ArrayIndex = 0; ArrayIndex < InProp->ArrayDim; ++ArrayIndex)
		{
			void* ValuePtr = ObjectProp->ContainerPtrToValuePtr<void>(InBaseAddr, ArrayIndex);
			UObject* CurrentValue = ObjectProp->GetObjectPropertyValue(ValuePtr);
			if (CurrentValue)
			{
				TObjectPtr<UObject> NewValue = CurrentValue;
				InCollector.AddReferencedObject(NewValue);
				if (NewValue != CurrentValue)
				{
					OutValueChanged = true;
					ObjectProp->SetObjectPropertyValue(ValuePtr, NewValue);
				}
			}
		}
		return;
	}

	if (const FInterfaceProperty* InterfaceProp = CastField<FInterfaceProperty>(InProp))
	{
		for (int32 ArrayIndex = 0; ArrayIndex < InProp->ArrayDim; ++ArrayIndex)
		{
			void* ValuePtr = InterfaceProp->ContainerPtrToValuePtr<void>(InBaseAddr, ArrayIndex);
			UObject* CurrentValue = InterfaceProp->GetPropertyValue(ValuePtr).GetObject();
			if (CurrentValue)
			{
				TObjectPtr<UObject> NewValue = CurrentValue;
				InCollector.AddReferencedObject(NewValue);
				if (NewValue != CurrentValue)
				{
					OutValueChanged = true;
					InterfaceProp->SetPropertyValue(ValuePtr, FScriptInterface(NewValue, NewValue ? NewValue->GetInterfaceAddress(InterfaceProp->InterfaceClass) : nullptr));
				}
			}
		}
		return;
	}

	if (const FStructProperty* StructProp = CastField<FStructProperty>(InProp))
	{
		for (int32 ArrayIndex = 0; ArrayIndex < InProp->ArrayDim; ++ArrayIndex)
		{
			AddReferencedObjectsFromStructInternal(InCollector, StructProp->Struct, StructProp->ContainerPtrToValuePtr<void>(InBaseAddr, ArrayIndex), OutValueChanged);
		}
		return;
	}

	if (const FDelegateProperty* DelegateProp = CastField<FDelegateProperty>(InProp))
	{
		for (int32 ArrayIndex = 0; ArrayIndex < InProp->ArrayDim; ++ArrayIndex)
		{
			FScriptDelegate* Value = DelegateProp->GetPropertyValuePtr(DelegateProp->ContainerPtrToValuePtr<void>(InBaseAddr, ArrayIndex));
			if (UUPyCallableForDelegate* Callable = Cast<UUPyCallableForDelegate>(Value->GetUObject()))
			{
				TObjectPtr<UUPyCallableForDelegate> CallablePtr = Callable;
				InCollector.AddReferencedObject(CallablePtr);
			}
		}
		return;
	}

	if (const FMulticastDelegateProperty* MulticastProp = CastField<FMulticastDelegateProperty>(InProp))
	{
		for (int32 ArrayIndex = 0; ArrayIndex < InProp->ArrayDim; ++ArrayIndex)
		{
			if (const FMulticastScriptDelegate* Value = MulticastProp->GetMulticastDelegate(MulticastProp->ContainerPtrToValuePtr<void>(InBaseAddr, ArrayIndex)))
			{
				for (UObject* DelegateObject : Value->GetAllObjectsEvenIfUnreachable())
				{
					if (UUPyCallableForDelegate* Callable = Cast<UUPyCallableForDelegate>(DelegateObject))
					{
						TObjectPtr<UUPyCallableForDelegate> CallablePtr = Callable;
						InCollector.AddReferencedObject(CallablePtr);
					}
				}
			}
		}
		return;
	}

	if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(InProp))
	{
		for (int32 ArrayIndex = 0; ArrayIndex < InProp->ArrayDim; ++ArrayIndex)
		{
			FScriptArrayHelper_InContainer ArrayHelper(ArrayProp, InBaseAddr, ArrayIndex);
			for (int32 ElementIndex = 0; ElementIndex < ArrayHelper.Num(); ++ElementIndex)
			{
				AddReferencedObjectsFromPropertyInternal(InCollector, ArrayProp->Inner, ArrayHelper.GetRawPtr(ElementIndex), OutValueChanged);
			}
		}
		return;
	}

	if (const FSetProperty* SetProp = CastField<FSetProperty>(InProp))
	{
		for (int32 ArrayIndex = 0; ArrayIndex < InProp->ArrayDim; ++ArrayIndex)
		{
			bool bSetValuesChanged = false;
			FScriptSetHelper_InContainer SetHelper(SetProp, InBaseAddr, ArrayIndex);
			for (FScriptSetHelper::FIterator It(SetHelper); It; ++It)
			{
				AddReferencedObjectsFromPropertyInternal(InCollector, SetHelper.GetElementProperty(), SetHelper.GetElementPtr(It), bSetValuesChanged);
			}
			if (bSetValuesChanged)
			{
				OutValueChanged = true;
				SetHelper.Rehash();
			}
		}
		return;
	}

	if (const FMapProperty* MapProp = CastField<FMapProperty>(InProp))
	{
		for (int32 ArrayIndex = 0; ArrayIndex < InProp->ArrayDim; ++ArrayIndex)
		{
			bool bMapKeysChanged = false;
			bool bMapValuesChanged = false;
			FScriptMapHelper_InContainer MapHelper(MapProp, InBaseAddr, ArrayIndex);
			for (FScriptMapHelper::FIterator It(MapHelper); It; ++It)
			{
				AddReferencedObjectsFromPropertyInternal(InCollector, MapHelper.GetKeyProperty(), MapHelper.GetPairPtr(It), bMapKeysChanged);
				AddReferencedObjectsFromPropertyInternal(InCollector, MapHelper.GetValueProperty(), MapHelper.GetPairPtr(It), bMapValuesChanged);
			}
			if (bMapKeysChanged || bMapValuesChanged)
			{
				OutValueChanged = true;
				if (bMapKeysChanged)
				{
					MapHelper.Rehash();
				}
			}
		}
		return;
	}

	if (const FOptionalProperty* OptionalProp = CastField<FOptionalProperty>(InProp))
	{
		for (int32 ArrayIndex = 0; ArrayIndex < InProp->ArrayDim; ++ArrayIndex)
		{
			void* ValuePtr = OptionalProp->ContainerPtrToValuePtr<void>(InBaseAddr, ArrayIndex);
			if (OptionalProp->IsSet(ValuePtr))
			{
				AddReferencedObjectsFromPropertyInternal(InCollector, OptionalProp->GetValueProperty(), OptionalProp->GetValuePointerForReadOrReplace(ValuePtr), OutValueChanged);
			}
		}
	}
}
