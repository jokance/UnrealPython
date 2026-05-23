
#include "Wrapper/UPyWrapperObjectBase.h"
#include "Wrapper/UPyWrapperTypeFactory.h"
#include "Wrapper/UPyWrapperTypeRegistry.h"
#include "Utils/UPyUtil.h"
#include "Core/UPyConversion.h"

extern PyMethodDef ObjectBasePyMethodDefs[];

PyTypeObject UPyWrapperObjectBaseType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"_ObjectBase", /* tp_name */
	sizeof(FUPyWrapperObjectBase), /* tp_basicsize */
};

FUPyWrapperObjectBase* FUPyWrapperObjectBase::New(PyTypeObject* InType)
{
	FUPyWrapperObjectBase* Self = (FUPyWrapperObjectBase*)FUPyWrapperBase::New(InType);
	if (Self)
	{
		Self->ObjectInstance = nullptr;
	}
	return Self;
}

void FUPyWrapperObjectBase::Free(FUPyWrapperObjectBase* InSelf)
{
	Deinit(InSelf);

	FUPyWrapperBase::Free(InSelf);
}

int FUPyWrapperObjectBase::Init(FUPyWrapperObjectBase* InSelf, UObject* InValue)
{
	Deinit(InSelf);

	const int BaseInit = FUPyWrapperBase::Init(InSelf);
	if (BaseInit != 0)
	{
		return BaseInit;
	}

	check(InValue);

	InSelf->ObjectInstance = InValue;
	FUPyWrapperObjectFactory::Get().MapInstance(InSelf->ObjectInstance, InSelf);
	return 0;
}

void FUPyWrapperObjectBase::Deinit(FUPyWrapperObjectBase* InSelf)
{
	if (InSelf->ObjectInstance)
	{
		FUPyWrapperObjectFactory::Get().UnmapInstance(InSelf->ObjectInstance);
	}
	InSelf->ObjectInstance = nullptr;
}

bool FUPyWrapperObjectBase::ValidateInternalState(FUPyWrapperObjectBase* InSelf)
{
	if (!IsValid(InSelf->ObjectInstance) || InSelf->ObjectInstance->GetFName() == NAME_None)
	{
		UPyUtil::SetPythonError(PyExc_Exception, Py_TYPE(InSelf), TEXT("Internal Error - ObjectInstance is null!"));
		return false;
	}

	return true;
}

FUPyWrapperObjectBase* FUPyWrapperObjectBase::CastPyObject(PyObject* InPyObject, FUPyConversionResult* OutCastResult)
{
	SetOptionalUPyConversionResult(FUPyConversionResult::Failure(), OutCastResult);

	if (PyObject_IsInstance(InPyObject, (PyObject*)&UPyWrapperObjectBaseType) == 1)
	{
		FUPyWrapperObjectBase* Self = (FUPyWrapperObjectBase*)InPyObject;
		if (!ValidateInternalState(Self))
		{
			return nullptr;
		}

		SetOptionalUPyConversionResult(FUPyConversionResult::Success(), OutCastResult);

		Py_INCREF(Self);
		return Self;
	}

	return nullptr;
}

FUPyWrapperObjectBase* FUPyWrapperObjectBase::CastPyObject(PyObject* InPyObject, PyTypeObject* InType, FUPyConversionResult* OutCastResult)
{
	SetOptionalUPyConversionResult(FUPyConversionResult::Failure(), OutCastResult);

	if (PyObject_IsInstance(InPyObject, (PyObject*)InType) == 1 && (InType == &UPyWrapperObjectBaseType || PyObject_IsInstance(InPyObject, (PyObject*)&UPyWrapperObjectBaseType) == 1))
	{
		FUPyWrapperObjectBase* Self = (FUPyWrapperObjectBase*)InPyObject;
		if (!ValidateInternalState(Self))
		{
			return nullptr;
		}

		SetOptionalUPyConversionResult(Py_TYPE(InPyObject) == InType ? FUPyConversionResult::Success() : FUPyConversionResult::SuccessWithCoercion(), OutCastResult);

		Py_INCREF(Self);
		return Self;
	}

	return nullptr;
}

