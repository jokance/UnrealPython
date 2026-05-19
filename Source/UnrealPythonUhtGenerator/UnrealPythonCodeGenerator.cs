using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.Json;
using EpicGames.Core;
using EpicGames.UHT.Types;
using EpicGames.UHT.Utils;

namespace UnrealPythonUhtGenerator;

internal sealed class UnrealPythonCodeGenerator
{
	private const string ScriptNameMeta = "ScriptName";
	private const string ScriptNoExportMeta = "ScriptNoExport";
	private const string ScriptMethodMeta = "UPyScriptMethod";
	private const string ScriptMethodSelfReturnMeta = "UPyScriptMethodSelfReturn";
	private const string ScriptMethodMutableMeta = "UPyScriptMethodMutable";
	private const string ScriptOperatorMeta = "UPyScriptOperator";
	private const string ScriptConstantMeta = "UPyScriptConstant";
	private const string ScriptHostMeta = "UPyScriptHost";
	private const string ScriptDefaultMakeMeta = "ScriptDefaultMake";
	private const string ScriptDefaultBreakMeta = "ScriptDefaultBreak";
	private const string ScriptStaticMeta = "UPyScriptStatic";
	private const string ScriptUseHelperMethodMeta = "UPyUseHelperMethod";
	private const string BlueprintInternalOnlyMeta = "BlueprintInternalUseOnly";
	private const string BlueprintInternalOnlyHierarchicalMeta = "BlueprintInternalUseOnlyHierarchical";
	private const string BlueprintTypeMeta = "BlueprintType";
	private const string NotBlueprintTypeMeta = "NotBlueprintType";
	private const string BlueprintGetterMeta = "BlueprintGetter";
	private const string BlueprintSetterMeta = "BlueprintSetter";
	private const string CustomThunkMeta = "CustomThunk";
	private const string NativeBreakFuncMeta = "NativeBreakFunc";
	private const string NativeMakeFuncMeta = "NativeMakeFunc";
	private const string DeprecatedFunctionMeta = "DeprecatedFunction";
	private const string DeprecatedPropertyMeta = "DeprecatedProperty";

	private readonly IUhtExportFactory _factory;

	private readonly Dictionary<UhtClass, ClassJsonInfo> _classMap = new();
	private readonly Dictionary<UhtScriptStruct, StructJsonInfo> _structMap = new();
	private readonly Dictionary<UhtEnum, EnumJsonInfo> _enumMap = new();

	private enum OperatorValueType
	{
		None,
		Any,
		Struct,
		Bool
	}

	private sealed class OperatorSignature
	{
		public OperatorSignature(string token, string pythonFunctionName, OperatorValueType returnType, OperatorValueType otherType)
		{
			Token = token;
			PythonFunctionName = pythonFunctionName;
			ReturnType = returnType;
			OtherType = otherType;
		}

		public string Token { get; }
		public string PythonFunctionName { get; }
		public OperatorValueType ReturnType { get; }
		public OperatorValueType OtherType { get; }
		public int ExpectedInputCount => OtherType == OperatorValueType.None ? 1 : 2;
		public int ExpectedOutputCount => ReturnType == OperatorValueType.None ? 0 : 1;
	}

	private enum PythonNameCase
	{
		Preserve,
		Lower,
		Upper
	}

	private static readonly Dictionary<string, OperatorSignature> OperatorSignatures = new(StringComparer.Ordinal)
	{
		{ "bool", new OperatorSignature("bool", "__bool__", OperatorValueType.Bool, OperatorValueType.None) },
		{ "==", new OperatorSignature("==", "__eq__", OperatorValueType.Bool, OperatorValueType.Any) },
		{ "!=", new OperatorSignature("!=", "__ne__", OperatorValueType.Bool, OperatorValueType.Any) },
		{ "<", new OperatorSignature("<", "__lt__", OperatorValueType.Bool, OperatorValueType.Any) },
		{ "<=", new OperatorSignature("<=", "__le__", OperatorValueType.Bool, OperatorValueType.Any) },
		{ ">", new OperatorSignature(">", "__gt__", OperatorValueType.Bool, OperatorValueType.Any) },
		{ ">=", new OperatorSignature(">=", "__ge__", OperatorValueType.Bool, OperatorValueType.Any) },
		{ "+", new OperatorSignature("+", "__add__", OperatorValueType.Any, OperatorValueType.Any) },
		{ "+=", new OperatorSignature("+=", "__iadd__", OperatorValueType.Struct, OperatorValueType.Any) },
		{ "-", new OperatorSignature("-", "__sub__", OperatorValueType.Any, OperatorValueType.Any) },
		{ "-=", new OperatorSignature("-=", "__isub__", OperatorValueType.Struct, OperatorValueType.Any) },
		{ "*", new OperatorSignature("*", "__mul__", OperatorValueType.Any, OperatorValueType.Any) },
		{ "*=", new OperatorSignature("*=", "__imul__", OperatorValueType.Struct, OperatorValueType.Any) },
		{ "/", new OperatorSignature("/", "__truediv__", OperatorValueType.Any, OperatorValueType.Any) },
		{ "/=", new OperatorSignature("/=", "__truediv__", OperatorValueType.Struct, OperatorValueType.Any) },
		{ "%", new OperatorSignature("%", "__mod__", OperatorValueType.Any, OperatorValueType.Any) },
		{ "%=", new OperatorSignature("%=", "__imod__", OperatorValueType.Struct, OperatorValueType.Any) },
		{ "&", new OperatorSignature("&", "__and__", OperatorValueType.Any, OperatorValueType.Any) },
		{ "&=", new OperatorSignature("&=", "__iand__", OperatorValueType.Struct, OperatorValueType.Any) },
		{ "|", new OperatorSignature("|", "__or__", OperatorValueType.Any, OperatorValueType.Any) },
		{ "|=", new OperatorSignature("|=", "__ior__", OperatorValueType.Struct, OperatorValueType.Any) },
		{ "^", new OperatorSignature("^", "__xor__", OperatorValueType.Any, OperatorValueType.Any) },
		{ "^=", new OperatorSignature("^=", "__ixor__", OperatorValueType.Struct, OperatorValueType.Any) },
		{ ">>", new OperatorSignature(">>", "__rshift__", OperatorValueType.Any, OperatorValueType.Any) },
		{ ">>=", new OperatorSignature(">>=", "__irshift__", OperatorValueType.Struct, OperatorValueType.Any) },
		{ "<<", new OperatorSignature("<<", "__lshift__", OperatorValueType.Any, OperatorValueType.Any) },
		{ "<<=", new OperatorSignature("<<=", "__ilshift__", OperatorValueType.Struct, OperatorValueType.Any) },
		{ "neg", new OperatorSignature("neg", "__neg__", OperatorValueType.Struct, OperatorValueType.None) },
	};

	public UnrealPythonCodeGenerator(IUhtExportFactory factory)
	{
		_factory = factory;
	}

