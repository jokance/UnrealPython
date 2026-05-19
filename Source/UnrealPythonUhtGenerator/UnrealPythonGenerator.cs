using EpicGames.UHT.Tables;
using EpicGames.UHT.Utils;
using System;
using System.Text;

namespace UnrealPythonUhtGenerator;

[UnrealHeaderTool]
internal static class UnrealPythonGenerator
{
	private const string PluginName = "UnrealPython";

	[UhtExporter(Name = "UnrealPythonGenerator", Description = "Generates UnrealPython reflection metadata", Options = UhtExporterOptions.Default, ModuleName = PluginName)]
	private static void Export(IUhtExportFactory factory)
	{
		if (string.IsNullOrEmpty(Environment.GetEnvironmentVariable("GENERATE_UNREAL_PYTHON")))
		{
			return;
		}

		Console.OutputEncoding = Encoding.UTF8;
		Console.WriteLine("Generating UnrealPython reflection data...");

		var generator = new UnrealPythonCodeGenerator(factory);
		generator.Generate();
		
		Console.WriteLine("Generating UnrealPython finished.");
	}
}