PyObject* FUPyWrapperObjectBase::GetPropertyValue(FUPyWrapperObjectBase* InSelf, const UPyGenUtil::FGeneratedWrappedProperty& InPropDef, const char* InPythonAttrName)
{
	if (!ValidateInternalState(InSelf))
	{
		return nullptr;
	}

	return UPyGenUtil::GetPropertyValue(InSelf->ObjectInstance->GetClass(), InSelf->ObjectInstance, InPropDef, InPythonAttrName, (PyObject*)InSelf, *UPyUtil::GetErrorContext(InSelf));
}

int FUPyWrapperObjectBase::SetPropertyValue(FUPyWrapperObjectBase* InSelf, PyObject* InValue, const UPyGenUtil::FGeneratedWrappedProperty& InPropDef, const char* InPythonAttrName, const EPropertyAccessChangeNotifyMode InNotifyMode, const uint64 InReadOnlyFlags)
{
	if (!ValidateInternalState(InSelf))
	{
		return -1;
	}

	const TUniquePtr<FPropertyAccessChangeNotify> ChangeNotify = FUPyWrapperOwnerContext((PyObject*)InSelf, InPropDef.Prop).BuildChangeNotify(InNotifyMode);

	// If the object is a template, gather instances (including subclass CDOs) inheriting their property value from this template.
	// Those instances are passed into UPyGenUtil::SetPropertyValue so that they will receive the value change as well.
	TArray<void*> ArchetypeInsts;
	const bool bIsObjectTemplate = PropertyAccessUtil::IsObjectTemplate(InSelf->ObjectInstance);
	if (bIsObjectTemplate)
	{
		PropertyAccessUtil::GetArchetypeInstancesInheritingPropertyValue_AsContainerData(InPropDef.Prop, InSelf->ObjectInstance, ArchetypeInsts);
	}

	return UPyGenUtil::SetPropertyValue(InSelf->ObjectInstance->GetClass(), InSelf->ObjectInstance, InValue, InPropDef, InPythonAttrName, ChangeNotify.Get(), InReadOnlyFlags, bIsObjectTemplate, *UPyUtil::GetErrorContext(InSelf), ArchetypeInsts);
}

PyObject* FUPyWrapperObjectBase::CallGetterFunction(FUPyWrapperObjectBase* InSelf, const UPyGenUtil::FGeneratedWrappedFunction& InFuncDef)
{
	if (!ValidateInternalState(InSelf))
	{
		return nullptr;
	}

	return CallFunction_Impl(InSelf->ObjectInstance, InFuncDef, InFuncDef.Func ? TCHAR_TO_UTF8(*InFuncDef.Func->GetName()) : "null", *UPyUtil::GetErrorContext(InSelf));
}

int FUPyWrapperObjectBase::CallSetterFunction(FUPyWrapperObjectBase* InSelf, PyObject* InValue, const UPyGenUtil::FGeneratedWrappedFunction& InFuncDef)
{
	if (!ValidateInternalState(InSelf))
	{
		return -1;
	}

	if (ensureAlways(InFuncDef.Func))
	{
		// Deprecated functions emit a warning
		if (InFuncDef.DeprecationMessage.IsSet())
		{
			if (UPyUtil::SetPythonWarning(PyExc_DeprecationWarning, InSelf, *FString::Printf(TEXT("Function '%s.%s' is deprecated: %s"), *InFuncDef.Func->GetOwnerClass()->GetName(), *InFuncDef.Func->GetName(), *InFuncDef.DeprecationMessage.GetValue())) == -1)
			{
				// -1 from SetPythonWarning means the warning should be an exception
				return -1;
			}
		}

		// Setter functions should have a single input parameter and no output parameters
		if (InFuncDef.InputParams.Num() != 1 || InFuncDef.OutputParams.Num() != 0)
		{
			UPyUtil::SetPythonError(PyExc_Exception, InSelf, *FString::Printf(TEXT("Setter function '%s.%s' on '%s' has the incorrect number of parameters (expected 1 input and 0 output, got %d input and %d output)"), *InFuncDef.Func->GetOwnerClass()->GetName(), *InFuncDef.Func->GetName(), *InSelf->ObjectInstance->GetName(), InFuncDef.InputParams.Num(), InFuncDef.OutputParams.Num()));
			return -1;
		}

		UPY_UFUNCTION_STACK(FuncParams, InFuncDef.Func);
		if (InValue)
		{
			if (!UPyConversion::NativizeProperty_InContainer(InValue, InFuncDef.InputParams[0].ParamProp, FuncParams.GetMemory(), 0))
			{
				UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert input parameter when calling function '%s.%s' on '%s'"), *InFuncDef.Func->GetOwnerClass()->GetName(), *InFuncDef.Func->GetName(), *InSelf->ObjectInstance->GetName()));
				return -1;
			}
		}
		if (!UPyUtil::InvokeFunctionCall(InSelf->ObjectInstance, InFuncDef.Func, FuncParams.GetMemory(), *UPyUtil::GetErrorContext(InSelf)))
		{
			return -1;
		}
	}

	return 0;
}

