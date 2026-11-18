#include "Core/UPyHotReload.h"

#if WITH_EDITOR
// Engine headers go before the plugin headers that include Python.h, whose macros break later engine headers.
#include "DirectoryWatcherModule.h"
#include "IDirectoryWatcher.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "UPySettings.h"
#include "UPyCommon.h"
#include "Helper/UPyBlueprintLibrary.h"

namespace UPyHotReload
{
	/** Python module that performs the reload. */
	const TCHAR* ReloaderModuleName = TEXT("ue_hotreload");

	/** Editors and source control write files in bursts; wait this long after the last change before reloading. */
	constexpr double SettleSeconds = 0.3;

	const FName DirectoryWatcherModuleName = TEXT("DirectoryWatcher");

	IDirectoryWatcher* GetDirectoryWatcher()
	{
		FDirectoryWatcherModule* DirectoryWatcherModule = FModuleManager::GetModulePtr<FDirectoryWatcherModule>(DirectoryWatcherModuleName);
		return DirectoryWatcherModule ? DirectoryWatcherModule->Get() : nullptr;
	}
}

FUPyHotReload& FUPyHotReload::Get()
{
	static FUPyHotReload Instance;
	return Instance;
}

void FUPyHotReload::Startup(const TArray<FString>& ScriptDirectories)
{
	if (ReloadCommand)
	{
		return;
	}

	ReloadCommand = IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("UnrealPython.Reload"),
		TEXT("Reload modified Python scripts. Pass module names to reload those loaded modules instead, for example: UnrealPython.Reload game.player"),
		FConsoleCommandWithArgsDelegate::CreateRaw(this, &FUPyHotReload::OnReloadCommand),
		ECVF_Default);

	FModuleManager::Get().LoadModule(UPyHotReload::DirectoryWatcherModuleName);
	IDirectoryWatcher* DirectoryWatcher = UPyHotReload::GetDirectoryWatcher();
	if (!DirectoryWatcher)
	{
		UE_LOG(LogUnrealPython, Warning, TEXT("Python scripts cannot be watched for hot reload on this platform; use the UnrealPython.Reload console command."));
		return;
	}

	for (const FString& ScriptDirectory : ScriptDirectories)
	{
		FDelegateHandle Handle;
		const IDirectoryWatcher::FDirectoryChanged Callback = IDirectoryWatcher::FDirectoryChanged::CreateRaw(this, &FUPyHotReload::OnDirectoryChanged);
		if (DirectoryWatcher->RegisterDirectoryChangedCallback_Handle(ScriptDirectory, Callback, Handle))
		{
			WatchedDirectories.Emplace(ScriptDirectory, Handle);
			UE_LOG(LogUnrealPython, Log, TEXT("Watching Python scripts for hot reload: %s"), *ScriptDirectory);
		}
		else
		{
			UE_LOG(LogUnrealPython, Warning, TEXT("Failed to watch Python scripts for hot reload: %s"), *ScriptDirectory);
		}
	}
}

void FUPyHotReload::Shutdown()
{
	if (ReloadCommand)
	{
		IConsoleManager::Get().UnregisterConsoleObject(ReloadCommand);
		ReloadCommand = nullptr;
	}

	if (IDirectoryWatcher* DirectoryWatcher = UPyHotReload::GetDirectoryWatcher())
	{
		for (const TPair<FString, FDelegateHandle>& WatchedDirectory : WatchedDirectories)
		{
			DirectoryWatcher->UnregisterDirectoryChangedCallback_Handle(WatchedDirectory.Key, WatchedDirectory.Value);
		}
	}

	WatchedDirectories.Reset();
	ModifiedScripts.Reset();
}

void FUPyHotReload::Tick(float DeltaTime)
{
	if (WatchedDirectories.IsEmpty())
	{
		return;
	}

	// The editor engine ticks the directory watcher; -game processes running editor binaries do not.
	if (!GIsEditor)
	{
		if (IDirectoryWatcher* DirectoryWatcher = UPyHotReload::GetDirectoryWatcher())
		{
			DirectoryWatcher->Tick(DeltaTime);
		}
	}

	if (ModifiedScripts.IsEmpty() || !GetDefault<UUPySettings>()->bAutoReloadScripts)
	{
		return;
	}

	if (FPlatformTime::Seconds() - LastChangeTime < UPyHotReload::SettleSeconds)
	{
		return;
	}

	ReloadModifiedScripts();
}

void FUPyHotReload::ReloadModifiedScripts()
{
	if (ModifiedScripts.IsEmpty())
	{
		return;
	}

	TArray<FString> ScriptFiles = ModifiedScripts.Array();
	ModifiedScripts.Reset();
	ScriptFiles.Sort();

	UUPyBlueprintLibrary::CallPythonMethod<void>(UPyHotReload::ReloaderModuleName, TEXT("reload_files"), ScriptFiles);
}

void FUPyHotReload::ReloadModules(const TArray<FString>& ModuleNames)
{
	UUPyBlueprintLibrary::CallPythonMethod<void>(UPyHotReload::ReloaderModuleName, TEXT("reload_modules"), ModuleNames);
}

void FUPyHotReload::OnDirectoryChanged(const TArray<FFileChangeData>& FileChanges)
{
	for (const FFileChangeData& FileChange : FileChanges)
	{
		if (FileChange.Action == FFileChangeData::FCA_RescanRequired)
		{
			UE_LOG(LogUnrealPython, Warning, TEXT("Too many file changes to track under %s; use UnrealPython.Reload <module> to reload Python modules."), *FileChange.Filename);
			continue;
		}

		if (FileChange.Action != FFileChangeData::FCA_Added && FileChange.Action != FFileChangeData::FCA_Modified)
		{
			continue;
		}

		if (!FPaths::GetExtension(FileChange.Filename).Equals(TEXT("py"), ESearchCase::IgnoreCase))
		{
			continue;
		}

		ModifiedScripts.Add(FPaths::ConvertRelativePathToFull(FileChange.Filename));
		LastChangeTime = FPlatformTime::Seconds();
	}
}

void FUPyHotReload::OnReloadCommand(const TArray<FString>& Args)
{
	if (!Args.IsEmpty())
	{
		ReloadModules(Args);
		return;
	}

	if (ModifiedScripts.IsEmpty())
	{
		UE_LOG(LogUnrealPython, Display, TEXT("No modified Python scripts to reload."));
		return;
	}

	ReloadModifiedScripts();
}
#endif // WITH_EDITOR
