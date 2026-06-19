
#include "Core/UPyCore.h"
#include "Wrapper/UPyWrapperTypeRegistry.h"
#include "Wrapper/UPyWrapperObjectBase.h"

PyTypeObject UPyDelegateHandleType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"_DelegateHandle", /* tp_name */
	sizeof(FUPyDelegateHandle), /* tp_basicsize */
};

PyTypeObject UPyScopedSlowTaskType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"ScopedSlowTask", /* tp_name */
	sizeof(FUPyScopedSlowTask), /* tp_basicsize */
};

PyTypeObject UPyObjectIteratorType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"ObjectIterator", /* tp_name */
	sizeof(FUPyObjectIterator), /* tp_basicsize */
};

PyTypeObject UPyClassIteratorType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"ClassIterator", /* tp_name */
	sizeof(FUPyClassIterator), /* tp_basicsize */
};

PyTypeObject UPyStructIteratorType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"StructIterator", /* tp_name */
	sizeof(FUPyStructIterator), /* tp_basicsize */
};

PyTypeObject UPyTypeIteratorType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"TypeIterator", /* tp_name */
	sizeof(FUPyTypeIterator), /* tp_basicsize */
};

PyTypeObject UPyUValueDefType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"ValueDef", /* tp_name */
	sizeof(FUPyUValueDef), /* tp_basicsize */
};

PyTypeObject UPyFPropertyDefType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"PropertyDef", /* tp_name */
	sizeof(FUPyFPropertyDef), /* tp_basicsize */
};

PyTypeObject UPyUFunctionDefType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"FunctionDef", /* tp_name */
	sizeof(FUPyUFunctionDef), /* tp_basicsize */
};

namespace UPyCoreUtil
{
	bool ConvertOptionalString(PyObject* InObj, FString& OutString, const TCHAR* InErrorCtxt, const TCHAR* InErrorMsg)
	{
		OutString.Reset();
		if (InObj && InObj != Py_None)
		{
			if (!UPyConversion::Nativize(InObj, OutString))
			{
				UPyUtil::SetPythonError(PyExc_TypeError, InErrorCtxt, InErrorMsg);
				return false;
			}
		}
		return true;
	}

	bool ConvertOptionalFunctionFlag(PyObject* InObj, EUPyUFunctionDefFlags& OutFlags, const EUPyUFunctionDefFlags InTrueFlagBit, const EUPyUFunctionDefFlags InFalseFlagBit, const TCHAR* InErrorCtxt, const TCHAR* InErrorMsg)
	{
		if (InObj && InObj != Py_None)
		{
			bool bFlagValue = false;
			if (!UPyConversion::Nativize(InObj, bFlagValue))
			{
				UPyUtil::SetPythonError(PyExc_TypeError, InErrorCtxt, InErrorMsg);
				return false;
			}
			OutFlags |= (bFlagValue ? InTrueFlagBit : InFalseFlagBit);
		}
		return true;
	}

	bool ConvertOptionalLifetimeCondition(PyObject* InObj, ELifetimeCondition& OutCondition, const TCHAR* InErrorCtxt)
	{
		OutCondition = COND_None;
		if (!InObj || InObj == Py_None)
		{
			return true;
		}
		if (!PyUnicode_Check(InObj))
		{
			UPyUtil::SetPythonError(PyExc_TypeError, InErrorCtxt, TEXT("Failed to convert parameter 'ReplicationCondition' (expected 'None' or 'str')"));
			return false;
		}

		const FString ConditionName = UPyUtil::PyObjectToUEString(InObj);
		static const TMap<FString, ELifetimeCondition> Conditions = {
			{ TEXT("None"), COND_None },
			{ TEXT("COND_None"), COND_None },
			{ TEXT("InitialOnly"), COND_InitialOnly },
			{ TEXT("COND_InitialOnly"), COND_InitialOnly },
			{ TEXT("OwnerOnly"), COND_OwnerOnly },
			{ TEXT("COND_OwnerOnly"), COND_OwnerOnly },
			{ TEXT("SkipOwner"), COND_SkipOwner },
			{ TEXT("COND_SkipOwner"), COND_SkipOwner },
			{ TEXT("SimulatedOnly"), COND_SimulatedOnly },
			{ TEXT("COND_SimulatedOnly"), COND_SimulatedOnly },
			{ TEXT("AutonomousOnly"), COND_AutonomousOnly },
			{ TEXT("COND_AutonomousOnly"), COND_AutonomousOnly },
			{ TEXT("SimulatedOrPhysics"), COND_SimulatedOrPhysics },
			{ TEXT("COND_SimulatedOrPhysics"), COND_SimulatedOrPhysics },
			{ TEXT("InitialOrOwner"), COND_InitialOrOwner },
			{ TEXT("COND_InitialOrOwner"), COND_InitialOrOwner },
			{ TEXT("Custom"), COND_Custom },
			{ TEXT("COND_Custom"), COND_Custom },
			{ TEXT("ReplayOrOwner"), COND_ReplayOrOwner },
			{ TEXT("COND_ReplayOrOwner"), COND_ReplayOrOwner },
			{ TEXT("ReplayOnly"), COND_ReplayOnly },
			{ TEXT("COND_ReplayOnly"), COND_ReplayOnly },
			{ TEXT("SimulatedOnlyNoReplay"), COND_SimulatedOnlyNoReplay },
			{ TEXT("COND_SimulatedOnlyNoReplay"), COND_SimulatedOnlyNoReplay },
			{ TEXT("SimulatedOrPhysicsNoReplay"), COND_SimulatedOrPhysicsNoReplay },
			{ TEXT("COND_SimulatedOrPhysicsNoReplay"), COND_SimulatedOrPhysicsNoReplay },
			{ TEXT("SkipReplay"), COND_SkipReplay },
			{ TEXT("COND_SkipReplay"), COND_SkipReplay },
			{ TEXT("Dynamic"), COND_Dynamic },
			{ TEXT("COND_Dynamic"), COND_Dynamic },
			{ TEXT("Never"), COND_Never },
			{ TEXT("COND_Never"), COND_Never },
		};

		if (const ELifetimeCondition* FoundCondition = Conditions.Find(ConditionName))
		{
			OutCondition = *FoundCondition;
			return true;
		}

		UPyUtil::SetPythonError(PyExc_ValueError, InErrorCtxt, *FString::Printf(TEXT("Unknown ReplicationCondition '%s'"), *ConditionName));
		return false;
	}

