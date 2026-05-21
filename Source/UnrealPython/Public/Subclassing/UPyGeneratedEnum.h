
#pragma once

#include "Wrapper/UPyWrapperBase.h"
#include "UPyGeneratedEnum.generated.h"

extern PyTypeObject UPyUEnumDecoratorType;

void InitializeUPyUEnumDecorator(UPyGenUtil::FNativePythonModule& ModuleInfo);

PyObject* PyCallGenerateEnum(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds);

/** An Unreal enum that was generated from a Python type */
UCLASS()
class UUPyGeneratedEnum final : public UEnum, public IUPythonResourceOwner
{
	GENERATED_BODY()

public:
	//~ UObject interface
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	virtual void BeginDestroy() override;
	virtual bool IsAsset() const override
	{
		return false;
	}

	//~ IPythonResourceOwner interface
	virtual void ReleasePythonResources() override;

	/** Unregister this type from FPyWrapperTypeRegistry */
	void UnregisterGeneratedType();

	/** Generate an Unreal enum from the given Python type */
	static UUPyGeneratedEnum* GenerateEnum(PyTypeObject* InPyType);

private:
	/** Definition data for an Unreal enum value generated from a Python type */
	struct FEnumValueDef
	{
		/** Index of the enum value as it was registered with us from the Python */
		int32 PyIndex = 0;

		/** Numeric value of the enum value */
		int64 Value = 0;

		/** Name of the enum value */
		FString Name;

		/** Documentation string of the enum value */
		FString DocString;
	};

	/** Python type this enum was generated from */
	FUPyTypeObjectPtr PyType;

	/** Array of values generated for this enum */
	TArray<TSharedPtr<FEnumValueDef>> EnumValueDefs;

	/** Meta-data for this generated enum that is applied to the Python type */
	// FUPyWrapperEnumMetaData PyMetaData;

	TObjectPtr<UObjectRedirector> EnumRedirector;

	friend class FUPythonGeneratedEnumBuilder;
};

struct FUPyUEnumDecorator
{
	/** Common Python Object */
	PyObject_HEAD
	
	static FUPyUEnumDecorator* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds);
	
	static void Dealloc(FUPyUEnumDecorator* InSelf);
	
	static int Init(FUPyUEnumDecorator* InSelf, PyObject* InArgs, PyObject* InKwds);
	
	static void Deinit(FUPyUEnumDecorator* InSelf);
	
	static PyObject* Call(FUPyUEnumDecorator* InSelf, PyObject* InArgs, PyObject* InKwds);
};
