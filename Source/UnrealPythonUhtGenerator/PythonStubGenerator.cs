using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using EpicGames.Core;
using EpicGames.UHT.Types;

namespace UnrealPythonUhtGenerator;

internal static class PythonStubGenerator
{
	public static IEnumerable<(string Content, string Suffix)> GenerateStub(RootJson metadata, GenerationConfig config, int includeComments = 0)
	{
		if (includeComments == 2)
		{
			yield return (new StubWriter(metadata, config, true).Build(), "");
			yield return (new StubWriter(metadata, config, false).Build(), "no_comments");
		}
		else
		{
			yield return (new StubWriter(metadata, config, includeComments != 0).Build(), "");
		}
	}

	private sealed class StubWriter
	{
		private const string IndentUnit = "    ";

		private static readonly Dictionary<string, string> BuiltinTypeMap = new(StringComparer.OrdinalIgnoreCase)
		{
			{ "float", "float" },
			{ "double", "float" },
			{ "int8", "int" },
			{ "uint8", "int" },
			{ "int16", "int" },
			{ "uint16", "int" },
			{ "int32", "int" },
			{ "uint32", "int" },
			{ "int64", "int" },
			{ "uint64", "int" },
			{ "bool", "bool" },
			{ "FString", "str" },
			{ "FName", "str" },
			{ "FText", "str" },
			{ "string", "str" },
			{ "name", "str" },
			{ "text", "str" },
			{ "guid", "str" },
			{ "BoolProperty", "bool" },
			{ "IntProperty", "int" },
			{ "Int8Property", "int" },
			{ "Int16Property", "int" },
			{ "Int64Property", "int" },
			{ "UInt16Property", "int" },
			{ "UInt32Property", "int" },
			{ "UInt64Property", "int" },
			{ "ByteProperty", "int" },
			{ "EnumProperty", "int" },
			{ "FloatProperty", "float" },
			{ "DoubleProperty", "float" },
			{ "LargeWorldCoordinatesRealProperty", "float" },
			{ "StrProperty", "str" },
			{ "AnsiStrProperty", "str" },
			{ "NameProperty", "str" },
			{ "TextProperty", "str" },
			{ "FieldPathProperty", "FieldPath" }
		};

		private static readonly HashSet<string> PythonKeywords = new(StringComparer.Ordinal)
		{
			"False", "None", "True", "and", "as", "assert", "async", "await",
			"break", "class", "continue", "def", "del", "elif", "else", "except",
			"finally", "for", "from", "global", "if", "import", "in", "is",
			"lambda", "nonlocal", "not", "or", "pass", "raise", "return",
			"try", "while", "with", "yield"
		};

		private static readonly Dictionary<string, string> SpecialDefaultValueMap = new(StringComparer.Ordinal)
		{
			{ "FVector::ZeroVector", "Vector.ZeroVector" },
			{ "FVector2D::ZeroVector", "Vector2D.ZeroVector" },
			{ "FVector4::ZeroVector", "Vector4.ZeroVector" },
			{ "FQuat::Identity", "Quat.Identity" },
			{ "FRotator::ZeroRotator", "Rotator.ZeroRotator" },
			{ "FTransform::Identity", "Transform.Identity" },
			{ "FName::None", "\"\"" },
			{ "NAME_None", "\"\"" },
			{ "Name_NONE", "\"\"" }
		};

		private static readonly Dictionary<string, string> SyntheticDefaultValueMap = new(StringComparer.Ordinal)
		{
			{ "bool", "False" },
			{ "int", "0" },
			{ "float", "0.0" },
			{ "str", "\"\"" },
			{ "Vector", "Vector()" },
			{ "Vector2D", "Vector2D()" },
			{ "Vector2f", "Vector2f()" },
			{ "Vector2d", "Vector2d()" },
			{ "Vector3f", "Vector3f()" },
			{ "Vector3d", "Vector3d()" },
			{ "Vector4", "Vector4()" },
			{ "Vector4f", "Vector4f()" },
			{ "Vector4d", "Vector4d()" },
			{ "Quat", "Quat()" },
			{ "Rotator", "Rotator()" },
			{ "Transform", "Transform()" },
			{ "LinearColor", "LinearColor()" },
			{ "Color", "Color()" },
			{ "Guid", "Guid()" },
			{ "FieldPath", "FieldPath()" }
		};

		private static readonly Dictionary<string, string[]> StructComponentOrderMap = new(StringComparer.Ordinal)
		{
			{ "Vector", new[] { "X", "Y", "Z" } },
			{ "Vector2D", new[] { "X", "Y" } },
			{ "Vector2f", new[] { "X", "Y" } },
			{ "Vector2d", new[] { "X", "Y" } },
			{ "Vector3f", new[] { "X", "Y", "Z" } },
			{ "Vector3d", new[] { "X", "Y", "Z" } },
			{ "Vector4", new[] { "X", "Y", "Z", "W" } },
			{ "Vector4f", new[] { "X", "Y", "Z", "W" } },
			{ "Vector4d", new[] { "X", "Y", "Z", "W" } },
			{ "Quat", new[] { "X", "Y", "Z", "W" } },
			{ "Rotator", new[] { "Pitch", "Yaw", "Roll" } },
			{ "LinearColor", new[] { "R", "G", "B", "A" } },
			{ "Color", new[] { "R", "G", "B", "A" } }
		};

		private static readonly Dictionary<string, string> GlobalConstantsMap = new(StringComparer.Ordinal)
		{
			{"GIsEditor", "bool"},
			{"BUILD_SHIPPING", "bool"}
		};

		private readonly RootJson _root;
		private readonly GenerationConfig _config;
		private readonly bool _includeComments;
		private readonly StringBuilder _builder = new();
		private readonly List<NativeModuleJsonInfo> _nonUnrealNativeModules;
		private readonly NativeModuleJsonInfo? _unrealNativeModule;
		private readonly HashSet<string> _emittedTypes = new(StringComparer.Ordinal);

