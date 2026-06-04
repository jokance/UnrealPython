
#pragma once

#include "Wrapper/UPyWrapperBase.h"
#include "Utils/UPyUtil.h"
#include "DynamicTypes/UPyGeneratedWrappedType.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "UPyGeneratedClass.generated.h"

extern PyTypeObject UPyUClassDecoratorType;
extern PyTypeObject UPyUFunctionDecoratorType;

void InitializeUPyUClassDecorator(UPyGenUtil::FNativePythonModule& ModuleInfo);
void InitializeUPyUFunctionDecorator(UPyGenUtil::FNativePythonModule& ModuleInfo);

PyObject* PyCallGenerateClass(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds);
PyObject* PyCallGenerateFunction(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds);

struct FUPyUClassDefinitionOptions
{
	PyObject* MetaData = nullptr;
	bool bHasBlueprintType = false;
	bool bBlueprintType = true;
	bool bHasNotBlueprintType = false;
	bool bNotBlueprintType = false;
	bool bHasBlueprintable = false;
	bool bBlueprintable = false;
	bool bHasNotBlueprintable = false;
	bool bNotBlueprintable = false;
	bool bHasAbstract = false;
	bool bAbstract = false;
};

/** An Unreal class that was generated from a Python type */
UCLASS(Transient)
class UUPyGeneratedClass final : public UBlueprintGeneratedClass, public IUPythonResourceOwner
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

	//~ UClass interface
	virtual void PostInitInstance(UObject* InObj, FObjectInstancingGraph* InstanceGraph) override;

	//~ UBlueprintGeneratedClass interface
	virtual void GetLifetimeBlueprintReplicationList(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//~ IPythonResourceOwner interface
	virtual void ReleasePythonResources() override;

	/** Unregister this type from FPyWrapperTypeRegistry */
	void UnregisterGeneratedType();

	virtual bool IsFunctionImplementedInScript(FName InFunctionName) const override;

	bool GetGeneratedPropertyReplicationInfo(FName InPropertyName, ELifetimeCondition& OutReplicationCondition, ELifetimeRepNotifyCondition& OutRepNotifyCondition, bool& bOutPushBased) const;

	/** Generate an Unreal class from the given Python type */
	static UUPyGeneratedClass* GenerateClass(PyTypeObject* InPyType, const FUPyUClassDefinitionOptions* InOptions = nullptr);

	/** Generate an Unreal class for all child classes of the old parent using the new parent class as their base (also update the Python types) */
	static bool ReparentDerivedClasses(UUPyGeneratedClass* InOldParent, UUPyGeneratedClass* InNewParent);

	/** Generate an Unreal class based on the given class, but using the given parent class (also update the Python type) */
	static UUPyGeneratedClass* ReparentClass(UUPyGeneratedClass* InOldClass, UUPyGeneratedClass* InNewParent);

private:
	/** Native function used to call the Python functions from C code */
	DECLARE_FUNCTION(CallPythonFunction);

	/** Python type this class was generated from */
	FUPyTypeObjectPtr PyType;

	/** PostInit function for this class */
	FUPyObjectPtr PyPostInitFunction;

	/** Array of properties generated for this class */
	TArray<TSharedPtr<UPyGenUtil::FPropertyDef>> PropertyDefs;

	struct FReplicationDef
	{
		FName PropertyName;
		ELifetimeCondition ReplicationCondition = COND_None;
		ELifetimeRepNotifyCondition RepNotifyCondition = REPNOTIFY_OnChanged;
		bool bPushBased = false;
	};

	/** Array of replication settings generated for this class */
	TArray<FReplicationDef> ReplicationDefs;

	/** Array of functions generated for this class */
	TArray<TSharedPtr<UPyGenUtil::FFunctionDef>> FunctionDefs;

	/** Meta-data for this generated class that is applied to the Python type */
	// FUPyWrapperObjectMetaData PyMetaData;

	TObjectPtr<UObjectRedirector> ClassRedirector;
	TObjectPtr<UObjectRedirector> CDORedirector;

	friend class FUPythonGeneratedClassBuilder;
};

struct FUPyUClassDecorator
{
	/** Common Python Object */
	PyObject_HEAD

	FUPyUClassDefinitionOptions Options;

	static FUPyUClassDecorator* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds);
	
	static void Dealloc(FUPyUClassDecorator* InSelf);
	
	static int Init(FUPyUClassDecorator* InSelf, PyObject* InArgs, PyObject* InKwds);
	
	static void Deinit(FUPyUClassDecorator* InSelf);
	
	static PyObject* Call(FUPyUClassDecorator* InSelf, PyObject* InArgs, PyObject* InKwds);
};

struct FUPyUFunctionDecorator
{
	/** Common Python Object */
	PyObject_HEAD
	
	PyObject* CacheArgs;
	PyObject* CacheKwds;
	
	static FUPyUFunctionDecorator* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds);
	
	static void Dealloc(FUPyUFunctionDecorator* InSelf);
	
	static int Init(FUPyUFunctionDecorator* InSelf, PyObject* InArgs, PyObject* InKwds);
	
	static void Deinit(FUPyUFunctionDecorator* InSelf);
	
	static PyObject* Call(FUPyUFunctionDecorator* InSelf, PyObject* InArgs, PyObject* InKwds);
};
