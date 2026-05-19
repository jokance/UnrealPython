#include "Modules/ModuleManager.h"
#include "Styling/AppStyle.h"
#include "ToolMenus.h"
#include "Framework/Commands/UIAction.h"
#include "UPyCodeGenerator.h"

#define LOCTEXT_NAMESPACE "FUnrealPythonEditorModule"

class FUnrealPythonEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterMenus();
	void OnGeneratePythonClicked() const;

	FDelegateHandle MenuExtensionHandle;
};

IMPLEMENT_MODULE(FUnrealPythonEditorModule, UnrealPythonEditor)

void FUnrealPythonEditorModule::StartupModule()
{
	MenuExtensionHandle = UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FUnrealPythonEditorModule::RegisterMenus));
}

void FUnrealPythonEditorModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(MenuExtensionHandle);

	UToolMenus::UnregisterOwner(this);
}

void FUnrealPythonEditorModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
		if (Menu)
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("UnrealPython");
			Section.Label = LOCTEXT("UnrealPythonLabel", "Unreal Python");
			Section.AddEntry(
				FToolMenuEntry::InitMenuEntry(
					"GeneratePython",
					LOCTEXT("GeneratePython_Label", "Generate Python"),
					LOCTEXT("GeneratePython_Tooltip", "Generate Python scripts."),
					FSlateIcon(FAppStyle::GetAppStyleSetName(), "DeveloperTools.MenuIcon"),
					FUIAction(FExecuteAction::CreateRaw(this, &FUnrealPythonEditorModule::OnGeneratePythonClicked))
				)
			);
		}
	}
}

void FUnrealPythonEditorModule::OnGeneratePythonClicked() const
{
	FUPyCodeGenerator::Get().GenerateAllCode();
}

#undef LOCTEXT_NAMESPACE