		public StubWriter(RootJson root, GenerationConfig config, bool includeComments)
		{
			_root = root;
			_config = config;
			_includeComments = includeComments;
			_nonUnrealNativeModules = root.NativeModules
				.Where(module => !module.ModuleName.Equals("ue", StringComparison.OrdinalIgnoreCase))
				.OrderBy(module => module.ModuleName, StringComparer.Ordinal)
				.ToList();
			_unrealNativeModule = root.NativeModules.FirstOrDefault(module => module.ModuleName.Equals("ue", StringComparison.OrdinalIgnoreCase));
		}

		public string Build()
		{
			WriteHeader();
			WriteUnrealNativeModule();
			WriteNonUnrealNativeModules();
			WriteGlobalConstants();
			WriteEnums();
			WriteStructs();
			WriteClasses();
			return _builder.ToString();
		}

		private void WriteHeader()
		{
			WriteLine("# Auto-generated by UnrealPythonUhtGenerator. DO NOT EDIT.");
			WriteLine("# pyright: reportGeneralTypeIssues=false, strictParameterNoneValue=false, reportImplicitOverride=false, reportInvalidStringEscapeSequence=false");
			WriteLine("from __future__ import annotations");
			WriteLine("from typing import Type, Iterable, Callable, Any, Union, ItemsView, KeysView, ValuesView, Optional, TypeVar, overload, Generic");
			WriteLine("from enum import IntEnum");
			WriteLine();
			WriteLine("_ElemType = TypeVar(\"_ElemType\")");
			WriteLine("_KeyType = TypeVar(\"_KeyType\")");
			WriteLine("_ValueType = TypeVar(\"_ValueType\")");
			WriteLine("_T = TypeVar(\"_T\")");
			WriteLine();
		}

		private void WriteNonUnrealNativeModules()
		{
			if (_nonUnrealNativeModules.Count == 0)
			{
				return;
			}

			WriteLine("# Native helper modules");
			foreach (NativeModuleJsonInfo module in _nonUnrealNativeModules)
			{
				string className = SanitizeIdentifier(module.ModuleName);
				if (!_emittedTypes.Add(className))
				{
					continue;
				}

				string? genericBase = GetGenericBaseType(className);
				if (className == "EnumBase")
				{
					WriteLine($"class {className}(IntEnum):");
				}
				else if (genericBase != null)
				{
					WriteLine($"class {className}({genericBase}):");
				}
				else
				{
					WriteLine($"class {className}:");
				}
				if (_includeComments)
				{
					WriteIndent(1);
					WriteLine($"\"\"\"Native functions for module '{module.ModuleName}'.\"\"\"");
				}
				if (module.NativeMethods.Count == 0)
				{
					WriteIndent(1);
					WriteLine("...");
					WriteLine();
					continue;
				}

				WriteLine();
				WriteNativeModuleMethods(module, 1);
				WriteLine();
			}
		}

		private void WriteGlobalConstants()
		{
			if (GlobalConstantsMap.Count == 0)
			{
				return;
			}

			WriteLine("# Global Constants");
			foreach (KeyValuePair<string, string> entry in GlobalConstantsMap)
			{
				WriteLine($"{entry.Key}: {entry.Value}");
			}
			WriteLine();
		}

		private void WriteUnrealNativeModule()
		{
			if (_unrealNativeModule == null || _unrealNativeModule.NativeMethods.Count == 0)
			{
				return;
			}

			WriteLine("# Native functions on the 'ue' module");
			WriteNativeModuleMethods(_unrealNativeModule, 0);
		}

		private void WriteNativeModuleMethods(NativeModuleJsonInfo module, int indentLevel)
		{
			IEnumerable<IGrouping<string, NativeModuleMethodJsonInfo>> methodGroups = module.NativeMethods
				.GroupBy(method => method.MethodName, StringComparer.Ordinal)
				.OrderBy(group => group.Key, StringComparer.Ordinal);

			foreach (IGrouping<string, NativeModuleMethodJsonInfo> group in methodGroups)
			{
				bool useOverload = group.Count() > 1;
				IEnumerable<NativeModuleMethodJsonInfo> orderedMethods = group.OrderBy(method => method.MethodDoc, StringComparer.Ordinal);
				foreach (NativeModuleMethodJsonInfo method in orderedMethods)
				{
					(string signature, string? documentation) = SplitNativeMethodDoc(method);
					if (useOverload)
					{
						WriteIndent(indentLevel);
						WriteLine("@overload");
					}

					bool isClassMethod = method.MethodName.Equals("StaticClass", StringComparison.Ordinal);
					if (isClassMethod)
					{
						WriteIndent(indentLevel);
						WriteLine("@classmethod");
					}
					else if ((method.PyMethodFlags & 0x0010) != 0) // METH_CLASS
					{
						WriteIndent(indentLevel);
						WriteLine("@classmethod");
					}
					else if ((method.PyMethodFlags & 0x0020) != 0) // METH_STATIC
					{
						WriteIndent(indentLevel);
						WriteLine("@staticmethod");
					}

					WriteIndent(indentLevel);
					WriteLine($"def {signature}:");

					if (_includeComments && !String.IsNullOrWhiteSpace(documentation))
					{
						WriteDocstring(documentation, indentLevel + 1);
					}

					WriteIndent(indentLevel + 1);
					WriteLine("...");
					WriteLine();
				}
			}
		}

