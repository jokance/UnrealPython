using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Text.Json;
using EpicGames.Core;
using EpicGames.UHT.Types;
using EpicGames.UHT.Utils;

namespace UnrealPythonUhtGenerator;

internal sealed class GenerationConfig
{
	public string? StubFilePath { get; set; }
	public string? GameExportPath { get; set; }
	public string? GameIncludePath { get; set; }
	public HashSet<string> ExportToGameModules { get; } = new(StringComparer.Ordinal);
	public Dictionary<string, AutoGenClassConfig> Classes { get; } = new(StringComparer.Ordinal);
	public Dictionary<string, AutoGenStructConfig> Structs { get; } = new(StringComparer.Ordinal);
}

internal sealed class AutoGenClassConfig
{
	public HashSet<string>? IncludeProperties;
	public HashSet<string>? ExcludeProperties;
	public HashSet<string>? ForceReflectionProperties;
	public HashSet<string>? IncludeMethods;
	public HashSet<string>? ExcludeMethods;
	public HashSet<string>? ForceReflectionMethods;
}

internal sealed class AutoGenStructConfig
{
	public bool IsInline;
	public HashSet<string>? IncludeProperties;
	public HashSet<string>? ExcludeProperties;
	public HashSet<string>? ForceReflectionProperties;
	public HashSet<string>? IncludeMethods;
	public HashSet<string>? ExcludeMethods;
	public HashSet<string>? ForceReflectionMethods;
}

internal static class AutoWrapperGenerator
{
	private static readonly Dictionary<string, string> TemplateCache = new(StringComparer.Ordinal);
	private static Dictionary<string, StructJsonInfo> StructLookupCache = new(StringComparer.Ordinal);
	private static Dictionary<string, ClassJsonInfo> ClassLookupCache = new(StringComparer.Ordinal);
	private static Dictionary<string, EnumJsonInfo> EnumLookupCache = new(StringComparer.Ordinal);

	private static string GetEnumStaticType(string? typeName)
	{
		if (string.IsNullOrEmpty(typeName))
		{
			return "uint8";
		}

		if (EnumLookupCache.TryGetValue(typeName, out EnumJsonInfo? enumInfo) && !string.IsNullOrEmpty(enumInfo.NativeTypeName))
		{
			return enumInfo.NativeTypeName;
		}

		return typeName;
	}

	private static string? GetHeaderFileForType(string typeName)
	{
		if (ClassLookupCache.TryGetValue(typeName, out ClassJsonInfo? classInfo))
		{
			return classInfo.HeaderFile;
		}
		if (StructLookupCache.TryGetValue(typeName, out StructJsonInfo? structInfo))
		{
			return structInfo.HeaderFile;
		}
		if (EnumLookupCache.TryGetValue(typeName, out EnumJsonInfo? enumInfo))
		{
			return enumInfo.HeaderFile;
		}
		return null;
	}

	private static string GetTemplateContent(string baseDirectory, string templateFileName)
	{
		string pluginRoot = ResolvePluginRootDirectory(baseDirectory);
		string templatePath = Path.Combine(pluginRoot, "Tools", "Template", templateFileName);
		if (!TemplateCache.TryGetValue(templatePath, out string? content))
		{
			content = File.ReadAllText(templatePath);
			TemplateCache[templatePath] = content;
		}
		return content;
	}

	private static string RenderTemplate(string templateContent, Dictionary<string, string> replacements)
	{
		StringBuilder builder = new(templateContent);
		foreach (KeyValuePair<string, string> pair in replacements)
		{
			builder.Replace("{" + pair.Key + "}", pair.Value);
		}

		string result = builder.ToString();
		return Regex.Replace(result, "{[A-Za-z0-9]+}", String.Empty);
	}

	private static List<PropJsonInfo> GetStructProperties(StructWrapperInfo structInfo)
	{
		return structInfo.Info.StructProps
			.Where(prop => ShouldIncludeStructProperty(structInfo.Config, prop))
			.OrderBy(prop => prop.PropName, StringComparer.Ordinal)
			.ToList();
	}

	private static List<MethodJsonInfo> GetStructMethods(StructWrapperInfo structInfo)
	{
		IEnumerable<MethodJsonInfo> methods = structInfo.Info.StructMethods;
		methods = methods.Where(method => ShouldIncludeStructMethod(structInfo.Config, method));
		return methods
			.OrderBy(method => method.MethodName, StringComparer.Ordinal)
			.ToList();
	}

	private static List<PropJsonInfo> GetClassProperties(ClassWrapperInfo classInfo)
	{
		IEnumerable<PropJsonInfo> props = classInfo.Info.ClassProps;
		props = props.Where(prop => ShouldIncludeClassProperty(classInfo.Config, prop));
		return props
			.OrderBy(prop => prop.PropName, StringComparer.Ordinal)
			.ToList();
	}

	private static List<MethodJsonInfo> GetClassMethods(ClassWrapperInfo classInfo)
	{
		IEnumerable<MethodJsonInfo> methods = classInfo.Info.ClassMethods;
		methods = methods.Where(method => ShouldIncludeClassMethod(classInfo.Config, method));
		return methods
			.OrderBy(method => method.MethodName, StringComparer.Ordinal)
			.ToList();
	}

	private static bool ShouldIncludeClassProperty(AutoGenClassConfig config, PropJsonInfo prop)
	{
		if (config.IncludeProperties != null && config.IncludeProperties.Count > 0)
		{
			if (!MatchesConfiguredName(config.IncludeProperties, prop))
			{
				return false;
			}
		}

		if (config.ExcludeProperties != null && MatchesConfiguredName(config.ExcludeProperties, prop))
		{
			return false;
		}

		return true;
	}

	private static bool ShouldIncludeClassMethod(AutoGenClassConfig config, MethodJsonInfo method)
	{
		EFunctionFlags flags = (EFunctionFlags)method.MethodFlags;
		if ((flags & EFunctionFlags.Delegate) != 0 || (flags & EFunctionFlags.MulticastDelegate) != 0)
		{
			return false;
		}

		if (config.IncludeMethods != null && config.IncludeMethods.Count > 0)
		{
			if (!MatchesConfiguredName(config.IncludeMethods, method))
			{
				return false;
			}
		}

		if (config.ExcludeMethods != null && MatchesConfiguredName(config.ExcludeMethods, method))
		{
			return false;
		}

		return true;
	}

	private static bool ShouldIncludeStructProperty(AutoGenStructConfig config, PropJsonInfo prop)
	{
		if (config.IncludeProperties != null && config.IncludeProperties.Count > 0)
		{
			if (!MatchesConfiguredName(config.IncludeProperties, prop))
			{
				return false;
			}
		}

		if (config.ExcludeProperties != null && MatchesConfiguredName(config.ExcludeProperties, prop))
		{
			return false;
		}

		return true;
	}

	private static bool ShouldIncludeStructMethod(AutoGenStructConfig config, MethodJsonInfo method)
	{
		EFunctionFlags flags = (EFunctionFlags)method.MethodFlags;
		if ((flags & EFunctionFlags.Delegate) != 0 || (flags & EFunctionFlags.MulticastDelegate) != 0)
		{
			return false;
		}

		if (config.IncludeMethods != null && config.IncludeMethods.Count > 0)
		{
			if (!MatchesConfiguredName(config.IncludeMethods, method))
			{
				return false;
			}
		}

		if (config.ExcludeMethods != null && MatchesConfiguredName(config.ExcludeMethods, method))
		{
			return false;
		}

		return true;
	}

	internal static bool MatchesConfiguredName(HashSet<string> configuredNames, PropJsonInfo prop)
	{
		if (configuredNames.Contains(prop.PropName) || configuredNames.Contains(prop.RawPropName))
		{
			return true;
		}
		return false;
	}

	internal static bool MatchesConfiguredName(HashSet<string> configuredNames, MethodJsonInfo method)
	{
		if (configuredNames.Contains(method.MethodName) || configuredNames.Contains(method.RawMethodName))
		{
			return true;
		}
		return false;
	}

	private static string EscapeForCxxLiteral(string value)
	{
		return value
			.Replace("\\", "\\\\")
			.Replace("\"", "\\\"")
			.Replace("\r", String.Empty)
			.Replace("\n", "\\n");
	}

	private static string FormatDocString(string? documentation)
	{
		return "nullptr";
	}

	private static bool IsPropertyExternallyAccessible(PropJsonInfo property)
	{
		EPropertyFlags flags = (EPropertyFlags)property.PropFlags;
		const EPropertyFlags AccessMask =
			EPropertyFlags.NativeAccessSpecifierPrivate |
			EPropertyFlags.NativeAccessSpecifierProtected |
			EPropertyFlags.NativeAccessSpecifierPublic;

		EPropertyFlags accessSpecifier = flags & AccessMask;
		if (accessSpecifier.HasFlag(EPropertyFlags.NativeAccessSpecifierPrivate) ||
			accessSpecifier.HasFlag(EPropertyFlags.NativeAccessSpecifierProtected))
		{
			return false;
		}
		return true;
	}

	internal static bool IsEditorOnlyProperty(PropJsonInfo property)
	{
		// CPF_EditorOnly = 0x0000000800000000
		return (property.PropFlags & 0x0000000800000000) != 0;
	}

	internal static bool IsEditorOnlyMethod(MethodJsonInfo method)
	{
		// FUNC_EditorOnly = 0x20000000
		return (method.MethodFlags & 0x20000000) != 0;
	}

	private static bool IsMethodExternallyAccessible(MethodJsonInfo method)
	{
		EFunctionFlags flags = (EFunctionFlags)method.MethodFlags;
		if ((flags & EFunctionFlags.Private) != 0 || (flags & EFunctionFlags.Protected) != 0)
		{
			return false;
		}
		return true;
	}

	private static readonly HashSet<string> IntrinsicPropertyTypes = new(StringComparer.Ordinal)
	{
		"BoolProperty",
		"ByteProperty",
		"Int8Property",
		"Int16Property",
		"IntProperty",
		"Int32Property",
		"Int64Property",
		"UInt16Property",
		"UInt32Property",
		"UInt64Property",
		"FloatProperty",
		"DoubleProperty",
		"LargeWorldCoordinatesRealProperty",
		"NameProperty",
		"StrProperty",
	"TextProperty"
	};

	private sealed class NumberOperatorMapping
	{
		public NumberOperatorMapping(string pyNumberField, string delegateType, bool isInPlace, bool isUnary)
		{
			PyNumberField = pyNumberField;
			DelegateType = delegateType;
			IsInPlace = isInPlace;
			IsUnary = isUnary;
		}

		public string PyNumberField { get; }
		public string DelegateType { get; }
		public bool IsInPlace { get; }
		public bool IsUnary { get; }
	}

	private static readonly Dictionary<string, NumberOperatorMapping> NumberOperatorMappings = new(StringComparer.Ordinal)
	{
		["__add__"] = new NumberOperatorMapping("nb_add", "binaryfunc", false, false),
		["__iadd__"] = new NumberOperatorMapping("nb_inplace_add", "binaryfunc", true, false),
		["__sub__"] = new NumberOperatorMapping("nb_subtract", "binaryfunc", false, false),
		["__isub__"] = new NumberOperatorMapping("nb_inplace_subtract", "binaryfunc", true, false),
		["__mul__"] = new NumberOperatorMapping("nb_multiply", "binaryfunc", false, false),
		["__imul__"] = new NumberOperatorMapping("nb_inplace_multiply", "binaryfunc", true, false),
		["__truediv__"] = new NumberOperatorMapping("nb_true_divide", "binaryfunc", false, false),
		["__itruediv__"] = new NumberOperatorMapping("nb_inplace_true_divide", "binaryfunc", true, false),
		["__mod__"] = new NumberOperatorMapping("nb_remainder", "binaryfunc", false, false),
		["__imod__"] = new NumberOperatorMapping("nb_inplace_remainder", "binaryfunc", true, false),
		["__and__"] = new NumberOperatorMapping("nb_and", "binaryfunc", false, false),
		["__iand__"] = new NumberOperatorMapping("nb_inplace_and", "binaryfunc", true, false),
		["__or__"] = new NumberOperatorMapping("nb_or", "binaryfunc", false, false),
		["__ior__"] = new NumberOperatorMapping("nb_inplace_or", "binaryfunc", true, false),
		["__xor__"] = new NumberOperatorMapping("nb_xor", "binaryfunc", false, false),
		["__ixor__"] = new NumberOperatorMapping("nb_inplace_xor", "binaryfunc", true, false),
		["__lshift__"] = new NumberOperatorMapping("nb_lshift", "binaryfunc", false, false),
		["__ilshift__"] = new NumberOperatorMapping("nb_inplace_lshift", "binaryfunc", true, false),
		["__rshift__"] = new NumberOperatorMapping("nb_rshift", "binaryfunc", false, false),
		["__irshift__"] = new NumberOperatorMapping("nb_inplace_rshift", "binaryfunc", true, false),
		["__neg__"] = new NumberOperatorMapping("nb_negative", "unaryfunc", false, true),
		["__bool__"] = new NumberOperatorMapping("nb_bool", "inquiry", false, true)
	};

	private static string IndentCode(string code, int tabCount)
	{
		string indent = new('\t', tabCount);
		string[] lines = code.Split(new[] { "\r\n", "\n" }, StringSplitOptions.None);
		for (int i = 0; i < lines.Length; ++i)
		{
			if (lines[i].Length > 0)
			{
				lines[i] = indent + lines[i];
			}
			else
			{
				lines[i] = indent;
			}
		}
		return String.Join(Environment.NewLine, lines);
	}

	private static bool TryBuildDirectAccessor(string rawType, PropJsonInfo property, string memberExpression, HashSet<string>? forceReflectionProperties, out string getterBody, out string setterBody)
	{
		getterBody = setterBody = String.Empty;
		string propType = property.PropType;
		string propertyName = property.PropName;

		if (!IsPropertyExternallyAccessible(property))
		{
			return false;
		}

		if (forceReflectionProperties != null && MatchesConfiguredName(forceReflectionProperties, property))
		{
			return false;
		}

		string errorContextLiteral = EscapeForCxxLiteral(BuildPropertyErrorContext(rawType, propertyName));
		string errorMessageLiteral = EscapeForCxxLiteral(GetDirectAccessorErrorMessage(propType));
		string setErrorStatement = $"UPyUtil::SetPythonError(PyExc_TypeError, TEXT(\"{errorContextLiteral}\"), TEXT(\"{errorMessageLiteral}\"));";

		if (propType.Equals("EnumProperty", StringComparison.Ordinal))
		{
			string? enumType = ResolveNativeTypeName(property.TypeName);
			if (String.IsNullOrEmpty(enumType))
			{
				enumType = property.TypeName;
			}

			if (String.IsNullOrEmpty(enumType))
			{
				return false;
			}

			getterBody = $"return UPyConversion::PythonizeEnumEntry((int64){memberExpression}, StaticEnum<{GetEnumStaticType(enumType)}>());";

			setterBody =
				$"if (UPyConversion::NativizeEnumEntry(InValue, StaticEnum<{GetEnumStaticType(enumType)}>(), {memberExpression}))\n" +
				"{\n" +
				"\treturn 0;\n" +
				"}\n" +
				$"{setErrorStatement}\n" +
				"return -1;";
			return true;
		}

		if (propType.Equals("ByteProperty", StringComparison.Ordinal) && !String.IsNullOrEmpty(property.TypeName))
		{
			string? enumType = ResolveNativeTypeName(property.TypeName);
			if (!String.IsNullOrEmpty(enumType))
			{
				getterBody = $"return UPyConversion::PythonizeEnumEntry((int64){memberExpression}, StaticEnum<{GetEnumStaticType(enumType)}>());";

				setterBody =
					$"if (UPyConversion::NativizeEnumEntry(InValue, StaticEnum<{GetEnumStaticType(enumType)}>(), {memberExpression}))\n" +
					"{\n" +
					"\treturn 0;\n" +
					"}\n" +
					$"{setErrorStatement}\n" +
					"return -1;";
				return true;
			}
			return false;
		}

		if (propType.Equals("BoolProperty", StringComparison.Ordinal))
		{
			getterBody = $"return UPyConversion::Pythonize({memberExpression});";

			setterBody =
				$"bool bTemp = false;\n" +
				$"if (UPyConversion::Nativize(InValue, bTemp))\n" +
				"{\n" +
				$"\t{memberExpression} = bTemp;\n" +
				"\treturn 0;\n" +
				"}\n" +
				$"{setErrorStatement}\n" +
				"return -1;";
			return true;
		}

		if (IntrinsicPropertyTypes.Contains(propType))
		{
			getterBody = $"return UPyConversion::Pythonize({memberExpression});";

			setterBody =
				$"if (UPyConversion::Nativize(InValue, {memberExpression}))\n" +
				"{\n" +
				"\treturn 0;\n" +
				"}\n" +
				$"{setErrorStatement}\n" +
				"return -1;";
			return true;
		}

		if (propType.Equals("StructProperty", StringComparison.Ordinal))
		{
			string? nativeType = ResolveNativeTypeName(property.TypeName);
			if (String.IsNullOrEmpty(nativeType))
			{
				nativeType = property.TypeName;
			}

			if (String.IsNullOrEmpty(nativeType))
			{
				return false;
			}

			getterBody = $"return (PyObject*)FUPyWrapperStructFactory::Get().CreateInstance(TBaseStructure<{nativeType}>::Get(), (void*)&{memberExpression}, FUPyWrapperOwnerContext((PyObject*)InSelf), EUPyConversionMethod::Reference);";

			setterBody =
				$"if (UPyConversion::NativizeStructInstance(InValue, {memberExpression}))\n" +
				"{\n" +
				"\treturn 0;\n" +
				"}\n" +
				$"{setErrorStatement}\n" +
				"return -1;";
			return true;
		}

		if (propType.Equals("ObjectProperty", StringComparison.Ordinal) ||
			propType.Equals("SoftObjectProperty", StringComparison.Ordinal) ||
			propType.Equals("ClassProperty", StringComparison.Ordinal) ||
			propType.Equals("SoftClassProperty", StringComparison.Ordinal))
		{
			getterBody = $"return UPyConversion::Pythonize({memberExpression});";

			setterBody =
				$"if (UPyConversion::Nativize(InValue, {memberExpression}))\n" +
				"{\n" +
				"\treturn 0;\n" +
				"}\n" +
				$"{setErrorStatement}\n" +
				"return -1;";
			return true;
		}



		return false;
	}

	private static string BuildPropertyErrorContext(string rawType, string propertyName)
	{
		string typeName = rawType;
		if (!String.IsNullOrEmpty(typeName) && typeName.Length > 1)
		{
			char prefix = typeName[0];
			if ((prefix == 'F' || prefix == 'U' || prefix == 'A') && Char.IsUpper(typeName[1]))
			{
				typeName = typeName.Substring(1);
			}
		}

		if (String.IsNullOrEmpty(propertyName))
		{
			return typeName;
		}

		return $"{typeName}::{propertyName}";
	}

	private static string GetDirectAccessorErrorMessage(string propType)
	{
		if (String.IsNullOrEmpty(propType))
		{
			return "value is not compatible with this property";
		}

		if (propType.Equals("BoolProperty", StringComparison.Ordinal))
		{
			return "value is not bool";
		}

		if (propType.Equals("NameProperty", StringComparison.Ordinal))
		{
			return "value is not a Name";
		}

		if (propType.Equals("StrProperty", StringComparison.Ordinal))
		{
			return "value is not a string";
		}

		if (propType.Equals("TextProperty", StringComparison.Ordinal))
		{
			return "value is not Text";
		}

		if (propType.Equals("StructProperty", StringComparison.Ordinal))
		{
			return "value is not a compatible struct";
		}

		if (propType.Equals("ObjectProperty", StringComparison.Ordinal) || propType.Equals("SoftObjectProperty", StringComparison.Ordinal))
		{
			return "value is not a compatible object";
		}

		if (propType.Equals("ClassProperty", StringComparison.Ordinal) || propType.Equals("SoftClassProperty", StringComparison.Ordinal))
		{
			return "value is not a compatible class";
		}

		if (propType.Equals("ByteProperty", StringComparison.Ordinal) ||
			propType.Equals("Int8Property", StringComparison.Ordinal) ||
			propType.Equals("Int16Property", StringComparison.Ordinal) ||
			propType.Equals("IntProperty", StringComparison.Ordinal) ||
			propType.Equals("Int32Property", StringComparison.Ordinal) ||
			propType.Equals("Int64Property", StringComparison.Ordinal) ||
			propType.Equals("UInt16Property", StringComparison.Ordinal) ||
			propType.Equals("UInt32Property", StringComparison.Ordinal) ||
			propType.Equals("UInt64Property", StringComparison.Ordinal) ||
			propType.Equals("FloatProperty", StringComparison.Ordinal) ||
			propType.Equals("DoubleProperty", StringComparison.Ordinal) ||
			propType.Equals("LargeWorldCoordinatesRealProperty", StringComparison.Ordinal))
		{
			return "value is not numeric";
		}

		return "value is not compatible with this property";
	}