	bool ConvertOptionalRepNotifyCondition(PyObject* InObj, ELifetimeRepNotifyCondition& OutCondition, const TCHAR* InErrorCtxt)
	{
		OutCondition = REPNOTIFY_OnChanged;
		if (!InObj || InObj == Py_None)
		{
			return true;
		}
		if (!PyUnicode_Check(InObj))
		{
			UPyUtil::SetPythonError(PyExc_TypeError, InErrorCtxt, TEXT("Failed to convert parameter 'RepNotifyCondition' (expected 'None' or 'str')"));
			return false;
		}

		const FString ConditionName = UPyUtil::PyObjectToUEString(InObj);
		if (ConditionName == TEXT("OnChanged") || ConditionName == TEXT("REPNOTIFY_OnChanged"))
		{
			OutCondition = REPNOTIFY_OnChanged;
			return true;
		}
		if (ConditionName == TEXT("Always") || ConditionName == TEXT("REPNOTIFY_Always"))
		{
			OutCondition = REPNOTIFY_Always;
			return true;
		}

		UPyUtil::SetPythonError(PyExc_ValueError, InErrorCtxt, *FString::Printf(TEXT("Unknown RepNotifyCondition '%s'"), *ConditionName));
		return false;
	}

	void ApplyMetaData(PyObject* InMetaData, const TFunctionRef<void(const FString&, const FString&)>& InPredicate)
	{
		if (PyDict_Check(InMetaData))
		{
			PyObject* MetaDataKey = nullptr;
			PyObject* MetaDataValue = nullptr;
			Py_ssize_t MetaDataIndex = 0;
			while (PyDict_Next(InMetaData, &MetaDataIndex, &MetaDataKey, &MetaDataValue))
			{
				const FString MetaDataKeyStr = UPyUtil::PyObjectToUEString(MetaDataKey);
				const FString MetaDataValueStr = UPyUtil::PyObjectToUEString(MetaDataValue);
				InPredicate(MetaDataKeyStr, MetaDataValueStr);
			}
		}
	}

} // namespace UPyCoreUtil

FUPyDelegateHandle* FUPyDelegateHandle::CreateInstance(const FDelegateHandle& InValue)
{
	FUPyDelegateHandlePtr NewInstance = FUPyDelegateHandlePtr::StealReference(FUPyDelegateHandle::New(&UPyDelegateHandleType));
	if (NewInstance)
	{
		if (FUPyDelegateHandle::Init(NewInstance, InValue) != 0)
		{
			UPyUtil::LogPythonError();
			return nullptr;
		}
	}
	return NewInstance.Release();
}

FUPyDelegateHandle* FUPyDelegateHandle::CastPyObject(PyObject* InPyObject)
{
	if (PyObject_IsInstance(InPyObject, (PyObject*)&UPyDelegateHandleType) == 1)
	{
		Py_INCREF(InPyObject);
		return (FUPyDelegateHandle*)InPyObject;
	}

	return nullptr;
}


FUPyScopedSlowTask* FUPyScopedSlowTask::New(PyTypeObject* InType)
{
	FUPyScopedSlowTask* Self = (FUPyScopedSlowTask*)InType->tp_alloc(InType, 0);
	if (Self)
	{
		Self->SlowTask = nullptr;
	}
	return Self;
}

void FUPyScopedSlowTask::Free(FUPyScopedSlowTask* InSelf)
{
	Deinit(InSelf);

	Py_TYPE(InSelf)->tp_free((PyObject*)InSelf);
}

int FUPyScopedSlowTask::Init(FUPyScopedSlowTask* InSelf, const float InAmountOfWork, const FText& InDefaultMessage, const bool InEnabled)
{
	Deinit(InSelf);

	InSelf->SlowTask = new FSlowTask(InAmountOfWork, InDefaultMessage, InEnabled);

	return 0;
}

void FUPyScopedSlowTask::Deinit(FUPyScopedSlowTask* InSelf)
{
	delete InSelf->SlowTask;
	InSelf->SlowTask = nullptr;
}

bool FUPyScopedSlowTask::ValidateInternalState(FUPyScopedSlowTask* InSelf)
{
	if (!InSelf->SlowTask)
	{
		UPyUtil::SetPythonError(PyExc_Exception, Py_TYPE(InSelf), TEXT("Internal Error - SlowTask is null!"));
		return false;
	}

	return true;
}

template <typename ObjectType, typename SelfType>
bool PyTypeIterator_PassesFilter(SelfType* InSelf)
{
	FPersistentThreadSafeObjectIterator& Iter = *InSelf->Iterator;
	ObjectType* IterObj = CastChecked<ObjectType>(*Iter);
	return !InSelf->IteratorFilter || IterObj->IsChildOf(InSelf->IteratorFilter);
}


bool FUPyClassIterator::PassesFilter(FUPyClassIterator* InSelf)
{
	return PyTypeIterator_PassesFilter<UClass, FUPyClassIterator>(InSelf);
}

UClass* FUPyClassIterator::ExtractFilter(FUPyClassIterator* InSelf, PyObject* InPyFilter)
{
	UClass* IterFilter = nullptr;
	if (!UPyConversion::NativizeClass(InPyFilter, IterFilter, nullptr))
	{
		UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'type' (%s) to 'Class'"), *UPyUtil::GetFriendlyTypename(InPyFilter)));
	}
	return IterFilter;
}


bool FUPyStructIterator::PassesFilter(FUPyStructIterator* InSelf)
{
	return PyTypeIterator_PassesFilter<UScriptStruct, FUPyStructIterator>(InSelf);
}

UScriptStruct* FUPyStructIterator::ExtractFilter(FUPyStructIterator* InSelf, PyObject* InPyFilter)
{
	UScriptStruct* IterFilter = nullptr;
	if (!UPyConversion::NativizeStruct(InPyFilter, IterFilter, nullptr))
	{
		UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'type' (%s) to 'Struct'"), *UPyUtil::GetFriendlyTypename(InPyFilter)));
	}
	return IterFilter;
}


PyObject* FUPyTypeIterator::GetIterValue(FUPyTypeIterator* InSelf)
{
	FPersistentThreadSafeObjectIterator& Iter = *InSelf->Iterator;
	UStruct* IterObj = CastChecked<UStruct>(*Iter);

	const PyTypeObject* IterType = nullptr;
	if (const UClass* IterClass = Cast<UClass>(IterObj))
	{
		IterType = FUPyWrapperTypeRegistry::Get().GetWrappedClassType(IterClass);
	}
	if (const UScriptStruct* IterStruct = Cast<UScriptStruct>(IterObj))
	{
		IterType = FUPyWrapperTypeRegistry::Get().GetWrappedStructType(IterStruct);
	}
	check(IterType);

	Py_INCREF(IterType);
	return (PyObject*)IterType;
}

bool FUPyTypeIterator::PassesFilter(FUPyTypeIterator* InSelf)
{
	if (!PyTypeIterator_PassesFilter<UStruct, FUPyTypeIterator>(InSelf))
	{
		return false;
	}

	FPersistentThreadSafeObjectIterator& Iter = *InSelf->Iterator;
	UStruct* IterObj = CastChecked<UStruct>(*Iter);

	if (const UClass* IterClass = Cast<UClass>(IterObj))
	{
		return FUPyWrapperTypeRegistry::Get().HasWrappedClassType(IterClass);
	}
	if (const UScriptStruct* IterStruct = Cast<UScriptStruct>(IterObj))
	{
		return FUPyWrapperTypeRegistry::Get().HasWrappedStructType(IterStruct);
	}

	return false;
}