		private static (string Signature, string? Documentation) SplitNativeMethodDoc(NativeModuleMethodJsonInfo method)
		{
			string rawDoc = method.MethodDoc ?? String.Empty;
			string signature;
			string? documentation = null;

			int separatorIndex = rawDoc.IndexOf(" -- ", StringComparison.Ordinal);
			if (separatorIndex >= 0)
			{
				signature = rawDoc.Substring(0, separatorIndex).Trim();
				documentation = rawDoc.Substring(separatorIndex + 4).Trim();
			}
			else
			{
				signature = rawDoc.Trim();
			}

			if (signature.Length == 0 || !signature.StartsWith(method.MethodName, StringComparison.Ordinal) || !signature.Contains('('))
			{
				signature = CreateNativeFallbackSignature(method.MethodName);
			}

			if (documentation != null && documentation.Length == 0)
			{
				documentation = null;
			}

			return (signature, documentation);
		}

		private static string CreateNativeFallbackSignature(string methodName)
		{
			string sanitizedName = SanitizeIdentifier(methodName);
			return $"{sanitizedName}(*args: Any, **kwargs: Any) -> Any";
		}

		private static string? GetGenericBaseType(string name)
		{
			if (name.Equals("Array", StringComparison.Ordinal) ||
				name.Equals("Set", StringComparison.Ordinal) ||
				name.Equals("FixedArray", StringComparison.Ordinal))
			{
				return "Generic[_ElemType]";
			}
			if (name.Equals("Map", StringComparison.Ordinal))
			{
				return "Generic[_KeyType, _ValueType]";
			}
			return null;
		}

		private void WriteEnums()
		{
			if (_root.Enums.Count == 0)
			{
				return;
			}

			WriteLine("# Enumerations");
			foreach (EnumJsonInfo enumInfo in _root.Enums)
			{
				if (!_emittedTypes.Add(enumInfo.EnumName))
				{
					continue;
				}

				WriteLine($"class {enumInfo.EnumName}(IntEnum):");
				if (_includeComments && !String.IsNullOrWhiteSpace(enumInfo.EnumDoc))
				{
					WriteDocstring(enumInfo.EnumDoc, 1);
				}

				foreach (EnumEntryJsonInfo entry in enumInfo.EnumEntries)
				{
					WriteIndent(1);
					WriteLine($"{entry.EntryName} = {entry.EntryValue.ToString(CultureInfo.InvariantCulture)}");
				}

				if (enumInfo.EnumEntries.Count == 0)
				{
					WriteIndent(1);
					WriteLine("...");
				}

				WriteLine();
			}
		}

		private void WriteStructs()
		{
			if (_root.Structs.Count == 0)
			{
				return;
			}

			WriteLine("# Structs");
			foreach (StructJsonInfo structInfo in _root.Structs)
			{
				if (!_emittedTypes.Add(structInfo.StructName))
				{
					continue;
				}

				string baseType = structInfo.BaseType.Length > 0 ? structInfo.BaseType : "StructBase";
				string? genericBase = GetGenericBaseType(structInfo.StructName);
				if (genericBase != null)
				{
					baseType += $", {genericBase}";
				}
				WriteLine($"class {structInfo.StructName}({baseType}):");
				if (_includeComments && !String.IsNullOrWhiteSpace(structInfo.StructDoc))
				{
					WriteDocstring(structInfo.StructDoc, 1);
				}

				_config.Structs.TryGetValue(structInfo.StructName, out AutoGenStructConfig? structConfig);

				List<PropJsonInfo> visibleProps = structInfo.StructProps.ToList();
				if (structConfig != null && structConfig.ExcludeProperties != null)
				{
					visibleProps = visibleProps.Where(p => !structConfig.ExcludeProperties.Contains(p.PropName)).ToList();
				}
				WriteProperties(visibleProps, 1);
				WriteMethodStub(structInfo.MakeFunc, 1, true);
				WriteMethodStub(structInfo.BreakFunc, 1, true);

				List<MethodJsonInfo> structConstants = structInfo.StructMethods.Where(method => method.bIsConstant).ToList();
				foreach (MethodJsonInfo constantMethod in structConstants)
				{
					WriteConstantAttribute(constantMethod);
				}

				List<MethodJsonInfo> nonConstantStructMethods = structInfo.StructMethods
					.Where(method => !method.bIsConstant)
					.ToList();

				if (structConfig != null && structConfig.ExcludeMethods != null)
				{
					nonConstantStructMethods = nonConstantStructMethods.Where(m => !structConfig.ExcludeMethods.Contains(m.MethodName)).ToList();
				}
				WriteMethodGroups(nonConstantStructMethods, 1, false);

				bool hasStructContent =
					visibleProps.Count > 0 ||
					!String.IsNullOrWhiteSpace(structInfo.MakeFunc.MethodName) ||
					!String.IsNullOrWhiteSpace(structInfo.BreakFunc.MethodName) ||
					structConstants.Count > 0 ||
					nonConstantStructMethods.Count > 0;

				if (!hasStructContent)
				{
					WriteIndent(1);
					WriteLine("...");
				}

				WriteLine();
			}
		}