	private static bool TryBuildInitParamSnippet(PropJsonInfo param, int index, out InitParamSnippet snippet)
	{
		string pyArgName = $"PyArg{index}";
		string varName = $"Arg{index}";

		string propType = param.PropType;
		if (String.IsNullOrEmpty(propType))
		{
			snippet = null!;
			return false;
		}

		switch (propType)
		{
			case "BoolProperty":
				snippet = new InitParamSnippet(
					$"bool {varName} = false;",
					$"if (!UPyConversion::Nativize({pyArgName}, {varName}))\n{{\n\treturn -1;\n}}",
					varName);
				return true;

			case "EnumProperty":
				{
					string? nativeType = ResolveNativeTypeName(param.TypeName);
					if (String.IsNullOrEmpty(nativeType))
					{
						nativeType = !String.IsNullOrEmpty(param.TypeName) ? param.TypeName : "uint8";
					}
					snippet = new InitParamSnippet(
						$"{nativeType} {varName};",
						$"if (!UPyConversion::NativizeEnumEntry({pyArgName}, StaticEnum<{GetEnumStaticType(nativeType)}>(), {varName}))\n{{\n\treturn -1;\n}}",
						varName);
					return true;
				}

			case "ByteProperty":
				{
					if (!String.IsNullOrEmpty(param.TypeName))
					{
						string? nativeType = ResolveNativeTypeName(param.TypeName);
						if (!String.IsNullOrEmpty(nativeType))
						{
							snippet = new InitParamSnippet(
								$"{nativeType} {varName};",
								$"if (!UPyConversion::NativizeEnumEntry({pyArgName}, StaticEnum<{GetEnumStaticType(nativeType)}>(), {varName}))\n{{\n\treturn -1;\n}}",
								varName);
							return true;
						}
					}
					snippet = new InitParamSnippet(
						$"uint8 {varName} = 0;",
						$"if (!UPyConversion::Nativize({pyArgName}, {varName}))\n{{\n\treturn -1;\n}}",
						varName);
					return true;
				}

			case "UInt8Property":
				snippet = new InitParamSnippet(
					$"uint8 {varName} = 0;",
					$"if (!UPyConversion::Nativize({pyArgName}, {varName}))\n{{\n\treturn -1;\n}}",
					varName);
				return true;

			case "Int8Property":
				snippet = new InitParamSnippet(
					$"int8 {varName} = 0;",
					$"if (!UPyConversion::Nativize({pyArgName}, {varName}))\n{{\n\treturn -1;\n}}",
					varName);
				return true;

			case "Int16Property":
				snippet = new InitParamSnippet(
					$"int16 {varName} = 0;",
					$"if (!UPyConversion::Nativize({pyArgName}, {varName}))\n{{\n\treturn -1;\n}}",
					varName);
				return true;

			case "IntProperty":
			case "Int32Property":
				snippet = new InitParamSnippet(
					$"int32 {varName} = 0;",
					$"if (!UPyConversion::Nativize({pyArgName}, {varName}))\n{{\n\treturn -1;\n}}",
					varName);
				return true;

			case "Int64Property":
				snippet = new InitParamSnippet(
					$"int64 {varName} = 0;",
					$"if (!UPyConversion::Nativize({pyArgName}, {varName}))\n{{\n\treturn -1;\n}}",
					varName);
				return true;

			case "UInt16Property":
				snippet = new InitParamSnippet(
					$"uint16 {varName} = 0;",
					$"if (!UPyConversion::Nativize({pyArgName}, {varName}))\n{{\n\treturn -1;\n}}",
					varName);
				return true;

			case "UInt32Property":
				snippet = new InitParamSnippet(
					$"uint32 {varName} = 0;",
					$"if (!UPyConversion::Nativize({pyArgName}, {varName}))\n{{\n\treturn -1;\n}}",
					varName);
				return true;

			case "UInt64Property":
				snippet = new InitParamSnippet(
					$"uint64 {varName} = 0;",
					$"if (!UPyConversion::Nativize({pyArgName}, {varName}))\n{{\n\treturn -1;\n}}",
					varName);
				return true;

			case "FloatProperty":
				snippet = new InitParamSnippet(
					$"float {varName} = 0.0f;",
					$"if (!UPyConversion::Nativize({pyArgName}, {varName}))\n{{\n\treturn -1;\n}}",
					varName);
				return true;

			case "DoubleProperty":
			case "LargeWorldCoordinatesRealProperty":
				snippet = new InitParamSnippet(
					$"double {varName} = 0.0;",
					$"if (!UPyConversion::Nativize({pyArgName}, {varName}))\n{{\n\treturn -1;\n}}",
					varName);
				return true;

			case "NameProperty":
				snippet = new InitParamSnippet(
					$"FName {varName};",
					$"if (!UPyConversion::Nativize({pyArgName}, {varName}))\n{{\n\treturn -1;\n}}",
					varName);
				return true;

			case "StrProperty":
				snippet = new InitParamSnippet(
					$"FString {varName};",
					$"if (!UPyConversion::Nativize({pyArgName}, {varName}))\n{{\n\treturn -1;\n}}",
					varName);
				return true;

			case "TextProperty":
				snippet = new InitParamSnippet(
					$"FText {varName};",
					$"if (!UPyConversion::Nativize({pyArgName}, {varName}))\n{{\n\treturn -1;\n}}",
					varName);
				return true;

			case "ArrayProperty":
				{
					string? innerType = ResolveContainerElementType(param.TypeName);
					if (String.IsNullOrEmpty(innerType))
					{
						snippet = null!;
						return false;
					}

					bool bIsStruct = !String.IsNullOrEmpty(param.TypeName) && StructLookupCache.ContainsKey(param.TypeName);
					string nativizeFunc = bIsStruct ? "UPyConversion::NativizeStructInstance" : "UPyConversion::Nativize";

					string loopCode =
						$"if (!PySequence_Check({pyArgName}))\n" +
						$"{{\n" +
						$"\treturn -1;\n" +
							$"}}\n" +
							$"Py_ssize_t Len_{varName} = PySequence_Size({pyArgName});\n" +
							$"int32 ElementCount_{varName} = 0;\n" +
							$"if (UPyUtil::ValidateContainerLenValue(Len_{varName}, ElementCount_{varName}, TEXT(\"Array\")) != 0)\n" +
							$"{{\n" +
							$"\treturn -1;\n" +
							$"}}\n" +
							$"{varName}.SetNum(ElementCount_{varName});\n" +
							$"for (int32 i = 0; i < ElementCount_{varName}; ++i)\n" +
							$"{{\n" +
							$"\tPyObject* Item = PySequence_GetItem({pyArgName}, i);\n" +
							$"\tif (!Item)\n" +
						$"\t{{\n" +
						$"\t\treturn -1;\n" +
						$"\t}}\n" +
						$"\tif (!{nativizeFunc}(Item, {varName}[i]))\n" +
						$"\t{{\n" +
						$"\t\tPy_DECREF(Item);\n" +
						$"\t\treturn -1;\n" +
						$"\t}}\n" +
						$"\tPy_DECREF(Item);\n" +
						$"}}";

					snippet = new InitParamSnippet(
						$"TArray<{innerType}> {varName};",
						loopCode,
						varName);
					return true;
				}

			case "SetProperty":
				{
					string? innerType = ResolveContainerElementType(param.TypeName);
					if (String.IsNullOrEmpty(innerType))
					{
						snippet = null!;
						return false;
					}

					bool bIsStruct = !String.IsNullOrEmpty(param.TypeName) && StructLookupCache.ContainsKey(param.TypeName);
					string nativizeFunc = bIsStruct ? "UPyConversion::NativizeStructInstance" : "UPyConversion::Nativize";

					string loopCode =
						$"PyObject* Iter_{varName} = PyObject_GetIter({pyArgName});\n" +
						$"if (!Iter_{varName})\n" +
						$"{{\n" +
						$"\treturn -1;\n" +
						$"}}\n" +
						$"PyObject* Item_{varName};\n" +
						$"while (true)\n" +
						$"{{\n" +
						$"\tItem_{varName} = PyIter_Next(Iter_{varName});\n" +
						$"\tif (!Item_{varName})\n" +
						$"\t{{\n" +
						$"\t\tbreak;\n" +
						$"\t}}\n" +
						$"\t{innerType} Val;\n" +
						$"\tif (!{nativizeFunc}(Item_{varName}, Val))\n" +
						$"\t{{\n" +
						$"\t\tPy_DECREF(Item_{varName});\n" +
						$"\t\tPy_DECREF(Iter_{varName});\n" +
						$"\t\treturn -1;\n" +
						$"\t}}\n" +
						$"\t{varName}.Add(Val);\n" +
						$"\tPy_DECREF(Item_{varName});\n" +
						$"}}\n" +
						$"Py_DECREF(Iter_{varName});\n" +
						$"if (PyErr_Occurred())\n" +
						$"{{\n" +
						$"\treturn -1;\n" +
						$"}}";

					snippet = new InitParamSnippet(
						$"TSet<{innerType}> {varName};",
						loopCode,
						varName);
					return true;
				}

			case "MapProperty":
				{
					string[] parts = param.TypeName?.Split(new[] { "->" }, StringSplitOptions.None) ?? Array.Empty<string>();
					if (parts.Length != 2)
					{
						snippet = null!;
						return false;
					}

					string? keyType = ResolveContainerElementType(parts[0]);
					string? valueType = ResolveContainerElementType(parts[1]);

					if (String.IsNullOrEmpty(keyType) || String.IsNullOrEmpty(valueType))
					{
						snippet = null!;
						return false;
					}

					bool bKeyIsStruct = !String.IsNullOrEmpty(parts[0]) && StructLookupCache.ContainsKey(parts[0]);
					bool bValueIsStruct = !String.IsNullOrEmpty(parts[1]) && StructLookupCache.ContainsKey(parts[1]);

					string nativizeKeyFunc = bKeyIsStruct ? "UPyConversion::NativizeStructInstance" : "UPyConversion::Nativize";
					string nativizeValueFunc = bValueIsStruct ? "UPyConversion::NativizeStructInstance" : "UPyConversion::Nativize";

					string loopCode =
						$"if (!PyDict_Check({pyArgName}))\n" +
						$"{{\n" +
						$"\treturn -1;\n" +
						$"}}\n" +
						$"PyObject *Key_{varName}, *Value_{varName};\n" +
						$"Py_ssize_t Pos_{varName} = 0;\n" +
						$"while (PyDict_Next({pyArgName}, &Pos_{varName}, &Key_{varName}, &Value_{varName}))\n" +
						$"{{\n" +
						$"\t{keyType} K;\n" +
						$"\t{valueType} V;\n" +
						$"\tif (!{nativizeKeyFunc}(Key_{varName}, K))\n" +
						$"{{\n" +
						$"\t\treturn -1;\n" +
						$"\t}}\n" +
						$"\tif (!{nativizeValueFunc}(Value_{varName}, V))\n" +
						$"{{\n" +
						$"\t\treturn -1;\n" +
						$"\t}}\n" +
						$"\t{varName}.Add(K, V);\n" +
						$"}}";

					snippet = new InitParamSnippet(
						$"TMap<{keyType}, {valueType}> {varName};",
						loopCode,
						varName);
					return true;
				}

			case "StructProperty":
				{
					string? nativeType = ResolveNativeTypeName(param.TypeName);
					if (String.IsNullOrEmpty(nativeType))
					{
						snippet = null!;
						return false;
					}

					snippet = new InitParamSnippet(
						$"{nativeType} {varName};",
						$"if (!UPyConversion::NativizeStructInstance({pyArgName}, {varName}))\n{{\n\treturn -1;\n}}",
						varName);
					return true;
				}

			case "ObjectProperty":
			case "SoftObjectProperty":
				{
					string? nativeType = ResolveNativeTypeName(param.TypeName);
					if (String.IsNullOrEmpty(nativeType))
					{
						snippet = null!;
						return false;
					}

					string declaration = $"{nativeType}* {varName} = nullptr;";
					string conversion =
						$"if (!UPyConversion::Nativize({pyArgName}, {varName}))\n{{\n\treturn -1;\n}}";

					snippet = new InitParamSnippet(declaration, conversion, varName);
					return true;
				}

			case "ClassProperty":
			case "SoftClassProperty":
				{
					string? nativeType = ResolveNativeTypeName(param.TypeName);
					if (String.IsNullOrEmpty(nativeType))
					{
						nativeType = "UClass";
					}

					string declaration = $"{nativeType}* {varName} = nullptr;";
					string conversion =
						$"if (!UPyConversion::Nativize({pyArgName}, {varName}))\n{{\n\treturn -1;\n}}";

					snippet = new InitParamSnippet(declaration, conversion, varName);
					return true;
				}
		}

		snippet = null!;
		return false;
	}

	private static string BuildContainerReturnSnippet(string varName, PropJsonInfo param)
	{
		if (param.PropType.Equals("ArrayProperty", StringComparison.Ordinal))
		{
			bool bIsStruct = !String.IsNullOrEmpty(param.TypeName) && StructLookupCache.ContainsKey(param.TypeName);
			string pythonizeFunc = bIsStruct ? "UPyConversion::PythonizeStructInstance" : "UPyConversion::Pythonize";

			return
				$"PyObject* PyList = PyList_New({varName}.Num());\n" +
				$"for (int32 i = 0; i < {varName}.Num(); ++i)\n" +
				$"{{\n" +
				$"\tPyObject* PyItem = {pythonizeFunc}({varName}[i]);\n" +
				$"\tPyList_SET_ITEM(PyList, i, PyItem);\n" +
				$"}}\n" +
				$"return PyList;";
		}
		if (param.PropType.Equals("SetProperty", StringComparison.Ordinal))
		{
			bool bIsStruct = !String.IsNullOrEmpty(param.TypeName) && StructLookupCache.ContainsKey(param.TypeName);
			string pythonizeFunc = bIsStruct ? "UPyConversion::PythonizeStructInstance" : "UPyConversion::Pythonize";

			return
				$"PyObject* PySet = PySet_New(nullptr);\n" +
				$"for (const auto& Elem : {varName})\n" +
				$"{{\n" +
				$"\tPyObject* PyItem = {pythonizeFunc}(Elem);\n" +
				$"\tPySet_Add(PySet, PyItem);\n" +
				$"\tPy_DECREF(PyItem);\n" +
				$"}}\n" +
				$"return PySet;";
		}
		if (param.PropType.Equals("MapProperty", StringComparison.Ordinal))
		{
			string[] parts = param.TypeName?.Split(new[] { "->" }, StringSplitOptions.None) ?? Array.Empty<string>();
			bool bKeyIsStruct = parts.Length == 2 && !String.IsNullOrEmpty(parts[0]) && StructLookupCache.ContainsKey(parts[0]);
			bool bValueIsStruct = parts.Length == 2 && !String.IsNullOrEmpty(parts[1]) && StructLookupCache.ContainsKey(parts[1]);

			string pythonizeKeyFunc = bKeyIsStruct ? "UPyConversion::PythonizeStructInstance" : "UPyConversion::Pythonize";
			string pythonizeValueFunc = bValueIsStruct ? "UPyConversion::PythonizeStructInstance" : "UPyConversion::Pythonize";

			return
				$"PyObject* PyDict = PyDict_New();\n" +
				$"for (const auto& Elem : {varName})\n" +
				$"{{\n" +
				$"\tPyObject* PyKey = {pythonizeKeyFunc}(Elem.Key);\n" +
				$"\tPyObject* PyValue = {pythonizeValueFunc}(Elem.Value);\n" +
				$"\tPyDict_SetItem(PyDict, PyKey, PyValue);\n" +
				$"\tPy_DECREF(PyKey);\n" +
				$"\tPy_DECREF(PyValue);\n" +
				$"}}\n" +
				$"return PyDict;";
		}
		return $"return UPyConversion::Pythonize({varName});";
	}

	private static string BuildContainerTupleItemSnippet(string varName, PropJsonInfo param, int tupleIndex)
	{
		if (param.PropType.Equals("ArrayProperty", StringComparison.Ordinal))
		{
			bool bIsStruct = !String.IsNullOrEmpty(param.TypeName) && StructLookupCache.ContainsKey(param.TypeName);
			string pythonizeFunc = bIsStruct ? "UPyConversion::PythonizeStructInstance" : "UPyConversion::Pythonize";

			return
				$"PyObject* PyList_{tupleIndex} = PyList_New({varName}.Num());\n" +
				$"for (int32 i = 0; i < {varName}.Num(); ++i)\n" +
				$"{{\n" +
				$"\tPyObject* PyItem = {pythonizeFunc}({varName}[i]);\n" +
				$"\tPyList_SET_ITEM(PyList_{tupleIndex}, i, PyItem);\n" +
				$"}}\n" +
				$"PyTuple_SET_ITEM(ReturnTuple, {tupleIndex}, PyList_{tupleIndex});";
		}
		if (param.PropType.Equals("SetProperty", StringComparison.Ordinal))
		{
			bool bIsStruct = !String.IsNullOrEmpty(param.TypeName) && StructLookupCache.ContainsKey(param.TypeName);
			string pythonizeFunc = bIsStruct ? "UPyConversion::PythonizeStructInstance" : "UPyConversion::Pythonize";

			return
				$"PyObject* PySet_{tupleIndex} = PySet_New(nullptr);\n" +
				$"for (const auto& Elem : {varName})\n" +
				$"{{\n" +
				$"\tPyObject* PyItem = {pythonizeFunc}(Elem);\n" +
				$"\tPySet_Add(PySet_{tupleIndex}, PyItem);\n" +
				$"\tPy_DECREF(PyItem);\n" +
				$"}}\n" +
				$"PyTuple_SET_ITEM(ReturnTuple, {tupleIndex}, PySet_{tupleIndex});";
		}
		if (param.PropType.Equals("MapProperty", StringComparison.Ordinal))
		{
			string[] parts = param.TypeName?.Split(new[] { "->" }, StringSplitOptions.None) ?? Array.Empty<string>();
			bool bKeyIsStruct = parts.Length == 2 && !String.IsNullOrEmpty(parts[0]) && StructLookupCache.ContainsKey(parts[0]);
			bool bValueIsStruct = parts.Length == 2 && !String.IsNullOrEmpty(parts[1]) && StructLookupCache.ContainsKey(parts[1]);

			string pythonizeKeyFunc = bKeyIsStruct ? "UPyConversion::PythonizeStructInstance" : "UPyConversion::Pythonize";
			string pythonizeValueFunc = bValueIsStruct ? "UPyConversion::PythonizeStructInstance" : "UPyConversion::Pythonize";

			return
				$"PyObject* PyDict_{tupleIndex} = PyDict_New();\n" +
				$"for (const auto& Elem : {varName})\n" +
				$"{{\n" +
				$"\tPyObject* PyKey = {pythonizeKeyFunc}(Elem.Key);\n" +
				$"\tPyObject* PyValue = {pythonizeValueFunc}(Elem.Value);\n" +
				$"\tPyDict_SetItem(PyDict_{tupleIndex}, PyKey, PyValue);\n" +
				$"\tPy_DECREF(PyKey);\n" +
				$"\tPy_DECREF(PyValue);\n" +
				$"}}\n" +
				$"PyTuple_SET_ITEM(ReturnTuple, {tupleIndex}, PyDict_{tupleIndex});";
		}
		return $"PyTuple_SET_ITEM(ReturnTuple, {tupleIndex}, UPyConversion::Pythonize({varName}));";
	}

	private static string? ResolveNativeTypeName(string? typeName)
	{
		if (String.IsNullOrEmpty(typeName))
		{
			return null;
		}

		if (EnumLookupCache.TryGetValue(typeName, out EnumJsonInfo? enumInfo) && !String.IsNullOrEmpty(enumInfo.NativeTypeName))
		{
			return enumInfo.NativeTypeName;
		}

		if (StructLookupCache.TryGetValue(typeName, out StructJsonInfo? structInfo))
		{
			if (!String.IsNullOrEmpty(structInfo.NativeTypeName))
			{
				return structInfo.NativeTypeName;
			}
		}

		if (ClassLookupCache.TryGetValue(typeName, out ClassJsonInfo? classInfo))
		{
			if (!String.IsNullOrEmpty(classInfo.NativeTypeName))
			{
				return classInfo.NativeTypeName;
			}
		}

		return null;
	}

	private static string? GetCppTypeForProperty(string propType, string? typeName)
	{
		switch (propType)
		{
			case "BoolProperty": return "bool";
			case "ByteProperty":
			case "EnumProperty":
				if (!String.IsNullOrEmpty(typeName))
				{
					return ResolveNativeTypeName(typeName) ?? typeName;
				}
				return "uint8";
			case "Int8Property": return "int8";
			case "Int16Property": return "int16";
			case "IntProperty":
			case "Int32Property": return "int32";
			case "Int64Property": return "int64";
			case "UInt16Property": return "uint16";
			case "UInt32Property": return "uint32";
			case "UInt64Property": return "uint64";
			case "FloatProperty": return "float";
			case "DoubleProperty":
			case "LargeWorldCoordinatesRealProperty": return "double";
			case "NameProperty": return "FName";
			case "StrProperty": return "FString";
			case "TextProperty": return "FText";
			case "StructProperty":
				return ResolveNativeTypeName(typeName) ?? typeName;
			case "ObjectProperty":
			case "SoftObjectProperty":
			case "WeakObjectProperty":
			case "LazyObjectProperty":
			case "ClassProperty":
			case "SoftClassProperty":
				{
					string? nativeType = ResolveNativeTypeName(typeName);
					if (String.IsNullOrEmpty(nativeType))
					{
						if (typeName == "Object") return "UObject*";
						return null;
					}
					return nativeType + "*";
				}
			case "InterfaceProperty":
				{
					string? nativeType = ResolveNativeTypeName(typeName);
					if (String.IsNullOrEmpty(nativeType)) return null;
					return $"TScriptInterface<{nativeType}>";
				}
		}
		return null;
	}

	private static string? ResolveContainerElementType(string? typeName)
	{
		if (String.IsNullOrEmpty(typeName))
		{
			return null;
		}

		if (typeName.StartsWith("ByteProperty<") && typeName.EndsWith(">"))
		{
			string enumName = typeName.Substring(13, typeName.Length - 14);
			string? resolvedEnum = ResolveNativeTypeName(enumName) ?? enumName;
			return $"TEnumAsByte<{resolvedEnum}>";
		}

		if (typeName.EndsWith("Property"))
		{
			return GetCppTypeForProperty(typeName, null);
		}

		if (ClassLookupCache.ContainsKey(typeName))
		{
			string? native = ResolveNativeTypeName(typeName);
			return native != null ? native + "*" : null;
		}

		if (StructLookupCache.ContainsKey(typeName))
		{
			return ResolveNativeTypeName(typeName);
		}

		if (typeName == "Object") return "UObject*";

		return ResolveNativeTypeName(typeName) ?? typeName;
	}

	private static string FormatEnumDefaultValue(string nativeType, string defaultValue)
	{
		if (String.IsNullOrEmpty(defaultValue))
		{
			return $"({nativeType})0";
		}

		if (Char.IsDigit(defaultValue[0]) || defaultValue[0] == '-')
		{
			return defaultValue;
		}

		int lastColonIndex = defaultValue.LastIndexOf("::");
		if (lastColonIndex != -1)
		{
			defaultValue = defaultValue.Substring(lastColonIndex + 2);
		}

		// string scope = nativeType;
		// if (scope.EndsWith("::Type"))
		// {
		// 	scope = scope.Substring(0, scope.Length - 6);
		// }

		return $"{nativeType}::{defaultValue}";
	}

