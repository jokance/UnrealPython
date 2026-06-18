
#include "UPyVirtualMachine.h"
#include "UPySettings.h"
#include "HAL/PlatformProcess.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/App.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/PackageName.h"

#if PLATFORM_IOS
#include <CoreFoundation/CoreFoundation.h>
#endif

#if PLATFORM_ANDROID
#include "Android/AndroidJavaEnv.h"
#endif

namespace
{
FString GetBundledPythonVersionName()
{
	return FString(UTF8_TO_TCHAR(UPY_PYTHON_VERSION));
}

FString GetPythonStdLibVersionName()
{
	return FString(UTF8_TO_TCHAR(UPY_PYTHON_STDLIB_VERSION));
}

FString GetBundledPythonZipFileName()
{
	return GetBundledPythonVersionName() + TEXT(".zip");
}

#if PLATFORM_WINDOWS
bool AddPythonZipPathIfExists(PyConfig& PythonConfig, const FString& PythonZipPath)
{
	if (!FPaths::FileExists(PythonZipPath))
	{
		return false;
	}

	const FString FullPythonZipPath = FPaths::ConvertRelativePathToFull(PythonZipPath);
	PyWideStringList_Append(&PythonConfig.module_search_paths, TCHAR_TO_WCHAR(*FullPythonZipPath));
	UE_LOG(LogUnrealPython, Log, TEXT("Added bundled Windows Python path: %s"), *FullPythonZipPath);
	return true;
}
#endif

#if PLATFORM_IOS
FString GetFileSystemPathFromCFURL(CFURLRef URL)
{
	if (URL == nullptr)
	{
		return FString();
	}

	CFStringRef PathString = CFURLCopyFileSystemPath(URL, kCFURLPOSIXPathStyle);
	if (PathString == nullptr)
	{
		return FString();
	}

	char PathBuffer[4096] = {};
	const bool bConverted = CFStringGetCString(PathString, PathBuffer, sizeof(PathBuffer), kCFStringEncodingUTF8);
	CFRelease(PathString);

	return bConverted ? FString(UTF8_TO_TCHAR(PathBuffer)) : FString();
}

void AddIOSBundlePythonHomeCandidates(TArray<FString>& OutCandidates)
{
	CFBundleRef MainBundle = CFBundleGetMainBundle();
	if (MainBundle == nullptr)
	{
		return;
	}

	if (CFURLRef ResourcesURL = CFBundleCopyResourcesDirectoryURL(MainBundle))
	{
		OutCandidates.Add(FPaths::Combine(GetFileSystemPathFromCFURL(ResourcesURL), TEXT("python")));
		CFRelease(ResourcesURL);
	}

	if (CFURLRef BundleURL = CFBundleCopyBundleURL(MainBundle))
	{
		OutCandidates.Add(FPaths::Combine(GetFileSystemPathFromCFURL(BundleURL), TEXT("python")));
		CFRelease(BundleURL);
	}
}
#endif

#if PLATFORM_ANDROID
#if USE_ANDROID_JNI
FString GetJavaFileAbsolutePath(JNIEnv* Env, jobject FileObject)
{
	if (Env == nullptr || FileObject == nullptr)
	{
		return FString();
	}

	jclass FileClass = Env->GetObjectClass(FileObject);
	if (FileClass == nullptr || AndroidJavaEnv::CheckJavaException())
	{
		return FString();
	}

	jmethodID GetAbsolutePathMethod = Env->GetMethodID(FileClass, "getAbsolutePath", "()Ljava/lang/String;");
	Env->DeleteLocalRef(FileClass);
	if (GetAbsolutePathMethod == nullptr || AndroidJavaEnv::CheckJavaException())
	{
		return FString();
	}

	jstring PathString = static_cast<jstring>(Env->CallObjectMethod(FileObject, GetAbsolutePathMethod));
	if (PathString == nullptr || AndroidJavaEnv::CheckJavaException())
	{
		return FString();
	}

	return FJavaHelper::FStringFromLocalRef(Env, PathString);
}
#endif

TArray<FString> GetAndroidObbDirsFromJava()
{
	TArray<FString> ObbDirs;

#if USE_ANDROID_JNI
	JNIEnv* Env = AndroidJavaEnv::GetJavaEnv();
	jobject Activity = AndroidJavaEnv::GetGameActivityThis();
	if (Env == nullptr || Activity == nullptr)
	{
		return ObbDirs;
	}

	jclass ActivityClass = Env->GetObjectClass(Activity);
	if (ActivityClass == nullptr || AndroidJavaEnv::CheckJavaException())
	{
		return ObbDirs;
	}

	jmethodID GetObbDirsMethod = Env->GetMethodID(ActivityClass, "getObbDirs", "()[Ljava/io/File;");
	if (GetObbDirsMethod != nullptr && !AndroidJavaEnv::CheckJavaException())
	{
		jobjectArray ObbDirArray = static_cast<jobjectArray>(Env->CallObjectMethod(Activity, GetObbDirsMethod));
		if (ObbDirArray != nullptr && !AndroidJavaEnv::CheckJavaException())
		{
			const jsize ObbDirCount = Env->GetArrayLength(ObbDirArray);
			for (jsize Index = 0; Index < ObbDirCount; ++Index)
			{
				jobject ObbDirObject = Env->GetObjectArrayElement(ObbDirArray, Index);
				if (ObbDirObject != nullptr && !AndroidJavaEnv::CheckJavaException())
				{
					const FString ObbDirPath = GetJavaFileAbsolutePath(Env, ObbDirObject);
					if (!ObbDirPath.IsEmpty())
					{
						ObbDirs.AddUnique(ObbDirPath);
					}

					Env->DeleteLocalRef(ObbDirObject);
				}
				else
				{
					AndroidJavaEnv::CheckJavaException();
				}
			}

			Env->DeleteLocalRef(ObbDirArray);
		}
		else
		{
			AndroidJavaEnv::CheckJavaException();
		}
	}
	else
	{
		AndroidJavaEnv::CheckJavaException();
	}

	if (ObbDirs.IsEmpty())
	{
		jmethodID GetObbDirMethod = Env->GetMethodID(ActivityClass, "getObbDir", "()Ljava/io/File;");
		if (GetObbDirMethod != nullptr && !AndroidJavaEnv::CheckJavaException())
		{
			jobject ObbDirObject = Env->CallObjectMethod(Activity, GetObbDirMethod);
			if (ObbDirObject != nullptr && !AndroidJavaEnv::CheckJavaException())
			{
				const FString ObbDirPath = GetJavaFileAbsolutePath(Env, ObbDirObject);
				if (!ObbDirPath.IsEmpty())
				{
					ObbDirs.AddUnique(ObbDirPath);
				}

				Env->DeleteLocalRef(ObbDirObject);
			}
			else
			{
				AndroidJavaEnv::CheckJavaException();
			}
		}
		else
		{
			AndroidJavaEnv::CheckJavaException();
		}
	}

	Env->DeleteLocalRef(ActivityClass);
#endif

	return ObbDirs;
}

FString GetAndroidPackageName()
{
	FString PackageName = TEXT("com.YourCompany.[PROJECT]");
	if (GConfig)
	{
		GConfig->GetString(TEXT("/Script/AndroidRuntimeSettings.AndroidRuntimeSettings"), TEXT("PackageName"), PackageName, GEngineIni);
	}

	PackageName.ReplaceInline(TEXT("[PROJECT]"), FApp::GetProjectName());
	return PackageName;
}

int32 GetAndroidStoreVersion()
{
	int32 StoreVersion = 1;
	if (GConfig)
	{
		GConfig->GetInt(TEXT("/Script/AndroidRuntimeSettings.AndroidRuntimeSettings"), TEXT("StoreVersion"), StoreVersion, GEngineIni);
	}

	return StoreVersion;
}

FString GetAndroidPythonHost()
{
#if PLATFORM_ANDROID_ARM64
	return TEXT("aarch64-linux-android");
#elif defined(__x86_64__)
	return TEXT("x86_64-linux-android");
#else
	return FString();
#endif
}

FString GetAndroidPythonBootstrapRoot()
{
	const FString BootstrapRoot = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("UnrealPythonBootstrap"));
	return IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(*BootstrapRoot);
}