		private void WriteClasses()
		{
			if (_root.Classes.Count == 0)
			{
				return;
			}

			WriteLine("# Classes");
			foreach (ClassJsonInfo classInfo in _root.Classes)
			{
				if (classInfo.ClassName.Equals("UPyEditorScriptExportHelperLibrary", StringComparison.OrdinalIgnoreCase))	
				{
					continue;
				}

				if (!_emittedTypes.Add(classInfo.ClassName))
				{
					continue;
				}

				string primaryBase = classInfo.BaseType;
				if (String.IsNullOrEmpty(primaryBase))
				{
					primaryBase = classInfo.ClassName.Equals("Object", StringComparison.Ordinal) ? "_ObjectBase" : "Object";
				}

				List<string> baseTypes = new() { primaryBase };
				string? genericBase = GetGenericBaseType(classInfo.ClassName);
				if (genericBase != null)
				{
					baseTypes.Add(genericBase);
				}

				foreach (InterfaceJsonInfo interfaceInfo in classInfo.ImplementedInterfaces)
			{
				if (!String.IsNullOrWhiteSpace(interfaceInfo.InterfaceName))
				{
					baseTypes.Add(interfaceInfo.InterfaceName);
				}
			}

				string baseList = String.Join(", ", baseTypes);
				WriteLine($"class {classInfo.ClassName}({baseList}):");
				if (_includeComments && !String.IsNullOrWhiteSpace(classInfo.ClassDoc))
				{
					WriteDocstring(classInfo.ClassDoc, 1);
				}

				_config.Classes.TryGetValue(classInfo.ClassName, out AutoGenClassConfig? classConfig);

				List<PropJsonInfo> visibleProps = classInfo.ClassProps.ToList();
				if (classConfig != null && classConfig.ExcludeProperties != null)
				{
					visibleProps = visibleProps.Where(p => !classConfig.ExcludeProperties.Contains(p.PropName)).ToList();
				}
				WriteProperties(visibleProps, 1);

				List<MethodJsonInfo> constantMethods = classInfo.ClassMethods.Where(method => method.bIsConstant).ToList();
				foreach (MethodJsonInfo constantMethod in constantMethods)
				{
					WriteConstantAttribute(constantMethod);
				}

				List<MethodJsonInfo> nonConstantMethods = classInfo.ClassMethods
					.Where(method => !method.bIsConstant)
					.ToList();

				if (classConfig != null && classConfig.ExcludeMethods != null)
				{
					nonConstantMethods = nonConstantMethods.Where(m => !classConfig.ExcludeMethods.Contains(m.MethodName)).ToList();
				}
				WriteMethodGroups(nonConstantMethods, 1, false);

			bool hasClassContent =
				visibleProps.Count > 0 ||
				constantMethods.Count > 0 ||
				nonConstantMethods.Count > 0;

				if (!hasClassContent)
				{
					WriteIndent(1);
					WriteLine("...");
				}

				WriteLine();
			}
		}

		private void WriteProperties(IEnumerable<PropJsonInfo> properties, int indentLevel)
		{
			foreach (PropJsonInfo prop in properties)
			{
				if (_includeComments && !String.IsNullOrWhiteSpace(prop.PropDoc))
				{
					WriteIndent(indentLevel);
					WriteLine($"# {NormalizeInlineText(prop.PropDoc)}");
				}

				string propName = SanitizeIdentifier(prop.PropName);
				string annotation = GetTypeAnnotation(prop);
				WriteIndent(indentLevel);
				WriteLine($"{propName}: {annotation}");
			}

			if (properties.Any())
			{
				WriteLine();
			}
		}

		private void WriteMethodGroups(List<MethodJsonInfo> methods, int indentLevel, bool isStructHelper)
		{

			if (methods.Count == 0)
			{
				return;
			}

			foreach (IGrouping<string, MethodJsonInfo> group in methods
				.GroupBy(method => method.MethodName, StringComparer.Ordinal)
				.OrderBy(group => group.Key, StringComparer.Ordinal))
			{
				List<MethodJsonInfo> uniqueMethods = new();
				HashSet<string> seenSignatures = new(StringComparer.Ordinal);

				foreach (MethodJsonInfo method in group)
				{
					if (String.IsNullOrWhiteSpace(method.MethodName))
					{
						continue;
					}

					bool isStatic = isStructHelper || IsStaticMethod(method);
					string signature = BuildMethodSignature(method, isStatic);
					string returnType = GetMethodReturnType(method);
					string fullSignature = $"{signature} -> {returnType}";

					if (seenSignatures.Add(fullSignature))
					{
						uniqueMethods.Add(method);
					}
				}

				bool useOverload = uniqueMethods.Count > 1;
				foreach (MethodJsonInfo method in uniqueMethods)
				{
					WriteMethodStub(method, indentLevel, isStructHelper, useOverload);
				}
			}
		}

		private void WriteMethodStub(MethodJsonInfo method, int indentLevel, bool isStructHelper, bool useOverload = false)
		{
			if (method.bIsConstant)
			{
				return;
			}

			// Visibility check is handled by caller (WriteMethodGroups or WriteStructs)

			if (String.IsNullOrWhiteSpace(method.MethodName))
			{
				return;
			}

			bool isStatic = isStructHelper || IsStaticMethod(method);

			if (useOverload)
			{
				WriteIndent(indentLevel);
				WriteLine("@overload");
			}

			if (isStatic)
			{
				WriteIndent(indentLevel);
				WriteLine("@staticmethod");
			}

			string signature = BuildMethodSignature(method, isStatic);
			string returnType = GetMethodReturnType(method);

			WriteIndent(indentLevel);
			WriteLine($"def {signature} -> {returnType}:");

			if (_includeComments && !String.IsNullOrWhiteSpace(method.MethodDoc))
			{
				WriteDocstring(method.MethodDoc, indentLevel + 1);
			}

			WriteIndent(indentLevel + 1);
			WriteLine("...");
			WriteLine();
		}

