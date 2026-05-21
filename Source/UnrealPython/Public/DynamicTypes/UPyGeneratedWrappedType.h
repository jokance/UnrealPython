
#pragma once

#include "UPyCommon.h"
#include "Utils/UPyGenUtil.h"
#include "DynamicTypes/UPyMethodWithClosure.h"
#include "DynamicTypes/UPyConstant.h"

struct FUPyWrapperBaseMetaData;

namespace UPyGenUtil
{
/** Stores the data needed by a runtime generated Python parameter */
struct UNREALPYTHON_API FGeneratedWrappedMethodParameter
{
	FGeneratedWrappedMethodParameter()
		: ParamProp(nullptr)
	{
	}

	FGeneratedWrappedMethodParameter(FGeneratedWrappedMethodParameter&&) = default;
	FGeneratedWrappedMethodParameter(const FGeneratedWrappedMethodParameter&) = default;
	FGeneratedWrappedMethodParameter& operator=(FGeneratedWrappedMethodParameter&&) = default;
	FGeneratedWrappedMethodParameter& operator=(const FGeneratedWrappedMethodParameter&) = default;

	/** The name of the parameter */
	FUTF8Buffer ParamName;

	/** The Unreal property for this parameter */
	const FProperty* ParamProp;

	/** The default Unreal ExportText value of this parameter; parameters with this set are considered optional */
	TOptional<FString> ParamDefaultValue;
};

/** Stores the data needed to call an Unreal function via Python */
struct UNREALPYTHON_API FGeneratedWrappedFunction
{
	enum ESetFunctionFlags
	{
		SFF_None = 0,
		SFF_CalculateDeprecationState = 1<<0,
		SFF_ExtractParameters = 1<<1,
		SFF_All = SFF_CalculateDeprecationState | SFF_ExtractParameters,
	};

	FGeneratedWrappedFunction()
		: Func(nullptr)
	{
	}

	FGeneratedWrappedFunction(FGeneratedWrappedFunction&&) = default;
	FGeneratedWrappedFunction(const FGeneratedWrappedFunction&) = default;
	FGeneratedWrappedFunction& operator=(FGeneratedWrappedFunction&&) = default;
	FGeneratedWrappedFunction& operator=(const FGeneratedWrappedFunction&) = default;

	/** Set the function, optionally also calculating some data about it */
	void SetFunction(const UFunction* InFunc, const uint32 InSetFuncFlags = SFF_All);

	/** The Unreal function to call (static dispatch) */
	TObjectPtr<const UFunction> Func;

	/** Array of input parameters associated with the function */
	TArray<FGeneratedWrappedMethodParameter> InputParams;

	/** Array of output (including return) parameters associated with the function */
	TArray<FGeneratedWrappedMethodParameter> OutputParams;

	/** Set if this function is deprecated and using it should emit a deprecation warning */
	TOptional<FString> DeprecationMessage;
};

/** Stores the data needed by a runtime generated Python method */
struct UNREALPYTHON_API FGeneratedWrappedMethod
{
	FGeneratedWrappedMethod()
		: MethodCallback(nullptr)
		, MethodFlags(0)
	{
	}

	FGeneratedWrappedMethod(FGeneratedWrappedMethod&&) = default;
	FGeneratedWrappedMethod(const FGeneratedWrappedMethod&) = default;
	FGeneratedWrappedMethod& operator=(FGeneratedWrappedMethod&&) = default;
	FGeneratedWrappedMethod& operator=(const FGeneratedWrappedMethod&) = default;

	/** Convert this wrapper type to its Python type */
	void ToPython(FUPyMethodWithClosureDef& OutPyMethod) const;

	/** The name of the method */
	FUTF8Buffer MethodName;

	/** The doc string of the method */
	FUTF8Buffer MethodDoc;

	/** The Unreal function for this method */
	FGeneratedWrappedFunction MethodFunc;

	/* The C function this method should call */
	UPyCFunctionWithClosure MethodCallback;

	/* The METH_ flags for this method */
	int MethodFlags;
};

/** Stores the data needed for a set of runtime generated Python methods */
struct FGeneratedWrappedMethods
{
	FGeneratedWrappedMethods() = default;
	FGeneratedWrappedMethods(FGeneratedWrappedMethods&&) = default;
	FGeneratedWrappedMethods(const FGeneratedWrappedMethods&) = delete;
	FGeneratedWrappedMethods& operator=(FGeneratedWrappedMethods&&) = default;
	FGeneratedWrappedMethods& operator=(const FGeneratedWrappedMethods&) = delete;

