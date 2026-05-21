
using System.IO;
using UnrealBuildTool;

public class UnrealPython : ModuleRules
{
	private const string PythonVersion = "python314";

	public UnrealPython(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDefinitions.AddRange([
			"_Py_USE_GCC_BUILTIN_ATOMICS=0",
			"__STDC_VERSION__=201112L"
		]);

		PublicDependencyModuleNames.AddRange(
			[
				"Core"
			]
		);

		PrivateDependencyModuleNames.AddRange(
			[
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"DeveloperSettings",
				"Projects",
				"InputCore",
				"NetCore",
				"UMG",
				"PhysicsCore",
				"FieldNotification",
			]
		);

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(
			[
				"EditorSubsystem",
				"BlueprintGraph",
				"UnrealEd",
				"ToolMenus"
			]);

			PublicDefinitions.Add("WITH_UPY_DOC_STRINGS=1");
		}

		string ThirdPartyPath = Path.GetFullPath($"{ModuleDirectory}/../../ThirdParty");

		ConfigurePythonDependencies(Target, ThirdPartyPath);
	}

	private void ConfigurePythonDependencies(ReadOnlyTargetRules Target, string ThirdPartyPath)
	{
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			ConfigureWin64Python(Target, ThirdPartyPath);
		}

		else if (Target.Platform == UnrealTargetPlatform.Android)
		{
			ConfigureAndroidPython(Target, ThirdPartyPath);
		}
		else if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			ConfigureMacPython(Target, ThirdPartyPath);
		}
		else if (Target.Platform == UnrealTargetPlatform.IOS)
		{
			ConfigureIOSPython(Target, ThirdPartyPath);
		}
		else
		{
			throw new System.Exception("Unsupported platform: " + Target.Platform);
		}
	}

	private void ConfigureWin64Python(ReadOnlyTargetRules Target, string ThirdPartyPath)
	{
		PublicIncludePaths.Add(Path.Combine(ThirdPartyPath, PythonVersion, "include"));

		if (Target.LinkType == TargetLinkType.Modular)
		{
			PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, PythonVersion, "Win64", "libs", $"{PythonVersion}.lib"));

			string DllOutputDir = Target.bBuildEditor ? "$(BinaryOutputDir)" : "$(TargetOutputDir)";
			RuntimeDependencies.Add(Path.Combine(DllOutputDir, $"{PythonVersion}.dll"),
				Path.Combine(ThirdPartyPath, PythonVersion, "Win64", $"{PythonVersion}.dll"));
		}
		else
		{
			PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, PythonVersion, "Win64", "libs", $"{PythonVersion}.lib"));
		}
	}

	private void ConfigureAndroidPython(ReadOnlyTargetRules Target, string ThirdPartyPath)
	{
		PublicIncludePaths.Add(Path.Combine(ThirdPartyPath, PythonVersion, "android", "aarch64-linux-android", "include"));
		PublicIncludePaths.Add(Path.Combine(ThirdPartyPath, PythonVersion, "android", "aarch64-linux-android", "include", "python3.14"));

		PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, PythonVersion, "android", "aarch64-linux-android", "lib", "libpython3.14.a"));
		PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, PythonVersion, "android", "aarch64-linux-android", "lib", "libssl.a"));
		PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, PythonVersion, "android", "aarch64-linux-android", "lib", "libcrypto.a"));
		PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, PythonVersion, "android", "aarch64-linux-android", "lib", "libbz2.a"));
		PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, PythonVersion, "android", "aarch64-linux-android", "lib", "libffi.a"));
		PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, PythonVersion, "android", "aarch64-linux-android", "lib", "liblzma.a"));
		PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, PythonVersion, "android", "aarch64-linux-android", "lib", "libzstd.a"));
		PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, PythonVersion, "android", "aarch64-linux-android", "lib", "libmpdec.a"));
		
	}

	private void ConfigureMacPython(ReadOnlyTargetRules Target, string ThirdPartyPath)
	{
		// PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, PythonVersion, "Mac", "libs", $"{PythonVersion}.lib"));
	}

	private void ConfigureIOSPython(ReadOnlyTargetRules Target, string ThirdPartyPath)
	{
		PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, PythonVersion, "IOS", "libs", $"{PythonVersion}.lib"));
	}
}
