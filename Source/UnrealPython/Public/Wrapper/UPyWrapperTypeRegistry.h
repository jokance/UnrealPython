// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UPyCommon.h"
#include "DynamicTypes/UPyGeneratedWrappedType.h"
#include "Wrapper/UPyWrapperStruct.h"
#include "Wrapper/UPyWrapperObjectBase.h"
#include "Containers/Set.h"
#include "Subclassing/UPyGeneratedClass.h"
#include "Subclassing/UPyGeneratedStruct.h"

struct FUPyGeneratedWrappedClassType;

class UNREALPYTHON_API FUPyWrapperTypeRegistry
{
public:
	static FUPyWrapperTypeRegistry& Get();
	
	/** Generate a wrapped type for the given class (only if this class has not yet been registered; will also register the type) */
	const PyTypeObject* GenerateWrappedClassType(const UClass* InClass);

	void GenerateClassPyAttrs(const UClass* InClass, FUPyGeneratedWrappedClassType* GeneratedWrappedType) const;
	
	/** Register the wrapped type associated with the given class name */
	void RegisterWrappedClassType(const UClass* Class, const PyTypeObject* PyType);

	/** Unregister the wrapped type associated with the given class name */
	void UnregisterWrappedClassType(const UClass* Class);

	/** True if we have wrapped type for the exact given class */
	bool HasWrappedClassType(const UClass* InClass) const;

	/** Get the best wrapped type for the given class */
	const PyTypeObject* GetWrappedClassType(const UClass* InClass);
	TSharedPtr<UPyGenUtil::FGeneratedWrappedType> GetGeneratedWrappedType(const UField* InField) const;
	
	const UClass* FindClass(const PyTypeObject* InPyType) const;
	const UScriptStruct* FindStruct(const PyTypeObject* InPyType) const;
	const UEnum* FindEnum(const PyTypeObject* InPyType) const;
	const UFunction* FindDelegate(const PyTypeObject* InPyType) const;
	
	/** Generate a wrapped type for the given struct (only if this struct has not yet been registered; will also register the type) */
	const PyTypeObject* GenerateWrappedStructType(const UScriptStruct* InStruct);

	/** Get the best wrapped type for the given struct */
	const PyTypeObject* GetWrappedStructType(const UScriptStruct* InStruct);

	/** Register the wrapped type associated with the given struct name */
	void RegisterWrappedStructType(const UScriptStruct* InStruct, const PyTypeObject* PyType);

	/** Unregister the wrapped type associated with the given struct name */
	void UnregisterWrappedStructType(const UScriptStruct* InStruct);

	/** True if we have wrapped type for the exact given struct */
	bool HasWrappedStructType(const UScriptStruct* InStruct) const;
	
	/** Get the factory for an inline struct (if known) from its Unreal struct name */
	const IUPyWrapperInlineStructFactory* GetInlineStructFactory(const FTopLevelAssetPath& StructName) const;

	/**
	 * Register the factory for an inline struct (ie, a struct known at compile time that will allocate its instance data inlined within the Python object).
	 * @note Inline struct registration must happen before the first call to GenerateWrappedStructType, and this function will assert if that is not the case!
	 */
	void RegisterInlineStructFactory(const TSharedRef<const IUPyWrapperInlineStructFactory>& InFactory);

	/** Get the best wrapped type for the given enum */
	const PyTypeObject* GetWrappedEnumType(const UEnum* InEnum);

	/** Generate a wrapped type for the given enum (only if this enum has not yet been registered; will also register the type) */
	const PyTypeObject* GenerateWrappedEnumType(const UEnum* InEnum);

	/** Register the wrapped type associated with the given enum name */
	void RegisterWrappedEnumType(const UEnum* InEnum, const PyTypeObject* PyType);
	
	/** Unregister the wrapped type associated with the given enum name */
	void UnregisterWrappedEnumType(const UEnum* InEnum);

	/** Get the best wrapped type for the given delegate signature */
	const PyTypeObject* GetWrappedDelegateType(const UFunction* InDelegateSignature);

	/** Generate a wrapped type for the given delegate signature (only if this delegate has not yet been registered; will also register the type) */
	const PyTypeObject* GenerateWrappedDelegateType(const UFunction* InDelegateSignature);

	/** Register the wrapped type associated with the given delegate name */
	void RegisterWrappedDelegateType(const UFunction* InDelegateSignature, PyTypeObject* PyType);

