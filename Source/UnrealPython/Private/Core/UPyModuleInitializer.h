
#pragma once

#include "UPyCommon.h"
#include "Core/UPyPtr.h"
#include "UnrealPython.h"

class FUPyModuleInitializer
{
public:
	static FUPyModuleInitializer& Get();

	bool InitializeModules();

	void CleanupModules();

	PyObject* FindOrAddPyTypeByName(const FString& AttrName);

	PyObject* GetPyUEModule()
	{
		return PyUEModule;
	}
	
private:
	FUPyModuleInitializer() = default;
	~FUPyModuleInitializer() = default;

	bool InitializeMainModule();
	bool InitializeUEModule();
	void InitializeCoreModule();

	void ConfigureModuleGlobals();
	
private:
	FUPyObjectPtr PyUEModule;
	FUPyObjectPtr PyUEDict;
	FUPyObjectPtr PyMainModule;
	FUPyObjectPtr PyMainDict;

	bool bIsInitialized = false;
};
