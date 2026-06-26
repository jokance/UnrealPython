
#include "Wrapper/UPyWrapperDelegate.h"
#include "Wrapper/UPyWrapperObjectBase.h"
#include "Wrapper/UPyWrapperTypeRegistry.h"
#include "Wrapper/UPyWrapperTypeFactory.h"
#include "Core/UPyGIL.h"
#include "Utils/UPyUtil.h"
#include "Utils/UPyGenUtil.h"
#include "Utils/UPyDelegateUtil.h"
#include "Core/UPyConversion.h"
#include "UObject/Class.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"
#include "UObject/StructOnScope.h"
#include "Templates/Casts.h"
#include "Misc/EngineVersionComparison.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UPyWrapperDelegate)

extern PyMethodDef DelegatePyMethodDefs[];
extern PyMethodDef MulticastDelegatePyMethodDefs[];

const FName UUPyCallableForDelegate::GeneratedFuncName = "CallPython";

PyTypeObject UPyWrapperDelegateType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"DelegateBase", /* tp_name */
	sizeof(FUPyWrapperDelegate), /* tp_basicsize */
};

PyTypeObject UPyWrapperMulticastDelegateType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"MulticastDelegateBase", /* tp_name */
	sizeof(FUPyWrapperMulticastDelegate), /* tp_basicsize */
};

DEFINE_FUNCTION(UUPyCallableForDelegate::CallPythonNative)
{
	// Note: This function *must not* return until InvokePythonCallableFromUnrealFunctionThunk has been called, as we need to step over the correct amount of data from the bytecode stack!

	const UFunction* Func = Stack.CurrentNativeFunction;

	// Execute Python code within this block
	{
		FUPyScopedGIL GIL;
		if (!UPyGenUtil::InvokePythonCallableFromUnrealFunctionThunk(FUPyObjectPtr(), P_THIS->PyCallable, Func, Context, Stack, RESULT_PARAM))
		{
			UPyUtil::ReThrowPythonError();
		}
	}
}

void UUPyCallableForDelegate::BeginDestroy()
{
	ReleasePythonResources();
	Super::BeginDestroy();
}

void UUPyCallableForDelegate::ReleasePythonResources()
{
	// This may be called after Python has already shut down
	if (Py_IsInitialized())
	{
		FUPyScopedGIL GIL;
		PyCallable.Reset();
	}
	else
	{
		// Release ownership if Python has been shut down to avoid attempting to delete the object (which is already dead)
		PyCallable.Release();
	}
}

PyObject* UUPyCallableForDelegate::GetCallable() const
{
	return (PyObject*)PyCallable.GetPtr();
}

void UUPyCallableForDelegate::SetCallable(PyObject* InCallable, const UFunction* InDelegateSignature)
{
	FUPyScopedGIL GIL;
	PyCallable = FUPyObjectPtr::NewReference(InCallable);
	DelegateSignature = InDelegateSignature;

	AddToRoot();
}

void UUPyCallableForDelegate::Deinit()
{
	if (IsRooted())
	{
		RemoveFromRoot();
	}

	DelegateSignature = nullptr;
	if (PyCallable.IsValid())
	{
		ReleasePythonResources();
	}
}

void UUPyCallableForDelegate::CallPython()
{
}