	/** Called to ready the generated methods with Python */
	void Finalize();

	/** Array of methods associated from the wrapped type */
	TArray<FGeneratedWrappedMethod> TypeMethods;

	/** The Python methods that were generated from TypeMethods (in Finalize) */
	TArray<FUPyMethodWithClosureDef> PyMethods;
};

/** Stores the data needed by a runtime generated Python method that is dynamically created and registered post-finalize of its owner type (for hoisting util functions onto structs) */
struct FGeneratedWrappedDynamicMethod : public FGeneratedWrappedMethod
{
	FGeneratedWrappedDynamicMethod() = default;
	FGeneratedWrappedDynamicMethod(FGeneratedWrappedDynamicMethod&&) = default;
	FGeneratedWrappedDynamicMethod(const FGeneratedWrappedDynamicMethod&) = default;
	FGeneratedWrappedDynamicMethod& operator=(FGeneratedWrappedDynamicMethod&&) = default;
	FGeneratedWrappedDynamicMethod& operator=(const FGeneratedWrappedDynamicMethod&) = default;

	/** The 'self' parameter information (this parameter is set to the instance that calls the method) */
	FGeneratedWrappedMethodParameter SelfParam;

	/** The 'self' return information (this value overwrites the instance that calls the method) */
	FGeneratedWrappedMethodParameter SelfReturn;
};

/** Stores the data needed by a runtime generated Python method that is dynamically created and registered post-finalize of its owner struct type (for hoisting util functions onto other types) */
struct FGeneratedWrappedDynamicMethodWithClosure : public FGeneratedWrappedDynamicMethod
{
	FGeneratedWrappedDynamicMethodWithClosure() = default;
	FGeneratedWrappedDynamicMethodWithClosure(FGeneratedWrappedDynamicMethodWithClosure&&) = default;
	FGeneratedWrappedDynamicMethodWithClosure(const FGeneratedWrappedDynamicMethodWithClosure&) = delete;
	FGeneratedWrappedDynamicMethodWithClosure& operator=(FGeneratedWrappedDynamicMethodWithClosure&&) = default;
	FGeneratedWrappedDynamicMethodWithClosure& operator=(const FGeneratedWrappedDynamicMethodWithClosure&) = delete;

	/** Called to ready the generated method for Python */
	void Finalize();

	/** Python method that was generated from this method */
	FUPyMethodWithClosureDef PyMethod;
};

/** Mixin used to add dynamic method support to a type (base for common non-templated functionality) */
struct FGeneratedWrappedDynamicMethodsMixinBase
{
	FGeneratedWrappedDynamicMethodsMixinBase() = default;
	FGeneratedWrappedDynamicMethodsMixinBase(FGeneratedWrappedDynamicMethodsMixinBase&&) = default;
	FGeneratedWrappedDynamicMethodsMixinBase(const FGeneratedWrappedDynamicMethodsMixinBase&) = delete;
	FGeneratedWrappedDynamicMethodsMixinBase& operator=(FGeneratedWrappedDynamicMethodsMixinBase&&) = default;
	FGeneratedWrappedDynamicMethodsMixinBase& operator=(const FGeneratedWrappedDynamicMethodsMixinBase&) = delete;

	/** Called to add a dynamic method to the Python type (this should only be called post-finalize) */
	void AddDynamicMethodImpl(FGeneratedWrappedDynamicMethod&& InDynamicMethod, PyTypeObject* InPyType);

	/** Array of dynamic methods associated with this type (call AddDynamicMethod to add methods) */
	TArray<TSharedRef<FGeneratedWrappedDynamicMethodWithClosure>> DynamicMethods;
};

/** Mixin used to add dynamic method support to a type */
template <typename MixedIntoType>
struct TGeneratedWrappedDynamicMethodsMixin : public FGeneratedWrappedDynamicMethodsMixinBase
{
	TGeneratedWrappedDynamicMethodsMixin() = default;
	TGeneratedWrappedDynamicMethodsMixin(TGeneratedWrappedDynamicMethodsMixin&&) = default;
	TGeneratedWrappedDynamicMethodsMixin(const TGeneratedWrappedDynamicMethodsMixin&) = delete;
	TGeneratedWrappedDynamicMethodsMixin& operator=(TGeneratedWrappedDynamicMethodsMixin&&) = default;
	TGeneratedWrappedDynamicMethodsMixin& operator=(const TGeneratedWrappedDynamicMethodsMixin&) = delete;