	/** Register the information about a native Python module */
	void RegisterNativePythonModule(UPyGenUtil::FNativePythonModule&& NativePythonModule);

	const TArray<UPyGenUtil::FNativePythonModule>& GetNativePythonModules() const;

	/** Register a function for creating Python type from UE struct definition */
	void RegisterAutoWrappedStructCreateFunc(const UScriptStruct* InStruct, UPyGenUtil::UPyAutoWrappedStructCreateFunc InCreateFunc);

	/** Register a function for creating Python type from UE class definition. */
	void RegisterAutoWrappedClassCreateFunc(const UClass* InClass, UPyGenUtil::UPyAutoWrappedClassCreateFunc InCreateFunc);

	/** Generate all statically registered Python types. */
	void StartCreateAutoWrappedTypes(UPyGenUtil::FNativePythonModule& ModuleInfo);
	
private:
	const PyTypeObject* CreateAutoWrappedStruct(const UScriptStruct* InStruct, TSet<const UScriptStruct*>& StructsInProgress, UPyGenUtil::FNativePythonModule& ModuleInfo);
	const PyTypeObject* CreateAutoWrappedClass(const UClass* InClass, TSet<const UClass*>& ClassesInProgress, UPyGenUtil::FNativePythonModule& ModuleInfo);

	void AddAutoWrappedType(PyTypeObject* InPyType);
		
	FUPyWrapperTypeRegistry();
	
	/** Map from the Unreal class name to the Python type */
	TMap<const UClass*, const PyTypeObject*> PythonWrappedClasses;

	/** Map from the Unreal struct name to the Python type */
	TMap<const UScriptStruct*, const PyTypeObject*> PythonWrappedStructs;

	/** Map from the Unreal enum name to the Python type */
	TMap<const UEnum*, const PyTypeObject*> PythonWrappedEnums;

	/** Map from the Unreal delegate signature name to the Python type */
	TMap<const UFunction*, const PyTypeObject*> PythonWrappedDelegates;

	/** Map from the Unreal type name to the generated Python type data */
	TMap<const UField*, TSharedPtr<UPyGenUtil::FGeneratedWrappedType>> GeneratedWrappedTypes;
	
	/** Map from the Unreal struct name to the factory data for an inline struct (ie, a struct known at compile time that will allocate its instance data inlined within the Python object) */
	TMap<FTopLevelAssetPath, TSharedPtr<const IUPyWrapperInlineStructFactory>> InlineStructFactories;

	TMap<const PyTypeObject*, const UClass*> PyType2Class;

	TMap<const PyTypeObject*, const UScriptStruct*> PyType2Struct;

	TMap<const PyTypeObject*, const UEnum*> PyType2Enum;

	TMap<const PyTypeObject*, const UFunction*> PyType2Delegate;

	/** Array of information about native Python modules */
	TArray<UPyGenUtil::FNativePythonModule> NativePythonModules;

	TMap<const UScriptStruct*, UPyGenUtil::UPyAutoWrappedStructCreateFunc> AutoWrappedStructCreateFuncs;
	TMap<const UClass*, UPyGenUtil::UPyAutoWrappedClassCreateFunc> AutoWrappedClassCreateFuncs;
};

/** Singleton instance that handles re-instancing Python types */
class FUPyWrapperTypeReinstancer : public FGCObject
{
public:
	/** Access the singleton instance */
	static FUPyWrapperTypeReinstancer& Get();

	//~ FGCObject interface
	virtual void AddReferencedObjects(FReferenceCollector& InCollector) override;
	virtual FString GetReferencerName() const override;

	/** Add a pending pair of classes to be re-instanced */
	void AddPendingClass(UUPyGeneratedClass* OldClass, UUPyGeneratedClass* NewClass);

	/** Add a pending pair of structs to be re-instanced */
	void AddPendingStruct(UUPyGeneratedStruct* OldStruct, UUPyGeneratedStruct* NewStruct);

	/** Process any pending re-instance requests */
	void ProcessPending();

private:
	/** Pending pairs of classes that to be re-instanced */
	TArray<TPair<TObjectPtr<UUPyGeneratedClass>, TObjectPtr<UUPyGeneratedClass>>> ClassesToReinstance;

	/** Pending pairs of structs that to be re-instanced */
	TArray<TPair<TObjectPtr<UUPyGeneratedStruct>, TObjectPtr<UUPyGeneratedStruct>>> StructsToReinstance;
};