	private static bool TryBuildOperatorArgumentConversion(string rawType, string rhsExpression, string errorContextLiteral, int paramIndex, PropJsonInfo param, string failureReturn, bool suppressErrors, out string conversionCode, out string argumentExpression)
	{
		string varName = $"Arg{paramIndex}";
		string indent = "\t\t";
		string failureStatement = suppressErrors
			? $"{indent}\treturn {failureReturn};\n"
			: $"{indent}\tUPyUtil::SetPythonError(PyExc_RuntimeError, TEXT(\"{errorContextLiteral}\"), TEXT(\"invalid argument\"));\n{indent}\treturn {failureReturn};\n";

		bool hasDefault = param.DefaultValue != null;
		string WrapNativize(string code)
		{
			if (hasDefault)
			{
				string indentedCode = "\t" + code.Replace("\n", "\n\t");
				return $"{indent}if ({rhsExpression} != nullptr)\n{indent}{{\n{indentedCode}\n{indent}}}";
			}
			return code;
		}

		switch (param.PropType)
		{
			case "BoolProperty":
				conversionCode =
					$"{indent}bool {varName} = {(hasDefault ? param.DefaultValue : "false")};\n" +
					WrapNativize(
						$"{indent}if (!UPyConversion::Nativize({rhsExpression}, {varName}))\n" +
						$"{indent}{{\n" +
						failureStatement +
						$"{indent}}}"
					);
				argumentExpression = varName;
				return true;

			case "EnumProperty":
				{
					string? nativeType = ResolveNativeTypeName(param.TypeName);
					if (String.IsNullOrEmpty(nativeType))
					{
						nativeType = !String.IsNullOrEmpty(param.TypeName) ? param.TypeName : "uint8";
					}
					conversionCode =
						$"{indent}{nativeType} {varName}" + (hasDefault ? $" = {FormatEnumDefaultValue(nativeType, param.DefaultValue!)}" : "") + ";\n" +
						WrapNativize(
							$"{indent}if (!UPyConversion::NativizeEnumEntry({rhsExpression}, StaticEnum<{GetEnumStaticType(nativeType)}>(), {varName}))\n" +
							$"{indent}{{\n" +
							failureStatement +
							$"{indent}}}"
						);
					argumentExpression = varName;
					return true;
				}

			case "ByteProperty":
				{
					// Check if this is an enum (has TypeName)
					if (!String.IsNullOrEmpty(param.TypeName))
					{
						string? nativeType = ResolveNativeTypeName(param.TypeName);
						if (String.IsNullOrEmpty(nativeType))
						{
							conversionCode = String.Empty;
							argumentExpression = String.Empty;
							return false;
						}

						conversionCode =
							$"{indent}{nativeType} {varName} = {(hasDefault ? FormatEnumDefaultValue(nativeType, param.DefaultValue!) : $"({nativeType})0")};\n" +
							WrapNativize(
								$"{indent}if (!UPyConversion::NativizeEnumEntry({rhsExpression}, StaticEnum<{GetEnumStaticType(nativeType)}>(), {varName}))\n" +
								$"{indent}{{\n" +
								failureStatement +
								$"{indent}}}"
							);
						argumentExpression = varName;
						return true;
					}
					// Plain byte property (no TypeName)
					conversionCode =
						$"{indent}uint8 {varName} = {(hasDefault ? param.DefaultValue : "0")};\n" +
						WrapNativize(
							$"{indent}if (!UPyConversion::Nativize({rhsExpression}, {varName}))\n" +
							$"{indent}{{\n" +
							failureStatement +
							$"{indent}}}"
						);
					argumentExpression = varName;
					return true;
				}

			case "UInt8Property":
				conversionCode =
					$"{indent}uint8 {varName} = {(hasDefault ? param.DefaultValue : "0")};\n" +
					WrapNativize(
						$"{indent}if (!UPyConversion::Nativize({rhsExpression}, {varName}))\n" +
						$"{indent}{{\n" +
						failureStatement +
						$"{indent}}}"
					);
				argumentExpression = varName;
				return true;

			case "Int8Property":
				conversionCode =
					$"{indent}int8 {varName} = {(hasDefault ? param.DefaultValue : "0")};\n" +
					WrapNativize(
						$"{indent}if (!UPyConversion::Nativize({rhsExpression}, {varName}))\n" +
						$"{indent}{{\n" +
						failureStatement +
						$"{indent}}}"
					);
				argumentExpression = varName;
				return true;

			case "Int16Property":
				conversionCode =
					$"{indent}int16 {varName} = {(hasDefault ? param.DefaultValue : "0")};\n" +
					WrapNativize(
						$"{indent}if (!UPyConversion::Nativize({rhsExpression}, {varName}))\n" +
						$"{indent}{{\n" +
						failureStatement +
						$"{indent}}}"
					);
				argumentExpression = varName;
				return true;

			case "IntProperty":
			case "Int32Property":
				conversionCode =
					$"{indent}int32 {varName} = {(hasDefault ? param.DefaultValue : "0")};\n" +
					WrapNativize(
						$"{indent}if (!UPyConversion::Nativize({rhsExpression}, {varName}))\n" +
						$"{indent}{{\n" +
						failureStatement +
						$"{indent}}}"
					);
				argumentExpression = varName;
				return true;

			case "Int64Property":
				conversionCode =
					$"{indent}int64 {varName} = {(hasDefault ? param.DefaultValue : "0")};\n" +
					WrapNativize(
						$"{indent}if (!UPyConversion::Nativize({rhsExpression}, {varName}))\n" +
						$"{indent}{{\n" +
						failureStatement +
						$"{indent}}}"
					);
				argumentExpression = varName;
				return true;

			case "UInt16Property":
				conversionCode =
					$"{indent}uint16 {varName} = {(hasDefault ? param.DefaultValue : "0")};\n" +
					WrapNativize(
						$"{indent}if (!UPyConversion::Nativize({rhsExpression}, {varName}))\n" +
						$"{indent}{{\n" +
						failureStatement +
						$"{indent}}}"
					);
				argumentExpression = varName;
				return true;

			case "UInt32Property":
				conversionCode =
					$"{indent}uint32 {varName} = {(hasDefault ? param.DefaultValue : "0")};\n" +
					WrapNativize(
						$"{indent}if (!UPyConversion::Nativize({rhsExpression}, {varName}))\n" +
						$"{indent}{{\n" +
						failureStatement +
						$"{indent}}}"
					);
				argumentExpression = varName;
				return true;

			case "UInt64Property":
				conversionCode =
					$"{indent}uint64 {varName} = {(hasDefault ? param.DefaultValue : "0")};\n" +
					WrapNativize(
						$"{indent}if (!UPyConversion::Nativize({rhsExpression}, {varName}))\n" +
						$"{indent}{{\n" +
						failureStatement +
						$"{indent}}}"
					);
				argumentExpression = varName;
				return true;

			case "FloatProperty":
				conversionCode =
					$"{indent}float {varName} = {(hasDefault ? param.DefaultValue : "0.0f")};\n" +
					WrapNativize(
						$"{indent}if (!UPyConversion::Nativize({rhsExpression}, {varName}))\n" +
						$"{indent}{{\n" +
						failureStatement +
						$"{indent}}}"
					);
				argumentExpression = varName;
				return true;

			case "DoubleProperty":
			case "LargeWorldCoordinatesRealProperty":
				conversionCode =
					$"{indent}double {varName} = {(hasDefault ? param.DefaultValue : "0.0")};\n" +
					WrapNativize(
						$"{indent}if (!UPyConversion::Nativize({rhsExpression}, {varName}))\n" +
						$"{indent}{{\n" +
						failureStatement +
						$"{indent}}}"
					);
				argumentExpression = varName;
				return true;

			case "NameProperty":
				{
					string defaultValue = param.DefaultValue ?? String.Empty;
					if (hasDefault)
					{
						if (defaultValue == "None" || defaultValue == "NAME_None")
						{
							defaultValue = "NAME_None";
						}
						else if (defaultValue.StartsWith("\""))
						{
							defaultValue = $"TEXT({defaultValue})";
						}
						else if (!defaultValue.StartsWith("TEXT") && !defaultValue.Contains("::") && !defaultValue.Contains("("))
						{
							defaultValue = $"TEXT(\"{defaultValue}\")";
						}
					}
					conversionCode =
						$"{indent}FName {varName}" + (hasDefault ? $" = {defaultValue}" : "") + ";\n" +
						WrapNativize(
							$"{indent}if (!UPyConversion::Nativize({rhsExpression}, {varName}))\n" +
							$"{indent}{{\n" +
							failureStatement +
							$"{indent}}}"
						);
					argumentExpression = varName;
					return true;
				}

			case "StrProperty":
				{
					string defaultValue = param.DefaultValue ?? String.Empty;
					if (hasDefault)
					{
						if (defaultValue.StartsWith("\""))
						{
							defaultValue = $"TEXT({defaultValue})";
						}
						else if (!defaultValue.StartsWith("TEXT") && !defaultValue.Contains("::") && !defaultValue.Contains("("))
						{
							defaultValue = $"TEXT(\"{defaultValue}\")";
						}
					}
					conversionCode =
						$"{indent}FString {varName}" + (hasDefault ? $" = {defaultValue}" : "") + ";\n" +
						WrapNativize(
							$"{indent}if (!UPyConversion::Nativize({rhsExpression}, {varName}))\n" +
							$"{indent}{{\n" +
							failureStatement +
							$"{indent}}}"
						);
					argumentExpression = varName;
					return true;
				}

			case "TextProperty":
				{
					string defaultValue = param.DefaultValue ?? String.Empty;
					if (hasDefault)
					{
						if (defaultValue.StartsWith("\""))
						{
							defaultValue = $"FText::FromString(TEXT({defaultValue}))";
						}
						else if (!defaultValue.StartsWith("TEXT") && !defaultValue.StartsWith("NSLOCTEXT") && !defaultValue.StartsWith("LOCTEXT") && !defaultValue.Contains("::") && !defaultValue.Contains("("))
						{
							defaultValue = $"FText::FromString(TEXT(\"{defaultValue}\"))";
						}
					}
					conversionCode =
						$"{indent}FText {varName}" + (hasDefault ? $" = {defaultValue}" : "") + ";\n" +
						WrapNativize(
							$"{indent}if (!UPyConversion::Nativize({rhsExpression}, {varName}))\n" +
							$"{indent}{{\n" +
							failureStatement +
							$"{indent}}}"
						);
					argumentExpression = varName;
					return true;
				}

			case "ArrayProperty":
				{
					string? innerType = ResolveContainerElementType(param.TypeName);
					if (String.IsNullOrEmpty(innerType))
					{
						conversionCode = String.Empty;
						argumentExpression = String.Empty;
						return false;
					}

					bool bIsStruct = !String.IsNullOrEmpty(param.TypeName) && StructLookupCache.ContainsKey(param.TypeName);
					string nativizeFunc = bIsStruct ? "UPyConversion::NativizeStructInstance" : "UPyConversion::Nativize";

					string loopFailureStatement = IndentCode(failureStatement, 1);
					string loopCode =
						$"{indent}if (!PySequence_Check({rhsExpression}))\n" +
						$"{indent}{{\n" +
							failureStatement +
							$"{indent}}}\n" +
							$"{indent}Py_ssize_t Len_{varName} = PySequence_Size({rhsExpression});\n" +
							$"{indent}int32 ElementCount_{varName} = 0;\n" +
							$"{indent}if (UPyUtil::ValidateContainerLenValue(Len_{varName}, ElementCount_{varName}, TEXT(\"Array\")) != 0)\n" +
							$"{indent}{{\n" +
							failureStatement +
							$"{indent}}}\n" +
							$"{indent}{varName}.SetNum(ElementCount_{varName});\n" +
							$"{indent}for (int32 i = 0; i < ElementCount_{varName}; ++i)\n" +
							$"{indent}{{\n" +
							$"{indent}\tPyObject* Item = PySequence_GetItem({rhsExpression}, i);\n" +
							$"{indent}\tif (!Item)\n" +
						$"{indent}\t{{\n" +
						loopFailureStatement + "\n" +
						$"{indent}\t}}\n" +
						$"{indent}\tif (!{nativizeFunc}(Item, {varName}[i]))\n" +
						$"{indent}\t{{\n" +
						$"{indent}\t\tPy_DECREF(Item);\n" +
						loopFailureStatement + "\n" +
						$"{indent}\t}}\n" +
						$"{indent}\tPy_DECREF(Item);\n" +
						$"{indent}}}";

					conversionCode =
						$"{indent}TArray<{innerType}> {varName};\n" +
						WrapNativize(loopCode);
					argumentExpression = varName;
					return true;
				}

			case "SetProperty":
				{
					string? innerType = ResolveContainerElementType(param.TypeName);
					if (String.IsNullOrEmpty(innerType))
					{
						conversionCode = String.Empty;
						argumentExpression = String.Empty;
						return false;
					}

					bool bIsStruct = !String.IsNullOrEmpty(param.TypeName) && StructLookupCache.ContainsKey(param.TypeName);
					string nativizeFunc = bIsStruct ? "UPyConversion::NativizeStructInstance" : "UPyConversion::Nativize";

					string loopFailureStatement = IndentCode(failureStatement, 1);
					string loopCode =
						$"{indent}PyObject* Iter_{varName} = PyObject_GetIter({rhsExpression});\n" +
						$"{indent}if (!Iter_{varName})\n" +
						$"{indent}{{\n" +
						failureStatement +
						$"{indent}}}\n" +
						$"{indent}PyObject* Item_{varName};\n" +
						$"{indent}while (true)\n" +
						$"{indent}{{\n" +
						$"{indent}\tItem_{varName} = PyIter_Next(Iter_{varName});\n" +
						$"{indent}\tif (!Item_{varName})\n" +
						$"{indent}\t{{\n" +
						$"{indent}\t\tbreak;\n" +
						$"{indent}\t}}\n" +
						$"{indent}\t{innerType} Val;\n" +
						$"{indent}\tif (!{nativizeFunc}(Item_{varName}, Val))\n" +
						$"{indent}\t{{\n" +
						$"{indent}\t\tPy_DECREF(Item_{varName});\n" +
						$"{indent}\t\tPy_DECREF(Iter_{varName});\n" +
						loopFailureStatement + "\n" +
						$"{indent}\t}}\n" +
						$"{indent}\t{varName}.Add(Val);\n" +
						$"{indent}\tPy_DECREF(Item_{varName});\n" +
						$"{indent}}}\n" +
						$"{indent}Py_DECREF(Iter_{varName});\n" +
						$"{indent}if (PyErr_Occurred())\n" +
						$"{indent}{{\n" +
						failureStatement +
						$"{indent}}}";

					conversionCode =
						$"{indent}TSet<{innerType}> {varName};\n" +
						WrapNativize(loopCode);
					argumentExpression = varName;
					return true;
				}

			case "MapProperty":
				{
					string[] parts = param.TypeName?.Split(new[] { "->" }, StringSplitOptions.None) ?? Array.Empty<string>();
					if (parts.Length != 2)
					{
						conversionCode = String.Empty;
						argumentExpression = String.Empty;
						return false;
					}

					string? keyType = ResolveContainerElementType(parts[0]);
					string? valueType = ResolveContainerElementType(parts[1]);

					if (String.IsNullOrEmpty(keyType) || String.IsNullOrEmpty(valueType))
					{
						conversionCode = String.Empty;
						argumentExpression = String.Empty;
						return false;
					}

					bool bKeyIsStruct = !String.IsNullOrEmpty(parts[0]) && StructLookupCache.ContainsKey(parts[0]);
					bool bValueIsStruct = !String.IsNullOrEmpty(parts[1]) && StructLookupCache.ContainsKey(parts[1]);

					string nativizeKeyFunc = bKeyIsStruct ? "UPyConversion::NativizeStructInstance" : "UPyConversion::Nativize";
					string nativizeValueFunc = bValueIsStruct ? "UPyConversion::NativizeStructInstance" : "UPyConversion::Nativize";

					string loopFailureStatement = IndentCode(failureStatement, 1);
					string loopCode =
						$"{indent}if (!PyDict_Check({rhsExpression}))\n" +
						$"{indent}{{\n" +
						failureStatement +
						$"{indent}}}\n" +
						$"{indent}PyObject *Key_{varName}, *Value_{varName};\n" +
						$"{indent}Py_ssize_t Pos_{varName} = 0;\n" +
						$"{indent}while (PyDict_Next({rhsExpression}, &Pos_{varName}, &Key_{varName}, &Value_{varName}))\n" +
						$"{indent}{{\n" +
						$"{indent}\t{keyType} K;\n" +
						$"{indent}\t{valueType} V;\n" +
						$"{indent}\tif (!{nativizeKeyFunc}(Key_{varName}, K))\n" +
						$"{indent}\t{{\n" +
						loopFailureStatement + "\n" +
						$"{indent}\t}}\n" +
						$"{indent}\tif (!{nativizeValueFunc}(Value_{varName}, V))\n" +
						$"{indent}\t{{\n" +
						loopFailureStatement + "\n" +
						$"{indent}\t}}\n" +
						$"{indent}\t{varName}.Add(K, V);\n" +
						$"{indent}}}";

					conversionCode =
						$"{indent}TMap<{keyType}, {valueType}> {varName};\n" +
						WrapNativize(loopCode);
					argumentExpression = varName;
					return true;
				}

			case "StructProperty":
				{
					string? nativeType = ResolveNativeTypeName(param.TypeName);
					if (String.IsNullOrEmpty(nativeType))
					{
						nativeType = param.TypeName;
					}

					string initCode = "";
					if (hasDefault && param.DefaultValue != "()")
					{
						initCode = ParseStructDefaultValue(nativeType ?? param.TypeName ?? "void", param.DefaultValue!);
					}

					conversionCode =
						$"{indent}{nativeType} {varName}{initCode};\n" +
						WrapNativize(
							$"{indent}if (!UPyConversion::NativizeStructInstance({rhsExpression}, {varName}))\n" +
							$"{indent}{{\n" +
							failureStatement +
							$"{indent}}}"
						);
					argumentExpression = varName;
					return true;
				}

			case "ObjectProperty":
			case "SoftObjectProperty":
			case "WeakObjectProperty":
			case "LazyObjectProperty":
				{
					string? nativeType = ResolveNativeTypeName(param.TypeName);
					if (String.IsNullOrEmpty(nativeType))
					{
						conversionCode = String.Empty;
						argumentExpression = String.Empty;
						return false;
					}

					string defaultValue = hasDefault ? param.DefaultValue! : "nullptr";
					if (defaultValue == "None")
					{
						defaultValue = "nullptr";
					}

					conversionCode =
						$"{indent}{nativeType}* {varName} = {defaultValue};\n" +
						WrapNativize(
							$"{indent}if (!UPyConversion::Nativize({rhsExpression}, {varName}))\n" +
							$"{indent}{{\n" +
							failureStatement +
							$"{indent}}}"
						);
					argumentExpression = varName;
					return true;
				}

			case "ClassProperty":
			case "SoftClassProperty":
				{
					string? nativeType = ResolveNativeTypeName(param.TypeName);
					if (String.IsNullOrEmpty(nativeType))
					{
						conversionCode = String.Empty;
						argumentExpression = String.Empty;
						return false;
					}

					string defaultValue = hasDefault ? param.DefaultValue! : "nullptr";
					if (defaultValue == "None")
					{
						defaultValue = "nullptr";
					}

					conversionCode =
						$"{indent}UClass* {varName} = {defaultValue};\n" +
						WrapNativize(
							$"{indent}if (!UPyConversion::NativizeClass({rhsExpression}, {varName}, nullptr))\n" +
							$"{indent}{{\n" +
							failureStatement +
							$"{indent}}}"
						);
					argumentExpression = varName;
					return true;
				}

			case "InterfaceProperty":
				{
					string? nativeType = ResolveNativeTypeName(param.TypeName);
					if (String.IsNullOrEmpty(nativeType))
					{
						conversionCode = String.Empty;
						argumentExpression = String.Empty;
						return false;
					}

					conversionCode =
						$"{indent}TScriptInterface<{nativeType}> {varName}" + (hasDefault ? $" = {param.DefaultValue}" : "") + ";\n" +
						WrapNativize(
							$"{indent}if (!UPyConversion::Nativize({rhsExpression}, {varName}))\n" +
							$"{indent}{{\n" +
							failureStatement +
							$"{indent}}}"
						);
					argumentExpression = varName;
					return true;
				}
		}

		conversionCode = String.Empty;
		argumentExpression = String.Empty;
		return false;
	}

	private static string BuildStructFunctionDefinitions(string cppType, string funcStructName, StructWrapperInfo structInfo, out bool hasRichCompare)
	{
		hasRichCompare = false;
		List<MethodJsonInfo> structMethods = GetStructMethods(structInfo);
		List<MethodJsonInfo> initMethods = structMethods
			.Where(method => method.MethodName.Equals("__init__", StringComparison.Ordinal))
			.ToList();

		string rawType = structInfo.Info.NativeTypeName;
		StringBuilder builder = new();
		builder.AppendLine($"struct {funcStructName}");
		builder.AppendLine("{");

		if (initMethods.Count > 0)
		{
			List<StructInitFunctionCode> initFunctionCodes = new();
			Dictionary<int, int> overloadIndices = new();
			for (int index = 0; index < initMethods.Count; ++index)
			{
				MethodJsonInfo method = initMethods[index];
				int argumentCount = method.Params.Count;
				overloadIndices.TryGetValue(argumentCount, out int overloadIndex);
				overloadIndices[argumentCount] = overloadIndex + 1;

				StructInitFunctionCode functionCode = BuildStructInitHelperFunction(cppType, rawType, funcStructName, structInfo, method, argumentCount, overloadIndex);
				initFunctionCodes.Add(functionCode);
				builder.AppendLine(functionCode.Definition);
				builder.AppendLine();
			}

			string initErrorContext = EscapeForCxxLiteral(BuildPropertyErrorContext(rawType, "__init__"));
			builder.AppendLine($"\tstatic int Init({cppType}* InSelf, PyObject* InArgs, PyObject* InKwds)");
			builder.AppendLine("\t{");
			builder.AppendLine("\t\tconst int BaseInit = FUPyWrapperStruct::Init(InSelf);");
			builder.AppendLine("\t\tif (BaseInit != 0)");
			builder.AppendLine("\t\t{");
			builder.AppendLine("\t\t\treturn BaseInit;");
			builder.AppendLine("\t\t}");
			builder.AppendLine();
			builder.AppendLine("\t\tswitch (PyTuple_Size(InArgs))");
			builder.AppendLine("\t\t{");

			foreach (IGrouping<int, StructInitFunctionCode> group in initFunctionCodes
				.GroupBy(code => code.ArgumentCount)
				.OrderBy(entry => entry.Key))
			{
				int argumentCount = group.Key;
				List<StructInitFunctionCode> overloads = group.ToList();

				builder.AppendLine($"\t\tcase {argumentCount}:");
				builder.AppendLine("\t\t{");
				builder.AppendLine("\t\t\tif (");
				for (int funcIndex = 0; funcIndex < overloads.Count; ++funcIndex)
				{
					string prefix = funcIndex == 0 ? "\t\t\t\t" : "\t\t\t\t|| ";
					builder.AppendLine($"{prefix}UPyUtil::TryInit(InSelf, InArgs, InKwds, &{funcStructName}::{overloads[funcIndex].FunctionName})");
				}
				builder.AppendLine("\t\t\t)");
				builder.AppendLine("\t\t\t{");
				builder.AppendLine("\t\t\t\treturn 0;");
				builder.AppendLine("\t\t\t}");
				builder.AppendLine("\t\t\tbreak;");
				builder.AppendLine("\t\t}");
			}

			builder.AppendLine("\t\tdefault:");
			builder.AppendLine("\t\t\tbreak;");
			builder.AppendLine("\t\t}");
			builder.AppendLine();
			builder.AppendLine($"\t\tUPyUtil::SetPythonError(PyExc_RuntimeError, TEXT(\"{initErrorContext}\"), TEXT(\"invalid argument\"));");
			builder.AppendLine("\t\treturn -1;");
			builder.AppendLine("\t}");
		}
		else
		{
			builder.AppendLine($"\tstatic int Init({cppType}* InSelf, PyObject* InArgs, PyObject* InKwds)");
			builder.AppendLine("\t{");
			builder.AppendLine("\t\treturn FUPyWrapperStruct::Init(InSelf);");
			builder.AppendLine("\t}");
		}

		List<MethodJsonInfo> comparisonMethods = structMethods
			.Where(method => method.bIsOperator && (method.MethodName.Equals("__eq__", StringComparison.Ordinal) || method.MethodName.Equals("__ne__", StringComparison.Ordinal)))
			.ToList();

		Dictionary<string, List<string>> comparisonAttempts = new(StringComparer.Ordinal);
		if (comparisonMethods.Count > 0)
		{
			builder.AppendLine();
			foreach (IGrouping<string, MethodJsonInfo> group in comparisonMethods.GroupBy(method => method.MethodName, StringComparer.Ordinal))
			{
				List<string> attemptNames = new();
				int overloadIndex = 0;
				foreach (MethodJsonInfo method in group)
				{
					int parameterCount = method.Params.Count(param => !param.PropName.Equals("ReturnValue", StringComparison.Ordinal));
					if (parameterCount != 1)
					{
						++overloadIndex;
						continue;
					}

					PropJsonInfo? operatorParam = method.Params.FirstOrDefault(param => !param.PropName.Equals("ReturnValue", StringComparison.Ordinal));
					if (operatorParam == null)
					{
						++overloadIndex;
						continue;
					}

					string attemptBase = $"{structInfo.WrapperSuffix}_{group.Key}_CmpAttempt{overloadIndex}";
					string attemptName = SanitizeIdentifier(attemptBase, "CmpAttempt");
					string failureReturn = "-1";
					string errorContextLiteral = EscapeForCxxLiteral($"{structInfo.Info.StructName}::{method.RawMethodName}");

					builder.AppendLine($"\tstatic int {attemptName}({cppType}* InSelf, PyObject* InOther)");
					builder.AppendLine("\t{");

					if (!TryBuildOperatorArgumentConversion(rawType, "InOther", errorContextLiteral, 0, operatorParam, failureReturn, true, out string conversionCode, out string argumentExpression))
					{
						builder.AppendLine("\t\treturn -1;");
						builder.AppendLine("\t}");
						builder.AppendLine();
						++overloadIndex;
						continue;
					}

					builder.AppendLine(conversionCode);
					builder.AppendLine();

					string? callExpression = GetBinaryOperatorCallExpression(method.MethodName, "InSelf->ValuePtr()", argumentExpression);
					if (String.IsNullOrEmpty(callExpression))
					{
						builder.AppendLine("\t\treturn -1;");
						builder.AppendLine("\t}");
						builder.AppendLine();
						++overloadIndex;
						continue;
					}

					builder.AppendLine($"\t\tconst bool bResult = {callExpression};");
					builder.AppendLine("\t\treturn bResult ? 1 : 0;");
					builder.AppendLine("\t}");
					builder.AppendLine();

					attemptNames.Add(attemptName);
					++overloadIndex;
				}

				if (attemptNames.Count > 0)
				{
					comparisonAttempts[group.Key] = attemptNames;
				}
			}
		}

		if (comparisonAttempts.Count > 0)
		{
			hasRichCompare = true;

			void AppendRichCompareCase(string caseLabel, List<string> attemptNames, bool invertResult)
			{
				if (attemptNames.Count == 0)
				{
					return;
				}

				builder.AppendLine($"\t\tcase {caseLabel}:");
				builder.AppendLine("\t\t{");
				foreach (string attemptName in attemptNames)
				{
					builder.AppendLine("\t\t\tif (!bHandled)");
					builder.AppendLine("\t\t\t{");
					builder.AppendLine($"\t\t\t\tint AttemptResult = {attemptName}(InSelf, InOther);");
					builder.AppendLine("\t\t\t\tif (AttemptResult >= 0)");
					builder.AppendLine("\t\t\t\t{");
					builder.AppendLine("\t\t\t\t\tbHandled = true;");
					builder.AppendLine("\t\t\t\t\tbResult = AttemptResult != 0;");
					if (invertResult)
					{
						builder.AppendLine("\t\t\t\t\tbResult = !bResult;");
					}
					builder.AppendLine("\t\t\t\t}");
					builder.AppendLine("\t\t\t\telse");
					builder.AppendLine("\t\t\t\t{");
					builder.AppendLine("\t\t\t\t\tPyErr_Clear();");
					builder.AppendLine("\t\t\t\t}");
					builder.AppendLine("\t\t\t}");
				}
				builder.AppendLine("\t\t\tbreak;");
				builder.AppendLine("\t\t}");
			}

			builder.AppendLine($"\tstatic PyObject* RichCmp({cppType}* InSelf, PyObject* InOther, int InOp)");
			builder.AppendLine("\t{");
			builder.AppendLine("\t\tbool bHandled = false;");
			builder.AppendLine("\t\tbool bResult = false;");
			builder.AppendLine("\t\tswitch (InOp)");
			builder.AppendLine("\t\t{");

			comparisonAttempts.TryGetValue("__eq__", out List<string>? eqAttempts);
			if (eqAttempts != null && eqAttempts.Count > 0)
			{
				AppendRichCompareCase("Py_EQ", eqAttempts, false);
			}

			comparisonAttempts.TryGetValue("__ne__", out List<string>? neAttempts);
			if (neAttempts != null && neAttempts.Count > 0)
			{
				AppendRichCompareCase("Py_NE", neAttempts, false);
			}
			else if (eqAttempts != null && eqAttempts.Count > 0)
			{
				AppendRichCompareCase("Py_NE", eqAttempts, true);
			}

			builder.AppendLine("\t\tdefault:");
			builder.AppendLine("\t\t\tbreak;");
			builder.AppendLine("\t\t}");
			builder.AppendLine("\t\tif (!bHandled)");
			builder.AppendLine("\t\t{");
			builder.AppendLine("\t\t\tPy_RETURN_NOTIMPLEMENTED;");
			builder.AppendLine("\t\t}");
			builder.AppendLine("\t\tif (bResult)");
			builder.AppendLine("\t\t{");
			builder.AppendLine("\t\t\tPy_RETURN_TRUE;");
			builder.AppendLine("\t\t}");
			builder.AppendLine("\t\tPy_RETURN_FALSE;");
			builder.AppendLine("\t}");
		}

		builder.AppendLine("};");
		return builder.ToString();
	}

