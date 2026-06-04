
#include "Subclassing/UPyGeneratedClass.h"
#include "Wrapper/UPyWrapperTypeRegistry.h"
#include "Wrapper/UPyWrapperTypeFactory.h"
#include "Core/UPyCore.h"
#include "Core/UPyGIL.h"
#if WITH_EDITOR
#include "BlueprintActionDatabase.h"
#endif

PyTypeObject UPyUClassDecoratorType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"UPyUClassDecorator", /* tp_name */
	sizeof(FUPyUClassDecorator), /* tp_basicsize */
};

PyTypeObject UPyUFunctionDecoratorType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"UPyUFunctionDecoratorType", /* tp_name */
	sizeof(FUPyUFunctionDecorator), /* tp_basicsize */
};

void InitializeUPyUClassDecorator(UPyGenUtil::FNativePythonModule& ModuleInfo)
{
	PyTypeObject* PyType = &UPyUClassDecoratorType;
	PyType->tp_flags = Py_TPFLAGS_DEFAULT;
	PyType->tp_new = (newfunc)FUPyUClassDecorator::New;
	PyType->tp_dealloc = (destructor)FUPyUClassDecorator::Dealloc;
	PyType->tp_init = (initproc)FUPyUClassDecorator::Init;
	PyType->tp_call = (ternaryfunc)FUPyUClassDecorator::Call;

	if (PyType_Ready(PyType) == 0)
	{
		ModuleInfo.AddType(PyType);
	}
}

void InitializeUPyUFunctionDecorator(UPyGenUtil::FNativePythonModule& ModuleInfo)
{
	PyTypeObject* PyType = &UPyUFunctionDecoratorType;
	PyType->tp_flags = Py_TPFLAGS_DEFAULT;
	PyType->tp_new = (newfunc)FUPyUFunctionDecorator::New;
	PyType->tp_dealloc = (destructor)FUPyUFunctionDecorator::Dealloc;
	PyType->tp_init = (initproc)FUPyUFunctionDecorator::Init;
	PyType->tp_call = (ternaryfunc)FUPyUFunctionDecorator::Call;

	if (PyType_Ready(PyType) == 0)
	{
		ModuleInfo.AddType(PyType);
	}
}

PyObject* PyCallGenerateClass(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	PyObject* PyObj = nullptr;
	if (!PyArg_ParseTuple(InArgs, "O:GenerateClass", &PyObj))
	{
		return nullptr;
	}
	check(PyObj);

	if (!PyType_Check(PyObj))
	{
		UPyUtil::SetPythonError(PyExc_TypeError, TEXT("GenerateClass"), *FString::Printf(TEXT("Parameter must be a 'type' not '%s'"), *UPyUtil::GetFriendlyTypename(PyObj)));
		return nullptr;
	}

	PyTypeObject* PyType = (PyTypeObject*)PyObj;
	if (!PyType_IsSubtype(PyType, &UPyWrapperObjectBaseType))
	{
		UPyUtil::SetPythonError(PyExc_Exception, TEXT("GenerateClass"), *FString::Printf(TEXT("Type '%s' does not derive from an Unreal class type"), *UPyUtil::GetFriendlyTypename(PyType)));
		return nullptr;
	}

	// We only need to generate classes for types that are not already registered.
	const bool bNeedsGeneration = !FUPyWrapperTypeRegistry::Get().FindClass(PyType);
	if (bNeedsGeneration && !UUPyGeneratedClass::GenerateClass(PyType))
	{
		UPyUtil::SetPythonError(PyExc_Exception, TEXT("GenerateClass"), *FString::Printf(TEXT("Failed to generate an Unreal class for the Python type '%s'"), *UPyUtil::GetFriendlyTypename(PyType)));
		return nullptr;
	}
	if (bNeedsGeneration)
	{
		Py_BEGIN_ALLOW_THREADS
		FUPyWrapperTypeReinstancer::Get().ProcessPending();
		Py_END_ALLOW_THREADS
	}

	Py_INCREF(PyType);
	return (PyObject*)PyType;
}

PyObject* PyCallGenerateFunction(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	FUPyUFunctionDef* PyFunction = FUPyUFunctionDef::New(&UPyUFunctionDefType);
	if (PyFunction)
	{
		if (PyFunction->PyInit(PyFunction, InArgs, InKwds) == 0)
		{
			return (PyObject*)PyFunction;
		}
		Py_CLEAR(PyFunction);
	}
	return nullptr;
}

class FUPythonGeneratedClassBuilder
{
public:
	FUPythonGeneratedClassBuilder(UClass* InSuperClass, PyTypeObject* InPyType)
		: ClassName()
		, PyType(InPyType)
		, OldClass(nullptr)
		, NewClass(nullptr)
	{
		UObject* ClassOuter = nullptr;
		UPyUtil::GetGeneratedTypeOuterAndName(PyType, ClassOuter, ClassName);

		// Find any existing class with the name we want to use
		OldClass = FindObject<UUPyGeneratedClass>(ClassOuter, *ClassName);

		// Create a new class with a temporary name; we will rename it as part of Finalize
		const FString NewClassName = MakeUniqueObjectName(ClassOuter, UUPyGeneratedClass::StaticClass(), *FString::Printf(TEXT("%s_NEWINST"), *ClassName)).ToString();
		NewClass = NewObject<UUPyGeneratedClass>(ClassOuter, *NewClassName, RF_Public | RF_Transient);
#if WITH_METADATA
		NewClass->SetMetaData(TEXT("DisplayName"), *UPyUtil::GetGeneratedTypeDisplayName(PyType));
		NewClass->SetMetaData(TEXT("BlueprintType"), TEXT("true"));
#endif
		NewClass->SetSuperStruct(InSuperClass);
	}

	FUPythonGeneratedClassBuilder(UUPyGeneratedClass* InOldClass, UClass* InSuperClass)
		: ClassName(InOldClass->GetName())
		, PyType(InOldClass->PyType)
		, OldClass(InOldClass)
		, NewClass(nullptr)
	{
		UObject* ClassOuter = InOldClass->GetOuter();

		// Create a new class with a temporary name; we will rename it as part of Finalize
		const FString NewClassName = MakeUniqueObjectName(ClassOuter, UUPyGeneratedClass::StaticClass(), *FString::Printf(TEXT("%s_NEWINST"), *ClassName)).ToString();
		NewClass = NewObject<UUPyGeneratedClass>(ClassOuter, *NewClassName, RF_Public | RF_Transient);
#if WITH_METADATA
		NewClass->SetMetaData(TEXT("DisplayName"), *UPyUtil::GetGeneratedTypeDisplayName(PyType));
		NewClass->SetMetaData(TEXT("BlueprintType"), TEXT("true"));
#endif
		NewClass->SetSuperStruct(InSuperClass);
	}

	~FUPythonGeneratedClassBuilder()
	{
		// If NewClass is still set at this point, if means Finalize wasn't called and we should destroy the partially built class
		if (NewClass)
		{
			NewClass->ClassFlags |= CLASS_NewerVersionExists | CLASS_TokenStreamAssembled;
			NewClass->SetFlags(RF_BeingRegenerated | RF_NewerVersionExists);
			NewClass->ClearFlags(RF_Public | RF_Standalone);
			NewClass->MarkAsGarbage();
			NewClass = nullptr;
		}
	}