FString GetAndroidPythonBootstrapVersion()
{
#ifdef UPY_PYTHON_BOOTSTRAP_VERSION
	return FString(UTF8_TO_TCHAR(UPY_PYTHON_BOOTSTRAP_VERSION));
#else
	return TEXT("unknown");
#endif
}

bool ExtractAndroidPythonBootstrap(const FString& DestinationPath)
{
#if USE_ANDROID_JNI
	JNIEnv* Env = AndroidJavaEnv::GetJavaEnv();
	jobject Activity = AndroidJavaEnv::GetGameActivityThis();
	if (Env == nullptr || Activity == nullptr)
	{
		UE_LOG(LogUnrealPython, Warning, TEXT("Unable to extract UnrealPython bootstrap: Android activity is unavailable"));
		return false;
	}

	jclass ActivityClass = Env->GetObjectClass(Activity);
	if (ActivityClass == nullptr || AndroidJavaEnv::CheckJavaException())
	{
		UE_LOG(LogUnrealPython, Warning, TEXT("Unable to extract UnrealPython bootstrap: GameActivity class is unavailable"));
		return false;
	}

	jmethodID ExtractMethod = Env->GetMethodID(ActivityClass, "AndroidThunkJava_ExtractUnrealPythonBootstrap", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)Z");
	Env->DeleteLocalRef(ActivityClass);
	if (ExtractMethod == nullptr || AndroidJavaEnv::CheckJavaException())
	{
		UE_LOG(LogUnrealPython, Warning, TEXT("Unable to extract UnrealPython bootstrap: Java extraction method was not found"));
		return false;
	}

	const FString BootstrapVersion = GetAndroidPythonBootstrapVersion();
	jstring AssetNameString = Env->NewStringUTF("unrealpython_bootstrap.zip");
	jstring DestinationPathString = Env->NewStringUTF(TCHAR_TO_UTF8(*DestinationPath));
	jstring VersionString = Env->NewStringUTF(TCHAR_TO_UTF8(*BootstrapVersion));

	const bool bExtracted = Env->CallBooleanMethod(Activity, ExtractMethod, AssetNameString, DestinationPathString, VersionString) == JNI_TRUE;
	const bool bHadException = AndroidJavaEnv::CheckJavaException();

	Env->DeleteLocalRef(AssetNameString);
	Env->DeleteLocalRef(DestinationPathString);
	Env->DeleteLocalRef(VersionString);

	if (bHadException || !bExtracted)
	{
		UE_LOG(LogUnrealPython, Warning, TEXT("Failed to extract UnrealPython bootstrap to: %s"), *DestinationPath);
		return false;
	}

	UE_LOG(LogUnrealPython, Log, TEXT("Extracted UnrealPython bootstrap to: %s"), *DestinationPath);
	return true;
#else
	return false;
#endif
}