	private static StructInitFunctionCode BuildStructInitHelperFunction(string cppType, string rawType, string funcStructName, StructWrapperInfo structInfo, MethodJsonInfo method, int paramCount, int overloadIndex)
	{
		string baseName = $"{structInfo.WrapperSuffix}_Init_ArgCnt{paramCount}_Overload{overloadIndex}";
		string functionName = SanitizeIdentifier(baseName, $"InitImpl_{paramCount}_{overloadIndex}");
		string errorContextLiteral = EscapeForCxxLiteral(BuildPropertyErrorContext(rawType, "__init__"));

		List<InitParamSnippet> paramSnippets = new();
		bool allParamsSupported = true;
		for (int paramIndex = 0; paramIndex < paramCount; ++paramIndex)
		{
			if (!TryBuildInitParamSnippet(method.Params[paramIndex], paramIndex, out InitParamSnippet snippet))
			{
				allParamsSupported = false;
				break;
			}
			paramSnippets.Add(snippet);
		}

		StringBuilder functionBuilder = new();
		functionBuilder.AppendLine($"\tstatic int {functionName}({cppType}* InSelf, PyObject* InArgs, PyObject* InKwds)");
		functionBuilder.AppendLine("\t{");

		if (!allParamsSupported)
		{
			functionBuilder.AppendLine($"\t\tUPyUtil::SetPythonError(PyExc_NotImplementedError, TEXT(\"{errorContextLiteral}\"), TEXT(\"init binding has not been generated yet\"));");
			functionBuilder.AppendLine("\t\treturn -1;");
			functionBuilder.AppendLine("\t}");
			return new StructInitFunctionCode(functionBuilder.ToString(), functionName, paramCount);
		}

		for (int paramIndex = 0; paramIndex < paramSnippets.Count; ++paramIndex)
		{
			string pyArgName = $"PyArg{paramIndex}";
			functionBuilder.AppendLine($"\t\tPyObject* {pyArgName} = PyTuple_GetItem(InArgs, {paramIndex});");
			functionBuilder.AppendLine($"\t\tif ({pyArgName} == nullptr)");
			functionBuilder.AppendLine("\t\t{");
			functionBuilder.AppendLine("\t\t\treturn -1;");
			functionBuilder.AppendLine("\t\t}");
			functionBuilder.AppendLine($"\t\t{paramSnippets[paramIndex].Declaration}");
			functionBuilder.AppendLine(IndentCode(paramSnippets[paramIndex].Conversion.TrimEnd(), 2));
			functionBuilder.AppendLine();
		}

		string constructorArgs = paramSnippets.Count > 0
			? String.Join(", ", paramSnippets.Select(snippet => snippet.ArgumentExpression))
			: String.Empty;
		if (constructorArgs.Length > 0)
		{
			functionBuilder.AppendLine($"\t\tnew (InSelf->StructInstance) {rawType}({constructorArgs});");
		}
		else
		{
			functionBuilder.AppendLine($"\t\tnew (InSelf->StructInstance) {rawType}();");
		}

		functionBuilder.AppendLine("\t\treturn 0;");
		functionBuilder.AppendLine("\t}");

		return new StructInitFunctionCode(functionBuilder.ToString(), functionName, paramCount);
	}

	private static NumberFuncCode BuildStructNumberFunctionDefinitions(string cppType, string rawType, StructWrapperInfo structInfo, string numberStructName)
	{
		return BuildNumberFunctionDefinitions(cppType, rawType, structInfo.WrapperSuffix, GetStructMethods(structInfo), numberStructName, structInfo.Info.StructName);
	}

	private static NumberFuncCode BuildNumberFunctionDefinitions(string cppType, string rawType, string wrapperSuffix, IReadOnlyList<MethodJsonInfo> methods, string numberStructName, string typeDisplayName)
	{
		List<MethodJsonInfo> operatorMethods = methods
			.Where(method => method.bIsOperator && IsMethodExternallyAccessible(method))
			.ToList();

		if (operatorMethods.Count == 0)
		{
			return new NumberFuncCode(String.Empty, String.Empty, numberStructName, false);
		}

		string helperName = $"FNumberFuncs_{wrapperSuffix}";
		StringBuilder helperBuilder = new();
		helperBuilder.AppendLine($"struct {helperName}");
		helperBuilder.AppendLine("{");

		List<string> assignmentLines = new();
		bool generatedAnyOperators = false;

		foreach (IGrouping<string, MethodJsonInfo> group in operatorMethods.GroupBy(method => method.MethodName, StringComparer.Ordinal))
		{
			if (!NumberOperatorMappings.TryGetValue(group.Key, out NumberOperatorMapping? mapping))
			{
				continue;
			}

			List<MethodJsonInfo> overloads = group.ToList();
			List<string> attemptNames = new();
			string sanitizedOperatorName = SanitizeIdentifier(group.Key.Trim('_'), "Operator");
			string attemptBaseName = $"Op_{sanitizedOperatorName}";

			for (int overloadIndex = 0; overloadIndex < overloads.Count; ++overloadIndex)
			{
				MethodJsonInfo method = overloads[overloadIndex];

				int parameterCount = method.Params.Count(param => !param.PropName.Equals("ReturnValue", StringComparison.Ordinal));
				if ((mapping.IsUnary && parameterCount != 0) || (!mapping.IsUnary && parameterCount != 1))
				{
					continue;
				}

				string attemptName = $"{attemptBaseName}_Attempt{overloadIndex}";
				string failureReturn = mapping.DelegateType.Equals("inquiry", StringComparison.Ordinal) ? "-1" : "nullptr";
				string errorContextLiteral = EscapeForCxxLiteral($"{typeDisplayName}::{method.RawMethodName}");

				helperBuilder.AppendLine($"\tstatic {(mapping.DelegateType == "inquiry" ? "int" : "PyObject*")} {attemptName}({cppType}* InSelf{(mapping.IsUnary ? String.Empty : ", PyObject* InRHS")})");
				helperBuilder.AppendLine("\t{");

				string? callExpression = null;

				if (!mapping.IsUnary)
				{
					PropJsonInfo? operatorParam = method.Params.FirstOrDefault(param => !param.PropName.Equals("ReturnValue", StringComparison.Ordinal));
					if (operatorParam == null)
					{
						helperBuilder.AppendLine($"\t\treturn {failureReturn};");
						helperBuilder.AppendLine("\t}");
						continue;
					}

					if (!TryBuildOperatorArgumentConversion(rawType, "InRHS", errorContextLiteral, overloadIndex, operatorParam, failureReturn, true, out string conversionCode, out string argumentExpression))
					{
						helperBuilder.AppendLine($"\t\tUPyUtil::SetPythonError(PyExc_RuntimeError, TEXT(\"{errorContextLiteral}\"), TEXT(\"operator not supported\"));");
						helperBuilder.AppendLine($"\t\treturn {failureReturn};");
						helperBuilder.AppendLine("\t}");
						continue;
					}

					helperBuilder.AppendLine(conversionCode);
					helperBuilder.AppendLine();

					callExpression = GetBinaryOperatorCallExpression(method.MethodName, "InSelf->ValuePtr()", argumentExpression);
				}
				else
				{
					callExpression = GetUnaryOperatorCallExpression(method.MethodName, "InSelf->ValuePtr()");
				}

				if (String.IsNullOrEmpty(callExpression))
				{
					helperBuilder.AppendLine($"\t\tUPyUtil::SetPythonError(PyExc_RuntimeError, TEXT(\"{errorContextLiteral}\"), TEXT(\"operator not supported\"));");
					helperBuilder.AppendLine($"\t\treturn {failureReturn};");
					helperBuilder.AppendLine("\t}");
					continue;
				}

				if (mapping.IsInPlace)
				{
					helperBuilder.AppendLine($"\t\t{callExpression};");
					helperBuilder.AppendLine("\t\tPy_INCREF(InSelf);");
					helperBuilder.AppendLine("\t\treturn (PyObject*)InSelf;");
				}
				else
				{
					PropJsonInfo? returnParam = method.Params.FirstOrDefault(param => param.PropName.Equals("ReturnValue", StringComparison.Ordinal));

					if (mapping.DelegateType.Equals("inquiry", StringComparison.Ordinal))
					{
						helperBuilder.AppendLine($"\t\tconst bool bResult = {callExpression};");
						helperBuilder.AppendLine("\t\treturn bResult ? 1 : 0;");
					}
					else if (returnParam != null && returnParam.PropType.Equals("BoolProperty", StringComparison.Ordinal))
					{
						helperBuilder.AppendLine($"\t\tconst bool Result = {callExpression};");
						helperBuilder.AppendLine("\t\treturn PyBool_FromLong(Result ? 1 : 0);");
					}
					else if (returnParam != null && returnParam.PropType.Equals("StructProperty", StringComparison.Ordinal))
					{
						helperBuilder.AppendLine($"\t\tconst auto Result = {callExpression};");
						helperBuilder.AppendLine("\t\treturn UPyConversion::PythonizeStructInstance(Result);");
					}
					else
					{
						helperBuilder.AppendLine($"\t\tconst auto Result = {callExpression};");
						helperBuilder.AppendLine("\t\treturn UPyConversion::Pythonize(Result);");
					}
				}

				helperBuilder.AppendLine("\t}");
				helperBuilder.AppendLine();

				attemptNames.Add(attemptName);
			}

			if (attemptNames.Count == 0)
			{
				continue;
			}

			string aggregatorErrorContext = EscapeForCxxLiteral($"{typeDisplayName}::{group.Key}");
			string finalFunctionName = attemptBaseName;
			string finalFailureReturn = mapping.DelegateType.Equals("inquiry", StringComparison.Ordinal) ? "-1" : "nullptr";

			helperBuilder.AppendLine($"\tstatic {(mapping.DelegateType == "inquiry" ? "int" : "PyObject*")} {finalFunctionName}({cppType}* InSelf{(mapping.IsUnary ? String.Empty : ", PyObject* InRHS")})");
			helperBuilder.AppendLine("\t{");

			foreach (string attemptName in attemptNames)
			{
				string resultType = mapping.DelegateType.Equals("inquiry", StringComparison.Ordinal) ? "int" : "PyObject*";
				helperBuilder.AppendLine("\t\t{");
				helperBuilder.AppendLine($"\t\t\t{resultType} AttemptResult = {attemptName}(InSelf{(mapping.IsUnary ? String.Empty : ", InRHS")});");
				if (mapping.DelegateType.Equals("inquiry", StringComparison.Ordinal))
				{
					helperBuilder.AppendLine("\t\t\tif (AttemptResult != -1)");
					helperBuilder.AppendLine("\t\t\t{");
					helperBuilder.AppendLine("\t\t\t\treturn AttemptResult;");
					helperBuilder.AppendLine("\t\t\t}");
				}
				else
				{
					helperBuilder.AppendLine("\t\t\tif (AttemptResult != nullptr)");
					helperBuilder.AppendLine("\t\t\t{");
					helperBuilder.AppendLine("\t\t\t\treturn AttemptResult;");
					helperBuilder.AppendLine("\t\t\t}");
				}
				helperBuilder.AppendLine("\t\t\tPyErr_Clear();");
				helperBuilder.AppendLine("\t\t}");
			}

			helperBuilder.AppendLine($"\t\tUPyUtil::SetPythonError(PyExc_RuntimeError, TEXT(\"{aggregatorErrorContext}\"), TEXT(\"operator not supported\"));");
			helperBuilder.AppendLine($"\t\treturn {finalFailureReturn};");
			helperBuilder.AppendLine("\t}");
			helperBuilder.AppendLine();

			assignmentLines.Add($"PyNumber.{mapping.PyNumberField} = ({mapping.DelegateType})&{helperName}::{finalFunctionName};");
			generatedAnyOperators = true;
		}

		if (!generatedAnyOperators)
		{
			return new NumberFuncCode(String.Empty, String.Empty, numberStructName, false);
		}

		helperBuilder.AppendLine("};");

		StringBuilder initStatementBuilder = new();
		initStatementBuilder.AppendLine("\tstatic PyNumberMethods PyNumber;");
		foreach (string assignment in assignmentLines)
		{
			initStatementBuilder.AppendLine($"\t{assignment}");
		}
		initStatementBuilder.AppendLine("\tPyType->tp_as_number = &PyNumber;");
		initStatementBuilder.AppendLine();

		return new NumberFuncCode(helperBuilder.ToString(), initStatementBuilder.ToString(), numberStructName, false);
	}

	private static string? GetBinaryOperatorCallExpression(string methodName, string selfExpression, string rhsExpression)
	{
		return methodName switch
		{
			"__add__" => $"{selfExpression}->operator+({rhsExpression})",
			"__iadd__" => $"{selfExpression}->operator+=({rhsExpression})",
			"__sub__" => $"{selfExpression}->operator-({rhsExpression})",
			"__isub__" => $"{selfExpression}->operator-=({rhsExpression})",
			"__mul__" => $"{selfExpression}->operator*({rhsExpression})",
			"__imul__" => $"{selfExpression}->operator*=({rhsExpression})",
			"__truediv__" => $"{selfExpression}->operator/({rhsExpression})",
			"__itruediv__" => $"{selfExpression}->operator/=({rhsExpression})",
			"__mod__" => $"{selfExpression}->operator%({rhsExpression})",
			"__imod__" => $"{selfExpression}->operator%=({rhsExpression})",
			"__and__" => $"{selfExpression}->operator&({rhsExpression})",
			"__iand__" => $"{selfExpression}->operator&=({rhsExpression})",
			"__or__" => $"{selfExpression}->operator|({rhsExpression})",
			"__ior__" => $"{selfExpression}->operator|=({rhsExpression})",
			"__xor__" => $"{selfExpression}->operator^({rhsExpression})",
			"__ixor__" => $"{selfExpression}->operator^=({rhsExpression})",
			"__lshift__" => $"{selfExpression}->operator<<({rhsExpression})",
			"__ilshift__" => $"{selfExpression}->operator<<=({rhsExpression})",
			"__rshift__" => $"{selfExpression}->operator>>({rhsExpression})",
			"__irshift__" => $"{selfExpression}->operator>>=({rhsExpression})",
			"__eq__" => $"*({selfExpression}) == {rhsExpression}",
			"__ne__" => $"*({selfExpression}) != {rhsExpression}",
			_ => null
		};
	}

	private static string? GetUnaryOperatorCallExpression(string methodName, string selfExpression)
	{
		return methodName switch
		{
			"__neg__" => $"{selfExpression}->operator-()",
			"__bool__" => $"{selfExpression}->operator bool()",
			_ => null
		};
	}

	private sealed class GetSetCode
	{
		public GetSetCode(string helper, string table, string tableName, bool hasProperties)
		{
			Helper = helper;
			Table = table;
			TableName = tableName;
			HasProperties = hasProperties;
		}

		public string Helper { get; }
		public string Table { get; }
		public string TableName { get; }
		public bool HasProperties { get; }

		public string Combine()
		{
			if (String.IsNullOrWhiteSpace(Helper))
			{
				return Table;
			}
			if (String.IsNullOrWhiteSpace(Table))
			{
				return Helper;
			}
			return $"{Helper}\n\n{Table}";
		}
	}

	private sealed class MethodCode
	{
		public MethodCode(string helper, string table, string tableName, string constants, string constantInit, bool requiresConstantSupport)
		{
			Helper = helper;
			Table = table;
			TableName = tableName;
			Constants = constants;
			ConstantInit = constantInit;
			RequiresConstantSupport = requiresConstantSupport;
		}

		public string Helper { get; }
		public string Table { get; }
		public string TableName { get; }
		public string Constants { get; }
		public string ConstantInit { get; }
		public bool RequiresConstantSupport { get; }

		public string Combine()
		{
			List<string> sections = new();
			if (!String.IsNullOrWhiteSpace(Helper))
			{
				sections.Add(Helper);
			}
			if (!String.IsNullOrWhiteSpace(Constants))
			{
				sections.Add(Constants);
			}
			if (!String.IsNullOrWhiteSpace(Table))
			{
				sections.Add(Table);
			}
			return String.Join("\n\n", sections);
		}
	}

	private sealed class NumberFuncCode
	{
		public NumberFuncCode(string helper, string initStatements, string numberStructName, bool requiresHelperInclude)
		{
			Helper = helper;
			InitStatements = initStatements;
			NumberStructName = numberStructName;
			RequiresHelperInclude = requiresHelperInclude;
		}

		public string Helper { get; }
		public string InitStatements { get; }
		public string NumberStructName { get; }
		public bool RequiresHelperInclude { get; }

		public string Combine()
		{
			return Helper;
		}
	}

	private sealed class StructInitFunctionCode
	{
		public StructInitFunctionCode(string definition, string functionName, int argumentCount)
		{
			Definition = definition;
			FunctionName = functionName;
			ArgumentCount = argumentCount;
		}

		public string Definition { get; }
		public string FunctionName { get; }
		public int ArgumentCount { get; }
	}

	private sealed class MethodFunctionCode
	{
		public MethodFunctionCode(string definition, string functionName, int argumentCount)
		{
			Definition = definition;
			FunctionName = functionName;
			ArgumentCount = argumentCount;
		}

		public string Definition { get; }
		public string FunctionName { get; }
		public int ArgumentCount { get; }
	}

	private sealed class InitParamSnippet
	{
		public InitParamSnippet(string declaration, string conversion, string argumentExpression)
		{
			Declaration = declaration;
			Conversion = conversion;
			ArgumentExpression = argumentExpression;
		}

		public string Declaration { get; }
		public string Conversion { get; }
		public string ArgumentExpression { get; }
	}

	private static string ResolveNativeMethodName(MethodJsonInfo method)
	{
		if (String.IsNullOrEmpty(method.HostHelper) && !String.IsNullOrEmpty(method.RawMethodName))
		{
			return method.RawMethodName;
		}
		if (!String.IsNullOrEmpty(method.HostHelper) && !String.IsNullOrEmpty(method.RawMethodName))
		{
			return $"{method.HostHelper}::{method.RawMethodName}";
		}
		return method.MethodName;
	}

	private static GetSetCode BuildStructGetSetDefinitions(string cppType, string rawType, string wrapperSuffix, IReadOnlyList<PropJsonInfo> properties, AutoGenStructConfig config)
	{
		string helperName = $"FGetSets_{wrapperSuffix}";
		string tableName = $"{cppType}GetSets";

		if (properties.Count == 0)
		{
			string helper = $"// {cppType} does not expose struct properties.";
			return new GetSetCode(helper, String.Empty, tableName, false);
		}

		Dictionary<string, int> nameCounts = new(StringComparer.Ordinal);
		StringBuilder helperBuilder = new();
		helperBuilder.AppendLine($"struct {helperName}");
		helperBuilder.AppendLine("{");

		StringBuilder tableBuilder = new();
		tableBuilder.AppendLine($"static PyGetSetDef {tableName}[] = {{");

		foreach (PropJsonInfo property in properties)
		{
			bool isEditorOnly = IsEditorOnlyProperty(property);
			if (isEditorOnly)
			{
				helperBuilder.AppendLine("#if WITH_EDITOR");
			}

			string accessorBase = SanitizeIdentifier(property.PropName, "Property");
			nameCounts.TryGetValue(accessorBase, out int count);
			nameCounts[accessorBase] = count + 1;
			string accessorSuffix = count == 0 ? accessorBase : $"{accessorBase}_{count}";

			string pythonAttrNameLiteral = EscapeForCxxLiteral(property.PropName);
			string enginePropNameLiteral = EscapeForCxxLiteral(property.RawPropName);
			string propertyDefName = $"GetPropertyDef_{accessorSuffix}";
			string getterName = $"Get{accessorSuffix}";
			string setterName = $"Set{accessorSuffix}";
			string memberAccess = $"InSelf->ValuePtr()->{property.RawPropName}";

			bool useDirectAccessor = TryBuildDirectAccessor(rawType, property, memberAccess, config.ForceReflectionProperties, out string getterBody, out string setterBody);

			if (!useDirectAccessor)
			{
				helperBuilder.AppendLine($"\tstatic UPyGenUtil::FGeneratedWrappedProperty& {propertyDefName}()");
				helperBuilder.AppendLine("\t{");
				helperBuilder.AppendLine("\t\tstatic bool bInitialized = false;");
				helperBuilder.AppendLine("\t\tstatic UPyGenUtil::FGeneratedWrappedProperty Property;");
				helperBuilder.AppendLine("\t\tif (!bInitialized)");
				helperBuilder.AppendLine("\t\t{");
				helperBuilder.AppendLine($"\t\t\tif (const UScriptStruct* ScriptStruct = TBaseStructure<{rawType}>::Get())");
				helperBuilder.AppendLine("\t\t\t{");
				helperBuilder.AppendLine($"\t\t\t\tif (const FProperty* FoundProperty = ScriptStruct->FindPropertyByName(TEXT(\"{enginePropNameLiteral}\")))");
				helperBuilder.AppendLine("\t\t\t\t{");
				helperBuilder.AppendLine("\t\t\t\t\tProperty.SetProperty(FoundProperty);");
				helperBuilder.AppendLine("\t\t\t\t}");
				helperBuilder.AppendLine("\t\t\t}");
				helperBuilder.AppendLine("\t\t\tbInitialized = true;");
				helperBuilder.AppendLine("\t\t}");
				helperBuilder.AppendLine("\t\treturn Property;");
				helperBuilder.AppendLine("\t}");
				helperBuilder.AppendLine();

				if (property.PropType == "ArrayProperty")
				{
					getterBody = $"return (PyObject*)FUPyWrapperArrayFactory::Get().CreateInstance((void*)&{memberAccess}, CastField<FArrayProperty>({propertyDefName}().Prop), FUPyWrapperOwnerContext((PyObject*)InSelf), EUPyConversionMethod::Reference);";
				}
				else if (property.PropType == "SetProperty")
				{
					getterBody = $"return (PyObject*)FUPyWrapperSetFactory::Get().CreateInstance((void*)&{memberAccess}, CastField<FSetProperty>({propertyDefName}().Prop), FUPyWrapperOwnerContext((PyObject*)InSelf), EUPyConversionMethod::Reference);";
				}
				else if (property.PropType == "MapProperty")
				{
					getterBody = $"return (PyObject*)FUPyWrapperMapFactory::Get().CreateInstance((void*)&{memberAccess}, CastField<FMapProperty>({propertyDefName}().Prop), FUPyWrapperOwnerContext((PyObject*)InSelf), EUPyConversionMethod::Reference);";
				}
				else
				{
					getterBody =
						$"return FUPyWrapperStruct::GetPropertyValue(InSelf, {propertyDefName}(), \"{pythonAttrNameLiteral}\");";
				}

				setterBody =
					$"return FUPyWrapperStruct::SetPropertyValue(InSelf, InValue, {propertyDefName}(), \"{pythonAttrNameLiteral}\");";
			}

			helperBuilder.AppendLine($"\tstatic PyObject* {getterName}({cppType}* InSelf, void* InClosure)");
			helperBuilder.AppendLine("\t{");
			helperBuilder.AppendLine($"\t\tif (!{cppType}::ValidateInternalState(InSelf))");
			helperBuilder.AppendLine("\t\t{");
			helperBuilder.AppendLine("\t\t\treturn nullptr;");
			helperBuilder.AppendLine("\t\t}");
			helperBuilder.AppendLine(IndentCode(getterBody.TrimEnd(), 2));
			helperBuilder.AppendLine("\t}");
			helperBuilder.AppendLine();

			bool bIsReadOnly = (property.PropFlags & 0x0000000000010000) != 0; // CPF_DisableEditOnInstance

			if (!bIsReadOnly)
			{
				helperBuilder.AppendLine($"\tstatic int {setterName}({cppType}* InSelf, PyObject* InValue, void* InClosure)");
				helperBuilder.AppendLine("\t{");
				helperBuilder.AppendLine($"\t\tif (!{cppType}::ValidateInternalState(InSelf))");
				helperBuilder.AppendLine("\t\t{");
				helperBuilder.AppendLine("\t\t\treturn -1;");
				helperBuilder.AppendLine("\t\t}");
				helperBuilder.AppendLine(IndentCode(setterBody.TrimEnd(), 2));
				helperBuilder.AppendLine("\t}");
				helperBuilder.AppendLine();
			}

			if (isEditorOnly)
			{
				helperBuilder.AppendLine("#endif");
			}

			string setterRef = bIsReadOnly ? "nullptr" : $"(setter)&{helperName}::{setterName}";
			
			if (isEditorOnly)
			{
				tableBuilder.AppendLine("#if WITH_EDITOR");
			}
			tableBuilder.AppendLine($"\t{{ UPyCStrCast(\"{pythonAttrNameLiteral}\"), (getter)&{helperName}::{getterName}, {setterRef}, nullptr, nullptr }},");
			if (isEditorOnly)
			{
				tableBuilder.AppendLine("#endif");
			}
		}

		helperBuilder.AppendLine("};");
		tableBuilder.AppendLine("\t{ nullptr }");
		tableBuilder.AppendLine("};");

		return new GetSetCode(helperBuilder.ToString(), tableBuilder.ToString(), tableName, true);
	}

	private static bool IsStaticMethod(MethodJsonInfo method)
	{
		EFunctionFlags flags = (EFunctionFlags)method.MethodFlags;
		return (flags & EFunctionFlags.Static) != 0;
	}

	private static bool IsReturnParameter(PropJsonInfo param)
	{
		EPropertyFlags flags = (EPropertyFlags)param.PropFlags;
		return (flags & EPropertyFlags.ReturnParm) != 0;
	}

	private static bool IsInputParameter(PropJsonInfo param)
	{
		EPropertyFlags flags = (EPropertyFlags)param.PropFlags;
		bool isParm = (flags & EPropertyFlags.Parm) != 0;
		bool isReturn = (flags & EPropertyFlags.ReturnParm) != 0;
		bool isReference = (flags & EPropertyFlags.ReferenceParm) != 0;
		bool isOut = (flags & EPropertyFlags.OutParm) != 0 && (flags & EPropertyFlags.ConstParm) == 0;
		return isParm && !isReturn && (!isOut || isReference);
	}