	UUPyGeneratedClass* Finalize(FUPyObjectPtr InPyPostInitFunction)
	{
		// Set the post-init function
		NewClass->PyPostInitFunction = InPyPostInitFunction;

		// Replace the definitions with real descriptors
		if (!RegisterDescriptors())
		{
			return nullptr;
		}

		// Let Python know that we've changed its type
		PyType_Modified(PyType);

		// We can no longer fail, so prepare the old class for removal and set the correct name on the new class
		if (OldClass)
		{
			PrepareOldClassForReinstancing();
		}

		const FString CleanPyTypeName = UPyUtil::GetCleanTypename(PyType);

		FNameBuilder CleanCDOName;
		CleanCDOName.Append(DEFAULT_OBJECT_PREFIX);
		CleanCDOName.Append(CleanPyTypeName);

		Py_BEGIN_ALLOW_THREADS
		NewClass->ClassRedirector = UPyUtil::CreatePythonTypeLegacyRedirector(CleanPyTypeName, FTopLevelAssetPath(NewClass->GetOuter()->GetFName(), *ClassName));
		if (NewClass->ClassRedirector)
		{
			NewClass->ClassRedirector->DestinationObject = NewClass;
		}
		else
		{
			// If the class doesn't have a redirector, then we also need to remove any existing redirector for the CDO (before the real CDO is created below)
			if (UObjectRedirector* ExistingCDORedirector = FindObject<UObjectRedirector>(UPyUtil::GetPythonTypeContainer(), *CleanCDOName))
			{
				ExistingCDORedirector->DestinationObject = nullptr;
				ExistingCDORedirector->ClearFlags(RF_Public | RF_Standalone);
				ExistingCDORedirector->Rename(*FString::Printf(TEXT("%s_DELETED"), *CleanCDOName), nullptr, REN_DontCreateRedirectors);
			}
		}
		NewClass->Rename(*ClassName, nullptr, REN_DontCreateRedirectors);

		// Finalize the class
		NewClass->Bind();
		NewClass->StaticLink(true);
		NewClass->AssembleReferenceTokenStream();
		Py_END_ALLOW_THREADS

		// Add the object meta-data to the type
		// todo(hzn): meta data
		// NewClass->PyMetaData.Class = NewClass;
		// FPyWrapperObjectMetaData::SetMetaData(PyType, &NewClass->PyMetaData);

		// Map the Unreal class to the Python type
		NewClass->PyType = FUPyTypeObjectPtr::NewReference(PyType);
		FUPyWrapperTypeRegistry::Get().RegisterWrappedClassType(NewClass, PyType);

		// Ensure the CDO exists and add a redirector for it if needed
		Py_BEGIN_ALLOW_THREADS
		if (UObject* NewClassCDO = NewClass->GetDefaultObject())
		{
			if (NewClass->ClassRedirector)
			{
				// If the class has a redirector, then we also need to redirect the CDO
				NewClass->CDORedirector = NewObject<UObjectRedirector>(UPyUtil::GetPythonTypeContainer(), FName(CleanCDOName), RF_Public | RF_Transient);
				NewClass->CDORedirector->DestinationObject = NewClassCDO;
			}
		}
		Py_END_ALLOW_THREADS

		// Re-instance the old class and re-parent any derived classes to this new type
		if (OldClass)
		{
			FUPyWrapperTypeReinstancer::Get().AddPendingClass(OldClass, NewClass);
			UUPyGeneratedClass::ReparentDerivedClasses(OldClass, NewClass);
		}

#if WITH_EDITOR
		if (FBlueprintActionDatabase* ActionDB = FBlueprintActionDatabase::TryGet())
		{
			Py_BEGIN_ALLOW_THREADS
			// Notify Blueprints that there is a new class to add to the action list
			ActionDB->RefreshClassActions(NewClass);

			if (OldClass)
			{
				ActionDB->RefreshClassActions(OldClass);
			}
			Py_END_ALLOW_THREADS
		}
#endif

		// Null the NewClass pointer so the destructor doesn't kill it
		UUPyGeneratedClass* FinalizedClass = NewClass;
		NewClass = nullptr;
		return FinalizedClass;
	}

	bool CreatePropertyFromDefinition(const FString& InFieldName, FUPyFPropertyDef* InPyPropDef)
	{
		UClass* SuperClass = NewClass->GetSuperClass();

		// Resolve the property name to match any previously exported properties from the parent type
		const FName PropName = *InFieldName; // FPyWrapperObjectMetaData::ResolvePropertyName(PyType->tp_base, *InFieldName);
		if (SuperClass->FindPropertyByName(PropName))
		{
			UPyUtil::SetPythonError(PyExc_Exception, PyType, *FString::Printf(TEXT("Property '%s' (%s) cannot override a property from the base type"), *InFieldName, *UPyUtil::GetFriendlyTypename(InPyPropDef->PropType)));
			return false;
		}

		// Create the property from its definition
		FProperty* Prop = UPyUtil::CreateProperty(InPyPropDef->PropType, 1, NewClass, PropName);
		if (!Prop)
		{
			UPyUtil::SetPythonError(PyExc_Exception, PyType, *FString::Printf(TEXT("Failed to create property for '%s' (%s)"), *InFieldName, *UPyUtil::GetFriendlyTypename(InPyPropDef->PropType)));
			return false;
		}
		Prop->PropertyFlags |= (CPF_Edit | CPF_BlueprintVisible);
		FUPyFPropertyDef::ApplyMetaData(InPyPropDef, Prop);

		if (InPyPropDef->bRepNotify)
		{
			const FString RepNotifyFuncNameStr = InPyPropDef->RepNotifyFuncName.IsEmpty() ? FString::Printf(TEXT("OnRep_%s"), *InFieldName) : InPyPropDef->RepNotifyFuncName;
			const FName RepNotifyFuncName = *RepNotifyFuncNameStr;
			UFunction* RepNotifyFunc = NewClass->FindFunctionByName(RepNotifyFuncName);
			if (!RepNotifyFunc)
			{
				UPyUtil::SetPythonError(PyExc_Exception, PyType, *FString::Printf(TEXT("Property '%s' specified RepNotify function '%s', but no method with that name was found"), *InFieldName, *RepNotifyFuncNameStr));
				return false;
			}
			if (RepNotifyFunc->HasAnyFunctionFlags(FUNC_Static))
			{
				UPyUtil::SetPythonError(PyExc_Exception, PyType, *FString::Printf(TEXT("Property '%s' specified RepNotify function '%s', but RepNotify methods cannot be static"), *InFieldName, *RepNotifyFuncNameStr));
				return false;
			}
			if (RepNotifyFunc->GetReturnProperty())
			{
				UPyUtil::SetPythonError(PyExc_Exception, PyType, *FString::Printf(TEXT("Property '%s' specified RepNotify function '%s', but RepNotify methods cannot return a value"), *InFieldName, *RepNotifyFuncNameStr));
				return false;
			}

			const FProperty* RepNotifyParamProp = nullptr;
			int32 RepNotifyParamCount = 0;
			for (TFieldIterator<FProperty> ParamIt(RepNotifyFunc); ParamIt; ++ParamIt)
			{
				const FProperty* ParamProp = *ParamIt;
				if (ParamProp->HasAnyPropertyFlags(CPF_Parm) && !ParamProp->HasAnyPropertyFlags(CPF_ReturnParm))
				{
					RepNotifyParamProp = ParamProp;
					++RepNotifyParamCount;
				}
			}
			if (RepNotifyParamCount > 1)
			{
				UPyUtil::SetPythonError(PyExc_Exception, PyType, *FString::Printf(TEXT("Property '%s' specified RepNotify function '%s', but RepNotify methods can have at most one parameter"), *InFieldName, *RepNotifyFuncNameStr));
				return false;
			}
			if (RepNotifyParamProp && !RepNotifyParamProp->SameType(Prop))
			{
				UPyUtil::SetPythonError(PyExc_Exception, PyType, *FString::Printf(TEXT("Property '%s' specified RepNotify function '%s', but its parameter type '%s' does not match the property type '%s'"), *InFieldName, *RepNotifyFuncNameStr, *RepNotifyParamProp->GetClass()->GetName(), *Prop->GetClass()->GetName()));
				return false;
			}

			Prop->RepNotifyFunc = RepNotifyFuncName;
		}

		NewClass->AddCppProperty(Prop);
		if (InPyPropDef->bReplicated)
		{
			UUPyGeneratedClass::FReplicationDef& ReplicationDef = NewClass->ReplicationDefs.AddDefaulted_GetRef();
			ReplicationDef.PropertyName = PropName;
			ReplicationDef.ReplicationCondition = InPyPropDef->ReplicationCondition;
			ReplicationDef.RepNotifyCondition = InPyPropDef->RepNotifyCondition;
			ReplicationDef.bPushBased = InPyPropDef->bPushBased;
		}

		// Resolve any getter/setter function names
		const FName GetterFuncName = *InPyPropDef->GetterFuncName; // FPyWrapperObjectMetaData::ResolveFunctionName(PyType->tp_base, *InPyPropDef->GetterFuncName);
		const FName SetterFuncName = *InPyPropDef->SetterFuncName; // FPyWrapperObjectMetaData::ResolveFunctionName(PyType->tp_base, *InPyPropDef->SetterFuncName);

#if WITH_EDITOR
		if (!GetterFuncName.IsNone())
		{
			Prop->SetMetaData(UPyGenUtil::BlueprintGetterMetaDataKey, *GetterFuncName.ToString());
		}
		if (!SetterFuncName.IsNone())
		{
			Prop->SetMetaData(UPyGenUtil::BlueprintSetterMetaDataKey, *SetterFuncName.ToString());
		}
#endif

		// Build the definition data for the new property accessor
		UPyGenUtil::FPropertyDef& PropDef = *NewClass->PropertyDefs.Add_GetRef(MakeShared<UPyGenUtil::FPropertyDef>());
		PropDef.GeneratedWrappedGetSet.GetSetName = UPyGenUtil::TCHARToUTF8Buffer(*InFieldName);
		PropDef.GeneratedWrappedGetSet.GetSetDoc = UPyGenUtil::TCHARToUTF8Buffer(*FString::Printf(TEXT("type: %s\n%s"), *UPyGenUtil::GetPropertyPythonType(Prop), *UPyGenUtil::GetFieldTooltip(Prop)));
		PropDef.GeneratedWrappedGetSet.Prop.SetProperty(Prop);
		PropDef.GeneratedWrappedGetSet.GetFunc.SetFunction(NewClass->FindFunctionByName(GetterFuncName));
		PropDef.GeneratedWrappedGetSet.SetFunc.SetFunction(NewClass->FindFunctionByName(SetterFuncName));
		PropDef.GeneratedWrappedGetSet.GetCallback = (getter)&FUPyWrapperObjectBase::Getter_Impl;
		PropDef.GeneratedWrappedGetSet.SetCallback = (setter)&FUPyWrapperObjectBase::Setter_Impl;
		PropDef.GeneratedWrappedGetSet.ToPython(PropDef.PyGetSet);

		// If this property has a getter or setter, also make an internal version with the get/set function cleared so that Python can read/write the internal property value
		if (PropDef.GeneratedWrappedGetSet.GetFunc.Func || PropDef.GeneratedWrappedGetSet.SetFunc.Func)
		{
			UPyGenUtil::FPropertyDef& InternalPropDef = *NewClass->PropertyDefs.Add_GetRef(MakeShared<UPyGenUtil::FPropertyDef>());
			InternalPropDef.GeneratedWrappedGetSet.GetSetName = UPyGenUtil::TCHARToUTF8Buffer(*FString::Printf(TEXT("_%s"), *InFieldName));
			InternalPropDef.GeneratedWrappedGetSet.GetSetDoc = PropDef.GeneratedWrappedGetSet.GetSetDoc;
			InternalPropDef.GeneratedWrappedGetSet.Prop.SetProperty(Prop);
			InternalPropDef.GeneratedWrappedGetSet.GetCallback = (getter)&FUPyWrapperObjectBase::Getter_Impl;
			InternalPropDef.GeneratedWrappedGetSet.SetCallback = (setter)&FUPyWrapperObjectBase::Setter_Impl;
			InternalPropDef.GeneratedWrappedGetSet.ToPython(InternalPropDef.PyGetSet);
		}

		return true;
	}