	public void Generate()
	{
		foreach (UhtModule module in _factory.Session.Modules)
		{
			if (!ShouldProcessModule(module))
			{
				continue;
			}

			ProcessType(module.ScriptPackage);
		}

		RootJson root = new();

		foreach (ClassJsonInfo classInfo in _classMap.Values.OrderBy(info => info.InheritanceDepth).ThenBy(info => info.ClassName, StringComparer.Ordinal))
		{
			root.Classes.Add(classInfo);
		}

		foreach (StructJsonInfo structInfo in _structMap.Values.OrderBy(info => info.InheritanceDepth).ThenBy(info => info.StructName, StringComparer.Ordinal))
		{
			root.Structs.Add(structInfo);
		}

		foreach (EnumJsonInfo enumInfo in _enumMap.Values.OrderBy(info => info.EnumName, StringComparer.Ordinal))
		{
			root.Enums.Add(enumInfo);
		}

		LoadNativeModules(root);

		JsonSerializerOptions options = new()
		{
			WriteIndented = true
		};

		string outputPath = ResolveOutputPath();
		Directory.CreateDirectory(Path.GetDirectoryName(outputPath)!);
		string payload = JsonSerializer.Serialize(root, options);
		_factory.CommitOutput(outputPath, payload);

		GenerationConfig config = AutoWrapperGenerator.LoadGenerationConfig(_factory);

		string stubOutputPath;
		if (_factory.PluginModule != null && !String.IsNullOrEmpty(_factory.PluginModule.BaseDirectory))
		{
			string pluginRoot = ResolvePluginRoot(_factory.PluginModule.BaseDirectory);
			if (!String.IsNullOrEmpty(config.StubFilePath))
			{
				stubOutputPath = Path.Combine(pluginRoot, config.StubFilePath);
			}
			else
			{
				stubOutputPath = Path.Combine(pluginRoot, "Tools", "pystubs", "ue", "__init__.pyi");
			}
		}
		else
		{
			stubOutputPath = Path.ChangeExtension(outputPath, ".pyi");
		}

		Directory.CreateDirectory(Path.GetDirectoryName(stubOutputPath)!);

		int includeComments = 0;
		string? envIncludeComments = Environment.GetEnvironmentVariable("UNREAL_PYTHON_STUB_COMMENTS");
		if (!String.IsNullOrEmpty(envIncludeComments) && Int32.TryParse(envIncludeComments, out int parsedIncludeComments))
		{
			includeComments = parsedIncludeComments;
		}

		foreach ((string stubPayload, string suffix) in PythonStubGenerator.GenerateStub(root, config, includeComments))
		{
			string currentStubOutputPath = stubOutputPath;
			if (!String.IsNullOrEmpty(suffix))
			{
				string directory = Path.GetDirectoryName(stubOutputPath)!;
				string fileName = Path.GetFileNameWithoutExtension(stubOutputPath);
				string extension = Path.GetExtension(stubOutputPath);
				currentStubOutputPath = Path.Combine(directory, $"{fileName}{suffix}{extension}");
			}
			_factory.CommitOutput(currentStubOutputPath, stubPayload);
		}

		AutoWrapperGenerator.Generate(_factory, root);
	}

	private void LoadNativeModules(RootJson root)
	{
		root.NativeModules.Clear();

		string? pluginBaseDirectory = _factory.PluginModule?.BaseDirectory;
		if (String.IsNullOrEmpty(pluginBaseDirectory))
		{
			return;
		}

		string pluginRoot = ResolvePluginRoot(pluginBaseDirectory);
		string nativeModulePath = Path.Combine(pluginRoot, "Tools", "ReflectionData", "native_module.json");
		if (!File.Exists(nativeModulePath))
		{
			return;
		}

		string jsonText = File.ReadAllText(nativeModulePath);
		if (String.IsNullOrWhiteSpace(jsonText))
		{
			return;
		}

		using JsonDocument document = JsonDocument.Parse(jsonText);
		foreach (JsonProperty moduleProperty in document.RootElement.EnumerateObject())
		{
			string moduleName = moduleProperty.Name;
			if (String.IsNullOrWhiteSpace(moduleName))
			{
				continue;
			}

			NativeModuleJsonInfo moduleInfo = new()
			{
				ModuleName = moduleName
			};

			if (moduleProperty.Value.TryGetProperty("NativeMethods", out JsonElement nativeMethodsElement) && nativeMethodsElement.ValueKind == JsonValueKind.Array)
			{
				foreach (JsonElement methodElement in nativeMethodsElement.EnumerateArray())
				{
					string methodName = methodElement.TryGetProperty("MethodName", out JsonElement methodNameElement) ? methodNameElement.GetString() ?? String.Empty : String.Empty;
					if (String.IsNullOrWhiteSpace(methodName))
					{
						continue;
					}

					string methodDoc = methodElement.TryGetProperty("MethodDoc", out JsonElement methodDocElement) ? methodDocElement.GetString() ?? String.Empty : String.Empty;
					long methodFlags = methodElement.TryGetProperty("PyMethodFlags", out JsonElement methodFlagsElement) && methodFlagsElement.TryGetInt64(out long flagsValue) ? flagsValue : 0;

					moduleInfo.NativeMethods.Add(new NativeModuleMethodJsonInfo
					{
						MethodName = methodName,
						MethodDoc = methodDoc,
						PyMethodFlags = methodFlags
					});
				}
			}

			root.NativeModules.Add(moduleInfo);
		}
	}

	private void ProcessType(UhtType type)
	{
		switch (type)
		{
			case UhtClass classObj:
				ProcessClass(classObj);
				break;
			case UhtScriptStruct structObj:
				ProcessStruct(structObj);
				break;
			case UhtEnum enumObj:
				ProcessEnum(enumObj);
				break;
		}

		foreach (UhtType child in type.Children)
		{
			ProcessType(child);
		}
	}

	private ClassJsonInfo? ProcessClass(UhtClass classObj, bool forceExport = false)
	{
		if (_classMap.TryGetValue(classObj, out ClassJsonInfo? existing))
		{
			return existing;
		}

		if (!forceExport && !ShouldExportClass(classObj))
		{
			return null;
		}

		ClassJsonInfo info = new();
		_classMap[classObj] = info;

		if (classObj.SuperClass is UhtClass superClass)
		{
			ClassJsonInfo? baseInfo = ProcessClass(superClass, forceExport);
			if (baseInfo != null)
			{
				info.BaseType = baseInfo.ClassName;
				info.InheritanceDepth = baseInfo.InheritanceDepth + 1;
			}
		}

		PopulateStructLikeInfo(classObj, info.ClassProps, info.ClassMethods);

		foreach (UhtStruct implementedInterface in classObj.Bases)
		{
			if (implementedInterface is UhtClass interfaceClass && interfaceClass.ClassType == UhtClassType.NativeInterface)
			{
				InterfaceJsonInfo interfaceInfo = CreateInterfaceJson(interfaceClass);
				info.ImplementedInterfaces.Add(interfaceInfo);
			}
		}

		info.ClassName = GetTypePythonName(classObj);
		info.ClassDoc = GetTooltipDoc(classObj);
		info.EngineName = classObj.EngineName;
		info.NativeTypeName = GetTypeNativeName(classObj);
		info.PackageName = GetTypePackageName(classObj);
		info.ModuleName = GetTypeModuleName(classObj);
		info.HeaderFile = GetTypeHeaderFile(classObj);
		info.ClassFlags = (uint)classObj.ClassFlags;

		return info;
	}

	private StructJsonInfo? ProcessStruct(UhtScriptStruct structObj, bool forceExport = false)
	{
		if (_structMap.TryGetValue(structObj, out StructJsonInfo? existing))
		{
			return existing;
		}

		if (!forceExport && !ShouldExportStruct(structObj))
		{
			return null;
		}

		StructJsonInfo info = new();
		_structMap[structObj] = info;

		if (structObj.Super is UhtScriptStruct superStruct)
		{
			StructJsonInfo? baseInfo = ProcessStruct(superStruct, forceExport);
			if (baseInfo != null)
			{
				info.BaseType = baseInfo.StructName;
				info.InheritanceDepth = baseInfo.InheritanceDepth + 1;
			}
		}

		PopulateStructLikeInfo(structObj, info.StructProps, info.StructMethods);

		info.StructName = GetTypePythonName(structObj);
		info.StructDoc = GetTooltipDoc(structObj);
		info.EngineName = structObj.EngineName;
		info.NativeTypeName = GetTypeNativeName(structObj);
		info.PackageName = GetTypePackageName(structObj);
		info.ModuleName = GetTypeModuleName(structObj);
		info.HeaderFile = GetTypeHeaderFile(structObj);
		info.StructFlags = (uint)structObj.ScriptStructFlags;

		PopulateStructMakeBreak(structObj, info);
		return info;
	}