UStruct* FUPyTypeIterator::ExtractFilter(FUPyTypeIterator* InSelf, PyObject* InPyFilter)
{
	const UStruct* IterFilter = nullptr;
	if (PyType_Check(InPyFilter))
	{
		if (PyType_IsSubtype((PyTypeObject*)InPyFilter, &UPyWrapperObjectBaseType))
		{
			IterFilter = FUPyWrapperTypeRegistry::Get().FindClass((PyTypeObject*)InPyFilter);
		}
		else if (PyType_IsSubtype((PyTypeObject*)InPyFilter, &UPyWrapperStructType))
		{
			IterFilter = FUPyWrapperTypeRegistry::Get().FindStruct((PyTypeObject*)InPyFilter);
		}
	}
	else
	{
		UPyConversion::NativizeObject(InPyFilter, (UObject*&)IterFilter, nullptr);
	}
	if (!IterFilter || !(IterFilter->IsA<UClass>() || IterFilter->IsA<UScriptStruct>()))
	{
		UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'type' (%s) to 'Class' or 'Struct'"), *UPyUtil::GetFriendlyTypename(InPyFilter)));
	}
	return (UStruct*)IterFilter;
}


FUPyUValueDef* FUPyUValueDef::New(PyTypeObject* InType)
{
	FUPyUValueDef* Self = (FUPyUValueDef*)InType->tp_alloc(InType, 0);
	if (Self)
	{
		Self->Value = nullptr;
		Self->MetaData = nullptr;
	}
	return Self;
}

void FUPyUValueDef::Free(FUPyUValueDef* InSelf)
{
	PyObject_GC_UnTrack(InSelf);
	Deinit(InSelf);
	Py_TYPE(InSelf)->tp_free((PyObject*)InSelf);
}

int FUPyUValueDef::Init(FUPyUValueDef* InSelf, PyObject* InValue, PyObject* InMetaData)
{
	Deinit(InSelf);

	Py_INCREF(InValue);
	InSelf->Value = InValue;

	Py_INCREF(InMetaData);
	InSelf->MetaData = InMetaData;

	return 0;
}

int FUPyUValueDef::PyInit(FUPyUValueDef* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	PyObject* PyValueObj = nullptr;
	PyObject* PyMetaObj = nullptr;

	static const char *ArgsKwdList[] = { "Val", "Meta", nullptr };
	if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "O|O:Call", (char**)ArgsKwdList, &PyValueObj, &PyMetaObj))
	{
		return -1;
	}

	if (PyValueObj == Py_None)
	{
		UPyUtil::SetPythonError(PyExc_Exception, InSelf, TEXT("'Val' cannot be 'None'"));
		return -1;
	}

	if (!PyMetaObj)
	{
		PyMetaObj = Py_None;
	}

	return Init(InSelf, PyValueObj, PyMetaObj);
}

void FUPyUValueDef::Deinit(FUPyUValueDef* InSelf)
{
	GCClear(InSelf);
}

int FUPyUValueDef::GCTraverse(FUPyUValueDef* InSelf, visitproc InVisit, void* InArg)
{
	visitproc visit = InVisit;
	void* arg = InArg;

	Py_VISIT(InSelf->Value);
	Py_VISIT(InSelf->MetaData);
	return 0;
}

int FUPyUValueDef::GCClear(FUPyUValueDef* InSelf)
{
	Py_CLEAR(InSelf->Value);
	Py_CLEAR(InSelf->MetaData);
	return 0;
}

void FUPyUValueDef::ApplyMetaData(FUPyUValueDef* InSelf, const TFunctionRef<void(const FString&, const FString&)>& InPredicate)
{
#if WITH_EDITOR
	UPyCoreUtil::ApplyMetaData(InSelf->MetaData, InPredicate);
#endif
}

FUPyFPropertyDef* FUPyFPropertyDef::New(PyTypeObject* InType)
{
	FUPyFPropertyDef* Self = (FUPyFPropertyDef*)InType->tp_alloc(InType, 0);
	if (Self)
	{
		Self->PropType = nullptr;
		Self->MetaData = nullptr;
		new(&Self->GetterFuncName) FString();
		new(&Self->SetterFuncName) FString();
		new(&Self->RepNotifyFuncName) FString();
		Self->bReplicated = false;
		Self->bRepNotify = false;
		Self->ReplicationCondition = COND_None;
		Self->RepNotifyCondition = REPNOTIFY_OnChanged;
		Self->bPushBased = false;
	}
	return Self;
}

void FUPyFPropertyDef::Free(FUPyFPropertyDef* InSelf)
{
	PyObject_GC_UnTrack(InSelf);
	Deinit(InSelf);

	InSelf->GetterFuncName.~FString();
	InSelf->SetterFuncName.~FString();
	InSelf->RepNotifyFuncName.~FString();
	Py_TYPE(InSelf)->tp_free((PyObject*)InSelf);
}

int FUPyFPropertyDef::Init(FUPyFPropertyDef* InSelf, PyObject* InPropType, PyObject* InMetaData, FString InGetterFuncName, FString InSetterFuncName, bool bInReplicated, FString InRepNotifyFuncName, bool bInRepNotify, ELifetimeCondition InReplicationCondition, ELifetimeRepNotifyCondition InRepNotifyCondition, bool bInPushBased)
{
	Deinit(InSelf);

	Py_INCREF(InPropType);
	InSelf->PropType = InPropType;

	Py_INCREF(InMetaData);
	InSelf->MetaData = InMetaData;

	InSelf->GetterFuncName = MoveTemp(InGetterFuncName);
	InSelf->SetterFuncName = MoveTemp(InSetterFuncName);
	InSelf->bReplicated = bInReplicated || bInRepNotify;
	InSelf->RepNotifyFuncName = MoveTemp(InRepNotifyFuncName);
	InSelf->bRepNotify = bInRepNotify;
	InSelf->ReplicationCondition = InReplicationCondition;
	InSelf->RepNotifyCondition = InRepNotifyCondition;
	InSelf->bPushBased = bInPushBased;

	return 0;
}

