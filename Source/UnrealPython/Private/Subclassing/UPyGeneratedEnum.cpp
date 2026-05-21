
#include "Subclassing/UPyGeneratedEnum.h"
#include "Core/UPyCore.h"
#include "Wrapper/UPyWrapperTypeRegistry.h"
#include "Wrapper/UPyWrapperEnum.h"
#include "Core/UPyGIL.h"

PyTypeObject UPyUEnumDecoratorType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"UPyUEnumDecorator", /* tp_name */
	sizeof(FUPyUEnumDecorator), /* tp_basicsize */
};

void InitializeUPyUEnumDecorator(UPyGenUtil::FNativePythonModule& ModuleInfo)
{
	PyTypeObject* PyType = &UPyUEnumDecoratorType;
	PyType->tp_flags = Py_TPFLAGS_DEFAULT;
	PyType->tp_new = (newfunc)FUPyUEnumDecorator::New;
	PyType->tp_dealloc = (destructor)FUPyUEnumDecorator::Dealloc;
	PyType->tp_init = (initproc)FUPyUEnumDecorator::Init;
	PyType->tp_call = (ternaryfunc)FUPyUEnumDecorator::Call;
	
	if (PyType_Ready(PyType) == 0)
	{
		ModuleInfo.AddType(PyType);
	}
}

PyObject* PyCallGenerateEnum(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	PyObject* PyObj = nullptr;
	if (!PyArg_ParseTuple(InArgs, "O:GenerateEnum", &PyObj))
	{
		return nullptr;
	}
	check(PyObj);

	if (!PyType_Check(PyObj))
	{
		UPyUtil::SetPythonError(PyExc_TypeError, TEXT("GenerateEnum"), *FString::Printf(TEXT("Parameter must be a 'type' not '%s'"), *UPyUtil::GetFriendlyTypename(PyObj)));
		return nullptr;
	}

	PyTypeObject* PyType = (PyTypeObject*)PyObj;
	if (!PyType_IsSubtype(PyType, &UPyWrapperEnumType))
	{
		UPyUtil::SetPythonError(PyExc_Exception, TEXT("GenerateEnum"), *FString::Printf(TEXT("Type '%s' does not derive from the Unreal enum type"), *UPyUtil::GetFriendlyTypename(PyType)));
		return nullptr;
	}

	// We only need to generate enums for types without meta-data, as any types with meta-data have already been generated
	if (!FUPyWrapperTypeRegistry::Get().FindEnum(PyType) && !UUPyGeneratedEnum::GenerateEnum(PyType))
	{
		UPyUtil::SetPythonError(PyExc_Exception, TEXT("GenerateEnum"), *FString::Printf(TEXT("Failed to generate an Unreal enum for the Python type '%s'"), *UPyUtil::GetFriendlyTypename(PyType)));
		return nullptr;
	}

	Py_INCREF(PyType);
	return (PyObject*)PyType;
}


class FUPythonGeneratedEnumBuilder
{
public:
	FUPythonGeneratedEnumBuilder(PyTypeObject* InPyType)
		: EnumName()
		, PyType(InPyType)
		, NewEnum(nullptr)
	{
		UObject* EnumOuter = nullptr;
		UPyUtil::GetGeneratedTypeOuterAndName(PyType, EnumOuter, EnumName);

		UObjectRedirector* EnumRedirector = UPyUtil::CreatePythonTypeLegacyRedirector(UPyUtil::GetCleanTypename(InPyType), FTopLevelAssetPath(EnumOuter->GetFName(), *EnumName));

		// Enum instances are re-used if they already exist
		NewEnum = FindObject<UUPyGeneratedEnum>(EnumOuter, *EnumName);
		if (!NewEnum)
		{
			NewEnum = NewObject<UUPyGeneratedEnum>(EnumOuter, *EnumName, RF_Public | RF_Transient);
#if WITH_METADATA
			NewEnum->SetMetaData(TEXT("DisplayName"), *UPyUtil::GetGeneratedTypeDisplayName(PyType));
			NewEnum->SetMetaData(TEXT("BlueprintType"), TEXT("true"));
#endif
		}
		NewEnum->EnumValueDefs.Reset();

		NewEnum->EnumRedirector = EnumRedirector;
		if (NewEnum->EnumRedirector)
		{
			NewEnum->EnumRedirector->DestinationObject = NewEnum;
		}
	}

	~FUPythonGeneratedEnumBuilder()
	{
		// If NewEnum is still set at this point, if means Finalize wasn't called and we should destroy the partially built enum
		if (NewEnum)
		{
			NewEnum->ClearFlags(RF_Public | RF_Standalone);
			NewEnum = nullptr;

			Py_BEGIN_ALLOW_THREADS
			CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
			Py_END_ALLOW_THREADS
		}
	}

