
#include "Wrapper/UPyWrapperTypeRegistry.h"

#include "Core/UPyGIL.h"
#include "Wrapper/UPyWrapperObject.h"
#include "Wrapper/UPyWrapperStruct.h"
#include "Wrapper/UPyWrapperEnum.h"
#include "Wrapper/UPyWrapperDelegate.h"
#include "DynamicTypes/UPyDynamicTypeFactory.h"
#include "DynamicTypes/UPyGeneratedWrappedClassType.h"
#include "DynamicTypes/UPyGeneratedWrappedStructType.h"
#include "DynamicTypes/UPyGeneratedWrappedEnumType.h"
#include "StructUtils/UserDefinedStruct.h"
#include "Engine/UserDefinedEnum.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Containers/Set.h"
#include "Core/UPyModuleInitializer.h"
#if WITH_EDITOR
#include "Kismet2/ReloadUtilities.h"
#endif

FUPyWrapperTypeRegistry::FUPyWrapperTypeRegistry()
{
}

void FUPyWrapperTypeRegistry::RegisterNativePythonModule(UPyGenUtil::FNativePythonModule&& NativePythonModule)
{
	NativePythonModules.Add(MoveTemp(NativePythonModule));
}

const TArray<UPyGenUtil::FNativePythonModule>& FUPyWrapperTypeRegistry::GetNativePythonModules() const
{
	return NativePythonModules;
}

void FUPyWrapperTypeRegistry::RegisterAutoWrappedStructCreateFunc(const UScriptStruct* InStruct, UPyGenUtil::UPyAutoWrappedStructCreateFunc InCreateFunc)
{
	AutoWrappedStructCreateFuncs.Add(InStruct, InCreateFunc);
}

void FUPyWrapperTypeRegistry::RegisterAutoWrappedClassCreateFunc(const UClass* InClass, UPyGenUtil::UPyAutoWrappedClassCreateFunc InCreateFunc)
{
	AutoWrappedClassCreateFuncs.Add(InClass, InCreateFunc);
}

void FUPyWrapperTypeRegistry::StartCreateAutoWrappedTypes(UPyGenUtil::FNativePythonModule& ModuleInfo)
{
	TSet<const UScriptStruct*> StructsInProgress;
	for (const TPair<const UScriptStruct*, UPyGenUtil::UPyAutoWrappedStructCreateFunc>& Pair : AutoWrappedStructCreateFuncs)
	{
		CreateAutoWrappedStruct(Pair.Key, StructsInProgress, ModuleInfo);
	}

	TSet<const UClass*> ClassesInProgress;
	for (const TPair<const UClass*, UPyGenUtil::UPyAutoWrappedClassCreateFunc>& Pair : AutoWrappedClassCreateFuncs)
	{
		CreateAutoWrappedClass(Pair.Key, ClassesInProgress, ModuleInfo);
	}

	AutoWrappedStructCreateFuncs.Empty();
	AutoWrappedClassCreateFuncs.Empty();
}

const PyTypeObject* FUPyWrapperTypeRegistry::CreateAutoWrappedStruct(const UScriptStruct* InStruct, TSet<const UScriptStruct*>& StructsInProgress, UPyGenUtil::FNativePythonModule& ModuleInfo)
{
	if (!InStruct)
	{
		return nullptr;
	}

	if (const PyTypeObject* ExistingType = PythonWrappedStructs.FindRef(InStruct))
	{
		return ExistingType;
	}

	if (StructsInProgress.Contains(InStruct))
	{
		return PythonWrappedStructs.FindRef(InStruct);
	}

	StructsInProgress.Add(InStruct);

	const UScriptStruct* SuperStruct = Cast<UScriptStruct>(InStruct->GetSuperStruct());
	const PyTypeObject* SuperPyType = nullptr;
	if (SuperStruct)
	{
		SuperPyType = CreateAutoWrappedStruct(SuperStruct, StructsInProgress, ModuleInfo);
		if (!SuperPyType)
		{
			SuperPyType = GetWrappedStructType(SuperStruct);
		}
	}

	const UPyGenUtil::UPyAutoWrappedStructCreateFunc* CreateFuncPtr = AutoWrappedStructCreateFuncs.Find(InStruct);
	const PyTypeObject* ResultType = nullptr;

	if (CreateFuncPtr && *CreateFuncPtr)
	{
		PyTypeObject* CreatedPyType = (*CreateFuncPtr)(InStruct, const_cast<PyTypeObject*>(SuperPyType ? SuperPyType : &UPyWrapperStructType));
		if (CreatedPyType)
		{
			if (!PythonWrappedStructs.Contains(InStruct))
			{
				RegisterWrappedStructType(InStruct, CreatedPyType);
			}
			ResultType = CreatedPyType;

			ModuleInfo.AddType(CreatedPyType);
		}
	}

	if (!ResultType)
	{
		ResultType = GetWrappedStructType(InStruct);
	}

	StructsInProgress.Remove(InStruct);
	return ResultType;
}