	private EnumJsonInfo? ProcessEnum(UhtEnum enumObj)
	{
		if (_enumMap.TryGetValue(enumObj, out EnumJsonInfo? existing))
		{
			return existing;
		}

		if (!ShouldExportEnum(enumObj))
		{
			return null;
		}

		EnumJsonInfo info = new()
		{
			EnumName = GetTypePythonName(enumObj),
			NativeTypeName = enumObj.CppType,
			HeaderFile = GetTypeHeaderFile(enumObj),
			EnumDoc = GetTooltipDoc(enumObj)
		};

		for (int index = 0; index < enumObj.EnumValues.Count; ++index)
		{
			if (!ShouldExportEnumEntry(enumObj, index))
			{
				continue;
			}

			UhtEnumValue value = enumObj.EnumValues[index];
			EnumEntryJsonInfo entry = new()
			{
				EntryName = GetEnumEntryPythonName(enumObj, value.Name),
				EntryDoc = GetTooltipDoc(enumObj, index),
				EntryValue = value.Value
			};
			info.EnumEntries.Add(entry);
		}

		_enumMap[enumObj] = info;
		return info;
	}

	private void PopulateStructLikeInfo(UhtStruct structObj, List<PropJsonInfo> propSink, List<MethodJsonInfo> methodSink)
	{
		foreach (UhtProperty property in structObj.Properties)
		{
			if (!ShouldExportProperty(property))
			{
				continue;
			}

			propSink.Add(CreatePropertyJson(property));
		}

		foreach (UhtFunction function in structObj.Functions)
		{
			MethodJsonInfo? method = CreateMethodJson(function);
			if (method != null)
			{
				methodSink.Add(method);
			}
		}
	}

	private void PopulateStructMakeBreak(UhtScriptStruct structObj, StructJsonInfo info)
	{
		if (info.MakeFunc.MethodName.Length == 0)
		{
			string scriptDefaultMake = structObj.MetaData.GetValueOrDefault(ScriptDefaultMakeMeta);
			if (!String.IsNullOrWhiteSpace(scriptDefaultMake))
			{
				UhtFunction? makeFunc = structObj.Functions.FirstOrDefault(func => func.EngineName.Equals(scriptDefaultMake, StringComparison.Ordinal));
				if (makeFunc != null)
				{
					MethodJsonInfo? makeInfo = CreateMethodJson(makeFunc);
					if (makeInfo != null)
					{
						CopyMethod(makeInfo, info.MakeFunc);
					}
				}
			}
		}

		if (info.BreakFunc.MethodName.Length == 0)
		{
			string scriptDefaultBreak = structObj.MetaData.GetValueOrDefault(ScriptDefaultBreakMeta);
			if (!String.IsNullOrWhiteSpace(scriptDefaultBreak))
			{
				UhtFunction? breakFunc = structObj.Functions.FirstOrDefault(func => func.EngineName.Equals(scriptDefaultBreak, StringComparison.Ordinal));
				if (breakFunc != null)
				{
					MethodJsonInfo? breakInfo = CreateMethodJson(breakFunc);
					if (breakInfo != null)
					{
						CopyMethod(breakInfo, info.BreakFunc);
					}
				}
			}
		}
	}

	private static void CopyMethod(MethodJsonInfo source, MethodJsonInfo destination)
	{
		destination.MethodName = source.MethodName;
		destination.RawMethodName = source.RawMethodName;
		destination.MethodDoc = source.MethodDoc;
		destination.bIsConstant = source.bIsConstant;
		destination.bIsOperator = source.bIsOperator;
		destination.bIsScriptMethod = source.bIsScriptMethod;
		destination.MethodFlags = source.MethodFlags;
		destination.Params.Clear();
		foreach (PropJsonInfo param in source.Params)
		{
			destination.Params.Add(new PropJsonInfo
			{
				PropName = param.PropName,
				RawPropName = param.RawPropName,
				PropDoc = param.PropDoc,
				PropType = param.PropType,
				TypeName = param.TypeName,
				PropFlags = param.PropFlags,
				DefaultValue = param.DefaultValue
			});
		}
	}

	private MethodJsonInfo? CreateMethodJson(UhtFunction function)
	{
		if (!ShouldExportFunction(function))
		{
			return null;
		}

		bool hasScriptConstantMeta = function.MetaData.ContainsKey(ScriptConstantMeta);
		bool hasScriptMethodMeta = function.MetaData.ContainsKey(ScriptMethodMeta);
		bool hasScriptOperatorMeta = function.MetaData.ContainsKey(ScriptOperatorMeta);
		bool hasScriptUseHelperMethodMeta = function.MetaData.ContainsKey(ScriptUseHelperMethodMeta);

		string methodName = GetFunctionPythonName(function);
		if (hasScriptConstantMeta)
		{
			methodName = GetScriptConstantPythonName(function);
		}
		else if (hasScriptMethodMeta)
		{
			methodName = GetScriptMethodPythonName(function);
		}

		MethodJsonInfo info = new()
		{
			MethodName = methodName,
			RawMethodName = function.EngineName,
			MethodFlags = (uint)function.FunctionFlags,
			bIsConstant = hasScriptConstantMeta,
			bIsOperator = false,
			bIsScriptMethod = hasScriptMethodMeta,
			bUseHelperMethod = hasScriptUseHelperMethodMeta
		};

		List<MethodParam> methodParams = new();

		foreach (UhtType child in function.Children)
		{
			if (child is UhtProperty property)
			{
				PropJsonInfo paramJson = CreateParameterJson(function, property);
				info.Params.Add(paramJson);

				methodParams.Add(new MethodParam(
					property,
					paramJson,
					IsInputParameter(property),
					IsOutputParameter(property),
					IsReturnParameter(property)));
			}
		}

		// Post-process parameters to handle implicit optional arguments
		bool previousWasOptional = false;
		foreach (var param in info.Params)
		{
			// Skip return parameters and output-only parameters (unless they are also input references)
			// Note: We don't have easy access to IsInputParameter/IsOutputParameter here without re-checking flags
			// But CreateParameterJson sets up the PropJsonInfo.
			// We need to check if this is an input parameter.
			// For simplicity, we'll assume all params in the list are relevant, but we should ideally filter.
			// However, the logic requested is based on the order in the Params list.
			
			// Check if it's an input parameter (not a return value)
			// We can check PropFlags for ReturnParm (0x0000000000000400) or OutParm (0x0000000000000100)
			// But let's rely on the fact that CreateMethodJson adds them in order.
			
			bool isReturn = (param.PropFlags & (ulong)EPropertyFlags.ReturnParm) != 0;
			bool isOut = (param.PropFlags & (ulong)EPropertyFlags.OutParm) != 0;
			bool isConst = (param.PropFlags & (ulong)EPropertyFlags.ConstParm) != 0;
			bool isReference = (param.PropFlags & (ulong)EPropertyFlags.ReferenceParm) != 0;
			
			bool isInput = !isReturn && (!isOut || isReference);

			if (!isInput)
			{
				continue;
			}

			bool isOptional = !String.IsNullOrEmpty(param.DefaultValue) || previousWasOptional;
			if (isOptional && String.IsNullOrEmpty(param.DefaultValue))
			{
				if (param.PropType == "BoolProperty")
				{
					param.DefaultValue = "false";
				}
				else if (param.PropType == "IntProperty" || param.PropType == "Int32Property")
				{
					param.DefaultValue = "0";
				}
				else if (param.PropType == "FloatProperty" || param.PropType == "DoubleProperty")
				{
					param.DefaultValue = "0.0f";
				}
				else if (param.PropType == "StrProperty")
				{
					param.DefaultValue = "";
				}
				else if (param.PropType == "NameProperty")
				{
					param.DefaultValue = "None";
				}
				else if (param.PropType == "TextProperty")
				{
					param.DefaultValue = "";
				}
			}
			
			previousWasOptional = isOptional;
		}

		bool keepMethod = true;

		if (hasScriptConstantMeta)
		{
			keepMethod = HandleScriptConstant(function, info, methodParams);
		}

		if (!keepMethod)
		{
			return null;
		}

		if (hasScriptMethodMeta)
		{
			HandleScriptMethod(function, methodParams);
		}

		if (hasScriptOperatorMeta)
		{
			HandleScriptOperator(function, methodParams);
		}

		info.MethodDoc = BuildMethodDocString(function, info.MethodName, methodParams);
		return info;
	}

