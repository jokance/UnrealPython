
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

	/** Absolute paths of the existing project script directories: the main script path and the Additional Paths setting. */
	TArray<FString> GetScriptDirectories() const;

private:
	FUPyVirtualMachine() = default;
	~FUPyVirtualMachine() = default;

	bool ConfigurePython(PyConfig& PythonConfig);

	void ConfigureSearchPaths(PyConfig& PythonConfig);

	FString GetPythonScriptPath() const;

	void SetPythonSysArgv();

	TArray<FString> ParseCommandLineArgs(const TCHAR* InCommandLine) const;
	
	bool bIsInitialized = false;

	PyThreadState* PyMainThreadState = nullptr;
};