	bool CreateFunctionFromDefinition(const FString& InFieldName, FUPyUFunctionDef* InPyFuncDef)
	{
		UClass* SuperClass = NewClass->GetSuperClass();

		// Validate the function definition makes sense
		if (EnumHasAllFlags(InPyFuncDef->FuncFlags, EUPyUFunctionDefFlags::Override))
		{
			if (EnumHasAnyFlags(InPyFuncDef->FuncFlags, EUPyUFunctionDefFlags::Static | EUPyUFunctionDefFlags::Getter | EUPyUFunctionDefFlags::Setter))
			{
				UPyUtil::SetPythonError(PyExc_Exception, PyType, *FString::Printf(TEXT("Method '%s' specified as 'override' cannot also specify 'static', 'getter', or 'setter'"), *InFieldName));
				return false;
			}
			if (InPyFuncDef->FuncRetType != Py_None || InPyFuncDef->FuncParamTypes != Py_None)
			{
				UPyUtil::SetPythonError(PyExc_Exception, PyType, *FString::Printf(TEXT("Method '%s' specified as 'override' cannot also specify 'ret' or 'params'"), *InFieldName));
				return false;
			}
		}
		if (EnumHasAllFlags(InPyFuncDef->FuncFlags, EUPyUFunctionDefFlags::Static) && EnumHasAnyFlags(InPyFuncDef->FuncFlags, EUPyUFunctionDefFlags::Getter | EUPyUFunctionDefFlags::Setter))
		{
			UPyUtil::SetPythonError(PyExc_Exception, PyType, *FString::Printf(TEXT("Method '%s' specified as 'static' cannot also specify 'getter' or 'setter'"), *InFieldName));
			return false;
		}
		if (EnumHasAllFlags(InPyFuncDef->FuncFlags, EUPyUFunctionDefFlags::Getter))
		{
			if (EnumHasAnyFlags(InPyFuncDef->FuncFlags, EUPyUFunctionDefFlags::Setter))
			{
				UPyUtil::SetPythonError(PyExc_Exception, PyType, *FString::Printf(TEXT("Method '%s' specified as 'getter' cannot also specify 'setter'"), *InFieldName));
				return false;
			}
			if (EnumHasAnyFlags(InPyFuncDef->FuncFlags, EUPyUFunctionDefFlags::Impure))
			{
				UPyUtil::SetPythonError(PyExc_Exception, PyType, *FString::Printf(TEXT("Method '%s' specified as 'getter' must also specify 'pure=True'"), *InFieldName));
				return false;
			}
		}
		const int32 NetTargetCount =
			(EnumHasAllFlags(InPyFuncDef->FuncFlags, EUPyUFunctionDefFlags::Server) ? 1 : 0) +
			(EnumHasAllFlags(InPyFuncDef->FuncFlags, EUPyUFunctionDefFlags::Client) ? 1 : 0) +
			(EnumHasAllFlags(InPyFuncDef->FuncFlags, EUPyUFunctionDefFlags::NetMulticast) ? 1 : 0);
		if (NetTargetCount > 1)
		{
			UPyUtil::SetPythonError(PyExc_Exception, PyType, *FString::Printf(TEXT("Method '%s' can only specify one of 'Server', 'Client', or 'NetMulticast'"), *InFieldName));
			return false;
		}
		if (EnumHasAllFlags(InPyFuncDef->FuncFlags, EUPyUFunctionDefFlags::Reliable) && EnumHasAllFlags(InPyFuncDef->FuncFlags, EUPyUFunctionDefFlags::Unreliable))
		{
			UPyUtil::SetPythonError(PyExc_Exception, PyType, *FString::Printf(TEXT("Method '%s' cannot specify both 'Reliable' and 'Unreliable'"), *InFieldName));
			return false;
		}
		if (NetTargetCount == 0 && EnumHasAnyFlags(InPyFuncDef->FuncFlags, EUPyUFunctionDefFlags::Reliable | EUPyUFunctionDefFlags::Unreliable))
		{
			UPyUtil::SetPythonError(PyExc_Exception, PyType, *FString::Printf(TEXT("Method '%s' cannot specify 'Reliable' or 'Unreliable' without 'Server', 'Client', or 'NetMulticast'"), *InFieldName));
			return false;
		}

		// Resolve the function name to match any previously exported functions from the parent type
		const FName FuncName = *InFieldName; // FPyWrapperObjectMetaData::ResolveFunctionName(PyType->tp_base, *InFieldName);
		const UFunction* SuperFunc = SuperClass->FindFunctionByName(FuncName);
		if (SuperFunc && !EnumHasAllFlags(InPyFuncDef->FuncFlags, EUPyUFunctionDefFlags::Override))
		{
			UPyUtil::SetPythonError(PyExc_Exception, PyType, *FString::Printf(TEXT("Method '%s' cannot override a method from the base type (did you forget to specify 'override=True'?)"), *InFieldName));
			return false;
		}
		if (EnumHasAllFlags(InPyFuncDef->FuncFlags, EUPyUFunctionDefFlags::Override))
		{
			if (!SuperFunc)
			{
				UPyUtil::SetPythonError(PyExc_Exception, PyType, *FString::Printf(TEXT("Method '%s' was set to 'override', but no method was found to override"), *InFieldName));
				return false;
			}
			if (!SuperFunc->HasAnyFunctionFlags(FUNC_BlueprintEvent))
			{
				UPyUtil::SetPythonError(PyExc_Exception, PyType, *FString::Printf(TEXT("Method '%s' was set to 'override', but the method found to override was not a blueprint event"), *InFieldName));
				return false;
			}
		}

		// Inspect the argument names and defaults from the Python function
		TArray<FString> FuncArgNames;
		TArray<FUPyObjectPtr> FuncArgDefaults;
		if (!UPyUtil::InspectFunctionArgs(InPyFuncDef->Func, FuncArgNames, &FuncArgDefaults))
		{
			UPyUtil::SetPythonError(PyExc_Exception, PyType, *FString::Printf(TEXT("Failed to inspect the arguments for '%s'"), *InFieldName));
			return false;
		}

		// Create the function, either from the definition, or from the super-function found to override
		NewClass->AddNativeFunction(*FuncName.ToString(), &UUPyGeneratedClass::CallPythonFunction); // Need to do this before the call to DuplicateObject in the case that the super-function already has FUNC_Native
		UFunction* Func = nullptr;
		if (SuperFunc)
		{
			FObjectDuplicationParameters FuncDuplicationParams(const_cast<UFunction*>(SuperFunc), NewClass);
			FuncDuplicationParams.DestName = FuncName;
			FuncDuplicationParams.InternalFlagMask &= ~EInternalObjectFlags::Native;
			Func = CastChecked<UFunction>(StaticDuplicateObjectEx(FuncDuplicationParams));
		}
		else
		{
			Func = NewObject<UFunction>(NewClass, FuncName);
		}
		if (!SuperFunc)
		{
			Func->FunctionFlags |= FUNC_Public;
			// The function is not in the linked list of class fields, insert it so that field iterators & funcs work
			Func->Next = NewClass->Children;
			NewClass->Children = Func;
		}
		if (EnumHasAllFlags(InPyFuncDef->FuncFlags, EUPyUFunctionDefFlags::Static))
		{
			Func->FunctionFlags |= FUNC_Static;
		}
		if (EnumHasAllFlags(InPyFuncDef->FuncFlags, EUPyUFunctionDefFlags::Pure))
		{
			Func->FunctionFlags |= FUNC_BlueprintPure;
		}
		if (EnumHasAllFlags(InPyFuncDef->FuncFlags, EUPyUFunctionDefFlags::Impure))
		{
			Func->FunctionFlags &= ~FUNC_BlueprintPure;
		}
		if (EnumHasAllFlags(InPyFuncDef->FuncFlags, EUPyUFunctionDefFlags::Server))
		{
			Func->FunctionFlags |= (FUNC_Net | FUNC_NetServer);
		}
		if (EnumHasAllFlags(InPyFuncDef->FuncFlags, EUPyUFunctionDefFlags::Client))
		{
			Func->FunctionFlags |= (FUNC_Net | FUNC_NetClient);
		}
		if (EnumHasAllFlags(InPyFuncDef->FuncFlags, EUPyUFunctionDefFlags::NetMulticast))
		{
			Func->FunctionFlags |= (FUNC_Net | FUNC_NetMulticast);
		}
		if (EnumHasAllFlags(InPyFuncDef->FuncFlags, EUPyUFunctionDefFlags::Reliable))
		{
			Func->FunctionFlags |= FUNC_NetReliable;
		}
		if (EnumHasAllFlags(InPyFuncDef->FuncFlags, EUPyUFunctionDefFlags::Unreliable))
		{
			Func->FunctionFlags &= ~FUNC_NetReliable;
		}
#if WITH_EDITOR
		if (EnumHasAllFlags(InPyFuncDef->FuncFlags, EUPyUFunctionDefFlags::Getter))
		{
			Func->SetMetaData(UPyGenUtil::BlueprintGetterMetaDataKey, TEXT(""));
		}
		if (EnumHasAllFlags(InPyFuncDef->FuncFlags, EUPyUFunctionDefFlags::Setter))
		{
			Func->SetMetaData(UPyGenUtil::BlueprintSetterMetaDataKey, TEXT(""));
		}
#endif
		Func->FunctionFlags |= (FUNC_Native | FUNC_Event | FUNC_BlueprintEvent | FUNC_BlueprintCallable);
		FUPyUFunctionDef::ApplyMetaData(InPyFuncDef, Func);
		NewClass->AddFunctionToFunctionMap(Func, Func->GetFName());
		if (!Func->HasAnyFunctionFlags(FUNC_Static))
		{
			// Check for a malformed function rather than assert in the remove
			if (FuncArgNames.Num() > 0 && FuncArgDefaults.Num() > 0)
			{
				// Strip the zero'th 'self' argument when processing a non-static function
				FuncArgNames.RemoveAt(0, EAllowShrinking::No);
				FuncArgDefaults.RemoveAt(0, EAllowShrinking::No);
			}
			else
			{
				UPyUtil::SetPythonError(PyExc_Exception, PyType, *FString::Printf(TEXT("Incorrect number of arguments specified for '%s' (missing self?)"), *InFieldName));
				return false;
			}
		}
		// Build the arguments struct if not overriding a function
		if (!SuperFunc)
		{
			// Make sure the number of function arguments matches the number of argument types specified
			int32 NumArgTypes = 0;
			if (InPyFuncDef->FuncParamTypes && InPyFuncDef->FuncParamTypes != Py_None)
			{
				const Py_ssize_t NumArgTypesRaw = PySequence_Size(InPyFuncDef->FuncParamTypes);
				if (UPyUtil::ValidateContainerLenValue(NumArgTypesRaw, NumArgTypes, *FString::Printf(TEXT("%s::%s"), *ClassName, *InFieldName)) != 0)
				{
					return false;
				}
			}
			if (NumArgTypes != FuncArgNames.Num())
			{
				UPyUtil::SetPythonError(PyExc_Exception, PyType, *FString::Printf(TEXT("Incorrect number of arguments specified for '%s' (expected %d, got %d)"), *InFieldName, NumArgTypes, FuncArgNames.Num()));
				return false;
			}

			// Adding properties to a function inserts them into a linked list, so we add the return and output values first so that they appear at the end
			if (InPyFuncDef->FuncRetType && InPyFuncDef->FuncRetType != Py_None)
			{
				// If we have a tuple, then we actually want to return a bool but add every type within the tuple as output parameters
				const bool bOptionalReturn = static_cast<bool>(PyTuple_Check(InPyFuncDef->FuncRetType));

				PyObject* RetType = bOptionalReturn ? (PyObject*)&PyBool_Type : InPyFuncDef->FuncRetType;
				FProperty* RetProp = UPyUtil::CreateProperty(RetType, 1, Func, TEXT("ReturnValue"));
				if (!RetProp)
				{
					UPyUtil::SetPythonError(PyExc_Exception, PyType, *FString::Printf(TEXT("Failed to create return property (%s) for function '%s'"), *UPyUtil::GetFriendlyTypename(RetType), *InFieldName));
					return false;
				}
				RetProp->PropertyFlags |= (CPF_Parm | CPF_OutParm | CPF_ReturnParm);
				Func->AddCppProperty(RetProp);

				if (bOptionalReturn)
				{
					const int32 NumOutArgs = PyTuple_Size(InPyFuncDef->FuncRetType);
					for (int32 ArgIndex = 0; ArgIndex < NumOutArgs; ++ArgIndex)
					{
						FUPyObjectPtr ArgTypeObj = FUPyObjectPtr::StealReference(PySequence_GetItem(InPyFuncDef->FuncRetType, ArgIndex));
						if (!ArgTypeObj)
						{
							UPyUtil::SetPythonError(PyExc_Exception, PyType, *FString::Printf(TEXT("Failed to read output property type for function '%s' at index %d"), *InFieldName, ArgIndex));
							return false;
						}
						FProperty* ArgProp = UPyUtil::CreateProperty(ArgTypeObj, 1, Func, *FString::Printf(TEXT("OutValue%d"), ArgIndex));
						if (!ArgProp)
						{
							UPyUtil::SetPythonError(PyExc_Exception, PyType, *FString::Printf(TEXT("Failed to create output property (%s) for function '%s' at index %d"), *UPyUtil::GetFriendlyTypename(ArgTypeObj), *InFieldName, ArgIndex));
							return false;
						}
						ArgProp->PropertyFlags |= (CPF_Parm | CPF_OutParm);
						Func->AddCppProperty(ArgProp);
						Func->FunctionFlags |= FUNC_HasOutParms;
					}
				}
			}

			// Adding properties to a function inserts them into a linked list, so we loop backwards to get the order right
			{
				int32 ArgIndex = FuncArgNames.Num() - 1;
				while (ArgIndex >= 0)
				{
					FUPyObjectPtr ArgTypeObj = FUPyObjectPtr::StealReference(PySequence_GetItem(InPyFuncDef->FuncParamTypes, ArgIndex));
					if (!ArgTypeObj)
					{
						UPyUtil::SetPythonError(PyExc_Exception, PyType, *FString::Printf(TEXT("Failed to read property type for function '%s' argument '%s'"), *InFieldName, *FuncArgNames[ArgIndex]));
						return false;
					}
					FProperty* ArgProp = UPyUtil::CreateProperty(ArgTypeObj, 1, Func, *FuncArgNames[ArgIndex]);
					if (!ArgProp)
					{
						UPyUtil::SetPythonError(PyExc_Exception, PyType, *FString::Printf(TEXT("Failed to create property (%s) for function '%s' argument '%s'"), *UPyUtil::GetFriendlyTypename(ArgTypeObj), *InFieldName, *FuncArgNames[ArgIndex]));
						return false;
					}
					ArgProp->PropertyFlags |= CPF_Parm;
					Func->AddCppProperty(ArgProp);
					ArgIndex--;
				}
			}
		}
		// Apply the defaults to the function arguments and build the Python method params
		UPyGenUtil::FGeneratedWrappedFunction GeneratedWrappedFunction;
		GeneratedWrappedFunction.SetFunction(Func);
		// SetFunction doesn't always use the correct names or defaults for generated classes
		for (int32 InputArgIndex = 0; InputArgIndex < GeneratedWrappedFunction.InputParams.Num(); ++InputArgIndex)
		{
			UPyGenUtil::FGeneratedWrappedMethodParameter& GeneratedWrappedMethodParam = GeneratedWrappedFunction.InputParams[InputArgIndex];
			const FProperty* Param = GeneratedWrappedMethodParam.ParamProp;

			const FName DefaultValueMetaDataKey = *FString::Printf(TEXT("CPP_Default_%s"), *Param->GetName());

			TOptional<FString> ResolvedDefaultValue;
			if (FuncArgDefaults.IsValidIndex(InputArgIndex) && FuncArgDefaults[InputArgIndex])
			{
				// Convert the default value to the given property...
				UPyUtil::FPropValueOnScope DefaultValue(UPyUtil::FConstPropOnScope::ExternalReference(Param));
				if (!DefaultValue.IsValid() || !DefaultValue.SetValue(FuncArgDefaults[InputArgIndex], *UPyUtil::GetErrorContext(PyType)))
				{
					UPyUtil::SetPythonError(PyExc_Exception, PyType, *FString::Printf(TEXT("Failed to convert default value for function '%s' argument '%s' (%s)"), *InFieldName, *FuncArgNames[InputArgIndex], *Param->GetClass()->GetName()));
					return false;
				}

				// ... and export it as meta-data
				FString ExportedDefaultValue;
				if (!DefaultValue.GetProp()->ExportText_Direct(ExportedDefaultValue, DefaultValue.GetValue(), DefaultValue.GetValue(), nullptr, PPF_None))
				{
					UPyUtil::SetPythonError(PyExc_Exception, PyType, *FString::Printf(TEXT("Failed to export default value for function '%s' argument '%s' (%s)"), *InFieldName, *FuncArgNames[InputArgIndex], *Param->GetClass()->GetName()));
					return false;
				}

				ResolvedDefaultValue = ExportedDefaultValue;
			}
			// if (!ResolvedDefaultValue.IsSet() && SuperFunc && SuperFunc->HasAnyFunctionFlags(FUNC_HasDefaults))
			// {
			// 	if (SuperFunc->HasMetaData(DefaultValueMetaDataKey))
			// 	{
			// 		ResolvedDefaultValue = SuperFunc->GetMetaData(DefaultValueMetaDataKey);
			// 	}
			// }
			if (ResolvedDefaultValue.IsSet())
			{
#if WITH_EDITOR
				Func->SetMetaData(DefaultValueMetaDataKey, *ResolvedDefaultValue.GetValue());
#endif
				Func->FunctionFlags |= FUNC_HasDefaults;
			}

			GeneratedWrappedMethodParam.ParamName = UPyGenUtil::TCHARToUTF8Buffer(FuncArgNames.IsValidIndex(InputArgIndex) ? *FuncArgNames[InputArgIndex] : *Param->GetName());
			GeneratedWrappedMethodParam.ParamDefaultValue = MoveTemp(ResolvedDefaultValue);
		}
		Func->Bind();
		Func->StaticLink(true);

		if (GeneratedWrappedFunction.InputParams.Num() != FuncArgNames.Num())
		{
			UPyUtil::SetPythonError(PyExc_Exception, PyType, *FString::Printf(TEXT("Incorrect number of arguments specified for '%s' (expected %d, got %d)"), *InFieldName, GeneratedWrappedFunction.InputParams.Num(), FuncArgNames.Num()));
			return false;
		}

		// Apply the doc string as the function tooltip
#if WITH_EDITOR

		{
			static const FName ToolTipKey = TEXT("ToolTip");

			const FString DocString = UPyUtil::GetDocString(InPyFuncDef->Func);
			if (!DocString.IsEmpty())
			{
				Func->SetMetaData(ToolTipKey, *DocString);
			}
		}
#endif

		// Build the definition data for the new method
		UPyGenUtil::FFunctionDef& FuncDef = *NewClass->FunctionDefs.Add_GetRef(MakeShared<UPyGenUtil::FFunctionDef>());
		FuncDef.GeneratedWrappedMethod.MethodName = UPyGenUtil::TCHARToUTF8Buffer(*InFieldName);
		FuncDef.GeneratedWrappedMethod.MethodDoc = UPyGenUtil::TCHARToUTF8Buffer(*UPyGenUtil::GetFieldTooltip(Func));
		FuncDef.GeneratedWrappedMethod.MethodFunc = MoveTemp(GeneratedWrappedFunction);
		FuncDef.GeneratedWrappedMethod.MethodFlags = FuncArgNames.Num() > 0 ? METH_VARARGS | METH_KEYWORDS : METH_NOARGS;
		if (Func->HasAnyFunctionFlags(FUNC_Static))
		{
			FuncDef.GeneratedWrappedMethod.MethodFlags |= METH_CLASS;
			FuncDef.GeneratedWrappedMethod.MethodCallback = FuncArgNames.Num() > 0 ? UPyCFunctionWithClosureCast(&FUPyWrapperObjectBase::CallClassMethodWithArgs_Impl) : UPyCFunctionWithClosureCast(&FUPyWrapperObjectBase::CallClassMethodNoArgs_Impl);
		}
		else
		{
			FuncDef.GeneratedWrappedMethod.MethodCallback = FuncArgNames.Num() > 0 ? UPyCFunctionWithClosureCast(&FUPyWrapperObjectBase::CallMethodWithArgs_Impl) : UPyCFunctionWithClosureCast(&FUPyWrapperObjectBase::CallMethodNoArgs_Impl);
		}
		FuncDef.GeneratedWrappedMethod.ToPython(FuncDef.PyMethod);
		FuncDef.PyFunction = FUPyObjectPtr::NewReference(InPyFuncDef->Func);
		FuncDef.bIsHidden = EnumHasAnyFlags(InPyFuncDef->FuncFlags, EUPyUFunctionDefFlags::Getter | EUPyUFunctionDefFlags::Setter);

		return true;
	}