void UUPyCallableForDelegate::ProcessEvent(UFunction* InFunc, void* InBuffer)
{
	if (!PyCallable.IsValid())
	{
		return;
	}

	FUPyScopedGIL GIL;

	const UFunction* FuncToUse = DelegateSignature ? DelegateSignature : InFunc;
	if (!FuncToUse)
	{
		return;
	}

	// Stores information about inputs and outputs
	FOutParmRec* OutParms = nullptr;
	FOutParmRec** LastOut = &OutParms;
	TArray<FUPyObjectPtr, TInlineAllocator<4>> PyParams;

	// Add any return property to the output params chain
	if (FProperty* ReturnProp = FuncToUse->GetReturnProperty())
	{
		FOutParmRec* Out = (FOutParmRec*)FMemory_Alloca(sizeof(FOutParmRec));
		Out->Property = ReturnProp;
		Out->PropAddr = ReturnProp->ContainerPtrToValuePtr<uint8>(InBuffer);

		// Link it to the head of the list, as UnpackReturnValues expects the return value to be first in the list
		Out->NextOutParm = OutParms;
		OutParms = Out;
	}

	bool bProcessedInputs = true;
	int32 ArgIndex = 0;

	for (TFieldIterator<FProperty> ParamIt(FuncToUse); ParamIt; ++ParamIt)
	{
		FProperty* Param = *ParamIt;

		// Skip the return value; it was added to the output params chain before this loop
		if (Param->HasAnyPropertyFlags(CPF_ReturnParm))
		{
			continue;
		}

		void* ValueAddress = Param->ContainerPtrToValuePtr<void>(InBuffer);

		// Add any output parameters to the output params chain
		if (UPyUtil::IsOutputParameter(Param))
		{
			CA_SUPPRESS(6263) // using _alloca in a loop
			FOutParmRec* Out = (FOutParmRec*)FMemory_Alloca(sizeof(FOutParmRec));
			Out->Property = Param;
			Out->PropAddr = (uint8*)ValueAddress;
			Out->NextOutParm = nullptr;

			// Link it to the end of the list
			if (*LastOut)
			{
				(*LastOut)->NextOutParm = Out;
				LastOut = &(*LastOut)->NextOutParm;
			}
			else
			{
				*LastOut = Out;
			}
		}

		// Convert any input parameters for use with Python
		if (UPyUtil::IsInputParameter(Param))
		{
			FUPyObjectPtr& PyParam = PyParams.AddDefaulted_GetRef();
			if (!UPyConversion::PythonizeProperty_Direct(Param, ValueAddress, PyParam.Get()))
			{
				UPyUtil::SetPythonError(PyExc_TypeError, PyCallable.Get(), *FString::Printf(TEXT("Failed to convert argument at pos '%d' when calling delegate '%s'"), ArgIndex + 1, *FuncToUse->GetName()));
				bProcessedInputs = false;
			}
			++ArgIndex;
		}
	}

	if (!bProcessedInputs)
	{
		return;
	}

	// Prepare the arguments tuple for the Python callable
	FUPyObjectPtr PyArgs;
	if (PyParams.Num() > 0)
	{
		PyArgs = FUPyObjectPtr::StealReference(PyTuple_New(PyParams.Num()));
		for (int32 PyParamIndex = 0; PyParamIndex < PyParams.Num(); ++PyParamIndex)
		{
			PyTuple_SetItem(PyArgs, PyParamIndex, PyParams[PyParamIndex].Release()); // SetItem steals the reference
		}
	}

	// Invoke the Python callable
	FUPyObjectPtr RetVals = FUPyObjectPtr::StealReference(PyObject_CallObject(PyCallable.Get(), PyArgs));
	if (!RetVals)
	{
		UPyUtil::ReThrowPythonError();
		return;
	}

	// Unpack any output values
	if (!UPyGenUtil::UnpackReturnValues(RetVals, OutParms, *UPyUtil::GetErrorContext(PyCallable.Get()), *FString::Printf(TEXT("delegate '%s'"), *FuncToUse->GetName())))
	{
		UPyUtil::ReThrowPythonError();
	}
}

template <typename DelegateType>
struct TPyDelegateInvocation
{
};

template <>
struct TPyDelegateInvocation<FScriptDelegate>
{
	static bool CanCall(const FScriptDelegate* Delegate)
	{
		return Delegate && Delegate->IsBound();
	}

	static void Call(const FScriptDelegate* Delegate, void* Params)
	{
		if (Delegate)
		{
			Delegate->ProcessDelegate<UObject>(Params);
		}
	}

	static const FScriptDelegate* Resolve(const FDelegateProperty* Prop, const void* PropAddr)
	{
		return Prop->GetPropertyValuePtr(PropAddr);
	}
};

template <>
struct TPyDelegateInvocation<FMulticastScriptDelegate>
{
	static bool CanCall(const FMulticastScriptDelegate* Delegate)
	{
		return Delegate && Delegate->IsBound();
	}

	static void Call(const FMulticastScriptDelegate* Delegate, void* Params)
	{
		if (Delegate)
		{
#if UE_VERSION_OLDER_THAN(5, 8, 0)
			Delegate->ProcessMulticastDelegate<UObject>(Params);
#else
			Delegate->ProcessDelegate<UObject>(Params);
#endif
		}
	}

	static const FMulticastScriptDelegate* Resolve(const FMulticastDelegateProperty* Prop, const void* PropAddr)
	{
		return Prop->GetMulticastDelegate(PropAddr);
	}
};

template <typename WrapperType, /*typename MetaDataType, */typename DelegateType, typename FactoryType, typename PropType>
struct TUPyWrapperDelegateImpl
{
	static WrapperType* New(PyTypeObject* InType)
	{
		WrapperType* Self = (WrapperType*)FUPyWrapperBase::New(InType);
		if (Self)
		{
			new(&Self->OwnerContext) FUPyWrapperOwnerContext();
			new(&Self->CallableProxies) TArray<TWeakObjectPtr<UUPyCallableForDelegate>>();
			Self->DelegateInstance = nullptr;
			Self->PropAddr = nullptr;
			Self->DelegateProp = nullptr;
			// new(&Self->InternalDelegateInstance) DelegateType();
		}
		return Self;
	}

