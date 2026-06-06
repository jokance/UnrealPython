
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
	/** The name of the Python module that creates the Python GameInstance object */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Python")
	FString GameInstanceModuleName;

	/** The factory function on the Python module that creates the callback target object */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Python")
	FString GameInstanceFactoryFunctionName;

	void CallGameInstanceFunction(const char* FunctionName, PyObject* PyArgs = nullptr);

private:
	bool EnsureModuleLoaded();
	bool CreateGameInstanceObject();
	FUPyObjectPtr GetCallableAttribute(PyObject* Target, const char* FunctionName, bool bWarnIfNotCallable = true);
	bool HasGameInstanceFunction(const char* FunctionName);
	void CleanupModule();

	bool Tick(float DeltaTime);
	void RegisterTicker();
	void UnregisterTicker();

	FUPyObjectPtr GameInstanceModule;
	FUPyObjectPtr GameInstanceObject;
	FTSTicker::FDelegateHandle TickerHandle;

	bool bIsInitialized;
};