	private static bool IsOutputParameter(PropJsonInfo param)
	{
		EPropertyFlags flags = (EPropertyFlags)param.PropFlags;
		bool isParm = (flags & EPropertyFlags.Parm) != 0;
		bool isReturn = (flags & EPropertyFlags.ReturnParm) != 0;
		bool isOut = (flags & EPropertyFlags.OutParm) != 0 && (flags & EPropertyFlags.ConstParm) == 0;
		return isParm && !isReturn && isOut;
	}

	private static bool TryBuildOutputParameterDeclaration(PropJsonInfo param, string varName, out string declaration)
	{
		switch (param.PropType)
		{
			case "BoolProperty":
				declaration = $"bool {varName} = false;";
				return true;

			case "EnumProperty":
				{
					string? nativeType = ResolveNativeTypeName(param.TypeName);
					if (String.IsNullOrEmpty(nativeType))
					{
						nativeType = !String.IsNullOrEmpty(param.TypeName) ? param.TypeName : "uint8";
					}
					declaration = $"{nativeType} {varName};";
					return true;
				}

			case "ByteProperty":
				{
					string? nativeType = ResolveNativeTypeName(param.TypeName);
					if (String.IsNullOrEmpty(nativeType) && !String.IsNullOrEmpty(param.TypeName))
					{
						nativeType = param.TypeName;
					}

					if (!String.IsNullOrEmpty(nativeType))
					{
						declaration = $"{nativeType} {varName};";
						return true;
					}
					declaration = $"uint8 {varName} = 0;";
					return true;
				}

			case "UInt8Property":
				declaration = $"uint8 {varName} = 0;";
				return true;

			case "Int8Property":
				declaration = $"int8 {varName} = 0;";
				return true;

			case "Int16Property":
				declaration = $"int16 {varName} = 0;";
				return true;

			case "IntProperty":
			case "Int32Property":
				declaration = $"int32 {varName} = 0;";
				return true;

			case "Int64Property":
				declaration = $"int64 {varName} = 0;";
				return true;

			case "UInt16Property":
				declaration = $"uint16 {varName} = 0;";
				return true;

			case "UInt32Property":
				declaration = $"uint32 {varName} = 0;";
				return true;

			case "UInt64Property":
				declaration = $"uint64 {varName} = 0;";
				return true;

			case "FloatProperty":
				declaration = $"float {varName} = 0.0f;";
				return true;

			case "DoubleProperty":
			case "LargeWorldCoordinatesRealProperty":
				declaration = $"double {varName} = 0.0;";
				return true;

			case "NameProperty":
				declaration = $"FName {varName};";
				return true;

			case "StrProperty":
				declaration = $"FString {varName};";
				return true;

			case "TextProperty":
				declaration = $"FText {varName};";
				return true;

			case "ArrayProperty":
				{
					string? innerType = ResolveContainerElementType(param.TypeName);
					if (String.IsNullOrEmpty(innerType))
					{
						declaration = String.Empty;
						return false;
					}
					declaration = $"TArray<{innerType}> {varName};";
					return true;
				}

			case "SetProperty":
				{
					string? innerType = ResolveContainerElementType(param.TypeName);
					if (String.IsNullOrEmpty(innerType))
					{
						declaration = String.Empty;
						return false;
					}
					declaration = $"TSet<{innerType}> {varName};";
					return true;
				}

			case "MapProperty":
				{
					string[] parts = param.TypeName?.Split(new[] { "->" }, StringSplitOptions.None) ?? Array.Empty<string>();
					if (parts.Length != 2)
					{
						declaration = String.Empty;
						return false;
					}

					string? keyType = ResolveContainerElementType(parts[0]);
					string? valueType = ResolveContainerElementType(parts[1]);

					if (String.IsNullOrEmpty(keyType) || String.IsNullOrEmpty(valueType))
					{
						declaration = String.Empty;
						return false;
					}
					declaration = $"TMap<{keyType}, {valueType}> {varName};";
					return true;
				}

			case "StructProperty":
				{
					string? nativeType = ResolveNativeTypeName(param.TypeName);
					if (String.IsNullOrEmpty(nativeType))
					{
						declaration = String.Empty;
						return false;
					}
					declaration = $"{nativeType} {varName};";
					return true;
				}

			case "ObjectProperty":
			case "SoftObjectProperty":
			case "ClassProperty":
			case "SoftClassProperty":
				{
					string? nativeType = ResolveNativeTypeName(param.TypeName);
					if (String.IsNullOrEmpty(nativeType))
					{
						declaration = String.Empty;
						return false;
					}
					declaration = $"{nativeType}* {varName} = nullptr;";
					return true;
				}

			default:
				declaration = String.Empty;
				return false;
		}
	}

	private sealed class ConstantCode
	{
		public ConstantCode(string definitions, string initStatements)
		{
			Definitions = definitions;
			InitStatements = initStatements;
		}

		public string Definitions { get; }
		public string InitStatements { get; }
	}

	private static ConstantCode BuildConstantDefinitions(string rawType, string wrapperSuffix, IReadOnlyList<MethodJsonInfo> methods)
	{
		StringBuilder constantBuilder = new();
		List<string> constantEntries = new();
		bool hasConstants = false;
		Dictionary<string, int> nameCounts = new(StringComparer.Ordinal);

		foreach (MethodJsonInfo method in methods)
		{
			if (String.IsNullOrWhiteSpace(method.MethodName) || method.bIsOperator || method.MethodName.Equals("__init__", StringComparison.Ordinal))
			{
				continue;
			}
			if (!IsMethodExternallyAccessible(method))
			{
				continue;
			}
			if (!method.bIsConstant)
			{
				continue;
			}

			string accessorBase = SanitizeIdentifier(method.MethodName, "Method");
			nameCounts.TryGetValue(accessorBase, out int count);
			nameCounts[accessorBase] = count + 1;
			string accessorSuffix = count == 0 ? accessorBase : $"{accessorBase}_{count}";

			string docLiteral = FormatDocString(method.MethodDoc);
			PropJsonInfo? returnParam = method.Params.FirstOrDefault(IsReturnParameter);
			string nativeMethodName = ResolveNativeMethodName(method);

			if (!hasConstants)
			{
				constantBuilder.AppendLine($"struct FConstants_{wrapperSuffix}");
				constantBuilder.AppendLine("{");
			}

			string constantGetterName = $"GetConstant_{accessorSuffix}";
			constantBuilder.AppendLine($"\tstatic PyObject* {constantGetterName}(PyTypeObject* InType, const void* InContext)");
			constantBuilder.AppendLine("\t{");

			string constantExpression;
			if (method.Params.Count > 0 && method.Params[0].PropType == "StructProperty" && !String.IsNullOrEmpty(method.Params[0].TypeName))
			{
				constantExpression = $"{rawType}::{method.MethodName}";
			}
			else
			{
				constantExpression = !String.IsNullOrEmpty(method.HostHelper)
					? $"{nativeMethodName}()"
					: $"{rawType}::{nativeMethodName}()";
			}

			if (returnParam == null)
			{
				constantBuilder.AppendLine("\t\tPy_RETURN_NONE;");
			}
			else if (returnParam.PropType.Equals("BoolProperty", StringComparison.Ordinal))
			{
				constantBuilder.AppendLine($"\t\tconst bool bResult = {constantExpression};");
				constantBuilder.AppendLine("\t\treturn PyBool_FromLong(bResult ? 1 : 0);");
			}
			else if (returnParam.PropType.Equals("StructProperty", StringComparison.Ordinal))
			{
				constantBuilder.AppendLine($"\t\tconst auto Result = {constantExpression};");
				constantBuilder.AppendLine("\t\treturn UPyConversion::PythonizeStructInstance(Result);");
			}
			else
			{
				constantBuilder.AppendLine($"\t\tconst auto Result = {constantExpression};");
				constantBuilder.AppendLine("\t\treturn UPyConversion::Pythonize(Result);");
			}

			constantBuilder.AppendLine("\t}");
			constantBuilder.AppendLine();

			string constantNameLiteral = EscapeForCxxLiteral(method.MethodName);
			constantEntries.Add($"\t\t{{ nullptr, (PyConstantGetter)&FConstants_{wrapperSuffix}::{constantGetterName}, UPyCStrCast(\"{constantNameLiteral}\"), {docLiteral} }},");
			hasConstants = true;
		}

		string constantDefs = String.Empty;
		string constantInit = String.Empty;

		if (hasConstants)
		{
			constantBuilder.AppendLine("\tstatic FUPyConstantDef* Table()");
			constantBuilder.AppendLine("\t{");
			constantBuilder.AppendLine("\t\tstatic FUPyConstantDef Constants[] = {");
			foreach (string entry in constantEntries)
			{
				constantBuilder.AppendLine(entry);
			}
			constantBuilder.AppendLine("\t\t\t{ nullptr, nullptr, nullptr, nullptr }");
			constantBuilder.AppendLine("\t\t};");
			constantBuilder.AppendLine("\t\treturn Constants;");
			constantBuilder.AppendLine("\t}");
			constantBuilder.AppendLine("};");

			constantDefs = constantBuilder.ToString();
			constantInit = $"\t\tFUPyConstantDef::AddConstantsToType(FConstants_{wrapperSuffix}::Table(), PyType);\n";
		}

		return new ConstantCode(constantDefs, constantInit);
	}

	private static MethodCode BuildStructMethodDefinitions(string cppType, string rawType, string wrapperSuffix, IReadOnlyList<MethodJsonInfo> methods, HashSet<string>? forceReflectionMethods)
	{
		string helperName = $"FMethods_{wrapperSuffix}";
		string tableName = $"{cppType}PyMethodDefs";

		if (methods.Count == 0)
		{
			return new MethodCode(String.Empty, String.Empty, String.Empty, String.Empty, String.Empty, false);
		}

		Dictionary<string, int> nameCounts = new(StringComparer.Ordinal);
		StringBuilder helperBuilder = new();
		helperBuilder.AppendLine($"struct {helperName}");
		helperBuilder.AppendLine("{");

		StringBuilder tableBuilder = new();
		tableBuilder.AppendLine($"static PyMethodDef {tableName}[] = {{");

		bool hasAnyMethods = false;

		var methodGroups = methods
			.Where(m => !String.IsNullOrWhiteSpace(m.MethodName) && !m.bIsOperator && !m.MethodName.Equals("__init__", StringComparison.Ordinal) && !m.bIsConstant)
			.GroupBy(m => m.MethodName, StringComparer.Ordinal);

		foreach (var group in methodGroups)
		{
			string methodName = group.Key;
			List<MethodJsonInfo> overloads = group.ToList();
			bool isOverloaded = overloads.Count > 1;
			List<MethodFunctionCode> validImpls = new();
			Dictionary<int, int> argCountToOverloadIndex = new();

			foreach (MethodJsonInfo method in overloads)
			{

			bool isEditorOnly = IsEditorOnlyMethod(method) && !method.bIsScriptMethod;
			if (isEditorOnly)
			{
				helperBuilder.AppendLine("#if WITH_EDITOR");
			}

			string accessorBase = SanitizeIdentifier(method.MethodName, "Method");
			List<PropJsonInfo> inputParams = method.Params.Where(IsInputParameter).ToList();
			int parameterCount = inputParams.Count;

			string methodFunctionName;
			if (isOverloaded)
			{
				if (!argCountToOverloadIndex.ContainsKey(parameterCount))
				{
					argCountToOverloadIndex[parameterCount] = 0;
				}
				int overloadIndex = argCountToOverloadIndex[parameterCount]++;
				methodFunctionName = $"Call{accessorBase}_ArgCnt{parameterCount}_Overload{overloadIndex}";
			}
			else
			{
				nameCounts.TryGetValue(accessorBase, out int count);
				nameCounts[accessorBase] = count + 1;
				string accessorSuffix = count == 0 ? accessorBase : $"{accessorBase}_{count}";
				methodFunctionName = $"Call{accessorSuffix}";
			}

			string pythonNameLiteral = EscapeForCxxLiteral(method.MethodName);
			string errorContextLiteral = EscapeForCxxLiteral(BuildPropertyErrorContext(rawType, method.MethodName));
			string docLiteral = FormatDocString(method.MethodDoc);
			List<PropJsonInfo> outputParams = method.Params.Where(param => !IsReturnParameter(param) && IsOutputParameter(param)).ToList();
			PropJsonInfo? returnParam = method.Params.FirstOrDefault(IsReturnParameter);
			bool isStaticMethod = IsStaticMethod(method);
			string nativeMethodName = ResolveNativeMethodName(method);
			bool generatedBinding = true;
			List<string> argumentExpressions = new();
			StringBuilder methodBuilder = new();
			Dictionary<string, string> outputParamMap = new();

			bool hasDefaultValues = inputParams.Any(p => p.DefaultValue != null);
			bool useVarargs = isOverloaded || parameterCount > 1 || (parameterCount == 1 && hasDefaultValues);

			if (generatedBinding)
			{
				string methodSignature = parameterCount == 0
					? $"{cppType}* InSelf, PyObject* Py_UNUSED(InUnused)"
					: !useVarargs
						? $"{cppType}* InSelf, PyObject* InArg"
						: $"{cppType}* InSelf, PyObject* InArgs";
				methodBuilder.AppendLine($"\tstatic PyObject* {methodFunctionName}({methodSignature})");
				methodBuilder.AppendLine("\t{");
				if (!isStaticMethod)
				{
					methodBuilder.AppendLine("\t\tif (!InSelf->ValidateInternalState(InSelf))");
					methodBuilder.AppendLine("\t\t{");
					methodBuilder.AppendLine("\t\t\treturn nullptr;");
					methodBuilder.AppendLine("\t\t}");
				}

				if (useVarargs && hasDefaultValues)
				{
					methodBuilder.AppendLine("\t\tPy_ssize_t ArgCount = PyTuple_GET_SIZE(InArgs);");
				}

				// Process input parameters
				string? previousPyArgName = null;
				bool previousWasOptional = false;
				for (int paramIndex = 0; paramIndex < parameterCount; ++paramIndex)
				{
					bool isOptional = inputParams[paramIndex].DefaultValue != null;
					string pyArgName;
					if (!useVarargs)
					{
						pyArgName = "InArg";
					}
					else
					{
						pyArgName = $"PyArg{paramIndex}";
						
						if (isOptional)
						{
							if (previousPyArgName != null && previousWasOptional)
							{
								methodBuilder.AppendLine($"\t\tPyObject* {pyArgName} = ({previousPyArgName} && ArgCount > {paramIndex}) ? PyTuple_GetItem(InArgs, {paramIndex}) : nullptr;");
							}
							else
							{
								methodBuilder.AppendLine($"\t\tPyObject* {pyArgName} = (ArgCount > {paramIndex}) ? PyTuple_GetItem(InArgs, {paramIndex}) : nullptr;");
							}
						}
						else
						{
							methodBuilder.AppendLine($"\t\tPyObject* {pyArgName} = PyTuple_GetItem(InArgs, {paramIndex});");
							methodBuilder.AppendLine($"\t\tif ({pyArgName} == nullptr)");
							methodBuilder.AppendLine("\t\t{");
							methodBuilder.AppendLine("\t\t\treturn nullptr;");
							methodBuilder.AppendLine("\t\t}");
						}
					}

					if (!TryBuildOperatorArgumentConversion(rawType, pyArgName, errorContextLiteral, paramIndex, inputParams[paramIndex], "nullptr", false, out string conversionCode, out string argumentExpression))
					{
						generatedBinding = false;
						break;
					}

					methodBuilder.AppendLine(conversionCode);
					methodBuilder.AppendLine();
					argumentExpressions.Add(argumentExpression);
					previousPyArgName = pyArgName;
					previousWasOptional = isOptional;
				}

				if (rawType.Equals("UGameplayStatics", StringComparison.Ordinal) && nativeMethodName.Equals("SpawnObject", StringComparison.Ordinal))
				{
					methodBuilder.AppendLine("\t\tif (Arg0 && Arg0->HasAnyClassFlags(CLASS_Abstract))");
					methodBuilder.AppendLine("\t\t{");
					methodBuilder.AppendLine("\t\t\tUPyUtil::SetPythonError(PyExc_Exception, TEXT(\"GameplayStatics::SpawnObject\"), *FString::Printf(TEXT(\"Class '%s' is abstract\"), *Arg0->GetName()));");
					methodBuilder.AppendLine("\t\t\treturn nullptr;");
					methodBuilder.AppendLine("\t\t}");
					methodBuilder.AppendLine();
				}

				if (generatedBinding)
				{
					// Declare output parameter variables and build a map for later
					for (int outIndex = 0; outIndex < outputParams.Count; ++outIndex)
					{
						PropJsonInfo outParam = outputParams[outIndex];
						
						// Check if this is also an input parameter (Reference)
						if (IsInputParameter(outParam))
						{
							int inputIndex = inputParams.IndexOf(outParam);
							if (inputIndex >= 0 && inputIndex < argumentExpressions.Count)
							{
								outputParamMap[outParam.PropName] = argumentExpressions[inputIndex];
								continue;
							}
						}

						string outVarName = $"OutParam{outIndex}";

						if (!TryBuildOutputParameterDeclaration(outParam, outVarName, out string declaration))
						{
							generatedBinding = false;
							break;
						}

						methodBuilder.AppendLine($"\t\t{declaration}");
						outputParamMap[outParam.PropName] = outVarName;
					}
				}

				if (generatedBinding)
				{
					// Build argument list in the correct order based on original parameter order
					List<string> orderedArguments = new();
					int inputParamIndex = 0;

					foreach (PropJsonInfo param in method.Params)
					{
						if (IsReturnParameter(param))
						{
							continue;
						}
						else if (IsOutputParameter(param))
						{
							if (outputParamMap.TryGetValue(param.PropName, out string? outVarName))
							{
								orderedArguments.Add(outVarName);
							}

							if (IsInputParameter(param))
							{
								inputParamIndex++;
							}
						}
						else if (IsInputParameter(param))
						{
							if (inputParamIndex < argumentExpressions.Count)
							{
								orderedArguments.Add(argumentExpressions[inputParamIndex]);
								inputParamIndex++;
							}
						}
					}

					string argumentList = orderedArguments.Count > 0
						? String.Join(", ", orderedArguments)
						: String.Empty;
					string callExpression;

					if (method.MethodName.Equals("__neg__", StringComparison.Ordinal))
					{
						callExpression = $"-({(String.IsNullOrEmpty(argumentList) ? "*InSelf->ValuePtr()" : argumentList)})";
					}
					else if (method.MethodName.Equals("__str__", StringComparison.Ordinal))
					{
						callExpression = "InSelf->ValuePtr()->ToString()";
					}
					else if (isStaticMethod)
					{
						if (!String.IsNullOrEmpty(method.HostHelper))
						{
							callExpression = $"{nativeMethodName}({argumentList})";
						}
						else
						{
							callExpression = $"{rawType}::{nativeMethodName}({argumentList})";
						}
					}
					else
					{
						if (!String.IsNullOrEmpty(method.HostHelper))
						{
							bool isClass = ClassLookupCache.ContainsKey(rawType);
							string selfArg = isClass ? "InSelf->ValuePtr()" : "*InSelf->ValuePtr()";
							string args = String.IsNullOrEmpty(argumentList) ? selfArg : $"{selfArg}, {argumentList}";
							callExpression = $"{nativeMethodName}({args})";
						}
						else
						{
							callExpression = $"InSelf->ValuePtr()->{nativeMethodName}({argumentList})";
						}
					}

					// Build return statement based on return param and output params
					int totalReturns = (returnParam != null ? 1 : 0) + outputParams.Count;

					bool isForcedReflection = !IsMethodExternallyAccessible(method);
					if (forceReflectionMethods != null && MatchesConfiguredName(forceReflectionMethods, method))
					{
						isForcedReflection = true;
					}

					if (isForcedReflection)
					{
						methodBuilder.AppendLine($"\t\tstatic UFunction* Func = {rawType}::StaticClass()->FindFunctionByName(TEXT(\"{nativeMethodName}\"));");
						
						HashSet<string> declaredProps = new HashSet<string>();
						foreach (var param in inputParams)
						{
							if (declaredProps.Add(param.RawPropName))
							{
								methodBuilder.AppendLine($"\t\tstatic FProperty* Prop_{param.RawPropName} = Func ? Func->FindPropertyByName(TEXT(\"{param.RawPropName}\")) : nullptr;");
							}
						}
						foreach (var param in outputParams)
						{
							if (declaredProps.Add(param.RawPropName))
							{
								methodBuilder.AppendLine($"\t\tstatic FProperty* Prop_{param.RawPropName} = Func ? Func->FindPropertyByName(TEXT(\"{param.RawPropName}\")) : nullptr;");
							}
						}
						if (returnParam != null)
						{
							methodBuilder.AppendLine($"\t\tstatic FProperty* Prop__ReturnValue = Func ? Func->GetReturnProperty() : nullptr;");
						}

						if (returnParam != null)
						{
							string resultVarName = totalReturns > 1 ? "ReturnValue" : "Result";
							if (TryBuildOutputParameterDeclaration(returnParam, resultVarName, out string declaration))
							{
								methodBuilder.AppendLine($"\t\t{declaration}");
							}
						}

						methodBuilder.AppendLine("\t\tif (Func)");
						methodBuilder.AppendLine("\t\t{");
						methodBuilder.AppendLine("\t\t\tuint8* Parms = (uint8*)FMemory::Malloc(Func->ParmsSize);");
						methodBuilder.AppendLine("\t\t\tFMemory::Memzero(Parms, Func->ParmsSize);");

						for (int i = 0; i < inputParams.Count; i++)
						{
							var param = inputParams[i];
							string varName = argumentExpressions[i];
							methodBuilder.AppendLine($"\t\t\tif (Prop_{param.RawPropName})");
							methodBuilder.AppendLine("\t\t\t{");
							methodBuilder.AppendLine($"\t\t\t\tProp_{param.RawPropName}->CopyCompleteValue_InContainer(Parms, &{varName});");
							methodBuilder.AppendLine("\t\t\t}");
						}

						if (isStaticMethod)
						{
							methodBuilder.AppendLine($"\t\t\tGetMutableDefault<{rawType}>()->ProcessEvent(Func, Parms);");
						}
						else
						{
							methodBuilder.AppendLine("\t\t\tInSelf->ValuePtr()->ProcessEvent(Func, Parms);");
						}

						foreach (var param in outputParams)
						{
							string varName = outputParamMap[param.PropName];
							methodBuilder.AppendLine($"\t\t\tif (Prop_{param.RawPropName})");
							methodBuilder.AppendLine("\t\t\t{");
							methodBuilder.AppendLine($"\t\t\t\tProp_{param.RawPropName}->CopyCompleteValue_InContainer(&{varName}, Parms);");
							methodBuilder.AppendLine("\t\t\t}");
						}

						if (returnParam != null)
						{
							string resultVarName = totalReturns > 1 ? "ReturnValue" : "Result";
							methodBuilder.AppendLine($"\t\t\tif (Prop__ReturnValue)");
							methodBuilder.AppendLine("\t\t\t{");
							methodBuilder.AppendLine($"\t\t\t\tProp__ReturnValue->CopyCompleteValue_InContainer(&{resultVarName}, Parms);");
							methodBuilder.AppendLine("\t\t\t}");
						}
						methodBuilder.AppendLine("\t\t\tFMemory::Free(Parms);");
						methodBuilder.AppendLine("\t\t}");

						// Packing for Forced Reflection
						if (totalReturns == 0)
						{
							methodBuilder.AppendLine("\t\tPy_RETURN_NONE;");
						}
						else if (totalReturns == 1)
						{
							if (returnParam != null)
							{
								if (returnParam.PropType.Equals("BoolProperty", StringComparison.Ordinal))
								{
									methodBuilder.AppendLine("\t\treturn PyBool_FromLong(Result ? 1 : 0);");
								}
								else if (returnParam.PropType.Equals("EnumProperty", StringComparison.Ordinal))
								{
									methodBuilder.AppendLine($"\t\treturn UPyConversion::PythonizeEnumEntry((int64)Result, StaticEnum<{GetEnumStaticType(returnParam.TypeName)}>());");
								}
								else if (returnParam.PropType.Equals("StructProperty", StringComparison.Ordinal))
								{
									methodBuilder.AppendLine("\t\treturn UPyConversion::PythonizeStructInstance(Result);");
								}
								else
								{
									methodBuilder.AppendLine(IndentCode(BuildContainerReturnSnippet("Result", returnParam), 2));
								}
							}
							else
							{
								PropJsonInfo outParam = outputParams[0];
								string outVarName = outputParamMap[outParam.PropName];

								if (outParam.PropType.Equals("BoolProperty", StringComparison.Ordinal))
								{
									methodBuilder.AppendLine($"\t\treturn PyBool_FromLong({outVarName} ? 1 : 0);");
								}
								else if (outParam.PropType.Equals("EnumProperty", StringComparison.Ordinal))
								{
									methodBuilder.AppendLine($"\t\treturn UPyConversion::PythonizeEnumEntry((int64){outVarName}, StaticEnum<{GetEnumStaticType(outParam.TypeName)}>());");
								}
								else if (outParam.PropType.Equals("StructProperty", StringComparison.Ordinal))
								{
									methodBuilder.AppendLine($"\t\treturn UPyConversion::PythonizeStructInstance({outVarName});");
								}
								else
								{
									methodBuilder.AppendLine(IndentCode(BuildContainerReturnSnippet(outVarName, outParam), 2));
								}
							}
						}
						else
						{
							methodBuilder.AppendLine($"\t\tPyObject* ReturnTuple = PyTuple_New({totalReturns});");
							methodBuilder.AppendLine("\t\tif (ReturnTuple == nullptr)");
							methodBuilder.AppendLine("\t\t{");
							methodBuilder.AppendLine("\t\t\treturn nullptr;");
							methodBuilder.AppendLine("\t\t}");
							methodBuilder.AppendLine();

							int tupleIndex = 0;

							if (returnParam != null)
							{
								if (returnParam.PropType.Equals("BoolProperty", StringComparison.Ordinal))
								{
									methodBuilder.AppendLine($"\t\tPyTuple_SET_ITEM(ReturnTuple, {tupleIndex}, PyBool_FromLong(ReturnValue ? 1 : 0));");
								}
								else if (returnParam.PropType.Equals("EnumProperty", StringComparison.Ordinal))
								{
									methodBuilder.AppendLine($"\t\tPyTuple_SET_ITEM(ReturnTuple, {tupleIndex}, UPyConversion::PythonizeEnumEntry((int64)ReturnValue, StaticEnum<{GetEnumStaticType(returnParam.TypeName)}>()));");
								}
								else if (returnParam.PropType.Equals("StructProperty", StringComparison.Ordinal))
								{
									methodBuilder.AppendLine($"\t\tPyTuple_SET_ITEM(ReturnTuple, {tupleIndex}, UPyConversion::PythonizeStructInstance(ReturnValue));");
								}
								else
								{
									methodBuilder.AppendLine(IndentCode(BuildContainerTupleItemSnippet("ReturnValue", returnParam, tupleIndex), 2));
								}
								tupleIndex++;
							}

							for (int outIndex = 0; outIndex < outputParams.Count; ++outIndex)
							{
								PropJsonInfo outParam = outputParams[outIndex];
								string outVarName = outputParamMap[outParam.PropName];

								if (outParam.PropType.Equals("BoolProperty", StringComparison.Ordinal))
								{
									methodBuilder.AppendLine($"\t\tPyTuple_SET_ITEM(ReturnTuple, {tupleIndex}, PyBool_FromLong({outVarName} ? 1 : 0));");
								}
								else if (outParam.PropType.Equals("EnumProperty", StringComparison.Ordinal))
								{
									methodBuilder.AppendLine($"\t\tPyTuple_SET_ITEM(ReturnTuple, {tupleIndex}, UPyConversion::PythonizeEnumEntry((int64){outVarName}, StaticEnum<{GetEnumStaticType(outParam.TypeName)}>()));");
								}
								else if (outParam.PropType.Equals("StructProperty", StringComparison.Ordinal))
								{
									methodBuilder.AppendLine($"\t\tPyTuple_SET_ITEM(ReturnTuple, {tupleIndex}, UPyConversion::PythonizeStructInstance({outVarName}));");
								}
								else
								{
									methodBuilder.AppendLine(IndentCode(BuildContainerTupleItemSnippet(outVarName, outParam, tupleIndex), 2));
								}
								tupleIndex++;
							}

							methodBuilder.AppendLine();
							methodBuilder.AppendLine("\t\treturn ReturnTuple;");
						}
					}
					else if (totalReturns == 0)
					{
						methodBuilder.AppendLine($"\t\t{callExpression};");
						methodBuilder.AppendLine("\t\tPy_RETURN_NONE;");
					}
					else if (totalReturns == 1)
					{
						if (returnParam != null)
						{
							// Single return value, no output params
							if (returnParam.PropType.Equals("BoolProperty", StringComparison.Ordinal))
							{
								methodBuilder.AppendLine($"\t\tconst bool bResult = {callExpression};");
								methodBuilder.AppendLine("\t\treturn PyBool_FromLong(bResult ? 1 : 0);");
							}
							else if (returnParam.PropType.Equals("EnumProperty", StringComparison.Ordinal))
							{
								methodBuilder.AppendLine($"\t\tconst auto Result = {callExpression};");
								methodBuilder.AppendLine($"\t\treturn UPyConversion::PythonizeEnumEntry((int64)Result, StaticEnum<{GetEnumStaticType(returnParam.TypeName)}>());");
							}
							else if (returnParam.PropType.Equals("StructProperty", StringComparison.Ordinal))
							{
								methodBuilder.AppendLine($"\t\tconst auto Result = {callExpression};");
								methodBuilder.AppendLine("\t\treturn UPyConversion::PythonizeStructInstance(Result);");
							}
							else
							{
								methodBuilder.AppendLine($"\t\tconst auto Result = {callExpression};");
								methodBuilder.AppendLine(IndentCode(BuildContainerReturnSnippet("Result", returnParam), 2));
							}
						}
						else
						{
							// Single output param, no return value
							methodBuilder.AppendLine($"\t\t{callExpression};");
							
							PropJsonInfo outParam = outputParams[0];
							string outVarName = outputParamMap[outParam.PropName];

							if (outParam.PropType.Equals("BoolProperty", StringComparison.Ordinal))
							{
								methodBuilder.AppendLine($"\t\treturn PyBool_FromLong({outVarName} ? 1 : 0);");
							}
							else if (outParam.PropType.Equals("EnumProperty", StringComparison.Ordinal))
							{
								methodBuilder.AppendLine($"\t\treturn UPyConversion::PythonizeEnumEntry((int64){outVarName}, StaticEnum<{GetEnumStaticType(outParam.TypeName)}>());");
							}
							else if (outParam.PropType.Equals("StructProperty", StringComparison.Ordinal))
							{
								methodBuilder.AppendLine($"\t\treturn UPyConversion::PythonizeStructInstance({outVarName});");
							}
							else
							{
								methodBuilder.AppendLine(IndentCode(BuildContainerReturnSnippet(outVarName, outParam), 2));
							}
						}
					}
					else
					{
						// Multiple return values: use tuple
						methodBuilder.AppendLine($"\t\t// Call the method");
						if (returnParam != null)
						{
							if (returnParam.PropType.Equals("BoolProperty", StringComparison.Ordinal))
							{
								methodBuilder.AppendLine($"\t\tconst bool ReturnValue = {callExpression};");
							}
							else if (returnParam.PropType.Equals("StructProperty", StringComparison.Ordinal))
							{
								methodBuilder.AppendLine($"\t\tconst auto ReturnValue = {callExpression};");
							}
							else
							{
								methodBuilder.AppendLine($"\t\tconst auto ReturnValue = {callExpression};");
							}
						}
						else
						{
							methodBuilder.AppendLine($"\t\t{callExpression};");
						}

						methodBuilder.AppendLine();
						methodBuilder.AppendLine($"\t\t// Build return tuple with {totalReturns} values");
						methodBuilder.AppendLine($"\t\tPyObject* ReturnTuple = PyTuple_New({totalReturns});");
						methodBuilder.AppendLine("\t\tif (ReturnTuple == nullptr)");
						methodBuilder.AppendLine("\t\t{");
						methodBuilder.AppendLine("\t\t\treturn nullptr;");
						methodBuilder.AppendLine("\t\t}");
						methodBuilder.AppendLine();

						int tupleIndex = 0;

						// Add return value to tuple if present
						if (returnParam != null)
						{
							methodBuilder.AppendLine($"\t\t// Add return value to tuple");
							if (returnParam.PropType.Equals("BoolProperty", StringComparison.Ordinal))
							{
								methodBuilder.AppendLine($"\t\tPyTuple_SET_ITEM(ReturnTuple, {tupleIndex}, PyBool_FromLong(ReturnValue ? 1 : 0));");
							}
							else if (returnParam.PropType.Equals("EnumProperty", StringComparison.Ordinal))
							{
								methodBuilder.AppendLine($"\t\tPyTuple_SET_ITEM(ReturnTuple, {tupleIndex}, UPyConversion::PythonizeEnumEntry((int64)ReturnValue, StaticEnum<{GetEnumStaticType(returnParam.TypeName)}>()));");
							}
							else if (returnParam.PropType.Equals("StructProperty", StringComparison.Ordinal))
							{
								methodBuilder.AppendLine($"\t\tPyTuple_SET_ITEM(ReturnTuple, {tupleIndex}, UPyConversion::PythonizeStructInstance(ReturnValue));");
							}
							else
							{
								methodBuilder.AppendLine(IndentCode(BuildContainerTupleItemSnippet("ReturnValue", returnParam, tupleIndex), 2));
							}
							tupleIndex++;
						}

						// Add output parameters to tuple
						for (int outIndex = 0; outIndex < outputParams.Count; ++outIndex)
						{
							PropJsonInfo outParam = outputParams[outIndex];
							string outVarName = outputParamMap[outParam.PropName];

							methodBuilder.AppendLine($"\t\t// Add output parameter '{outParam.PropName}' to tuple");
							if (outParam.PropType.Equals("BoolProperty", StringComparison.Ordinal))
							{
								methodBuilder.AppendLine($"\t\tPyTuple_SET_ITEM(ReturnTuple, {tupleIndex}, PyBool_FromLong({outVarName} ? 1 : 0));");
							}
							else if (outParam.PropType.Equals("EnumProperty", StringComparison.Ordinal))
							{
								methodBuilder.AppendLine($"\t\tPyTuple_SET_ITEM(ReturnTuple, {tupleIndex}, UPyConversion::PythonizeEnumEntry((int64){outVarName}, StaticEnum<{GetEnumStaticType(outParam.TypeName)}>()));");
							}
							else if (outParam.PropType.Equals("StructProperty", StringComparison.Ordinal))
							{
								methodBuilder.AppendLine($"\t\tPyTuple_SET_ITEM(ReturnTuple, {tupleIndex}, UPyConversion::PythonizeStructInstance({outVarName}));");
							}
							else
							{
								methodBuilder.AppendLine(IndentCode(BuildContainerTupleItemSnippet(outVarName, outParam, tupleIndex), 2));
							}
							tupleIndex++;
						}

						methodBuilder.AppendLine();
						methodBuilder.AppendLine("\t\treturn ReturnTuple;");
					}

					methodBuilder.AppendLine("\t}");
					methodBuilder.AppendLine();
				}
			}

			if (!generatedBinding)
			{
				string stubSignature = parameterCount == 0
					? $"{cppType}* InSelf, PyObject* Py_UNUSED(InUnused)"
					: parameterCount == 1
						? $"{cppType}* InSelf, PyObject* Py_UNUSED(InArg)"
						: $"{cppType}* InSelf, PyObject* Py_UNUSED(InArgs)";
				helperBuilder.AppendLine($"\tstatic PyObject* {methodFunctionName}({stubSignature})");
				helperBuilder.AppendLine("\t{");
				helperBuilder.AppendLine($"\t\tUPyUtil::SetPythonError(PyExc_NotImplementedError, TEXT(\"{errorContextLiteral}\"), TEXT(\"method binding has not been generated yet\"));");
				helperBuilder.AppendLine("\t\treturn nullptr;");
				helperBuilder.AppendLine("\t}");
				helperBuilder.AppendLine();
			}
			else
			{
				helperBuilder.Append(methodBuilder.ToString());
			}

			if (isEditorOnly)
			{
				helperBuilder.AppendLine("#endif");
			}



				if (generatedBinding)
				{
					validImpls.Add(new MethodFunctionCode(String.Empty, methodFunctionName, parameterCount));
				}

				if (!isOverloaded)
				{
					string methodFlags = inputParams.Count == 0
						? "METH_NOARGS"
						: !useVarargs
							? "METH_O"
							: "METH_VARARGS";
					if (isStaticMethod)
					{
						methodFlags += " | METH_STATIC";
					}
					if (isEditorOnly)
					{
						tableBuilder.AppendLine("#if WITH_EDITOR");
					}
					tableBuilder.AppendLine($"\t{{ \"{pythonNameLiteral}\", UPyCFunctionCast(&{helperName}::{methodFunctionName}), {methodFlags}, {docLiteral} }},");
					if (isEditorOnly)
					{
						tableBuilder.AppendLine("#endif");
					}
					hasAnyMethods = true;
				}
			}

			if (isOverloaded && validImpls.Count > 0)
			{
				string accessorBase = SanitizeIdentifier(methodName, "Method");
				nameCounts.TryGetValue(accessorBase, out int count);
				nameCounts[accessorBase] = count + 1; // Increment for the dispatcher itself
				string dispatcherName = $"Call{accessorBase}_Dispatch";
				
				helperBuilder.AppendLine($"\tstatic PyObject* {dispatcherName}({cppType}* InSelf, PyObject* InArgs)");
				helperBuilder.AppendLine("\t{");
				helperBuilder.AppendLine("\t\tPy_ssize_t ArgCount = PyTuple_Size(InArgs);");
				helperBuilder.AppendLine("\t\tswitch (ArgCount)");
				helperBuilder.AppendLine("\t\t{");

				foreach (var implGroup in validImpls.GroupBy(x => x.ArgumentCount))
				{
					helperBuilder.AppendLine($"\t\tcase {implGroup.Key}:");
					helperBuilder.AppendLine("\t\t{");
					foreach (var impl in implGroup)
					{
						helperBuilder.AppendLine("\t\t\t{");
						helperBuilder.AppendLine($"\t\t\t\tPyObject* Result = {impl.FunctionName}(InSelf, InArgs);");
						helperBuilder.AppendLine("\t\t\t\tif (Result)");
						helperBuilder.AppendLine("\t\t\t\t{");
						helperBuilder.AppendLine("\t\t\t\t\treturn Result;");
						helperBuilder.AppendLine("\t\t\t\t}");
						helperBuilder.AppendLine("\t\t\t\tPyErr_Clear();");
						helperBuilder.AppendLine("\t\t\t}");
					}
					helperBuilder.AppendLine("\t\t\tbreak;");
					helperBuilder.AppendLine("\t\t}");
				}

				helperBuilder.AppendLine("\t\tdefault:");
				helperBuilder.AppendLine("\t\t\tbreak;");
				helperBuilder.AppendLine("\t\t}");
				
				string errorContextLiteral = EscapeForCxxLiteral(BuildPropertyErrorContext(rawType, methodName));
				helperBuilder.AppendLine($"\t\tUPyUtil::SetPythonError(PyExc_TypeError, TEXT(\"{errorContextLiteral}\"), TEXT(\"invalid argument count\"));");
				helperBuilder.AppendLine("\t\treturn nullptr;");
				helperBuilder.AppendLine("\t}");
				helperBuilder.AppendLine();

				string methodFlags = "METH_VARARGS";
				bool isStatic = overloads.Any(m => IsStaticMethod(m)); 
				if (isStatic) methodFlags += " | METH_STATIC";

				tableBuilder.AppendLine($"\t{{ \"{EscapeForCxxLiteral(methodName)}\", UPyCFunctionCast(&{helperName}::{dispatcherName}), {methodFlags}, \"Overloaded function\" }},");
				hasAnyMethods = true;
			}
		}

		// If no methods were actually generated (all were filtered out), don't export anything
		if (!hasAnyMethods)
		{
			return new MethodCode(String.Empty, String.Empty, String.Empty, String.Empty, String.Empty, false);
		}

		helperBuilder.AppendLine("};");
		tableBuilder.AppendLine("\t{ nullptr }");
		tableBuilder.AppendLine("};");

		return new MethodCode(helperBuilder.ToString(), tableBuilder.ToString(), tableName, String.Empty, String.Empty, false);
	}

