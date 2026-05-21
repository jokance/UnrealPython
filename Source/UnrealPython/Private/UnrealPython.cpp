
#include "UnrealPython.h"
#include "UPyManager.h"
#include "Core/UPyModuleInitializer.h"
#if WITH_EDITOR
#include "ToolMenus.h"
#include "Core/UPyGIL.h"
#include "Core/UPyCommandExecutor.h"
#include "Features/IModularFeatures.h"
#endif

#define LOCTEXT_NAMESPACE "FUnrealPythonModule"

FOnInitializePythonWrappers FUnrealPythonModule::OnInitializePythonWrappers;

void FUnrealPythonModule::StartupModule()
{
	UUPyManager* PyManager = UUPyManager::Get();
	PyManager->Initialize();
	
#if WITH_EDITOR
	CmdExec = new FUnrealPythonCommandExecutor();
	IModularFeatures::Get().RegisterModularFeature(IConsoleCommandExecutor::ModularFeatureName(), CmdExec);

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([]()
	{
		UToolMenus::Get()->RegisterStringCommandHandler("UnrealPython", FToolMenuExecuteString::CreateLambda([](const FString& InString, const FToolMenuContext& InContext) {
			FUPyScopedGIL GIL;
			PyRun_SimpleString(TCHAR_TO_UTF8(*InString));
		}));
	}));
#endif
}

void FUnrealPythonModule::ShutdownModule()
{
	UUPyManager* PyManager = UUPyManager::Get();
	PyManager->Shutdown();
	
#if WITH_EDITOR
	if (CmdExec)
	{
		IModularFeatures::Get().UnregisterModularFeature(IConsoleCommandExecutor::ModularFeatureName(), CmdExec);
		delete CmdExec;
		CmdExec = nullptr;
	}

	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
	if (UToolMenus* ToolMenus = UToolMenus::TryGet())
	{
		ToolMenus->UnregisterStringCommandHandler("UnrealPython");
	}
#endif
}

FUnrealPythonModule& FUnrealPythonModule::Get()
{
	return FModuleManager::LoadModuleChecked<FUnrealPythonModule>("UnrealPython");
}

FOnInitializePythonWrappers& FUnrealPythonModule::GetOnInitializePythonWrappers()
{
	return OnInitializePythonWrappers;
}

void* FUnrealPythonModule::GetPyUEModule()
{
	return FUPyModuleInitializer::Get().GetPyUEModule();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FUnrealPythonModule, UnrealPython)