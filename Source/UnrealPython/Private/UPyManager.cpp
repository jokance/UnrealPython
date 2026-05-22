
#include "UPyManager.h"

#include "UPyCommon.h"
#include "Core/UPyVirtualMachine.h"
#include "Core/UPyModuleInitializer.h"
#include "Core/UPyGIL.h"
#include "Wrapper/UPyWrapperObjectBase.h"
#include "Wrapper/UPyWrapperTypeFactory.h"
#include "HAL/PlatformFileManager.h"

UUPyManager* UUPyManager::Get()
{
	static UUPyManager* PyManagerInst = nullptr;
	if (!PyManagerInst)
	{
		PyManagerInst = NewObject<UUPyManager>(GetTransientPackage(), TEXT("UPyManager"), RF_Public | RF_MarkAsRootSet);
	}
	return PyManagerInst;
}

void UUPyManager::Initialize()
{
	GUObjectArray.AddUObjectDeleteListener(this);
#if WITH_EDITOR
	FCoreUObjectDelegates::OnObjectsReplaced.AddUObject(this, &UUPyManager::OnObjectsReplaced);
#endif

	CheckPyScript();

	if (!FUPyVirtualMachine::Get().InitializePython())
	{
		return;
	}

	FUPyModuleInitializer::Get().InitializeModules();

	RedirectOutput();

	PyRun_SimpleString("import ue_site; ue_site.on_init()");

	FUPyVirtualMachine::Get().SaveThread();

#if WITH_EDITOR
	RegisterTicker();
#endif
}

void UUPyManager::Shutdown()
{
	GUObjectArray.RemoveUObjectDeleteListener(this);

#if WITH_EDITOR
	FCoreUObjectDelegates::OnObjectsReplaced.RemoveAll(this);
#endif

	if (!FUPyVirtualMachine::Get().IsInitialized())
	{
		return;
	}

	FUPyVirtualMachine::Get().RestoreThread();

	PyRun_SimpleString("import ue_site; ue_site.on_shutdown()");

	FUPyModuleInitializer::Get().CleanupModules();

	FUPyVirtualMachine::Get().ShutdownPython();
}

void UUPyManager::BeginDestroy()
{
	Super::BeginDestroy();
	Shutdown();
}

void UUPyManager::RedirectOutput()
{
	const char* PyScript =
"import sys\n"
"import ue\n"
"\n"
"class UnrealOutputRedirector:\n"
"    def __init__(self, log_func):\n"
"        self.log_func = log_func\n"
"        self.buffer = \"\"\n"
"\n"
"    def write(self, message):\n"
"        if message == '\\n':\n"
"            self.flush()\n"
"        elif message.endswith('\\n'):\n"
"            self.buffer += message[:-1]\n"
"            self.flush()\n"
"        else:\n"
"            self.buffer += message\n"
"\n"
"    def flush(self):\n"
"        if self.buffer:\n"
"            self.log_func(self.buffer)\n"
"            self.buffer = \"\"\n"
"\n"
"sys.stdout = UnrealOutputRedirector(ue.Log)\n"
"sys.stderr = UnrealOutputRedirector(ue.LogError)\n";

	if (PyRun_SimpleString(PyScript) != 0)
	{
		UE_LOG(LogUnrealPython, Error, TEXT("Failed to redirect Python output."));
	}
}

void UUPyManager::AddPythonOwnedObject(FUPyWrapperObjectBase* InSelf)
{
	if (FUPyWrapperObjectBase::ValidateInternalState(InSelf))
	{
		PythonOwnedObjects.Add(InSelf->ObjectInstance);
	}
}

void UUPyManager::RemovePythonOwnedObject(FUPyWrapperObjectBase* InSelf)
{
	if (InSelf->ObjectInstance)
	{
		PythonOwnedObjects.Remove(InSelf->ObjectInstance);
	}
}

bool UUPyManager::IsPythonOwnedObject(FUPyWrapperObjectBase* InSelf) const
{
	return InSelf->ObjectInstance && PythonOwnedObjects.Contains(InSelf->ObjectInstance);
}

void UUPyManager::NotifyUObjectDeleted(const UObjectBase* ObjectBase, int32 Index)
{
	UObject* Object = (UObject*)ObjectBase;

	if (!FUPyWrapperObjectFactory::Get().ContainsInstance(Object))
	{
		return;
	}

	FUPyScopedGIL GIL;

	FUPyWrapperObjectFactory::Get().RemoveOwnedPyProps(Object);

	// FUPyWrapperObjectBase* PyObj = FUPyWrapperObjectFactory::Get().FindInstance(Object);
	// Py_XDECREF(PyObj);
	FUPyWrapperObjectFactory::Get().UnmapInstance(Object);
}

#if WITH_EDITOR
void UUPyManager::OnObjectsReplaced(const TMap<UObject*, UObject*>& ReplacementMap)
{
	FUPyWrapperObjectFactory::Get().OnObjectsReplaced(ReplacementMap);
}
#endif

void UUPyManager::RegisterTicker()
{
	if (TickerHandle.IsValid())
	{
		return;
	}

	TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UUPyManager::Tick),
		0.0f // Tick every frame
	);
}

