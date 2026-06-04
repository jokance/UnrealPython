
#include "Subclassing/UPyGeneratedStruct.h"
#include "Core/UPyGIL.h"
#include "Core/UPyCore.h"
#include "Wrapper/UPyWrapperTypeRegistry.h"
#include "Wrapper/UPyWrapperTypeFactory.h"

PyTypeObject UPyUStructDecoratorType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"UPyUStructDecorator", /* tp_name */
	sizeof(FUPyUStructDecorator), /* tp_basicsize */
};

void InitializeUPyUStructDecorator(UPyGenUtil::FNativePythonModule& ModuleInfo)
{
	PyTypeObject* PyType = &UPyUStructDecoratorType;
	PyType->tp_flags = Py_TPFLAGS_DEFAULT;
	PyType->tp_new = (newfunc)FUPyUStructDecorator::New;
	PyType->tp_dealloc = (destructor)FUPyUStructDecorator::Dealloc;
	PyType->tp_init = (initproc)FUPyUStructDecorator::Init;
	PyType->tp_call = (ternaryfunc)FUPyUStructDecorator::Call;
	
	if (PyType_Ready(PyType) == 0)
	{
		ModuleInfo.AddType(PyType);
	}
}

PyObject* PyCallGenerateStruct(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	PyObject* PyObj = nullptr;
	if (!PyArg_ParseTuple(InArgs, "O:GenerateStruct ", &PyObj))
	{
		return nullptr;
	}
	check(PyObj);

	if (!PyType_Check(PyObj))
	{
		UPyUtil::SetPythonError(PyExc_TypeError, TEXT("GenerateStruct "), *FString::Printf(TEXT("Parameter must be a 'type' not '%s'"), *UPyUtil::GetFriendlyTypename(PyObj)));
		return nullptr;
	}

	PyTypeObject* PyType = (PyTypeObject*)PyObj;
	if (!PyType_IsSubtype(PyType, &UPyWrapperStructType))
	{
		UPyUtil::SetPythonError(PyExc_Exception, TEXT("GenerateStruct "), *FString::Printf(TEXT("Type '%s' does not derive from an Unreal struct type"), *UPyUtil::GetFriendlyTypename(PyType)));
		return nullptr;
	}

	// We only need to generate structs for types that are not already registered.
	const bool bNeedsGeneration = !FUPyWrapperTypeRegistry::Get().FindStruct(PyType);
	if (bNeedsGeneration && !UUPyGeneratedStruct::GenerateStruct(PyType))
	{
		UPyUtil::SetPythonError(PyExc_Exception, TEXT("GenerateStruct "), *FString::Printf(TEXT("Failed to generate an Unreal struct for the Python type '%s'"), *UPyUtil::GetFriendlyTypename(PyType)));
		return nullptr;
	}
	if (bNeedsGeneration)
	{
		Py_BEGIN_ALLOW_THREADS
		FUPyWrapperTypeReinstancer::Get().ProcessPending();
		Py_END_ALLOW_THREADS
	}

	Py_INCREF(PyType);
	return (PyObject*)PyType;
}

class FUPythonGeneratedStructBuilder
{
public:
	FUPythonGeneratedStructBuilder(UScriptStruct* InSuperStruct, PyTypeObject* InPyType)
		: StructName()
		, PyType(InPyType)
		, OldStruct(nullptr)
		, NewStruct(nullptr)
	{
		UObject* StructOuter = nullptr;
		UPyUtil::GetGeneratedTypeOuterAndName(PyType, StructOuter, StructName);

		// Find any existing struct with the name we want to use
		OldStruct = FindObject<UUPyGeneratedStruct>(StructOuter, *StructName);

		// Create a new struct with a temporary name; we will rename it as part of Finalize
		const FString NewStructName = MakeUniqueObjectName(StructOuter, UUPyGeneratedStruct::StaticClass(), *FString::Printf(TEXT("%s_NEWINST"), *StructName)).ToString();
		NewStruct = NewObject<UUPyGeneratedStruct>(StructOuter, *NewStructName, RF_Public | RF_Transient);
#if WITH_METADATA
		NewStruct->SetMetaData(TEXT("DisplayName"), *UPyUtil::GetGeneratedTypeDisplayName(PyType));
		NewStruct->SetMetaData(TEXT("BlueprintType"), TEXT("true"));
#endif
		NewStruct->SetSuperStruct(InSuperStruct);
	}