	UUPyGeneratedEnum* Finalize(const TArray<FUPyUValueDef*>& InPyValueDefs)
	{
		// Populate the enum with its values, and replace the definitions with real descriptors
		if (!RegisterDescriptors(InPyValueDefs))
		{
			return nullptr;
		}

		// Let Python know that we've changed its type
		PyType_Modified(PyType);

		// Finalize the enum
		NewEnum->Bind();

		// Add the object meta-data to the type
		// NewEnum->PyMetaData.Enum = NewEnum;
		// NewEnum->PyMetaData.bFinalized = true;
		// FPyWrapperEnumMetaData::SetMetaData(PyType, &NewEnum->PyMetaData);

		// Map the Unreal enum to the Python type
		NewEnum->PyType = FUPyTypeObjectPtr::NewReference(PyType);
		FUPyWrapperTypeRegistry::Get().RegisterWrappedEnumType(NewEnum, PyType);

		// Null the NewEnum pointer so the destructor doesn't kill it
		UUPyGeneratedEnum* FinalizedEnum = NewEnum;
		NewEnum = nullptr;
		return FinalizedEnum;
	}

	bool CreateValueFromDefinition(const FString& InFieldName, FUPyUValueDef* InPyValueDef)
	{
		int64 EnumValue = 0;
		if (!UPyConversion::Nativize(InPyValueDef->Value, EnumValue))
		{
			UPyUtil::SetPythonError(PyExc_TypeError, PyType, *FString::Printf(TEXT("Failed to convert enum value for '%s'"), *InFieldName));
			return false;
		}

		// Build the definition data for the new enum value
		UUPyGeneratedEnum::FEnumValueDef& EnumValueDef = *NewEnum->EnumValueDefs.Add_GetRef(MakeShared<UUPyGeneratedEnum::FEnumValueDef>());
		EnumValueDef.PyIndex = NewEnum->EnumValueDefs.Num() - 1;
		EnumValueDef.Value = EnumValue;
		EnumValueDef.Name = InFieldName;

		return true;
	}

private:
	bool RegisterDescriptors(const TArray<FUPyUValueDef*>& InPyValueDefs)
	{
		// The enum entries came from a Python dict, so sort them into a consistent order (using the value) before registering them
		NewEnum->EnumValueDefs.Sort([](const TSharedPtr<UUPyGeneratedEnum::FEnumValueDef>& EnumValueDefOne, const TSharedPtr<UUPyGeneratedEnum::FEnumValueDef>& EnumValueDefTwo)
		{
			return EnumValueDefOne->Value < EnumValueDefTwo->Value;
		});

		// Populate the enum with its values
		check(InPyValueDefs.Num() == NewEnum->EnumValueDefs.Num());
		{
			TArray<TPair<FName, int64>> ValueNames;
			for (const TSharedPtr<UUPyGeneratedEnum::FEnumValueDef>& EnumValueDef : NewEnum->EnumValueDefs)
			{
				const FString NamespacedValueName = FString::Printf(TEXT("%s::%s"), *EnumName, *EnumValueDef->Name);
				ValueNames.Emplace(MakeTuple(FName(*NamespacedValueName), EnumValueDef->Value));
			}
			if (!NewEnum->SetEnums(ValueNames, UEnum::ECppForm::Namespaced))
			{
				UPyUtil::SetPythonError(PyExc_Exception, PyType, TEXT("Failed to set enum values"));
				return false;
			}

			// Can't set the meta-data until SetEnums has been called
			// Note: Beware, InPyValueDefs is not guaranteed to be in the same order due to the sort above - index it via the PyIndex of the value definition!
#if WITH_EDITOR
			for (int32 EnumValueIndex = 0; EnumValueIndex < NewEnum->EnumValueDefs.Num(); ++EnumValueIndex)
			{
				TSharedPtr<UUPyGeneratedEnum::FEnumValueDef>& EnumValueDef = NewEnum->EnumValueDefs[EnumValueIndex];
				FUPyUValueDef::ApplyMetaData(InPyValueDefs[EnumValueDef->PyIndex], [this, EnumValueIndex](const FString& InMetaDataKey, const FString& InMetaDataValue)
				{
					NewEnum->SetMetaData(*InMetaDataKey, *InMetaDataValue, EnumValueIndex);
				});
				NewEnum->EnumValueDefs[EnumValueIndex]->DocString = UPyGenUtil::GetEnumEntryTooltip(NewEnum, EnumValueIndex);
			}
#endif
		}

		// Replace the definitions with real descriptors
		// todo(hzn): meta data
		for (const TSharedPtr<UUPyGeneratedEnum::FEnumValueDef>& EnumValueDef : NewEnum->EnumValueDefs)
		{
			FUPyWrapperEnum* PyEnumEntry = FUPyWrapperEnum::AddEnumEntry(PyType, EnumValueDef->Value, TCHAR_TO_UTF8(*EnumValueDef->Name), TCHAR_TO_UTF8(*EnumValueDef->DocString));
			// if (PyEnumEntry)
			// {
			// 	NewEnum->PyMetaData.EnumEntries.Add(PyEnumEntry);
			// }
		}

		return true;
	}

