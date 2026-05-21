
#pragma once

#include "Helper/UPyTestInterface.h"
#include "UPyTest.generated.h"

/**
 * Delegate to allow testing of the various script delegate features that are exposed to Python wrapped types.
 */
DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(int32, FUPyTestDelegate, int32, InValue);

/**
 * Multicast delegate to allow testing of the various script delegate features that are exposed to Python wrapped types.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUPyTestMulticastDelegate, FString, InStr);

/**
 * Delegate for slate pre/post tick event.
 */
DECLARE_DYNAMIC_DELEGATE_OneParam(FUPyTestSlateTickDelegate, float, InDeltaTime);

/**
 * Enum to allow testing of the various UEnum features that are exposed to Python wrapped types.
 */
UENUM(BlueprintType)
enum class EUPyTestEnum : uint8
{
	One UMETA(DisplayName = "Says One but my value is Zero"),
	Two,
};

/**
 * Simple struct to be used as a property inside FUPyTestStruct.
 */
USTRUCT(BlueprintType)
struct FUPyTestInnerStruct
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	int32 IntValue = 0;
};

/**
 * Struct to allow testing of the various UStruct features that are exposed to Python wrapped types.
 */
USTRUCT(BlueprintType)
struct FUPyTestStruct
{
	GENERATED_BODY()

public:
	FUPyTestStruct();
	FUPyTestStruct(bool InBool, int32 InInt)
		: Bool(InBool)
		, Int(InInt)
	{
	}

	FUPyTestStruct(const TArray<FString>& InStringArray, const TMap<FString, int32>& InStringIntMap, const TSet<FString>& InStringSet)
		: StringArray(InStringArray)
		, StringSet(InStringSet)
		, StringIntMap(InStringIntMap)
	{
	}

	FUPyTestStruct(EUPyTestEnum InEnum, EUPyTestEnum InOtherEnum = EUPyTestEnum::Two)
		: Enum(InEnum)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	bool Bool;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	int32 Int;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	float Float;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	EUPyTestEnum Enum;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	FString String;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	FText Text;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	TFieldPath<FProperty> FieldPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	TFieldPath<FStructProperty> StructFieldPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	TArray<FString> StringArray;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	TSet<FString> StringSet;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	TMap<FString, int32> StringIntMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	FUPyTestInnerStruct InnerStruct;

	UPROPERTY(meta=(DeprecatedProperty, DeprecationMessage="LegacyInt is deprecated. Please use Int instead."))
	int32 LegacyInt_DEPRECATED;

	UPROPERTY(EditInstanceOnly, Category = "Python|Internal")
	bool BoolInstanceOnly;

	UPROPERTY(EditDefaultsOnly, Category = "Python|Internal")
	bool BoolDefaultsOnly;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Python|Internal")
	mutable bool BoolMutable = false;

	static int32 ConstantValue;

	bool IsBoolSet();
	bool LegacyIsBoolSet();
	FUPyTestStruct AddInt(int32 InValue);
	FUPyTestStruct AddFloat(float InValue);
	FUPyTestStruct AddStr(const FString& InValue);

	FUPyTestStruct operator+(int32 InValue) const;
	FUPyTestStruct operator+(float InValue) const;
	FUPyTestStruct operator+(const FString& InValue) const;

	FUPyTestStruct& operator+=(int32 InValue);
	FUPyTestStruct& operator+=(float InValue);
	FUPyTestStruct& operator+=(const FString& InValue);

	void SetBoolMutable();
	void ClearBoolMutable();
	void SetBoolMutableViaRef();
	void ClearBoolMutableViaRef();

	bool operator==(const FUPyTestStruct& Other) const
	{
		return Int == Other.Int && String == Other.String;
	}

	friend uint32 GetTypeHash(const FUPyTestStruct& Thing)
	{
		return HashCombine(GetTypeHash(Thing.Int), GetTypeHash(Thing.String));
	}
};

/**
 * Struct to allow testing of inheritance on Python wrapped types.
 */
USTRUCT(BlueprintType)
struct FUPyTestChildStruct : public FUPyTestStruct
{
	GENERATED_BODY()
};

