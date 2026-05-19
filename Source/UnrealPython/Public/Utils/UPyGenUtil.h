// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UPyCommon.h"
#include "Core/UPyPtr.h"
// #include "PythonScriptPluginSettings.h"
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/WeakObjectPtr.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "UObject/PropertyAccessUtil.h"
#include "UObject/WeakFieldPtr.h"
#include "UPySettings.h"

struct FOutParmRec;
struct FUPyWrapperBaseMetaData;

struct FReportUnrealPythonGenerationIssue_Private
{
	static FORCEINLINE void ConditionalBreak(const ELogVerbosity::Type InVerbosity)
	{
		if (InVerbosity <= ELogVerbosity::Error && FPlatformMisc::IsDebuggerPresent())
		{
			UE_DEBUG_BREAK();
		}
	}
};

#define REPORT_UNREAL_PYTHON_GENERATION_ISSUE(Verbosity, Format, ...)	\
	UE_LOG(LogUnrealPython, Verbosity, Format, ##__VA_ARGS__);		\
	FReportUnrealPythonGenerationIssue_Private::ConditionalBreak(ELogVerbosity::Verbosity);


namespace UPyGenUtil
{
	struct FGeneratedWrappedMethodParameter;
	struct FGeneratedWrappedProperty;
	
	extern const FName ScriptNameMetaDataKey;
	extern const FName ScriptNoExportMetaDataKey;
	extern const FName ScriptMethodMetaDataKey;
	extern const FName ScriptMethodSelfReturnMetaDataKey;
	extern const FName ScriptMethodMutableMetaDataKey;
	extern const FName ScriptOperatorMetaDataKey;
	extern const FName ScriptConstantMetaDataKey;
	extern const FName ScriptConstantHostMetaDataKey;
	extern const FName ScriptDefaultMakeMetaDataKey;
	extern const FName ScriptDefaultBreakMetaDataKey;
	extern const FName BlueprintTypeMetaDataKey;
	extern const FName NotBlueprintTypeMetaDataKey;
	extern const FName BlueprintSpawnableComponentMetaDataKey;
	extern const FName BlueprintGetterMetaDataKey;
	extern const FName BlueprintSetterMetaDataKey;
	extern const FName BlueprintInternalUseOnlyMetaDataKey;
	extern const FName BlueprintInternalUseOnlyHierarchicalMetaDataKey;
	extern const FName CustomThunkMetaDataKey;
	extern const FName HasNativeMakeMetaDataKey;
	extern const FName HasNativeBreakMetaDataKey;
	extern const FName NativeBreakFuncMetaDataKey;
	extern const FName NativeMakeFuncMetaDataKey;
	extern const FName DeprecatedPropertyMetaDataKey;
	extern const FName DeprecatedFunctionMetaDataKey;
	extern const FName DeprecationMessageMetaDataKey;

	typedef PyTypeObject* (*UPyAutoWrappedStructCreateFunc)(const UScriptStruct* InStruct, PyTypeObject* InSuperType);
	typedef PyTypeObject* (*UPyAutoWrappedClassCreateFunc)(const UClass* InClass, PyTypeObject* InSuperType, PyObject* InSuperTypes);
	
	/** Name used by the Python equivalent of PostInitProperties */
	static const char* PostInitFuncName = "_post_init";

	/** Buffer for storing the UTF-8 strings used by Python types */
	typedef TArray<char> FUTF8Buffer;

	/** Case sensitive hashing function for TMap */
	template <typename ValueType>
	struct FCaseSensitiveStringMapFuncs : BaseKeyFuncs<ValueType, FString, /*bInAllowDuplicateKeys*/false>
	{
		static FORCEINLINE const FString& GetSetKey(const TPair<FString, ValueType>& Element)
		{
			return Element.Key;
		}
		static FORCEINLINE bool Matches(const FString& A, const FString& B)
		{
			return A.Equals(B, ESearchCase::CaseSensitive);
		}
		static FORCEINLINE uint32 GetKeyHash(const FString& Key)
		{
			return FCrc::StrCrc32<TCHAR>(*Key);
		}
	};

	/** Context used during multithreaded wrapper generation. */
	struct FMultithreadedGenerationContext
	{
		FMultithreadedGenerationContext();
		~FMultithreadedGenerationContext();

		static bool IsMultithreadedGenerationActive();
		static void EnqueueGameThreadTask(TFunction<void()>&& Task);
		static void ExecuteGameThreadTask(TFunctionRef<void()> Task);
		
		bool ExecuteGameThreadTasks();
		void ExecuteGameThreadTasksAndWait();

	private:
		static bool& GetMultithreadedGenerationRef();
	};

	/** Stores the information about a native Python module */
	struct FNativePythonModule
	{
		FNativePythonModule()
			: PyModule(nullptr)
			, PyModuleMethods(nullptr)
			, PyModuleTypes()
		{
		}

		FNativePythonModule(FNativePythonModule&&) = default;
		FNativePythonModule(const FNativePythonModule&) = default;
		FNativePythonModule& operator=(FNativePythonModule&&) = default;
		FNativePythonModule& operator=(const FNativePythonModule&) = default;

		/** Add the given type to this module (the type must have had PyType_Ready called on it) */
		void AddType(PyTypeObject* InType, bool bIsNative=false);

		/** Pointer to the native Python module */
		PyObject* PyModule;

		/** Pointer to a null-terminated array of methods exposed in this module (if any) */
		PyMethodDef* PyModuleMethods;

		/** Array of types exposed in this module (if any) */
		TArray<PyTypeObject*> PyModuleTypes;
	};

	// The following code has been moved to UPyGeneratedWrapper.h.
	/** Stores the data needed by a runtime generated Python parameter */
	// struct FGeneratedWrappedMethodParameter

	/** Stores the data needed to call an Unreal function via Python */
	// struct FGeneratedWrappedFunction

	/** Stores the data needed by a runtime generated Python method */
	// struct FGeneratedWrappedMethod
	
	/** Stores the data needed for a set of runtime generated Python methods */
	// struct FGeneratedWrappedMethods

	/** Stores the data needed by a runtime generated Python method that is dynamically created and registered post-finalize of its owner type (for hoisting util functions onto structs) */
	// struct FGeneratedWrappedDynamicMethod : public FGeneratedWrappedMethod

	/** Stores the data needed by a runtime generated Python method that is dynamically created and registered post-finalize of its owner struct type (for hoisting util functions onto other types) */
	// struct FGeneratedWrappedDynamicMethodWithClosure : public FGeneratedWrappedDynamicMethod

	/** Mixin used to add dynamic method support to a type (base for common non-templated functionality) */
	// struct FGeneratedWrappedDynamicMethodsMixinBase
	
	/** Mixin used to add dynamic method support to a type */
	// template <typename MixedIntoType>
	// struct TGeneratedWrappedDynamicMethodsMixin : public FGeneratedWrappedDynamicMethodsMixinBase

	/** Operator types supported by Python */
	enum class EGeneratedWrappedOperatorType : uint8
	{
		Bool = 0,			// bool
		Equal,				// ==
		NotEqual,			// !=
		Less,				// <
		LessEqual,			// <=
		Greater,			// >
		GreaterEqual,		// >=
		Add,				// +
		InlineAdd,			// +=
		Subtract,			// -
		InlineSubtract,		// -=
		Multiply,			// *
		InlineMultiply,		// *=
		Divide,				// /
		InlineDivide,		// /=
		Modulus,			// %
		InlineModulus,		// %=
		And,				// &
		InlineAnd,			// &=
		Or,					// |
		InlineOr,			// |=
		Xor,				// ^
		InlineXor,			// ^=
		RightShift,			// >>
		InlineRightShift,	// >>=
		LeftShift,			// <<
		InlineLeftShift,	// <<=
		Negated,			// -obj prefix operator
		Num,
	};

	/** Stores the signature definition of a runtime generated Python operator stack function */
	// struct FGeneratedWrappedOperatorSignature

	/** Stores the data needed by a runtime generated Python operator stack function */
	// struct FGeneratedWrappedOperatorFunction

	/** Stores the data needed by a runtime generated Python operator stack that is dynamically created and registered post-finalize of its owner type (for hoisting operators onto other types) */
	// struct FGeneratedWrappedOperatorStack

	/** Stores the data needed to access an Unreal property via Python */
	// struct FGeneratedWrappedProperty

	/** Stores the data needed by a runtime generated Python get/set */
	// struct FGeneratedWrappedGetSet
	
	/** Stores the data needed for a set of runtime generated Python get/sets */
	// struct FGeneratedWrappedGetSets

	/** Stores the data needed by a runtime generated Python constant */
	// struct FGeneratedWrappedConstant

	/** Stores the data needed for a set of runtime generated Python constants */
	// struct FGeneratedWrappedConstants
	
	/** Stores the data needed by a runtime generated Python constant that is dynamically created and registered post-finalize of its owner type (for hoisting constant functions onto other types) */
	// struct FGeneratedWrappedDynamicConstantWithClosure : public FGeneratedWrappedConstant

	/** Mixin used to add dynamic constant support to a type (base for common non-templated functionality) */
	// struct FGeneratedWrappedDynamicConstantsMixinBase

	/** Mixin used to add dynamic constant support to a type */
	// template <typename MixedIntoType>
	// struct TGeneratedWrappedDynamicConstantsMixin : public FGeneratedWrappedDynamicConstantsMixinBase
	
	/** Stores the data needed to generate a Python doc string for editor exposed properties */
	// struct FGeneratedWrappedPropertyDoc
	
	/** Utility for tracking and logging field conflicts when exported to Python */
	// struct FGeneratedWrappedFieldTracker
	
	/** Stores the minimal data needed by a runtime generated Python type */
	// struct FGeneratedWrappedType

	// The following code has been moved to UPyGeneratedWrapperStructType.h.
	/** Stores the data needed by a runtime generated Python struct type */
	// struct FGeneratedWrappedStructType : public FGeneratedWrappedType, public TGeneratedWrappedDynamicMethodsMixin<FGeneratedWrappedStructType>, public TGeneratedWrappedDynamicConstantsMixin<FGeneratedWrappedStructType>
	
	// The following code has been moved to UPyGeneratedWrapperClassType.h.
	/** Stores the data needed by a runtime generated Python class type */
	// struct FGeneratedWrappedClassType : public FGeneratedWrappedType, public TGeneratedWrappedDynamicMethodsMixin<FGeneratedWrappedStructType>, public TGeneratedWrappedDynamicConstantsMixin<FGeneratedWrappedStructType>

	// The following code has been moved to UPyGeneratedWrapperEnumType.h.
	/** Stores the data needed by a runtime generated Python enum entry */
	// struct FGeneratedWrappedEnumEntry

	// The following code has been moved to UPyGeneratedWrapperEnumType.h.
	/** Stores the data needed by a runtime generated Python enum type */
	// struct FGeneratedWrappedEnumType : public FGeneratedWrappedType
	
	/** Definition data for an Unreal property generated from a Python type */
	// struct FPropertyDef
	
	/** Definition data for an Unreal function generated from a Python type */
	// struct FFunctionDef

	/** How should PythonizeName adjust the final name? */
	enum EPythonizeNameCase : uint8
	{
		/** lower_snake_case */
		Lower,
		/** UPPER_SNAKE_CASE */
		Upper,
	};

	/** Flags controlling the behavior of PythonizeValue/PythonizeMethodParam/PythonizeMethodReturnType. */
	enum class EPythonizeFlags
	{
		None = 0,
		IncludeUnrealNamespace = 1<<0,
		UseStrictTyping = 1<<1,
		DefaultConstructStructs = 1<<2,
		DefaultConstructDateTime = 1<<3,

		/** Type hinting is enabled. */
		WithTypeHinting = 1<<4,

		/** If the type of a value being hinted can be None, wrap the type in Optional[T], which is an alias to Union[T, None]. */
		WithOptionalType = 1<<5,

		/** Declare the type along with it known type coercion. For example [unreal.Name, str] because a string can be passed in place of a Name. */
		WithTypeCoercion = 1<<6,

		/** In method declaration, add '...' as default value even if no default value was found. Used in stub generation to workaround 'non-defaulted param after defaulted param' coming from the 'malformed' UFUNCTION. */
		AddMissingDefaultValueEllipseWorkaround = 1<<7,
	};
	ENUM_CLASS_FLAGS(EPythonizeFlags)

	/** Context information passed to PythonizeTooltip */
	struct FPythonizeTooltipContext
	{
		FPythonizeTooltipContext()
			: Prop(nullptr)
			, Func(nullptr)
			, ReadOnlyFlags(PropertyAccessUtil::RuntimeReadOnlyFlags)
		{
		}

		explicit FPythonizeTooltipContext(const FProperty* InProp, const uint64 InReadOnlyFlags = PropertyAccessUtil::RuntimeReadOnlyFlags);

		explicit FPythonizeTooltipContext(const UFunction* InFunc, const TSet<FName>& InParamsToIgnore = TSet<FName>());

		/** Optional property that should be used when converting property tooltips */
		const FProperty* Prop;

		/** Optional function that should be used when converting function tooltips */
		const UFunction* Func;

		/** Flags that dictate whether the property should be considered read-only */
		uint64 ReadOnlyFlags;

		/** Optional deprecation message for the property or function */
		FString DeprecationMessage;

		/** Optional set of parameters that we should ignore when generating function tooltips */
		TSet<FName> ParamsToIgnore;
	};

	/** Parsed tooltip data used by PythonizeTooltip */
	struct FParsedTooltip
	{
		struct FTokenString
		{
			FTokenString() = default;
			FTokenString(FTokenString&&) = default;
			FTokenString& operator=(FTokenString&&) = default;

			bool operator==(const FTokenString& InOther) const
			{
				return GetValue() == InOther.GetValue();
			}

			bool operator!=(const FTokenString& InOther) const
			{
				return GetValue() != InOther.GetValue();
			}

			FStringView GetValue() const
			{
				return SimpleValue.Len() > 0
					? SimpleValue
					: FStringView(ComplexValue);
			}

			void SetValue(FStringView InValue)
			{
				SimpleValue = InValue;
				ComplexValue.Reset();
			}

			void SetValue(FString&& InValue)
			{
				SimpleValue.Reset();
				ComplexValue = MoveTemp(InValue);
			}

			FStringView SimpleValue;
			FString ComplexValue;
		};

		struct FMiscToken
		{
			FMiscToken() = default;
			FMiscToken(FMiscToken&&) = default;
			FMiscToken& operator=(FMiscToken&&) = default;

			FTokenString TokenName;
			FTokenString TokenValue;
		};

		struct FParamToken
		{
			FParamToken() = default;
			FParamToken(FParamToken&&) = default;
			FParamToken& operator=(FParamToken&&) = default;

			FTokenString ParamName;
			FTokenString ParamType;
			FTokenString ParamComment;
		};

		typedef TArray<FMiscToken, TInlineAllocator<4>> FMiscTokensArray;
		typedef TArray<FParamToken, TInlineAllocator<8>> FParamTokensArray;

		int32 SourceTooltipLen = 0;

		FString BasicTooltipText;
		FMiscTokensArray MiscTokens;
		FParamTokensArray ParamTokens;
		FParamToken ReturnToken;
	};

	/** Convert a TCHAR to a UTF-8 buffer */
	FUTF8Buffer TCHARToUTF8Buffer(const TCHAR* InStr);

	/** Get the PostInit function from this Python type */
	PyObject* GetPostInitFunc(PyTypeObject* InPyType);

	/** Add a struct init parameter to the given array of method parameters */
	void AddStructInitParam(const FProperty* InUnrealProp, const TCHAR* InPythonAttrName, TArray<FGeneratedWrappedMethodParameter>& OutInitParams);

	/** Given a function, extract all of its parameter information (input and output) */
	void ExtractFunctionParams(const UFunction* InFunc, TArray<FGeneratedWrappedMethodParameter>& OutInputParams, TArray<FGeneratedWrappedMethodParameter>& OutOutputParams);

	/** Apply default values to function arguments */
	void ApplyParamDefaults(void* InBaseParamsAddr, const TArray<FGeneratedWrappedMethodParameter>& InParamDef);

	/** Parse an array Python objects from the args and keywords based on the generated method parameter data */
	bool ParseMethodParameters(PyObject* InArgs, PyObject* InKwds, const TArray<FGeneratedWrappedMethodParameter>& InParamDef, const char* InPyMethodName, TArray<PyObject*>& OutPyParams);

	/** Given a set of return values and the struct data associated with them, pack them appropriately for returning to Python */
	PyObject* PackReturnValues(const void* InBaseParamsAddr, const TArray<FGeneratedWrappedMethodParameter>& InOutputParams, const TCHAR* InErrorCtxt, const TCHAR* InCallingCtxt);

	/** Given a Python return value, unpack the values into the struct data associated with them */
	bool UnpackReturnValues(PyObject* InRetVals, const FOutParmRec* InOutputParms, const TCHAR* InErrorCtxt, const TCHAR* InCallingCtxt);

	/** Invoke a Python callable from an Unreal function thunk (ie, DEFINE_FUNCTION), ensuring that the calling convention is correct for both native and script function callers */
	bool InvokePythonCallableFromUnrealFunctionThunk(FUPyObjectPtr InSelf, PyObject* InCallable, const UFunction* InFunc, UObject* Context, FFrame& Stack, RESULT_DECL);

	/** Get the current value of the given property from the given struct */
	PyObject* GetPropertyValue(const UStruct* InStruct, const void* InStructData, const FGeneratedWrappedProperty& InPropDef, const char *InAttributeName, PyObject* InOwnerPyObject, const TCHAR* InErrorCtxt);

	/** Set the current value of the given property from the given struct */
	int SetPropertyValue(const UStruct* InStruct, void* InStructData, PyObject* InValue, const FGeneratedWrappedProperty& InPropDef, const char *InAttributeName, const FPropertyAccessChangeNotify* InChangeNotify, const uint64 InReadOnlyFlags, const bool InOwnerIsTemplate, const TCHAR* InErrorCtxt, const TConstArrayView<void*>& InArchetypeInstContainers);

	/** Build a Python doc string for the given function and arguments list */
	FString BuildFunctionDocString(const UFunction* InFunc, const FString& InFuncPythonName, const TArray<FGeneratedWrappedMethodParameter>& InInputParams, const TArray<FGeneratedWrappedMethodParameter>& InOutputParams, const bool* InStaticOverride = nullptr);

	/** Is the given class generated by Blueprints? */
	bool IsBlueprintGeneratedClass(const UClass* InClass);

	/** Is the given struct generated by Blueprints? */
	bool IsBlueprintGeneratedStruct(const UScriptStruct* InStruct);

	/** Is the given enum generated by Blueprints? */
	bool IsBlueprintGeneratedEnum(const UEnum* InEnum);

	/** Is the given class marked as deprecated? */
	bool IsDeprecatedClass(const UClass* InClass, FString* OutDeprecationMessage = nullptr);

	/** Is the given property marked as deprecated? */
	bool IsDeprecatedProperty(const FProperty* InProp, FString* OutDeprecationMessage = nullptr);

	/** Is the given function marked as deprecated? */
	bool IsDeprecatedFunction(const UFunction* InFunc, FString* OutDeprecationMessage = nullptr);

	/** Given a class, get all the interface classes related to it that should be included when exporting it to Python */
	TArray<const UClass*> GetExportedInterfacesForClass(const UClass* InClass);

	/** Should the given class be exported to Python? */
	bool ShouldExportClass(const UClass* InClass);

	/** Should the given struct be exported to Python? */
	bool ShouldExportStruct(const UScriptStruct* InStruct);

	/** Should the given enum be exported to Python? */
	bool ShouldExportEnum(const UEnum* InEnum);

	/** Should the given enum entry be exported to Python? */
	bool ShouldExportEnumEntry(const UEnum* InEnum, int32 InEnumEntryIndex);

	/** Should the given property be exported to Python? */
	bool ShouldExportProperty(const FProperty* InProp);

	/** Should the given property be exported to Python as editor-only data? */
	bool ShouldExportEditorOnlyProperty(const FProperty* InProp);

	/** Should the given function be exported to Python? */
	bool ShouldExportFunction(const UFunction* InFunc);

	/** Check that the given name will be valid for Python */
	bool IsValidName(FStringView InName, FText* OutError = nullptr);

	/** Given a CamelCase name, convert it to snake_case */
	FString PythonizeName(FStringView InName, const EPythonizeNameCase InNameCase);

	/** Given a CamelCase property name, convert it to snake_case (may remove some superfluous parts of the property name) */
	FString PythonizePropertyName(FStringView InName, const EPythonizeNameCase InNameCase);

	/** Given a property tooltip, convert it to a doc string */
	FString PythonizePropertyTooltip(const FParsedTooltip& InTooltip, const FProperty* InProp, const uint64 InReadOnlyFlags = PropertyAccessUtil::RuntimeReadOnlyFlags);

	/** Given a function tooltip, convert it to a doc string */
	FString PythonizeFunctionTooltip(const FParsedTooltip& InTooltip, const UFunction* InFunc, const TSet<FName>& ParamsToIgnore = TSet<FName>());

	/** Given a parsed tooltip, convert it to a doc string */
	FString PythonizeTooltip(const FParsedTooltip& InTooltip, const FPythonizeTooltipContext& InContext = FPythonizeTooltipContext());

	/**
	 * Given a tooltip, parse it into something that can be converted into a doc string
	 * @note The parsed result may referenced sub-strings of the given view, so InTooltip must exist as long as the result does.
	 */
	FParsedTooltip ParseTooltip(FStringView InTooltip);

	/** Given a property and its value, convert it into something that could be used in a Python script */
	FString PythonizeValue(const FProperty* InProp, const void* InPropValue, const EPythonizeFlags InPythonizeFlags = EPythonizeFlags::None);

	/** Given a property and its default value, convert it into something that could be used in a Python script */
	FString PythonizeDefaultValue(const FProperty* InProp, const FString& InDefaultValue, const EPythonizeFlags InPythonizeFlags = EPythonizeFlags::None);

	/** Given a generated method parameter, extract the name, type and default value and format a parameter declaration usable in a Python method signature according to the specified flags. */
	FString PythonizeMethodParam(const FGeneratedWrappedMethodParameter& InMethodParam, EPythonizeFlags InPythonizeFlags = EPythonizeFlags::None, const TFunction<bool(const FString&)>& ShouldEllipseDefaultValue = TFunction<bool(const FString&)>());

	/** Given a property and a name, format a parameter declaration usable in a Python method signature according to the specified flags. */
	FString PythonizeMethodParam(const FString& InParamName, const FProperty* ParamProp, EPythonizeFlags InPythonizeFlags = EPythonizeFlags::None);

	/** Given a param name and optional param type and default value, format a parameter declaration usable in a Python method signature. */
	FString PythonizeMethodParam(const FString& InParamName, const FString& InParamType = FString(), const FString& InDefaultValue = FString());

	/** Given list of return parameter, format the return type declaration usable in the Python method signature according to the flags. */
	FString PythonizeMethodReturnType(const TArray<UPyGenUtil::FGeneratedWrappedMethodParameter>& InOutputParams, EPythonizeFlags InPythonizeFlags = EPythonizeFlags::None);

	/** Given a property, format a return type declaration usable in the Python method signature according to the flags. */
	FString PythonizeMethodReturnType(const FProperty* InProp, EPythonizeFlags InPythonizeFlags = EPythonizeFlags::None);

	/** Get the type that should be used with the Python type registry for an asset (eg, a Blueprint asset should use its generated class) */
	const UObject* GetAssetTypeRegistryType(const UObject* InObj);

	/** Get the native module the given field belongs to */
	FString GetFieldModule(const UField* InField);

	/** Get the plugin module the given field belongs to (if any) */
	FString GetFieldPlugin(const UField* InField);

	/** Given a native module name, get the Python module we should use */
	FString GetModulePythonName(const FName InModuleName, const bool bIncludePrefix = true);

	/** Get the Python name of the given class */
	FString GetClassPythonName(const UClass* InClass);

	/** Get the deprecated Python names of the given class */
	TArray<TTuple<FSoftObjectPath, FString>> GetDeprecatedClassPythonNames(const UClass* InClass);

	/** Get the Python name of the given struct */
	FString GetStructPythonName(const UScriptStruct* InStruct);

	/** Get the deprecated Python names of the given struct */
	TArray<TTuple<FSoftObjectPath, FString>> GetDeprecatedStructPythonNames(const UScriptStruct* InStruct);

	/** Get the Python name of the given enum */
	FString GetEnumPythonName(const UEnum* InEnum);

	/** Get the deprecated Python names of the given enum */
	TArray<TTuple<FSoftObjectPath, FString>> GetDeprecatedEnumPythonNames(const UEnum* InEnum);

	/** Get the Python name of the given enum entry */
	FString GetEnumEntryPythonName(const UEnum* InEnum, const int32 InEntryIndex);

	/** Get the deprecated Python names of the given enum entry */
	TArray<FString> GetDeprecatedEnumEntryPythonNames(const UEnum* InEnum, const int32 InEntryIndex);

	/** Get the Python name of the given delegate signature */
	FString GetDelegatePythonName(const UFunction* InDelegateSignature);

	/** Get the Python name of the given function */
	FString GetFunctionPythonName(const UFunction* InFunc);

	/** Get the deprecated Python names of the given function */
	TArray<TTuple<FSoftObjectPath, FString>> GetDeprecatedFunctionPythonNames(const UFunction* InFunc);

	/** Get the Python name of the given function when it's hoisted as a script method */
	FString GetScriptMethodPythonName(const UFunction* InFunc);

	/** Get the deprecated Python names of the given function it's hoisted as a script method */
	TArray<TTuple<FSoftObjectPath, FString>> GetDeprecatedScriptMethodPythonNames(const UFunction* InFunc);

	/** Get the Python name of the given function when it's hoisted as a script constant */
	FString GetScriptConstantPythonName(const UFunction* InFunc);

	/** Get the deprecated Python names of the given function it's hoisted as a script constant */
	TArray<TTuple<FSoftObjectPath, FString>> GetDeprecatedScriptConstantPythonNames(const UFunction* InFunc);

	/** Get the Python name of the given property */
	FString GetPropertyPythonName(const FProperty* InProp);

	/** Get the deprecated Python names of the given property */
	TArray<TTuple<FSoftObjectPath, FString>> GetDeprecatedPropertyPythonNames(const FProperty* InProp);

	/** Get the Python name of the given property */
	FString GetPropertyTypePythonName(const FProperty* InProp, const bool InIncludeUnrealNamespace = false, const bool InIsForDocString = true, const EPythonizeFlags PythonizeFlags = EPythonizeFlags::None);

	/** Get the Python type of the given property */
	FString GetPropertyPythonType(const FProperty* InProp);

	/** Append the Python type of the given property to the given string */
	void AppendPropertyPythonType(const FProperty* InProp, FString& OutStr);

	/** Append the given Python property read /write state to the given string */
	void AppendPropertyPythonReadWriteState(const FProperty* InProp, FString& OutStr, const uint64 InReadOnlyFlags = PropertyAccessUtil::RuntimeReadOnlyFlags);

	/** Get the tooltip for the given field */
	FString GetFieldTooltip(const UField* InField);
	FString GetFieldTooltip(const FField* InField);

	/** Get the tooltip for the given enum entry */
	FString GetEnumEntryTooltip(const UEnum* InEnum, const int64 InEntryIndex);

	/** Get the doc string for the C++ or asset source information of the given type */
	FString BuildSourceInformationDocString(const UField* InOwnerType);

	/** Append the doc string for the C++ or asset source information of the given type to the given string */
	void AppendSourceInformationDocString(const UField* InOwnerType, FString& OutStr);

	/** Save a generated text file to disk as UTF-8 (only writes the file if the contents differs, unless forced) */
	bool SaveGeneratedTextFile(const TCHAR* InFilename, FStringView InFileContents, const bool InForceWrite = false);

	/**
	 * Whether Python type hints should be generated. This is only effective if Python interpreter version has the support. This
	 * affects the Python stub and docstrings. The function must be called before Python glue and the stub are generated. It
	 * can be set from the user settings or forced to a value for a specific purpose, like generating the online doc.
	 */
	void SetTypeHintingMode(EUPyTypeHintingMode InMode);

	/**
	 * Return the effecting type hinting mode.
	 * @see SetTypeHintingMode.
	 */
	EUPyTypeHintingMode GetTypeHintingMode();

	/**
	 * Whether the stub and docstrings are type hinted during glue/stub generation. Default to false. This doesn't read
	 * the user settings, but rather the value set with SetTypeHintingMode(). The function always returns false if
	 * the linked Python interpreter version is older than 3.7.
	 */
	bool IsTypeHintingEnabled();

	UFunction* FindMakeBreakFunction(const UScriptStruct* InStruct, const FName& InNativeMetaDataKey, const FName& InScriptDefaultMetaDataKey);
}
