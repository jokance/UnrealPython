
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UPyManager.generated.h"

struct FUPyWrapperObjectBase;

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

	UPROPERTY()
	TSet<TObjectPtr<UObject>> PythonOwnedObjects;
};