/**
 * Function library containing methods that should be hoisted onto the test struct in Python.
 */
UCLASS()
class UUPyTestStructLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	UFUNCTION(BlueprintPure, Category = "Python|Internal", meta=(UPyScriptMethod="IsBoolSet"))
	static bool IsBoolSet(const FUPyTestStruct& InStruct);

	UFUNCTION(BlueprintPure, Category = "Python|Internal", meta=(UPyScriptMethod, DeprecatedFunction, DeprecationMessage="LegacyIsBoolSet is deprecated. Please use IsBoolSet instead."))
	static bool LegacyIsBoolSet(const FUPyTestStruct& InStruct);

	UFUNCTION(BlueprintPure, Category = "Python|Internal", meta=(UPyScriptConstant="ConstantValue", UPyScriptHost="/Script/UnrealPython.UPyTestStruct"))
	static int32 GetConstantValue();

	UFUNCTION(BlueprintCallable, Category = "Python|Internal", meta=(UPyScriptMethod, UPyScriptMethodSelfReturn, UPyScriptOperator="+;+="))
	static FUPyTestStruct AddInt(const FUPyTestStruct& InStruct, const int32 InValue);

	UFUNCTION(BlueprintCallable, Category = "Python|Internal", meta=(UPyScriptMethod, UPyScriptMethodSelfReturn, UPyScriptOperator="+;+="))
	static FUPyTestStruct AddFloat(const FUPyTestStruct& InStruct, const float InValue);

	UFUNCTION(BlueprintCallable, Category = "Python|Internal", meta=(UPyScriptMethod, UPyScriptMethodSelfReturn, UPyScriptOperator="+;+="))
	static FUPyTestStruct AddStr(const FUPyTestStruct& InStruct, const FString& InValue);

	UFUNCTION(BlueprintCallable, Category = "Python|Internal", meta=(UPyScriptMethod, UPyScriptMethodMutable))
	static void SetBoolMutable(const FUPyTestStruct& InStruct);

	UFUNCTION(BlueprintCallable, Category = "Python|Internal", meta=(UPyScriptMethod, UPyScriptMethodMutable))
	static void ClearBoolMutable(const FUPyTestStruct& InStruct);

	UFUNCTION(BlueprintCallable, Category = "Python|Internal", meta=(UPyScriptMethod, UPyScriptMethodMutable))
	static void SetBoolMutableViaRef(UPARAM(ref) FUPyTestStruct& InStruct);

	UFUNCTION(BlueprintCallable, Category = "Python|Internal", meta=(UPyScriptMethod, UPyScriptMethodMutable))
	static void ClearBoolMutableViaRef(UPARAM(ref) FUPyTestStruct& InStruct);

	UFUNCTION(BlueprintCallable, Category = "Python|Internal", meta=(UPyScriptMethod="__init__"))
	static void UPyTestStruct_Init1 (const FUPyTestStruct& InStruct, const TArray<FString>& InStringArray, const TMap<FString, int32>& InIntStringMap, const TSet<FString>& InStringSet);

	UFUNCTION(BlueprintCallable, Category = "Python|Internal", meta=(UPyScriptMethod="__init__"))
	static void UPyTestStruct_Init2 (const FUPyTestStruct& InStruct, EUPyTestEnum InEnum, EUPyTestEnum InOtherEnum = EUPyTestEnum::Two);

	UFUNCTION(BlueprintCallable, Category = "Python|Internal", meta=(UPyScriptMethod="__init__"))
	static void UPyTestStruct_Init3 (const FUPyTestStruct& InStruct);
};

/**
 * Struct to allow testing of class sparse data on a Python exposed type.
 */
USTRUCT(BlueprintType)
struct FUPyTestClassSparseData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Python|Internal")
	int32 IntFromSparseData = 0;
};

/**
 * Object to allow testing of the various UObject features that are exposed to Python wrapped types.
 */