	~FUPythonGeneratedStructBuilder()
	{
		// If NewStruct is still set at this point, if means Finalize wasn't called and we should destroy the partially built struct
		if (NewStruct)
		{
			NewStruct->PrepareCppStructOps();
			NewStruct->ClearFlags(RF_Public | RF_Standalone);
			NewStruct->MarkAsGarbage();
			NewStruct = nullptr;
		}
	}

	UUPyGeneratedStruct* Finalize(FUPyObjectPtr InPyPostInitFunction)
	{
		// Set the post-init function
		NewStruct->PyPostInitFunction = InPyPostInitFunction;

		// Replace the definitions with real descriptors
		if (!RegisterDescriptors())
		{
			return nullptr;
		}

		// Let Python know that we've changed its type
		PyType_Modified(PyType);

		// Build a complete list of init params for this struct
		TArray<UPyGenUtil::FGeneratedWrappedMethodParameter> StructInitParams;
		// if (const FPyWrapperStructMetaData* SuperMetaData = FPyWrapperStructMetaData::GetMetaData(PyType->tp_base))
		// {
		// 	StructInitParams = SuperMetaData->InitParams;
		// }
		for (const TSharedPtr<UPyGenUtil::FPropertyDef>& PropDef : NewStruct->PropertyDefs)
		{
			if (!PropDef->GeneratedWrappedGetSet.Prop.DeprecationMessage.IsSet())
			{
				UPyGenUtil::FGeneratedWrappedMethodParameter& StructInitParam = StructInitParams.AddDefaulted_GetRef();
				StructInitParam.ParamName = PropDef->GeneratedWrappedGetSet.GetSetName;
				StructInitParam.ParamProp = PropDef->GeneratedWrappedGetSet.Prop.Prop;
				StructInitParam.ParamDefaultValue = FString();
			}
		}

		// We can no longer fail, so prepare the old struct for removal and set the correct name on the new struct
		if (OldStruct)
		{
			PrepareOldStructForReinstancing();
		}

		NewStruct->StructRedirector = UPyUtil::CreatePythonTypeLegacyRedirector(UPyUtil::GetCleanTypename(PyType), FTopLevelAssetPath(NewStruct->GetOuter()->GetFName(), *StructName));
		if (NewStruct->StructRedirector)
		{
			NewStruct->StructRedirector->DestinationObject = NewStruct;
		}
		NewStruct->Rename(*StructName, nullptr, REN_DontCreateRedirectors);

		// Finalize the struct
		NewStruct->Bind();
		NewStruct->StaticLink(true);

		// Add the object meta-data to the type
		// todo(hzn): meta data
		// NewStruct->PyMetaData.Struct = NewStruct;
		// NewStruct->PyMetaData.InitParams = MoveTemp(StructInitParams);
		// FPyWrapperStructMetaData::SetMetaData(PyType, &NewStruct->PyMetaData);

		// Map the Unreal struct to the Python type
		NewStruct->PyType = FUPyTypeObjectPtr::NewReference(PyType);
		FUPyWrapperTypeRegistry::Get().RegisterWrappedStructType(NewStruct, PyType);

		// Re-instance the old struct
		if (OldStruct)
		{
			FUPyWrapperTypeReinstancer::Get().AddPendingStruct(OldStruct, NewStruct);
		}

		// Null the NewStruct pointer so the destructor doesn't kill it
		UUPyGeneratedStruct* FinalizedStruct = NewStruct;
		NewStruct = nullptr;
		return FinalizedStruct;
	}

