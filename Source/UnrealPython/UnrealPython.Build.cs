
using System.IO;
using EpicGames.Core;
using UnrealBuildTool;

public class UnrealPython : ModuleRules
{
	private const string PythonVersion = "python314";
	private const string PythonStdLibVersion = "python3.14";

	public UnrealPython(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDefinitions.AddRange([
			"_Py_USE_GCC_BUILTIN_ATOMICS=0",
			"__STDC_VERSION__=201112L",
			$"UPY_PYTHON_VERSION=\"{PythonVersion}\"",
			$"UPY_PYTHON_STDLIB_VERSION=\"{PythonStdLibVersion}\""
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

		PublicAdditionalLibraries.Add(Path.Combine(ThirdPartyPath, PythonVersion, "Win64", "libs", $"{PythonVersion}.lib"));

		string DllOutputDir = Target.bBuildEditor ? "$(BinaryOutputDir)" : "$(TargetOutputDir)";
		RuntimeDependencies.Add(Path.Combine(DllOutputDir, $"{PythonVersion}.dll"),
			Path.Combine(ThirdPartyPath, PythonVersion, "Win64", $"{PythonVersion}.dll"));
		RuntimeDependencies.Add(Path.Combine(DllOutputDir, $"{PythonVersion}.zip"),
			Path.Combine(ThirdPartyPath, PythonVersion, "Win64", $"{PythonVersion}.zip"));
	}

	private void ConfigureAndroidPython(ReadOnlyTargetRules Target, string ThirdPartyPath)
	{
		string AndroidHost = GetAndroidPythonHost(Target);
		string AndroidPythonPath = Path.Combine(ThirdPartyPath, PythonVersion, "android", AndroidHost);
		if (!Directory.Exists(AndroidPythonPath))
		{
			throw new BuildException($"Missing UnrealPython Android Python runtime for {AndroidHost}: {AndroidPythonPath}");
		}

		PublicIncludePaths.Add(Path.Combine(AndroidPythonPath, "include"));
		PublicIncludePaths.Add(Path.Combine(AndroidPythonPath, "include", PythonStdLibVersion));

		AddAndroidPythonLibrary(AndroidPythonPath, "libUnrealPython3.14.a");
		AddAndroidPythonLibrary(AndroidPythonPath, "libUnrealPythonSSL.a");
		AddAndroidPythonLibrary(AndroidPythonPath, "libUnrealPythonCrypto.a");
		AddAndroidPythonLibrary(AndroidPythonPath, "libbz2.a");
		AddAndroidPythonLibrary(AndroidPythonPath, "libffi.a");
		AddAndroidPythonLibrary(AndroidPythonPath, "liblzma.a");
		AddAndroidPythonLibrary(AndroidPythonPath, "libzstd.a");
		AddAndroidPythonLibrary(AndroidPythonPath, "libmpdec.a");

		string AndroidSupportModule = Path.Combine(ThirdPartyPath, PythonVersion, "android", "_android_support.py");
		string AndroidStdLibDir = Path.Combine(AndroidPythonPath, "lib", PythonStdLibVersion);
		if (!File.Exists(AndroidSupportModule))
		{
			throw new BuildException($"Missing UnrealPython Android Python support module: {AndroidSupportModule}");
		}
		if (!Directory.Exists(AndroidStdLibDir))
		{
			throw new BuildException($"Missing UnrealPython Android Python stdlib directory for {AndroidHost}: {AndroidStdLibDir}");
		}

		RuntimeDependencies.Add(AndroidSupportModule, StagedFileType.NonUFS);
		RuntimeDependencies.Add(Path.Combine(AndroidStdLibDir, "..."), StagedFileType.NonUFS);
	}

	private string GetAndroidPythonHost(ReadOnlyTargetRules Target)
	{
		string ArchitectureName = Target.Architecture.ToString().ToLowerInvariant();
		if (ArchitectureName.Contains("arm64") || ArchitectureName.Contains("aarch64"))
		{
			return "aarch64-linux-android";
		}

		if (ArchitectureName.Contains("x64") || ArchitectureName.Contains("x86_64"))
		{
			return "x86_64-linux-android";
		}

		if (ArchitectureName.Contains("x86") || ArchitectureName.Contains("armv7") || ArchitectureName.Contains("armv"))
		{
			throw new BuildException($"UnrealPython has no Android Python runtime for architecture '{Target.Architecture}'. Build and add that host first.");
		}

		return "aarch64-linux-android";
	}

	private void AddAndroidPythonLibrary(string AndroidPythonPath, string LibraryName)
	{
		string LibraryPath = Path.Combine(AndroidPythonPath, "lib", LibraryName);
		if (!File.Exists(LibraryPath))
		{
			throw new BuildException($"Missing UnrealPython Android Python library: {LibraryPath}");
		}

		PublicAdditionalLibraries.Add(LibraryPath);
	}

	private void ConfigureMacPython(ReadOnlyTargetRules Target, string ThirdPartyPath)
	{
		string MacPythonPath = Path.Combine(ThirdPartyPath, PythonVersion, "Mac");
		string MacLibPath = Path.Combine(MacPythonPath, "lib");
		string PythonDylibPath = Path.Combine(MacLibPath, $"lib{PythonStdLibVersion}.dylib");
		string PythonStdLibPath = Path.Combine(MacLibPath, PythonStdLibVersion);

		PublicIncludePaths.Add(Path.Combine(MacPythonPath, "include"));
		PublicAdditionalLibraries.Add(PythonDylibPath);
		PublicSystemLibraries.Add("dl");
		PublicFrameworks.Add("CoreFoundation");

		string RuntimeOutputDir = Target.bBuildEditor ? "$(BinaryOutputDir)" : "$(TargetOutputDir)";
		RuntimeDependencies.Add(Path.Combine(RuntimeOutputDir, $"lib{PythonStdLibVersion}.dylib"), PythonDylibPath);

		foreach (string RuntimeFile in Directory.EnumerateFiles(PythonStdLibPath, "*", SearchOption.AllDirectories))
		{
			FileAttributes RuntimeFileAttributes = File.GetAttributes(RuntimeFile);
			if (RuntimeFile.EndsWith(".pyc") || (RuntimeFileAttributes & FileAttributes.ReparsePoint) != 0)
			{
				continue;
			}

			string RelativePath = Path.GetRelativePath(PythonStdLibPath, RuntimeFile);
			RuntimeDependencies.Add(Path.Combine(RuntimeOutputDir, PythonStdLibVersion, RelativePath), RuntimeFile);
		}
	}

	private void ConfigureIOSPython(ReadOnlyTargetRules Target, string ThirdPartyPath)
	{
		string IOSPythonPath = Path.Combine(ThirdPartyPath, PythonVersion, "IOS");
		string PythonXCFrameworkPath = Path.Combine(IOSPythonPath, "Python.xcframework");
		string PythonXCFrameworkRelativePath = Path.Combine("..", "..", "ThirdParty", PythonVersion, "IOS", "Python.xcframework");
		string PythonHeadersPath = Path.Combine(PythonXCFrameworkPath, "ios-arm64_x86_64-simulator", "Python.framework", "Headers");
		string PythonRuntimePath = Path.Combine(IOSPythonPath, "Runtime");
		string PlatformRuntimeName = ShouldUseIOSSimulatorRuntime(Target) ? "IOSSimulator" : "IOSDevice";

		PublicIncludePaths.Add(PythonHeadersPath);
		PublicAdditionalFrameworks.Add(new Framework("Python", PythonXCFrameworkRelativePath, Framework.FrameworkMode.LinkAndCopy));
		PublicFrameworks.Add("CoreFoundation");
		PublicSystemLibraries.Add("dl");

		AdditionalBundleResources.Add(new BundleResource(Path.Combine(PythonRuntimePath, PlatformRuntimeName, "python")));
		AdditionalBundleResources.Add(new BundleResource(Path.Combine(PythonRuntimePath, PlatformRuntimeName, "Frameworks")));
	}

	private bool ShouldUseIOSSimulatorRuntime(ReadOnlyTargetRules Target)
	{
		if (Target.Architecture == UnrealArch.IOSSimulator)
		{
			return true;
		}

		if (Target.IntermediateEnvironment == UnrealIntermediateEnvironment.GenerateProjectFiles)
		{
			ConfigHierarchy PlatformEngineConfig = ConfigCache.ReadHierarchy(ConfigHierarchyType.Engine, Target.ProjectFile?.Directory, UnrealTargetPlatform.IOS);
			if (PlatformEngineConfig.GetBool("/Script/IOSRuntimeSettings.IOSRuntimeSettings", "bEnableSimulatorSupport", out bool bEnableSimulatorSupport) && bEnableSimulatorSupport)
			{
				return true;
			}
		}

		return false;
	}
}