const PyTypeObject* FUPyWrapperTypeRegistry::CreateAutoWrappedClass(const UClass* InClass, TSet<const UClass*>& ClassesInProgress, UPyGenUtil::FNativePythonModule& ModuleInfo)
{
	if (!InClass)
	{
		return nullptr;
	}

	if (const PyTypeObject* ExistingType = PythonWrappedClasses.FindRef(InClass))
	{
		return ExistingType;
	}

	if (ClassesInProgress.Contains(InClass))
	{
		return PythonWrappedClasses.FindRef(InClass);
	}

	ClassesInProgress.Add(InClass);

	const UClass* SuperClass = InClass->GetSuperClass();
	const PyTypeObject* SuperPyType = nullptr;
	if (SuperClass)
	{
		SuperPyType = CreateAutoWrappedClass(SuperClass, ClassesInProgress, ModuleInfo);
		if (!SuperPyType)
		{
			SuperPyType = GetWrappedClassType(SuperClass);
		}
	}

	const UPyGenUtil::UPyAutoWrappedClassCreateFunc* CreateFuncPtr = AutoWrappedClassCreateFuncs.Find(InClass);
	const PyTypeObject* ResultType = nullptr;

	if (CreateFuncPtr && *CreateFuncPtr)
	{
		PyObject* SuperPyTypes = nullptr;
		// if (!InClass->Interfaces.IsEmpty())
		// {
		// 	const int32 NumInterfaces = InClass->Interfaces.Num();
		// 	SuperPyTypes = PyTuple_New(NumInterfaces + 1);
		//
		// 	PyTypeObject* BaseTypeForTuple = const_cast<PyTypeObject*>(SuperPyType ? SuperPyType : &UPyWrapperObjectBaseType);
		// 	Py_INCREF(BaseTypeForTuple);
		// 	PyTuple_SetItem(SuperPyTypes, NumInterfaces, (PyObject*)BaseTypeForTuple);
		//
		// 	for (int32 InterfaceIndex = 0; InterfaceIndex < NumInterfaces; ++InterfaceIndex)
		// 	{
		// 		const UClass* InterfaceClass = InClass->Interfaces[InterfaceIndex].Class;
		// 		const PyTypeObject* InterfacePyType = InterfaceClass ? CreateAutoWrappedClass(InterfaceClass, ClassesInProgress, ModuleInfo) : nullptr;
		// 		if (!InterfacePyType && InterfaceClass)
		// 		{
		// 			InterfacePyType = GetWrappedClassType(InterfaceClass);
		// 		}
		//
		// 		PyObject* InterfacePyTypeObj = InterfacePyType ? (PyObject*)const_cast<PyTypeObject*>(InterfacePyType) : Py_None;
		// 		Py_INCREF(InterfacePyTypeObj);
		// 		PyTuple_SetItem(SuperPyTypes, InterfaceIndex, InterfacePyTypeObj);
		// 	}
		// }

		PyTypeObject* SuperTypeForCall = const_cast<PyTypeObject*>(SuperPyType ? SuperPyType : &UPyWrapperObjectType);
		PyTypeObject* CreatedPyType = (*CreateFuncPtr)(InClass, SuperTypeForCall, SuperPyTypes);

		if (CreatedPyType)
		{
			if (!PythonWrappedClasses.Contains(InClass))
			{
				RegisterWrappedClassType(InClass, CreatedPyType);
			}

			if (SuperPyTypes && CreatedPyType->tp_bases != SuperPyTypes)
			{
				Py_DECREF(SuperPyTypes);
			}

			ResultType = CreatedPyType;

			ModuleInfo.AddType(CreatedPyType);
		}
		else
		{
			Py_XDECREF(SuperPyTypes);
		}
	}

	if (!ResultType)
	{
		ResultType = GetWrappedClassType(InClass);
	}

	ClassesInProgress.Remove(InClass);
	return ResultType;
}