	private PropJsonInfo CreatePropertyJson(UhtProperty property)
	{
		PropJsonInfo info = new()
		{
			PropName = GetPropertyPythonName(property),
			RawPropName = property.EngineName,
			PropDoc = GetTooltipDoc(property),
			PropType = GetPropertyPythonType(property),
			TypeName = GetPropertyTypeName(property),
			PropFlags = (ulong)property.PropertyFlags
		};

		return info;
	}

	private PropJsonInfo CreateParameterJson(UhtFunction function, UhtProperty property)
	{
		PropJsonInfo info = CreatePropertyJson(property);
		string defaultKey = $"CPP_Default_{property.EngineName}";
		if (function.MetaData.TryGetValue(defaultKey, out string? defaultValue) && !String.IsNullOrWhiteSpace(defaultValue))
		{
			info.DefaultValue = defaultValue.Trim();
		}
		return info;
	}

	private InterfaceJsonInfo CreateInterfaceJson(UhtClass interfaceClass)
	{
		InterfaceJsonInfo info = new()
		{
			InterfaceName = GetTypePythonName(interfaceClass),
			InterfaceRawName = interfaceClass.EngineName,
			InterfaceDoc = GetTooltipDoc(interfaceClass)
		};
		return info;
	}

	private static bool IsInputParameter(UhtProperty property)
	{
		bool isParam = property.PropertyFlags.HasAnyFlags(EPropertyFlags.Parm);
		bool isReturn = property.PropertyFlags.HasAnyFlags(EPropertyFlags.ReturnParm);
		bool isReference = property.PropertyFlags.HasAnyFlags(EPropertyFlags.ReferenceParm);
		bool isOut = property.PropertyFlags.HasAnyFlags(EPropertyFlags.OutParm) && !property.PropertyFlags.HasAnyFlags(EPropertyFlags.ConstParm);
		return isParam && !isReturn && (!isOut || isReference);
	}

	private static bool IsOutputParameter(UhtProperty property)
	{
		bool isParam = property.PropertyFlags.HasAnyFlags(EPropertyFlags.Parm);
		bool isReturn = property.PropertyFlags.HasAnyFlags(EPropertyFlags.ReturnParm);
		bool isOut = property.PropertyFlags.HasAnyFlags(EPropertyFlags.OutParm) && !property.PropertyFlags.HasAnyFlags(EPropertyFlags.ConstParm);
		return isParam && !isReturn && isOut;
	}

	private static bool IsReturnParameter(UhtProperty property)
	{
		return property.PropertyFlags.HasAnyFlags(EPropertyFlags.ReturnParm);
	}

	private bool HandleScriptConstant(UhtFunction function, MethodJsonInfo methodInfo, List<MethodParam> methodParams)
	{
		if (!function.FunctionFlags.HasAnyFlags(EFunctionFlags.Static))
		{
			function.LogError($"Function '{function.EngineName}' is marked as 'ScriptConstant' but is not static.");
			return false;
		}

		if (methodParams.Any(param => param.IsInput))
		{
			// Allow one input parameter if it's a struct or object (used as host)
			if (methodParams.Count(param => param.IsInput) > 1)
			{
				function.LogError($"Function '{function.EngineName}' is marked as 'ScriptConstant' but has too many input parameters.");
				return false;
			}

			MethodParam inputParam = methodParams.First(param => param.IsInput);
			if (!(inputParam.Property is UhtStructProperty || inputParam.Property is UhtObjectPropertyBase))
			{
				function.LogError($"Function '{function.EngineName}' is marked as 'ScriptConstant' but the input parameter is not a valid host type.");
				return false;
			}
		}

		MethodParam? returnParam = methodParams.FirstOrDefault(param => param.IsReturn);
		if (returnParam == null)
		{
			function.LogError($"Function '{function.EngineName}' is marked as 'ScriptConstant' but has no return value.");
			return false;
		}

		string hostName = function.MetaData.GetValueOrDefault(ScriptHostMeta);
		if (String.IsNullOrWhiteSpace(hostName))
		{
			// Check if we have a host parameter
			MethodParam? hostParam = methodParams.FirstOrDefault(param => param.IsInput);
			if (hostParam != null)
			{
				if (hostParam.Property is UhtStructProperty structProperty && structProperty.ScriptStruct != null)
				{
					StructJsonInfo? hostInfo = EnsureStructInfo(structProperty.ScriptStruct);
					if (hostInfo == null)
					{
						function.LogError($"Function '{function.EngineName}' is marked as 'ScriptConstant' but the host struct '{structProperty.ScriptStruct.EngineName}' could not be exported.");
						return false;
					}
					
					// Remove the host parameter from the method info since it's just for context
					methodInfo.Params.RemoveAll(p => p.PropName == hostParam.Json.PropName);
					
					methodInfo.bIsConstant = true;
					hostInfo.StructMethods.Add(methodInfo);
					return false; // Don't keep the original method
				}
				else if (hostParam.Property is UhtObjectPropertyBase objectProperty)
				{
					UhtClass? paramHostClass = objectProperty.Class ?? objectProperty.ReferencedClass;
					if (paramHostClass != null)
					{
						ClassJsonInfo hostInfo = EnsureClassInfo(paramHostClass);
						
						// Remove the host parameter from the method info since it's just for context
						methodInfo.Params.RemoveAll(p => p.PropName == hostParam.Json.PropName);
						
						methodInfo.bIsConstant = true;
						hostInfo.ClassMethods.Add(methodInfo);
						return false; // Don't keep the original method
					}
				}
			}

			methodInfo.bIsConstant = true;
			return true;
		}

		UhtType? hostType = FindTypeByName(hostName);
		if (hostType is UhtClass hostClass)
		{
			if (function.Outer == hostClass)
			{
				methodInfo.bIsConstant = true;
				return true;
			}
			ClassJsonInfo hostInfo = EnsureClassInfo(hostClass);
			List<MethodParam> hostParams = CloneParams(methodParams);
			MethodJsonInfo hostMethod = CreateHostMethodInfo(function, methodInfo.MethodName, true, false, hostParams);
			hostInfo.ClassMethods.Add(hostMethod);
		}
		else if (hostType is UhtScriptStruct hostStruct)
		{
			StructJsonInfo? hostInfo = EnsureStructInfo(hostStruct);
			if (hostInfo == null)
			{
				function.LogError($"Function '{function.EngineName}' is marked as 'ScriptHost' but the host struct '{hostName}' could not be exported.");
				return false;
			}
			if (function.Outer == hostStruct)
			{
				methodInfo.bIsConstant = true;
				return true;
			}
			List<MethodParam> hostParams = CloneParams(methodParams);
			MethodJsonInfo hostMethod = CreateHostMethodInfo(function, methodInfo.MethodName, true, false, hostParams);
			hostInfo.StructMethods.Add(hostMethod);
		}
		else
		{
			function.LogError($"Function '{function.EngineName}' is marked as 'ScriptHost' but the host '{hostName}' could not be resolved.");
			return false;
		}

		return false;
	}