	private static GetSetCode BuildClassGetSetDefinitions(string cppType, string rawType, string wrapperSuffix, IReadOnlyList<PropJsonInfo> properties, AutoGenClassConfig config)
	{
		string helperName = $"FGetSets_{wrapperSuffix}";
		string tableName = $"{cppType}GetSets";

		if (properties.Count == 0)
		{
			return new GetSetCode(String.Empty, String.Empty, tableName, false);
		}

		Dictionary<string, int> nameCounts = new(StringComparer.Ordinal);
		StringBuilder helperBuilder = new();
		helperBuilder.AppendLine($"struct {helperName}");
		helperBuilder.AppendLine("{");

		StringBuilder tableBuilder = new();
		tableBuilder.AppendLine($"static PyGetSetDef {tableName}[] = {{");

		foreach (PropJsonInfo property in properties)
		{
			bool isEditorOnly = IsEditorOnlyProperty(property);
			if (isEditorOnly)
			{
				helperBuilder.AppendLine("#if WITH_EDITOR");
			}

			string accessorBase = SanitizeIdentifier(property.PropName, "Property");
			nameCounts.TryGetValue(accessorBase, out int count);
			nameCounts[accessorBase] = count + 1;
			string accessorSuffix = count == 0 ? accessorBase : $"{accessorBase}_{count}";

			string pythonAttrNameLiteral = EscapeForCxxLiteral(property.PropName);
			string enginePropNameLiteral = EscapeForCxxLiteral(property.RawPropName);
			string propertyDefName = $"GetPropertyDef_{accessorSuffix}";
			string getterName = $"Get{accessorSuffix}";
			string setterName = $"Set{accessorSuffix}";
			string memberAccess = $"InSelf->ValuePtr()->{property.RawPropName}";

			bool useDirectAccessor = TryBuildDirectAccessor(rawType, property, memberAccess, config.ForceReflectionProperties, out string getterBody, out string setterBody);

			if (!useDirectAccessor)
			{
				helperBuilder.AppendLine($"\tstatic UPyGenUtil::FGeneratedWrappedProperty& {propertyDefName}()");
				helperBuilder.AppendLine("\t{");
				helperBuilder.AppendLine("\t\tstatic bool bInitialized = false;");
				helperBuilder.AppendLine("\t\tstatic UPyGenUtil::FGeneratedWrappedProperty Property;");
				helperBuilder.AppendLine("\t\tif (!bInitialized)");
				helperBuilder.AppendLine("\t\t{");
				helperBuilder.AppendLine($"\t\t\tif (const UClass* Class = {rawType}::StaticClass())");
				helperBuilder.AppendLine("\t\t\t{");
				helperBuilder.AppendLine($"\t\t\t\tif (const FProperty* FoundProperty = Class->FindPropertyByName(TEXT(\"{enginePropNameLiteral}\")))");
				helperBuilder.AppendLine("\t\t\t\t{");
				helperBuilder.AppendLine("\t\t\t\t\tProperty.SetProperty(FoundProperty);");
				helperBuilder.AppendLine("\t\t\t\t}");
				helperBuilder.AppendLine("\t\t\t}");
				helperBuilder.AppendLine("\t\t\tbInitialized = true;");
				helperBuilder.AppendLine("\t\t}");
				helperBuilder.AppendLine("\t\treturn Property;");
				helperBuilder.AppendLine("\t}");
				helperBuilder.AppendLine();

				if (IsPropertyExternallyAccessible(property) && property.PropType == "ArrayProperty")
				{
					getterBody = $"return (PyObject*)FUPyWrapperArrayFactory::Get().CreateInstance((void*)&{memberAccess}, CastField<FArrayProperty>({propertyDefName}().Prop), FUPyWrapperOwnerContext((PyObject*)InSelf), EUPyConversionMethod::Reference);";
				}
				else if (IsPropertyExternallyAccessible(property) && property.PropType == "SetProperty")
				{
					getterBody = $"return (PyObject*)FUPyWrapperSetFactory::Get().CreateInstance((void*)&{memberAccess}, CastField<FSetProperty>({propertyDefName}().Prop), FUPyWrapperOwnerContext((PyObject*)InSelf), EUPyConversionMethod::Reference);";
				}
				else if (IsPropertyExternallyAccessible(property) && property.PropType == "MapProperty")
				{
					getterBody = $"return (PyObject*)FUPyWrapperMapFactory::Get().CreateInstance((void*)&{memberAccess}, CastField<FMapProperty>({propertyDefName}().Prop), FUPyWrapperOwnerContext((PyObject*)InSelf), EUPyConversionMethod::Reference);";
				}
				else
				{
					getterBody =
						$"return FUPyWrapperObject::GetPropertyValue(InSelf, {propertyDefName}(), \"{pythonAttrNameLiteral}\");";
				}

				setterBody =
					$"return FUPyWrapperObject::SetPropertyValue(InSelf, InValue, {propertyDefName}(), \"{pythonAttrNameLiteral}\");";
			}

			helperBuilder.AppendLine($"\tstatic PyObject* {getterName}({cppType}* InSelf, void* InClosure)");
			helperBuilder.AppendLine("\t{");
			helperBuilder.AppendLine($"\t\tif (!{cppType}::ValidateInternalState(InSelf))");
			helperBuilder.AppendLine("\t\t{");
			helperBuilder.AppendLine("\t\t\treturn nullptr;");
			helperBuilder.AppendLine("\t\t}");
			helperBuilder.AppendLine(IndentCode(getterBody.TrimEnd(), 2));
			helperBuilder.AppendLine("\t}");
			helperBuilder.AppendLine();

			bool bIsReadOnly = (property.PropFlags & 0x0000000000010000) != 0; // CPF_DisableEditOnInstance

			if (!bIsReadOnly)
			{
				helperBuilder.AppendLine($"\tstatic int {setterName}({cppType}* InSelf, PyObject* InValue, void* InClosure)");
				helperBuilder.AppendLine("\t{");
				helperBuilder.AppendLine($"\t\tif (!{cppType}::ValidateInternalState(InSelf))");
				helperBuilder.AppendLine("\t\t{");
				helperBuilder.AppendLine("\t\t\treturn -1;");
				helperBuilder.AppendLine("\t\t}");
				helperBuilder.AppendLine(IndentCode(setterBody.TrimEnd(), 2));
				helperBuilder.AppendLine("\t}");
				helperBuilder.AppendLine();
			}

			if (isEditorOnly)
			{
				helperBuilder.AppendLine("#endif");
			}

			string setterRef = bIsReadOnly ? "nullptr" : $"(setter)&{helperName}::{setterName}";
			
			if (isEditorOnly)
			{
				tableBuilder.AppendLine("#if WITH_EDITOR");
			}
			tableBuilder.AppendLine($"\t{{ UPyCStrCast(\"{pythonAttrNameLiteral}\"), (getter)&{helperName}::{getterName}, {setterRef}, nullptr, nullptr }},");
			if (isEditorOnly)
			{
				tableBuilder.AppendLine("#endif");
			}
		}

		helperBuilder.AppendLine("};");
		tableBuilder.AppendLine("\t{ nullptr }");
		tableBuilder.AppendLine("};");

		return new GetSetCode(helperBuilder.ToString(), tableBuilder.ToString(), tableName, true);
	}

	public static void Generate(IUhtExportFactory factory, RootJson root)
	{
		GenerationConfig config = LoadGenerationConfig(factory);
		if (config.Classes.Count == 0 && config.Structs.Count == 0)
		{
			return;
		}

		string? baseDirectory = factory.PluginModule?.BaseDirectory;
		if (String.IsNullOrEmpty(baseDirectory))
		{
			return;
		}

		string pluginRoot = ResolvePluginRootDirectory(baseDirectory);
		string autoGenRoot = Path.Combine(baseDirectory, "Public", "Wrapper", "AutoGen");
		ClearDirectory(autoGenRoot);
		Directory.CreateDirectory(autoGenRoot);

		// Clear GameExportPath directory if configured
		if (!String.IsNullOrEmpty(config.GameExportPath))
		{
			DirectoryInfo? pluginDir = new DirectoryInfo(pluginRoot);
			DirectoryInfo? projectRoot = pluginDir.Parent?.Parent;
			if (projectRoot != null)
			{
				string gameExportFullPath = Path.Combine(projectRoot.FullName, config.GameExportPath);
				ClearDirectory(gameExportFullPath);
				Directory.CreateDirectory(gameExportFullPath);
			}
		}

		Dictionary<string, ModuleGeneration> modules = BuildModuleMap(root, config, pluginRoot, autoGenRoot);
		if (modules.Count == 0)
		{
			return;
		}

		List<InlineStructRegistration> inlineStructs = new();
		List<ModuleGeneration> orderedModules = modules.Values
			.OrderBy(module => module.ModuleName, StringComparer.Ordinal)
			.ToList();

		EnumLookupCache.Clear();
		foreach (EnumJsonInfo enm in root.Enums)
		{
			EnumLookupCache[enm.EnumName] = enm;
			if (!String.IsNullOrEmpty(enm.NativeTypeName))
			{
				EnumLookupCache[enm.NativeTypeName] = enm;
			}
		}

		foreach (ModuleGeneration module in orderedModules)
		{
			GenerateModuleOutputs(factory, pluginRoot, module, inlineStructs);
		}

		GenerateAggregator(factory, pluginRoot, autoGenRoot, config, orderedModules, inlineStructs);
	}

