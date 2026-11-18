
#include "UPyManager.h"

#include "UPyCommon.h"
#include "Core/UPyVirtualMachine.h"
#include "Core/UPyModuleInitializer.h"
#include "Core/UPyGIL.h"
#include "Wrapper/UPyWrapperObjectBase.h"
#include "Wrapper/UPyReferenceCollector.h"
#include "Wrapper/UPyWrapperTypeFactory.h"
#include "PyGameFramework/UPyTimerManager.h"
#if WITH_EDITOR
#include "Core/UPyHotReload.h"
#endif

void FUPyPythonOwnedObjectReferencer::Add(UObject* Object)
{
	if (Object)
	{
		Objects.Add(Object);
	}
}

void FUPyPythonOwnedObjectReferencer::Remove(UObject* Object)
{
	if (Object)
	{
		Objects.Remove(Object);
	}
}

bool FUPyPythonOwnedObjectReferencer::Contains(UObject* Object) const
{
	return Object && Objects.Contains(Object);
}

void FUPyPythonOwnedObjectReferencer::Reset()
{
	Objects.Reset();
}

#if WITH_EDITOR
void FUPyPythonOwnedObjectReferencer::OnObjectsReplaced(const TMap<UObject*, UObject*>& ReplacementMap)
{
	for (const TPair<UObject*, UObject*>& ReplacementPair : ReplacementMap)
	{
		if (Objects.Remove(ReplacementPair.Key) > 0 && ReplacementPair.Value)
		{
			Objects.Add(ReplacementPair.Value);
		}
	}
}
#endif

void FUPyPythonOwnedObjectReferencer::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddStableReferenceSet(&Objects);
}

FString FUPyPythonOwnedObjectReferencer::GetReferencerName() const
{
	return TEXT("FUPyPythonOwnedObjectReferencer");
}

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
	// Re-instancing Python generated classes only broadcasts OnObjectsReplaced through GEditor, which -game processes lack.
	FCoreUObjectDelegates::OnObjectsReinstanced.AddUObject(this, &UUPyManager::OnObjectsReplaced);
#endif

	if (!FUPyVirtualMachine::Get().InitializePython())
	{
		return;
	}

	if (!FUPyModuleInitializer::Get().InitializeModules())
	{
		FUPyVirtualMachine::Get().ShutdownPython();
		return;
	}

	RedirectOutput();

	PyRun_SimpleString("import ue_site; ue_site.on_init()");

	FUPyVirtualMachine::Get().SaveThread();

	RegisterTicker();

#if WITH_EDITOR
	FUPyHotReload::Get().Startup(FUPyVirtualMachine::Get().GetScriptDirectories());
#endif
}

void UUPyManager::Shutdown()
{
	GUObjectArray.RemoveUObjectDeleteListener(this);

#if WITH_EDITOR
	FCoreUObjectDelegates::OnObjectsReplaced.RemoveAll(this);
	FCoreUObjectDelegates::OnObjectsReinstanced.RemoveAll(this);
#endif

	if (!FUPyVirtualMachine::Get().IsInitialized())
	{
		UUPyTimerManager::Shutdown();
		return;
	}

#if WITH_EDITOR
	FUPyHotReload::Get().Shutdown();
#endif

	UnregisterTicker();

	FUPyVirtualMachine::Get().RestoreThread();

	PyRun_SimpleString("import ue_site; ue_site.on_shutdown()");
	UUPyTimerManager::Shutdown();

	FUPyModuleInitializer::Get().CleanupModules();
	FUPyReferenceCollector::Get().Shutdown();

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

bool UUPyManager::AddPythonOwnedObject(FUPyWrapperObjectBase* InSelf)
{
	if (FUPyWrapperObjectBase::ValidateInternalState(InSelf))
	{
		PythonOwnedObjects.Add(InSelf->ObjectInstance);
		return true;
	}
	return false;
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
	PythonOwnedObjects.Remove(Object);

	if (FUPyWrapperObjectBase* PyObj = FUPyWrapperObjectFactory::Get().FindInstance(Object))
	{
		PyObj->ObjectInstance = nullptr;
	}
	FUPyWrapperObjectFactory::Get().UnmapInstance(Object);
}

#if WITH_EDITOR
void UUPyManager::OnObjectsReplaced(const TMap<UObject*, UObject*>& ReplacementMap)
{
	if (!Py_IsInitialized())
	{
		return;
	}

	FUPyScopedGIL GIL;
	PythonOwnedObjects.OnObjectsReplaced(ReplacementMap);
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
#if WITH_EDITOR
	FUPyHotReload::Get().Tick(DeltaTime);
#endif

	FUPyScopedGIL GIL;
	PyRun_SimpleString("import ue_site; ue_site.on_tick()");
	return true;
}