UCLASS(Blueprintable, SparseClassDataTypes = UPyTestClassSparseData)
class UUPyTestObject : public UObject, public IUPyTestChildInterface, public IUPyTestOtherInterface
{
	GENERATED_BODY()

public:
	UUPyTestObject();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	bool Bool;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	int32 Int;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	float Float;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	EUPyTestEnum Enum;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	FString String;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	FText Text;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	TFieldPath<FProperty> FieldPath = TFieldPath<FProperty>(FUPyTestStruct::StaticStruct()->FindPropertyByName(TEXT("StringArray")));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	TFieldPath<FStructProperty> StructFieldPath = TFieldPath<FStructProperty>(CastField<FStructProperty>(UUPyTestObject::GetClass()->FindPropertyByName(TEXT("Struct"))));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	TArray<FString> StringArray;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	TSet<FString> StringSet;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	TMap<FString, int32> StringIntMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	FUPyTestDelegate Delegate;

	UPROPERTY(EditAnywhere, BlueprintAssignable, Category = "Python|Internal")
	FUPyTestMulticastDelegate MulticastDelegate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	FUPyTestStruct Struct;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	TArray<FUPyTestStruct> StructArray;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	TSet<FUPyTestStruct> StructSet;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	TMap<int32, FUPyTestStruct> StructMap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	FUPyTestChildStruct ChildStruct;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	FVector Vector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	int64 Int64;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	double Double;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	UObject* Object;

	// todo(hzn): 为实现软引用的支持，暂时注释掉以下属性
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	// TSoftObjectPtr<UUPyTestObject> SoftObject;
	//
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	// TSoftClassPtr<UUPyTestObject> SoftClass;
	//
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	// TSubclassOf<UUPyTestObject> Subclass;

	UPROPERTY(EditInstanceOnly, Category = "Python|Internal")
	bool BoolInstanceOnly;

	UPROPERTY(EditDefaultsOnly, Category = "Python|Internal")
	bool BoolDefaultsOnly;

	UFUNCTION(BlueprintImplementableEvent, Category = "Python|Internal")
	int32 FuncBlueprintImplementable(const int32 InValue) const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Python|Internal")
	bool FuncBlueprintImplementablePackedGetter(int32& OutValue) const;

	UFUNCTION(BlueprintNativeEvent, Category = "Python|Internal")
	int32 FuncBlueprintNative(const int32 InValue) const;

	UFUNCTION(BlueprintNativeEvent, Category = "Python|Internal")
	void FuncBlueprintNativeRef(UPARAM(ref) FUPyTestStruct& InOutStruct) const;

	UFUNCTION(BlueprintCallable, Category = "Python|Internal")
	int32 CallFuncBlueprintImplementable(const int32 InValue) const;

	UFUNCTION(BlueprintCallable, Category = "Python|Internal")
	bool CallFuncBlueprintImplementablePackedGetter(int32& OutValue) const;

	UFUNCTION(BlueprintCallable, Category = "Python|Internal")
	int32 CallFuncBlueprintNative(const int32 InValue) const;

	UFUNCTION(BlueprintCallable, Category = "Python|Internal")
	void CallFuncBlueprintNativeRef(UPARAM(ref) FUPyTestStruct& InOutStruct) const;

	UFUNCTION(BlueprintCallable, Category = "Python|Internal")
	void FuncTakingPyTestStruct(const FUPyTestStruct& InStruct) const;

	UFUNCTION(BlueprintCallable, Category = "Python|Internal")
	void FuncTakingPyTestChildStruct(const FUPyTestChildStruct& InStruct) const;

	UFUNCTION(BlueprintCallable, Category = "Python|Internal", meta=(DeprecatedFunction, DeprecationMessage="LegacyFuncTakingPyTestStruct is deprecated. Please use FuncTakingPyTestStruct instead."))
	void LegacyFuncTakingPyTestStruct(const FUPyTestStruct& InStruct) const;

	UFUNCTION(BlueprintCallable, Category = "Python|Internal")
	void FuncTakingPyTestStructDefault(const FUPyTestStruct& InStruct = FUPyTestStruct());

	UFUNCTION(BlueprintCallable, Category = "Python|Internal")
	int32 FuncTakingPyTestDelegate(const FUPyTestDelegate& InDelegate, const int32 InValue) const;