	private static void GenerateModuleOutputs(IUhtExportFactory factory, string baseDirectory, ModuleGeneration module, List<InlineStructRegistration> inlineStructs)
	{
		string moduleDirectory = module.OutputDirectory;
		string structDirectory = Path.Combine(moduleDirectory, "Struct");
		string classDirectory = Path.Combine(moduleDirectory, "Class");

		if (module.Structs.Count > 0)
		{
			Directory.CreateDirectory(structDirectory);
		}

		if (module.Classes.Count > 0)
		{
			Directory.CreateDirectory(classDirectory);
		}

		foreach (StructWrapperInfo structInfo in module.Structs)
		{
			GenerateStructWrapper(factory, baseDirectory, structDirectory, module.ModuleName, module.IsGameModule, structInfo, inlineStructs);
		}

		foreach (ClassWrapperInfo classInfo in module.Classes)
		{
			GenerateClassWrapper(factory, baseDirectory, classDirectory, module.ModuleName, module.IsGameModule, classInfo);
		}

		if (module.Structs.Count > 0 || module.Classes.Count > 0)
		{
			GenerateModuleWrapper(factory, moduleDirectory, module);
		}
	}

	private static void GenerateStructWrapper(IUhtExportFactory factory, string baseDirectory, string structDirectory, string moduleName, bool isGameModule, StructWrapperInfo structInfo, List<InlineStructRegistration> inlineStructs)
	{
		string fileBaseName = $"UPyWrapper{structInfo.WrapperSuffix}";

		string headerPath = Path.Combine(structDirectory, $"{fileBaseName}.h");
		string sourcePath = Path.Combine(structDirectory, $"{fileBaseName}.cpp");

		string headerTemplate = GetTemplateContent(baseDirectory, "UPyWrapperStructTemplate.h");
		if (isGameModule)
		{
			headerTemplate = headerTemplate.Replace("UNREALPYTHON_API ", String.Empty);
		}
		string sourceTemplate = GetTemplateContent(baseDirectory, "UPyWrapperStructTemplate.cpp");

		string cppType = $"FUPyWrapper{structInfo.WrapperSuffix}";
		string pyType = $"UPyWrapper{structInfo.WrapperSuffix}Type";
		string rawType = structInfo.Info.NativeTypeName;
		string tpName = structInfo.WrapperSuffix;
		string baseType = structInfo.Config.IsInline
			? $"TUPyWrapperInlineStruct<{rawType}>"
			: "FUPyWrapperStruct";

		List<PropJsonInfo> structProperties = GetStructProperties(structInfo);
		GetSetCode structGetSetCode = BuildStructGetSetDefinitions(cppType, rawType, structInfo.WrapperSuffix, structProperties, structInfo.Config);
		string getSetArrayName = structGetSetCode.TableName;
		List<MethodJsonInfo> structMethods = GetStructMethods(structInfo);
		MethodCode structMethodCode = BuildStructMethodDefinitions(cppType, rawType, structInfo.WrapperSuffix, structMethods, structInfo.Config.ForceReflectionMethods);
		string methodArrayName = structMethodCode.TableName;
		ConstantCode structConstantCode = BuildConstantDefinitions(rawType, structInfo.WrapperSuffix, structMethods);
		string numberStructName = $"{cppType}NumberMethods";
		string funcStructName = $"FFuncs_{structInfo.WrapperSuffix}";
		Dictionary<string, string> commonValues = new(StringComparer.Ordinal)
		{
			["CppType"] = cppType,
			["PyType"] = pyType,
			["RawType"] = rawType,
			["TpName"] = tpName,
			["BaseType"] = baseType,
			["FunctionName"] = structInfo.FunctionName
		};

		string funcDefs = BuildStructFunctionDefinitions(cppType, funcStructName, structInfo, out bool hasRichCompare);

		NumberFuncCode structNumberFuncCode = BuildStructNumberFunctionDefinitions(cppType, rawType, structInfo, numberStructName);

		StringBuilder headerIncludeBuilder = new();
		headerIncludeBuilder.AppendLine("#include \"Wrapper/UPyWrapperStruct.h\"");
		headerIncludeBuilder.AppendLine("#include \"Utils/UPyGenUtil.h\"");
		if (!String.IsNullOrEmpty(structInfo.HeaderInclude))
		{
			headerIncludeBuilder.AppendLine($"#include \"{structInfo.HeaderInclude}\"");
		}

		Dictionary<string, string> headerReplacements = new(commonValues, StringComparer.Ordinal)
		{
			["IncludeFiles"] = headerIncludeBuilder.ToString()
		};

		string headerContent = RenderTemplate(headerTemplate, headerReplacements);
		CommitOutput(factory, headerPath, new StringBuilder(headerContent));

		StringBuilder sourceIncludeBuilder = new();
		sourceIncludeBuilder.AppendLine($"#include \"{structInfo.WrapperHeader}\"");
		sourceIncludeBuilder.AppendLine("#include \"Wrapper/UPyWrapperTypeRegistry.h\"");
		sourceIncludeBuilder.AppendLine("#include \"Wrapper/UPyWrapperTypeFactory.h\"");
		sourceIncludeBuilder.AppendLine("#include \"Utils/UPyUtil.h\"");
		sourceIncludeBuilder.AppendLine("#include \"Templates/SharedPointer.h\"");
		sourceIncludeBuilder.AppendLine("#include \"UObject/NoExportTypes.h\"");
		if (!String.IsNullOrEmpty(structConstantCode.Definitions))
		{
			sourceIncludeBuilder.AppendLine("#include \"DynamicTypes/UPyConstant.h\"");
		}
		HashSet<string> extraHeaders = new(StringComparer.Ordinal);
		foreach (MethodJsonInfo method in structMethods)
		{
			if (method.bUseHelperMethod && !String.IsNullOrEmpty(method.HostHelper) && method.HostHelper != rawType)
			{
				string className = method.HostHelper;
				int sepIndex = className.IndexOf("::", StringComparison.Ordinal);
				if (sepIndex > 0)
				{
					className = className.Substring(0, sepIndex);
				}

				string? header = GetHeaderFileForType(className);
				if (!String.IsNullOrEmpty(header))
				{
					extraHeaders.Add(header);
				}
			}

			foreach (PropJsonInfo param in method.Params)
			{
				if (!String.IsNullOrEmpty(param.TypeName))
				{
					string? header = GetHeaderFileForType(param.TypeName);
					if (!String.IsNullOrEmpty(header))
					{
						extraHeaders.Add(header);
					}
				}
			}
		}
		foreach (string header in extraHeaders)
		{
			sourceIncludeBuilder.AppendLine($"#include \"{header}\"");
		}
		if (structInfo.Config.IsInline)
		{
			inlineStructs.Add(new InlineStructRegistration(structInfo.Info.NativeTypeName, structInfo.HeaderInclude));
		}

		StringBuilder initializeBodyBuilder = new();
		if (structInfo.Config.IsInline)
		{
			initializeBodyBuilder.AppendLine($"\tFUPyWrapperTypeRegistry::Get().RegisterInlineStructFactory(StaticCastSharedRef<const IUPyWrapperInlineStructFactory>(MakeShared<TUPyWrapperInlineStructFactory<{rawType}>>()));");
		}
		initializeBodyBuilder.Append($"\tFUPyWrapperTypeRegistry::Get().RegisterAutoWrappedStructCreateFunc(TBaseStructure<{rawType}>::Get(), DoInitWrapper{tpName});");

		string richCompareSetter = hasRichCompare
			? $"\tPyType->tp_richcompare = (richcmpfunc)&{funcStructName}::RichCmp;\n"
			: String.Empty;

		StringBuilder initPyTypeBuilder = new();
		initPyTypeBuilder.AppendLine("\tif (InStruct == nullptr)");
		initPyTypeBuilder.AppendLine("\t{");
		initPyTypeBuilder.AppendLine("\t\treturn nullptr;");
		initPyTypeBuilder.AppendLine("\t}");
		initPyTypeBuilder.AppendLine();
		initPyTypeBuilder.AppendLine("\tif (FUPyWrapperTypeRegistry::Get().HasWrappedStructType(InStruct))");
		initPyTypeBuilder.AppendLine("\t{");
		initPyTypeBuilder.AppendLine("\t\treturn const_cast<PyTypeObject*>(FUPyWrapperTypeRegistry::Get().GetWrappedStructType(InStruct));");
		initPyTypeBuilder.AppendLine("\t}");
		initPyTypeBuilder.AppendLine();
		initPyTypeBuilder.AppendLine($"\tPyTypeObject* PyType = &{pyType};");
		initPyTypeBuilder.AppendLine("\tPyType->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;");
		initPyTypeBuilder.AppendLine("\tPyType->tp_base = InSuperType != nullptr ? InSuperType : &UPyWrapperStructType;");
		initPyTypeBuilder.AppendLine($"\tPyType->tp_init = (initproc)&{funcStructName}::Init;");
		if (!String.IsNullOrEmpty(methodArrayName))
		{
			initPyTypeBuilder.AppendLine($"\tPyType->tp_methods = {methodArrayName};");
		}
		if (structGetSetCode.HasProperties)
		{
			initPyTypeBuilder.AppendLine($"\tPyType->tp_getset = {getSetArrayName};");
		}
		if (!String.IsNullOrEmpty(richCompareSetter))
		{
			initPyTypeBuilder.Append(richCompareSetter);
		}
		if (!String.IsNullOrEmpty(structNumberFuncCode.InitStatements))
		{
			initPyTypeBuilder.Append(structNumberFuncCode.InitStatements);
		}
		else
		{
			initPyTypeBuilder.AppendLine();
		}
		initPyTypeBuilder.AppendLine("\tif (PyType_Ready(PyType) == 0)");
		initPyTypeBuilder.AppendLine("\t{");
		if (!String.IsNullOrEmpty(structConstantCode.InitStatements))
		{
			initPyTypeBuilder.Append(structConstantCode.InitStatements);
		}
		initPyTypeBuilder.AppendLine("\t\tFUPyWrapperTypeRegistry::Get().RegisterWrappedStructType(InStruct, PyType);");
		initPyTypeBuilder.AppendLine("\t\treturn PyType;");
		initPyTypeBuilder.AppendLine("\t}");
		initPyTypeBuilder.AppendLine();
		initPyTypeBuilder.AppendLine("\treturn nullptr;");

		string initPyTypeBody = initPyTypeBuilder.ToString();

		Dictionary<string, string> sourceReplacements = new(commonValues, StringComparer.Ordinal)
		{
			["IncludeFiles"] = sourceIncludeBuilder.ToString(),
			["PropGetSetDefs"] = structGetSetCode.Combine(),
			["FuncDefs"] = funcDefs,
			["NumberFuncDefs"] = structNumberFuncCode.Combine(),
			["MethodDefs"] = structMethodCode.Combine(),
			["ConstantDefs"] = structConstantCode.Definitions,
			["InitPyType"] = initPyTypeBody,
			["InitializeBody"] = initializeBodyBuilder.ToString()
		};
		string sourceContent = RenderTemplate(sourceTemplate, sourceReplacements);
		CommitOutput(factory, sourcePath, new StringBuilder(sourceContent));
	}

	private static void GenerateClassWrapper(IUhtExportFactory factory, string baseDirectory, string classDirectory, string moduleName, bool isGameModule, ClassWrapperInfo classInfo)
	{
		string fileBaseName = $"UPyWrapper{classInfo.WrapperSuffix}";
		string headerPath = Path.Combine(classDirectory, $"{fileBaseName}.h");
		string sourcePath = Path.Combine(classDirectory, $"{fileBaseName}.cpp");

		string headerTemplate = GetTemplateContent(baseDirectory, "UPyWrapperObjectTemplate.h");
		if (isGameModule)
		{
			headerTemplate = headerTemplate.Replace("UNREALPYTHON_API ", String.Empty);
		}
		string sourceTemplate = GetTemplateContent(baseDirectory, "UPyWrapperObjectTemplate.cpp");

		string cppType = $"FUPyWrapper{classInfo.WrapperSuffix}";
		string pyType = $"UPyWrapper{classInfo.WrapperSuffix}Type";
		string rawType = classInfo.Info.NativeTypeName;
		string tpName = classInfo.WrapperSuffix;

		List<PropJsonInfo> classProperties = GetClassProperties(classInfo);
		GetSetCode classGetSetCode = BuildClassGetSetDefinitions(cppType, rawType, classInfo.WrapperSuffix, classProperties, classInfo.Config);
		string classGetSetArrayName = classGetSetCode.TableName;
		List<MethodJsonInfo> classMethods = GetClassMethods(classInfo);
		MethodCode classMethodCode = BuildStructMethodDefinitions(cppType, rawType, classInfo.WrapperSuffix, classMethods, classInfo.Config.ForceReflectionMethods);
		string classMethodArrayName = classMethodCode.TableName;
		ConstantCode classConstantCode = BuildConstantDefinitions(rawType, classInfo.WrapperSuffix, classMethods);
		string classNumberStructName = $"{cppType}NumberMethods";
		NumberFuncCode classNumberFuncCode = BuildNumberFunctionDefinitions(cppType, rawType, classInfo.WrapperSuffix, classMethods, classNumberStructName, classInfo.Info.ClassName);
		Dictionary<string, string> commonValues = new(StringComparer.Ordinal)
		{
			["CppType"] = cppType,
			["PyType"] = pyType,
			["RawType"] = rawType,
			["TpName"] = tpName,
			["FunctionName"] = classInfo.FunctionName
		};

		string classFuncDefs = String.Empty;
		StringBuilder headerIncludeBuilder = new();
		headerIncludeBuilder.AppendLine("#include \"Wrapper/UPyWrapperObject.h\"");
		headerIncludeBuilder.AppendLine("#include \"Utils/UPyGenUtil.h\"");
		if (!String.IsNullOrEmpty(classInfo.HeaderInclude))
		{
			headerIncludeBuilder.AppendLine($"#include \"{classInfo.HeaderInclude}\"");
		}

		Dictionary<string, string> headerReplacements = new(commonValues, StringComparer.Ordinal)
		{
			["IncludeFiles"] = headerIncludeBuilder.ToString()
		};

		string headerContent = RenderTemplate(headerTemplate, headerReplacements);
		CommitOutput(factory, headerPath, new StringBuilder(headerContent));

		StringBuilder sourceIncludeBuilder = new();
		sourceIncludeBuilder.AppendLine($"#include \"{classInfo.WrapperHeader}\"");
		sourceIncludeBuilder.AppendLine("#include \"Wrapper/UPyWrapperTypeRegistry.h\"");
		sourceIncludeBuilder.AppendLine("#include \"Wrapper/UPyWrapperTypeFactory.h\"");
		sourceIncludeBuilder.AppendLine("#include \"Utils/UPyUtil.h\"");
		if (!String.IsNullOrEmpty(classConstantCode.Definitions))
		{
			sourceIncludeBuilder.AppendLine("#include \"DynamicTypes/UPyConstant.h\"");
		}
		HashSet<string> extraHeaders = new(StringComparer.Ordinal);
		foreach (MethodJsonInfo method in classMethods)
		{
			if (method.bUseHelperMethod && !String.IsNullOrEmpty(method.HostHelper) && method.HostHelper != rawType)
			{
				string className = method.HostHelper;
				int sepIndex = className.IndexOf("::", StringComparison.Ordinal);
				if (sepIndex > 0)
				{
					className = className.Substring(0, sepIndex);
				}

				string? header = GetHeaderFileForType(className);
				if (!String.IsNullOrEmpty(header))
				{
					extraHeaders.Add(header);
				}
			}

			foreach (PropJsonInfo param in method.Params)
			{
				if (!String.IsNullOrEmpty(param.TypeName))
				{
					string? header = GetHeaderFileForType(param.TypeName);
					if (!String.IsNullOrEmpty(header))
					{
						extraHeaders.Add(header);
					}
				}

				if ((param.PropType.Equals("ArrayProperty", StringComparison.Ordinal) || param.PropType.Equals("SetProperty", StringComparison.Ordinal)) && !String.IsNullOrEmpty(param.TypeName))
				{
					if (param.TypeName.StartsWith("ByteProperty<") && param.TypeName.EndsWith(">"))
					{
						string enumName = param.TypeName.Substring(13, param.TypeName.Length - 14);
						string? resolvedEnum = ResolveNativeTypeName(enumName) ?? enumName;
						string? header = GetHeaderFileForType(resolvedEnum);
						if (!String.IsNullOrEmpty(header))
						{
							extraHeaders.Add(header);
						}
					}
				}
				else if (param.PropType.Equals("MapProperty", StringComparison.Ordinal) && !String.IsNullOrEmpty(param.TypeName))
				{
					string[] parts = param.TypeName.Split(new[] { "->" }, StringSplitOptions.None);
					foreach (string part in parts)
					{
						if (part.StartsWith("ByteProperty<") && part.EndsWith(">"))
						{
							string enumName = part.Substring(13, part.Length - 14);
							string? resolvedEnum = ResolveNativeTypeName(enumName) ?? enumName;
							string? header = GetHeaderFileForType(resolvedEnum);
							if (!String.IsNullOrEmpty(header))
							{
								extraHeaders.Add(header);
							}
						}
					}
				}
			}
		}
		foreach (string header in extraHeaders)
		{
			sourceIncludeBuilder.AppendLine($"#include \"{header}\"");
		}
		StringBuilder initializeBodyBuilder = new();
		initializeBodyBuilder.Append($"\tFUPyWrapperTypeRegistry::Get().RegisterAutoWrappedClassCreateFunc({rawType}::StaticClass(), DoInitWrapper{tpName});");

		string methodSetter = !String.IsNullOrEmpty(classMethodArrayName) ? $"\tPyType->tp_methods = {classMethodArrayName};\n" : String.Empty;

		string getSetSetter = classGetSetCode.HasProperties
		? $"\tPyType->tp_getset = {classGetSetArrayName};\n"
		: String.Empty;

		string initPyTypeBody =
			"\tif (InClass == nullptr)\n" +
			"\t{\n" +
			"\t\treturn nullptr;\n" +
			"\t}\n\n" +
			"\tif (FUPyWrapperTypeRegistry::Get().HasWrappedClassType(InClass))\n" +
			"\t{\n" +
			"\t\treturn const_cast<PyTypeObject*>(FUPyWrapperTypeRegistry::Get().GetWrappedClassType(InClass));\n" +
			"\t}\n\n" +
			$"\tPyTypeObject* PyType = &{pyType};\n" +
			"\tPyType->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;\n" +
			"\tPyType->tp_base = InSuperType != nullptr ? InSuperType : &UPyWrapperObjectType;\n" +
			"\tPyType->tp_bases = InSuperTypes;\n" +
			methodSetter +
			getSetSetter +
			"{ClassNumberInit}" +
			"\tif (PyType_Ready(PyType) == 0)\n" +
			"\t{\n" +
			"\t\tFUPyWrapperTypeRegistry::Get().RegisterWrappedClassType(InClass, PyType);\n" +
			"\t\treturn PyType;\n" +
			"\t}\n\n" +
			"\treturn nullptr;";

		string classNumberInit = !String.IsNullOrEmpty(classNumberFuncCode.InitStatements) ? classNumberFuncCode.InitStatements : "\n";

		initPyTypeBody = initPyTypeBody.Replace("{ClassNumberInit}", classNumberInit);

		if (!String.IsNullOrEmpty(classConstantCode.InitStatements))
		{
			initPyTypeBody = initPyTypeBody.Replace("\tif (PyType_Ready(PyType) == 0)\n\t{\n", "\tif (PyType_Ready(PyType) == 0)\n\t{\n" + classConstantCode.InitStatements);
		}

		Dictionary<string, string> sourceReplacements = new(commonValues, StringComparer.Ordinal)
		{
			["IncludeFiles"] = sourceIncludeBuilder.ToString(),
			["PropGetSetDefs"] = classGetSetCode.Combine(),
			["FuncDefs"] = classFuncDefs,
			["NumberFuncDefs"] = classNumberFuncCode.Combine(),
			["MethodDefs"] = classMethodCode.Combine(),
			["ConstantDefs"] = classConstantCode.Definitions,
			["InitPyType"] = initPyTypeBody,
			["InitializeBody"] = initializeBodyBuilder.ToString()
		};

		string sourceContent = RenderTemplate(sourceTemplate, sourceReplacements);
		CommitOutput(factory, sourcePath, new StringBuilder(sourceContent));
	}

	private static void GenerateModuleWrapper(IUhtExportFactory factory, string moduleDirectory, ModuleGeneration module)
	{
		string headerPath = Path.Combine(moduleDirectory, $"{module.ModuleWrapperName}.h");
		string sourcePath = Path.Combine(moduleDirectory, $"{module.ModuleWrapperName}.cpp");

		StringBuilder headerBuilder = new();
		headerBuilder.AppendLine("// This file is automatically generated. Do not edit manually.");
		headerBuilder.AppendLine("#pragma once");
		headerBuilder.AppendLine();
		headerBuilder.AppendLine("#include \"Utils/UPyGenUtil.h\"");
		headerBuilder.AppendLine();
		headerBuilder.AppendLine($"void {module.InitializerName}(UPyGenUtil::FNativePythonModule& ModuleInfo);");

		CommitOutput(factory, headerPath, headerBuilder);

		StringBuilder sourceBuilder = new();
		sourceBuilder.AppendLine("// This file is automatically generated. Do not edit manually.");
		sourceBuilder.AppendLine($"#include \"{module.IncludePrefix}/{module.ModuleName}/{module.ModuleWrapperName}.h\"");

		HashSet<string> emittedIncludes = new(StringComparer.Ordinal);
		foreach (StructWrapperInfo structInfo in module.Structs)
		{
			if (emittedIncludes.Add(structInfo.WrapperHeader))
			{
				sourceBuilder.AppendLine($"#include \"{structInfo.WrapperHeader}\"");
			}
		}
		foreach (ClassWrapperInfo classInfo in module.Classes)
		{
			if (emittedIncludes.Add(classInfo.WrapperHeader))
			{
				sourceBuilder.AppendLine($"#include \"{classInfo.WrapperHeader}\"");
			}
		}

		sourceBuilder.AppendLine();
		sourceBuilder.AppendLine($"void {module.InitializerName}(UPyGenUtil::FNativePythonModule& ModuleInfo)");
		sourceBuilder.AppendLine("{");
		foreach (StructWrapperInfo structInfo in module.Structs)
		{
			sourceBuilder.AppendLine($"\t{structInfo.FunctionName}(ModuleInfo);");
		}
		foreach (ClassWrapperInfo classInfo in module.Classes)
		{
			sourceBuilder.AppendLine($"\t{classInfo.FunctionName}(ModuleInfo);");
		}
		sourceBuilder.AppendLine("}");

		CommitOutput(factory, sourcePath, sourceBuilder);
	}

	private static void GenerateAggregator(IUhtExportFactory factory, string pluginRoot, string autoGenRoot, GenerationConfig config, IReadOnlyList<ModuleGeneration> modules, List<InlineStructRegistration> inlineStructs)
	{
		List<ModuleGeneration> pluginModules = modules.Where(m => !m.IsGameModule).ToList();
		List<ModuleGeneration> gameModules = modules.Where(m => m.IsGameModule).ToList();

		// Generate plugin aggregator (UPyAutoGenWrapper.h)
		string aggregatorPath = Path.Combine(autoGenRoot, "UPyAutoGenWrapper.h");

		HashSet<string> includeHeaders = new(StringComparer.Ordinal)
		{
			"Wrapper/UPyWrapperTypeRegistry.h",
			"Wrapper/UPyWrapperTypeFactory.h",
			"Templates/SharedPointer.h",
			"Utils/UPyGenUtil.h"
		};

		foreach (ModuleGeneration module in pluginModules)
		{
			includeHeaders.Add($"Wrapper/AutoGen/{module.ModuleName}/{module.ModuleWrapperName}.h");
		}

		Dictionary<string, string> defaultInlineStructs = GetDefaultInlineStructs();
		foreach (KeyValuePair<string, string> defaultEntry in defaultInlineStructs)
		{
			if (!String.IsNullOrEmpty(defaultEntry.Value))
			{
				includeHeaders.Add(defaultEntry.Value);
			}
		}

		foreach (InlineStructRegistration registration in inlineStructs)
		{
			if (!String.IsNullOrEmpty(registration.HeaderInclude))
			{
				includeHeaders.Add(registration.HeaderInclude);
			}
		}

		StringBuilder builder = new();
		builder.AppendLine("// This file is automatically generated. Do not edit manually.");
		builder.AppendLine("#pragma once");
		builder.AppendLine();

		foreach (string header in includeHeaders.OrderBy(h => h, StringComparer.Ordinal))
		{
			builder.AppendLine($"#include \"{header}\"");
		}

		builder.AppendLine();
		builder.AppendLine("inline void InitializeAutoGenWrapperTypes(UPyGenUtil::FNativePythonModule& ModuleInfo)");
		builder.AppendLine("{");
		foreach (ModuleGeneration module in pluginModules)
		{
			builder.AppendLine($"\t{module.InitializerName}(ModuleInfo);");
		}
		builder.AppendLine("}");

		CommitOutput(factory, aggregatorPath, builder);

		// Generate game aggregator (UPyGameAutoGenWrapper.h) if there are game modules
		if (gameModules.Count > 0 && !String.IsNullOrEmpty(config.GameExportPath) && !String.IsNullOrEmpty(config.GameIncludePath))
		{
			DirectoryInfo? pluginDir = new DirectoryInfo(pluginRoot);
			DirectoryInfo? projectRoot = pluginDir.Parent?.Parent;
			if (projectRoot != null)
			{
				string gameExportFullPath = Path.Combine(projectRoot.FullName, config.GameExportPath);
				string gameAggregatorPath = Path.Combine(gameExportFullPath, "UPyGameAutoGenWrapper.h");

				HashSet<string> gameIncludeHeaders = new(StringComparer.Ordinal)
				{
					"Utils/UPyGenUtil.h"
				};

				foreach (ModuleGeneration module in gameModules)
				{
					gameIncludeHeaders.Add($"{config.GameIncludePath}/{module.ModuleName}/{module.ModuleWrapperName}.h");
				}

				StringBuilder gameBuilder = new();
				gameBuilder.AppendLine("// This file is automatically generated. Do not edit manually.");
				gameBuilder.AppendLine("#pragma once");
				gameBuilder.AppendLine();

				foreach (string header in gameIncludeHeaders.OrderBy(h => h, StringComparer.Ordinal))
				{
					gameBuilder.AppendLine($"#include \"{header}\"");
				}

				gameBuilder.AppendLine();
				gameBuilder.AppendLine("inline void InitializeGameAutoGenWrapperTypes(UPyGenUtil::FNativePythonModule& ModuleInfo)");
				gameBuilder.AppendLine("{");
				foreach (ModuleGeneration module in gameModules)
				{
					gameBuilder.AppendLine($"\t{module.InitializerName}(ModuleInfo);");
				}
				gameBuilder.AppendLine("}");

				CommitOutput(factory, gameAggregatorPath, gameBuilder);
			}
		}
	}

