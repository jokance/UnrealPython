
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintPlatformLibrary.h"
#include "Core/UPyPtr.h"
#include "UPyGameInstance.generated.h"

UCLASS(Blueprintable)
class UNREALPYTHON_API UUPyGameInstance: public UPlatformGameInstance
{
	GENERATED_UCLASS_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;
	virtual void OnStart() override;

	UFUNCTION(Exec)
	void PyGC();

protected:
	/** The name of the Python module to load for game instance callbacks (init, shutdown, on_start, tick) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Python")
	FString GameInstanceModuleName;

	void CallModuleFunction(const char* FunctionName, PyObject* PyArgs = nullptr);

private:
	void CleanupModule();
	FUPyObjectPtr CacheModuleFunction(const char* FunctionName);

	bool Tick(float DeltaTime);
	void RegisterTicker();
	void UnregisterTicker();

	FUPyObjectPtr GameInstanceModule;
	FUPyObjectPtr PyTickFunction;
	FTSTicker::FDelegateHandle TickerHandle;

	bool bIsInitialized;
};