void FUPyWrapperTypeRegistry::AddAutoWrappedType(PyTypeObject* InPyType)
{
	Py_INCREF(InPyType);
	PyObject* UEModule = FUPyModuleInitializer::Get().GetPyUEModule();
	PyModule_AddObject(UEModule, InPyType->tp_name, (PyObject*)InPyType);
}

FUPyWrapperTypeRegistry& FUPyWrapperTypeRegistry::Get()
{
	static FUPyWrapperTypeRegistry Instance;
	return Instance;
}

const PyTypeObject* FUPyWrapperTypeRegistry::GenerateWrappedClassType(const UClass* InClass)
{
	if (const PyTypeObject* ExistingPyType = PythonWrappedClasses.FindRef(InClass))
	{
		return ExistingPyType;
	}

	// todo: allow generation of Blueprint generated classes
	if (((UObject*)InClass)->IsA<UBlueprintGeneratedClass>() || !EnumHasAnyFlags(InClass->ClassFlags, EClassFlags::CLASS_Native))
	{
		return nullptr;
	}

	// Make sure the parent class is also wrapped
	PyTypeObject* SuperPyType = nullptr;
	if (const UClass* SuperClass = InClass->GetSuperClass())
	{
		SuperPyType = const_cast<PyTypeObject*>(GenerateWrappedClassType(SuperClass));
	}

	if (!SuperPyType)
	{
		UE_LOG(LogUnrealPython, Fatal, TEXT("Failed to generate wrapped class type for '%s': could not generate parent class type!"), *InClass->GetPathName());
		return nullptr;
	}

	PyObject* SuperPyTypes = nullptr;
	//if (!InClass->Interfaces.IsEmpty())
	//{
	//	SuperPyTypes = PyTuple_New(InClass->Interfaces.Num() + 1);
	//	Py_INCREF(SuperPyType);
	//	PyTuple_SetItem(SuperPyTypes, 0, (PyObject*)SuperPyType);

	//	for (int32 i = 0; i < InClass->Interfaces.Num(); i++)
	//	{
	//		const PyTypeObject* InterfacePyType = GenerateWrappedClassType(InClass->Interfaces[i].Class);
	//		Py_INCREF(InterfacePyType);
	//		PyTuple_SetItem(SuperPyTypes, i + 1, (PyObject*)InterfacePyType);
	//	}
	//}

	TSharedPtr<FUPyGeneratedWrappedClassType> GeneratedWrappedType = MakeShared<FUPyGeneratedWrappedClassType>();
	GeneratedWrappedTypes.Add(InClass, GeneratedWrappedType);

	const FString PythonClassName = InClass->GetName();
	GeneratedWrappedType->TypeName = UPyGenUtil::TCHARToUTF8Buffer(*PythonClassName);

	GenerateClassPyAttrs(InClass, GeneratedWrappedType.Get());

	GeneratedWrappedType->PyType.tp_basicsize = SuperPyType->tp_basicsize;
	GeneratedWrappedType->PyType.tp_base = SuperPyType;
	GeneratedWrappedType->PyType.tp_bases = SuperPyTypes;
	GeneratedWrappedType->PyType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
	GeneratedWrappedType->PyType.tp_str = SuperPyType->tp_str;
	GeneratedWrappedType->PyType.tp_repr = SuperPyType->tp_repr;

	if (GeneratedWrappedType->Finalize())
	{
		RegisterWrappedClassType(InClass, &GeneratedWrappedType->PyType);

		return &GeneratedWrappedType->PyType;
	}

	UE_LOG(LogUnrealPython, Fatal, TEXT("Failed to generate Python glue code for class '%s'!"), *InClass->GetPathName());
	return nullptr;
}

void FUPyWrapperTypeRegistry::GenerateClassPyAttrs(const UClass* InClass, FUPyGeneratedWrappedClassType* GeneratedWrappedType) const
{
	for (TFieldIterator<const FProperty> FieldIt(InClass, EFieldIteratorFlags::ExcludeSuper); FieldIt; ++FieldIt)
	{
		const FProperty* Prop = *FieldIt;
		FUPyDynamicTypeFactory::GenerateWrappedPropertyForClass(InClass, GeneratedWrappedType, Prop);
	}

	for (TFieldIterator<const UFunction> FieldIt(InClass, EFieldIteratorFlags::ExcludeSuper, EFieldIteratorFlags::IncludeDeprecated); FieldIt; ++FieldIt)
	{
		const UFunction* Func = *FieldIt;
		FUPyDynamicTypeFactory::GenerateWrappedMethodForClass(InClass, GeneratedWrappedType, Func);
	}
}

