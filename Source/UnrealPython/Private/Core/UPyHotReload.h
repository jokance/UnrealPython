#pragma once

#if WITH_EDITOR
#include "CoreMinimal.h"

class IConsoleObject;
struct FFileChangeData;

/**
 * Watches the Python script directories in editor builds and reloads modified modules
 * through the plugin's ue_hotreload Python module.
 */
class FUPyHotReload
{
public:
	static FUPyHotReload& Get();

	/** Start watching the script directories and register the UnrealPython.Reload console command. */
	void Startup(const TArray<FString>& ScriptDirectories);

	/** Stop watching and unregister the console command. */
	void Shutdown();

	/** Reload modified scripts once their changes have settled. Called every frame on the game thread. */
	void Tick(float DeltaTime);

	/** Reload the modules of all modified scripts collected so far. */
	void ReloadModifiedScripts();

	/** Reload loaded Python modules by name. */
	void ReloadModules(const TArray<FString>& ModuleNames);

private:
	FUPyHotReload() = default;

	void OnDirectoryChanged(const TArray<FFileChangeData>& FileChanges);
	void OnReloadCommand(const TArray<FString>& Args);

	TArray<TPair<FString, FDelegateHandle>> WatchedDirectories;
	TSet<FString> ModifiedScripts;
	double LastChangeTime = 0.0;
	IConsoleObject* ReloadCommand = nullptr;
};
#endif // WITH_EDITOR
