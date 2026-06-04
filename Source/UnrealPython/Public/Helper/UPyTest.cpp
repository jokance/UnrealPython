
#include "UPyTest.h"
#include "UPyCommon.h"
#include "Helper/UPyBlueprintLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UPyTest)

int32 FUPyTestStruct::ConstantValue = 100;
int32 UUPyTestObject::OtherConstantValue = 1000;

FUPyTestStruct::FUPyTestStruct()
{
	Bool = false;
	Int = 0;
	Float = 0.0f;
	Enum = EUPyTestEnum::One;
	LegacyInt_DEPRECATED = 0;
	BoolInstanceOnly = false;
	BoolDefaultsOnly = false;
}

bool FUPyTestStruct::IsBoolSet()
{
	return Bool;
}

bool FUPyTestStruct::LegacyIsBoolSet()
{
	return IsBoolSet();
}

FUPyTestStruct FUPyTestStruct::AddInt(int32 InValue)
{
	FUPyTestStruct Result = *this;
	Result.Int += InValue;
	return Result;
}

FUPyTestStruct FUPyTestStruct::AddFloat(float InValue)
{
	FUPyTestStruct Result = *this;
	Result.Float += InValue;
	return Result;
}

FUPyTestStruct FUPyTestStruct::AddStr(const FString& InValue)
{
	FUPyTestStruct Result = *this;
	Result.String += InValue;
	return Result;
}

FUPyTestStruct FUPyTestStruct::operator+(int32 InValue) const
{
	FUPyTestStruct Result = *this;
	Result.Int += InValue;
	return Result;
}

FUPyTestStruct FUPyTestStruct::operator+(float InValue) const
{
	FUPyTestStruct Result = *this;
	Result.Float += InValue;
	return Result;
}

FUPyTestStruct FUPyTestStruct::operator+(const FString& InValue) const
{
	FUPyTestStruct Result = *this;
	Result.String += InValue;
	return Result;
}

FUPyTestStruct& FUPyTestStruct::operator+=(int32 InValue)
{
	Int += InValue;
	return *this;
}

FUPyTestStruct& FUPyTestStruct::operator+=(float InValue)
{
	Float += InValue;
	return *this;
}

FUPyTestStruct& FUPyTestStruct::operator+=(const FString& InValue)
{
	String += InValue;
	return *this;
}

void FUPyTestStruct::SetBoolMutable()
{
	BoolMutable = true;
}

void FUPyTestStruct::ClearBoolMutable()
{
	BoolMutable = false;
}

void FUPyTestStruct::SetBoolMutableViaRef()
{
	BoolMutable = true;
}

void FUPyTestStruct::ClearBoolMutableViaRef()
{
	BoolMutable = false;
}

bool UUPyTestStructLibrary::IsBoolSet(const FUPyTestStruct& InStruct)
{
	return InStruct.Bool;
}

bool UUPyTestStructLibrary::LegacyIsBoolSet(const FUPyTestStruct& InStruct)
{
	return IsBoolSet(InStruct);
}

int32 UUPyTestStructLibrary::GetConstantValue()
{
	return 10;
}

FUPyTestStruct UUPyTestStructLibrary::AddInt(const FUPyTestStruct& InStruct, const int32 InValue)
{
	FUPyTestStruct Result = InStruct;
	Result.Int += InValue;
	return Result;
}

FUPyTestStruct UUPyTestStructLibrary::AddFloat(const FUPyTestStruct& InStruct, const float InValue)
{
	FUPyTestStruct Result = InStruct;
	Result.Float += InValue;
	return Result;
}

FUPyTestStruct UUPyTestStructLibrary::AddStr(const FUPyTestStruct& InStruct, const FString& InValue)
{
	FUPyTestStruct Result = InStruct;
	Result.String += InValue;
	return Result;
}

void UUPyTestStructLibrary::SetBoolMutable(const FUPyTestStruct& InStruct)
{
	InStruct.BoolMutable = true;
}

void UUPyTestStructLibrary::ClearBoolMutable(const FUPyTestStruct& InStruct)
{
	InStruct.BoolMutable = false;
}

void UUPyTestStructLibrary::SetBoolMutableViaRef(FUPyTestStruct& InStruct)
{
	InStruct.BoolMutable = true;
}

void UUPyTestStructLibrary::ClearBoolMutableViaRef(FUPyTestStruct& InStruct)
{
	InStruct.BoolMutable = false;
}