		private string BuildMethodSignature(MethodJsonInfo method, bool isStatic)
		{
			List<string> parameters = new();
			bool seenExplicitDefault = false;

			if (!isStatic)
			{
				parameters.Add("self");
			}

			foreach (PropJsonInfo param in method.Params)
			{
				if (IsReturnParameter(param))
				{
					continue;
				}

				if (!IsInputParameter(param))
				{
					continue;
				}

				string name = SanitizeIdentifier(param.PropName);
				string annotation = GetTypeAnnotation(param);
				string defaultSuffix = FormatDefaultValue(param);

				bool hasExplicitDefault = defaultSuffix.Length > 0;

				if (!hasExplicitDefault && seenExplicitDefault)
				{
					string implicitDefault = GetImplicitDefaultValue(param, annotation);
					defaultSuffix = $"={implicitDefault}";
				}

				if (hasExplicitDefault)
				{
					seenExplicitDefault = true;
				}

				parameters.Add($"{name}: {annotation}{defaultSuffix}");
			}

			string joined = String.Join(", ", parameters);
			return $"{SanitizeIdentifier(method.MethodName)}({joined})";
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
			bool isParam = (flags & EPropertyFlags.Parm) != 0;
			bool isReturn = (flags & EPropertyFlags.ReturnParm) != 0;
			bool isReference = (flags & EPropertyFlags.ReferenceParm) != 0;
			bool isOut = (flags & EPropertyFlags.OutParm) != 0 && (flags & EPropertyFlags.ConstParm) == 0;
			return isParam && !isReturn && (!isOut || isReference);
		}

		private static bool IsOutputParameter(PropJsonInfo param)
		{
			EPropertyFlags flags = (EPropertyFlags)param.PropFlags;
			bool isParam = (flags & EPropertyFlags.Parm) != 0;
			bool isReturn = (flags & EPropertyFlags.ReturnParm) != 0;
			bool isOut = (flags & EPropertyFlags.OutParm) != 0 && (flags & EPropertyFlags.ConstParm) == 0;
			return isParam && !isReturn && isOut;
		}

		private string GetMethodReturnType(MethodJsonInfo method)
		{
			List<PropJsonInfo> outputs = new();

			PropJsonInfo? returnParam = method.Params.FirstOrDefault(IsReturnParameter);
			if (returnParam != null)
			{
				outputs.Add(returnParam);
			}

			foreach (PropJsonInfo param in method.Params)
			{
				if (!IsReturnParameter(param) && IsOutputParameter(param))
				{
					outputs.Add(param);
				}
			}

			if (outputs.Count == 0)
			{
				return "None";
			}

			if (outputs.Count == 1)
			{
				return GetTypeAnnotation(outputs[0]);
			}

			string tupleTypes = String.Join(", ", outputs.Select(GetTypeAnnotation));
			return $"tuple[{tupleTypes}]";
		}

		private string GetTypeAnnotation(PropJsonInfo prop)
		{
			string propType = prop.PropType ?? String.Empty;
			string? typeName = String.IsNullOrWhiteSpace(prop.TypeName) ? null : prop.TypeName;

			if (propType.Equals("DelegateProperty", StringComparison.OrdinalIgnoreCase))
			{
				return "DelegateBase";
			}

			if (propType.Equals("MulticastDelegateProperty", StringComparison.OrdinalIgnoreCase) ||
				propType.Equals("MulticastInlineDelegateProperty", StringComparison.OrdinalIgnoreCase) ||
				propType.Equals("MulticastSparseDelegateProperty", StringComparison.OrdinalIgnoreCase))
			{
				return "MulticastDelegateBase";
			}

			if (propType.Equals("ArrayProperty", StringComparison.OrdinalIgnoreCase))
			{
				string elementType = typeName ?? "Any";
				elementType = NormalizeSimpleType(elementType);
				return $"list[{elementType}]";
			}

			if (propType.Equals("SetProperty", StringComparison.OrdinalIgnoreCase))
			{
				string elementType = typeName ?? "Any";
				elementType = NormalizeSimpleType(elementType);
				return $"set[{elementType}]";
			}

			if (propType.Equals("MapProperty", StringComparison.OrdinalIgnoreCase))
			{
				string keyType = "Any";
				string valueType = "Any";

				if (!String.IsNullOrWhiteSpace(typeName))
				{
					string[] parts = typeName!.Split(new[] { "->" }, 2, StringSplitOptions.None);
					if (parts.Length > 0)
					{
						keyType = NormalizeSimpleType(parts[0].Trim());
					}
					if (parts.Length > 1)
					{
						valueType = NormalizeSimpleType(parts[1].Trim());
					}
				}

				return $"dict[{keyType}, {valueType}]";
			}

			if (propType.Equals("ObjectProperty", StringComparison.OrdinalIgnoreCase))
			{
				string annotation = "Any";
				if (!String.IsNullOrWhiteSpace(typeName))
				{
					annotation = NormalizeSimpleType(typeName!);
				}

				if (IsNullDefaultValue(prop))
				{
					return $"{annotation} | None";
				}

				return annotation;
			}

			if (propType.Equals("ClassProperty", StringComparison.OrdinalIgnoreCase))
			{
				string annotation = "Any";
				if (!String.IsNullOrWhiteSpace(typeName))
				{
					annotation = NormalizeSimpleType(typeName!);
				}

				string suffix = IsNullDefaultValue(prop) ? " | None" : "";
				return $"{annotation} | type{suffix}";
			}

			if (!String.IsNullOrWhiteSpace(typeName))
			{
				return NormalizeSimpleType(typeName!);
			}

			if (!String.IsNullOrWhiteSpace(prop.PropType) && BuiltinTypeMap.TryGetValue(prop.PropType, out string? builtin))
			{
				return builtin;
			}

			return "Any";
		}