void FUPyWrapperTypeRegistry::RegisterWrappedClassType(const UClass* Class, const PyTypeObject* PyType)
{
	checkf(!PythonWrappedClasses.Contains(Class), TEXT("Wrapper class %hs register agian!"), PyType->tp_name);
	PythonWrappedClasses.Add(Class, PyType);
	PyType2Class.Add(PyType, Class);

	// Py_INCREF?
}

void FUPyWrapperTypeRegistry::UnregisterWrappedClassType(const UClass* Class)
{
	const PyTypeObject* PyType = nullptr;
	if (!PythonWrappedClasses.RemoveAndCopyValue(Class, PyType))
	{
		return;
	}

	PyType2Class.FindAndRemoveChecked(PyType);
}

bool FUPyWrapperTypeRegistry::HasWrappedClassType(const UClass* InClass) const
{
	return PythonWrappedClasses.Contains(InClass);
}

const PyTypeObject* FUPyWrapperTypeRegistry::GetWrappedClassType(const UClass* InClass)
{
	const PyTypeObject* PyType = &UPyWrapperObjectType;

	for (const UClass* Class = InClass; Class; Class = Class->GetSuperClass())
    {
    	if (const PyTypeObject* ClassPyType = GenerateWrappedClassType(Class))
    	{
    		PyType = ClassPyType;
    		break;
    	}
    }

	return PyType;
}

TSharedPtr<UPyGenUtil::FGeneratedWrappedType> FUPyWrapperTypeRegistry::GetGeneratedWrappedType(const UField* InField) const
{
	return GeneratedWrappedTypes.FindRef(InField);
}

const UClass* FUPyWrapperTypeRegistry::FindClass(const PyTypeObject* InPyType) const
{
	if (auto Class = PyType2Class.Find(InPyType))
	{
		return *Class;
	}
	return nullptr;
}

const UScriptStruct* FUPyWrapperTypeRegistry::FindStruct(const PyTypeObject* InPyType) const
{
	if (auto Struct = PyType2Struct.Find(InPyType))
	{
		return *Struct;
	}
	return nullptr;
}

const UEnum* FUPyWrapperTypeRegistry::FindEnum(const PyTypeObject* InPyType) const
{
	if (auto Enum = PyType2Enum.Find(InPyType))
	{
		return *Enum;
	}
	return nullptr;
}

const UFunction* FUPyWrapperTypeRegistry::FindDelegate(const PyTypeObject* InPyType) const
{
	if (auto Delegate = PyType2Delegate.Find(InPyType))
	{
		return *Delegate;
	}
	return nullptr;
}

const PyTypeObject* FUPyWrapperTypeRegistry::GenerateWrappedStructType(const UScriptStruct* InStruct)
{
	// struct FFuncs
	// {
	// 	static int Init(FUPyWrapperStruct* InSelf, PyObject* InArgs, PyObject* InKwds)
	// 	{
	// 		const int SuperResult = UPyWrapperStructType.tp_init((PyObject*)InSelf, InArgs, InKwds);
	// 		if (SuperResult != 0)
	// 		{
	// 			return SuperResult;
	// 		}
	//
	// 		return FUPyWrapperStruct::MakeStruct(InSelf, InArgs, InKwds);
	// 	}
	// };

	// Already processed? Nothing more to do
	if (const PyTypeObject* ExistingPyType = PythonWrappedStructs.FindRef(InStruct))
	{
		return ExistingPyType;
	}

	// todo: allow generation of Blueprint generated structs
	if (InStruct->IsA<UUserDefinedStruct>())
	{
		return nullptr;
	}

	// Make sure the parent struct is also wrapped
	PyTypeObject* SuperPyType = nullptr;
	if (const UScriptStruct* SuperStruct = Cast<UScriptStruct>(InStruct->GetSuperStruct()))
	{
		SuperPyType = (PyTypeObject*)GenerateWrappedStructType(SuperStruct);
	}

	check(EnumHasAnyFlags(InStruct->StructFlags, EStructFlags(STRUCT_Native | STRUCT_NoExport)));

	TSharedPtr<FUPyGeneratedWrappedStructType> GeneratedWrappedType = MakeShared<FUPyGeneratedWrappedStructType>();
	GeneratedWrappedTypes.Add(InStruct, GeneratedWrappedType);

	const FString PythonStructName = InStruct->GetName();
	GeneratedWrappedType->TypeName = UPyGenUtil::TCHARToUTF8Buffer(*PythonStructName);

	for (TFieldIterator<const FProperty> PropIt(InStruct, EFieldIteratorFlags::ExcludeSuper); PropIt; ++PropIt)
	{
		const FProperty* Prop = *PropIt;
		FUPyDynamicTypeFactory::GenerateWrappedPropertyForStruct(InStruct, GeneratedWrappedType.Get(), Prop);
	}

	int32 WrappedStructSizeBytes = sizeof(FUPyWrapperStruct);
	if (const IUPyWrapperInlineStructFactory* InlineStructFactory = GetInlineStructFactory(InStruct->GetStructPathName()))
	{
		WrappedStructSizeBytes = InlineStructFactory->GetPythonObjectSizeBytes();
	}

	GeneratedWrappedType->PyType.tp_basicsize = WrappedStructSizeBytes;
	GeneratedWrappedType->PyType.tp_base = SuperPyType ? SuperPyType : &UPyWrapperStructType;
	// GeneratedWrappedType->PyType.tp_init = (initproc)&FFuncs::Init;
	GeneratedWrappedType->PyType.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;

	if (GeneratedWrappedType->Finalize())
	{
		RegisterWrappedStructType(InStruct, &GeneratedWrappedType->PyType);

		return &GeneratedWrappedType->PyType;
	}

	UE_LOG(LogUnrealPython, Fatal, TEXT("Failed to generate Python glue code for struct '%s'!"), *InStruct->GetPathName());
	return nullptr;
}