void UUPyTestStructLibrary::UPyTestStruct_Init1(const FUPyTestStruct& InStruct, const TArray<FString>& InStringArray,
	const TMap<FString, int32>& InIntStringMap, const TSet<FString>& InStringSet)
{
}

void UUPyTestStructLibrary::UPyTestStruct_Init2(const FUPyTestStruct& InStruct, EUPyTestEnum InEnum, EUPyTestEnum InOtherEnum)
{
}

void UUPyTestStructLibrary::UPyTestStruct_Init3(const FUPyTestStruct& InStruct)
{
}

UUPyTestStructLibrary::UUPyTestStructLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UUPyTestObjectLibrary::UUPyTestObjectLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UUPyTestInterface::UUPyTestInterface(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UUPyTestChildInterface::UUPyTestChildInterface(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UUPyTestOtherInterface::UUPyTestOtherInterface(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UUPyTestObject::UUPyTestObject()
{
	StructArray.AddDefaulted();
	StructArray.AddDefaulted();
}

int32 UUPyTestObject::FuncBlueprintNative_Implementation(const int32 InValue) const
{
	return InValue;
}

void UUPyTestObject::FuncBlueprintNativeRef_Implementation(FUPyTestStruct& InOutStruct) const
{
	InOutStruct.Int += 100;
}

int32 UUPyTestObject::CallFuncBlueprintImplementable(const int32 InValue) const
{
	return FuncBlueprintImplementable(InValue);
}

bool UUPyTestObject::CallFuncBlueprintImplementablePackedGetter(int32& OutValue) const
{
	return FuncBlueprintImplementablePackedGetter(OutValue);
}

int32 UUPyTestObject::CallFuncBlueprintNative(const int32 InValue) const
{
	return FuncBlueprintNative(InValue);
}

void UUPyTestObject::CallFuncBlueprintNativeRef(FUPyTestStruct& InOutStruct) const
{
	return FuncBlueprintNativeRef(InOutStruct);
}

void UUPyTestObject::FuncTakingPyTestStruct(const FUPyTestStruct& InStruct) const
{
}

void UUPyTestObject::FuncTakingPyTestChildStruct(const FUPyTestChildStruct& InStruct) const
{
}

void UUPyTestObject::LegacyFuncTakingPyTestStruct(const FUPyTestStruct& InStruct) const
{
	FuncTakingPyTestStruct(InStruct);
}

void UUPyTestObject::FuncTakingPyTestStructDefault(const FUPyTestStruct& InStruct)
{
}

int32 UUPyTestObject::FuncTakingPyTestDelegate(const FUPyTestDelegate& InDelegate, const int32 InValue) const
{
	return InDelegate.IsBound() ? InDelegate.Execute(InValue) : INDEX_NONE;
}

void UUPyTestObject::FuncTakingFieldPath(const TFieldPath<FProperty>& InFieldPath)
{
	FieldPath = InFieldPath;
}

int32 UUPyTestObject::DelegatePropertyCallback(const int32 InValue) const
{
	if (InValue != Int)
	{
		UE_LOG(LogUnrealPython, Error, TEXT("Given value (%d) did not match the Int property value (%d)"), InValue, Int);
	}

	return InValue;
}

void UUPyTestObject::MulticastDelegatePropertyCallback(FString InStr) const
{
	if (InStr != String)
	{
		UE_LOG(LogUnrealPython, Error, TEXT("Given value (%s) did not match the String property value (%s)"), *InStr, *String);
	}
}

TArray<int32> UUPyTestObject::ReturnArray()
{
	TArray<int32> TmpArray;
	TmpArray.Add(10);
	return TmpArray;
}

TSet<int32> UUPyTestObject::ReturnSet()
{
	TSet<int32> TmpSet;
	TmpSet.Add(10);
	return TmpSet;
}

TMap<int32, bool> UUPyTestObject::ReturnMap()
{
	TMap<int32, bool> TmpMap;
	TmpMap.Add(10, true);
	return TmpMap;
}

TFieldPath<FProperty> UUPyTestObject::ReturnFieldPath()
{
	return TFieldPath<FProperty>(UUPyTestObject::StaticClass()->FindPropertyByName(TEXT("FieldPath")));
}

void UUPyTestObject::EmitScriptError()
{
	FFrame::KismetExecutionMessage(TEXT("EmitScriptError was called"), ELogVerbosity::Error);
}

void UUPyTestObject::EmitScriptWarning()
{
	FFrame::KismetExecutionMessage(TEXT("EmitScriptWarning was called"), ELogVerbosity::Warning);
}

int32 UUPyTestObject::GetConstantValue()
{
	return 10;
}

void UUPyTestObject::TestDefaultArgs(float Arg1, int32 Arg2, FString Arg3, FName Arg4)
{
}

bool UUPyTestObject::CheckBoolParam(bool bValue) const
{
	return bValue;
}

uint8 UUPyTestObject::CheckUInt8Param(uint8 Value) const
{
	return Value;
}

int64 UUPyTestObject::CheckInt64Param(int64 Value) const
{
	return Value;
}

double UUPyTestObject::CheckDoubleParam(double Value) const
{
	return Value;
}

FName UUPyTestObject::CheckNameParam(FName Value) const
{
	return Value;
}

FText UUPyTestObject::CheckTextParam(FText Value) const
{
	return Value;
}

FVector UUPyTestObject::CheckVectorParam(FVector Value) const
{
	return Value;
}

UObject* UUPyTestObject::CheckObjectParam(UObject* Value) const
{
	return Value;
}

// TSubclassOf<UObject> UUPyTestObject::CheckClassParam(TSubclassOf<UObject> Value) const
// {
// 	return Value;
// }

TArray<int32> UUPyTestObject::CheckArrayParam(const TArray<int32>& Value) const
{
	return Value;
}

bool UUPyTestObject::CheckBlueprintLibraryArrayCall() const
{
	const FString ModuleName = TEXT("upy_test.upy_blueprint_library_array_target");

	const TArray<int32> IntInput = {1, 2, 3};
	const TArray<int32> IntExpected = {2, 4, 6};
	const TArray<int32> IntTemplateResult = UUPyBlueprintLibrary::CallPythonMethod<TArray<int32>>(ModuleName, TEXT("double_ints"), IntInput);
	const TArray<int32> IntBlueprintResult = UUPyBlueprintLibrary::CallPythonMethod_IntArray_RetIntArray(ModuleName, TEXT("double_ints"), IntInput);

	const TArray<FString> StrInput = {TEXT("red"), TEXT("blue")};
	const TArray<FString> StrExpected = {TEXT("red!"), TEXT("blue!")};
	const TArray<FString> StrTemplateResult = UUPyBlueprintLibrary::CallPythonMethod<TArray<FString>>(ModuleName, TEXT("suffix_strings"), StrInput);
	const TArray<FString> StrBlueprintResult = UUPyBlueprintLibrary::CallPythonMethod_StrArray_RetStrArray(ModuleName, TEXT("suffix_strings"), StrInput);

	const TArray<FString> IntToStrExpected = {TEXT("n=1"), TEXT("n=2"), TEXT("n=3")};
	const TArray<FString> IntToStrResult = UUPyBlueprintLibrary::CallPythonMethod_IntArray_RetStrArray(ModuleName, TEXT("ints_to_strings"), IntInput);

	const TArray<int32> StrToIntExpected = {3, 4};
	const TArray<int32> StrToIntResult = UUPyBlueprintLibrary::CallPythonMethod_StrArray_RetIntArray(ModuleName, TEXT("strings_to_lengths"), StrInput);

	return IntTemplateResult == IntExpected
		&& IntBlueprintResult == IntExpected
		&& StrTemplateResult == StrExpected
		&& StrBlueprintResult == StrExpected
		&& IntToStrResult == IntToStrExpected
		&& StrToIntResult == StrToIntExpected;
}

TSet<FString> UUPyTestObject::CheckSetParam(const TSet<FString>& Value) const
{
	return Value;
}

TMap<FString, int32> UUPyTestObject::CheckMapParam(const TMap<FString, int32>& Value) const
{
	return Value;
}

int32 UUPyTestObject::FuncInterface(const int32 InValue) const
{
	return InValue;
}

int32 UUPyTestObject::FuncInterfaceChild(const int32 InValue) const
{
	return InValue;
}

int32 UUPyTestObject::FuncInterfaceOther(const int32 InValue) const
{
	return InValue;
}

bool UUPyTestObjectLibrary::IsBoolSet(const UUPyTestObject* InObj)
{
	return InObj->Bool;
}

int32 UUPyTestObjectLibrary::GetOtherConstantValue()
{
	return 20;
}

FString UUPyTestTypeHint::GetStringConst()
{
	return FString("Foo");
}

int32 UUPyTestTypeHint::GetIntConst()
{
	return 777;
}


UUPyTestTypeHint::UUPyTestTypeHint()
{
	ObjectProp = NewObject<UUPyTestObject>();
}

UUPyTestTypeHint::UUPyTestTypeHint(bool bParam1, int32 Param2, float Param3, const FString& Param4, const FText& Param5)
	: BoolProp(bParam1)
	, IntProp(Param2)
	, FloatProp(Param3)
	, StringProp(Param4)
	, TextProp(Param5)
{
}

bool UUPyTestTypeHint::CheckBoolTypeHints(bool bParam1, bool bParam2, bool bParam3)
{
	return true;
}

int32 UUPyTestTypeHint::CheckIntegerTypeHints(uint8 Param1, int32 Param2, int64 Param3)
{
	return 0;
}

double UUPyTestTypeHint::CheckFloatTypeHints(float Param1, double Param2, float Param3, double Param4)
{
	return 0.0;
}

EUPyTestEnum UUPyTestTypeHint::CheckEnumTypeHints(EUPyTestEnum Param1, EUPyTestEnum Param2)
{
	return EUPyTestEnum::One;
}

FString UUPyTestTypeHint::CheckStringTypeHints(const FString& Param1, const FString& Param2)
{
	return FString();
}

FName UUPyTestTypeHint::CheckNameTypeHints(const FName& Param1, const FName& Param2)
{
	return FName();
}

FText UUPyTestTypeHint::CheckTextTypeHints(const FText& Param1, const FText& Param2)
{
	return FText::GetEmpty();
}

TFieldPath<FProperty> UUPyTestTypeHint::CheckFieldPathTypeHints(const TFieldPath<FProperty> Param1)
{
	return TFieldPath<FProperty>();
}

FUPyTestStruct UUPyTestTypeHint::CheckStructTypeHints(const FUPyTestStruct& Param1, const FUPyTestStruct& Param2)
{
	return FUPyTestStruct();
}

UUPyTestObject* UUPyTestTypeHint::CheckObjectTypeHints(const UUPyTestObject* Param1, const UUPyTestObject* Param3)
{
	return nullptr;
}

TArray<FText> UUPyTestTypeHint::CheckArrayTypeHints(const TArray<FString>& Param1, const TArray<FName>& Param2, const TArray<FText>& Param3, const TArray<UObject*>& Param4)
{
	return TArray<FText>();
}

TArray<FText> UUPyTestTypeHint::CheckArrayTypeHintsMultiReturn(const TArray<FString>& Param1,
	const TArray<FName>& Param2, const TArray<FText>& Param3, const TArray<UObject*>& Param4, FString& OutStr)
{
	return TArray<FText>();
}

TSet<FName> UUPyTestTypeHint::CheckSetTypeHints(const TSet<FString>& Param1, const TSet<FName>& Param2, const TSet<UObject*>& Param4)
{
	return TSet<FName>();
}

TSet<FName> UUPyTestTypeHint::CheckSetTypeHintsMultiReturn(const TSet<FString>& Param1, const TSet<FName>& Param2,
	const TSet<UObject*>& Param3, FString& OutStr)
{
	return TSet<FName>();
}

TMap<FString, UObject*> UUPyTestTypeHint::CheckMapTypeHints(const TMap<int, FString>& Param1, const TMap<int, FName>& Param2, const TMap<int, FText>& Param3, const TMap<int, UObject*>& Param4)
{
	return TMap<FString, UObject*>();
}

TMap<FString, UObject*> UUPyTestTypeHint::CheckMapTypeHintsMultiReturn(const TMap<int, FString>& Param1,
	const TMap<int, FName>& Param2, const TMap<int, FText>& Param3, const TMap<int, UObject*>& Param4, FString& OutStr)
{
	return TMap<FString, UObject*>();
}

FUPyTestDelegate& UUPyTestTypeHint::CheckDelegateTypeHints(const FUPyTestDelegate& Param1)
{
	return DelegateProp;
}

bool UUPyTestTypeHint::CheckStaticFunction(bool Param1, int32 Param2, double Param3, const FString& Param4)
{
	return true;
}

int UUPyTestTypeHint::CheckTupleReturnType(UPARAM(ref) FString& InOutString)
{
	InOutString = TEXT("Foo");
	return 0;
}