		private string GetImplicitDefaultValue(PropJsonInfo prop, string annotation)
		{
			if (SyntheticDefaultValueMap.TryGetValue(annotation, out string? directMatch))
			{
				return directMatch;
			}

			string? normalizedTypeName = null;
			string? mappedTypeName = null;

			if (!String.IsNullOrWhiteSpace(prop.TypeName))
			{
				normalizedTypeName = NormalizeSimpleType(prop.TypeName!);

				if (SyntheticDefaultValueMap.TryGetValue(normalizedTypeName, out string? normalizedMatch))
				{
					return normalizedMatch;
				}

				mappedTypeName = MapConstantTypeName(normalizedTypeName);
				if (SyntheticDefaultValueMap.TryGetValue(mappedTypeName, out string? mappedMatch))
				{
					return mappedMatch;
				}
			}

			string propType = prop.PropType ?? String.Empty;

			if (propType.Equals("StructProperty", StringComparison.OrdinalIgnoreCase))
			{
				string candidate = mappedTypeName ?? normalizedTypeName ?? annotation;
				if (!String.IsNullOrEmpty(candidate) && IsSimpleIdentifier(candidate))
				{
					return $"{candidate}()";
				}
			}

			if (propType.Equals("BoolProperty", StringComparison.OrdinalIgnoreCase))
			{
				return "False";
			}

			if (propType.Equals("FloatProperty", StringComparison.OrdinalIgnoreCase) ||
				propType.Equals("DoubleProperty", StringComparison.OrdinalIgnoreCase))
			{
				return "0.0";
			}

			if (propType.Equals("StrProperty", StringComparison.OrdinalIgnoreCase) ||
				propType.Equals("AnsiStrProperty", StringComparison.OrdinalIgnoreCase) ||
				propType.Equals("NameProperty", StringComparison.OrdinalIgnoreCase) ||
				propType.Equals("TextProperty", StringComparison.OrdinalIgnoreCase))
			{
				return "\"\"";
			}

			if (propType.Equals("EnumProperty", StringComparison.OrdinalIgnoreCase) ||
				propType.Equals("ByteProperty", StringComparison.OrdinalIgnoreCase) ||
				propType.Equals("IntProperty", StringComparison.OrdinalIgnoreCase) ||
				propType.Equals("Int8Property", StringComparison.OrdinalIgnoreCase) ||
				propType.Equals("Int16Property", StringComparison.OrdinalIgnoreCase) ||
				propType.Equals("Int64Property", StringComparison.OrdinalIgnoreCase) ||
				propType.Equals("UInt16Property", StringComparison.OrdinalIgnoreCase) ||
				propType.Equals("UInt32Property", StringComparison.OrdinalIgnoreCase) ||
				propType.Equals("UInt64Property", StringComparison.OrdinalIgnoreCase))
			{
				return "0";
			}

			if (propType.Equals("ArrayProperty", StringComparison.OrdinalIgnoreCase) ||
				propType.Equals("SetProperty", StringComparison.OrdinalIgnoreCase) ||
				propType.Equals("MapProperty", StringComparison.OrdinalIgnoreCase))
			{
				return "...";
			}

			if (propType.Equals("ObjectProperty", StringComparison.OrdinalIgnoreCase) ||
				propType.Equals("ClassProperty", StringComparison.OrdinalIgnoreCase) ||
				propType.Equals("InterfaceProperty", StringComparison.OrdinalIgnoreCase) ||
				propType.Equals("WeakObjectProperty", StringComparison.OrdinalIgnoreCase) ||
				propType.Equals("LazyObjectProperty", StringComparison.OrdinalIgnoreCase) ||
				propType.Equals("SoftObjectProperty", StringComparison.OrdinalIgnoreCase) ||
				propType.Equals("SoftClassProperty", StringComparison.OrdinalIgnoreCase))
			{
				return "None";
			}

			return "...";
		}

		private string FormatDefaultValue(PropJsonInfo prop)
		{
			if (String.IsNullOrWhiteSpace(prop.DefaultValue))
			{
				return String.Empty;
			}

			string raw = prop.DefaultValue!.Trim();
			if (raw.Length == 0)
			{
				return String.Empty;
			}

			if (String.Equals(prop.PropType, "EnumProperty", StringComparison.OrdinalIgnoreCase) ||
				String.Equals(prop.PropType, "ByteProperty", StringComparison.OrdinalIgnoreCase))
			{
				string? enumExpression = TryFormatEnumDefaultValue(prop, raw);
				if (!String.IsNullOrEmpty(enumExpression))
				{
					return $"={enumExpression}";
				}
			}

			string? structExpression = TryFormatStructDefaultValue(prop, raw);
			if (!String.IsNullOrEmpty(structExpression))
			{
				return $"={structExpression}";
			}

			if (SpecialDefaultValueMap.TryGetValue(raw, out string? mappedValue))
			{
				return $"={mappedValue}";
			}

			if (String.Equals(raw, "None", StringComparison.OrdinalIgnoreCase) &&
				String.Equals(prop.PropType, "NameProperty", StringComparison.Ordinal))
			{
				return "=\"\"";
			}

			if (String.Equals(raw, "true", StringComparison.OrdinalIgnoreCase))
			{
				return "=True";
			}
			if (String.Equals(raw, "false", StringComparison.OrdinalIgnoreCase))
			{
				return "=False";
			}
			if (String.Equals(raw, "nullptr", StringComparison.OrdinalIgnoreCase) ||
				String.Equals(raw, "null", StringComparison.OrdinalIgnoreCase) ||
				String.Equals(raw, "None", StringComparison.OrdinalIgnoreCase))
			{
				return "=None";
			}

			string numericCandidate = raw;
			if (numericCandidate.EndsWith("f", StringComparison.OrdinalIgnoreCase))
			{
				numericCandidate = numericCandidate[..^1];
			}

			if (Int64.TryParse(numericCandidate, NumberStyles.Integer, CultureInfo.InvariantCulture, out long intValue))
			{
				return $"={intValue.ToString(CultureInfo.InvariantCulture)}";
			}

			if (Double.TryParse(numericCandidate, NumberStyles.Float, CultureInfo.InvariantCulture, out double doubleValue))
			{
				return $"={doubleValue.ToString(CultureInfo.InvariantCulture)}";
			}

			if (raw.StartsWith("TEXT(\"", StringComparison.Ordinal) && raw.EndsWith("\")"))
			{
				string contents = raw.Substring(6, raw.Length - 8);
				return $"='{EscapePythonString(contents)}'";
			}

			if ((raw.StartsWith("\"", StringComparison.Ordinal) && raw.EndsWith("\"")) ||
				(raw.StartsWith("'", StringComparison.Ordinal) && raw.EndsWith("'")))
			{
				string contents = raw.Substring(1, raw.Length - 2);
				return $"='{EscapePythonString(contents)}'";
			}

			if (raw.Contains("::"))
			{
				string[] parts = raw.Split(new[] { "::" }, 2, StringSplitOptions.None);
				if (parts.Length == 2 && IsSimpleIdentifier(parts[0]) && IsSimpleIdentifier(parts[1]))
				{
					string typeName = MapConstantTypeName(parts[0]);
					string constantName = parts[1];
					if (!String.IsNullOrEmpty(typeName) && !String.IsNullOrEmpty(constantName))
					{
						return $"={typeName}.{constantName}";
					}
				}
			}

			return $"='{EscapePythonString(raw)}'";
		}