const PyTypeObject* FUPyWrapperTypeRegistry::GetWrappedStructType(const UScriptStruct* InStruct)
{
	const PyTypeObject* PyType = &UPyWrapperStructType;

	for (const UScriptStruct* Struct = InStruct; Struct; Struct = Cast<UScriptStruct>(Struct->GetSuperStruct()))
	{
		if (const PyTypeObject* StructPyType = GenerateWrappedStructType(Struct))
		{
			PyType = StructPyType;
			break;
		}
	}

	return PyType;
}

void FUPyWrapperTypeRegistry::RegisterWrappedStructType(const UScriptStruct* InStruct, const PyTypeObject* PyType)
{
	checkf(!PythonWrappedStructs.Contains(InStruct), TEXT("Wrapper struct %hs register agian!"), PyType->tp_name);
	PythonWrappedStructs.Add(InStruct, PyType);
	PyType2Struct.Add(PyType, InStruct);
}

void FUPyWrapperTypeRegistry::UnregisterWrappedStructType(const UScriptStruct* InStruct)
{
	const PyTypeObject* PyType = nullptr;
	if (!PythonWrappedStructs.RemoveAndCopyValue(InStruct, PyType))
	{
		return;
	}

	PyType2Struct.FindAndRemoveChecked(PyType);
}

bool FUPyWrapperTypeRegistry::HasWrappedStructType(const UScriptStruct* InStruct) const
{
	return PythonWrappedStructs.Contains(InStruct);
}

const IUPyWrapperInlineStructFactory* FUPyWrapperTypeRegistry::GetInlineStructFactory(const FTopLevelAssetPath& StructName) const
{
	return InlineStructFactories.FindRef(StructName).Get();
}

void FUPyWrapperTypeRegistry::RegisterInlineStructFactory(const TSharedRef<const IUPyWrapperInlineStructFactory>& InFactory)
{
	InlineStructFactories.Add(InFactory->GetStructName(), InFactory);
}

const PyTypeObject* FUPyWrapperTypeRegistry::GetWrappedEnumType(const UEnum* InEnum)
{
	const PyTypeObject* PyType = PythonWrappedEnums.FindRef(InEnum);
	if (!PyType)
	{
		PyType = GenerateWrappedEnumType(InEnum);
	}
	return PyType;
}