	static void Free(WrapperType* InSelf)
	{
		Deinit(InSelf);

		InSelf->OwnerContext.~FUPyWrapperOwnerContext();
		InSelf->CallableProxies.~TArray<TWeakObjectPtr<UUPyCallableForDelegate>>();
		// InSelf->InternalDelegateInstance.~DelegateType();
		FUPyWrapperBase::Free(InSelf);
	}

	static int Init(WrapperType* InSelf)
	{
		Deinit(InSelf);

		const int BaseInit = FUPyWrapperBase::Init(InSelf);
		if (BaseInit != 0)
		{
			return BaseInit;
		}

		// InSelf->DelegateInstance = &InSelf->InternalDelegateInstance;

		// FactoryType::Get().MapInstance(InSelf->DelegateInstance, InSelf);
		return 0;
	}

	static int Init(WrapperType* InSelf, const FUPyWrapperOwnerContext& InOwnerContext, DelegateType* InValue, void* InPropAddr, const PropType* InProp, const EUPyConversionMethod InConversionMethod)
	{
		InOwnerContext.AssertValidConversionMethod(InConversionMethod);

		Deinit(InSelf);

		const int BaseInit = FUPyWrapperBase::Init(InSelf);
		if (BaseInit != 0)
		{
			return BaseInit;
		}

		// check(InValue);

		DelegateType* DelegateInstanceToUse = nullptr; // &InSelf->InternalDelegateInstance;
		switch (InConversionMethod)
		{
		case EUPyConversionMethod::Copy:
		case EUPyConversionMethod::Steal:
			checkf(0, TEXT("Can't copy Delegate"));
			// InSelf->InternalDelegateInstance = *InValue;
			break;

		case EUPyConversionMethod::Reference:
			DelegateInstanceToUse = InValue;
			if (DelegateInstanceToUse || (InProp && InPropAddr))
			{
				InOwnerContext.AddOwnedPyProp(InSelf);
			}
			break;

		default:
			checkf(false, TEXT("Unknown EUPyConversionMethod"));
			break;
		}

		// check(DelegateInstanceToUse);

		InSelf->OwnerContext = InOwnerContext;
		InSelf->DelegateInstance = DelegateInstanceToUse;
		InSelf->PropAddr = InPropAddr;
		InSelf->DelegateProp = InProp;

		if (InSelf->DelegateInstance && InOwnerContext.HasOwner())
		{
			FactoryType::Get().MapInstance(InSelf->DelegateInstance, InSelf);
		}
		return 0;
	}

	static void Deinit(WrapperType* InSelf)
	{
		if (InSelf->DelegateInstance || InSelf->DelegateProp)
		{
			WrapperType::Unbind(InSelf);
		}
		ReleaseTrackedCallableProxies(InSelf);

		if (InSelf->DelegateInstance)
		{
			FactoryType::Get().UnmapInstance(InSelf->DelegateInstance);
		}

		if (InSelf->OwnerContext.HasOwner())
		{
			InSelf->OwnerContext.RemoveOwnedPyProp(InSelf);
			InSelf->OwnerContext.Reset();
		}

		InSelf->DelegateInstance = nullptr;
		InSelf->PropAddr = nullptr;
		InSelf->DelegateProp = nullptr;

		// InSelf->InternalDelegateInstance.Clear();
	}

	static void TrackCallableProxy(WrapperType* InSelf, UUPyCallableForDelegate* InProxy)
	{
		if (InProxy)
		{
			InSelf->CallableProxies.AddUnique(InProxy);
		}
	}

	static void ReleaseTrackedCallableProxies(WrapperType* InSelf)
	{
		for (const TWeakObjectPtr<UUPyCallableForDelegate>& ProxyPtr : InSelf->CallableProxies)
		{
			if (UUPyCallableForDelegate* Proxy = ProxyPtr.Get())
			{
				Proxy->Deinit();
			}
		}
		InSelf->CallableProxies.Reset();
	}

	static bool ValidateInternalState(WrapperType* InSelf)
	{
		if (!InSelf->DelegateInstance)
		{
			// Allow null internal instance if we have a valid property (supports Sparse Delegates)
			if (InSelf->DelegateProp && InSelf->PropAddr)
			{
				return true;
			}

			UPyUtil::SetPythonError(PyExc_Exception, Py_TYPE(InSelf), TEXT("Internal Error - DelegateInstance is null!"));
			return false;
		}

		return true;
	}

	static const DelegateType* ResolveDelegateInstance(WrapperType* InSelf)
	{
		typedef TPyDelegateInvocation<DelegateType> FDelegateInvocation;

		const DelegateType* DelegateInstanceToUse = InSelf->DelegateInstance;
		if (!DelegateInstanceToUse && InSelf->DelegateProp && InSelf->PropAddr)
		{
			// For Sparse Delegates, DelegateInstance holds the Handle Address, not the Delegate itself.
			DelegateInstanceToUse = FDelegateInvocation::Resolve(InSelf->DelegateProp, InSelf->PropAddr);
		}

		return DelegateInstanceToUse;
	}