	/** Called to add a dynamic method to the Python type (this should only be called post-finalize) */
	void AddDynamicMethod(FGeneratedWrappedDynamicMethod&& InDynamicMethod)
	{
		AddDynamicMethodImpl(MoveTemp(InDynamicMethod), &static_cast<MixedIntoType*>(this)->PyType);
	}
};

/** Stores the signature definition of a runtime generated Python operator stack function */
struct UNREALPYTHON_API FGeneratedWrappedOperatorSignature
{
	enum class EType
	{
		/** This operator type should be void */
		None,
		/** This operator type can be anything */
		Any,
		/** This operator type should be a struct (of the same type as 'self') */
		Struct,
		/** This operator type should be a bool */
		Bool
	};

	FGeneratedWrappedOperatorSignature()
		: OpType(EGeneratedWrappedOperatorType::Num)
		, OpTypeStr(TEXT(""))
		, PyFuncName(TEXT(""))
		, ReturnType(EType::None)
		, OtherType(EType::None)
	{
	}

	FGeneratedWrappedOperatorSignature(const EGeneratedWrappedOperatorType InOpType, const TCHAR* InOpTypeStr, const TCHAR* InPyFuncName, const EType InReturnType, const EType InOtherType)
		: OpType(InOpType)
		, OpTypeStr(InOpTypeStr)
		, PyFuncName(InPyFuncName)
		, ReturnType(InReturnType)
		, OtherType(InOtherType)
	{
	}

	/**
	 * Given an operator type, return its signature
	 */
	static const FGeneratedWrappedOperatorSignature& OpTypeToSignature(const EGeneratedWrappedOperatorType InOpType);

	/**
	 * Given a potential operator string, try and convert it to a known operator signature
	 * @return true if the conversion was a success, false otherwise
	 */
	static bool StringToSignature(const TCHAR* InStr, FGeneratedWrappedOperatorSignature& OutSignature);

	/**
	 * Validate that the given parameter is compatible with the expected type.
	 */
	static bool ValidateParam(const FGeneratedWrappedMethodParameter& InParam, const EType InType, const UScriptStruct* InStructType, FString* OutError = nullptr);

	/**
	 * Get the number of input parameter that are expected by this operator.
	 */
	int32 GetInputParamCount() const;

	/**
	 * Get the number of output parameter that are expected by this operator.
	 */
	int32 GetOutputParamCount() const;

	/** The operator enum value */
	EGeneratedWrappedOperatorType OpType;

	/** The string representation of this operator (see the comments on EGeneratedWrappedOperatorType) */
	const TCHAR* OpTypeStr;

	/** The name of the function used in Python for this operator) */
	const TCHAR* PyFuncName;

	/** The type that is required by the return value of this operator (a struct return implies a 'self' return) */
	EType ReturnType;

	/** The type that is required by the other value of this operator (for binary operators) */
	EType OtherType;
};

/** Stores the data needed by a runtime generated Python operator stack function */
struct UNREALPYTHON_API FGeneratedWrappedOperatorFunction
{
	FGeneratedWrappedOperatorFunction() = default;
	FGeneratedWrappedOperatorFunction(FGeneratedWrappedOperatorFunction&&) = default;
	FGeneratedWrappedOperatorFunction(const FGeneratedWrappedOperatorFunction&) = default;
	FGeneratedWrappedOperatorFunction& operator=(FGeneratedWrappedOperatorFunction&&) = default;
	FGeneratedWrappedOperatorFunction& operator=(const FGeneratedWrappedOperatorFunction&) = default;

	/** Set the function, validating that it's compatible with the given signature */
	bool SetFunction(const UFunction* InFunc, const FGeneratedWrappedOperatorSignature& InSignature, FString* OutError = nullptr);
	bool SetFunction(const FGeneratedWrappedFunction& InFuncDef, const FGeneratedWrappedOperatorSignature& InSignature, FString* OutError = nullptr);

	/** The Unreal function to call (static dispatch) */
	const UFunction* Func;