void AddAndroidBootstrapPathIfExists(PyConfig& PythonConfig, const FString& Path, const TCHAR* Description)
{
	if (!FPaths::DirectoryExists(Path))
	{
		UE_LOG(LogUnrealPython, Warning, TEXT("Android Python bootstrap %s was not found: %s"), Description, *Path);
		return;
	}

	PyWideStringList_Append(&PythonConfig.module_search_paths, TCHAR_TO_WCHAR(*Path));
	UE_LOG(LogUnrealPython, Log, TEXT("Added Android Python bootstrap %s: %s"), Description, *Path);
}

void AddAndroidBootstrapSearchPaths(PyConfig& PythonConfig)
{
	const FString AndroidPythonHost = GetAndroidPythonHost();
	if (AndroidPythonHost.IsEmpty())
	{
		UE_LOG(LogUnrealPython, Warning, TEXT("Unable to use Android Python bootstrap: unsupported Android architecture"));
		return;
	}

	const FString BootstrapRoot = GetAndroidPythonBootstrapRoot();
	if (!ExtractAndroidPythonBootstrap(BootstrapRoot))
	{
		return;
	}

	const FString AndroidSupportPath = BootstrapRoot / TEXT("android");
	const FString AndroidPythonHome = AndroidSupportPath / AndroidPythonHost;
	const FString StdLibPath = AndroidPythonHome / TEXT("lib") / GetPythonStdLibVersionName();
	const FString ScriptPath = BootstrapRoot / TEXT("Content/Scripts");

	if (FPaths::DirectoryExists(StdLibPath))
	{
		PyConfig_SetString(&PythonConfig, &PythonConfig.home, TCHAR_TO_WCHAR(*AndroidPythonHome));
		PyConfig_SetString(&PythonConfig, &PythonConfig.prefix, TCHAR_TO_WCHAR(*AndroidPythonHome));
		PyConfig_SetString(&PythonConfig, &PythonConfig.exec_prefix, TCHAR_TO_WCHAR(*AndroidPythonHome));
		PyConfig_SetString(&PythonConfig, &PythonConfig.base_prefix, TCHAR_TO_WCHAR(*AndroidPythonHome));
		PyConfig_SetString(&PythonConfig, &PythonConfig.base_exec_prefix, TCHAR_TO_WCHAR(*AndroidPythonHome));
		PyConfig_SetString(&PythonConfig, &PythonConfig.stdlib_dir, TCHAR_TO_WCHAR(*StdLibPath));
	}

	AddAndroidBootstrapPathIfExists(PythonConfig, AndroidSupportPath, TEXT("support path"));
	AddAndroidBootstrapPathIfExists(PythonConfig, StdLibPath, TEXT("stdlib path"));
	AddAndroidBootstrapPathIfExists(PythonConfig, ScriptPath, TEXT("script path"));
}