	static PyObject* CallDelegate(WrapperType* InSelf, PyObject* InArgs)
	{
		typedef TPyDelegateInvocation<DelegateType> FDelegateInvocation;

		if (!ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		const DelegateType* DelegateToCall = ResolveDelegateInstance(InSelf);
		if (!FDelegateInvocation::CanCall(DelegateToCall))
		{
			UPyUtil::SetPythonError(PyExc_Exception, InSelf, TEXT("Cannot call an unbound delegate"));
			return nullptr;
		}

		// todo(hzn): Add it to a Map to improve performance.
		UPyGenUtil::FGeneratedWrappedFunction DelegateSignature;
		DelegateSignature.SetFunction(InSelf->DelegateProp->SignatureFunction);
		// const UPyGenUtil::FGeneratedWrappedFunction& DelegateSignature = MetaDataType::GetDelegateSignature(InSelf);

		if (DelegateSignature.Func->ChildProperties == nullptr)
		{
			// Simple case, no parameters or return value
			FDelegateInvocation::Call(DelegateToCall, nullptr);
			Py_RETURN_NONE;
		}

		// Complex case, parameters or return value
		TArray<PyObject*> Params;
		if (!UPyGenUtil::ParseMethodParameters(InArgs, nullptr, DelegateSignature.InputParams, "delegate", Params))
		{
			return nullptr;
		}

		UPY_UFUNCTION_STACK(DelegateParams, DelegateSignature.Func);
		UPyGenUtil::ApplyParamDefaults(DelegateParams.GetMemory(), DelegateSignature.InputParams);
		for (int32 ParamIndex = 0; ParamIndex < Params.Num(); ++ParamIndex)
		{
			const UPyGenUtil::FGeneratedWrappedMethodParameter& ParamDef = DelegateSignature.InputParams[ParamIndex];

			PyObject* PyValue = Params[ParamIndex];
			if (PyValue)
			{
				if (!UPyConversion::NativizeProperty_InContainer(PyValue, ParamDef.ParamProp, DelegateParams.GetMemory(), 0))
				{
					UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert parameter '%s' when calling delegate"), UTF8_TO_TCHAR(ParamDef.ParamName.GetData())));
					return nullptr;
				}
			}
		}
		FDelegateInvocation::Call(DelegateToCall, DelegateParams.GetMemory());
		return UPyGenUtil::PackReturnValues(DelegateParams.GetMemory(), DelegateSignature.OutputParams, *UPyUtil::GetErrorContext(InSelf), TEXT("delegate"));
	}
};

typedef TUPyWrapperDelegateImpl<FUPyWrapperDelegate, /*FUPyWrapperDelegateMetaData,*/FScriptDelegate, FUPyWrapperDelegateFactory, FDelegateProperty> FUPyWrapperDelegateImpl;
typedef TUPyWrapperDelegateImpl<FUPyWrapperMulticastDelegate, /*FUPyWrapperMulticastDelegateMetaData,*/FMulticastScriptDelegate, FUPyWrapperMulticastDelegateFactory, FMulticastDelegateProperty> FPyWrapperMulticastDelegateImpl;


FUPyWrapperDelegate* FUPyWrapperDelegate::New(PyTypeObject* InType)
{
	return FUPyWrapperDelegateImpl::New(InType);
}

void FUPyWrapperDelegate::Free(FUPyWrapperDelegate* InSelf)
{
	FUPyWrapperDelegateImpl::Free(InSelf);
}

int FUPyWrapperDelegate::Init(FUPyWrapperDelegate* InSelf)
{
	return FUPyWrapperDelegateImpl::Init(InSelf);
}

int FUPyWrapperDelegate::Init(FUPyWrapperDelegate* InSelf, const FUPyWrapperOwnerContext& InOwnerContext, FScriptDelegate* InValue, const FDelegateProperty* InProp, const EUPyConversionMethod InConversionMethod)
{
	// For Single Delegates, InValue is the address of the FScriptDelegate which acts as the ValueAddr
	return FUPyWrapperDelegateImpl::Init(InSelf, InOwnerContext, InValue, (void*)InValue, InProp, InConversionMethod);
}

void FUPyWrapperDelegate::Deinit(FUPyWrapperDelegate* InSelf)
{
	FUPyWrapperDelegateImpl::Deinit(InSelf);
}

bool FUPyWrapperDelegate::ValidateInternalState(FUPyWrapperDelegate* InSelf)
{
	return FUPyWrapperDelegateImpl::ValidateInternalState(InSelf);
}

FUPyWrapperDelegate* FUPyWrapperDelegate::CastPyObject(PyObject* InPyObject, FUPyConversionResult* OutCastResult)
{
	SetOptionalUPyConversionResult(FUPyConversionResult::Failure(), OutCastResult);

	if (PyObject_IsInstance(InPyObject, (PyObject*)&UPyWrapperDelegateType) == 1)
	{
		FUPyWrapperDelegate* Self = (FUPyWrapperDelegate*)InPyObject;
		if (!ValidateInternalState(Self))
		{
			return nullptr;
		}

		SetOptionalUPyConversionResult(FUPyConversionResult::Success(), OutCastResult);

		Py_INCREF(InPyObject);
		return Self;
	}

	return nullptr;
}

FUPyWrapperDelegate* FUPyWrapperDelegate::CastPyObject(PyObject* InPyObject, const PyTypeObject* InType, FUPyConversionResult* OutCastResult)
{
	SetOptionalUPyConversionResult(FUPyConversionResult::Failure(), OutCastResult);

	if (PyObject_IsInstance(InPyObject, (PyObject*)InType) == 1 && (InType == &UPyWrapperDelegateType || PyObject_IsInstance(InPyObject, (PyObject*)&UPyWrapperDelegateType) == 1))
	{
		FUPyWrapperDelegate* Self = (FUPyWrapperDelegate*)InPyObject;
		if (!ValidateInternalState(Self))
		{
			return nullptr;
		}

		SetOptionalUPyConversionResult(Py_TYPE(InPyObject) == InType ? FUPyConversionResult::Success() : FUPyConversionResult::SuccessWithCoercion(), OutCastResult);

		Py_INCREF(InPyObject);
		return Self;
	}

	// Note: -----------------------------------------------------------------------------------------------------------------------------------------------------------
	//  We currently don't allow coercion from a Python callable here as the lifetime rules of delegate proxies mean we want people to make that choice explicitly

	return nullptr;
}

PyObject* FUPyWrapperDelegate::CallDelegate(FUPyWrapperDelegate* InSelf, PyObject* InArgs)
{
	return FUPyWrapperDelegateImpl::CallDelegate(InSelf, InArgs);
}

bool FUPyWrapperDelegate::Unbind(FUPyWrapperDelegate* InSelf)
{
	if (!ValidateInternalState(InSelf))
	{
		return false;
	}

	if (UUPyCallableForDelegate* PyCallableObj = Cast<UUPyCallableForDelegate>(InSelf->DelegateInstance->GetUObject()))
	{
		PyCallableObj->Deinit();
	}

	InSelf->DelegateInstance->Unbind();
	return true;
}

void FUPyWrapperDelegate::TrackCallableProxy(FUPyWrapperDelegate* InSelf, UUPyCallableForDelegate* InProxy)
{
	FUPyWrapperDelegateImpl::TrackCallableProxy(InSelf, InProxy);
}

void FUPyWrapperDelegate::ReleaseTrackedCallableProxies(FUPyWrapperDelegate* InSelf)
{
	FUPyWrapperDelegateImpl::ReleaseTrackedCallableProxies(InSelf);
}

FUPyWrapperMulticastDelegate* FUPyWrapperMulticastDelegate::New(PyTypeObject* InType)
{
	return FPyWrapperMulticastDelegateImpl::New(InType);
}

void FUPyWrapperMulticastDelegate::Free(FUPyWrapperMulticastDelegate* InSelf)
{
	FPyWrapperMulticastDelegateImpl::Free(InSelf);
}

int FUPyWrapperMulticastDelegate::Init(FUPyWrapperMulticastDelegate* InSelf)
{
	return FPyWrapperMulticastDelegateImpl::Init(InSelf);
}

int FUPyWrapperMulticastDelegate::Init(FUPyWrapperMulticastDelegate* InSelf, const FUPyWrapperOwnerContext& InOwnerContext, FMulticastScriptDelegate* InValue, void* InPropAddr, const FMulticastDelegateProperty* InProp, const EUPyConversionMethod InConversionMethod)
{
	return FPyWrapperMulticastDelegateImpl::Init(InSelf, InOwnerContext, InValue, InPropAddr, InProp, InConversionMethod);
}

void FUPyWrapperMulticastDelegate::Deinit(FUPyWrapperMulticastDelegate* InSelf)
{
	FPyWrapperMulticastDelegateImpl::Deinit(InSelf);
}

bool FUPyWrapperMulticastDelegate::ValidateInternalState(FUPyWrapperMulticastDelegate* InSelf)
{
	return FPyWrapperMulticastDelegateImpl::ValidateInternalState(InSelf);
}

FUPyWrapperMulticastDelegate* FUPyWrapperMulticastDelegate::CastPyObject(PyObject* InPyObject, FUPyConversionResult* OutCastResult)
{
	SetOptionalUPyConversionResult(FUPyConversionResult::Failure(), OutCastResult);

	if (PyObject_IsInstance(InPyObject, (PyObject*)&UPyWrapperMulticastDelegateType) == 1)
	{
		FUPyWrapperMulticastDelegate* Self = (FUPyWrapperMulticastDelegate*)InPyObject;
		if (!ValidateInternalState(Self))
		{
			return nullptr;
		}

		SetOptionalUPyConversionResult(FUPyConversionResult::Success(), OutCastResult);

		Py_INCREF(InPyObject);
		return Self;
	}

	return nullptr;
}

FUPyWrapperMulticastDelegate* FUPyWrapperMulticastDelegate::CastPyObject(PyObject* InPyObject, const PyTypeObject* InType, FUPyConversionResult* OutCastResult)
{
	SetOptionalUPyConversionResult(FUPyConversionResult::Failure(), OutCastResult);

	if (PyObject_IsInstance(InPyObject, (PyObject*)InType) == 1 && (InType == &UPyWrapperMulticastDelegateType || PyObject_IsInstance(InPyObject, (PyObject*)&UPyWrapperMulticastDelegateType) == 1))
	{
		FUPyWrapperMulticastDelegate* Self = (FUPyWrapperMulticastDelegate*)InPyObject;
		if (!ValidateInternalState(Self))
		{
			return nullptr;
		}

		SetOptionalUPyConversionResult(Py_TYPE(InPyObject) == InType ? FUPyConversionResult::Success() : FUPyConversionResult::SuccessWithCoercion(), OutCastResult);

		Py_INCREF(InPyObject);
		return Self;
	}

	return nullptr;
}

PyObject* FUPyWrapperMulticastDelegate::CallDelegate(FUPyWrapperMulticastDelegate* InSelf, PyObject* InArgs)
{
	return FPyWrapperMulticastDelegateImpl::CallDelegate(InSelf, InArgs);
}

bool FUPyWrapperMulticastDelegate::Unbind(FUPyWrapperMulticastDelegate* InSelf)
{
	return true;
}

void FUPyWrapperMulticastDelegate::TrackCallableProxy(FUPyWrapperMulticastDelegate* InSelf, UUPyCallableForDelegate* InProxy)
{
	FPyWrapperMulticastDelegateImpl::TrackCallableProxy(InSelf, InProxy);
}

void FUPyWrapperMulticastDelegate::ReleaseTrackedCallableProxies(FUPyWrapperMulticastDelegate* InSelf)
{
	FPyWrapperMulticastDelegateImpl::ReleaseTrackedCallableProxies(InSelf);
}

// ==================== Wrapper WrapperDelegate type BEGIN ====================

struct FFuncs_WrapperDelegate
{
	static PyObject* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
	{
		return (PyObject*)FUPyWrapperDelegate::New(InType);
	}

