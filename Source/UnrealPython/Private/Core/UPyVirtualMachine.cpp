
#include "UPyVirtualMachine.h"
#include "UPySettings.h"
#include "HAL/PlatformProcess.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/PackageName.h"

FUPyVirtualMachine& FUPyVirtualMachine::Get()
{
	static FUPyVirtualMachine PyVMInst;
	return PyVMInst;
}

bool FUPyVirtualMachine::InitializePython()
{
	if (bIsInitialized)
	{
		return true;
	}

	if (Py_IsInitialized())
	{
		UE_LOG(LogUnrealPython, Error, TEXT("Py_IsInitialized()"));
		return false;
	}

	PyConfig PythonConfig;
	ConfigurePython(PythonConfig);

	PyStatus Status = Py_InitializeFromConfig(&PythonConfig);
	if (PyStatus_Exception(Status))
	{
		UE_LOG(LogUnrealPython, Error, TEXT("Py_InitializeFromConfig() failed: %s"), Status.err_msg ? UTF8_TO_TCHAR(Status.err_msg) : TEXT("Unknown error"));
		PyConfig_Clear(&PythonConfig);
		return false;
	}
	PyConfig_Clear(&PythonConfig);

	if (!Py_IsInitialized())
	{
		UE_LOG(LogUnrealPython, Error, TEXT("Py_IsInitialized() failed"));
		return false;
	}

	SetPythonSysArgv();

	bIsInitialized = true;

	UE_LOG(LogUnrealPython, Log, TEXT("Python VM init success!"));
	return true;
}

void FUPyVirtualMachine::ShutdownPython()
{
	if (!bIsInitialized)
	{
		return;
	}

	if (Py_IsInitialized())
	{
		Py_Finalize();
	}

	bIsInitialized = false;
}

void FUPyVirtualMachine::SaveThread()
{
	// Release the GIL taken by Py_Initialize now that initialization has finished, to allow other threads access to Python
	// We have to take this again prior to calling Py_Finalize, and all other code will lock on-demand via FPyScopedGIL
	PyMainThreadState = PyEval_SaveThread();
}

void FUPyVirtualMachine::RestoreThread()
{
	PyEval_RestoreThread(PyMainThreadState);
	PyMainThreadState = nullptr;
}

void FUPyVirtualMachine::ConfigurePython(PyConfig& PythonConfig)
{
	// Check if the interpreter is should run in isolation mode.
	int IsolatedInterpreterFlag = 1; // PythonPluginSettings->bIsolateInterpreterEnvironment ? 1 : 0;

	// Pre-initialize python with utf-8 encoding and possibly isolated mode
	PyPreConfig PreConfig;
	PyPreConfig_InitIsolatedConfig(&PreConfig);

	PreConfig.parse_argv = 0;
	PreConfig.utf8_mode = 1;
	PreConfig.isolated = IsolatedInterpreterFlag;
	PreConfig.use_environment = !IsolatedInterpreterFlag;

	if (PyStatus Status = Py_PreInitialize(&PreConfig);
		!ensure(!PyStatus_Exception(Status)))
	{
		UE_LOG(LogUnrealPython, Error, TEXT("Py_PreInitialize failed! Error: '%hs'. Python will be disabled."), Status.err_msg)
		return;
	}

	// Create empty init config
	PyConfig_InitIsolatedConfig(&PythonConfig);
	PythonConfig.use_environment = !IsolatedInterpreterFlag;
	PythonConfig.isolated = IsolatedInterpreterFlag;
	PythonConfig.module_search_paths_set = 1;
	// PythonConfig.stdio_encoding = Utf8String.GetData();

	ConfigureSearchPaths( PythonConfig);
}

void FUPyVirtualMachine::ConfigureSearchPaths(PyConfig& PythonConfig)
{
	const UUPySettings* PySettings = GetDefault<UUPySettings>();

	const FString ScriptPath = GetPythonScriptPath();

#if PLATFORM_MAC
	if (TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("UnrealPython")))
	{
		const FString PluginBaseDir = Plugin->GetBaseDir();
		const TArray<FString> BundledPythonPaths = {
			FPaths::Combine(PluginBaseDir, TEXT("ThirdParty/python314/Mac/lib/python3.14")),
			FPaths::Combine(PluginBaseDir, TEXT("ThirdParty/python314/Mac/lib/python3.14/lib-dynload")),
			FPaths::Combine(PluginBaseDir, TEXT("Binaries/Mac/python3.14")),
			FPaths::Combine(PluginBaseDir, TEXT("Binaries/Mac/python3.14/lib-dynload"))
		};

		for (const FString& BundledPythonPath : BundledPythonPaths)
		{
			if (FPaths::DirectoryExists(BundledPythonPath))
			{
				PyWideStringList_Append(&PythonConfig.module_search_paths, TCHAR_TO_WCHAR(*FPaths::ConvertRelativePathToFull(BundledPythonPath)));
			}
		}
	}