int FUPyFPropertyDef::PyInit(FUPyFPropertyDef* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	PyObject* PyPropTypeObj = nullptr;
	PyObject* PyMetaObj = nullptr;
	PyObject* PyPropGetterObj = nullptr;
	PyObject* PyPropSetterObj = nullptr;
	PyObject* PyPropReplicatedObj = nullptr;
	PyObject* PyPropRepNotifyObj = nullptr;
	PyObject* PyPropReplicationConditionObj = nullptr;
	PyObject* PyPropRepNotifyConditionObj = nullptr;
	PyObject* PyPropPushBasedObj = nullptr;

	static const char *ArgsKwdList[] = { "Type", "Meta", "Getter", "Setter", "Replicated", "RepNotify", "ReplicationCondition", "RepNotifyCondition", "PushBased", nullptr };
	if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "O|OOOOOOOO:call", (char**)ArgsKwdList, &PyPropTypeObj, &PyMetaObj, &PyPropGetterObj, &PyPropSetterObj, &PyPropReplicatedObj, &PyPropRepNotifyObj, &PyPropReplicationConditionObj, &PyPropRepNotifyConditionObj, &PyPropPushBasedObj))
	{
		return -1;
	}

	if (!PyMetaObj)
	{
		PyMetaObj = Py_None;
	}

	const FString ErrorCtxt = UPyUtil::GetErrorContext(InSelf);

	FString PropGetter;
	if (!UPyCoreUtil::ConvertOptionalString(PyPropGetterObj, PropGetter, *ErrorCtxt, TEXT("Failed to convert parameter 'Getter' to a string (expected 'None' or 'str')")))
	{
		return -1;
	}

	FString PropSetter;
	if (!UPyCoreUtil::ConvertOptionalString(PyPropSetterObj, PropSetter, *ErrorCtxt, TEXT("Failed to convert parameter 'Setter' to a string (expected 'None' or 'str')")))
	{
		return -1;
	}

	bool bPropReplicated = false;
	if (PyPropReplicatedObj && PyPropReplicatedObj != Py_None && !UPyConversion::Nativize(PyPropReplicatedObj, bPropReplicated))
	{
		UPyUtil::SetPythonError(PyExc_TypeError, *ErrorCtxt, TEXT("Failed to convert parameter 'Replicated' to a bool (expected 'None' or 'bool')"));
		return -1;
	}

	bool bPropRepNotify = false;
	FString PropRepNotifyFuncName;
	if (PyPropRepNotifyObj && PyPropRepNotifyObj != Py_None)
	{
		if (PyUnicode_Check(PyPropRepNotifyObj))
		{
			PropRepNotifyFuncName = UPyUtil::PyObjectToUEString(PyPropRepNotifyObj);
			if (PropRepNotifyFuncName.IsEmpty())
			{
				UPyUtil::SetPythonError(PyExc_ValueError, *ErrorCtxt, TEXT("Parameter 'RepNotify' cannot be an empty string"));
				return -1;
			}
			bPropRepNotify = true;
		}
		else if (PyBool_Check(PyPropRepNotifyObj))
		{
			bPropRepNotify = PyObject_IsTrue(PyPropRepNotifyObj) == 1;
		}
		else
		{
			UPyUtil::SetPythonError(PyExc_TypeError, *ErrorCtxt, TEXT("Failed to convert parameter 'RepNotify' (expected 'None', 'bool', or 'str')"));
			return -1;
		}
	}

	ELifetimeCondition ReplicationCondition = COND_None;
	if (!UPyCoreUtil::ConvertOptionalLifetimeCondition(PyPropReplicationConditionObj, ReplicationCondition, *ErrorCtxt))
	{
		return -1;
	}

	ELifetimeRepNotifyCondition RepNotifyCondition = REPNOTIFY_OnChanged;
	if (!UPyCoreUtil::ConvertOptionalRepNotifyCondition(PyPropRepNotifyConditionObj, RepNotifyCondition, *ErrorCtxt))
	{
		return -1;
	}
	if (PyPropRepNotifyConditionObj && PyPropRepNotifyConditionObj != Py_None && !bPropRepNotify)
	{
		UPyUtil::SetPythonError(PyExc_ValueError, *ErrorCtxt, TEXT("Parameter 'RepNotifyCondition' requires 'RepNotify'"));
		return -1;
	}

	bool bPropPushBased = false;
	if (PyPropPushBasedObj && PyPropPushBasedObj != Py_None && !UPyConversion::Nativize(PyPropPushBasedObj, bPropPushBased))
	{
		UPyUtil::SetPythonError(PyExc_TypeError, *ErrorCtxt, TEXT("Failed to convert parameter 'PushBased' to a bool (expected 'None' or 'bool')"));
		return -1;
	}
	if ((PyPropReplicationConditionObj && PyPropReplicationConditionObj != Py_None) || bPropPushBased)
	{
		bPropReplicated = true;
	}

	return Init(InSelf, PyPropTypeObj, PyMetaObj, MoveTemp(PropGetter), MoveTemp(PropSetter), bPropReplicated, MoveTemp(PropRepNotifyFuncName), bPropRepNotify, ReplicationCondition, RepNotifyCondition, bPropPushBased);
}

void FUPyFPropertyDef::Deinit(FUPyFPropertyDef* InSelf)
{
	GCClear(InSelf);

	InSelf->GetterFuncName.Reset();
	InSelf->SetterFuncName.Reset();
	InSelf->RepNotifyFuncName.Reset();
	InSelf->bReplicated = false;
	InSelf->bRepNotify = false;
	InSelf->ReplicationCondition = COND_None;
	InSelf->RepNotifyCondition = REPNOTIFY_OnChanged;
	InSelf->bPushBased = false;
}

int FUPyFPropertyDef::GCTraverse(FUPyFPropertyDef* InSelf, visitproc InVisit, void* InArg)
{
	visitproc visit = InVisit;
	void* arg = InArg;

	Py_VISIT(InSelf->PropType);
	Py_VISIT(InSelf->MetaData);
	return 0;
}

int FUPyFPropertyDef::GCClear(FUPyFPropertyDef* InSelf)
{
	Py_CLEAR(InSelf->PropType);
	Py_CLEAR(InSelf->MetaData);
	return 0;
}

void FUPyFPropertyDef::ApplyMetaData(FUPyFPropertyDef* InSelf, FProperty* InProp)
{
	if (InSelf->bReplicated)
	{
		InProp->PropertyFlags |= CPF_Net;
	}
	if (InSelf->bRepNotify)
	{
		InProp->PropertyFlags |= CPF_RepNotify;
	}

#if WITH_EDITOR
	UPyCoreUtil::ApplyMetaData(InSelf->MetaData, [InProp](const FString& InMetaDataKey, const FString& InMetaDataValue)
	{
		InProp->SetMetaData(*InMetaDataKey, *InMetaDataValue);
	});
#endif
}

FUPyUFunctionDef* FUPyUFunctionDef::New(PyTypeObject* InType)
{
	FUPyUFunctionDef* Self = (FUPyUFunctionDef*)InType->tp_alloc(InType, 0);
	if (Self)
	{
		Self->Func = nullptr;
		Self->FuncRetType = nullptr;
		Self->FuncParamTypes = nullptr;
		Self->MetaData = nullptr;
		Self->FuncFlags = EUPyUFunctionDefFlags::None;
	}
	return Self;
}

void FUPyUFunctionDef::Free(FUPyUFunctionDef* InSelf)
{
	PyObject_GC_UnTrack(InSelf);
	Deinit(InSelf);

	Py_TYPE(InSelf)->tp_free((PyObject*)InSelf);
}

