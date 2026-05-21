
#pragma once

#include "UPyCommon.h"
#include "Core/UPyConversion.h"
#include "Core/UPyPtr.h"
#include "Wrapper/UPyWrapperOwnerContext.h"
#include "Wrapper/UPyWrapperObjectBase.h"
#include "Wrapper/UPyWrapperStruct.h"

struct FUPyWrapperBase;
//struct FUPyWrapperStruct;
struct FUPyWrapperFieldPath;
struct FUPyWrapperArray;
struct FUPyWrapperSet;
struct FUPyWrapperMap;
struct FUPyWrapperFixedArray;
struct FUPyWrapperDelegate;
struct FUPyWrapperMulticastDelegate;

/** Generic factory implementation for Python wrapped types. Types should derive from this and implement a CreateInstance and FindInstance function */
template <typename UnrealType, typename PythonType, typename KeyType = UnrealType>
class TUPyWrapperTypeFactory
{
public:
	virtual ~TUPyWrapperTypeFactory() {}

	/** Map a wrapped Python instance associated with the given Unreal instance (called internally by the Python type) */
	virtual void MapInstance(UnrealType InUnrealInstance, PythonType* InPythonInstance)
	{
		MappedInstances.Add(InUnrealInstance, InPythonInstance);
	}

	/** Unmap the wrapped instance associated with the given UObject instance (called internally by the Python type) */
	virtual void UnmapInstance(UnrealType InUnrealInstance)
	{
		MappedInstances.Remove(InUnrealInstance);
	}

	/** Find the wrapped Python instance associated with the given Unreal instance (if any, returns borrowed reference) */
	PythonType* FindInstance(UnrealType InUnrealInstance) const
	{
		return MappedInstances.FindRef(InUnrealInstance);
	}

	bool ContainsInstance(UnrealType InUnrealInstance) const
	{
		return MappedInstances.Contains(InUnrealInstance);
	}

protected:
	/** Callback used to initialize this type */
	typedef TFunctionRef<int(PythonType*)> FCreateInstanceInitializerFunc;

	/** Find the wrapped Python instance associated with the given Unreal instance (if any, returns borrowed reference) */
	PythonType* FindInstanceInternal(UnrealType InUnrealInstance, PyTypeObject* InWrappedPyType) const
	{
		return MappedInstances.FindRef(InUnrealInstance);
	}

	/** Find the wrapped Python instance associated with the given Unreal instance, or create one if needed (returns new reference) */
	PythonType* CreateInstanceInternal(UnrealType InUnrealInstance, const PyTypeObject* InWrappedPyType, FCreateInstanceInitializerFunc CreateInstanceInitializerFunc, const bool bForceCreate = false)
	{
		if (!bForceCreate)
		{
			if (PythonType* ExistingInstance = MappedInstances.FindRef(InUnrealInstance))
			{
				Py_INCREF(ExistingInstance);
				return ExistingInstance;
			}
		}

		TUPyPtr<PythonType> NewInstance = TUPyPtr<PythonType>::StealReference(PythonType::New((PyTypeObject*)InWrappedPyType));
		if (NewInstance)
		{
			if (CreateInstanceInitializerFunc(NewInstance) != 0)
			{
				return nullptr;
			}
		}
		return NewInstance.Release();
	}

	PythonType* CreateInstanceInternal(UnrealType InUnrealInstance, const PyTypeObject* InWrappedPyType, const bool bForceCreate = false)
	{
		if (!bForceCreate)
		{
			if (PythonType* ExistingInstance = MappedInstances.FindRef(InUnrealInstance))
			{
				Py_INCREF(ExistingInstance);
				return ExistingInstance;
			}
		}

		return PythonType::New((PyTypeObject*)InWrappedPyType);
	}

	/** Map from the internal key to wrapped Python instance */
	TMap<UnrealType, PythonType*> MappedInstances;
};

/** Factory for wrapped UObject instances */
class UNREALPYTHON_API FUPyWrapperObjectFactory : public TUPyWrapperTypeFactory<UObject*, FUPyWrapperObjectBase>
{
public:
	/** Access the singleton instance */
	static FUPyWrapperObjectFactory& Get();

	virtual void MapInstance(UObject* InUnrealInstance, FUPyWrapperObjectBase* InPythonInstance) override;
	virtual void UnmapInstance(UObject* InUnrealInstance) override;

#if WITH_EDITOR
	void OnObjectsReplaced(const TMap<UObject*, UObject*>& ReplacementMap);
#endif

	/** Find the wrapped Python instance associated with the given Unreal instance, or create one if needed (returns new reference) */
	FUPyWrapperObjectBase* CreateInstance(UObject* InUnrealInstance);

	FUPyWrapperObjectBase* CreateInstance(const PyTypeObject* InPyType, UObject* InUnrealInstance);

	/** Find the wrapped Python instance associated with the given Unreal instance, or create one if needed (returns new reference) */
	FUPyWrapperObjectBase* CreateInstance(UClass* InInterfaceClass, UObject* InUnrealInstance);

	void AddOwnedPyProp(UObject* InUnrealInstance, FUPyWrapperBase* PyProp);

	void RemoveOwnedPyProps(const UObject* InUnrealInstance);

	/** Map from the internal key to wrapped Python instance */
	TMap<const UObject*, TSet<FUPyWrapperBase*>> MappedOwnedPyProps;
};

/** Factory for wrapped UScriptStruct instances */
class UNREALPYTHON_API FUPyWrapperStructFactory : public TUPyWrapperTypeFactory<void*, FUPyWrapperStruct>
{
public:
	/** Access the singleton instance */
	static FUPyWrapperStructFactory& Get();