	bool CreatePropertyFromDefinition(const FString& InFieldName, FUPyFPropertyDef* InPyPropDef)
	{
		UScriptStruct* SuperStruct = Cast<UScriptStruct>(NewStruct->GetSuperStruct());

		// Resolve the property name to match any previously exported properties from the parent type
		const FName PropName = *InFieldName; // FPyWrapperStructMetaData::ResolvePropertyName(PyType->tp_base, *InFieldName);
		if (SuperStruct && SuperStruct->FindPropertyByName(PropName))
		{
			UPyUtil::SetPythonError(PyExc_Exception, PyType, *FString::Printf(TEXT("Property '%s' (%s) cannot override a property from the base type"), *InFieldName, *UPyUtil::GetFriendlyTypename(InPyPropDef->PropType)));
			return false;
		}

		// Structs cannot support getter/setter functions (or any functions)
		if (!InPyPropDef->GetterFuncName.IsEmpty() || !InPyPropDef->SetterFuncName.IsEmpty())
		{
			UPyUtil::SetPythonError(PyExc_Exception, PyType, *FString::Printf(TEXT("Struct property '%s' (%s) has a getter or setter, which is not supported on structs"), *InFieldName, *UPyUtil::GetFriendlyTypename(InPyPropDef->PropType)));
			return false;
		}
		if (InPyPropDef->bReplicated || InPyPropDef->bRepNotify)
		{
			UPyUtil::SetPythonError(PyExc_Exception, PyType, *FString::Printf(TEXT("Struct property '%s' (%s) cannot use 'Replicated' or 'RepNotify'; network replication is only supported on generated classes"), *InFieldName, *UPyUtil::GetFriendlyTypename(InPyPropDef->PropType)));
			return false;
		}

		// Create the property from its definition
		FProperty* Prop = UPyUtil::CreateProperty(InPyPropDef->PropType, 1, NewStruct, PropName);
		if (!Prop)
		{
			UPyUtil::SetPythonError(PyExc_Exception, PyType, *FString::Printf(TEXT("Failed to create property for '%s' (%s)"), *InFieldName, *UPyUtil::GetFriendlyTypename(InPyPropDef->PropType)));
			return false;
		}
		Prop->PropertyFlags |= (CPF_Edit | CPF_BlueprintVisible);
		FUPyFPropertyDef::ApplyMetaData(InPyPropDef, Prop);
		NewStruct->AddCppProperty(Prop);

		// Build the definition data for the new property accessor
		UPyGenUtil::FPropertyDef& PropDef = *NewStruct->PropertyDefs.Add_GetRef(MakeShared<UPyGenUtil::FPropertyDef>());
		PropDef.GeneratedWrappedGetSet.GetSetName = UPyGenUtil::TCHARToUTF8Buffer(*InFieldName);
		PropDef.GeneratedWrappedGetSet.GetSetDoc = UPyGenUtil::TCHARToUTF8Buffer(*FString::Printf(TEXT("type: %s\n%s"), *UPyGenUtil::GetPropertyPythonType(Prop), *UPyGenUtil::GetFieldTooltip(Prop)));
		PropDef.GeneratedWrappedGetSet.Prop.SetProperty(Prop);
		PropDef.GeneratedWrappedGetSet.GetCallback = (getter)&FUPyWrapperStruct::Getter_Impl;
		PropDef.GeneratedWrappedGetSet.SetCallback = (setter)&FUPyWrapperStruct::Setter_Impl;
		PropDef.GeneratedWrappedGetSet.ToPython(PropDef.PyGetSet);

		return true;
	}

private:
	bool RegisterDescriptors()
	{
		for (const TSharedPtr<UPyGenUtil::FPropertyDef>& PropDef : NewStruct->PropertyDefs)
		{
			FUPyObjectPtr GetSetDesc = FUPyObjectPtr::StealReference(PyDescr_NewGetSet(PyType, &PropDef->PyGetSet));
			if (!GetSetDesc)
			{
				UPyUtil::SetPythonError(PyExc_Exception, PyType, *FString::Printf(TEXT("Failed to create descriptor for '%s'"), UTF8_TO_TCHAR(PropDef->PyGetSet.name)));
				return false;
			}
			if (PyDict_SetItemString(PyType->tp_dict, PropDef->PyGetSet.name, GetSetDesc) != 0)
			{
				UPyUtil::SetPythonError(PyExc_Exception, PyType, *FString::Printf(TEXT("Failed to assign descriptor for '%s'"), UTF8_TO_TCHAR(PropDef->PyGetSet.name)));
				return false;
			}
		}

		return true;
	}