	private void HandleScriptMethod(UhtFunction function, List<MethodParam> methodParams)
	{
		string? hostName = function.MetaData.GetValueOrDefault(ScriptHostMeta);
		if (!String.IsNullOrWhiteSpace(hostName))
		{
			UhtType? hostType = FindTypeByName(hostName);
			if (hostType is UhtClass hostClass)
			{
				ClassJsonInfo hostInfo = EnsureClassInfo(hostClass);
				List<MethodParam> hostParams = CloneParams(methodParams);
				MethodJsonInfo hostMethod = CreateHostMethodInfo(function, GetScriptMethodPythonName(function), false, false, hostParams);
				ApplyScriptStaticBehavior(function, hostMethod);
				hostInfo.ClassMethods.Add(hostMethod);
				return;
			}
			else if (hostType is UhtScriptStruct hostStruct)
			{
				StructJsonInfo? hostInfo = EnsureStructInfo(hostStruct);
				if (hostInfo != null)
				{
					List<MethodParam> hostParams = CloneParams(methodParams);
					MethodJsonInfo hostMethod = CreateHostMethodInfo(function, GetScriptMethodPythonName(function), false, false, hostParams);
					ApplyScriptStaticBehavior(function, hostMethod);
					hostInfo.StructMethods.Add(hostMethod);
					return;
				}
			}
		}

		MethodParam? selfParam = methodParams.FirstOrDefault(param =>
			(param.IsInput || param.IsOutput) && (param.Property is UhtStructProperty || param.Property is UhtObjectPropertyBase));

		if (selfParam == null)
		{
			function.LogError($"Function '{function.EngineName}' is marked as 'ScriptMethod' but does not contain a valid struct or object as the first argument.");
			return;
		}

		if (selfParam.Property is UhtObjectPropertyBase objectProperty)
		{
			UhtClass? hostClass = objectProperty.Class ?? objectProperty.ReferencedClass;
			if (hostClass == null)
			{
				function.LogError($"Function '{function.EngineName}' is marked as 'ScriptMethod' but the object argument type could not be resolved.");
				return;
			}

			if (function.Outer is UhtClass ownerClass && IsChildOf(hostClass, ownerClass))
			{
				function.LogError($"Function '{function.EngineName}' is marked as 'ScriptMethod' but the object argument type '{hostClass.EngineName}' is a child of the static function class. This is not allowed.");
				return;
			}

			ClassJsonInfo hostInfo = EnsureClassInfo(hostClass);
			IEnumerable<MethodParam> paramSource = methodParams.Where(param => !ReferenceEquals(param, selfParam));
			List<MethodParam> hostParams = CloneParams(paramSource);
			MethodJsonInfo hostMethod = CreateHostMethodInfo(function, GetScriptMethodPythonName(function), false, false, hostParams);
			ApplyScriptStaticBehavior(function, hostMethod);
			hostInfo.ClassMethods.Add(hostMethod);
		}
		else if (selfParam.Property is UhtStructProperty structProperty)
		{
			if (structProperty.ScriptStruct == null)
			{
				function.LogError($"Function '{function.EngineName}' is marked as 'ScriptMethod' but the struct argument type could not be resolved.");
				return;
			}

			StructJsonInfo? hostInfo = EnsureStructInfo(structProperty.ScriptStruct);
			if (hostInfo == null)
			{
				function.LogError($"Function '{function.EngineName}' is marked as 'ScriptMethod' but the host struct '{structProperty.ScriptStruct.EngineName}' could not be exported.");
				return;
			}

			IEnumerable<MethodParam> paramSource = methodParams.Where(param => !ReferenceEquals(param, selfParam));
			List<MethodParam> hostParams = CloneParams(paramSource);
			MethodJsonInfo hostMethod = CreateHostMethodInfo(function, GetScriptMethodPythonName(function), false, false, hostParams);
			ApplyScriptStaticBehavior(function, hostMethod);
			hostInfo.StructMethods.Add(hostMethod);
		}
		else
		{
			function.LogError($"Function '{function.EngineName}' is marked as 'ScriptMethod' but the first argument is not a supported type.");
		}
	}

	private void HandleScriptOperator(UhtFunction function, List<MethodParam> methodParams)
	{
		string operatorsMeta = function.MetaData.GetValueOrDefault(ScriptOperatorMeta);
		if (String.IsNullOrWhiteSpace(operatorsMeta))
		{
			return;
		}

		if (!function.FunctionFlags.HasAnyFlags(EFunctionFlags.Static))
		{
			function.LogError($"Function '{function.EngineName}' is marked as 'ScriptOperator' but is not static.");
			return;
		}

		string[] operatorTokens = operatorsMeta.Split(';');
		if (operatorTokens.Length == 0)
		{
			return;
		}

		StructJsonInfo? hostStructInfo = null;
		UhtScriptStruct? selfStruct = null;

		string? hostName = function.MetaData.GetValueOrDefault(ScriptHostMeta);
		if (!String.IsNullOrWhiteSpace(hostName))
		{
			UhtType? hostType = FindTypeByName(hostName);
			if (hostType is UhtScriptStruct hostStruct)
			{
				hostStructInfo = EnsureStructInfo(hostStruct);
				selfStruct = hostStruct;
			}
		}

		List<MethodParam> inputParams = methodParams.Where(param => param.IsInput || param.IsOutput).ToList();
		if (hostStructInfo == null)
		{
			MethodParam? selfParam = inputParams.FirstOrDefault();
			if (selfParam == null || selfParam.Property is not UhtStructProperty selfStructProperty || selfStructProperty.ScriptStruct == null)
			{
				function.LogError($"Function '{function.EngineName}' is marked as 'ScriptOperator' but does not contain a valid struct as its first argument.");
				return;
			}

			hostStructInfo = EnsureStructInfo(selfStructProperty.ScriptStruct);
			selfStruct = selfStructProperty.ScriptStruct;
		}

		if (hostStructInfo == null || selfStruct == null)
		{
			function.LogError($"Function '{function.EngineName}' is marked as 'ScriptOperator' but the host struct could not be resolved.");
			return;
		}

		foreach (string operatorToken in operatorTokens)
		{
			string op = operatorToken.Trim();
			if (op.Length == 0)
			{
				continue;
			}

			if (!OperatorSignatures.TryGetValue(op, out OperatorSignature? signature) || signature == null)
			{
				function.LogError($"Function '{function.EngineName}' is marked as 'ScriptOperator' but uses an unknown operator type '{op}'.");
				continue;
			}

			OperatorSignature signatureValue = signature;

			int requiredInputs = signatureValue.ExpectedInputCount;
			int requiredOutputs = signatureValue.ExpectedOutputCount;

			int significantInputs = inputParams.Count(param => !param.HasDefault);
			if (significantInputs > requiredInputs || inputParams.Count < requiredInputs)
			{
				function.LogError($"Function '{function.EngineName}' is marked as 'ScriptOperator' but has an invalid number of input parameters for operator '{op}'.");
				continue;
			}

			List<MethodParam> outputParams = methodParams.Where(param => param.IsReturn || (param.IsOutput && !param.IsReturn)).ToList();
			if (outputParams.Count != requiredOutputs)
			{
				function.LogError($"Function '{function.EngineName}' is marked as 'ScriptOperator' but has an invalid number of output parameters for operator '{op}'.");
				continue;
			}

			MethodParam? otherParam = requiredInputs > 1 ? inputParams.Skip(1).FirstOrDefault() : null;
			if (!ValidateOperatorParam(signatureValue.OtherType, otherParam, selfStruct))
			{
				function.LogError($"Function '{function.EngineName}' is marked as 'ScriptOperator' but the 'other' parameter is not valid for operator '{op}'.");
				continue;
			}

			MethodParam? returnParam = requiredOutputs > 0 ? methodParams.FirstOrDefault(param => param.IsReturn) : null;
			if (!ValidateOperatorParam(signatureValue.ReturnType, returnParam, selfStruct))
			{
				function.LogError($"Function '{function.EngineName}' is marked as 'ScriptOperator' but the return value is not valid for operator '{op}'.");
				continue;
			}

			List<MethodParam> hostParams = new();
			if (returnParam != null)
			{
				hostParams.Add(CloneSingleParam(returnParam));
			}
			if (otherParam != null)
			{
				hostParams.Add(CloneSingleParam(otherParam));
			}

			MethodJsonInfo operatorMethod = CreateHostMethodInfo(function, signatureValue.PythonFunctionName, false, true, hostParams);
			ApplyScriptStaticBehavior(function, operatorMethod);
			hostStructInfo.StructMethods.Add(operatorMethod);
		}
	}