void UUPyManager::UnregisterTicker()
{
	if (TickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		TickerHandle.Reset();
	}
}

bool UUPyManager::Tick(float DeltaTime)
{
	FUPyScopedGIL GIL;
	PyRun_SimpleString("import ue_site; ue_site.on_tick()");
	return true;
}

void UUPyManager::CheckPyScript()
{
#if !WITH_EDITOR
	UE_LOG(LogTemp, Warning, TEXT("begin CheckPyScript"));
	CopyAllAssetsToExternal("Scripts");
	CopyAllAssetsToExternal("Settings");
	UE_LOG(LogTemp, Warning, TEXT("end CheckPyScript"));
#endif
}

void UUPyManager::CopyAllAssetsToExternal(const FString& InDirectory)
{
	//extern FString GExternalFilePath;

	FString SourceDirectory = FPaths::ProjectContentDir() / InDirectory;
	//FString DestDirectory = GExternalFilePath / InDirectory;
	FString DestDirectory = IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(*SourceDirectory);

	FPaths::NormalizeDirectoryName(DestDirectory);
	FPaths::NormalizeDirectoryName(SourceDirectory);

	UE_LOG(LogTemp, Warning, TEXT("CopyAllAssetsToExternal source: %s,dest:%s"), *SourceDirectory, *DestDirectory);

	bool bOverwqriteAllExisting = true;

	IPlatformFile& TmpPlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	if (!TmpPlatformFile.DirectoryExists(*SourceDirectory))
	{
		UE_LOG(LogTemp, Warning, TEXT("CopyAllAssetsToExternal fail DirectoryExists, %s"), *SourceDirectory);
		return;
	}

	if (TmpPlatformFile.DirectoryExists(*DestDirectory))
	{
		//UE_LOG(LogTemp, Warning, TEXT("CopyAllAssetsToExternal fail DirectoryExists, %s"), *DestDirectory);
		//return;
	}

	if (!TmpPlatformFile.CreateDirectoryTree(*DestDirectory))
	{
		UE_LOG(LogTemp, Warning, TEXT("CopyAllAssetsToExternal CreateDirectory fail, %s"), *DestDirectory);
		return;
	}

	// Copy all files and directories
	struct FCopyFilesAndDirs : public IPlatformFile::FDirectoryVisitor
	{
		IPlatformFile& PlatformFile;
		bool bOverwrite;
		FString SourcePathHead;
		FString DestPathHead;

		FCopyFilesAndDirs(IPlatformFile& InPlatformFile, bool bInOverwrite)
			: PlatformFile(InPlatformFile)
			, bOverwrite(bInOverwrite)
		{
			SourcePathHead = FApp::GetProjectName() / FString(TEXT("Content"));
			//DestPathHead = GExternalFilePath;
			DestPathHead = IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(*SourcePathHead);
		}

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory)
		{
			FString NewName(FilenameOrDirectory);
//			while (NewName.StartsWith(TEXT("../")))
//			{
//				NewName = NewName.RightChop(3);
//			}
			// change the root
			FString _SourcePathHead = FPaths::ProjectContentDir();
			// DestPathHead = GExternalFilePath;
			FString _DestPathHead = IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(*_SourcePathHead);
			// replace insternal path to external path, SourcePathHead such as G114/Content
			NewName = NewName.Replace(*_SourcePathHead, *_DestPathHead);

			///UE_LOG(LogTemp, Log, TEXT("FCopyFilesAndDirs SourceRoot: %s  ,DestRoot: %s,file:%s"), *SourcePathHead, *DestPathHead, FilenameOrDirectory);
			UE_LOG(LogTemp, Log, TEXT("FCopyFilesAndDirs NewName: %s  ,FilenameOrDirectory: %s"), *NewName,  FilenameOrDirectory);

			if (bIsDirectory)
			{
				// create new directory structure
				if (!PlatformFile.CreateDirectoryTree(*NewName) && !PlatformFile.DirectoryExists(*NewName))
				{
					UE_LOG(LogTemp, Warning, TEXT("CreateDirectoryTree fail, %s"), *NewName);
					// return false;
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("CreateDirectoryTree success, %s"), *NewName);
				}
			}
			else
			{
				// Delete destination file if it exists and we are overwriting
				//if (PlatformFile.FileExists(*NewName) && bOverwrite)
				//{
				//	PlatformFile.DeleteFile(*NewName);
				//}

				PlatformFile.CreateDirectoryTree(*FPaths::GetPath(NewName));
				// Copy file from source
				if (!PlatformFile.CopyFile(*NewName, FilenameOrDirectory))
				{

					// Not all files could be copied
					UE_LOG(LogTemp, Warning, TEXT("CopyFile fail, %s  , %s"), *NewName, FilenameOrDirectory);

					// return false;
				}
				// UE_LOG(LogTemp,Log , TEXT("CopyFile success, %s  , %s"), *NewName, FilenameOrDirectory);
			}
			return true; // continue searching
		}
	};

	// copy files and directories visitor
	FCopyFilesAndDirs CopyFilesAndDirs(TmpPlatformFile, true);

	// create all files subdirectories and files in subdirectories!
	TmpPlatformFile.IterateDirectoryRecursively(*SourceDirectory, CopyFilesAndDirs);

}
