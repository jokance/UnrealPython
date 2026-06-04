
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UObject/GCObject.h"
#include "UPyManager.generated.h"

struct FUPyWrapperObjectBase;

class UNREALPYTHON_API FUPyPythonOwnedObjectReferencer : public FGCObject
{
public:
	void Add(UObject* Object);
	void Remove(UObject* Object);
	bool Contains(UObject* Object) const;
	void Reset();
#if WITH_EDITOR
	void OnObjectsReplaced(const TMap<UObject*, UObject*>& ReplacementMap);
#endif

	// FGCObject interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override;

private:
	TSet<TObjectPtr<UObject>> Objects;
};

UCLASS()
class UNREALPYTHON_API UUPyManager : public UObject, public FUObjectArray::FUObjectDeleteListener
{
	GENERATED_BODY()

public:

	static UUPyManager* Get();

	void Initialize();
	void Shutdown();

	void RedirectOutput();

	bool AddPythonOwnedObject(FUPyWrapperObjectBase* InSelf);
	void RemovePythonOwnedObject(FUPyWrapperObjectBase* InSelf);
	bool IsPythonOwnedObject(FUPyWrapperObjectBase* InSelf) const;

	// UObjectArray listener interface
	virtual void NotifyUObjectDeleted(const class UObjectBase *Object, int32 Index) override;
	virtual void OnUObjectArrayShutdown() override { GUObjectArray.RemoveUObjectDeleteListener(this); }
	virtual void BeginDestroy() override;
	// End of interface

#if WITH_EDITOR
	void OnObjectsReplaced(const TMap<UObject*, UObject*>& ReplacementMap);
#endif

	bool Tick(float DeltaTime);
	void RegisterTicker();
	void UnregisterTicker();

private:
	FTSTicker::FDelegateHandle TickerHandle;

	FUPyPythonOwnedObjectReferencer PythonOwnedObjects;
};