	/** The 'self' parameter information (this parameter is set to the instance that calls the function) */
	FGeneratedWrappedMethodParameter SelfParam;

	/** The 'self' return information (this value overwrites the instance that calls the function) */
	FGeneratedWrappedMethodParameter SelfReturn;

	/** The 'other' parameter information (for binary operators) */
	FGeneratedWrappedMethodParameter OtherParam;

	/** The return parameter information */
	FGeneratedWrappedMethodParameter ReturnParam;

	/** Array of additional defaulted input parameters */
	TArray<FGeneratedWrappedMethodParameter> AdditionalParams;
};

/** Stores the data needed by a runtime generated Python operator stack that is dynamically created and registered post-finalize of its owner type (for hoisting operators onto other types) */
struct UNREALPYTHON_API FGeneratedWrappedOperatorStack
{
	FGeneratedWrappedOperatorStack() = default;
	FGeneratedWrappedOperatorStack(FGeneratedWrappedOperatorStack&&) = default;
	FGeneratedWrappedOperatorStack(const FGeneratedWrappedOperatorStack&) = default;
	FGeneratedWrappedOperatorStack& operator=(FGeneratedWrappedOperatorStack&&) = default;
	FGeneratedWrappedOperatorStack& operator=(const FGeneratedWrappedOperatorStack&) = default;

	/** Array of operator functions associated with this operator stack */
	TArray<FGeneratedWrappedOperatorFunction> Funcs;
};

/** Stores the data needed to access an Unreal property via Python */
struct UNREALPYTHON_API FGeneratedWrappedProperty
{
	enum ESetPropertyFlags
	{
		SPF_None = 0,
		SPF_CalculateDeprecationState = 1<<0,
		SPF_All = SPF_CalculateDeprecationState,
	};

	FGeneratedWrappedProperty()
		: Prop(nullptr)
	{
	}

	FGeneratedWrappedProperty(FGeneratedWrappedProperty&&) = default;
	FGeneratedWrappedProperty(const FGeneratedWrappedProperty&) = default;
	FGeneratedWrappedProperty& operator=(FGeneratedWrappedProperty&&) = default;
	FGeneratedWrappedProperty& operator=(const FGeneratedWrappedProperty&) = default;

	/** Set the property, optionally also calculating some data about it */
	void SetProperty(const FProperty* InProp, const uint32 InSetPropFlags = SPF_All);

	/** The Unreal property to access */
	const FProperty* Prop;

	/** Set if this property is deprecated and using it should emit a deprecation warning */
	TOptional<FString> DeprecationMessage;
};

/** Stores the data needed by a runtime generated Python get/set */
struct UNREALPYTHON_API FGeneratedWrappedGetSet
{
	FGeneratedWrappedGetSet()
		: GetCallback(nullptr)
		, SetCallback(nullptr)
	{
	}

	FGeneratedWrappedGetSet(FGeneratedWrappedGetSet&&) = default;
	FGeneratedWrappedGetSet(const FGeneratedWrappedGetSet&) = default;
	FGeneratedWrappedGetSet& operator=(FGeneratedWrappedGetSet&&) = default;
	FGeneratedWrappedGetSet& operator=(const FGeneratedWrappedGetSet&) = default;

	/** Convert this wrapper type to its Python type */
	void ToPython(PyGetSetDef& OutPyGetSet) const;

	/** The name of the get/set */
	FUTF8Buffer GetSetName;

	/** The doc string of the get/set */
	FUTF8Buffer GetSetDoc;

	/** The Unreal property for this get/set */
	FGeneratedWrappedProperty Prop;

	/** The Unreal function for the get (if any) */
	FGeneratedWrappedFunction GetFunc;

	/** The Unreal function for the set (if any) */
	FGeneratedWrappedFunction SetFunc;

	/* The C function that should be called for get */
	getter GetCallback;

	/* The C function that should be called for set */
	setter SetCallback;
};

/** Stores the data needed for a set of runtime generated Python get/sets */
struct FGeneratedWrappedGetSets
{
	FGeneratedWrappedGetSets() = default;
	FGeneratedWrappedGetSets(FGeneratedWrappedGetSets&&) = default;
	FGeneratedWrappedGetSets(const FGeneratedWrappedGetSets&) = delete;
	FGeneratedWrappedGetSets& operator=(FGeneratedWrappedGetSets&&) = default;
	FGeneratedWrappedGetSets& operator=(const FGeneratedWrappedGetSets&) = delete;