	bool CopyPropertiesFromOldClass()
	{
		check(OldClass);

		NewClass->PropertyDefs.Reserve(OldClass->PropertyDefs.Num());
		NewClass->ReplicationDefs = OldClass->ReplicationDefs;
		for (const TSharedPtr<UPyGenUtil::FPropertyDef>& OldPropDef : OldClass->PropertyDefs)
		{
			const FProperty* OldProp = OldPropDef->GeneratedWrappedGetSet.Prop.Prop;
			const UFunction* OldGetter = OldPropDef->GeneratedWrappedGetSet.GetFunc.Func;
			const UFunction* OldSetter = OldPropDef->GeneratedWrappedGetSet.SetFunc.Func;

			FProperty* Prop = CastFieldChecked<FProperty>(FField::Duplicate(OldProp, NewClass, OldProp->GetFName()));
			if (!Prop)
			{
				UPyUtil::SetPythonError(PyExc_Exception, PyType, *FString::Printf(TEXT("Failed to duplicate property for '%s'"), UTF8_TO_TCHAR(OldPropDef->PyGetSet.name)));
				return false;
			}

#if WITH_EDITOR
			FField::CopyMetaData(OldProp, Prop);
#endif
			NewClass->AddCppProperty(Prop);

			UPyGenUtil::FPropertyDef& PropDef = *NewClass->PropertyDefs.Add_GetRef(MakeShared<UPyGenUtil::FPropertyDef>());
			PropDef.GeneratedWrappedGetSet = OldPropDef->GeneratedWrappedGetSet;
			PropDef.GeneratedWrappedGetSet.Prop.SetProperty(Prop);
			if (OldGetter)
			{
				PropDef.GeneratedWrappedGetSet.GetFunc.SetFunction(NewClass->FindFunctionByName(OldGetter->GetFName()));
			}
			if (OldSetter)
			{
				PropDef.GeneratedWrappedGetSet.SetFunc.SetFunction(NewClass->FindFunctionByName(OldSetter->GetFName()));
			}
			PropDef.GeneratedWrappedGetSet.ToPython(PropDef.PyGetSet);
		}

		return true;
	}

