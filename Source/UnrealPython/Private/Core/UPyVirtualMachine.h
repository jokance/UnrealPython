// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UPyCommon.h"

class FUPyVirtualMachine
{
public:
	static FUPyVirtualMachine& Get();

	bool InitializePython();

	void ShutdownPython();

	bool IsInitialized() const { return bIsInitialized; }

	void SaveThread();
	void RestoreThread();

private:
	FUPyVirtualMachine() = default;
	~FUPyVirtualMachine() = default;

	void ConfigurePython(PyConfig& PythonConfig);

	void ConfigureSearchPaths(PyConfig& PythonConfig);

	FString GetPythonScriptPath();

	void SetPythonSysArgv();

	TArray<FString> ParseCommandLineArgs(const TCHAR* InCommandLine) const;
	
	bool bIsInitialized = false;

	PyThreadState* PyMainThreadState = nullptr;
};