	/** Called to ready the generated get/sets with Python */
	void Finalize();

	/** Array of get/sets from the wrapped type */
	TArray<FGeneratedWrappedGetSet> TypeGetSets;

	/** The Python get/sets that were generated from TypeGetSets (in Finalize) */
	TArray<PyGetSetDef> PyGetSets;
};

/** Stores the data needed by a runtime generated Python constant */
struct FGeneratedWrappedConstant
{
	FGeneratedWrappedConstant() = default;
	FGeneratedWrappedConstant(FGeneratedWrappedConstant&&) = default;
	FGeneratedWrappedConstant(const FGeneratedWrappedConstant&) = default;
	FGeneratedWrappedConstant& operator=(FGeneratedWrappedConstant&&) = default;
	FGeneratedWrappedConstant& operator=(const FGeneratedWrappedConstant&) = default;

	/** Convert this wrapper type to its Python type */
	void ToPython(FUPyConstantDef& OutPyConstant) const;

	/** The name of the constant */
	FUTF8Buffer ConstantName;

	/** The doc string of the constant */
	FUTF8Buffer ConstantDoc;

	/** The Unreal function for getting the constant data */
	FGeneratedWrappedFunction ConstantFunc;
};

/** Stores the data needed for a set of runtime generated Python constants */
struct FGeneratedWrappedConstants
{
	FGeneratedWrappedConstants() = default;
	FGeneratedWrappedConstants(FGeneratedWrappedConstants&&) = default;
	FGeneratedWrappedConstants(const FGeneratedWrappedConstants&) = delete;
	FGeneratedWrappedConstants& operator=(FGeneratedWrappedConstants&&) = default;
	FGeneratedWrappedConstants& operator=(const FGeneratedWrappedConstants&) = delete;

	/** Called to ready the generated constants with Python */
	void Finalize();

	/** Array of constants from the wrapped type */
	TArray<FGeneratedWrappedConstant> TypeConstants;

	/** The Python constants that were generated from TypeConstants (in Finalize) */
	TArray<FUPyConstantDef> PyConstants;
};

/** Stores the data needed by a runtime generated Python constant that is dynamically created and registered post-finalize of its owner type (for hoisting constant functions onto other types) */
struct FGeneratedWrappedDynamicConstantWithClosure : public FGeneratedWrappedConstant
{
	FGeneratedWrappedDynamicConstantWithClosure() = default;
	FGeneratedWrappedDynamicConstantWithClosure(FGeneratedWrappedDynamicConstantWithClosure&&) = default;
	FGeneratedWrappedDynamicConstantWithClosure(const FGeneratedWrappedDynamicConstantWithClosure&) = delete;
	FGeneratedWrappedDynamicConstantWithClosure& operator=(FGeneratedWrappedDynamicConstantWithClosure&&) = default;
	FGeneratedWrappedDynamicConstantWithClosure& operator=(const FGeneratedWrappedDynamicConstantWithClosure&) = delete;

	/** Called to ready the generated constant for Python */
	void Finalize();

	/** Python constant that was generated from this data */
	FUPyConstantDef PyConstant;
};

/** Mixin used to add dynamic constant support to a type (base for common non-templated functionality) */
struct FGeneratedWrappedDynamicConstantsMixinBase
{
	FGeneratedWrappedDynamicConstantsMixinBase() = default;
	FGeneratedWrappedDynamicConstantsMixinBase(FGeneratedWrappedDynamicConstantsMixinBase&&) = default;
	FGeneratedWrappedDynamicConstantsMixinBase(const FGeneratedWrappedDynamicConstantsMixinBase&) = delete;
	FGeneratedWrappedDynamicConstantsMixinBase& operator=(FGeneratedWrappedDynamicConstantsMixinBase&&) = default;
	FGeneratedWrappedDynamicConstantsMixinBase& operator=(const FGeneratedWrappedDynamicConstantsMixinBase&) = delete;

	/** Called to add a dynamic constant to the Python type (this should only be called post-finalize) */
	void AddDynamicConstantImpl(FGeneratedWrappedConstant&& InDynamicConstant, PyTypeObject* InPyType);