PyObject* FUPyWrapperObjectBase::CallFunction(PyTypeObject* InType, const UPyGenUtil::FGeneratedWrappedFunction& InFuncDef, const char* InPythonFuncName)
{
	const UClass* Class = FUPyWrapperTypeRegistry::Get().FindClass(InType);
	UObject* Obj = Class ? Class->GetDefaultObject() : nullptr;
	return CallFunction_Impl(Obj, InFuncDef, InPythonFuncName, *UPyUtil::GetErrorContext(InType));
}

PyObject* FUPyWrapperObjectBase::CallFunction(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds, const UPyGenUtil::FGeneratedWrappedFunction& InFuncDef, const char* InPythonFuncName)
{
	const UClass* Class = FUPyWrapperTypeRegistry::Get().FindClass(InType);
	UObject* Obj = Class ? Class->GetDefaultObject() : nullptr;
	return CallFunction_Impl(Obj, InArgs, InKwds, InFuncDef, InPythonFuncName, *UPyUtil::GetErrorContext(InType));
}

PyObject* FUPyWrapperObjectBase::CallFunction(FUPyWrapperObjectBase* InSelf, const UPyGenUtil::FGeneratedWrappedFunction& InFuncDef, const char* InPythonFuncName)
{
	if (!ValidateInternalState(InSelf))
	{
		return nullptr;
	}

	return CallFunction_Impl(InSelf->ObjectInstance, InFuncDef, InPythonFuncName, *UPyUtil::GetErrorContext(InSelf));
}

PyObject* FUPyWrapperObjectBase::CallFunction(FUPyWrapperObjectBase* InSelf, PyObject* InArgs, PyObject* InKwds, const UPyGenUtil::FGeneratedWrappedFunction& InFuncDef, const char* InPythonFuncName)
{
	if (!ValidateInternalState(InSelf))
	{
		return nullptr;
	}

	return CallFunction_Impl(InSelf->ObjectInstance, InArgs, InKwds, InFuncDef, InPythonFuncName, *UPyUtil::GetErrorContext(InSelf));
}

PyObject* FUPyWrapperObjectBase::CallFunction_Impl(UObject* InObj, const UPyGenUtil::FGeneratedWrappedFunction& InFuncDef, const char* InPythonFuncName, const TCHAR* InErrorCtxt)
{
	if (InObj && ensureAlways(InFuncDef.Func))
	{
		// Deprecated functions emit a warning
		if (InFuncDef.DeprecationMessage.IsSet())
		{
			if (UPyUtil::SetPythonWarning(PyExc_DeprecationWarning, InErrorCtxt, *FString::Printf(TEXT("Function '%s' on '%s' is deprecated: %s"), UTF8_TO_TCHAR(InPythonFuncName), *InFuncDef.Func->GetOwnerClass()->GetName(), *InFuncDef.DeprecationMessage.GetValue())) == -1)
			{
				// -1 from SetPythonWarning means the warning should be an exception
				return nullptr;
			}
		}

		if (InFuncDef.Func->ChildProperties == nullptr)
		{
			// No return value
			if (!UPyUtil::InvokeFunctionCall(InObj, InFuncDef.Func, nullptr, InErrorCtxt))
			{
				return nullptr;
			}
		}
		else
		{
			// Return value requires that we create a params struct to hold the result
			UPY_UFUNCTION_STACK(FuncParams, InFuncDef.Func);
			if (!UPyUtil::InvokeFunctionCall(InObj, InFuncDef.Func, FuncParams.GetMemory(), InErrorCtxt))
			{
				return nullptr;
			}
			return UPyGenUtil::PackReturnValues(FuncParams.GetMemory(), InFuncDef.OutputParams, InErrorCtxt, *FString::Printf(TEXT("function '%s.%s' on '%s'"), *InFuncDef.Func->GetOwnerClass()->GetName(), *InFuncDef.Func->GetName(), *InObj->GetName()));
		}
	}

	Py_RETURN_NONE;
}