void AddAndroidObbScriptPaths(PyConfig& PythonConfig)
{
	const FString PackageName = GetAndroidPackageName();
	const int32 StoreVersion = GetAndroidStoreVersion();
	const FString ObbFileName = FString::Printf(TEXT("main.%d.%s.obb"), StoreVersion, *PackageName);
	const FString AndroidPythonHost = GetAndroidPythonHost();
	const TArray<FString> ObbRoots = GetAndroidObbDirsFromJava();

	for (const FString& ObbRoot : ObbRoots)
	{
		const FString ObbPath = FPaths::Combine(ObbRoot, ObbFileName);
		if (FPaths::FileExists(ObbPath))
		{
			const FString ObbScriptPath = ObbPath / FApp::GetProjectName() / TEXT("Content/Scripts");
			PyWideStringList_Append(&PythonConfig.module_search_paths, TCHAR_TO_WCHAR(*ObbScriptPath));
			UE_LOG(LogUnrealPython, Log, TEXT("Added packaged Android OBB script path: %s"), *ObbScriptPath);

			const FString ObbAndroidPythonPath = ObbPath / FApp::GetProjectName() / TEXT("Plugins/UnrealPython/ThirdParty") / GetBundledPythonVersionName() / TEXT("android");
			PyWideStringList_Append(&PythonConfig.module_search_paths, TCHAR_TO_WCHAR(*ObbAndroidPythonPath));
			UE_LOG(LogUnrealPython, Log, TEXT("Added packaged Android Python support path: %s"), *ObbAndroidPythonPath);

			if (!AndroidPythonHost.IsEmpty())
			{
				const FString ObbPythonStdLibPath = ObbAndroidPythonPath / AndroidPythonHost / TEXT("lib") / GetPythonStdLibVersionName();
				PyWideStringList_Append(&PythonConfig.module_search_paths, TCHAR_TO_WCHAR(*ObbPythonStdLibPath));
				UE_LOG(LogUnrealPython, Log, TEXT("Added packaged Android Python stdlib path: %s"), *ObbPythonStdLibPath);
			}
		}
	}
}
#endif
}

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
			FPaths::Combine(PluginBaseDir, TEXT("ThirdParty"), GetBundledPythonVersionName(), TEXT("Mac/lib"), GetPythonStdLibVersionName()),
			FPaths::Combine(PluginBaseDir, TEXT("ThirdParty"), GetBundledPythonVersionName(), TEXT("Mac/lib"), GetPythonStdLibVersionName(), TEXT("lib-dynload")),
			FPaths::Combine(PluginBaseDir, TEXT("Binaries/Mac"), GetPythonStdLibVersionName()),
			FPaths::Combine(PluginBaseDir, TEXT("Binaries/Mac"), GetPythonStdLibVersionName(), TEXT("lib-dynload"))
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