	static void Dealloc(FUPyWrapperDelegate* InSelf)
	{
		FUPyWrapperDelegate::Free(InSelf);
	}

	static int Init(FUPyWrapperDelegate* InSelf, PyObject* InArgs, PyObject* InKwds)
	{
		UPyUtil::SetPythonError(PyExc_RuntimeError, TEXT("FUPyWrapperDelegate"), TEXT("Direct initialize of FUPyWrapperDelegate failed"));
		return -1;
		// const int BaseInit = FUPyWrapperDelegate::Init(InSelf);
		// if (BaseInit != 0)
		// {
		// 	return BaseInit;
		// }
		//
		// const UPyGenUtil::FGeneratedWrappedFunction& DelegateSignature = FUPyWrapperDelegateMetaData::GetDelegateSignature(InSelf);
		// const UClass* PythonCallableForDelegateClass = FUPyWrapperDelegateMetaData::GetPythonCallableForDelegateClass(InSelf);
		//
		// const int32 ArgsCount = PyTuple_Size(InArgs);
		// if (ArgsCount == 1)
		// {
		// 	// One argument is assumed to be a callable
		// 	if (!UPyDelegateUtil::PythonArgsToDelegate_Callable(InArgs, DelegateSignature, PythonCallableForDelegateClass, *InSelf->DelegateInstance, TEXT("call"), *UPyUtil::GetErrorContext(InSelf)))
		// 	{
		// 		return -1;
		// 	}
		// }
		// else if (ArgsCount > 0)
		// {
		// 	// Anything else is assumed to be an object and name pair
		// 	if (!UPyDelegateUtil::PythonArgsToDelegate_ObjectAndName(InArgs, DelegateSignature, *InSelf->DelegateInstance, TEXT("call"), *UPyUtil::GetErrorContext(InSelf)))
		// {
		// 	return -1;
		// }
		// }
		//
		// return 0;
	}