	bool CopyFunctionsFromOldClass()
	{
		check(OldClass);

		NewClass->FunctionDefs.Reserve(OldClass->FunctionDefs.Num());
		for (const TSharedPtr<UPyGenUtil::FFunctionDef>& OldFuncDef : OldClass->FunctionDefs)
		{
			const UFunction* OldFunc = OldFuncDef->GeneratedWrappedMethod.MethodFunc.Func;

			NewClass->AddNativeFunction(*OldFunc->GetName(), &UUPyGeneratedClass::CallPythonFunction);
			UFunction* Func = DuplicateObject<UFunction>(OldFunc, NewClass, OldFunc->GetFName());
			if (!Func)
			{
				UPyUtil::SetPythonError(PyExc_Exception, PyType, *FString::Printf(TEXT("Failed to duplicate function for '%s'"), UTF8_TO_TCHAR(OldFuncDef->PyMethod.MethodName)));
				return false;
			}

#if WITH_EDITOR
			FMetaData::CopyMetadata((UFunction*)OldFunc, Func);
#endif
			NewClass->AddFunctionToFunctionMap(Func, Func->GetFName());

			Func->Bind();
			Func->StaticLink(true);

			UPyGenUtil::FFunctionDef& FuncDef = *NewClass->FunctionDefs.Add_GetRef(MakeShared<UPyGenUtil::FFunctionDef>());
			FuncDef.GeneratedWrappedMethod = OldFuncDef->GeneratedWrappedMethod;
			FuncDef.GeneratedWrappedMethod.MethodFunc.SetFunction(Func);
			FuncDef.PyFunction = OldFuncDef->PyFunction;
			FuncDef.bIsHidden = OldFuncDef->bIsHidden;
			FuncDef.GeneratedWrappedMethod.ToPython(FuncDef.PyMethod);
		}

		return true;
	}

