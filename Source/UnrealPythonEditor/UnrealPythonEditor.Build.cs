
using UnrealBuildTool;

public class UnrealPythonEditor : ModuleRules
{
    public UnrealPythonEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new[]
            {
                "Core"
            });

        PrivateDependencyModuleNames.AddRange(
            new[]
            {
                "CoreUObject",
                "Engine",
                "UnrealEd",
                "ToolMenus",
                "Slate",
                "SlateCore",
                "LevelEditor",
                "Projects",
                "UnrealPython",
                "Json",
                "JsonUtilities",
            });
    }
}