int FUPyUFunctionDef::Init(FUPyUFunctionDef* InSelf, PyObject* InFunc, PyObject* InFuncRetType, PyObject* InFuncParamTypes, PyObject* InMetaData, EUPyUFunctionDefFlags InFuncFlags)
{
	Deinit(InSelf);

	Py_INCREF(InFunc);
	InSelf->Func = InFunc;

	Py_INCREF(InFuncRetType);
	InSelf->FuncRetType = InFuncRetType;

	Py_INCREF(InFuncParamTypes);
	InSelf->FuncParamTypes = InFuncParamTypes;

	Py_INCREF(InMetaData);
	InSelf->MetaData = InMetaData;

	InSelf->FuncFlags = InFuncFlags;

	return 0;
}

int FUPyUFunctionDef::PyInit(FUPyUFunctionDef* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	PyObject* PyFuncObj = nullptr;
	PyObject* PyMetaObj = nullptr;
	PyObject* PyFuncRetTypeObj = nullptr;
	PyObject* PyFuncParamTypesObj = nullptr;
	PyObject* PyFuncOverrideObj = nullptr;
	PyObject* PyFuncStaticObj = nullptr;
	PyObject* PyFuncPureObj = nullptr;
	PyObject* PyFuncGetterObj = nullptr;
	PyObject* PyFuncSetterObj = nullptr;
	PyObject* PyFuncServerObj = nullptr;
	PyObject* PyFuncClientObj = nullptr;
	PyObject* PyFuncNetMulticastObj = nullptr;
	PyObject* PyFuncReliableObj = nullptr;
	PyObject* PyFuncUnreliableObj = nullptr;

	static const char *ArgsKwdList[] = { "Func", "Meta", "Ret", "Params", "Override", "Static", "Pure", "Getter", "Setter", "Server", "Client", "NetMulticast", "Reliable", "Unreliable", nullptr };
	if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "O|OOOOOOOOOOOOO:call", (char**)ArgsKwdList, &PyFuncObj, &PyMetaObj, &PyFuncRetTypeObj, &PyFuncParamTypesObj, &PyFuncOverrideObj, &PyFuncStaticObj, &PyFuncPureObj, &PyFuncGetterObj, &PyFuncSetterObj, &PyFuncServerObj, &PyFuncClientObj, &PyFuncNetMulticastObj, &PyFuncReliableObj, &PyFuncUnreliableObj))
	{
		return -1;
	}

	if (!PyMetaObj)
	{
		PyMetaObj = Py_None;
	}

	if (!PyFuncRetTypeObj)
	{
		PyFuncRetTypeObj = Py_None;
	}

	if (!PyFuncParamTypesObj)
	{
		PyFuncParamTypesObj = Py_None;
	}

	const FString ErrorCtxt = UPyUtil::GetErrorContext(InSelf);

	EUPyUFunctionDefFlags FuncFlags = EUPyUFunctionDefFlags::None;
	if (!UPyCoreUtil::ConvertOptionalFunctionFlag(PyFuncOverrideObj, FuncFlags, EUPyUFunctionDefFlags::Override, EUPyUFunctionDefFlags::None, *ErrorCtxt, TEXT("Failed to convert parameter 'override' to a flag (expected 'None' or 'bool')")))
	{
		return -1;
	}
	if (!UPyCoreUtil::ConvertOptionalFunctionFlag(PyFuncStaticObj, FuncFlags, EUPyUFunctionDefFlags::Static, EUPyUFunctionDefFlags::None, *ErrorCtxt, TEXT("Failed to convert parameter 'static' to a flag (expected 'None' or 'bool')")))
	{
		return -1;
	}
	if (!UPyCoreUtil::ConvertOptionalFunctionFlag(PyFuncPureObj, FuncFlags, EUPyUFunctionDefFlags::Pure, EUPyUFunctionDefFlags::Impure, *ErrorCtxt, TEXT("Failed to convert parameter 'pure' to a flag (expected 'None' or 'bool')")))
	{
		return -1;
	}
	if (!UPyCoreUtil::ConvertOptionalFunctionFlag(PyFuncGetterObj, FuncFlags, EUPyUFunctionDefFlags::Getter, EUPyUFunctionDefFlags::None, *ErrorCtxt, TEXT("Failed to convert parameter 'getter' to a flag (expected 'None' or 'bool')")))
	{
		return -1;
	}
	if (!UPyCoreUtil::ConvertOptionalFunctionFlag(PyFuncSetterObj, FuncFlags, EUPyUFunctionDefFlags::Setter, EUPyUFunctionDefFlags::None, *ErrorCtxt, TEXT("Failed to convert parameter 'setter' to a flag (expected 'None' or 'bool')")))
	{
		return -1;
	}
	if (!UPyCoreUtil::ConvertOptionalFunctionFlag(PyFuncServerObj, FuncFlags, EUPyUFunctionDefFlags::Server, EUPyUFunctionDefFlags::None, *ErrorCtxt, TEXT("Failed to convert parameter 'Server' to a flag (expected 'None' or 'bool')")))
	{
		return -1;
	}
	if (!UPyCoreUtil::ConvertOptionalFunctionFlag(PyFuncClientObj, FuncFlags, EUPyUFunctionDefFlags::Client, EUPyUFunctionDefFlags::None, *ErrorCtxt, TEXT("Failed to convert parameter 'Client' to a flag (expected 'None' or 'bool')")))
	{
		return -1;
	}
	if (!UPyCoreUtil::ConvertOptionalFunctionFlag(PyFuncNetMulticastObj, FuncFlags, EUPyUFunctionDefFlags::NetMulticast, EUPyUFunctionDefFlags::None, *ErrorCtxt, TEXT("Failed to convert parameter 'NetMulticast' to a flag (expected 'None' or 'bool')")))
	{
		return -1;
	}
	if (!UPyCoreUtil::ConvertOptionalFunctionFlag(PyFuncReliableObj, FuncFlags, EUPyUFunctionDefFlags::Reliable, EUPyUFunctionDefFlags::None, *ErrorCtxt, TEXT("Failed to convert parameter 'Reliable' to a flag (expected 'None' or 'bool')")))
	{
		return -1;
	}
	if (!UPyCoreUtil::ConvertOptionalFunctionFlag(PyFuncUnreliableObj, FuncFlags, EUPyUFunctionDefFlags::Unreliable, EUPyUFunctionDefFlags::None, *ErrorCtxt, TEXT("Failed to convert parameter 'Unreliable' to a flag (expected 'None' or 'bool')")))
	{
		return -1;
	}

	return Init(InSelf, PyFuncObj, PyFuncRetTypeObj, PyFuncParamTypesObj, PyMetaObj, FuncFlags);
}

void FUPyUFunctionDef::Deinit(FUPyUFunctionDef* InSelf)
{
	GCClear(InSelf);

	InSelf->FuncFlags = EUPyUFunctionDefFlags::None;
}