PyObject* FUPyWrapperObjectBase::CallFunction_Impl(UObject* InObj, PyObject* InArgs, PyObject* InKwds, const UPyGenUtil::FGeneratedWrappedFunction& InFuncDef, const char* InPythonFuncName, const TCHAR* InErrorCtxt)
{
	TArray<PyObject*> Params;
	if (!UPyGenUtil::ParseMethodParameters(InArgs, InKwds, InFuncDef.InputParams, InPythonFuncName, Params))
	{
		return nullptr;
	}

	if (InObj && ensureAlways(InFuncDef.Func))
	{
		// Deprecated functions emit a warning
		if (InFuncDef.DeprecationMessage.IsSet())
		{
			if (UPyUtil::SetPythonWarning(PyExc_DeprecationWarning, InErrorCtxt, *FString::Printf(TEXT("Function '%s' on '%s' is deprecated: %s"), UTF8_TO_TCHAR(InPythonFuncName), *InFuncDef.Func->GetOwnerClass()->GetName(), *InFuncDef.DeprecationMessage.GetValue())) == -1)
			{
				// -1 from SetPythonWarning means the warning should be an exception
				return nullptr;
			}
		}

		UPY_UFUNCTION_STACK(FuncParams, InFuncDef.Func);
		UPyGenUtil::ApplyParamDefaults(FuncParams.GetMemory(), InFuncDef.InputParams);
		for (int32 ParamIndex = 0; ParamIndex < Params.Num(); ++ParamIndex)
		{
			const UPyGenUtil::FGeneratedWrappedMethodParameter& ParamDef = InFuncDef.InputParams[ParamIndex];

			PyObject* PyValue = Params[ParamIndex];
			if (PyValue)
			{
				if (!UPyConversion::NativizeProperty_InContainer(PyValue, ParamDef.ParamProp, FuncParams.GetMemory(), 0))
				{
					UPyUtil::SetPythonError(PyExc_TypeError, InErrorCtxt, *FString::Printf(TEXT("Failed to convert parameter '%s' when calling function '%s.%s' on '%s'"), UTF8_TO_TCHAR(ParamDef.ParamName.GetData()), *InFuncDef.Func->GetOwnerClass()->GetName(), *InFuncDef.Func->GetName(), *InObj->GetName()));
					return nullptr;
				}
			}
		}
		if (!UPyUtil::InvokeFunctionCall(InObj, InFuncDef.Func, FuncParams.GetMemory(), InErrorCtxt))
		{
			return nullptr;
		}
		return UPyGenUtil::PackReturnValues(FuncParams.GetMemory(), InFuncDef.OutputParams, InErrorCtxt, *FString::Printf(TEXT("function '%s.%s' on '%s'"), *InFuncDef.Func->GetOwnerClass()->GetName(), *InFuncDef.Func->GetName(), *InObj->GetName()));
	}

	Py_RETURN_NONE;
}

PyObject* FUPyWrapperObjectBase::CallClassMethodNoArgs_Impl(PyTypeObject* InType, void* InClosure)
{
	const UPyGenUtil::FGeneratedWrappedMethod* Closure = (UPyGenUtil::FGeneratedWrappedMethod*)InClosure;
	return CallFunction(InType, Closure->MethodFunc, Closure->MethodName.GetData());
}