	UFUNCTION(BlueprintCallable, Category = "Python|Internal")
	void FuncTakingFieldPath(const TFieldPath<FProperty>& InFieldPath); // UHT couldn't parse any default value for the FieldPath.

	UFUNCTION(BlueprintCallable, Category = "Python|Internal")
	int32 DelegatePropertyCallback(const int32 InValue) const;

	UFUNCTION(BlueprintCallable, Category = "Python|Internal")
	void MulticastDelegatePropertyCallback(FString InStr) const;

	UFUNCTION(BlueprintCallable, Category = "Python|Internal")
	static TArray<int32> ReturnArray();

	UFUNCTION(BlueprintCallable, Category = "Python|Internal")
	static TSet<int32> ReturnSet();

	UFUNCTION(BlueprintCallable, Category = "Python|Internal")
	static TMap<int32, bool> ReturnMap();

	UFUNCTION(BlueprintCallable, Category = "Python|Internal")
	static TFieldPath<FProperty> ReturnFieldPath();

	UFUNCTION(BlueprintCallable, Category = "Python|Internal")
	static void EmitScriptError();

	UFUNCTION(BlueprintCallable, Category = "Python|Internal")
	static void EmitScriptWarning();

	UFUNCTION(BlueprintPure, Category = "Python|Internal", meta=(UPyScriptConstant="ConstantValue"))
	static int32 GetConstantValue();

	UFUNCTION(BlueprintCallable, Category = "Python|Internal")
	void TestDefaultArgs(float Arg1, int32 Arg2 = 42, FString Arg3 = TEXT("Hello"), FName Arg4 = NAME_None);

	// Parameter Type Testing
	UFUNCTION(BlueprintCallable, Category = "Python|Internal")
	bool CheckBoolParam(bool bValue) const;

	UFUNCTION(BlueprintCallable, Category = "Python|Internal")
	uint8 CheckUInt8Param(uint8 Value) const;

	UFUNCTION(BlueprintCallable, Category = "Python|Internal")
	int64 CheckInt64Param(int64 Value) const;

	UFUNCTION(BlueprintCallable, Category = "Python|Internal")
	double CheckDoubleParam(double Value) const;

	UFUNCTION(BlueprintCallable, Category = "Python|Internal")
	FName CheckNameParam(FName Value) const;

	UFUNCTION(BlueprintCallable, Category = "Python|Internal")
	FText CheckTextParam(FText Value) const;

	UFUNCTION(BlueprintCallable, Category = "Python|Internal")
	FVector CheckVectorParam(FVector Value) const;

	UFUNCTION(BlueprintCallable, Category = "Python|Internal")
	UObject* CheckObjectParam(UObject* Value) const;

	// todo(hzn): 为实现软引用的支持，暂时注释掉以下方法
	// UFUNCTION(BlueprintCallable, Category = "Python|Internal")
	// TSubclassOf<UObject> CheckClassParam(TSubclassOf<UObject> Value) const;

	UFUNCTION(BlueprintCallable, Category = "Python|Internal")
	TArray<int32> CheckArrayParam(const TArray<int32>& Value) const;

	UFUNCTION(BlueprintCallable, Category = "Python|Internal")
	TSet<FString> CheckSetParam(const TSet<FString>& Value) const;

	UFUNCTION(BlueprintCallable, Category = "Python|Internal")
	TMap<FString, int32> CheckMapParam(const TMap<FString, int32>& Value) const;

	virtual int32 FuncInterface(const int32 InValue) const override;
	virtual int32 FuncInterfaceChild(const int32 InValue) const override;
	virtual int32 FuncInterfaceOther(const int32 InValue) const override;

	bool IsBoolSet() {return Bool;}

	static int32 OtherConstantValue;
};

/**
 * Object to allow testing of inheritance on Python wrapped types.
 */
UCLASS(Blueprintable)
class UUPyTestChildObject : public UUPyTestObject
{
	GENERATED_BODY()
};

/**
 * Object to test deprecation of Python wrapped types.
 */
UCLASS(Blueprintable, deprecated, meta=(DeprecationMessage="LegacyPyTestObject is deprecated. Please use PyTestObject instead."))
class UDEPRECATED_LegacyUPyTestObject : public UUPyTestObject
{
	GENERATED_BODY()
};