	static PyObject* Str(FUPyWrapperDelegate* InSelf)
	{
		if (!FUPyWrapperDelegate::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		const FScriptDelegate* DelegateInstanceToUse = FUPyWrapperDelegateImpl::ResolveDelegateInstance(InSelf);
		const FString DelegateString = DelegateInstanceToUse ? DelegateInstanceToUse->ToString<UObject>() : TEXT("");
		return PyUnicode_FromFormat("<Delegate '%s' (%p) %s>", TCHAR_TO_UTF8(*UPyUtil::GetFriendlyTypename(InSelf)), DelegateInstanceToUse, TCHAR_TO_UTF8(*DelegateString));
	}

	static PyObject* Call(FUPyWrapperDelegate* InSelf, PyObject* InArgs, PyObject* InKwds)
	{
		if (InKwds && PyDict_Size(InKwds) != 0)
		{
			UPyUtil::SetPythonError(PyExc_Exception, InSelf, TEXT("Cannot call a delegate with keyword arguments"));
			return nullptr;
		}

		return FUPyWrapperDelegate::CallDelegate(InSelf, InArgs);
	}
};

struct FNumberFuncs_WrapperDelegate
{
	static int Bool(FUPyWrapperDelegate* InSelf)
	{
		if (!FUPyWrapperDelegate::ValidateInternalState(InSelf))
		{
			return -1;
		}

		const FScriptDelegate* DelegateInstanceToUse = FUPyWrapperDelegateImpl::ResolveDelegateInstance(InSelf);
		return DelegateInstanceToUse && DelegateInstanceToUse->IsBound() ? 1 : 0;
	}
};

void InitializeUPyWrapperDelegateType(UPyGenUtil::FNativePythonModule& ModuleInfo)
{
	PyTypeObject* PyType = &UPyWrapperDelegateType;

	PyType->tp_base = &UPyWrapperBaseType;
	PyType->tp_new = (newfunc)&FFuncs_WrapperDelegate::New;
	PyType->tp_dealloc = (destructor)&FFuncs_WrapperDelegate::Dealloc;
	PyType->tp_init = (initproc)&FFuncs_WrapperDelegate::Init;
	PyType->tp_str = (reprfunc)&FFuncs_WrapperDelegate::Str;
	PyType->tp_repr = (reprfunc)&FFuncs_WrapperDelegate::Str;
	PyType->tp_call = (ternaryfunc)&FFuncs_WrapperDelegate::Call;

	PyType->tp_methods = DelegatePyMethodDefs;

	PyType->tp_flags = Py_TPFLAGS_DEFAULT;
	PyType->tp_doc = "Type for all Unreal exposed delegate instances";

	static PyNumberMethods PyNumber;
	PyNumber.nb_bool = (inquiry)&FNumberFuncs_WrapperDelegate::Bool;
	PyType->tp_as_number = &PyNumber;

	if (PyType_Ready(PyType) == 0)
	{
		// static FUPyWrapperDelegateMetaData MetaData;
		// FUPyWrapperDelegateMetaData::SetMetaData(&UPyWrapperDelegateType, &MetaData);
		ModuleInfo.AddType(PyType, true);
	}
}
// ==================== Wrapper WrapperDelegate type END ====================

// ==================== Wrapper WrapperMulticastDelegate type BEGIN ====================
struct FFuncs_WrapperMulticastDelegate
{
	static PyObject* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
	{
		return (PyObject*)FUPyWrapperMulticastDelegate::New(InType);
	}