PyObject* FUPyWrapperObjectBase::CallClassMethodWithArgs_Impl(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds, void* InClosure)
{
	const UPyGenUtil::FGeneratedWrappedMethod* Closure = (UPyGenUtil::FGeneratedWrappedMethod*)InClosure;
	return CallFunction(InType, InArgs, InKwds, Closure->MethodFunc, Closure->MethodName.GetData());
}

PyObject* FUPyWrapperObjectBase::CallMethodNoArgs_Impl(FUPyWrapperObjectBase* InSelf, void* InClosure)
{
	const UPyGenUtil::FGeneratedWrappedMethod* Closure = (UPyGenUtil::FGeneratedWrappedMethod*)InClosure;
	return CallFunction(InSelf, Closure->MethodFunc, Closure->MethodName.GetData());
}

PyObject* FUPyWrapperObjectBase::CallMethodWithArgs_Impl(FUPyWrapperObjectBase* InSelf, PyObject* InArgs, PyObject* InKwds, void* InClosure)
{
	const UPyGenUtil::FGeneratedWrappedMethod* Closure = (UPyGenUtil::FGeneratedWrappedMethod*)InClosure;
	return CallFunction(InSelf, InArgs, InKwds, Closure->MethodFunc, Closure->MethodName.GetData());
}

PyObject* FUPyWrapperObjectBase::CallDynamicFunction_Impl(FUPyWrapperObjectBase* InSelf, PyObject* InArgs, PyObject* InKwds, const UPyGenUtil::FGeneratedWrappedFunction& InFuncDef, const UPyGenUtil::FGeneratedWrappedMethodParameter& InSelfParam, const char* InPythonFuncName)
{
	TArray<PyObject*> Params;
	if ((InArgs || InKwds) && !UPyGenUtil::ParseMethodParameters(InArgs, InKwds, InFuncDef.InputParams, InPythonFuncName, Params))
	{
		return nullptr;
	}

	if (ensureAlways(InFuncDef.Func))
	{
		UClass* Class = InFuncDef.Func->GetOwnerClass();
		UObject* Obj = Class->GetDefaultObject();

		// Deprecated functions emit a warning
		if (InFuncDef.DeprecationMessage.IsSet())
		{
			if (UPyUtil::SetPythonWarning(PyExc_DeprecationWarning, InSelf, *FString::Printf(TEXT("Function '%s' on '%s' is deprecated: %s"), UTF8_TO_TCHAR(InPythonFuncName), *Class->GetName(), *InFuncDef.DeprecationMessage.GetValue())) == -1)
			{
				// -1 from SetPythonWarning means the warning should be an exception
				return nullptr;
			}
		}

		UPY_UFUNCTION_STACK(FuncParams, InFuncDef.Func);
		UPyGenUtil::ApplyParamDefaults(FuncParams.GetMemory(), InFuncDef.InputParams);
		if (ensureAlways(CastField<FObjectPropertyBase>(InSelfParam.ParamProp)))
		{
			void* SelfArgInstance = InSelfParam.ParamProp->ContainerPtrToValuePtr<void>(FuncParams.GetMemory());
			CastField<FObjectPropertyBase>(InSelfParam.ParamProp)->SetObjectPropertyValue(SelfArgInstance, InSelf->ObjectInstance);
		}
		for (int32 ParamIndex = 0; ParamIndex < Params.Num(); ++ParamIndex)
		{
			const UPyGenUtil::FGeneratedWrappedMethodParameter& ParamDef = InFuncDef.InputParams[ParamIndex];

			PyObject* PyValue = Params[ParamIndex];
			if (PyValue)
			{
				if (!UPyConversion::NativizeProperty_InContainer(PyValue, ParamDef.ParamProp, FuncParams.GetMemory(), 0))
				{
					UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert parameter '%s' when calling function '%s.%s' on '%s'"), UTF8_TO_TCHAR(ParamDef.ParamName.GetData()), *Class->GetName(), *InFuncDef.Func->GetName(), *Obj->GetName()));
					return nullptr;
				}
			}
		}
		const FString ErrorCtxt = UPyUtil::GetErrorContext(InSelf);
		if (!UPyUtil::InvokeFunctionCall(Obj, InFuncDef.Func, FuncParams.GetMemory(), *ErrorCtxt))
		{
			return nullptr;
		}
		return UPyGenUtil::PackReturnValues(FuncParams.GetMemory(), InFuncDef.OutputParams, *ErrorCtxt, *FString::Printf(TEXT("function '%s.%s' on '%s'"), *Class->GetName(), *InFuncDef.Func->GetName(), *Obj->GetName()));
	}

	Py_RETURN_NONE;
}