	void ReparentPythonType(PyTypeObject* InNewBasePyType)
	{
		auto UpdateTuple = [](PyObject* InTuple, PyTypeObject* InOldType, PyTypeObject* InNewType)
		{
			if (InTuple)
			{
				const int32 TupleSize = PyTuple_Size(InTuple);
				for (int32 TupleIndex = 0; TupleIndex < TupleSize; ++TupleIndex)
				{
					if (PyTuple_GetItem(InTuple, TupleIndex) == (PyObject*)InOldType)
					{
						FUPyTypeObjectPtr NewType = FUPyTypeObjectPtr::NewReference(InNewType);
						PyTuple_SetItem(InTuple, TupleIndex, (PyObject*)NewType.Release()); // PyTuple_SetItem steals the reference
					}
				}
			}
		};

		UpdateTuple(PyType->tp_bases, PyType->tp_base, InNewBasePyType);
		UpdateTuple(PyType->tp_mro, PyType->tp_base, InNewBasePyType);
		PyType->tp_base = InNewBasePyType;
	}

private:
	bool RegisterDescriptors()
	{
		for (const TSharedPtr<UPyGenUtil::FPropertyDef>& PropDef : NewClass->PropertyDefs)
		{
			FUPyObjectPtr GetSetDesc = FUPyObjectPtr::StealReference(PyDescr_NewGetSet(PyType, &PropDef->PyGetSet));
			if (!GetSetDesc)
			{
				UPyUtil::SetPythonError(PyExc_Exception, PyType, *FString::Printf(TEXT("Failed to create descriptor for '%s'"), UTF8_TO_TCHAR(PropDef->PyGetSet.name)));
				return false;
			}
			if (PyDict_SetItemString(PyType->tp_dict, PropDef->PyGetSet.name, GetSetDesc) != 0)
			{
				UPyUtil::SetPythonError(PyExc_Exception, PyType, *FString::Printf(TEXT("Failed to assign descriptor for '%s'"), UTF8_TO_TCHAR(PropDef->PyGetSet.name)));
				return false;
			}
		}

		for (const TSharedPtr<UPyGenUtil::FFunctionDef>& FuncDef : NewClass->FunctionDefs)
		{
			if (FuncDef->bIsHidden)
			{
				PyDict_DelItemString(PyType->tp_dict, FuncDef->PyMethod.MethodName);
			}
			else
			{
				FUPyObjectPtr MethodDesc = FUPyObjectPtr::StealReference(FUPyMethodWithClosureDef::NewMethodDescriptor(PyType, &FuncDef->PyMethod));
				if (!MethodDesc)
				{
					UPyUtil::SetPythonError(PyExc_Exception, PyType, *FString::Printf(TEXT("Failed to create descriptor for '%s'"), UTF8_TO_TCHAR(FuncDef->PyMethod.MethodName)));
					return false;
				}
				if (PyDict_SetItemString(PyType->tp_dict, FuncDef->PyMethod.MethodName, MethodDesc) != 0)
				{
					UPyUtil::SetPythonError(PyExc_Exception, PyType, *FString::Printf(TEXT("Failed to assign descriptor for '%s'"), UTF8_TO_TCHAR(FuncDef->PyMethod.MethodName)));
					return false;
				}
			}
		}

		return true;
	}