	private static List<MethodParam> CloneParams(IEnumerable<MethodParam> source)
	{
		List<MethodParam> result = new();
		foreach (MethodParam param in source)
		{
			result.Add(CloneSingleParam(param));
		}
		return result;
	}

	private static MethodParam CloneSingleParam(MethodParam source)
	{
		PropJsonInfo clone = CloneProp(source.Json);
		return new MethodParam(source.Property, clone, source.IsInput, source.IsOutput, source.IsReturn);
	}

	private MethodJsonInfo CreateHostMethodInfo(UhtFunction function, string methodName, bool isConstant, bool isOperator, List<MethodParam> parameters)
	{
		bool hasScriptUseHelperMethodMeta = function.MetaData.ContainsKey(ScriptUseHelperMethodMeta);
		bool hasScriptMethodMeta = function.MetaData.ContainsKey(ScriptMethodMeta);
		MethodJsonInfo method = new()
		{
			MethodName = methodName,
			RawMethodName = function.EngineName,
			MethodFlags = (uint)function.FunctionFlags,
			bIsConstant = isConstant,
			bIsOperator = isOperator,
			bIsScriptMethod = hasScriptMethodMeta,
			bUseHelperMethod = hasScriptUseHelperMethodMeta
		};

		string? hostHelper = function.Outer switch
		{
			UhtClass ownerClass => GetTypeNativeName(ownerClass),
			UhtScriptStruct ownerStruct => GetTypeNativeName(ownerStruct),
			_ => null
		};

		if (!String.IsNullOrWhiteSpace(hostHelper))
		{
			if (function.MetaData.ContainsKey(ScriptUseHelperMethodMeta))
			{
				method.HostHelper = hostHelper;
			}
			else if (!isOperator && !isConstant)
			{
				method.RawMethodName = methodName;
			}
			else
			{
				method.HostHelper = hostHelper;
			}
		}

		foreach (MethodParam param in parameters)
		{
			method.Params.Add(param.Json);
		}

		method.MethodDoc = BuildMethodDocString(function, methodName, parameters);
		return method;
	}

	private static PropJsonInfo CloneProp(PropJsonInfo source)
	{
		return new PropJsonInfo
		{
			PropName = source.PropName,
			RawPropName = source.RawPropName,
			PropDoc = source.PropDoc,
			PropType = source.PropType,
			TypeName = source.TypeName,
			PropFlags = source.PropFlags,
			DefaultValue = source.DefaultValue
		};
	}

	private ClassJsonInfo EnsureClassInfo(UhtClass classObj)
	{
		ClassJsonInfo? info = ProcessClass(classObj, true);
		if (info == null)
		{
			throw new InvalidOperationException($"Unable to export class '{classObj.EngineName}' for Python generation.");
		}
		return info;
	}

	private StructJsonInfo? EnsureStructInfo(UhtScriptStruct structObj)
	{
		return ProcessStruct(structObj, true);
	}

	private UhtType? FindTypeByName(string rawName)
	{
		if (String.IsNullOrWhiteSpace(rawName))
		{
			return null;
		}

		string trimmed = rawName.Trim();

		UhtType? type = _factory.Session.FindType(null, UhtFindOptions.EngineName | UhtFindOptions.Class | UhtFindOptions.ScriptStruct, trimmed);
		if (type != null)
		{
			return type;
		}

		string simpleName = trimmed;
		int lastDot = simpleName.LastIndexOf('.');
		if (lastDot >= 0)
		{
			simpleName = simpleName.Substring(lastDot + 1);
		}
		int lastSlash = simpleName.LastIndexOf('/');
		if (lastSlash >= 0)
		{
			simpleName = simpleName.Substring(lastSlash + 1);
		}

		type = _factory.Session.FindType(null, UhtFindOptions.EngineName | UhtFindOptions.Class | UhtFindOptions.ScriptStruct, simpleName);
		if (type != null)
		{
			return type;
		}

		type = _factory.Session.FindType(null, UhtFindOptions.EngineName | UhtFindOptions.Class | UhtFindOptions.ScriptStruct | UhtFindOptions.CaseCompare, simpleName);
		return type;
	}

	private static bool IsChildOf(UhtClass? child, UhtClass? parent)
	{
		if (child == null || parent == null)
		{
			return false;
		}

		for (UhtClass? current = child; current != null; current = current.SuperClass)
		{
			if (current == parent)
			{
				return true;
			}
		}

		return false;
	}

	private static bool ValidateOperatorParam(OperatorValueType expectedType, MethodParam? param, UhtScriptStruct selfStruct)
	{
		switch (expectedType)
		{
			case OperatorValueType.None:
				return param == null;

			case OperatorValueType.Any:
				return param != null;

			case OperatorValueType.Struct:
				return param?.Property is UhtStructProperty structProperty && structProperty.ScriptStruct == selfStruct;

			case OperatorValueType.Bool:
				return param?.Property is UhtBoolProperty;

			default:
				return false;
		}
	}

	private sealed class MethodParam
	{
		public MethodParam(UhtProperty property, PropJsonInfo json, bool isInput, bool isOutput, bool isReturn)
		{
			Property = property;
			Json = json;
			IsInput = isInput;
			IsOutput = isOutput;
			IsReturn = isReturn;
		}

		public UhtProperty Property { get; }
		public PropJsonInfo Json { get; }
		public bool IsInput { get; }
		public bool IsOutput { get; }
		public bool IsReturn { get; }
		public bool HasDefault => !String.IsNullOrEmpty(Json.DefaultValue);
	}

	private static bool ShouldProcessModule(UhtModule module)
	{
		if (module.Module == null)
		{
			return true;
		}

		return module.Module.ModuleType switch
		{
			UHTModuleType.EngineRuntime => true,
			UHTModuleType.EngineEditor => true,
			UHTModuleType.EngineDeveloper => true,
			UHTModuleType.EngineUncooked => true,
			UHTModuleType.GameRuntime => true,
			UHTModuleType.GameEditor => true,
			UHTModuleType.GameDeveloper => true,
			_ => false
		};
	}