		private string? TryFormatEnumDefaultValue(PropJsonInfo prop, string raw)
		{
			string? enumTypeName = String.IsNullOrWhiteSpace(prop.TypeName) ? null : NormalizeSimpleType(prop.TypeName!);
			if (String.IsNullOrEmpty(enumTypeName) || BuiltinTypeMap.Values.Contains(enumTypeName))
			{
				return null;
			}

			string candidateValue = raw;
			if (candidateValue.Contains("::"))
			{
				string[] parts = candidateValue.Split(new[] { "::" }, StringSplitOptions.RemoveEmptyEntries);
				if (parts.Length > 0)
				{
					candidateValue = parts[^1];
				}
			}

			candidateValue = candidateValue.Trim();
			if (candidateValue.Length == 0)
			{
				return null;
			}

			EnumJsonInfo? enumInfo = _root.Enums.FirstOrDefault(enumEntry =>
				enumEntry.EnumName.Equals(enumTypeName, StringComparison.Ordinal));

			string? resolvedEntryName = null;
			if (enumInfo != null)
			{
				resolvedEntryName = FindEnumEntryName(enumInfo, candidateValue);
			}

			resolvedEntryName ??= candidateValue;

			return $"{enumTypeName}.{resolvedEntryName}";
		}

		private string? TryFormatStructDefaultValue(PropJsonInfo prop, string raw)
		{
			if (!String.Equals(prop.PropType, "StructProperty", StringComparison.OrdinalIgnoreCase))
			{
				return null;
			}

			string? rawTypeName = String.IsNullOrWhiteSpace(prop.TypeName) ? null : prop.TypeName;
			if (String.IsNullOrEmpty(rawTypeName))
			{
				return null;
			}

			string typeName = NormalizeSimpleType(rawTypeName);
			if (!StructComponentOrderMap.ContainsKey(typeName))
			{
				string mappedTypeName = MapConstantTypeName(typeName);
				if (StructComponentOrderMap.ContainsKey(mappedTypeName))
				{
					typeName = mappedTypeName;
				}
			}

			if (raw == "()")
			{
				return $"{typeName}()";
			}

			if (!StructComponentOrderMap.TryGetValue(typeName, out string[]? componentOrder))
			{
				return null;
			}

			return TryParseNamedStructInitializer(raw, typeName, componentOrder);
		}

		private static string? TryParseNamedStructInitializer(string raw, string typeName, IReadOnlyList<string> componentOrder)
		{
			string candidate = raw.Trim();

			if ((candidate.StartsWith("'", StringComparison.Ordinal) && candidate.EndsWith("'", StringComparison.Ordinal)) ||
				(candidate.StartsWith("\"", StringComparison.Ordinal) && candidate.EndsWith("\"", StringComparison.Ordinal)))
			{
				candidate = candidate.Substring(1, candidate.Length - 2).Trim();
			}

			bool hadTypePrefix = false;
			if (candidate.StartsWith(typeName + "(", StringComparison.Ordinal))
			{
				candidate = candidate.Substring(typeName.Length);
				hadTypePrefix = true;
			}
			else
			{
				string prefixedTypeName = "F" + typeName;
				if (candidate.StartsWith(prefixedTypeName + "(", StringComparison.Ordinal))
				{
					candidate = candidate.Substring(prefixedTypeName.Length);
					hadTypePrefix = true;
				}
			}

			if (candidate.StartsWith("(", StringComparison.Ordinal) && candidate.EndsWith(")", StringComparison.Ordinal))
			{
				candidate = candidate.Substring(1, candidate.Length - 2);
			}
			else if (hadTypePrefix)
			{
				return null;
			}

			if (candidate.Length == 0)
			{
				return null;
			}

			Dictionary<string, string> components = new(StringComparer.OrdinalIgnoreCase);
			string[] parts = candidate.Split(new[] { ',' }, StringSplitOptions.RemoveEmptyEntries);
			foreach (string part in parts)
			{
				string trimmedPart = part.Trim();
				int separatorIndex = trimmedPart.IndexOf('=');
				if (separatorIndex > 0 && separatorIndex < trimmedPart.Length - 1)
				{
					string key = trimmedPart.Substring(0, separatorIndex).Trim();
					string value = trimmedPart.Substring(separatorIndex + 1).Trim();
					if (key.Length == 0 || value.Length == 0)
					{
						return null;
					}

					components[key] = value;
					continue;
				}

				// If we reach here, this entry is not a named component; assume raw list
				components.Clear();
				List<string> positionalValues = new();
				foreach (string valuePart in parts)
				{
					string valueToken = NormalizeNumericToken(valuePart.Trim());
					if (valueToken.Length == 0)
					{
						return null;
					}
					positionalValues.Add(valueToken);
				}

				if (positionalValues.Count != componentOrder.Count)
				{
					return null;
				}

				return $"{typeName}({String.Join(", ", positionalValues)})";
			}

			List<string> orderedValues = new();
			foreach (string key in componentOrder)
			{
				if (!components.TryGetValue(key, out string? value))
				{
					return null;
				}

				orderedValues.Add(NormalizeNumericToken(value));
			}

			return $"{typeName}({String.Join(", ", orderedValues)})";
		}

