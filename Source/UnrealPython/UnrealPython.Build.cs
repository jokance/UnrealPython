using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Security.Cryptography;
using System.Linq;
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
		string PluginRoot = Path.GetFullPath(Path.Combine(ModuleDirectory, "..", ".."));
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

		// CPython's Android bootstrap files must be readable from a real filesystem.
		// Store them in an APK asset zip and extract them before Py_Initialize instead of staging loose NonUFS files into OBB data.
		string BootstrapArchive = BuildAndroidBootstrapArchive(Target, PluginRoot, AndroidHost, AndroidSupportModule, AndroidStdLibDir);
		string BootstrapVersion = GetFileSha256(BootstrapArchive);
		PublicDefinitions.Add($"UPY_PYTHON_BOOTSTRAP_VERSION=\"{BootstrapVersion}\"");
		AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(PluginRoot, "UnrealPython_UPL.xml"));
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

	private string BuildAndroidBootstrapArchive(ReadOnlyTargetRules Target, string PluginRoot, string AndroidHost, string AndroidSupportModule, string AndroidStdLibDir)
	{
		string OutputDir = Path.Combine(PluginRoot, "Intermediate", "Android");
		Directory.CreateDirectory(OutputDir);

		string OutputArchive = Path.Combine(OutputDir, "unrealpython_bootstrap.zip");
		if (File.Exists(OutputArchive))
		{
			File.Delete(OutputArchive);
		}

		string StagingDir = Path.Combine(OutputDir, "BootstrapStaging");
		if (Directory.Exists(StagingDir))
		{
			Directory.Delete(StagingDir, true);
		}
		Directory.CreateDirectory(StagingDir);

		HashSet<string> Entries = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
		CopyFileToBootstrap(StagingDir, AndroidSupportModule, Path.Combine("android", Path.GetFileName(AndroidSupportModule)), Entries);
		CopyDirectoryToBootstrap(StagingDir, AndroidStdLibDir, Path.Combine("android", AndroidHost, "lib", PythonStdLibVersion), Entries);

		string ProjectDirectory = Target.ProjectFile?.Directory.FullName;
		if (!String.IsNullOrEmpty(ProjectDirectory))
		{
			string ProjectScriptsDir = Path.Combine(ProjectDirectory, "Content", "Scripts");
			if (Directory.Exists(ProjectScriptsDir))
			{
				CopyDirectoryToBootstrap(StagingDir, ProjectScriptsDir, Path.Combine("Content", "Scripts"), Entries);
			}
		}

		CreateZipFromDirectory(StagingDir, OutputArchive);
		return OutputArchive;
	}

	private static void CopyDirectoryToBootstrap(string StagingDir, string SourceDirectory, string EntryRoot, HashSet<string> Entries)
	{
		foreach (string SourceFile in Directory.EnumerateFiles(SourceDirectory, "*", SearchOption.AllDirectories))
		{
			FileAttributes Attributes = File.GetAttributes(SourceFile);
			if ((Attributes & FileAttributes.ReparsePoint) != 0)
			{
				continue;
			}

			string FileName = Path.GetFileName(SourceFile);
			if (FileName.EndsWith(".pyc", StringComparison.OrdinalIgnoreCase) || FileName.EndsWith(".pyo", StringComparison.OrdinalIgnoreCase))
			{
				continue;
			}

			string RelativePath = Path.GetRelativePath(SourceDirectory, SourceFile);
			if (RelativePath.Split(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar).Contains("__pycache__"))
			{
				continue;
			}

			CopyFileToBootstrap(StagingDir, SourceFile, Path.Combine(EntryRoot, RelativePath), Entries);
		}
	}

	private static void CopyFileToBootstrap(string StagingDir, string SourceFile, string EntryPath, HashSet<string> Entries)
	{
		string NormalizedEntryPath = EntryPath.Replace('\\', '/');
		if (!Entries.Add(NormalizedEntryPath))
		{
			return;
		}

		string DestinationFile = Path.Combine(StagingDir, NormalizedEntryPath.Replace('/', Path.DirectorySeparatorChar));
		string DestinationDirectory = Path.GetDirectoryName(DestinationFile);
		if (!String.IsNullOrEmpty(DestinationDirectory))
		{
			Directory.CreateDirectory(DestinationDirectory);
		}

		File.Copy(SourceFile, DestinationFile, true);
	}

	private static void CreateZipFromDirectory(string SourceDirectory, string OutputArchive)
	{
		ProcessStartInfo StartInfo = new ProcessStartInfo("tar.exe")
		{
			UseShellExecute = false,
			CreateNoWindow = true,
			RedirectStandardOutput = true,
			RedirectStandardError = true
		};
		StartInfo.ArgumentList.Add("-a");
		StartInfo.ArgumentList.Add("-cf");
		StartInfo.ArgumentList.Add(OutputArchive);
		StartInfo.ArgumentList.Add("-C");
		StartInfo.ArgumentList.Add(SourceDirectory);
		StartInfo.ArgumentList.Add(".");

		using Process ArchiveProcess = Process.Start(StartInfo);
		if (ArchiveProcess == null)
		{
			throw new BuildException("Failed to start tar.exe for UnrealPython Android bootstrap archive creation.");
		}

		string StandardOutput = ArchiveProcess.StandardOutput.ReadToEnd();
		string StandardError = ArchiveProcess.StandardError.ReadToEnd();
		ArchiveProcess.WaitForExit();

		if (ArchiveProcess.ExitCode != 0 || !File.Exists(OutputArchive))
		{
			throw new BuildException($"Failed to create UnrealPython Android bootstrap archive: {StandardOutput}\n{StandardError}");
		}
	}

	private static string GetFileSha256(string FilePath)
	{
		using FileStream Stream = File.OpenRead(FilePath);
		byte[] Hash = SHA256.HashData(Stream);
		return Convert.ToHexString(Hash).ToLowerInvariant();
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