	/** Find the wrapped Python instance associated with the given Unreal instance, or create one if needed (returns new reference) */
	FUPyWrapperStruct* CreateInstance(const UScriptStruct* InStruct, void* InUnrealInstance, const FUPyWrapperOwnerContext& InOwnerContext, const EUPyConversionMethod InConversionMethod);

	FUPyWrapperStruct* CreateInstance(const PyTypeObject* InPyType, const UScriptStruct* InStruct, void* InUnrealInstance, const FUPyWrapperOwnerContext& InOwnerContext, const EUPyConversionMethod InConversionMethod);

	// template<typename PyStructType>
	// PyStructType* CreateInstance(void* InUnrealInstance, const FUPyWrapperOwnerContext& InOwnerContext, const EUPyConversionMethod InConversionMethod);
};

/** Factory for wrapped delegate instances */
class FUPyWrapperDelegateFactory : public TUPyWrapperTypeFactory<FScriptDelegate*, FUPyWrapperDelegate>
{
public:
	/** Access the singleton instance */
	static FUPyWrapperDelegateFactory& Get();

	/** Find the wrapped Python instance associated with the given Unreal instance, or create one if needed (returns new reference) */
	FUPyWrapperDelegate* CreateInstance(const UFunction* InDelegateSignature, FScriptDelegate* InUnrealInstance, const FDelegateProperty* InProp, const FUPyWrapperOwnerContext& InOwnerContext, const EUPyConversionMethod InConversionMethod);
};

/** Factory for wrapped multicast delegate instances */
class FUPyWrapperMulticastDelegateFactory : public TUPyWrapperTypeFactory<FMulticastScriptDelegate*, FUPyWrapperMulticastDelegate>
{
public:
	/** Access the singleton instance */
	static FUPyWrapperMulticastDelegateFactory& Get();

	/** Find the wrapped Python instance associated with the given Unreal instance, or create one if needed (returns new reference) */
	FUPyWrapperMulticastDelegate* CreateInstance(const UFunction* InDelegateSignature, FMulticastScriptDelegate* InUnrealInstance, void* InPropAddr, const FMulticastDelegateProperty* InProp, const FUPyWrapperOwnerContext& InOwnerContext, const EUPyConversionMethod InConversionMethod);
};


/** Factory for wrapped array instances */
class FUPyWrapperArrayFactory : public TUPyWrapperTypeFactory<void*, FUPyWrapperArray>
{
public:
	/** Access the singleton instance */
	static FUPyWrapperArrayFactory& Get();

	/** Find the wrapped Python instance associated with the given Unreal instance, or create one if needed (returns new reference) */
	FUPyWrapperArray* CreateInstance(void* InUnrealInstance, const FArrayProperty* InProp, const FUPyWrapperOwnerContext& InOwnerContext, const EUPyConversionMethod InConversionMethod);
};

/** Factory for wrapped fixed-array instances */
class FUPyWrapperFixedArrayFactory : public TUPyWrapperTypeFactory<void*, FUPyWrapperFixedArray>
{
public:
	/** Access the singleton instance */
	static FUPyWrapperFixedArrayFactory& Get();

	/** Find the wrapped Python instance associated with the given Unreal instance (if any, returns borrowed reference) */
	FUPyWrapperFixedArray* FindInstance(void* InUnrealInstance) const;

	/** Find the wrapped Python instance associated with the given Unreal instance, or create one if needed (returns new reference) */
	FUPyWrapperFixedArray* CreateInstance(void* InUnrealInstance, const FProperty* InProp, const FUPyWrapperOwnerContext& InOwnerContext, const EUPyConversionMethod InConversionMethod);
};

/** Factory for wrapped set instances */
class FUPyWrapperSetFactory : public TUPyWrapperTypeFactory<void*, FUPyWrapperSet>
{
public:
	/** Access the singleton instance */
	static FUPyWrapperSetFactory& Get();

	/** Find the wrapped Python instance associated with the given Unreal instance, or create one if needed (returns new reference) */
	FUPyWrapperSet* CreateInstance(void* InUnrealInstance, const FSetProperty* InProp, const FUPyWrapperOwnerContext& InOwnerContext, const EUPyConversionMethod InConversionMethod);
};

/** Factory for wrapped map instances */
class FUPyWrapperMapFactory : public TUPyWrapperTypeFactory<void*, FUPyWrapperMap>
{
public:
	/** Access the singleton instance */
	static FUPyWrapperMapFactory& Get();

	/** Find the wrapped Python instance associated with the given Unreal instance (if any, returns borrowed reference) */
	FUPyWrapperMap* FindInstance(void* InUnrealInstance) const;

	/** Find the wrapped Python instance associated with the given Unreal instance, or create one if needed (returns new reference) */
	FUPyWrapperMap* CreateInstance(void* InUnrealInstance, const FMapProperty* InProp, const FUPyWrapperOwnerContext& InOwnerContext, const EUPyConversionMethod InConversionMethod);
};

/** Factory for wrapped field type instances */
class FUPyWrapperFieldPathFactory : public TUPyWrapperTypeFactory<FFieldPath, FUPyWrapperFieldPath>
{
public:
	/** Access the singleton instance */
	static FUPyWrapperFieldPathFactory& Get();

	/** Find the wrapped Python instance associated with the given Unreal instance (if any, returns borrowed reference) */
	FUPyWrapperFieldPath* FindInstance(FFieldPath InUnrealInstance) const;

	/** Find the wrapped Python instance associated with the given Unreal instance, or create one if needed (returns new reference) */
	FUPyWrapperFieldPath* CreateInstance(FFieldPath InUnrealInstance);
};