#if PLATFORM_WINDOWS
	TArray<FString> WindowsPythonZipPathCandidates = {
		FPaths::Combine(FPlatformProcess::BaseDir(), GetBundledPythonZipFileName()),
		FPaths::Combine(FPaths::ProjectDir(), TEXT("Binaries/Win64"), GetBundledPythonZipFileName())
	};

	if (TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("UnrealPython")))
	{
		const FString PluginBaseDir = Plugin->GetBaseDir();
		WindowsPythonZipPathCandidates.Add(FPaths::Combine(PluginBaseDir, TEXT("Binaries/Win64"), GetBundledPythonZipFileName()));
		WindowsPythonZipPathCandidates.Add(FPaths::Combine(PluginBaseDir, TEXT("ThirdParty"), GetBundledPythonVersionName(), TEXT("Win64"), GetBundledPythonZipFileName()));
	}

	bool bAddedWindowsPythonZipPath = false;
	for (const FString& WindowsPythonZipPath : WindowsPythonZipPathCandidates)
	{
		bAddedWindowsPythonZipPath |= AddPythonZipPathIfExists(PythonConfig, WindowsPythonZipPath);
	}

	if (!bAddedWindowsPythonZipPath)
	{
		UE_LOG(LogUnrealPython, Warning, TEXT("Bundled Windows Python standard library was not found; expected %s beside the editor/game executable or in the UnrealPython plugin runtime directories."), *GetBundledPythonZipFileName());
	}
#endif

#if PLATFORM_IOS
	TArray<FString> IOSPythonHomeCandidates = {
		FPaths::Combine(FPlatformProcess::BaseDir(), TEXT("python")),
		FPaths::Combine(FPaths::ProjectDir(), TEXT("python")),
		FPaths::Combine(FPaths::ProjectContentDir(), TEXT("../python"))
	};
	AddIOSBundlePythonHomeCandidates(IOSPythonHomeCandidates);

	FString IOSPythonHome;
	for (const FString& IOSPythonHomeCandidate : IOSPythonHomeCandidates)
	{
		const FString FullPythonHomeCandidate = FPaths::ConvertRelativePathToFull(IOSPythonHomeCandidate);
		if (FPaths::DirectoryExists(FullPythonHomeCandidate) || FullPythonHomeCandidate.Contains(TEXT(".app/python")))
		{
			IOSPythonHome = FullPythonHomeCandidate;
			break;
		}
	}

	if (!IOSPythonHome.IsEmpty())
	{
		const FString IOSStdLibPath = FPaths::Combine(IOSPythonHome, TEXT("lib"), GetPythonStdLibVersionName());
		const TArray<FString> BundledPythonPaths = {
			IOSStdLibPath,
			FPaths::Combine(IOSStdLibPath, TEXT("lib-dynload"))
		};

		PyConfig_SetString(&PythonConfig, &PythonConfig.home, TCHAR_TO_WCHAR(*IOSPythonHome));
		PyConfig_SetString(&PythonConfig, &PythonConfig.prefix, TCHAR_TO_WCHAR(*IOSPythonHome));
		PyConfig_SetString(&PythonConfig, &PythonConfig.exec_prefix, TCHAR_TO_WCHAR(*IOSPythonHome));
		PyConfig_SetString(&PythonConfig, &PythonConfig.base_prefix, TCHAR_TO_WCHAR(*IOSPythonHome));
		PyConfig_SetString(&PythonConfig, &PythonConfig.base_exec_prefix, TCHAR_TO_WCHAR(*IOSPythonHome));
		PyConfig_SetString(&PythonConfig, &PythonConfig.stdlib_dir, TCHAR_TO_WCHAR(*IOSStdLibPath));

		for (const FString& BundledPythonPath : BundledPythonPaths)
		{
			PyWideStringList_Append(&PythonConfig.module_search_paths, TCHAR_TO_WCHAR(*BundledPythonPath));
			UE_LOG(LogUnrealPython, Log, TEXT("Added bundled iOS Python path: %s"), *BundledPythonPath);
		}
	}
	else
	{
		UE_LOG(LogUnrealPython, Warning, TEXT("Bundled iOS Python runtime was not found"));
	}
#endif

	if (!ScriptPath.IsEmpty())
	{
		PyWideStringList_Append(&PythonConfig.module_search_paths, TCHAR_TO_WCHAR(*ScriptPath));
	}

#if PLATFORM_IOS || PLATFORM_ANDROID
	const FString PackagedScriptPath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Scripts")));
	PyWideStringList_Append(&PythonConfig.module_search_paths, TCHAR_TO_WCHAR(*PackagedScriptPath));
	UE_LOG(LogUnrealPython, Log, TEXT("Added packaged script path: %s"), *PackagedScriptPath);
#endif

#if PLATFORM_ANDROID
	AddAndroidBootstrapSearchPaths(PythonConfig);
	AddAndroidObbScriptPaths(PythonConfig);
#endif

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

	return FPaths::ConvertRelativePathToFull(FPaths::Combine(*FPaths::ProjectContentDir(), TEXT("Scripts")));
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
