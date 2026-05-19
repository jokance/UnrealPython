using System.Collections.Generic;
using System.Text.Json.Serialization;

namespace UnrealPythonUhtGenerator;

internal sealed class PropJsonInfo
{
	public string PropName { get; set; } = string.Empty;
	public string RawPropName { get; set; } = string.Empty;
	public string PropDoc { get; set; } = string.Empty;
	public string PropType { get; set; } = string.Empty;
	public string? TypeName { get; set; }
	public ulong PropFlags { get; set; }
	public string? DefaultValue { get; set; }
}

internal sealed class MethodJsonInfo
{
	public string MethodName { get; set; } = string.Empty;
	public string RawMethodName { get; set; } = string.Empty;
	public string MethodDoc { get; set; } = string.Empty;
	public bool bIsConstant { get; set; }
	public bool bIsOperator { get; set; }
	public bool bIsScriptMethod { get; set; }
	public bool bUseHelperMethod { get; set; }
	public uint MethodFlags { get; set; }
	[JsonIgnore(Condition = JsonIgnoreCondition.WhenWritingNull)]
	public string? HostHelper { get; set; }
	public List<PropJsonInfo> Params { get; } = new();
}

internal sealed class ClassJsonInfo
{
	public string ClassName { get; set; } = string.Empty;
	public string EngineName { get; set; } = string.Empty;
	public string NativeTypeName { get; set; } = string.Empty;
	public string BaseType { get; set; } = string.Empty;
	public string ClassDoc { get; set; } = string.Empty;
	public string PackageName { get; set; } = string.Empty;
	public string ModuleName { get; set; } = string.Empty;
	public string HeaderFile { get; set; } = string.Empty;
	public uint ClassFlags { get; set; }
	public int InheritanceDepth { get; set; }
	public List<PropJsonInfo> ClassProps { get; } = new();
	public List<MethodJsonInfo> ClassMethods { get; } = new();
	public List<InterfaceJsonInfo> ImplementedInterfaces { get; } = new();
}

internal sealed class StructJsonInfo
{
	public string StructName { get; set; } = string.Empty;
	public string EngineName { get; set; } = string.Empty;
	public string NativeTypeName { get; set; } = string.Empty;
	public string BaseType { get; set; } = string.Empty;
	public string StructDoc { get; set; } = string.Empty;
	public string PackageName { get; set; } = string.Empty;
	public string ModuleName { get; set; } = string.Empty;
	public string HeaderFile { get; set; } = string.Empty;
	public uint StructFlags { get; set; }
	public int InheritanceDepth { get; set; }
	public MethodJsonInfo MakeFunc { get; } = new();
	public MethodJsonInfo BreakFunc { get; } = new();
	public List<PropJsonInfo> StructProps { get; } = new();
	public List<MethodJsonInfo> StructMethods { get; } = new();
}

internal sealed class EnumEntryJsonInfo
{
	public string EntryName { get; set; } = string.Empty;
	public string EntryDoc { get; set; } = string.Empty;
	public long EntryValue { get; set; }
}

internal sealed class EnumJsonInfo
{
	public string EnumName { get; set; } = string.Empty;
	public string NativeTypeName { get; set; } = string.Empty;
	public string HeaderFile { get; set; } = string.Empty;
	public string EnumDoc { get; set; } = string.Empty;
	public List<EnumEntryJsonInfo> EnumEntries { get; } = new();
}

internal sealed class InterfaceJsonInfo
{
	public string InterfaceName { get; set; } = string.Empty;
	public string InterfaceRawName { get; set; } = string.Empty;
	public string InterfaceDoc { get; set; } = string.Empty;
}

internal sealed class RootJson
{
	public List<ClassJsonInfo> Classes { get; } = new();
	public List<StructJsonInfo> Structs { get; } = new();
	public List<EnumJsonInfo> Enums { get; } = new();
	public List<NativeModuleJsonInfo> NativeModules { get; } = new();
}

internal sealed class NativeModuleJsonInfo
{
	public string ModuleName { get; set; } = string.Empty;
	public List<NativeModuleMethodJsonInfo> NativeMethods { get; } = new();
}

internal sealed class NativeModuleMethodJsonInfo
{
	public string MethodName { get; set; } = string.Empty;
	public string? MethodDoc { get; set; }
	public long PyMethodFlags { get; set; }
}
