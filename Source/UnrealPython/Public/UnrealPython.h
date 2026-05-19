// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

DECLARE_MULTICAST_DELEGATE(FOnInitializePythonWrappers);

class UNREALPYTHON_API FUnrealPythonModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static FUnrealPythonModule& Get();

	static FOnInitializePythonWrappers OnInitializePythonWrappers;

	static FOnInitializePythonWrappers& GetOnInitializePythonWrappers();

	void* GetPyUEModule();

#if WITH_EDITOR
private:
	class IConsoleCommandExecutor* CmdExec = nullptr;
#endif
};