	private static bool ShouldExportClass(UhtClass classObj)
	{
		if (classObj.ClassFlags.HasAnyFlags(EClassFlags.Deprecated))
		{
			return false;
		}

		return !classObj.MetaData.ContainsKey(ScriptNoExportMeta);

		// if (classObj.MetaData.ContainsKey(BlueprintInternalOnlyMeta))
		// {
		// 	return false;
		// }

		// if (classObj.MetaData.GetBooleanHierarchical(BlueprintTypeMeta))
		// {
		// 	return true;
		// }

		// for (UhtClass? current = classObj; current != null; current = current.SuperClass)
		// {
		// 	if (current.MetaData.GetBoolean(NotBlueprintTypeMeta))
		// 	{
		// 		return false;
		// 	}
		// }

		// return HasScriptExposedFields(classObj);
	}

	private static bool ShouldExportStruct(UhtScriptStruct structObj)
	{
		return !structObj.MetaData.ContainsKey(ScriptNoExportMeta);
	}

	private static bool ShouldExportEnum(UhtEnum enumObj)
	{
		return !enumObj.MetaData.ContainsKey(ScriptNoExportMeta);
	}

	private static bool ShouldExportEnumEntry(UhtEnum enumObj, int index)
	{
		return true;
	}

	private static bool ShouldExportProperty(UhtProperty property)
	{
		if (property.Deprecated)
		{
			return false;
		}

		if (property.MetaData.ContainsKey(ScriptNoExportMeta))
		{
			return false;
		}

		if (property.MetaData.ContainsKey(DeprecatedPropertyMeta))
		{
			return false;
		}

		// return property.PropertyFlags.HasAnyFlags(EPropertyFlags.BlueprintVisible | EPropertyFlags.BlueprintAssignable | EPropertyFlags.BlueprintReadOnly);
		return true;
	}

	private static bool ShouldExportFunction(UhtFunction function)
	{
		if (function.Deprecated)
		{
			return false;
		}

		if (function.MetaData.ContainsKey(ScriptNoExportMeta))
		{
			return false;
		}

		if (function.MetaData.ContainsKey(DeprecatedFunctionMeta))
		{
			return false;
		}

		// if (!function.FunctionFlags.HasAnyFlags(EFunctionFlags.BlueprintCallable | EFunctionFlags.BlueprintEvent))
		// {
		// 	return false;
		// }

		// if (function.MetaData.ContainsKey(BlueprintGetterMeta) ||
		// 	function.MetaData.ContainsKey(BlueprintSetterMeta) ||
		// 	function.MetaData.ContainsKey(BlueprintInternalOnlyMeta) ||
		// 	function.MetaData.ContainsKey(CustomThunkMeta) ||
		// 	function.MetaData.ContainsKey(NativeBreakFuncMeta) ||
		// 	function.MetaData.ContainsKey(NativeMakeFuncMeta))
		// {
		// 	return false;
		// }

		return true;
	}

	private static bool HasScriptExposedFields(UhtStruct structObj)
	{
		foreach (UhtFunction function in structObj.Functions)
		{
			if (ShouldExportFunction(function))
			{
				return true;
			}
		}

		foreach (UhtProperty property in structObj.Properties)
		{
			if (ShouldExportProperty(property))
			{
				return true;
			}
		}

		return false;
	}

	private static string GetTooltipDoc(UhtType type)
	{
		string toolTip = type.MetaData.GetValueOrDefault(UhtNames.ToolTip);;
		return toolTip.Trim();
	}

	private static string GetTooltipDoc(UhtEnum enumObj, int index)
	{
		string toolTip = enumObj.MetaData.GetValueOrDefault(UhtNames.ToolTip, index);
		return toolTip.Trim();
	}

	private static string PythonizeName(string name, PythonNameCase nameCase)
	{
		if (String.IsNullOrEmpty(name))
		{
			return name;
		}

		string result = nameCase switch
		{
			PythonNameCase.Lower => name.ToLower(),
			PythonNameCase.Upper => name.ToUpper(),
			_ => name,
		};

		return AvoidPythonNone(result);
	}

	private static string PythonizePropertyName(string name)
	{
		return PythonizeName(name, PythonNameCase.Preserve);
	}

	private static string GetPropertyPythonName(UhtProperty property)
	{
		string baseName = TryGetScriptName(property) ?? property.EngineName;
		return PythonizePropertyName(baseName);
	}

	private static string GetFunctionPythonName(UhtFunction function)
	{
		string baseName = TryGetScriptName(function) ?? function.EngineName;
		return PythonizeName(baseName, PythonNameCase.Preserve);
	}

	private static string GetScriptConstantPythonName(UhtFunction function)
	{
		string baseName = TryGetScriptConstantName(function) ?? TryGetScriptName(function) ?? function.EngineName;
		return PythonizeName(baseName, PythonNameCase.Preserve);
	}

	private static string GetScriptMethodPythonName(UhtFunction function)
	{
		string baseName = TryGetScriptMethodName(function) ?? TryGetScriptName(function) ?? function.EngineName;
		return PythonizeName(baseName, PythonNameCase.Preserve);
	}

	private static string GetTypePythonName(UhtClass classObj)
	{
		string? scriptName = TryGetScriptName(classObj);
		return AvoidPythonNone(scriptName ?? classObj.EngineName);
	}

	private static string GetTypePythonName(UhtScriptStruct structObj)
	{
		string? scriptName = TryGetScriptName(structObj);
		return AvoidPythonNone(scriptName ?? structObj.EngineName);
	}

	private static string GetTypePythonName(UhtEnum enumObj)
	{
		string? scriptName = TryGetScriptName(enumObj);
		if (!String.IsNullOrWhiteSpace(scriptName))
		{
			return AvoidPythonNone(scriptName!);
		}

		string name = enumObj.EngineName;
		return AvoidPythonNone(name);
	}

	private static string GetTypePackageName(UhtType type)
	{
		string packageName = type.Package.SourceName;
		return String.IsNullOrWhiteSpace(packageName) ? String.Empty : packageName;
	}

	private static string GetTypeModuleName(UhtType type)
	{
		return type.Package.Module?.ShortName ?? String.Empty;
	}

	private static string GetTypeNativeName(UhtType type)
	{
		return type.SourceName;
	}

	private static string GetTypeHeaderFile(UhtType type)
	{
		try
		{
			string includePath = type.HeaderFile.IncludeFilePath;
			return String.IsNullOrWhiteSpace(includePath) ? type.HeaderFile.FilePath : includePath;
		}
		catch (UhtIceException)
		{
			return String.Empty;
		}
	}

	private static string GetEnumEntryPythonName(UhtEnum enumObj, string entryName)
	{
		int index = EnumIndexFromName(enumObj, entryName);
		string? metaName = TryGetScriptName(enumObj, index);
		if (!String.IsNullOrWhiteSpace(metaName))
		{
			return AvoidPythonNone(metaName!);
		}

		string shortName = entryName;
		int doubleColonIndex = shortName.LastIndexOf("::", StringComparison.Ordinal);
		if (doubleColonIndex >= 0)
		{
			shortName = shortName.Substring(doubleColonIndex + 2);
		}
		return AvoidPythonNone(shortName);
	}

	private static string AvoidPythonNone(string name)
	{
		return String.Equals(name, "None", StringComparison.Ordinal) ? "NONE" : name;
	}

	private static void ApplyScriptStaticBehavior(UhtFunction function, MethodJsonInfo method)
	{
		if (!function.MetaData.ContainsKey(ScriptStaticMeta))
		{
			method.MethodFlags &= ~(uint)EFunctionFlags.Static;
		}
	}

	private static string? TryGetScriptName(UhtProperty property)
	{
		return null; // NormalizeScriptMetaValue(property.MetaData.GetValueOrDefault(ScriptNameMeta));
	}