	void PrepareOldClassForReinstancing()
	{
		check(OldClass);

		const FString OldClassName = MakeUniqueObjectName(OldClass->GetOuter(), OldClass->GetClass(), *FString::Printf(TEXT("%s_REINST"), *OldClass->GetName())).ToString();
		OldClass->ClassFlags |= CLASS_NewerVersionExists;
		OldClass->SetFlags(RF_NewerVersionExists);
		OldClass->ClearFlags(RF_Public | RF_Standalone);
		OldClass->Rename(*OldClassName, nullptr, REN_DontCreateRedirectors);
		OldClass->UnregisterGeneratedType();
	}

	FString ClassName;
	PyTypeObject* PyType;
	UUPyGeneratedClass* OldClass;
	UUPyGeneratedClass* NewClass;
};

void UUPyGeneratedClass::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	Super::AddReferencedObjects(InThis, Collector);

	UUPyGeneratedClass* This = CastChecked<UUPyGeneratedClass>(InThis);
	Collector.AddReferencedObject(This->ClassRedirector, This);
	Collector.AddReferencedObject(This->CDORedirector, This);
}

void UUPyGeneratedClass::PostRename(UObject* OldOuter, const FName OldName)
{
	Super::PostRename(OldOuter, OldName);

	if (PyType)
	{
		FUPyWrapperTypeRegistry::Get().UnregisterWrappedClassType(this);
		FUPyWrapperTypeRegistry::Get().RegisterWrappedClassType(this, PyType);
	}
}

void UUPyGeneratedClass::PostInitInstance(UObject* InObj, FObjectInstancingGraph* InstanceGraph)
{
	Super::PostInitInstance(InObj, InstanceGraph);

	// Execute Python code within this block
	{
		FUPyScopedGIL GIL;

		if (PyPostInitFunction)
		{
			TGuardValue<bool> IsRunningUserScriptGuard(UE::UnrealPython::Private::GIsRunningUserScript, true);

			FUPyObjectPtr PySelf = FUPyObjectPtr::StealReference((PyObject*)FUPyWrapperObjectFactory::Get().CreateInstance(InObj));
			if (PySelf && ensureAlwaysMsgf(PySelf->ob_type == PyType, TEXT("Object '%s' (class: %s) had an unexpected PyType when calling PostInitInstance! Self is '%s' but we expected '%s'"), *InObj->GetPathName(), *GetPathName(), *UPyUtil::GetFriendlyTypename(PySelf), *UPyUtil::GetFriendlyTypename(PyType)))
			{
				FUPyObjectPtr PyArgs = FUPyObjectPtr::StealReference(PyTuple_New(1));
				PyTuple_SetItem(PyArgs, 0, PySelf.Release()); // SetItem steals the reference

				FUPyObjectPtr Result = FUPyObjectPtr::StealReference(PyObject_CallObject(PyPostInitFunction, PyArgs));
				if (!Result)
				{
					UPyUtil::ReThrowPythonError();
				}
			}
		}
	}
}

void UUPyGeneratedClass::BeginDestroy()
{
	ReleasePythonResources();
	Super::BeginDestroy();
}

void UUPyGeneratedClass::ReleasePythonResources()
{
	// This may be called after Python has already shut down
	if (Py_IsInitialized())
	{
		FUPyScopedGIL GIL;
		UnregisterGeneratedType();
		PyType.Reset();
		PyPostInitFunction.Reset();
		for (const TSharedPtr<UPyGenUtil::FFunctionDef>& FunctionDef : FunctionDefs)
		{
			FunctionDef->PyFunction.Reset();
		}
	}
	else
	{
		// Release ownership if Python has been shut down to avoid attempting to delete the objects (which are already dead)
		PyType.Release();
		PyPostInitFunction.Release();
		for (const TSharedPtr<UPyGenUtil::FFunctionDef>& FunctionDef : FunctionDefs)
		{
			FunctionDef->PyFunction.Release();
		}
	}

	PropertyDefs.Reset();
	ReplicationDefs.Reset();
	FunctionDefs.Reset();
	// PyMetaData = FPyWrapperObjectMetaData();
}

void UUPyGeneratedClass::UnregisterGeneratedType()
{
	if (PyType)
	{
		FUPyWrapperTypeRegistry::Get().UnregisterWrappedClassType(this);
	}
}

bool UUPyGeneratedClass::IsFunctionImplementedInScript(FName InFunctionName) const
{
	UFunction* Function = FindFunctionByName(InFunctionName);
	return Function && Function->GetOuter() && Function->GetOuter()->IsA(UUPyGeneratedClass::StaticClass());
}

void UUPyGeneratedClass::GetLifetimeBlueprintReplicationList(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	for (const FReplicationDef& ReplicationDef : ReplicationDefs)
	{
		FProperty* Prop = FindPropertyByName(ReplicationDef.PropertyName);
		if (Prop && Prop->HasAnyPropertyFlags(CPF_Net))
		{
			FLifetimeProperty* ExistingLifetimeProp = OutLifetimeProps.FindByPredicate([Prop](const FLifetimeProperty& InLifetimeProp)
			{
				return InLifetimeProp.RepIndex == Prop->RepIndex;
			});
			if (ExistingLifetimeProp)
			{
				ExistingLifetimeProp->Condition = ReplicationDef.ReplicationCondition;
				ExistingLifetimeProp->RepNotifyCondition = ReplicationDef.RepNotifyCondition;
				ExistingLifetimeProp->bIsPushBased = ReplicationDef.bPushBased;
			}
			else
			{
				OutLifetimeProps.Add(FLifetimeProperty(Prop->RepIndex, ReplicationDef.ReplicationCondition, ReplicationDef.RepNotifyCondition, ReplicationDef.bPushBased));
			}
		}
	}

	if (const UUPyGeneratedClass* SuperPyClass = Cast<UUPyGeneratedClass>(GetSuperStruct()))
	{
		SuperPyClass->GetLifetimeBlueprintReplicationList(OutLifetimeProps);
	}
}

bool UUPyGeneratedClass::GetGeneratedPropertyReplicationInfo(FName InPropertyName, ELifetimeCondition& OutReplicationCondition, ELifetimeRepNotifyCondition& OutRepNotifyCondition, bool& bOutPushBased) const
{
	if (const FReplicationDef* ReplicationDef = ReplicationDefs.FindByPredicate([InPropertyName](const FReplicationDef& InReplicationDef)
	{
		return InReplicationDef.PropertyName == InPropertyName;
	}))
	{
		OutReplicationCondition = ReplicationDef->ReplicationCondition;
		OutRepNotifyCondition = ReplicationDef->RepNotifyCondition;
		bOutPushBased = ReplicationDef->bPushBased;
		return true;
	}

	OutReplicationCondition = COND_None;
	OutRepNotifyCondition = REPNOTIFY_OnChanged;
	bOutPushBased = false;
	return false;
}

