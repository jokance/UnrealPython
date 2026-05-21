
#pragma once

#include "UPyCommon.h"
#include "Wrapper/UPyWrapperBase.h"
#include "Core/UPyPtr.h"
#include "Wrapper/UPyWrapperOwnerContext.h"
#include "UObject/WeakObjectPtr.h"
#include "DynamicTypes/UPyGeneratedWrappedType.h"
#include "UPyWrapperDelegate.generated.h"

/**
 * UObject proxy base used to wrap a callable Python object so that it can be used with an Unreal delegate
 * @note This can't go inside the WITH_PYTHON block due to UHT parsing limitations (it doesn't understand that macro)
 */
UCLASS(HideDropDown)
class UUPyCallableForDelegate : public UObject, public IUPythonResourceOwner
{
	GENERATED_BODY()

public:
	/** Name given to the generated function that we should bind to the Unreal delegate */
	static const FName GeneratedFuncName;

	/** Native function implementation used by the signature correct Unreal functions added to the derived classes (the ones that are bound to the delegate itself) */
	DECLARE_FUNCTION(CallPythonNative);

public:
	//~ UObject interface
	virtual void BeginDestroy() override;

	//~ IPythonResourceOwner interface
	virtual void ReleasePythonResources() override;

	/** Get the Python callable object on this instance (borrowed reference) */
	PyObject* GetCallable() const;

	/** Set the Python callable object on this instance */
	void SetCallable(PyObject* InCallable, const UFunction* InDelegateSignature);

	void Deinit();
	
	UFUNCTION()
	void CallPython();
	
	virtual void ProcessEvent(UFunction* InFunc, void* InBuffer) override;
	
private:
	/** The callable Python callable object this object wraps (if any) */
	FUPyObjectPtr PyCallable;
	
	const UFunction* DelegateSignature;
};

/** Python type for FUPyWrapperDelegate */
extern PyTypeObject UPyWrapperDelegateType;

/** Python type for FUPyWrapperMulticastDelegate */
extern PyTypeObject UPyWrapperMulticastDelegateType;

/** Initialize the PyWrapperDelegate types and add them to the given Python module */
void InitializeUPyWrapperDelegate(UPyGenUtil::FNativePythonModule& ModuleInfo);

/** Base type for all Unreal exposed delegate instances */
template <typename DelegateType, typename PropType>
struct TUPyWrapperDelegate : public FUPyWrapperBase
{
	/** The owner of the wrapped delegate instance (if any) */
	FUPyWrapperOwnerContext OwnerContext;

	/** Wrapped delegate instance */
	DelegateType* DelegateInstance;

	void* PropAddr;

	/** Internal delegate instance (DelegateInstance is set to this when we own the instance) */
	// DelegateType InternalDelegateInstance;
	
	const PropType* DelegateProp;
};

/** Base meta-data for all Unreal exposed delegate types */
// template <typename WrapperType>
// struct TUPyWrapperDelegateMetaData : public FUPyWrapperBaseMetaData
// {
// 	UPY_OVERRIDE_GETSET_METADATA(TUPyWrapperDelegateMetaData)
//
// 	TUPyWrapperDelegateMetaData()
// 		: PythonCallableForDelegateClass(nullptr)
// 	{
// 	}
//
// 	/** Get the delegate signature from the given type */
// 	static UPyGenUtil::FGeneratedWrappedFunction& GetDelegateSignature(PyTypeObject* PyType)
// 	{
// 		TUPyWrapperDelegateMetaData* PyWrapperMetaData = TUPyWrapperDelegateMetaData::GetMetaData(PyType);
// 		static UPyGenUtil::FGeneratedWrappedFunction NullDelegateSignature = UPyGenUtil::FGeneratedWrappedFunction();
// 		return PyWrapperMetaData ? PyWrapperMetaData->DelegateSignature : NullDelegateSignature;
// 	}
//
// 	/** Get the delegate signature from the type of the given instance */
// 	static UPyGenUtil::FGeneratedWrappedFunction& GetDelegateSignature(WrapperType* Instance)
// 	{
// 		return GetDelegateSignature(Py_TYPE(Instance));
// 	}
//
// 	/** Get the generated class type used to wrap Python callables for this delegate type */
// 	static const UClass* GetPythonCallableForDelegateClass(PyTypeObject* PyType)
// 	{
// 		TUPyWrapperDelegateMetaData* PyWrapperMetaData = TUPyWrapperDelegateMetaData::GetMetaData(PyType);
// 		return PyWrapperMetaData ? PyWrapperMetaData->PythonCallableForDelegateClass : nullptr;
// 	}
//
// 	/** Get the generated class type used to wrap Python callables for this delegate type */
// 	static const UClass* GetPythonCallableForDelegateClass(WrapperType* Instance)
// 	{
// 		return GetPythonCallableForDelegateClass(Py_TYPE(Instance));
// 	}
//
// 	/** Add object references from this type meta-data to the given collector */
// 	virtual void AddTypeReferencedObjects(FReferenceCollector& Collector) override
// 	{
// 		Collector.AddReferencedObject(DelegateSignature.Func);
// 		Collector.AddReferencedObject(PythonCallableForDelegateClass);
// 	}
//
// 	/** Get the reflection meta data type object associated with this wrapper type if there is one or nullptr if not. */
// 	virtual const UField* GetMetaType() const override
// 	{
// 		return DelegateSignature.Func;
// 	}
//
// 	/** Unreal function representing the signature for the delegate */
// 	UPyGenUtil::FGeneratedWrappedFunction DelegateSignature;
//
// 	/** Generated class type used to wrap Python callables for this delegate type */
// 	TObjectPtr<const UClass> PythonCallableForDelegateClass;
// };