	static void Dealloc(FUPyWrapperMulticastDelegate* InSelf)
	{
		FUPyWrapperMulticastDelegate::Free(InSelf);
	}

	static int Init(FUPyWrapperMulticastDelegate* InSelf, PyObject* InArgs, PyObject* InKwds)
	{
		UPyUtil::SetPythonError(PyExc_RuntimeError, TEXT("FUPyWrapperMulticastDelegate"), TEXT("Direct initialize of FUPyWrapperMulticastDelegate failed"));
		return -1;
		// const int BaseInit = FUPyWrapperMulticastDelegate::Init(InSelf);
		// if (BaseInit != 0)
		// {
		// 	return BaseInit;
		// }
		//
		// if (PyTuple_Size(InArgs) > 0)
		// {
		// 	const UPyGenUtil::FGeneratedWrappedFunction& DelegateSignature = FUPyWrapperMulticastDelegateMetaData::GetDelegateSignature(InSelf);
		//
		// 	FScriptDelegate Delegate;
		// 	if (!UPyDelegateUtil::PythonArgsToDelegate_ObjectAndName(InArgs, DelegateSignature, Delegate, TEXT("call"), *UPyUtil::GetErrorContext(InSelf)))
		// 	{
		// 		return -1;
		// 	}
		// 	InSelf->DelegateInstance->Add(Delegate);
		// }
		//
		// return 0;
	}