int FUPyUFunctionDef::GCTraverse(FUPyUFunctionDef* InSelf, visitproc InVisit, void* InArg)
{
	visitproc visit = InVisit;
	void* arg = InArg;

	Py_VISIT(InSelf->Func);
	Py_VISIT(InSelf->FuncRetType);
	Py_VISIT(InSelf->FuncParamTypes);
	Py_VISIT(InSelf->MetaData);
	return 0;
}

int FUPyUFunctionDef::GCClear(FUPyUFunctionDef* InSelf)
{
	Py_CLEAR(InSelf->Func);
	Py_CLEAR(InSelf->FuncRetType);
	Py_CLEAR(InSelf->FuncParamTypes);
	Py_CLEAR(InSelf->MetaData);
	return 0;
}

void FUPyUFunctionDef::ApplyMetaData(FUPyUFunctionDef* InSelf, UFunction* InFunc)
{
#if WITH_EDITOR
	UPyCoreUtil::ApplyMetaData(InSelf->MetaData, [InFunc](const FString& InMetaDataKey, const FString& InMetaDataValue)
	{
		InFunc->SetMetaData(*InMetaDataKey, *InMetaDataValue);
	});
#endif
}


void InitializeUPyScopedSlowTask(UPyGenUtil::FNativePythonModule& ModuleInfo)
{
	struct FFuncs
	{
		static PyObject* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
		{
			return (PyObject*)FUPyScopedSlowTask::New(InType);
		}

		static void Dealloc(FUPyScopedSlowTask* InSelf)
		{
			FUPyScopedSlowTask::Free(InSelf);
		}

		static int Init(FUPyScopedSlowTask* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			PyObject* PyWorkObj = nullptr;
			PyObject* PyDescObj = nullptr;
			PyObject* PyEnabledObj = nullptr;

			static const char *ArgsKwdList[] = { "work", "desc", "enabled", nullptr };
			if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "O|OO:call", (char**)ArgsKwdList, &PyWorkObj, &PyDescObj, &PyEnabledObj))
			{
				return -1;
			}

			float Work = 0.0f;
			if (!UPyConversion::Nativize(PyWorkObj, Work))
			{
				UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'work' (%s) to 'float'"), *UPyUtil::GetFriendlyTypename(PyWorkObj)));
				return -1;
			}

			FText Desc;
			if (PyDescObj && !UPyConversion::Nativize(PyDescObj, Desc))
			{
				UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'desc' (%s) to 'Text'"), *UPyUtil::GetFriendlyTypename(PyDescObj)));
				return -1;
			}

			bool bEnabled = true;
			if (PyEnabledObj && !UPyConversion::Nativize(PyEnabledObj, bEnabled))
			{
				UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'enabled' (%s) to 'bool'"), *UPyUtil::GetFriendlyTypename(PyEnabledObj)));
				return -1;
			}

			return FUPyScopedSlowTask::Init(InSelf, Work, Desc, bEnabled);
		}
	};

	struct FMethods
	{
		static PyObject* EnterScope(FUPyScopedSlowTask* InSelf)
		{
			if (!FUPyScopedSlowTask::ValidateInternalState(InSelf))
			{
				return nullptr;
			}

			InSelf->SlowTask->Initialize();

			Py_INCREF(InSelf);
			return (PyObject*)InSelf;
		}

		static PyObject* ExitScope(FUPyScopedSlowTask* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			if (!FUPyScopedSlowTask::ValidateInternalState(InSelf))
			{
				return nullptr;
			}

			InSelf->SlowTask->Destroy();

			Py_RETURN_NONE;
		}

		static PyObject* MakeDialogDelayed(FUPyScopedSlowTask* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			if (!FUPyScopedSlowTask::ValidateInternalState(InSelf))
			{
				return nullptr;
			}

			PyObject* PyDelayObj = nullptr;
			PyObject* PyCanCancelObj = nullptr;
			PyObject* PyAllowInPIEObj = nullptr;

			static const char *ArgsKwdList[] = { "delay", "can_cancel", "allow_in_pie", nullptr };
			if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "O|OO:make_dialog_delayed", (char**)ArgsKwdList, &PyDelayObj, &PyCanCancelObj, &PyAllowInPIEObj))
			{
				return nullptr;
			}

			float Delay = 0.0f;
			if (!UPyConversion::Nativize(PyDelayObj, Delay))
			{
				UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'delay' (%s) to 'float'"), *UPyUtil::GetFriendlyTypename(PyDelayObj)));
				return nullptr;
			}

			bool bCanCancel = false;
			if (PyCanCancelObj && !UPyConversion::Nativize(PyCanCancelObj, bCanCancel))
			{
				UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'can_cancel' (%s) to 'bool'"), *UPyUtil::GetFriendlyTypename(PyCanCancelObj)));
				return nullptr;
			}

			bool bAllowInPIE = false;
			if (PyAllowInPIEObj && !UPyConversion::Nativize(PyAllowInPIEObj, bAllowInPIE))
			{
				UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'allow_in_pie' (%s) to 'bool'"), *UPyUtil::GetFriendlyTypename(PyAllowInPIEObj)));
				return nullptr;
			}

			InSelf->SlowTask->MakeDialogDelayed(Delay, bCanCancel, bAllowInPIE);

			Py_RETURN_NONE;
		}

		static PyObject* MakeDialog(FUPyScopedSlowTask* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			if (!FUPyScopedSlowTask::ValidateInternalState(InSelf))
			{
				return nullptr;
			}

			PyObject* PyCanCancelObj = nullptr;
			PyObject* PyAllowInPIEObj = nullptr;

			static const char *ArgsKwdList[] = { "can_cancel", "allow_in_pie", nullptr };
			if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "|OO:make_dialog", (char**)ArgsKwdList, &PyCanCancelObj, &PyAllowInPIEObj))
			{
				return nullptr;
			}

			bool bCanCancel = false;
			if (PyCanCancelObj && !UPyConversion::Nativize(PyCanCancelObj, bCanCancel))
			{
				UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'can_cancel' (%s) to 'bool'"), *UPyUtil::GetFriendlyTypename(PyCanCancelObj)));
				return nullptr;
			}

			bool bAllowInPIE = false;
			if (PyAllowInPIEObj && !UPyConversion::Nativize(PyAllowInPIEObj, bAllowInPIE))
			{
				UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'allow_in_pie' (%s) to 'bool'"), *UPyUtil::GetFriendlyTypename(PyAllowInPIEObj)));
				return nullptr;
			}

			InSelf->SlowTask->MakeDialog(bCanCancel, bAllowInPIE);

			Py_RETURN_NONE;
		}

		static PyObject* EnterProgressFrame(FUPyScopedSlowTask* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			if (!FUPyScopedSlowTask::ValidateInternalState(InSelf))
			{
				return nullptr;
			}

			PyObject* PyWorkObj = nullptr;
			PyObject* PyDescObj = nullptr;

			static const char *ArgsKwdList[] = { "work", "desc", nullptr };
			if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "|OO:enter_progress_frame", (char**)ArgsKwdList, &PyWorkObj, &PyDescObj))
			{
				return nullptr;
			}

			float Work = 1.0f;
			if (PyWorkObj && !UPyConversion::Nativize(PyWorkObj, Work))
			{
				UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'work' (%s) to 'float'"), *UPyUtil::GetFriendlyTypename(PyWorkObj)));
				return nullptr;
			}

			FText Desc;
			if (PyDescObj && !UPyConversion::Nativize(PyDescObj, Desc))
			{
				UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'description' (%s) to 'Text'"), *UPyUtil::GetFriendlyTypename(PyDescObj)));
				return nullptr;
			}

			InSelf->SlowTask->EnterProgressFrame(Work, Desc);

			Py_RETURN_NONE;
		}

		static PyObject* ShouldCancel(FUPyScopedSlowTask* InSelf)
		{
			if (!FUPyScopedSlowTask::ValidateInternalState(InSelf))
			{
				return nullptr;
			}

			const bool bShouldCancel = InSelf->SlowTask->ShouldCancel();
			return UPyConversion::Pythonize(bShouldCancel);
		}
	};

	static PyMethodDef PyMethods[] = {
		{ "__enter__", UPyCFunctionCast(&FMethods::EnterScope), METH_NOARGS, "__enter__(self) -> ScopedSlowTask -- begin this slow task" },
		{ "__exit__", UPyCFunctionCast(&FMethods::ExitScope), METH_VARARGS | METH_KEYWORDS, "__exit__(self, type: Optional[Type[BaseException]], value: Optional[BaseException], traceback: Optional[TracebackType]) -> bool -- end this slow task" },
		{ "make_dialog_delayed", UPyCFunctionCast(&FMethods::MakeDialogDelayed), METH_VARARGS | METH_KEYWORDS, "make_dialog_delayed(self, delay: float, can_cancel: bool = False, allow_in_pie: bool = False) -> None -- creates a new dialog for this slow task after the given time delay (in seconds). If the task completes before this time, no dialog will be shown" },
		{ "make_dialog", UPyCFunctionCast(&FMethods::MakeDialog), METH_VARARGS | METH_KEYWORDS, "make_dialog(self, can_cancel: bool = False, allow_in_pie: bool = False) -> None -- creates a new dialog for this slow task, if there is currently not one open" },
		{ "enter_progress_frame", UPyCFunctionCast(&FMethods::EnterProgressFrame), METH_VARARGS | METH_KEYWORDS, "enter_progress_frame(self, work: float = 1.0, desc: Union[Text, str] = \"\") -> None -- indicate that we are to enter a frame that will take up the specified amount of work (completes any previous frames)" },
		{ "should_cancel", UPyCFunctionCast(&FMethods::ShouldCancel), METH_NOARGS, "x.should_cancel() -> bool -- True if the user has requested that the slow task be canceled" },
		{ nullptr, nullptr, 0, nullptr }
	};

	PyTypeObject* PyType = &UPyScopedSlowTaskType;

	PyType->tp_new = (newfunc)&FFuncs::New;
	PyType->tp_dealloc = (destructor)&FFuncs::Dealloc;
	PyType->tp_init = (initproc)&FFuncs::Init;

	PyType->tp_methods = PyMethods;

	PyType->tp_flags = Py_TPFLAGS_DEFAULT;
	PyType->tp_doc = "Type used to create and managed a scoped slow task in Python";

	if (PyType_Ready(PyType) == 0)
	{
		ModuleInfo.AddType(PyType);
	}
}