/** Type for all Unreal exposed delegate instances */
struct FUPyWrapperDelegate : public TUPyWrapperDelegate<FScriptDelegate, FDelegateProperty>
{
	/** New this wrapper instance (called via tp_new for Python, or directly in C++) */
	static FUPyWrapperDelegate* New(PyTypeObject* InType);

	/** Free this wrapper instance (called via tp_dealloc for Python) */
	static void Free(FUPyWrapperDelegate* InSelf);

	/** Initialize this wrapper instance (called via tp_init for Python, or directly in C++) */
	static int Init(FUPyWrapperDelegate* InSelf);

	/** Initialize this wrapper instance to the given value (called via tp_init for Python, or directly in C++) */
	static int Init(FUPyWrapperDelegate* InSelf, const FUPyWrapperOwnerContext& InOwnerContext, FScriptDelegate* InValue, const FDelegateProperty* InProp, const EUPyConversionMethod InConversionMethod);

	/** Deinitialize this wrapper instance (called via Init and Free to restore the instance to its New state) */
	static void Deinit(FUPyWrapperDelegate* InSelf);

	/** Called to validate the internal state of this wrapper instance prior to operating on it (should be called by all functions that expect to operate on an initialized type; will set an error state on failure) */
	static bool ValidateInternalState(FUPyWrapperDelegate* InSelf);

	/** Cast the given Python object to this wrapped type (returns a new reference) */
	static FUPyWrapperDelegate* CastPyObject(PyObject* InPyObject, FUPyConversionResult* OutCastResult = nullptr);

	/** Cast the given Python object to this wrapped type, or attempt to convert the type into a new wrapped instance (returns a new reference) */
	static FUPyWrapperDelegate* CastPyObject(PyObject* InPyObject, const PyTypeObject* InType, FUPyConversionResult* OutCastResult = nullptr);

	/** Call the delegate */
	static PyObject* CallDelegate(FUPyWrapperDelegate* InSelf, PyObject* InArgs);

	static bool Unbind(FUPyWrapperDelegate* InSelf);
};

/** Meta-data for all Unreal exposed delegate types */
// struct FUPyWrapperDelegateMetaData : public TUPyWrapperDelegateMetaData<FUPyWrapperDelegate>
// {
// 	UPY_METADATA_METHODS(FUPyWrapperDelegateMetaData, FGuid(0xCB3D0485, 0x8A3A443E, 0xBEE336F4, 0x82888A81))
//
// 	/** Add object references from the given Python object to the given collector */
// 	virtual void AddInstanceReferencedObjects(FUPyWrapperBase* Instance, FReferenceCollector& Collector) override;
// };

/** Type for all Unreal exposed multicast delegate instances */
struct FUPyWrapperMulticastDelegate : public TUPyWrapperDelegate<FMulticastScriptDelegate, FMulticastDelegateProperty>
{
	/** New this wrapper instance (called via tp_new for Python, or directly in C++) */
	static FUPyWrapperMulticastDelegate* New(PyTypeObject* InType);

	/** Free this wrapper instance (called via tp_dealloc for Python) */
	static void Free(FUPyWrapperMulticastDelegate* InSelf);

	/** Initialize this wrapper instance (called via tp_init for Python, or directly in C++) */
	static int Init(FUPyWrapperMulticastDelegate* InSelf);

	/** Initialize this wrapper instance to the given value (called via tp_init for Python, or directly in C++) */
	static int Init(FUPyWrapperMulticastDelegate* InSelf, const FUPyWrapperOwnerContext& InOwnerContext, FMulticastScriptDelegate* InValue, void* InPropAddr, const FMulticastDelegateProperty* InProp, const EUPyConversionMethod InConversionMethod);

	/** Deinitialize this wrapper instance (called via Init and Free to restore the instance to its New state) */
	static void Deinit(FUPyWrapperMulticastDelegate* InSelf);

	/** Called to validate the internal state of this wrapper instance prior to operating on it (should be called by all functions that expect to operate on an initialized type; will set an error state on failure) */
	static bool ValidateInternalState(FUPyWrapperMulticastDelegate* InSelf);

	/** Cast the given Python object to this wrapped type (returns a new reference) */
	static FUPyWrapperMulticastDelegate* CastPyObject(PyObject* InPyObject, FUPyConversionResult* OutCastResult = nullptr);

	/** Cast the given Python object to this wrapped type, or attempt to convert the type into a new wrapped instance (returns a new reference) */
	static FUPyWrapperMulticastDelegate* CastPyObject(PyObject* InPyObject, const PyTypeObject* InType, FUPyConversionResult* OutCastResult = nullptr);

	/** Call the delegate */
	static PyObject* CallDelegate(FUPyWrapperMulticastDelegate* InSelf, PyObject* InArgs);
	
	static bool Unbind(FUPyWrapperMulticastDelegate* InSelf);
};

/** Meta-data for all Unreal exposed multicast delegate types */
// struct FUPyWrapperMulticastDelegateMetaData : public TUPyWrapperDelegateMetaData<FUPyWrapperMulticastDelegate>
// {
// 	UPY_METADATA_METHODS(FUPyWrapperMulticastDelegateMetaData, FGuid(0x448FB4DA, 0x38DC4386, 0xBCAFF448, 0x29C0F3A4))
//
// 	/** Add object references from the given Python object to the given collector */
// 	virtual void AddInstanceReferencedObjects(FUPyWrapperBase* Instance, FReferenceCollector& Collector) override;
// };

typedef TUPyPtr<FUPyWrapperDelegate> FUPyWrapperDelegatePtr;
typedef TUPyPtr<FUPyWrapperMulticastDelegate> FUPyWrapperMulticastDelegatePtr;