UUPyGeneratedClass* UUPyGeneratedClass::GenerateClass(PyTypeObject* InPyType)
{
	// Get the correct super class from the parent type in Python
	UClass* SuperClass = (UClass*)FUPyWrapperTypeRegistry::Get().FindClass(InPyType->tp_base);
	if (!SuperClass)
	{
		UPyUtil::SetPythonError(PyExc_Exception, InPyType, TEXT("No super class could be found for this Python type"));
		return nullptr;
	}

	// Builder used to generate the class
	FUPythonGeneratedClassBuilder PythonClassBuilder(SuperClass, InPyType);

	// Add the functions to this class
	// We have to process these first as properties may reference them as get/set functions
	{
		PyObject* FieldKey = nullptr;
		PyObject* FieldValue = nullptr;
		Py_ssize_t FieldIndex = 0;
		while (PyDict_Next(InPyType->tp_dict, &FieldIndex, &FieldKey, &FieldValue))
		{
			const FString FieldName = UPyUtil::PyObjectToUEString(FieldKey);

			if (PyObject_IsInstance(FieldValue, (PyObject*)&UPyUValueDefType) == 1)
			{
				// Values are not supported on classes
				UPyUtil::SetPythonError(PyExc_Exception, InPyType, TEXT("Classes do not support values"));
				return nullptr;
			}

			if (PyObject_IsInstance(FieldValue, (PyObject*)&UPyUFunctionDefType) == 1)
			{
				FUPyUFunctionDef* PyFuncDef = (FUPyUFunctionDef*)FieldValue;
				if (!PythonClassBuilder.CreateFunctionFromDefinition(FieldName, PyFuncDef))
				{
					return nullptr;
				}
			}
		}
	}

	// Add the properties to this class
	{
		PyObject* FieldKey = nullptr;
		PyObject* FieldValue = nullptr;
		Py_ssize_t FieldIndex = 0;
		while (PyDict_Next(InPyType->tp_dict, &FieldIndex, &FieldKey, &FieldValue))
		{
			const FString FieldName = UPyUtil::PyObjectToUEString(FieldKey);

			if (PyObject_IsInstance(FieldValue, (PyObject*)&UPyFPropertyDefType) == 1)
			{
				FUPyFPropertyDef* PyPropDef = (FUPyFPropertyDef*)FieldValue;
				if (!PythonClassBuilder.CreatePropertyFromDefinition(FieldName, PyPropDef))
				{
					return nullptr;
				}
			}
		}
	}

	FUPyObjectPtr PyPostInitFunction = FUPyObjectPtr::StealReference(UPyGenUtil::GetPostInitFunc(InPyType));
	if (PyErr_Occurred())
	{
		return nullptr;
	}

	// Finalize the class with its optional post-init function
	return PythonClassBuilder.Finalize(MoveTemp(PyPostInitFunction));
}

bool UUPyGeneratedClass::ReparentDerivedClasses(UUPyGeneratedClass* InOldParent, UUPyGeneratedClass* InNewParent)
{
	TArray<UClass*> DerivedClasses;
	GetDerivedClasses(InOldParent, DerivedClasses, /*bRecursive*/false);

	bool bSuccess = true;

	for (UClass* DerivedClass : DerivedClasses)
	{
		if (DerivedClass->HasAnyClassFlags(CLASS_NewerVersionExists))
		{
			continue;
		}

		// todo: Blueprint classes?

		if (UUPyGeneratedClass* PyDerivedClass = Cast<UUPyGeneratedClass>(DerivedClass))
		{
			bSuccess &= !!ReparentClass(PyDerivedClass, InNewParent);
		}
	}

	return bSuccess;
}

UUPyGeneratedClass* UUPyGeneratedClass::ReparentClass(UUPyGeneratedClass* InOldClass, UUPyGeneratedClass* InNewParent)
{
	// Builder used to generate the class
	FUPythonGeneratedClassBuilder PythonClassBuilder(InOldClass, InNewParent);

	// Copy the data from the old class
	if (!PythonClassBuilder.CopyFunctionsFromOldClass())
	{
		return nullptr;
	}
	if (!PythonClassBuilder.CopyPropertiesFromOldClass())
	{
		return nullptr;
	}

	UUPyGeneratedClass* NewClass = PythonClassBuilder.Finalize(InOldClass->PyPostInitFunction);
	if (NewClass)
	{
		// Update the base of the Python type
		PythonClassBuilder.ReparentPythonType(InNewParent->PyType);
	}
	return NewClass;
}

DEFINE_FUNCTION(UUPyGeneratedClass::CallPythonFunction)
{
	// Note: This function *must not* return until InvokePythonCallableFromUnrealFunctionThunk has been called, as we need to step over the correct amount of data from the bytecode stack!

	const UFunction* Func = Stack.CurrentNativeFunction;

	// Find the Python function to call
	TSharedPtr<UPyGenUtil::FFunctionDef> FuncDef;
	{
		// Get the correct class from the UFunction so that we can perform static dispatch to the correct type
		const UUPyGeneratedClass* This = CastChecked<UUPyGeneratedClass>(Func->GetOwnerClass());

		const TSharedPtr<UPyGenUtil::FFunctionDef>* FuncDefPtr = This->FunctionDefs.FindByPredicate([Func](const TSharedPtr<UPyGenUtil::FFunctionDef>& InFuncDef)
		{
			return InFuncDef->GeneratedWrappedMethod.MethodFunc.Func == Func;
		});
		FuncDef = FuncDefPtr ? *FuncDefPtr : nullptr;

		if (!FuncDef.IsValid())
		{
			UE_LOG(LogUnrealPython, Error, TEXT("Failed to find Python function for '%s' on '%s'"), *Func->GetName(), *This->GetName());
		}
	}

	// Find the Python object to call the function on
	FUPyObjectPtr PySelf;
	bool bSelfError = false;
	if (!Func->HasAnyFunctionFlags(FUNC_Static))
	{
		FUPyScopedGIL GIL;
		PySelf = FUPyObjectPtr::StealReference((PyObject*)FUPyWrapperObjectFactory::Get().CreateInstance(P_THIS_OBJECT));
		if (!PySelf)
		{
			UE_LOG(LogUnrealPython, Error, TEXT("Failed to create a Python wrapper for '%s'"), *P_THIS_OBJECT->GetName());
			bSelfError = true;
		}
	}

	// Execute Python code within this block
	{
		FUPyScopedGIL GIL;
		if (!UPyGenUtil::InvokePythonCallableFromUnrealFunctionThunk(PySelf, FuncDef ? FuncDef->PyFunction.GetPtr() : nullptr, Func, Context, Stack, RESULT_PARAM) || bSelfError)
		{
			UPyUtil::ReThrowPythonError();
		}
		PySelf.Reset(); // Need to reset this while still under the GIL
	}
}

FUPyUClassDecorator* FUPyUClassDecorator::New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
{
	FUPyUClassDecorator* Self = (FUPyUClassDecorator*)InType->tp_alloc(InType, 0);
	return Self;
}

void FUPyUClassDecorator::Dealloc(FUPyUClassDecorator* InSelf)
{
	Deinit(InSelf);
	Py_TYPE(InSelf)->tp_free((PyObject*)InSelf);
}

int FUPyUClassDecorator::Init(FUPyUClassDecorator* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	Deinit(InSelf);
	return 0;
}

void FUPyUClassDecorator::Deinit(FUPyUClassDecorator* InSelf)
{
}

PyObject* FUPyUClassDecorator::Call(FUPyUClassDecorator* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	return PyCallGenerateClass((PyObject*)InSelf, InArgs, nullptr);
}

FUPyUFunctionDecorator* FUPyUFunctionDecorator::New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
{
	FUPyUFunctionDecorator* Self = (FUPyUFunctionDecorator*)InType->tp_alloc(InType, 0);
	if (Self)
	{
		Self->CacheArgs = nullptr;
		Self->CacheKwds = nullptr;
	}
	return Self;
}

void FUPyUFunctionDecorator::Dealloc(FUPyUFunctionDecorator* InSelf)
{
	Deinit(InSelf);
	Py_TYPE(InSelf)->tp_free((PyObject*)InSelf);
}

int FUPyUFunctionDecorator::Init(FUPyUFunctionDecorator* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	Deinit(InSelf);

	InSelf->CacheArgs = FUPyObjectPtr::NewReference(InArgs).Release();
	InSelf->CacheKwds = FUPyObjectPtr::NewReference(InKwds).Release();

	return 0;
}

void FUPyUFunctionDecorator::Deinit(FUPyUFunctionDecorator* InSelf)
{
	Py_XDECREF(InSelf->CacheArgs);
	InSelf->CacheArgs = nullptr;

	Py_XDECREF(InSelf->CacheKwds);
	InSelf->CacheKwds = nullptr;
}

PyObject* FUPyUFunctionDecorator::Call(FUPyUFunctionDecorator* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	return PyCallGenerateFunction((PyObject*)InSelf, InArgs, InSelf->CacheKwds);
}