	private static string? TryGetScriptName(UhtFunction function)
	{
		return null; // NormalizeScriptMetaValue(function.MetaData.GetValueOrDefault(ScriptNameMeta));
	}

	private static string? TryGetScriptName(UhtClass classObj)
	{
		return null; // NormalizeScriptMetaValue(classObj.MetaData.GetValueOrDefault(ScriptNameMeta));
	}

	private static string? TryGetScriptName(UhtScriptStruct structObj)
	{
		return null; // NormalizeScriptMetaValue(structObj.MetaData.GetValueOrDefault(ScriptNameMeta));
	}

	private static string? TryGetScriptName(UhtEnum enumObj)
	{
		return null; // NormalizeScriptMetaValue(enumObj.MetaData.GetValueOrDefault(ScriptNameMeta));
	}

	private static string? TryGetScriptName(UhtEnum enumObj, int index)
	{
		return null; // NormalizeScriptMetaValue(enumObj.MetaData.GetValueOrDefault(ScriptNameMeta, index));
	}

	private static string? TryGetScriptConstantName(UhtFunction function)
	{
		return NormalizeScriptMetaValue(function.MetaData.GetValueOrDefault(ScriptConstantMeta));
	}

	private static string? TryGetScriptMethodName(UhtFunction function)
	{
		return NormalizeScriptMetaValue(function.MetaData.GetValueOrDefault(ScriptMethodMeta));
	}

	private static string? NormalizeScriptMetaValue(string? rawValue)
	{
		if (String.IsNullOrWhiteSpace(rawValue))
		{
			return null;
		}

		int separatorIndex = rawValue.IndexOf(';');
		string trimmed = separatorIndex >= 0 ? rawValue.Substring(0, separatorIndex) : rawValue;
		trimmed = trimmed.Trim();
		return trimmed.Length > 0 ? trimmed : null;
	}

	private static int EnumIndexFromName(UhtEnum enumObj, string entryName)
	{
		for (int index = 0; index < enumObj.EnumValues.Count; ++index)
		{
			if (enumObj.EnumValues[index].Name.Equals(entryName, StringComparison.Ordinal))
			{
				return index;
			}
		}
		return -1;
	}

	private string BuildMethodDocString(UhtFunction function, string methodName, IEnumerable<MethodParam> parameters)
	{
		return GetTooltipDoc(function);
	}

	private static string GetPropertyPythonType(UhtProperty property)
	{
		return property switch
		{
			UhtBoolProperty => "BoolProperty",
			UhtInt8Property => "Int8Property",
			UhtInt16Property => "Int16Property",
			UhtIntProperty => "IntProperty",
			UhtUInt16Property => "UInt16Property",
			UhtUInt32Property => "UInt32Property",
			UhtUInt64Property => "UInt64Property",
			UhtInt64Property => "Int64Property",
			UhtFloatProperty => "FloatProperty",
			UhtDoubleProperty => "DoubleProperty",
			UhtStrProperty => "StrProperty",
			UhtNameProperty => "NameProperty",
			UhtTextProperty => "TextProperty",
			UhtEnumProperty => "EnumProperty",
			UhtByteProperty => "ByteProperty",
			UhtArrayProperty => "ArrayProperty",
			UhtSetProperty => "SetProperty",
			UhtMapProperty => "MapProperty",
			UhtStructProperty => "StructProperty",
			UhtClassProperty => "ClassProperty",
			UhtSoftClassProperty => "SoftClassProperty",
			UhtSoftObjectProperty => "SoftObjectProperty",
			UhtObjectProperty => "ObjectProperty",
			UhtWeakObjectPtrProperty => "WeakObjectProperty",
			UhtLazyObjectPtrProperty => "LazyObjectProperty",
			UhtFieldPathProperty => "FieldPathProperty",
			UhtMulticastDelegateProperty => "MulticastDelegateProperty",
			UhtDelegateProperty => "DelegateProperty",
			_ => property.EngineClassName
		};
	}

	private string? GetPropertyTypeName(UhtProperty property)
	{
		switch (property)
		{
			case UhtStructProperty structProperty when structProperty.ScriptStruct != null:
				return GetTypePythonName(structProperty.ScriptStruct);
			case UhtEnumProperty enumProperty when enumProperty.Enum != null:
				return GetTypePythonName(enumProperty.Enum);
			case UhtByteProperty byteProperty when byteProperty.Enum != null:
				return GetTypePythonName(byteProperty.Enum);
			case UhtClassProperty classProperty when classProperty.Class != null:
				return GetTypePythonName(classProperty.Class);
			case UhtSoftClassProperty softClassProperty when softClassProperty.Class != null:
				return GetTypePythonName(softClassProperty.Class);
			case UhtObjectPropertyBase objectProperty when objectProperty.Class != null:
				return GetTypePythonName(objectProperty.Class);
			case UhtArrayProperty arrayProperty:
				return GetPropertyDescription(arrayProperty.ValueProperty);
			case UhtSetProperty setProperty:
				return GetPropertyDescription(setProperty.ValueProperty);
			case UhtMapProperty mapProperty:
				string keyName = GetPropertyDescription(mapProperty.KeyProperty);
				string valueName = GetPropertyDescription(mapProperty.ValueProperty);
				return $"{keyName}->{valueName}";
			default:
				return null;
		}
	}

	private string GetPropertyDescription(UhtProperty property)
	{
		string? innerType = GetPropertyInnerTypeName(property);
		if (innerType != null)
		{
			if (property is UhtByteProperty byteProperty && byteProperty.Enum != null)
			{
				return $"ByteProperty<{innerType}>";
			}
			return innerType;
		}

		return property switch
		{
			UhtArrayProperty arrayProperty => $"ArrayProperty[{GetPropertyDescription(arrayProperty.ValueProperty)}]",
			UhtSetProperty setProperty => $"SetProperty[{GetPropertyDescription(setProperty.ValueProperty)}]",
			UhtMapProperty mapProperty => $"MapProperty[{GetPropertyDescription(mapProperty.KeyProperty)}, {GetPropertyDescription(mapProperty.ValueProperty)}]",
			_ => GetPropertyPythonType(property)
		};
	}

	private string? GetPropertyInnerTypeName(UhtProperty property)
	{
		return property switch
		{
			UhtStructProperty structProperty when structProperty.ScriptStruct != null => GetTypePythonName(structProperty.ScriptStruct),
			UhtEnumProperty enumProperty when enumProperty.Enum != null => GetTypePythonName(enumProperty.Enum),
			UhtByteProperty byteProperty when byteProperty.Enum != null => GetTypePythonName(byteProperty.Enum),
			UhtObjectPropertyBase objectProperty when objectProperty.Class != null => GetTypePythonName(objectProperty.Class),
			_ => null
		};
	}

	private string ResolveOutputPath()
	{
		if (_factory.PluginModule != null && !String.IsNullOrEmpty(_factory.PluginModule.BaseDirectory))
		{
			string pluginRoot = ResolvePluginRoot(_factory.PluginModule.BaseDirectory);
			return Path.Combine(pluginRoot, "Tools", "ReflectionData", "unreal.json");
		}

		return _factory.MakePath("unreal", ".json");
	}

	private static string ResolvePluginRoot(string moduleBaseDirectory)
	{
		DirectoryInfo moduleDirectory = new(moduleBaseDirectory);
		DirectoryInfo? pluginRoot = moduleDirectory.Parent?.Parent;
		if (pluginRoot == null)
		{
			throw new InvalidOperationException($"Unable to resolve plugin root from base directory '{moduleBaseDirectory}'.");
		}

		return pluginRoot.FullName;
	}

}