#endif

#if PLATFORM_IOS
	const FString IOSBundleDir = FPlatformProcess::BaseDir();
	const FString IOSPythonHome = FPaths::Combine(IOSBundleDir, TEXT("python"));
	const TArray<FString> BundledPythonPaths = {
		FPaths::Combine(IOSPythonHome, TEXT("lib/python3.14")),
		FPaths::Combine(IOSPythonHome, TEXT("lib/python3.14/lib-dynload"))
	};

	if (FPaths::DirectoryExists(IOSPythonHome))
	{
		PyConfig_SetString(&PythonConfig, &PythonConfig.home, TCHAR_TO_WCHAR(*FPaths::ConvertRelativePathToFull(IOSPythonHome)));
	}

	for (const FString& BundledPythonPath : BundledPythonPaths)
	{
		if (FPaths::DirectoryExists(BundledPythonPath))
		{
			PyWideStringList_Append(&PythonConfig.module_search_paths, TCHAR_TO_WCHAR(*FPaths::ConvertRelativePathToFull(BundledPythonPath)));
		}
	}
#endif

	if (!ScriptPath.IsEmpty())
	{
		PyWideStringList_Append(&PythonConfig.module_search_paths, TCHAR_TO_WCHAR(*ScriptPath));
	}

	for (const FDirectoryPath& AdditionalPath : PySettings->AdditionalPaths)
	{
		FString PathToAdd = FPaths::Combine(FPaths::ProjectDir(), AdditionalPath.Path);
		PyWideStringList_Append(&PythonConfig.module_search_paths, TCHAR_TO_WCHAR(*FPaths::ConvertRelativePathToFull(PathToAdd)));

		FString AbsoluteScriptDir = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*PathToAdd);
		PyWideStringList_Append(&PythonConfig.module_search_paths, TCHAR_TO_WCHAR(*AbsoluteScriptDir));
	}




	UE_LOG(LogUnrealPython, Log, TEXT("ConfigureSearchPaths--------: %s"), *FPaths::ProjectUserDir());
}

FString FUPyVirtualMachine::GetPythonScriptPath()
{
	FString OverridePath;
	if (GConfig && GConfig->GetString(TEXT("UnrealPython"), TEXT("ScriptPath"), OverridePath, GGameIni))
	{
		return FPaths::ConvertRelativePathToFull(FPaths::Combine(*FPaths::ProjectDir(), OverridePath));
	}

	return TEXT(""); // FPaths::ConvertRelativePathToFull(FPaths::Combine(*FPaths::ProjectContentDir(), TEXT("Scripts")));
}

void FUPyVirtualMachine::SetPythonSysArgv()
{
	PyObject* SysModule = PyImport_ImportModule("sys");
	if (!SysModule)
	{
		PyErr_Print();
		return;
	}

	const TCHAR* CommandLine = FCommandLine::GetOriginal();
	TArray<FString> Args = ParseCommandLineArgs(CommandLine);
	if (Args.Num() <= 0)
	{
		Py_DECREF(SysModule);
		return;
	}

	PyObject* ListArgs = PyList_New(Args.Num());
	if (!ListArgs)
	{
		Py_DECREF(SysModule);
		return;
	}

	for (int32 i = 0; i < Args.Num(); i++)
	{
		PyObject* PyArg = PyUnicode_FromKindAndData((int)sizeof(TCHAR), *Args[i], Args[i].Len());
		PyList_SetItem(ListArgs, i, PyArg);
	}
	PyObject_SetAttrString(SysModule, "argv", ListArgs);
	Py_DECREF(ListArgs);
	Py_DECREF(SysModule);
}

TArray<FString> FUPyVirtualMachine::ParseCommandLineArgs(const TCHAR* InCommandLine) const
{
	TArray<FString> ParsedArgs;
	for (;;)
	{
		FString Arg = FParse::Token(InCommandLine, 0);
		if (Arg.Len() <= 0)
		{
			break;
		}
		ParsedArgs.Add(Arg);
	}
	return ParsedArgs;
}
