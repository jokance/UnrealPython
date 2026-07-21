#pragma once

#include "UObject/GCObject.h"

struct FUPyWrapperBase;
class FProperty;
class UStruct;

/** Keeps Unreal references owned by live Python wrapper instances visible to UE GC. */
class FUPyReferenceCollector : public FGCObject
{
public:
	static FUPyReferenceCollector& Get();

	void AddWrappedInstance(FUPyWrapperBase* InInstance);
	void RemoveWrappedInstance(FUPyWrapperBase* InInstance);
	void Shutdown();

	//~ FGCObject interface
	virtual void AddReferencedObjects(FReferenceCollector& InCollector) override;
	virtual FString GetReferencerName() const override;

	static void AddReferencedObjectsFromStruct(FReferenceCollector& InCollector, const UStruct* InStruct, void* InStructAddr);
	static void AddReferencedObjectsFromProperty(FReferenceCollector& InCollector, const FProperty* InProp, void* InBaseAddr);

private:
	static void AddReferencedObjectsFromStructInternal(FReferenceCollector& InCollector, const UStruct* InStruct, void* InStructAddr, bool& OutValueChanged);
	static void AddReferencedObjectsFromPropertyInternal(FReferenceCollector& InCollector, const FProperty* InProp, void* InBaseAddr, bool& OutValueChanged);

	TSet<FUPyWrapperBase*> PythonWrappedInstances;
};