	static PyObject* Str(FUPyWrapperMulticastDelegate* InSelf)
	{
		if (!FUPyWrapperMulticastDelegate::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		const FMulticastScriptDelegate* DelegateInstanceToUse = FPyWrapperMulticastDelegateImpl::ResolveDelegateInstance(InSelf);
		const FString DelegateString = DelegateInstanceToUse ? DelegateInstanceToUse->ToString<UObject>() : TEXT("");
		return PyUnicode_FromFormat("<Multicast delegate '%s' (%p) %s>", TCHAR_TO_UTF8(*UPyUtil::GetFriendlyTypename(InSelf)), DelegateInstanceToUse, TCHAR_TO_UTF8(*DelegateString));
	}

	static PyObject* Call(FUPyWrapperMulticastDelegate* InSelf, PyObject* InArgs, PyObject* InKwds)
	{
		if (InKwds && PyDict_Size(InKwds) != 0)
		{
			UPyUtil::SetPythonError(PyExc_Exception, InSelf, TEXT("Cannot call a delegate with keyword arguments"));
			return nullptr;
		}

		return FUPyWrapperMulticastDelegate::CallDelegate(InSelf, InArgs);
	}
};

struct FNumberFuncs_WrapperMulticastDelegate
{
	static int Bool(FUPyWrapperMulticastDelegate* InSelf)
	{
		if (!FUPyWrapperMulticastDelegate::ValidateInternalState(InSelf))
		{
			return -1;
		}

		const FMulticastScriptDelegate* DelegateInstanceToUse = FPyWrapperMulticastDelegateImpl::ResolveDelegateInstance(InSelf);
		return DelegateInstanceToUse && DelegateInstanceToUse->IsBound() ? 1 : 0;
	}
};

void InitializeUPyWrapperMulticastDelegateType(UPyGenUtil::FNativePythonModule& ModuleInfo)
{
	PyTypeObject* PyType = &UPyWrapperMulticastDelegateType;

	PyType->tp_base = &UPyWrapperBaseType;
	PyType->tp_new = (newfunc)&FFuncs_WrapperMulticastDelegate::New;
	PyType->tp_dealloc = (destructor)&FFuncs_WrapperMulticastDelegate::Dealloc;
	PyType->tp_init = (initproc)&FFuncs_WrapperMulticastDelegate::Init;
	PyType->tp_str = (reprfunc)&FFuncs_WrapperMulticastDelegate::Str;
	PyType->tp_repr = (reprfunc)&FFuncs_WrapperMulticastDelegate::Str;
	PyType->tp_call = (ternaryfunc)&FFuncs_WrapperMulticastDelegate::Call;

	PyType->tp_methods = MulticastDelegatePyMethodDefs;

	PyType->tp_flags = Py_TPFLAGS_DEFAULT;
	PyType->tp_doc = "Type for all Unreal exposed multicast delegate instances";

	static PyNumberMethods PyNumber;
	PyNumber.nb_bool = (inquiry)&FNumberFuncs_WrapperMulticastDelegate::Bool;
	PyType->tp_as_number = &PyNumber;

	if (PyType_Ready(PyType) == 0)
	{
		// static FUPyWrapperMulticastDelegateMetaData MetaData;
		// FUPyWrapperMulticastDelegateMetaData::SetMetaData(&UPyWrapperMulticastDelegateType, &MetaData);
		ModuleInfo.AddType(PyType, true);
	}
}

void InitializeUPyWrapperDelegate(UPyGenUtil::FNativePythonModule& ModuleInfo)
{
	InitializeUPyWrapperDelegateType(ModuleInfo);

	InitializeUPyWrapperMulticastDelegateType(ModuleInfo);
}
// ==================== Wrapper WrapperMulticastDelegate type END ====================

// void FUPyWrapperDelegateMetaData::AddInstanceReferencedObjects(FUPyWrapperBase* Instance, FReferenceCollector& Collector)
// {
// 	FUPyWrapperDelegate* Self = static_cast<FUPyWrapperDelegate*>(Instance);
// 	if (Self->DelegateInstance)
// 	{
// 		FPyReferenceCollector::AddReferencedObjectsFromDelegate(Collector, *Self->DelegateInstance);
// 	}
// }
//
// void FUPyWrapperMulticastDelegateMetaData::AddInstanceReferencedObjects(FUPyWrapperBase* Instance, FReferenceCollector& Collector)
// {
// 	FUPyWrapperMulticastDelegate* Self = static_cast<FUPyWrapperMulticastDelegate*>(Instance);
// 	if (Self->DelegateInstance)
// 	{
// 		FPyReferenceCollector::AddReferencedObjectsFromMulticastDelegate(Collector, *Self->DelegateInstance);
// 	}
// }
