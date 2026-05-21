
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
		string MacPythonPath = Path.Combine(ThirdPartyPath, PythonVersion, "Mac");
		string MacLibPath = Path.Combine(MacPythonPath, "lib");
		string PythonDylibPath = Path.Combine(MacLibPath, "libpython3.14.dylib");
		string PythonStdLibPath = Path.Combine(MacLibPath, "python3.14");

		PublicIncludePaths.Add(Path.Combine(MacPythonPath, "include"));
		PublicAdditionalLibraries.Add(PythonDylibPath);
		PublicSystemLibraries.Add("dl");
		PublicFrameworks.Add("CoreFoundation");

		string RuntimeOutputDir = Target.bBuildEditor ? "$(BinaryOutputDir)" : "$(TargetOutputDir)";
		RuntimeDependencies.Add(Path.Combine(RuntimeOutputDir, "libpython3.14.dylib"), PythonDylibPath);

		foreach (string RuntimeFile in Directory.EnumerateFiles(PythonStdLibPath, "*", SearchOption.AllDirectories))
		{
			FileAttributes RuntimeFileAttributes = File.GetAttributes(RuntimeFile);
			if (RuntimeFile.EndsWith(".pyc") || (RuntimeFileAttributes & FileAttributes.ReparsePoint) != 0)
			{
				continue;
			}

			string RelativePath = Path.GetRelativePath(PythonStdLibPath, RuntimeFile);
			RuntimeDependencies.Add(Path.Combine(RuntimeOutputDir, "python3.14", RelativePath), RuntimeFile);
		}
	}

	private void ConfigureIOSPython(ReadOnlyTargetRules Target, string ThirdPartyPath)
	{
		string IOSPythonPath = Path.Combine(ThirdPartyPath, PythonVersion, "IOS");
		string PythonXCFrameworkPath = Path.Combine(IOSPythonPath, "Python.xcframework");
		string PythonXCFrameworkRelativePath = Path.Combine("..", "..", "ThirdParty", PythonVersion, "IOS", "Python.xcframework");
		string PythonHeadersPath = Path.Combine(PythonXCFrameworkPath, "ios-arm64_x86_64-simulator", "Python.framework", "Headers");
		string PythonRuntimePath = Path.Combine(IOSPythonPath, "Runtime");
		string PlatformRuntimeName = Target.Architecture == UnrealArch.IOSSimulator ? "IOSSimulator" : "IOSDevice";

		PublicIncludePaths.Add(PythonHeadersPath);
		PublicAdditionalFrameworks.Add(new Framework("Python", PythonXCFrameworkRelativePath, Framework.FrameworkMode.LinkAndCopy));
		PublicFrameworks.Add("CoreFoundation");
		PublicSystemLibraries.Add("dl");

		AdditionalBundleResources.Add(new BundleResource(Path.Combine(PythonRuntimePath, PlatformRuntimeName, "python")));
		AdditionalBundleResources.Add(new BundleResource(Path.Combine(PythonRuntimePath, PlatformRuntimeName, "Frameworks")));
	}
}