const PyTypeObject* FUPyWrapperTypeRegistry::GenerateWrappedEnumType(const UEnum* InEnum)
{
	// Already processed? Nothing more to do
	if (const PyTypeObject* ExistingPyType = PythonWrappedEnums.FindRef(InEnum))
	{
		return ExistingPyType;
	}

	// Should we allow Blueprint generated enums?
	if (InEnum->IsA<UUserDefinedEnum>())
	{
		return nullptr;
	}

	TSharedPtr<FUPyGeneratedWrappedEnumType> GeneratedWrappedType = MakeShared<FUPyGeneratedWrappedEnumType>();
	GeneratedWrappedTypes.Add(InEnum, GeneratedWrappedType);

	const FString PythonEnumName = InEnum->GetName();
	GeneratedWrappedType->TypeName = UPyGenUtil::TCHARToUTF8Buffer(*PythonEnumName);
	// GeneratedWrappedType->TypeDoc = PyGenUtil::TCHARToUTF8Buffer(*TypeDocString);
	GeneratedWrappedType->ExtractEnumEntries(InEnum);

	GeneratedWrappedType->PyType.tp_basicsize = 0;
	GeneratedWrappedType->PyType.tp_base = &UPyWrapperEnumType;
	GeneratedWrappedType->PyType.tp_flags = Py_TPFLAGS_DEFAULT;

	// TSharedRef<FPyWrapperEnumMetaData> EnumMetaData = MakeShared<FPyWrapperEnumMetaData>();
	// EnumMetaData->Enum = (UEnum*)InEnum;
	// GeneratedWrappedType->MetaData = EnumMetaData;

	if (GeneratedWrappedType->Finalize())
	{
		RegisterWrappedEnumType(InEnum, &GeneratedWrappedType->PyType);

		return &GeneratedWrappedType->PyType;
	}

	UE_LOG(LogUnrealPython, Fatal, TEXT("Failed to generate Python glue code for enum '%s'!"), *InEnum->GetPathName());
	return nullptr;
}

void FUPyWrapperTypeRegistry::RegisterWrappedEnumType(const UEnum* InEnum, const PyTypeObject* PyType)
{
	checkf(!PythonWrappedEnums.Contains(InEnum), TEXT("Wrapper enum %hs register agian!"), PyType->tp_name);
	PythonWrappedEnums.Add(InEnum, PyType);
	PyType2Enum.Add(PyType, InEnum);
}

void FUPyWrapperTypeRegistry::UnregisterWrappedEnumType(const UEnum* InEnum)
{
	const PyTypeObject* PyType = nullptr;
	if (!PythonWrappedEnums.RemoveAndCopyValue(InEnum, PyType))
	{
		return;
	}

	PyType2Enum.FindAndRemoveChecked(PyType);
}

const PyTypeObject* FUPyWrapperTypeRegistry::GetWrappedDelegateType(const UFunction* InDelegateSignature)
{
	const PyTypeObject* PyType = PythonWrappedDelegates.FindRef(InDelegateSignature);
	if (!PyType)
	{
		PyType = GenerateWrappedDelegateType(InDelegateSignature);
	}
	return PyType;
}