	FString EnumName;
	PyTypeObject* PyType;
	UUPyGeneratedEnum* NewEnum;
};

void UUPyGeneratedEnum::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	Super::AddReferencedObjects(InThis, Collector);

	UUPyGeneratedEnum* This = CastChecked<UUPyGeneratedEnum>(InThis);
	Collector.AddReferencedObject(This->EnumRedirector, This);
}

void UUPyGeneratedEnum::BeginDestroy()
{
	ReleasePythonResources();
	Super::BeginDestroy();
}

void UUPyGeneratedEnum::ReleasePythonResources()
{
	// This may be called after Python has already shut down
	if (Py_IsInitialized())
	{
		FUPyScopedGIL GIL;
		UnregisterGeneratedType();
		PyType.Reset();
	}
	else
	{
		// Release ownership if Python has been shut down to avoid attempting to delete the object (which is already dead)
		PyType.Release();
	}

	EnumValueDefs.Reset();
	// PyMetaData = FPyWrapperEnumMetaData();
}

void UUPyGeneratedEnum::UnregisterGeneratedType()
{
	if (PyType)
	{
		FUPyWrapperTypeRegistry::Get().UnregisterWrappedEnumType(this);
	}
}

UUPyGeneratedEnum* UUPyGeneratedEnum::GenerateEnum(PyTypeObject* InPyType)
{
	// Builder used to generate the enum
	FUPythonGeneratedEnumBuilder PythonEnumBuilder(InPyType);

	// Add the values to this enum
	TArray<FUPyUValueDef*> PyValueDefs;
	{
		PyObject* FieldKey = nullptr;
		PyObject* FieldValue = nullptr;
		Py_ssize_t FieldIndex = 0;
		while (PyDict_Next(InPyType->tp_dict, &FieldIndex, &FieldKey, &FieldValue))
		{
			const FString FieldName = UPyUtil::PyObjectToUEString(FieldKey);

			if (PyObject_IsInstance(FieldValue, (PyObject*)&UPyUValueDefType) == 1)
			{
				FUPyUValueDef* PyValueDef = (FUPyUValueDef*)FieldValue;
				PyValueDefs.Add(PyValueDef);

				if (!PythonEnumBuilder.CreateValueFromDefinition(FieldName, PyValueDef))
				{
					return nullptr;
				}
			}

			if (PyObject_IsInstance(FieldValue, (PyObject*)&UPyFPropertyDefType) == 1)
			{
				// Properties are not supported on enums
				UPyUtil::SetPythonError(PyExc_Exception, InPyType, TEXT("Enums do not support properties"));
				return nullptr;
			}

			if (PyObject_IsInstance(FieldValue, (PyObject*)&UPyUFunctionDefType) == 1)
			{
				// Functions are not supported on enums
				UPyUtil::SetPythonError(PyExc_Exception, InPyType, TEXT("Enums do not support methods"));
				return nullptr;
			}
		}
	}

	// Finalize the struct with its value meta-data
	return PythonEnumBuilder.Finalize(PyValueDefs);
}

FUPyUEnumDecorator* FUPyUEnumDecorator::New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
{
	FUPyUEnumDecorator* Self = (FUPyUEnumDecorator*)InType->tp_alloc(InType, 0);
	return Self;
}

void FUPyUEnumDecorator::Dealloc(FUPyUEnumDecorator* InSelf)
{
	Deinit(InSelf);
	Py_TYPE(InSelf)->tp_free((PyObject*)InSelf);
}

int FUPyUEnumDecorator::Init(FUPyUEnumDecorator* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	Deinit(InSelf);
	return 0;
}

void FUPyUEnumDecorator::Deinit(FUPyUEnumDecorator* InSelf)
{
}

PyObject* FUPyUEnumDecorator::Call(FUPyUEnumDecorator* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	return PyCallGenerateEnum((PyObject*)InSelf, InArgs, nullptr);
}