PyObject* FUPyWrapperObjectBase::CallDynamicMethodNoArgs_Impl(FUPyWrapperObjectBase* InSelf, void* InClosure)
{
	if (!ValidateInternalState(InSelf))
	{
		return nullptr;
	}

	const UPyGenUtil::FGeneratedWrappedDynamicMethod* Closure = (UPyGenUtil::FGeneratedWrappedDynamicMethod*)InClosure;
	return CallDynamicFunction_Impl(InSelf, nullptr, nullptr, Closure->MethodFunc, Closure->SelfParam, Closure->MethodName.GetData());
}

PyObject* FUPyWrapperObjectBase::CallDynamicMethodWithArgs_Impl(FUPyWrapperObjectBase* InSelf, PyObject* InArgs, PyObject* InKwds, void* InClosure)
{
	if (!ValidateInternalState(InSelf))
	{
		return nullptr;
	}

	const UPyGenUtil::FGeneratedWrappedDynamicMethod* Closure = (UPyGenUtil::FGeneratedWrappedDynamicMethod*)InClosure;
	return CallDynamicFunction_Impl(InSelf, InArgs, InKwds, Closure->MethodFunc, Closure->SelfParam, Closure->MethodName.GetData());
}

PyObject* FUPyWrapperObjectBase::Getter_Impl(FUPyWrapperObjectBase* InSelf, void* InClosure)
{
	const UPyGenUtil::FGeneratedWrappedGetSet* Closure = (UPyGenUtil::FGeneratedWrappedGetSet*)InClosure;
	return Closure->GetFunc.Func
		? CallGetterFunction(InSelf, Closure->GetFunc)
		: GetPropertyValue(InSelf, Closure->Prop, Closure->GetSetName.GetData());
}

int FUPyWrapperObjectBase::Setter_Impl(FUPyWrapperObjectBase* InSelf, PyObject* InValue, void* InClosure)
{
	const UPyGenUtil::FGeneratedWrappedGetSet* Closure = (UPyGenUtil::FGeneratedWrappedGetSet*)InClosure;
	return Closure->SetFunc.Func
		? CallSetterFunction(InSelf, InValue, Closure->SetFunc)
		: SetPropertyValue(InSelf, InValue, Closure->Prop, Closure->GetSetName.GetData());
}

// ==================== Wrapper Object Type BEGIN ====================

struct FFuncs_WrapperObjectBase
{
	static PyObject* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
	{
		return (PyObject*)FUPyWrapperObjectBase::New(InType);
	}

	static void Dealloc(FUPyWrapperObjectBase* InSelf)
	{
		FUPyWrapperObjectBase::Free(InSelf);
	}