		private static string NormalizeNumericToken(string token)
		{
			string normalized = token;
			if (normalized.EndsWith("f", StringComparison.OrdinalIgnoreCase) && normalized.Length > 1)
			{
				bool isNumeric = Double.TryParse(normalized[..^1], NumberStyles.Float, CultureInfo.InvariantCulture, out _);
				if (isNumeric)
				{
					normalized = normalized[..^1];
				}
			}

			return normalized;
		}

		private static string? FindEnumEntryName(EnumJsonInfo enumInfo, string candidateValue)
		{
			foreach (EnumEntryJsonInfo entry in enumInfo.EnumEntries)
			{
				if (entry.EntryName.Equals(candidateValue, StringComparison.Ordinal))
				{
					return entry.EntryName;
				}
			}

			foreach (EnumEntryJsonInfo entry in enumInfo.EnumEntries)
			{
				if (entry.EntryName.Equals(candidateValue, StringComparison.OrdinalIgnoreCase))
				{
					return entry.EntryName;
				}
			}

			if (Int64.TryParse(candidateValue, NumberStyles.Integer, CultureInfo.InvariantCulture, out long numericValue))
			{
				foreach (EnumEntryJsonInfo entry in enumInfo.EnumEntries)
				{
					if (entry.EntryValue == numericValue)
					{
						return entry.EntryName;
					}
				}
			}

			return null;
		}

		private static bool IsNullDefaultValue(PropJsonInfo prop)
		{
			if (String.IsNullOrWhiteSpace(prop.DefaultValue))
			{
				return false;
			}

			string raw = prop.DefaultValue!.Trim();
			if (raw.Length == 0)
			{
				return false;
			}

			return String.Equals(raw, "nullptr", StringComparison.OrdinalIgnoreCase) ||
				String.Equals(raw, "null", StringComparison.OrdinalIgnoreCase) ||
				String.Equals(raw, "None", StringComparison.OrdinalIgnoreCase);
		}

		private static bool HasDefaultValue(PropJsonInfo prop)
		{
			return !String.IsNullOrWhiteSpace(prop.DefaultValue);
		}

		private static bool IsSimpleIdentifier(string value)
		{
			if (String.IsNullOrEmpty(value))
			{
				return false;
			}

			foreach (char ch in value)
			{
				if (!Char.IsLetterOrDigit(ch) && ch != '_')
				{
					return false;
				}
			}
			return true;
		}

		private static string MapConstantTypeName(string typeName)
		{
			if (String.IsNullOrEmpty(typeName))
			{
				return typeName;
			}

			if (typeName.StartsWith("F", StringComparison.Ordinal) && typeName.Length > 1)
			{
				return typeName.Substring(1);
			}

			return typeName;
		}


		private string NormalizeSimpleType(string typeName)
		{
			if (typeName.StartsWith("ByteProperty<", StringComparison.Ordinal) && typeName.EndsWith(">", StringComparison.Ordinal))
			{
				typeName = typeName.Substring(13, typeName.Length - 14);
			}

			if (BuiltinTypeMap.TryGetValue(typeName, out string? builtin))
			{
				return builtin;
			}

			return typeName;
		}

		private static string EscapePythonString(string value)
		{
			return value.Replace("\\", "\\\\").Replace("'", "\\'");
		}

		private void WriteConstantAttribute(MethodJsonInfo method)
		{
			if (_includeComments && !String.IsNullOrWhiteSpace(method.MethodDoc))
			{
				foreach (string line in method.MethodDoc.Split(new[] { "\r\n", "\n" }, StringSplitOptions.None))
				{
					if (line.Trim().Length == 0)
					{
						continue;
					}
					WriteIndent(1);
					WriteLine($"# {line.Trim()}");
				}
			}

			string typeAnnotation = GetMethodReturnType(method);
			if (typeAnnotation == "None")
			{
				typeAnnotation = "Any";
			}

			string name = SanitizeIdentifier(method.MethodName);
			WriteIndent(1);
			WriteLine($"{name}: {typeAnnotation}");
			WriteLine();
		}

		private static string SanitizeIdentifier(string name)
		{
			if (String.IsNullOrEmpty(name))
			{
				return "unnamed";
			}

			string sanitized = name.Replace(' ', '_');
			if (PythonKeywords.Contains(sanitized))
			{
				sanitized += "_";
			}

			return sanitized;
		}

		private static string NormalizeInlineText(string text)
		{
			return text.Replace('\r', ' ').Replace('\n', ' ').Trim();
		}

		private void WriteDocstring(string doc, int indentLevel)
		{
			string sanitized = doc.Replace("\"\"\"", "\"\\\"\\\"\"");
			if (sanitized.IndexOfAny(new[] { '\r', '\n' }) >= 0)
			{
				WriteIndent(indentLevel);
				WriteLine("\"\"\"");
				foreach (string line in sanitized.Split(new[] { "\r\n", "\n" }, StringSplitOptions.None))
				{
					WriteIndent(indentLevel);
					WriteLine(line.TrimEnd());
				}
				WriteIndent(indentLevel);
				WriteLine("\"\"\"");
			}
			else
			{
				WriteIndent(indentLevel);
				WriteLine($"\"\"\" {sanitized} \"\"\"");
			}
		}

		private void WriteLine(string text = "")
		{
			_builder.AppendLine(text);
		}

		private void WriteIndent(int indentLevel)
		{
			for (int index = 0; index < indentLevel; ++index)
			{
				_builder.Append(IndentUnit);
			}
		}
	}
}