	void PrepareOldStructForReinstancing()
	{
		check(OldStruct);

		const FString OldStructName = MakeUniqueObjectName(OldStruct->GetOuter(), OldStruct->GetClass(), *FString::Printf(TEXT("%s_REINST"), *OldStruct->GetName())).ToString();
		OldStruct->SetFlags(RF_NewerVersionExists);
		OldStruct->ClearFlags(RF_Public | RF_Standalone);
		OldStruct->Rename(*OldStructName, nullptr, REN_DontCreateRedirectors);
		OldStruct->UnregisterGeneratedType();
	}

	FString StructName;
	PyTypeObject* PyType;
	UUPyGeneratedStruct* OldStruct;
	UUPyGeneratedStruct* NewStruct;
};


void UUPyGeneratedStruct::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	Super::AddReferencedObjects(InThis, Collector);

	UUPyGeneratedStruct* This = CastChecked<UUPyGeneratedStruct>(InThis);
	Collector.AddReferencedObject(This->StructRedirector, This);
}

void UUPyGeneratedStruct::PostRename(UObject* OldOuter, const FName OldName)
{
	Super::PostRename(OldOuter, OldName);

	if (PyType)
	{
		FUPyWrapperTypeRegistry::Get().UnregisterWrappedStructType(this);
		FUPyWrapperTypeRegistry::Get().RegisterWrappedStructType(this, PyType);
	}
}

void UUPyGeneratedStruct::InitializeStruct(void* Dest, int32 ArrayDim) const
{
	Super::InitializeStruct(Dest, ArrayDim);

	// Execute Python code within this block
	{
		FUPyScopedGIL GIL;

		if (PyPostInitFunction)
		{
			TGuardValue<bool> IsRunningUserScriptGuard(UE::UnrealPython::Private::GIsRunningUserScript, true);

			const int32 Stride = GetStructureSize();
			for (int32 ArrIndex = 0; ArrIndex < ArrayDim; ++ArrIndex)
			{
				void* StructInstance = static_cast<uint8*>(Dest) + (ArrIndex * Stride);
				FUPyObjectPtr PySelf = FUPyObjectPtr::StealReference((PyObject*)FUPyWrapperStructFactory::Get().CreateInstance((UUPyGeneratedStruct*)this, StructInstance, FUPyWrapperOwnerContext(Py_None), EUPyConversionMethod::Reference));
				if (PySelf && ensureAlwaysMsgf(PySelf->ob_type == PyType, TEXT("Struct instance (struct: %s) had an unexpected PyType when calling InitializeStruct! Self is '%s' but we expected '%s'"), *GetPathName(), *UPyUtil::GetFriendlyTypename(PySelf), *UPyUtil::GetFriendlyTypename(PyType)))
				{
					FUPyObjectPtr PyArgs = FUPyObjectPtr::StealReference(PyTuple_New(1));
					PyTuple_SetItem(PyArgs, 0, PySelf.Release()); // SetItem steals the reference

					FUPyObjectPtr Result = FUPyObjectPtr::StealReference(PyObject_CallObject((PyObject*)PyPostInitFunction.GetPtr(), PyArgs));
					if (!Result)
					{
						UPyUtil::ReThrowPythonError();
					}
				}
			}
		}
	}
}

void UUPyGeneratedStruct::BeginDestroy()
{
	ReleasePythonResources();
	Super::BeginDestroy();
}