	/** Array of dynamic constants associated with this type (call AddDynamicConstant to add constants) */
	TArray<TSharedRef<FGeneratedWrappedDynamicConstantWithClosure>> DynamicConstants;
};

/** Mixin used to add dynamic constant support to a type */
template <typename MixedIntoType>
struct TGeneratedWrappedDynamicConstantsMixin : public FGeneratedWrappedDynamicConstantsMixinBase
{
	TGeneratedWrappedDynamicConstantsMixin() = default;
	TGeneratedWrappedDynamicConstantsMixin(TGeneratedWrappedDynamicConstantsMixin&&) = default;
	TGeneratedWrappedDynamicConstantsMixin(const TGeneratedWrappedDynamicConstantsMixin&) = delete;
	TGeneratedWrappedDynamicConstantsMixin& operator=(TGeneratedWrappedDynamicConstantsMixin&&) = default;
	TGeneratedWrappedDynamicConstantsMixin& operator=(const TGeneratedWrappedDynamicConstantsMixin&) = delete;

	/** Called to add a dynamic constant to the Python type (this should only be called post-finalize) */
	void AddDynamicConstant(FGeneratedWrappedConstant&& InDynamicConstant)
	{
		AddDynamicConstantImpl(MoveTemp(InDynamicConstant), &static_cast<MixedIntoType*>(this)->PyType);
	}
};

/** Stores the data needed to generate a Python doc string for editor exposed properties */
struct FGeneratedWrappedPropertyDoc
{
	explicit FGeneratedWrappedPropertyDoc(const FProperty* InProp);

	FGeneratedWrappedPropertyDoc(FGeneratedWrappedPropertyDoc&&) = default;
	FGeneratedWrappedPropertyDoc(const FGeneratedWrappedPropertyDoc&) = default;
	FGeneratedWrappedPropertyDoc& operator=(FGeneratedWrappedPropertyDoc&&) = default;
	FGeneratedWrappedPropertyDoc& operator=(const FGeneratedWrappedPropertyDoc&) = default;

	/** Util function to sort an array of doc structs based on the Pythonized property name */
	static bool SortPredicate(const FGeneratedWrappedPropertyDoc& InOne, const FGeneratedWrappedPropertyDoc& InTwo);

	/** Util function to convert an array of doc structs into a combined doc string (the array should have been sorted before calling this) */
	static FString BuildDocString(const TArray<FGeneratedWrappedPropertyDoc>& InDocs);

	/** Util function to convert an array of doc structs into a combined doc string (the array should have been sorted before calling this) */
	static void AppendDocString(const TArray<FGeneratedWrappedPropertyDoc>& InDocs, FString& OutStr);

	/** Pythonized name of the property */
	FString PythonPropName;

	/** Pythonized doc string of the property */
	FString DocString;

	/** Pythonized editor doc string of the property */
	FString EditorDocString;
};

/** Utility for tracking and logging field conflicts when exported to Python */
struct FGeneratedWrappedFieldTracker
{
	FGeneratedWrappedFieldTracker() = default;
	FGeneratedWrappedFieldTracker(FGeneratedWrappedFieldTracker&&) = default;
	FGeneratedWrappedFieldTracker(const FGeneratedWrappedFieldTracker&) = default;
	FGeneratedWrappedFieldTracker& operator=(FGeneratedWrappedFieldTracker&&) = default;
	FGeneratedWrappedFieldTracker& operator=(const FGeneratedWrappedFieldTracker&) = default;

	/** Register a Python field name, and detect if a name conflict has occurred */
	void RegisterPythonFieldName(const FString& InPythonFieldName, const FFieldVariant& InUnrealField);

	/** Map from the Python wrapped field name to the Unreal field it was generated from (for conflict detection) */
	typedef TMap<FString, TWeakObjectPtr<const UField>, FDefaultSetAllocator, FCaseSensitiveStringMapFuncs<TWeakObjectPtr<const UField>>> FCaseSensitiveStringToFieldMap;
	typedef TMap<FString, TWeakFieldPtr<const FField>, FDefaultSetAllocator, FCaseSensitiveStringMapFuncs<TWeakFieldPtr<const FField>>> FCaseSensitiveStringToFFieldMap;
	FCaseSensitiveStringToFieldMap PythonWrappedFieldNameToUnrealField;
	FCaseSensitiveStringToFFieldMap PythonWrappedFieldNameToFField;
};

/** Stores the minimal data needed by a runtime generated Python type */
struct FGeneratedWrappedType
{
	enum class EFinalizedState : uint8
	{
		Initial,
		Reset,
		Finalized,
	};

