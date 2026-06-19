
#pragma once

#include "Wrapper/UPyWrapperBase.h"
#include "DynamicTypes/UPyGeneratedWrappedType.h"
#include "UPyGeneratedStruct.generated.h"

extern PyTypeObject UPyUStructDecoratorType;

void InitializeUPyUStructDecorator(UPyGenUtil::FNativePythonModule& ModuleInfo);

PyObject* PyCallGenerateStruct(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds);

struct FUPyUStructDefinitionOptions
{
	PyObject* MetaData = nullptr;
	bool bHasBlueprintType = false;
	bool bBlueprintType = true;
	bool bHasNotBlueprintType = false;
	bool bNotBlueprintType = false;
};

/** An Unreal struct that was generated from a Python type */
UCLASS()
class UUPyGeneratedStruct final : public UScriptStruct, public IUPythonResourceOwner
{
	GENERATED_BODY()

public:
	//~ UObject interface
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	virtual void PostRename(UObject* OldOuter, const FName OldName) override;
	virtual void BeginDestroy() override;
	virtual bool IsAsset() const override
	{
		return false;
	}

	//~ UStruct interface
	virtual void InitializeStruct(void* Dest, int32 ArrayDim = 1) const override;

	//~ IPythonResourceOwner interface
	virtual void ReleasePythonResources() override;

	/** Unregister this type from FPyWrapperTypeRegistry */
	void UnregisterGeneratedType();

	/** Generate an Unreal struct from the given Python type */
	static UUPyGeneratedStruct* GenerateStruct(PyTypeObject* InPyType, const FUPyUStructDefinitionOptions* InOptions = nullptr);

private:
	/** Python type this struct was generated from */
	FUPyTypeObjectPtr PyType;

	/** PostInit function for this struct */
	FUPyObjectPtr PyPostInitFunction;

	/** Array of properties generated for this struct */
	TArray<TSharedPtr<UPyGenUtil::FPropertyDef>> PropertyDefs;

	/** Meta-data for this generated struct that is applied to the Python type */
	// FPyWrapperStructMetaData PyMetaData;

	TObjectPtr<UObjectRedirector> StructRedirector;

	friend class FUPythonGeneratedStructBuilder;
};

struct FUPyUStructDecorator
{
	/** Common Python Object */
	PyObject_HEAD

	FUPyUStructDefinitionOptions Options;

	static FUPyUStructDecorator* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds);
	
	static void Dealloc(FUPyUStructDecorator* InSelf);
	
	static int Init(FUPyUStructDecorator* InSelf, PyObject* InArgs, PyObject* InKwds);
	
	static void Deinit(FUPyUStructDecorator* InSelf);

	static int GCTraverse(FUPyUStructDecorator* InSelf, visitproc InVisit, void* InArg);

	static int GCClear(FUPyUStructDecorator* InSelf);

	static PyObject* Call(FUPyUStructDecorator* InSelf, PyObject* InArgs, PyObject* InKwds);
};