void InitializeUPyObjectIterator(UPyGenUtil::FNativePythonModule& ModuleInfo)
{
	struct FFuncs
	{
		static PyObject* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
		{
			return (PyObject*)FUPyObjectIterator::New(InType);
		}

		static void Dealloc(FUPyObjectIterator* InSelf)
		{
			FUPyObjectIterator::Free(InSelf);
		}

		static int Init(FUPyObjectIterator* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			PyObject* PyTypeObj = nullptr;

			static const char *ArgsKwdList[] = { "type", nullptr };
			if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "|O:call", (char**)ArgsKwdList, &PyTypeObj))
			{
				return -1;
			}

			UClass* IterClass = UObject::StaticClass();
			if (PyTypeObj)
			{
				if (PyTypeObj == Py_None)
				{
					UPyUtil::SetPythonError(PyExc_Exception, InSelf, TEXT("'type' cannot be 'None'"));
					return -1;
				}
				if (!UPyConversion::NativizeClass(PyTypeObj, IterClass, nullptr))
				{
					UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'type' (%s) to 'Class'"), *UPyUtil::GetFriendlyTypename(PyTypeObj)));
					return -1;
				}
			}

			return FUPyObjectIterator::Init(InSelf, IterClass, nullptr);
		}

		static FUPyObjectIterator* GetIter(FUPyObjectIterator* InSelf)
		{
			return FUPyObjectIterator::GetIter(InSelf);
		}

		static PyObject* IterNext(FUPyObjectIterator* InSelf)
		{
			return FUPyObjectIterator::IterNext(InSelf);
		}
	};

	PyTypeObject* PyType = &UPyObjectIteratorType;

	PyType->tp_new = (newfunc)&FFuncs::New;
	PyType->tp_dealloc = (destructor)&FFuncs::Dealloc;
	PyType->tp_init = (initproc)&FFuncs::Init;
	PyType->tp_iter = (getiterfunc)&FFuncs::GetIter;
	PyType->tp_iternext = (iternextfunc)&FFuncs::IterNext;

	PyType->tp_flags = Py_TPFLAGS_DEFAULT;
	PyType->tp_doc = "Type for iterating Unreal Object instances";

	if (PyType_Ready(PyType) == 0)
	{
		ModuleInfo.AddType(PyType);
	}
}


template <typename ObjectType, typename SelfType>
PyTypeObject* InitializeUPyTypeIteratorType(PyTypeObject* InPyType, const char* InTypeDoc)
{
	struct FFuncs
	{
		static PyObject* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
		{
			return (PyObject*)SelfType::New(InType);
		}

		static void Dealloc(SelfType* InSelf)
		{
			SelfType::Free(InSelf);
		}

		static int Init(SelfType* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			PyObject* PyTypeObj = nullptr;

			static const char *ArgsKwdList[] = { "type", nullptr };
			if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "O:call", (char**)ArgsKwdList, &PyTypeObj))
			{
				return -1;
			}

			ObjectType* IterFilter = SelfType::ExtractFilter(InSelf, PyTypeObj);
			if (!IterFilter)
			{
				return -1;
			}

			return SelfType::Init(InSelf, ObjectType::StaticClass(), IterFilter);
		}

		static SelfType* GetIter(SelfType* InSelf)
		{
			return SelfType::GetIter(InSelf);
		}

		static PyObject* IterNext(SelfType* InSelf)
		{
			return SelfType::IterNext(InSelf);
		}
	};

	InPyType->tp_new = (newfunc)&FFuncs::New;
	InPyType->tp_dealloc = (destructor)&FFuncs::Dealloc;
	InPyType->tp_init = (initproc)&FFuncs::Init;
	InPyType->tp_iter = (getiterfunc)&FFuncs::GetIter;
	InPyType->tp_iternext = (iternextfunc)&FFuncs::IterNext;

	InPyType->tp_flags = Py_TPFLAGS_DEFAULT;
	InPyType->tp_doc = InTypeDoc;

	return InPyType;
}