/**
 * Function library containing methods that should be hoisted onto the test object in Python.
 */
UCLASS()
class UUPyTestObjectLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	UFUNCTION(BlueprintPure, Category = "Python|Internal", meta=(UPyScriptMethod="IsBoolSet"))
	static bool IsBoolSet(const UUPyTestObject* InObj);

	UFUNCTION(BlueprintPure, Category = "Python|Internal", meta=(UPyScriptConstant="OtherConstantValue", UPyScriptHost="/Script/UnrealPython.UPyTestObject"))
	static int32 GetOtherConstantValue();
};

/**
 * This class along with UUPyTestVectorDelegate verify that 2 UObjects with the same delegate name/type, do not collide.
 */
UCLASS(Blueprintable)
class UUPyTestStructDelegate : public UObject
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUPyOnNameCollisionDelegate, const FUPyTestStruct&, PyStruct);

	/** Called when a new item is selected in the combobox. */
	UPROPERTY(BlueprintAssignable, Category = "Python|Internal")
	FUPyOnNameCollisionDelegate OnNameCollisionTestDelegate; // Same prop name and type name as UUPyTestVectorDelegate::FOnNameCollisionTestDelegate, but different params.
};

/**
 * This class along with UPyTestStructDelegate verify that 2 UObjects with the same delegate name/type, do not collide.
 */
UCLASS(Blueprintable)
class UUPyTestVectorDelegate : public UObject
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUPyOnNameCollisionDelegate, const FVector2D&, Vec);

	/** Called when a new item is selected in the combobox. */
	UPROPERTY(BlueprintAssignable, Category = "Python|Internal")
	FUPyOnNameCollisionDelegate OnNameCollisionTestDelegate; // Same prop name and type name as UPyTestStructDelegate::FOnNameCollisionTestDelegate, but different params.
};


/* Used to verify if the generated Python stub is correctly type-hinted (if type hint is enabled). The stub is generated
 * in the project intermediate folder when the Python developer mode is enabled (Editor preferences). The type hints can
 * be checked in the stub itself or PythonScriptPlugin/Content/Python/test_type_hints.py can be loaded in a Python IDE that
 * supports type checking and look at the code to verify that there is not problems with the types.
 */
UCLASS(Blueprintable)
class UUPyTestTypeHint : public UObject
{
	GENERATED_BODY()

public:
	//
	// Check type hinted init methods.
	//

	UUPyTestTypeHint();
	UUPyTestTypeHint(bool bParam1, int32 Param2, float Param3, const FString& Param4 = "Hi", const FText& Param5 = FText());

	//
	// Check type hinted constants
	//

	UFUNCTION(BlueprintPure, Category = "Python|Internal", meta=(ScriptConstant="StrConst"))
	static FString GetStringConst();

	UFUNCTION(BlueprintPure, Category = "Python|Internal", meta=(ScriptConstant="IntConst"))
	static int32 GetIntConst();

	//
	// Check type hinted properties (setter/getter)
	//

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	bool BoolProp = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	int32 IntProp = 32;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	float FloatProp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	EUPyTestEnum EnumProp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	FString StringProp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	FName NameProp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	FText TextProp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	TFieldPath<FProperty> FieldPathProp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	FUPyTestStruct StructProp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	TObjectPtr<UUPyTestObject> ObjectProp = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	TArray<FString> StrArrayProp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	TArray<FName> NameArrayProp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	TArray<FText> TextArrayProp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	TArray<TObjectPtr<UObject>> ObjectArrayProp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	TSet<FString> SetProp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	TMap<int32, FString> MapProp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	FUPyTestDelegate DelegateProp;

	UPROPERTY(EditAnywhere, BlueprintAssignable, Category = "Python|Internal")
	FUPyTestMulticastDelegate MulticastDelegateProp;

	//
	// Check type hinted methods.
	//

	UFUNCTION(BlueprintPure, Category = "Python|Internal")
	bool CheckBoolTypeHints(bool bParam1, bool bParam2 = true, bool bParam3 = false);