	private static bool TryGetGameExportDirectory(string pluginRoot, string? gameExportPath, string moduleName, out string moduleDirectory)
	{
		if (String.IsNullOrEmpty(gameExportPath))
		{
			moduleDirectory = String.Empty;
			return false;
		}

		DirectoryInfo? pluginDir = new DirectoryInfo(pluginRoot);
		DirectoryInfo? projectRoot = pluginDir.Parent?.Parent;

		if (projectRoot != null)
		{
			// GameExportPath is relative to project root, e.g. "Source/MyGame/Public/UPy/Wrapper/AutoGen"
			string gameExportFullPath = Path.Combine(projectRoot.FullName, gameExportPath);
			moduleDirectory = Path.Combine(gameExportFullPath, moduleName);
			return true;
		}

		moduleDirectory = String.Empty;
		return false;
	}

	private static bool TryGetProjectModuleDirectory(string pluginRoot, string? gameExportPath, string moduleName, out string moduleDirectory)
	{
		DirectoryInfo? pluginDir = new DirectoryInfo(pluginRoot);
		DirectoryInfo? projectRoot = pluginDir.Parent?.Parent;

		if (projectRoot != null)
		{
			string potentialPath = Path.Combine(projectRoot.FullName, "Source", moduleName);
			if (Directory.Exists(potentialPath))
			{
				// Use GameExportPath if specified, otherwise keep using legacy path
				if (!String.IsNullOrEmpty(gameExportPath))
				{
					string gameExportFullPath = Path.Combine(projectRoot.FullName, gameExportPath);
					moduleDirectory = Path.Combine(gameExportFullPath, moduleName);
				}
				else
				{
					moduleDirectory = Path.Combine(potentialPath, "Public", "UPy", "Wrapper", "AutoGen");
				}
				return true;
			}
		}

		moduleDirectory = String.Empty;
		return false;
	}

	private static Dictionary<string, ModuleGeneration> BuildModuleMap(RootJson root, GenerationConfig config, string pluginRoot, string autoGenRoot)
	{
		Dictionary<string, ModuleGeneration> modules = new(StringComparer.Ordinal);
		Dictionary<string, ClassJsonInfo> classLookup = BuildClassLookup(root);
		Dictionary<string, StructJsonInfo> structLookup = BuildStructLookup(root);

		foreach (KeyValuePair<string, AutoGenClassConfig> entry in config.Classes)
		{
			if (!classLookup.TryGetValue(entry.Key, out ClassJsonInfo? classInfo))
			{
				Console.WriteLine($"[UnrealPython] Warning: AutoGen class '{entry.Key}' was not found in the exported metadata.");
				continue;
			}

			string moduleName = ResolveModuleName(classInfo.ModuleName, classInfo.PackageName);
			ModuleGeneration module = GetOrAddModule(modules, moduleName, config.ExportToGameModules, pluginRoot, autoGenRoot, config.GameExportPath, config.GameIncludePath);
			module.Classes.Add(new ClassWrapperInfo(classInfo, entry.Value, moduleName, module.IncludePrefix));
		}

		foreach (KeyValuePair<string, AutoGenStructConfig> entry in config.Structs)
		{
			if (!structLookup.TryGetValue(entry.Key, out StructJsonInfo? structInfo))
			{
				Console.WriteLine($"[UnrealPython] Warning: AutoGen struct '{entry.Key}' was not found in the exported metadata.");
				continue;
			}

			string moduleName = ResolveModuleName(structInfo.ModuleName, structInfo.PackageName);
			ModuleGeneration module = GetOrAddModule(modules, moduleName, config.ExportToGameModules, pluginRoot, autoGenRoot, config.GameExportPath, config.GameIncludePath);
			module.Structs.Add(new StructWrapperInfo(structInfo, entry.Value, moduleName, module.IncludePrefix));
		}

		StructLookupCache = structLookup;
		ClassLookupCache = classLookup;

		return modules;
	}

	private static ModuleGeneration GetOrAddModule(Dictionary<string, ModuleGeneration> modules, string moduleName, HashSet<string> exportToGameModules, string pluginRoot, string autoGenRoot, string? gameExportPath, string? gameIncludePath)
	{
		// Check if module is in ExportToGameModules list
		bool exportToGame = exportToGameModules.Contains(moduleName);

		// Create unique key for module based on export location
		string moduleKey = exportToGame ? $"{moduleName}@game" : moduleName;

		if (!modules.TryGetValue(moduleKey, out ModuleGeneration? module))
		{
			// Check if this is a project module (exists in Source/{moduleName})
			bool isProjectModule = TryGetProjectModuleDirectory(pluginRoot, gameExportPath, moduleName, out string projectModuleDir);

			if (exportToGame)
			{
				// Module is in ExportToGameModules list, use GameExportPath
				if (TryGetGameExportDirectory(pluginRoot, gameExportPath, moduleName, out string gameModuleDir))
				{
					module = new ModuleGeneration(moduleName, gameModuleDir, true, gameIncludePath);
				}
				else
				{
					// Fallback to plugin AutoGen directory if GameExportPath is not configured
					string defaultModuleDir = Path.Combine(autoGenRoot, moduleName);
					module = new ModuleGeneration(moduleName, defaultModuleDir, false, gameIncludePath);
					Console.WriteLine($"[UnrealPython] Warning: Module '{moduleName}' is in ExportToGameModules but GameExportPath is not configured. Using plugin directory.");
				}
			}
			else if (isProjectModule)
			{
				// Project module defaults to GameExportPath
				module = new ModuleGeneration(moduleName, projectModuleDir, true, gameIncludePath);
			}
			else
			{
				// Plugin module
				string defaultModuleDir = Path.Combine(autoGenRoot, moduleName);
				module = new ModuleGeneration(moduleName, defaultModuleDir, false, gameIncludePath);
			}
			modules[moduleKey] = module;
		}
		return module;
	}

	private static Dictionary<string, ClassJsonInfo> BuildClassLookup(RootJson root)
	{
		Dictionary<string, ClassJsonInfo> lookup = new(StringComparer.Ordinal);
		foreach (ClassJsonInfo info in root.Classes)
		{
			AddLookup(lookup, info.ClassName, info);
			AddLookup(lookup, info.EngineName, info);
		}

		foreach (ClassJsonInfo info in root.Classes)
		{
			if (!String.IsNullOrEmpty(info.NativeTypeName))
			{
				AddLookup(lookup, info.NativeTypeName, info);
			}
		}
		return lookup;
	}

	private static Dictionary<string, StructJsonInfo> BuildStructLookup(RootJson root)
	{
		Dictionary<string, StructJsonInfo> lookup = new(StringComparer.Ordinal);
		foreach (StructJsonInfo info in root.Structs)
		{
			AddLookup(lookup, info.StructName, info);
			AddLookup(lookup, info.EngineName, info);
		}

		foreach (StructJsonInfo info in root.Structs)
		{
			if (!String.IsNullOrEmpty(info.NativeTypeName))
			{
				AddLookup(lookup, info.NativeTypeName, info);
			}
		}
		return lookup;
	}

	private static void AddLookup<T>(Dictionary<string, T> lookup, string key, T value) where T : class
	{
		if (!String.IsNullOrWhiteSpace(key))
		{
			lookup.TryAdd(key, value);
		}
	}

	internal static GenerationConfig LoadGenerationConfig(IUhtExportFactory factory)
	{
		GenerationConfig config = new();
		string? moduleBaseDirectory = factory.PluginModule?.BaseDirectory;
		if (String.IsNullOrEmpty(moduleBaseDirectory))
		{
			return config;
		}

		string pluginRoot = ResolvePluginRootDirectory(moduleBaseDirectory);

		string settingsPath = Path.Combine(pluginRoot, "Tools", "GenerationSettings.json");
		if (!File.Exists(settingsPath))
		{
			ApplyGameExportDefaults(config, pluginRoot);
			return config;
		}

		string jsonContent = File.ReadAllText(settingsPath);
		using JsonDocument document = JsonDocument.Parse(jsonContent);
		JsonElement root = document.RootElement;

		if (root.TryGetProperty("StubFilePath", out JsonElement stubFilePathElement) && stubFilePathElement.ValueKind == JsonValueKind.String)
		{
			config.StubFilePath = stubFilePathElement.GetString();
		}

		if (root.TryGetProperty("GameExportPath", out JsonElement gameExportPathElement) && gameExportPathElement.ValueKind == JsonValueKind.String)
		{
			config.GameExportPath = gameExportPathElement.GetString();
		}

		if (root.TryGetProperty("ExportToGameModules", out JsonElement exportToGameModulesElement) && exportToGameModulesElement.ValueKind == JsonValueKind.Array)
		{
			foreach (JsonElement moduleElement in exportToGameModulesElement.EnumerateArray())
			{
				if (moduleElement.ValueKind == JsonValueKind.String)
				{
					string? moduleName = moduleElement.GetString();
					if (!String.IsNullOrEmpty(moduleName))
					{
						config.ExportToGameModules.Add(moduleName);
					}
				}
			}
		}

		static void PopulateStringSet(JsonElement parent, string propertyName, ref HashSet<string>? target)
		{
			if (!parent.TryGetProperty(propertyName, out JsonElement arrayElement) || arrayElement.ValueKind != JsonValueKind.Array)
			{
				return;
			}

			HashSet<string> set = EnsureSet(ref target);
			foreach (JsonElement valueElement in arrayElement.EnumerateArray())
			{
				if (valueElement.ValueKind == JsonValueKind.String)
				{
					string? value = valueElement.GetString();
					if (!String.IsNullOrEmpty(value))
					{
						set.Add(value);
					}
				}
			}
		}

		if (root.TryGetProperty("AutoGenClasses", out JsonElement classesElement) && classesElement.ValueKind == JsonValueKind.Object)
		{
			foreach (JsonProperty classProperty in classesElement.EnumerateObject())
			{
				AutoGenClassConfig classConfig = new();
				config.Classes[classProperty.Name] = classConfig;

				if (classProperty.Value.ValueKind != JsonValueKind.Object)
				{
					continue;
				}

				PopulateStringSet(classProperty.Value, "IncludeProperties", ref classConfig.IncludeProperties);
				PopulateStringSet(classProperty.Value, "ExcludeProperties", ref classConfig.ExcludeProperties);
				PopulateStringSet(classProperty.Value, "ForceReflectionProperties", ref classConfig.ForceReflectionProperties);
				PopulateStringSet(classProperty.Value, "IncludeMethods", ref classConfig.IncludeMethods);
				PopulateStringSet(classProperty.Value, "ExcludeMethods", ref classConfig.ExcludeMethods);
				PopulateStringSet(classProperty.Value, "ForceReflectionMethods", ref classConfig.ForceReflectionMethods);
			}
		}

		if (root.TryGetProperty("AutoGenStructs", out JsonElement structsElement) && structsElement.ValueKind == JsonValueKind.Object)
		{
			foreach (JsonProperty structProperty in structsElement.EnumerateObject())
			{
				AutoGenStructConfig structConfig = new();
				config.Structs[structProperty.Name] = structConfig;

				if (structProperty.Value.ValueKind != JsonValueKind.Object)
				{
					continue;
				}

				if (structProperty.Value.TryGetProperty("IsInline", out JsonElement isInlineElement) && isInlineElement.ValueKind == JsonValueKind.True)
				{
					structConfig.IsInline = true;
				}
				else if (structProperty.Value.TryGetProperty("IsInline", out JsonElement isInlineFalseElement) && isInlineFalseElement.ValueKind == JsonValueKind.False)
				{
					structConfig.IsInline = false;
				}

				PopulateStringSet(structProperty.Value, "IncludeProperties", ref structConfig.IncludeProperties);
				PopulateStringSet(structProperty.Value, "ExcludeProperties", ref structConfig.ExcludeProperties);
				PopulateStringSet(structProperty.Value, "ForceReflectionProperties", ref structConfig.ForceReflectionProperties);
				PopulateStringSet(structProperty.Value, "IncludeMethods", ref structConfig.IncludeMethods);
				PopulateStringSet(structProperty.Value, "ExcludeMethods", ref structConfig.ExcludeMethods);
				PopulateStringSet(structProperty.Value, "ForceReflectionMethods", ref structConfig.ForceReflectionMethods);
			}
		}

		ApplyGameExportDefaults(config, pluginRoot);
		return config;
	}

	private static void ApplyGameExportDefaults(GenerationConfig config, string pluginRoot)
	{
		if (String.IsNullOrWhiteSpace(config.GameExportPath))
		{
			config.GameExportPath = ResolveDefaultGameExportPath(pluginRoot);
		}

		config.GameIncludePath = GetGameIncludePath(config.GameExportPath);
	}

	private static string ResolveDefaultGameExportPath(string pluginRoot)
	{
		string moduleName = ResolveProjectGameModuleName(pluginRoot);
		return $"Source/{moduleName}/Public/UPy/Wrapper/AutoGen";
	}

	private static string ResolveProjectGameModuleName(string pluginRoot)
	{
		DirectoryInfo? pluginDir = new(pluginRoot);
		DirectoryInfo? projectRoot = pluginDir.Parent?.Parent;
		if (projectRoot == null)
		{
			return "Game";
		}

		string[] projectFiles = Directory.GetFiles(projectRoot.FullName, "*.uproject");
		foreach (string projectFile in projectFiles.OrderBy(path => path, StringComparer.Ordinal))
		{
			string? moduleName = TryReadProjectModuleName(projectFile, projectRoot.FullName);
			if (!String.IsNullOrWhiteSpace(moduleName))
			{
				return moduleName;
			}
		}

		return projectRoot.Name;
	}

	private static string? TryReadProjectModuleName(string projectFile, string projectRoot)
	{
		using JsonDocument document = JsonDocument.Parse(File.ReadAllText(projectFile));
		if (!document.RootElement.TryGetProperty("Modules", out JsonElement modulesElement) || modulesElement.ValueKind != JsonValueKind.Array)
		{
			return null;
		}

		List<(string Name, bool IsRuntime, bool HasSourceDirectory)> modules = new();
		foreach (JsonElement moduleElement in modulesElement.EnumerateArray())
		{
			if (moduleElement.ValueKind != JsonValueKind.Object ||
				!moduleElement.TryGetProperty("Name", out JsonElement nameElement) ||
				nameElement.ValueKind != JsonValueKind.String)
			{
				continue;
			}

			string? moduleName = nameElement.GetString();
			if (String.IsNullOrWhiteSpace(moduleName))
			{
				continue;
			}

			string moduleType = moduleElement.TryGetProperty("Type", out JsonElement typeElement) && typeElement.ValueKind == JsonValueKind.String
				? typeElement.GetString() ?? String.Empty
				: String.Empty;
			bool hasSourceDirectory = Directory.Exists(Path.Combine(projectRoot, "Source", moduleName));
			modules.Add((moduleName, moduleType.Equals("Runtime", StringComparison.OrdinalIgnoreCase), hasSourceDirectory));
		}

		return modules
			.OrderByDescending(module => module.HasSourceDirectory)
			.ThenByDescending(module => module.IsRuntime)
			.Select(module => module.Name)
			.FirstOrDefault();
	}

	private static string? GetGameIncludePath(string? gameExportPath)
	{
		if (String.IsNullOrWhiteSpace(gameExportPath))
		{
			return null;
		}

		// Extract include path from GameExportPath, e.g.
		// "Source/MyGame/Public/UPy/Wrapper/AutoGen" -> "UPy/Wrapper/AutoGen".
		int publicIndex = gameExportPath.IndexOf("/Public/", StringComparison.OrdinalIgnoreCase);
		if (publicIndex >= 0)
		{
			return gameExportPath[(publicIndex + 8)..];
		}

		return null;
	}

	private static string ResolveModuleName(string moduleName, string packageName)
	{
		if (!String.IsNullOrEmpty(moduleName))
		{
			return moduleName;
		}

		if (!String.IsNullOrEmpty(packageName))
		{
			int lastSlash = packageName.LastIndexOf('/');
			if (lastSlash >= 0 && lastSlash + 1 < packageName.Length)
			{
				return packageName[(lastSlash + 1)..];
			}
			return packageName;
		}

		return "Misc";
	}

	private static string SanitizeIdentifier(string name, string defaultValue)
	{
		if (String.IsNullOrEmpty(name))
		{
			return defaultValue;
		}

		StringBuilder builder = new(name.Length);
		foreach (char ch in name)
		{
			if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9'))
			{
				builder.Append(ch);
			}
			else
			{
				builder.Append('_');
			}
		}

		string result = builder.ToString();
		return result.Length > 0 ? result : defaultValue;
	}

	private static string NormalizeIncludePath(string path)
	{
		return path.Replace('\\', '/');
	}

	private static void CommitOutput(IUhtExportFactory factory, string path, StringBuilder builder)
	{
		Directory.CreateDirectory(Path.GetDirectoryName(path)!);
		string content = builder.ToString();
		string[] lines = content.Split(new[] { "\r\n", "\n" }, StringSplitOptions.None);
		factory.CommitOutput(path, String.Join(Environment.NewLine, lines));
	}

	private static void ClearDirectory(string directoryPath)
	{
		if (!Directory.Exists(directoryPath))
		{
			return;
		}

		string fullPath = Path.GetFullPath(directoryPath);
		string directoryName = Path.GetFileName(fullPath.TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar));
		// Safety check: only allow clearing AutoGen directories (or subdirectories within them)
		if (!directoryName.Equals("AutoGen", StringComparison.Ordinal) && !fullPath.Contains("AutoGen"))
		{
			Console.WriteLine($"[UnrealPython] Skipping clear for '{fullPath}' because it is not an AutoGen directory.");
			return;
		}

		Directory.Delete(fullPath, true);
	}

	private static string ResolvePluginRootDirectory(string baseDirectory)
	{
		DirectoryInfo directory = new(baseDirectory);
		DirectoryInfo? current = directory;
		while (current != null)
		{
			string toolsPath = Path.Combine(current.FullName, "Tools");
			if (Directory.Exists(toolsPath))
			{
				return current.FullName;
			}
			current = current.Parent;
		}

		return directory.FullName;
	}

	private static HashSet<string> EnsureSet(ref HashSet<string>? set)
	{
		set ??= new HashSet<string>(StringComparer.Ordinal);
		return set;
	}

	private static Dictionary<string, string> GetDefaultInlineStructs()
	{
		// Key: native type name, Value: header include
		return new Dictionary<string, string>(StringComparer.Ordinal)
		{
			{ "FVector", "Math/Vector.h" },
			{ "FVector2D", "Math/Vector2D.h" },
			{ "FVector4", "Math/Vector4.h" },
			{ "FQuat", "Math/Quat.h" },
			{ "FRotator", "Math/Rotator.h" },
			{ "FTransform", "Math/Transform.h" },
			{ "FPlane", "Math/Plane.h" },
			{ "FBox2D", "Math/Box2D.h" },
			{ "FColor", "Math/Color.h" },
			{ "FLinearColor", "Math/Color.h" }
		};
	}

	private sealed class ModuleGeneration
	{
		public ModuleGeneration(string moduleName, string outputDirectory, bool isGameModule, string? gameIncludePath)
		{
			ModuleName = moduleName;
			OutputDirectory = outputDirectory;
			IsGameModule = isGameModule;
			ModuleIdentifier = SanitizeIdentifier(moduleName, "Module");
			ModuleWrapperName = $"UPy{ModuleIdentifier}ModuleWrapper";
			InitializerName = $"Initialize{ModuleIdentifier}ModuleWrapperTypes";
			IncludePrefix = isGameModule && !String.IsNullOrEmpty(gameIncludePath) ? gameIncludePath : "Wrapper/AutoGen";
		}

		public string ModuleName { get; }
		public string OutputDirectory { get; }
		public bool IsGameModule { get; }
		public string ModuleIdentifier { get; }
		public string ModuleWrapperName { get; }
		public string InitializerName { get; }
		public string IncludePrefix { get; }
		public List<StructWrapperInfo> Structs { get; } = new();
		public List<ClassWrapperInfo> Classes { get; } = new();
	}

	private sealed class StructWrapperInfo
	{
		public StructWrapperInfo(StructJsonInfo info, AutoGenStructConfig config, string moduleName, string includePrefix)
		{
			Info = info;
			Config = config;
			WrapperSuffix = SanitizeIdentifier(info.EngineName, "Struct");
			FunctionName = $"InitializeUPyWrapper{WrapperSuffix}";
			WrapperHeader = $"{includePrefix}/{moduleName}/Struct/UPyWrapper{WrapperSuffix}.h";
			HeaderInclude = NormalizeIncludePath(info.HeaderFile);
		}

		public StructJsonInfo Info { get; }
		public AutoGenStructConfig Config { get; }
		public string WrapperSuffix { get; }
		public string FunctionName { get; }
		public string WrapperHeader { get; }
		public string HeaderInclude { get; }
	}

	private sealed class ClassWrapperInfo
	{
		public ClassWrapperInfo(ClassJsonInfo info, AutoGenClassConfig config, string moduleName, string includePrefix)
		{
			Info = info;
			Config = config;
			WrapperSuffix = SanitizeIdentifier(info.EngineName, "Class");
			FunctionName = $"InitializeUPyWrapper{WrapperSuffix}";
			WrapperHeader = $"{includePrefix}/{moduleName}/Class/UPyWrapper{WrapperSuffix}.h";
			HeaderInclude = NormalizeIncludePath(info.HeaderFile);
		}

		public ClassJsonInfo Info { get; }
		public AutoGenClassConfig Config { get; }
		public string WrapperSuffix { get; }
		public string FunctionName { get; }
		public string WrapperHeader { get; }
		public string HeaderInclude { get; }
	}

	private sealed class InlineStructRegistration
	{
		public InlineStructRegistration(string nativeTypeName, string headerInclude)
		{
			NativeTypeName = nativeTypeName;
			HeaderInclude = NormalizeIncludePath(headerInclude);
		}

		public string NativeTypeName { get; }
		public string HeaderInclude { get; }
	}

	private static List<string> SplitArgs(string input)
	{
		List<string> args = new List<string>();
		int depth = 0;
		int start = 0;
		bool inQuote = false;
		for (int i = 0; i < input.Length; i++)
		{
			char c = input[i];
			if (c == '"')
			{
				inQuote = !inQuote;
			}
			else if (!inQuote)
			{
				if (c == '(') depth++;
				else if (c == ')') depth--;
				else if (c == ',' && depth == 0)
				{
					args.Add(input.Substring(start, i - start).Trim());
					start = i + 1;
				}
			}
		}
		if (start < input.Length)
		{
			args.Add(input.Substring(start).Trim());
		}
		return args;
	}

	private static string ParseStructDefaultValue(string nativeType, string defaultValue)
	{
		if (string.IsNullOrEmpty(defaultValue) || defaultValue == "()")
		{
			return "";
		}

		// Handle (R=1.000000,G=0.000000,B=0.000000,A=1.000000) format
		if (defaultValue.StartsWith("(") && defaultValue.EndsWith(")"))
		{
			string content = defaultValue.Substring(1, defaultValue.Length - 2);
			List<string> parts = SplitArgs(content);
			List<string> values = new List<string>();
			
			foreach (string part in parts)
			{
				int eqIndex = part.IndexOf('=');
				if (eqIndex > 0)
				{
					values.Add(part.Substring(eqIndex + 1).Trim());
				}
				else
				{
					values.Add(part.Trim());
				}
			}

			if (values.Count > 0)
			{
				return $" = {nativeType}({string.Join(", ", values)})";
			}
		}

		// Handle plain comma-separated values (e.g., 0.0,1.0,0.0)
		if (defaultValue.Contains(",") && !defaultValue.Contains("(") && !defaultValue.Contains(")"))
		{
			return $" = {nativeType}({defaultValue})";
		}

		// Fallback: assign as-is
		return $" = {defaultValue}";
	}
}