void UUPyGeneratedStruct::ReleasePythonResources()
{
	// This may be called after Python has already shut down
	if (Py_IsInitialized())
	{
		FUPyScopedGIL GIL;
		UnregisterGeneratedType();
		PyType.Reset();
		PyPostInitFunction.Reset();
	}
	else
	{
		// Release ownership if Python has been shut down to avoid attempting to delete the objects (which are already dead)
		PyType.Release();
		PyPostInitFunction.Release();
	}

	PropertyDefs.Reset();
	// PyMetaData = FPyWrapperStructMetaData();
}

void UUPyGeneratedStruct::UnregisterGeneratedType()
{
	if (PyType)
	{
		FUPyWrapperTypeRegistry::Get().UnregisterWrappedStructType(this);
	}
}

UUPyGeneratedStruct* UUPyGeneratedStruct::GenerateStruct(PyTypeObject* InPyType)
{
	// Get the correct super struct from the parent type in Python
	const UScriptStruct* SuperStruct = nullptr;
	if (InPyType->tp_base != &UPyWrapperStructType)
	{
		SuperStruct = FUPyWrapperTypeRegistry::Get().FindStruct(InPyType->tp_base);
		if (!SuperStruct)
		{
			UPyUtil::SetPythonError(PyExc_Exception, InPyType, TEXT("No super struct could be found for this Python type"));
			return nullptr;
		}
	}

	// Builder used to generate the struct
	FUPythonGeneratedStructBuilder PythonStructBuilder((UScriptStruct*)SuperStruct, InPyType);

	// Add the fields to this struct
	{
		PyObject* FieldKey = nullptr;
		PyObject* FieldValue = nullptr;
		Py_ssize_t FieldIndex = 0;
		while (PyDict_Next(InPyType->tp_dict, &FieldIndex, &FieldKey, &FieldValue))
		{
			const FString FieldName = UPyUtil::PyObjectToUEString(FieldKey);

			if (PyObject_IsInstance(FieldValue, (PyObject*)&UPyUValueDefType) == 1)
			{
				// Values are not supported on structs
				UPyUtil::SetPythonError(PyExc_Exception, InPyType, TEXT("Structs do not support values"));
				return nullptr;
			}

			if (PyObject_IsInstance(FieldValue, (PyObject*)&UPyFPropertyDefType) == 1)
			{
				FUPyFPropertyDef* PyPropDef = (FUPyFPropertyDef*)FieldValue;
				if (!PythonStructBuilder.CreatePropertyFromDefinition(FieldName, PyPropDef))
				{
					return nullptr;
				}
			}

			if (PyObject_IsInstance(FieldValue, (PyObject*)&UPyUFunctionDefType) == 1)
			{
				// Functions are not supported on structs
				UPyUtil::SetPythonError(PyExc_Exception, InPyType, TEXT("Structs do not support methods"));
				return nullptr;
			}
		}
	}

	FUPyObjectPtr PyPostInitFunction = FUPyObjectPtr::StealReference(UPyGenUtil::GetPostInitFunc(InPyType));
	if (PyErr_Occurred())
	{
		return nullptr;
	}

	// Finalize the struct with its optional post-init function
	return PythonStructBuilder.Finalize(MoveTemp(PyPostInitFunction));
}

FUPyUStructDecorator* FUPyUStructDecorator::New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
{
	FUPyUStructDecorator* Self = (FUPyUStructDecorator*)InType->tp_alloc(InType, 0);
	return Self;
}

void FUPyUStructDecorator::Dealloc(FUPyUStructDecorator* InSelf)
{
	Deinit(InSelf);
	Py_TYPE(InSelf)->tp_free((PyObject*)InSelf);
}

int FUPyUStructDecorator::Init(FUPyUStructDecorator* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	Deinit(InSelf);
	return 0;
}

void FUPyUStructDecorator::Deinit(FUPyUStructDecorator* InSelf)
{
}

PyObject* FUPyUStructDecorator::Call(FUPyUStructDecorator* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	return PyCallGenerateStruct((PyObject*)InSelf, InArgs, nullptr);
}