void InitializeUPyUValueDef(UPyGenUtil::FNativePythonModule& ModuleInfo)
{
	struct FFuncs
	{
		static PyObject* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
		{
			return (PyObject*)FUPyUValueDef::New(InType);
		}

		static void Dealloc(FUPyUValueDef* InSelf)
		{
			FUPyUValueDef::Free(InSelf);
		}

		static int Init(FUPyUValueDef* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			return FUPyUValueDef::PyInit(InSelf, InArgs, InKwds);
		}

		static int Traverse(FUPyUValueDef* InSelf, visitproc InVisit, void* InArg)
		{
			return FUPyUValueDef::GCTraverse(InSelf, InVisit, InArg);
		}

		static int Clear(FUPyUValueDef* InSelf)
		{
			return FUPyUValueDef::GCClear(InSelf);
		}
	};

	PyTypeObject* PyType = &UPyUValueDefType;

	PyType->tp_new = (newfunc)&FFuncs::New;
	PyType->tp_dealloc = (destructor)&FFuncs::Dealloc;
	PyType->tp_init = (initproc)&FFuncs::Init;
	PyType->tp_traverse = (traverseproc)&FFuncs::Traverse;
	PyType->tp_clear = (inquiry)&FFuncs::Clear;
	PyType->tp_alloc = PyType_GenericAlloc;
	PyType->tp_free = PyObject_GC_Del;

	PyType->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC;
	PyType->tp_doc = "Type used to define constant values from Python";

	if (PyType_Ready(PyType) == 0)
	{
		ModuleInfo.AddType(PyType, true);
	}
}

void InitializeUPyFPropertyDef(UPyGenUtil::FNativePythonModule& ModuleInfo)
{
	struct FFuncs
	{
		static PyObject* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
		{
			return (PyObject*)FUPyFPropertyDef::New(InType);
		}

		static void Dealloc(FUPyFPropertyDef* InSelf)
		{
			FUPyFPropertyDef::Free(InSelf);
		}

		static int Init(FUPyFPropertyDef* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			return FUPyFPropertyDef::PyInit(InSelf, InArgs, InKwds);
		}

		static int Traverse(FUPyFPropertyDef* InSelf, visitproc InVisit, void* InArg)
		{
			return FUPyFPropertyDef::GCTraverse(InSelf, InVisit, InArg);
		}

		static int Clear(FUPyFPropertyDef* InSelf)
		{
			return FUPyFPropertyDef::GCClear(InSelf);
		}
	};

	PyTypeObject* PyType = &UPyFPropertyDefType;

	PyType->tp_new = (newfunc)&FFuncs::New;
	PyType->tp_dealloc = (destructor)&FFuncs::Dealloc;
	PyType->tp_init = (initproc)&FFuncs::Init;
	PyType->tp_traverse = (traverseproc)&FFuncs::Traverse;
	PyType->tp_clear = (inquiry)&FFuncs::Clear;
	PyType->tp_alloc = PyType_GenericAlloc;
	PyType->tp_free = PyObject_GC_Del;

	PyType->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC;
	PyType->tp_doc = "Type used to define FProperty fields from Python";

	if (PyType_Ready(PyType) == 0)
	{
		ModuleInfo.AddType(PyType, true);
	}
}


void InitializeUPyUFunctionDef(UPyGenUtil::FNativePythonModule& ModuleInfo)
{
	struct FFuncs
	{
		static PyObject* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
		{
			return (PyObject*)FUPyUFunctionDef::New(InType);
		}

		static void Dealloc(FUPyUFunctionDef* InSelf)
		{
			FUPyUFunctionDef::Free(InSelf);
		}

		static int Init(FUPyUFunctionDef* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			return FUPyUFunctionDef::PyInit(InSelf, InArgs, InKwds);
		}

		static int Traverse(FUPyUFunctionDef* InSelf, visitproc InVisit, void* InArg)
		{
			return FUPyUFunctionDef::GCTraverse(InSelf, InVisit, InArg);
		}

		static int Clear(FUPyUFunctionDef* InSelf)
		{
			return FUPyUFunctionDef::GCClear(InSelf);
		}
	};

	PyTypeObject* PyType = &UPyUFunctionDefType;

	PyType->tp_new = (newfunc)&FFuncs::New;
	PyType->tp_dealloc = (destructor)&FFuncs::Dealloc;
	PyType->tp_init = (initproc)&FFuncs::Init;
	PyType->tp_traverse = (traverseproc)&FFuncs::Traverse;
	PyType->tp_clear = (inquiry)&FFuncs::Clear;
	PyType->tp_alloc = PyType_GenericAlloc;
	PyType->tp_free = PyObject_GC_Del;

	PyType->tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC;
	PyType->tp_doc = "Type used to define UFunction fields from Python";

	if (PyType_Ready(PyType) == 0)
	{
		ModuleInfo.AddType(PyType, true);
	}
}

void InitializeUPyDelegateHandle(UPyGenUtil::FNativePythonModule& ModuleInfo)
{
	PyTypeObject* PyType = &UPyDelegateHandleType;
	InitializeUPyWrapperBasicType<FUPyDelegateHandle>(PyType, "Type for all Unreal exposed FDelegateHandle instances");
	PyType_Ready(PyType);
}

void InitializeUPyClassIterator(UPyGenUtil::FNativePythonModule& ModuleInfo)
{
	PyTypeObject* PyType = &UPyClassIteratorType;
	InitializeUPyTypeIteratorType<UClass, FUPyClassIterator>(&UPyClassIteratorType,"Type for iterating Unreal class types");
	if (PyType_Ready(PyType) == 0)
	{
		ModuleInfo.AddType(PyType);
	}
}

void InitializeUPyStructIterator(UPyGenUtil::FNativePythonModule& ModuleInfo)
{
	PyTypeObject* PyType = &UPyStructIteratorType;
	InitializeUPyTypeIteratorType<UScriptStruct, FUPyStructIterator>(&UPyStructIteratorType, "Type for iterating Unreal struct types");
	if (PyType_Ready(PyType) == 0)
	{
		ModuleInfo.AddType(PyType);
	}
}

void InitializeUPyTypeIterator(UPyGenUtil::FNativePythonModule& ModuleInfo)
{
	PyTypeObject* PyType = &UPyTypeIteratorType;
	InitializeUPyTypeIteratorType<UStruct, FUPyTypeIterator>(&UPyTypeIteratorType, "Type for iterating Python types");
	if (PyType_Ready(PyType) == 0)
	{
		ModuleInfo.AddType(PyType);
	}
}