const PyTypeObject* FUPyWrapperTypeRegistry::GenerateWrappedDelegateType(const UFunction* InDelegateSignature)
{
	// Already processed? Nothing more to do
	if (const PyTypeObject* ExistingPyType = PythonWrappedDelegates.FindRef(InDelegateSignature))
	{
		return ExistingPyType;
	}

	// Is this actually a delegate signature?
	if (!InDelegateSignature->HasAnyFunctionFlags(FUNC_Delegate))
	{
		return nullptr;
	}

	TSharedPtr<UPyGenUtil::FGeneratedWrappedType> GeneratedWrappedType = MakeShared<UPyGenUtil::FGeneratedWrappedType>();
	GeneratedWrappedTypes.Add(InDelegateSignature, GeneratedWrappedType);

	// for (TFieldIterator<const FProperty> ParamIt(InDelegateSignature); ParamIt; ++ParamIt)
	// {
	// 	const FProperty* Param = *ParamIt;
	// 	GatherWrappedTypesForPropertyReferences(Param, OutGeneratedWrappedTypeReferences);
	// }

	const FString DelegateBaseTypename = UPyGenUtil::GetDelegatePythonName(InDelegateSignature);
	// GeneratedWrappedType->TypeDoc = PyGenUtil::TCHARToUTF8Buffer(*TypeDocString);
	GeneratedWrappedType->PyType.tp_flags = Py_TPFLAGS_DEFAULT;

	// Two different UObject-based classes can each declare in their body a delegate with the same type name, but possibly
	// with different parameters. While they will share the same 'DelegateBaseTypename', they aren't necessarily the same type. Make
	// the name of the 'PythonCallableForDelegateClass' unique because NewObject<> will find pre-existing object with the same name and
	// return the same address, then the code below will basically erase the previous delegate type with the new delegate type, possibly
	// confusing the parameter types if they aren't the same.
	FNameBuilder NameBuilder;
	if (UClass* OuterClass = Cast<UClass>(InDelegateSignature->GetOuter()))
	{
		FString DelegateOuterClassName = OuterClass->GetName(); // UPyGenUtil::GetClassPythonName(OuterClass);

		// The name of the UObject proxy wrapping the Python callable.
		// NameBuilder += DelegateOuterClassName;
		// NameBuilder += TEXT("_");
		// NameBuilder += DelegateBaseTypename;

		// The name of the Python type. Note that we don't 'nest' in the pure Python sense. The type is flatten and looks like "unreal.OuterName_DelegateName"
		// A correctly nested type would require us to update the outer object Python data structure to mention it has an inner type which we don't.
		FNameBuilder TypenameBuilder;
		TypenameBuilder += DelegateOuterClassName;
		TypenameBuilder += TEXT("_"); // Cannot use "." because we don't really nest the object.
		TypenameBuilder += DelegateBaseTypename;
		GeneratedWrappedType->TypeName = UPyGenUtil::TCHARToUTF8Buffer(*TypenameBuilder);
	}
	else
	{
		// NameBuilder += DelegateBaseTypename;
		GeneratedWrappedType->TypeName = UPyGenUtil::TCHARToUTF8Buffer(*DelegateBaseTypename);
	}
	// NameBuilder += TEXT("__PythonCallable");

	// FName PythonCallableForDelegateObjectName = *NameBuilder;

	// This will be executed on the GameThread after the parallel initialization step
	// auto CreatePythonCallableForDelegate_GameThread =
	// 	[InDelegateSignature, PythonCallableForDelegateObjectName]()
	// 	{
	// 		// Generate the proxy class needed to wrap Python callables in Unreal delegates
	// 		UClass* PythonCallableForDelegateClass = NewObject<UClass>(UPyUtil::GetPythonTypeContainer(), PythonCallableForDelegateObjectName, RF_Public | RF_Transient);
	// 		UFunction* PythonCallableForDelegateFunc = nullptr;
	// 		{
	// 			FObjectDuplicationParameters FuncDuplicationParams(const_cast<UFunction*>(InDelegateSignature), PythonCallableForDelegateClass);
	// 			FuncDuplicationParams.DestName = UUPyCallableForDelegate::GeneratedFuncName;
	// 			FuncDuplicationParams.InternalFlagMask &= ~EInternalObjectFlags::Native;
	// 			PythonCallableForDelegateFunc = CastChecked<UFunction>(StaticDuplicateObjectEx(FuncDuplicationParams));
	// 		}
	// 		PythonCallableForDelegateFunc->FunctionFlags = (PythonCallableForDelegateFunc->FunctionFlags | FUNC_Native) & ~(FUNC_Delegate | FUNC_MulticastDelegate);
	// 		PythonCallableForDelegateFunc->SetNativeFunc(&UUPyCallableForDelegate::CallPythonNative);
	// 		PythonCallableForDelegateFunc->StaticLink(true);
	// 		PythonCallableForDelegateClass->AddFunctionToFunctionMap(PythonCallableForDelegateFunc, PythonCallableForDelegateFunc->GetFName());
	// 		PythonCallableForDelegateClass->SetSuperStruct(UUPyCallableForDelegate::StaticClass());
	// 		PythonCallableForDelegateClass->ClassFlags |= CLASS_HideDropDown;
	// 		PythonCallableForDelegateClass->Bind();
	// 		PythonCallableForDelegateClass->StaticLink(true);
	// 		PythonCallableForDelegateClass->AssembleReferenceTokenStream();
	// 		PythonCallableForDelegateClass->GetDefaultObject();
	//
	// 		return PythonCallableForDelegateClass;
	// 	};

	if (InDelegateSignature->HasAnyFunctionFlags(FUNC_MulticastDelegate))
	{
		GeneratedWrappedType->PyType.tp_basicsize = sizeof(FUPyWrapperMulticastDelegate);
		GeneratedWrappedType->PyType.tp_base = &UPyWrapperMulticastDelegateType;

		// TSharedRef<FPyWrapperMulticastDelegateMetaData> DelegateMetaData = MakeShared<FPyWrapperMulticastDelegateMetaData>();
		// DelegateMetaData->DelegateSignature.SetFunction(InDelegateSignature);
		// GeneratedWrappedType->MetaData = DelegateMetaData;

		// PyGenUtil::FMultithreadedGenerationContext::EnqueueGameThreadTask(
		// 	[DelegateMetaData, CreatePythonCallableForDelegate_GameThread]()
		// 	{
		// 		DelegateMetaData->PythonCallableForDelegateClass = CreatePythonCallableForDelegate_GameThread();
		// 	}
		// );
	}
	else
	{
		GeneratedWrappedType->PyType.tp_basicsize = sizeof(FUPyWrapperDelegate);
		GeneratedWrappedType->PyType.tp_base = &UPyWrapperDelegateType;

		// TSharedRef<FPyWrapperDelegateMetaData> DelegateMetaData = MakeShared<FPyWrapperDelegateMetaData>();
		// DelegateMetaData->DelegateSignature.SetFunction(InDelegateSignature);
		// GeneratedWrappedType->MetaData = DelegateMetaData;

		// PyGenUtil::FMultithreadedGenerationContext::EnqueueGameThreadTask(
		// 	[DelegateMetaData, CreatePythonCallableForDelegate_GameThread]()
		// 	{
		// 		DelegateMetaData->PythonCallableForDelegateClass = CreatePythonCallableForDelegate_GameThread();
		// 	}
		// );
	}

	if (GeneratedWrappedType->Finalize())
	{
		RegisterWrappedDelegateType(InDelegateSignature, &GeneratedWrappedType->PyType);

		return &GeneratedWrappedType->PyType;
	}

	REPORT_UNREAL_PYTHON_GENERATION_ISSUE(Fatal, TEXT("Failed to generate Python glue code for delegate '%s'!"), *InDelegateSignature->GetPathName());
	return nullptr;
}