	static int Init(FUPyWrapperObjectBase* InSelf, PyObject* InArgs, PyObject* InKwds)
	{
		// todo(hzn): can init directly?
		PyTypeObject* InType = Py_TYPE(InSelf);
		PyErr_Format(PyExc_RuntimeError, "Direct ObjectBase initialize of %s failed, please use World.SpawnActor() or ue.NewObject() to create objects", InType->tp_name);
		return -1;

		// UObject* InitValue = nullptr;
		//
		// UObject* ObjectOuter = GetTransientPackage();
		// FName ObjectName;
		//
		// // Parse the args
		// {
		// 	PyObject* PyOuterObj = nullptr;
		// 	PyObject* PyNameObj = nullptr;
		//
		// 	static const char *ArgsKwdList[] = { "outer", "name", nullptr };
		// 	if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "|OO:call", (char**)ArgsKwdList, &PyOuterObj, &PyNameObj))
		// 	{
		// 		return -1;
		// 	}
		//
		// 	if (PyOuterObj && !UPyConversion::Nativize(PyOuterObj, ObjectOuter))
		// 	{
		// 		UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'outer' (%s) to 'Object'"), *UPyUtil::GetFriendlyTypename(PyOuterObj)));
		// 		return -1;
		// 	}
		//
		// 	if (PyNameObj && !UPyConversion::Nativize(PyNameObj, ObjectName))
		// 	{
		// 		UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'name' (%s) to 'Name'"), *UPyUtil::GetFriendlyTypename(PyNameObj)));
		// 		return -1;
		// 	}
		// }

		// todo(hzn): metadata
		// Deprecated classes emit a warning
		// {
		// 	FString DeprecationMessage;
		// 	if (FPyWrapperObjectMetaData::IsClassDeprecated(InSelf, &DeprecationMessage) &&
		// 		UPyUtil::SetPythonWarning(PyExc_DeprecationWarning, InSelf, *FString::Printf(TEXT("Class '%s' is deprecated: %s"), UTF8_TO_TCHAR(Py_TYPE(InSelf)->tp_name), *DeprecationMessage)) == -1
		// 		)
		// 	{
		// 		// -1 from SetPythonWarning means the warning should be an exception
		// 		return -1;
		// 	}
		// }

		// Create the instance

		// UClass* ObjClass = (UClass*)FUPyWrapperTypeRegistry::Get().FindClass(Py_TYPE(InSelf));
		// InitValue = UPyUtil::NewObject(ObjClass, ObjectOuter, ObjectName, nullptr, *UPyUtil::GetErrorContext(InSelf));
		// if (!InitValue)
		// {
		// 	// UPyUtil::NewObject should have set an error state
		// 	return -1;
		// }
		//
		// return FUPyWrapperObjectBase::Init(InSelf, InitValue);
	}

	static PyObject* Str(FUPyWrapperObjectBase* InSelf)
	{
		if (!FUPyWrapperObjectBase::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		return PyUnicode_FromFormat("<Object '%s' (%p) Class '%s'>", TCHAR_TO_UTF8(*InSelf->ObjectInstance->GetPathName()), InSelf->ObjectInstance.Get(), TCHAR_TO_UTF8(*InSelf->ObjectInstance->GetClass()->GetName()));
	}

	static UPyUtil::FPyHashType Hash(FUPyWrapperObjectBase* InSelf)
	{
		if (!FUPyWrapperObjectBase::ValidateInternalState(InSelf))
		{
			return -1;
		}

		const UPyUtil::FPyHashType PyHash = (UPyUtil::FPyHashType)GetTypeHash(InSelf->ObjectInstance);
		return PyHash != -1 ? PyHash : 0;
	}
};

void InitializeUPyWrapperObjectBase(UPyGenUtil::FNativePythonModule& ModuleInfo)
{
	PyTypeObject* PyType = &UPyWrapperObjectBaseType;

	PyType->tp_base = &UPyWrapperBaseType;
	PyType->tp_new = (newfunc)&FFuncs_WrapperObjectBase::New;
	PyType->tp_dealloc = (destructor)&FFuncs_WrapperObjectBase::Dealloc;
	PyType->tp_init = (initproc)&FFuncs_WrapperObjectBase::Init;
	PyType->tp_str = (reprfunc)&FFuncs_WrapperObjectBase::Str;
	PyType->tp_repr = (reprfunc)&FFuncs_WrapperObjectBase::Str;

	PyType->tp_methods = ObjectBasePyMethodDefs;

	PyType->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
	PyType->tp_doc = "Type for all Unreal exposed object instances";

	if (PyType_Ready(PyType) == 0)
	{
		ModuleInfo.AddType(PyType, true);
		// FUPyWrapperTypeRegistry::Get().RegisterWrappedClassType(UObject::StaticClass(), PyType);
	}
}
// ==================== Wrapper Object Type END ====================