	UFUNCTION(BlueprintPure, Category = "Python|Internal")
	int32 CheckIntegerTypeHints(uint8 Param1, int32 Param2 = 4, int64 Param3 = 5);

	UFUNCTION(BlueprintPure, Category = "Python|Internal")
	double CheckFloatTypeHints(float Param1, double Param2, float Param3 = -3.3f, double Param4 = 4.4);

	UFUNCTION(BlueprintPure, Category = "Python|Internal")
	EUPyTestEnum CheckEnumTypeHints(EUPyTestEnum Param1, EUPyTestEnum Param2 = EUPyTestEnum::One);

	UFUNCTION(BlueprintPure, Category = "Python|Internal")
	FString CheckStringTypeHints(const FString& Param1, const FString& Param2 = TEXT("Hi"));

	UFUNCTION(BlueprintPure, Category = "Python|Internal")
	FName CheckNameTypeHints(const FName& Param1, const FName& Param2 = FName(TEXT("Hi")));

	UFUNCTION(BlueprintPure, Category = "Python|Internal")
	FText CheckTextTypeHints(const FText& Param1, const FText& Param2 = INVTEXT("Hi"));

	UFUNCTION(BlueprintPure, Category = "Python|Internal")
	TFieldPath<FProperty> CheckFieldPathTypeHints(const TFieldPath<FProperty> Param1);

	UFUNCTION(BlueprintPure, Category = "Python|Internal")
	FUPyTestStruct CheckStructTypeHints(const FUPyTestStruct& Param1, const FUPyTestStruct& Param2 = FUPyTestStruct());

	UFUNCTION(BlueprintPure, Category = "Python|Internal")
	UUPyTestObject* CheckObjectTypeHints(const UUPyTestObject* Param1, const UUPyTestObject* Param4 = nullptr);

	UFUNCTION(BlueprintPure, Category = "Python|Internal")
	TArray<FText> CheckArrayTypeHints(const TArray<FString>& Param1, const TArray<FName>& Param2, const TArray<FText>& Param3, const TArray<UObject*>& Param4);

	UFUNCTION(BlueprintPure, Category = "Python|Internal")
	TArray<FText> CheckArrayTypeHintsMultiReturn(const TArray<FString>& Param1, const TArray<FName>& Param2, const TArray<FText>& Param3, const TArray<UObject*>& Param4, FString& OutStr);

	UFUNCTION(BlueprintPure, Category = "Python|Internal")
	TSet<FName> CheckSetTypeHints(const TSet<FString>& Param1, const TSet<FName>& Param2, const TSet<UObject*>& Param3);

	UFUNCTION(BlueprintPure, Category = "Python|Internal")
	TSet<FName> CheckSetTypeHintsMultiReturn(const TSet<FString>& Param1, const TSet<FName>& Param2, const TSet<UObject*>& Param3, FString& OutStr);

	UFUNCTION(BlueprintPure, Category = "Python|Internal")
	TMap<FString, UObject*> CheckMapTypeHints(const TMap<int, FString>& Param1, const TMap<int, FName>& Param2, const TMap<int, FText>& Param3, const TMap<int, UObject*>& Param4);

	UFUNCTION(BlueprintPure, Category = "Python|Internal")
	TMap<FString, UObject*> CheckMapTypeHintsMultiReturn(const TMap<int, FString>& Param1, const TMap<int, FName>& Param2, const TMap<int, FText>& Param3, const TMap<int, UObject*>& Param4, FString& OutStr);

	UFUNCTION(BlueprintPure, Category = "Python|Internal")
	FUPyTestDelegate& CheckDelegateTypeHints(const FUPyTestDelegate& Param1);

	UFUNCTION(BlueprintPure, Category = "Python|Internal")
	static bool CheckStaticFunction(bool Param1, int32 Param2, double Param3, const FString& Param4);

	UFUNCTION(BlueprintPure, Category = "Python|Internal")
	static int CheckTupleReturnType(UPARAM(ref) FString& InOutString);

	//
	// Members to facilitate testing particular Python API.
	//

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Python|Internal")
	FUPyTestSlateTickDelegate SlateTickDelegate;
};