	FGeneratedWrappedType()
	{
		PyType = { PyVarObject_HEAD_INIT(nullptr, 0) };
	}

	virtual ~FGeneratedWrappedType() = default;

	FGeneratedWrappedType(FGeneratedWrappedType&&) = delete;
	FGeneratedWrappedType(const FGeneratedWrappedType&) = delete;
	FGeneratedWrappedType& operator=(FGeneratedWrappedType&&) = delete;
	FGeneratedWrappedType& operator=(const FGeneratedWrappedType&) = delete;
	
	/** Called to ready the generated type with Python */
	bool Finalize();

	/** Called to reset this type back to its clean state */
	void Reset();

	/** Internal version of Finalize, called before readying the type with Python */
	virtual void Finalize_PreReady();

	/** Internal version of Finalize, called after readying the type with Python */
	virtual void Finalize_PostReady();

	/** Internal version of Reset, called to remove data added to the Python type */
	virtual void Reset_CleansePyType();

	/** Internal version of Reset, called to reset the data on this type */
	virtual void Reset_CleanseSelf();

	/** The name of the type */
	FUTF8Buffer TypeName;

	/** The doc string of the type */
	FUTF8Buffer TypeDoc;

	/** The meta-data associated with this type */
	TSharedPtr<FUPyWrapperBaseMetaData> MetaData;

	/* The Python type that was generated */
	PyTypeObject PyType;

	/** What is the current finalization state of this generated type? */
	EFinalizedState FinalizedState = EFinalizedState::Initial;

	/* Per-object lock used during the construction phase */
	// struct FDebuggableMutex
	// {
	// 	// We should never go above 1 lock per stack because
	// 	// going recursive on those lock will create a very deadlock
	// 	// prone situation when run under multiple thread that will
	// 	// be hard to repro depending on the order in which things
	// 	// are recursed into.
	// 	static int32& GetThreadLocal()
	// 	{
	// 		static thread_local int32 StackLockCount = 0;
	// 		return StackLockCount;
	// 	}
	//
	// 	UE::FMutex Mutex;
	// 	uint32     OwnerThreadId = 0;
	//
	// 	void Lock()
	// 	{
	// 		check(GetThreadLocal() == 0);
	// 		Mutex.Lock();
	// 		GetThreadLocal()++;
	// 		check(OwnerThreadId == 0);
	// 		OwnerThreadId = FPlatformTLS::GetCurrentThreadId();
	// 	}
	//
	// 	void Unlock()
	// 	{
	// 		OwnerThreadId = 0;
	// 		check(GetThreadLocal() == 1);
	// 		GetThreadLocal()--;
	// 		Mutex.Unlock();
	// 	}
	// } Lock;

	/* Used to atomically publish readyness of this object */
	// std::atomic<bool> bIsReadyForUse;
};

/** Definition data for an Unreal property generated from a Python type */
struct FPropertyDef
{
	FPropertyDef() = default;
	FPropertyDef(FPropertyDef&&) = default;
	FPropertyDef(const FPropertyDef&) = delete;
	FPropertyDef& operator=(FPropertyDef&&) = default;
	FPropertyDef& operator=(const FPropertyDef&) = delete;

	/** Data needed to re-wrap this property for Python */
	FGeneratedWrappedGetSet GeneratedWrappedGetSet;

	/** Definition of the re-wrapped property for Python */
	PyGetSetDef PyGetSet;
};

/** Definition data for an Unreal function generated from a Python type */
struct FFunctionDef
{
	FFunctionDef() = default;
	FFunctionDef(FFunctionDef&&) = default;
	FFunctionDef(const FFunctionDef&) = delete;
	FFunctionDef& operator=(FFunctionDef&&) = default;
	FFunctionDef& operator=(const FFunctionDef&) = delete;

	/** Data needed to re-wrap this function for Python */
	FGeneratedWrappedMethod GeneratedWrappedMethod;

	/** Definition of the re-wrapped function for Python */
	FUPyMethodWithClosureDef PyMethod;

	/** Python function to call when invoking this re-wrapped function */
	FUPyObjectPtr PyFunction;

	/** Is this a function hidden from Python? (eg, internal getter/setter function) */
	bool bIsHidden = false;
};
}