void FUPyWrapperTypeRegistry::RegisterWrappedDelegateType(const UFunction* InDelegateSignature, PyTypeObject* PyType)
{
	checkf(!PythonWrappedDelegates.Contains(InDelegateSignature), TEXT("Wrapper delegate %hs register agian!"), PyType->tp_name);
	PythonWrappedDelegates.Add(InDelegateSignature, PyType);
	PyType2Delegate.Add(PyType, InDelegateSignature);
}

FUPyWrapperTypeReinstancer& FUPyWrapperTypeReinstancer::Get()
{
	static FUPyWrapperTypeReinstancer Instance;
	return Instance;
}

void FUPyWrapperTypeReinstancer::AddPendingClass(UUPyGeneratedClass* OldClass, UUPyGeneratedClass* NewClass)
{
	ClassesToReinstance.Emplace(MakeTuple(OldClass, NewClass));
}

void FUPyWrapperTypeReinstancer::AddPendingStruct(UUPyGeneratedStruct* OldStruct, UUPyGeneratedStruct* NewStruct)
{
	StructsToReinstance.Emplace(MakeTuple(OldStruct, NewStruct));
}

void FUPyWrapperTypeReinstancer::ProcessPending()
{
#if WITH_EDITOR
	if (ClassesToReinstance.Num() > 0 || StructsToReinstance.Num() > 0)
	{
		TUniquePtr<FReload> Reload(new FReload(EActiveReloadType::Reinstancing, TEXT(""), *GLog));

		for (const auto& ClassToReinstancePair : ClassesToReinstance)
		{
			if (ClassToReinstancePair.Key && ClassToReinstancePair.Value)
			{
				if (!ClassToReinstancePair.Value->HasAnyClassFlags(CLASS_NewerVersionExists))
				{
					// Assume the classes have changed
					Reload->NotifyChange(ClassToReinstancePair.Value, ClassToReinstancePair.Key);
				}
			}
		}

		// This doesn't do anything ATM
		for (const auto& StructToReinstancePair : StructsToReinstance)
		{
			if (StructToReinstancePair.Key && StructToReinstancePair.Value)
			{
				// Assume the structures have changed
				Reload->NotifyChange(StructToReinstancePair.Value, StructToReinstancePair.Key);
			}
		}

		Reload->Reinstance();

		ClassesToReinstance.Reset();
		StructsToReinstance.Reset();

		CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	}
#endif
}

void FUPyWrapperTypeReinstancer::AddReferencedObjects(FReferenceCollector& InCollector)
{
	for (auto& ClassToReinstancePair : ClassesToReinstance)
	{
		InCollector.AddReferencedObject(ClassToReinstancePair.Key);
		InCollector.AddReferencedObject(ClassToReinstancePair.Value);
	}

	for (auto& StructToReinstancePair : StructsToReinstance)
	{
		InCollector.AddReferencedObject(StructToReinstancePair.Key);
		InCollector.AddReferencedObject(StructToReinstancePair.Value);
	}
}

FString FUPyWrapperTypeReinstancer::GetReferencerName() const
{
	return TEXT("FUPyWrapperTypeReinstancer");
}
