
#include "UPyCommon.h"
#include "Core/UPyConversion.h"
#include "Core/UPyCore.h"
#include "Utils/UPyUtil.h"
#include "Wrapper/UPyWrapperObjectBase.h"
#include "Wrapper/UPyWrapperStruct.h"
#include "Wrapper/UPyWrapperEnum.h"
#include "Wrapper/UPyWrapperTypeRegistry.h"
#include "Subclassing/UPyGeneratedClass.h"
#include "Subclassing/UPyGeneratedStruct.h"
#include "Subclassing/UPyGeneratedEnum.h"

PyObject* CallLog(PyObject* InSelf, PyObject* InArgs)
{
	PyObject* PyObj = nullptr;
	if (PyArg_ParseTuple(InArgs, "O:CallLog", &PyObj))
	{
		const FString LogMessage = UPyUtil::PyObjectToUEString(PyObj);
		// if (ShouldLog(LogMessage))
		{
			Py_BEGIN_ALLOW_THREADS
			UE_LOG(LogUnrealPython, Log, TEXT("%s"), *LogMessage);
			Py_END_ALLOW_THREADS
			// GetPythonLogCapture().Broadcast(EPythonLogOutputType::Info, *LogMessage);
		}

		Py_RETURN_NONE;
	}

	return nullptr;
}

PyObject* CallLogWarning(PyObject* InSelf, PyObject* InArgs)
{
	PyObject* PyObj = nullptr;
	if (PyArg_ParseTuple(InArgs, "O:log_warning", &PyObj))
	{
		const FString LogMessage = UPyUtil::PyObjectToUEString(PyObj);
		// if (ShouldLog(LogMessage))
		{
			Py_BEGIN_ALLOW_THREADS
			UE_LOG(LogUnrealPython, Warning, TEXT("%s"), *LogMessage);
			Py_END_ALLOW_THREADS
			// GetPythonLogCapture().Broadcast(EPythonLogOutputType::Warning, *LogMessage);
		}

		Py_RETURN_NONE;
	}

	return nullptr;
}

PyObject* CallLogError(PyObject* InSelf, PyObject* InArgs)
{
	PyObject* PyObj = nullptr;
	if (PyArg_ParseTuple(InArgs, "O:log_error", &PyObj))
	{
		const FString LogMessage = UPyUtil::PyObjectToUEString(PyObj);
		// if (ShouldLog(LogMessage))
		{
			Py_BEGIN_ALLOW_THREADS
			UE_LOG(LogUnrealPython, Error, TEXT("%s"), *LogMessage);
			Py_END_ALLOW_THREADS
			// GetPythonLogCapture().Broadcast(EPythonLogOutputType::Error, *LogMessage);
		}

		Py_RETURN_NONE;
	}

	return nullptr;
}

PyObject* CallLogFatal(PyObject* InSelf, PyObject* InArgs)
{
	PyObject* PyObj = nullptr;
	if (PyArg_ParseTuple(InArgs, "O:log_fatal", &PyObj))
	{
		const FString LogMessage = UPyUtil::PyObjectToUEString(PyObj);
		// if (ShouldLog(LogMessage))
		{
			Py_BEGIN_ALLOW_THREADS
			UE_LOG(LogUnrealPython, Fatal, TEXT("%s"), *LogMessage);
			Py_END_ALLOW_THREADS
			// GetPythonLogCapture().Broadcast(EPythonLogOutputType::Fatal, *LogMessage);
		}

		Py_RETURN_NONE;
	}

	return nullptr;
}

PyObject* CallLogDisplay(PyObject* InSelf, PyObject* InArgs)
{
	PyObject* PyObj = nullptr;
	if (PyArg_ParseTuple(InArgs, "O:log_display", &PyObj))
	{
		const FString LogMessage = UPyUtil::PyObjectToUEString(PyObj);
		// if (ShouldLog(LogMessage))
		{
			Py_BEGIN_ALLOW_THREADS
			UE_LOG(LogUnrealPython, Display, TEXT("%s"), *LogMessage);
			Py_END_ALLOW_THREADS
			// GetPythonLogCapture().Broadcast(EPythonLogOutputType::Display, *LogMessage);
		}

		Py_RETURN_NONE;
	}

	return nullptr;
}

PyObject* CallLogVerbose(PyObject* InSelf, PyObject* InArgs)
{
	PyObject* PyObj = nullptr;
	if (PyArg_ParseTuple(InArgs, "O:log_verbose", &PyObj))
	{
		const FString LogMessage = UPyUtil::PyObjectToUEString(PyObj);
		// if (ShouldLog(LogMessage))
		{
			Py_BEGIN_ALLOW_THREADS
			UE_LOG(LogUnrealPython, Verbose, TEXT("%s"), *LogMessage);
			Py_END_ALLOW_THREADS
			// GetPythonLogCapture().Broadcast(EPythonLogOutputType::Verbose, *LogMessage);
		}

		Py_RETURN_NONE;
	}

	return nullptr;
}

PyObject* CallLogFlush(PyObject* InSelf)
{
	if (GLog)
	{
		Py_BEGIN_ALLOW_THREADS
		GLog->Flush();
		Py_END_ALLOW_THREADS
	}

	Py_RETURN_NONE;
}

PyObject* FindOrLoadObjectImpl(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds, const TCHAR* InFuncName, const TFunctionRef<UObject*(UClass*, UObject*, const TCHAR*)>& InFunc)
{
	PyObject* PyOuterObj = nullptr;
	PyObject* PyNameObj = nullptr;
	PyObject* PyTypeObj = nullptr;
	PyObject* PyFollowRedirectorsObj = nullptr;

	static const char *ArgsKwdList[] = { "Type", "Name", "Outer", "FollowRedirectors", nullptr };
	if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, TCHAR_TO_UTF8(*FString::Printf(TEXT("OO|OO:%s"), InFuncName)), (char**)ArgsKwdList, &PyTypeObj, &PyNameObj, &PyOuterObj, &PyFollowRedirectorsObj))
	{
		return nullptr;
	}

	UClass* ObjectType = nullptr;
	if (!UPyConversion::NativizeClass(PyTypeObj, ObjectType, nullptr))
	{
		UPyUtil::SetPythonError(PyExc_TypeError, InFuncName, *FString::Printf(TEXT("Failed to convert 'Type' (%s) to 'Class'"), *UPyUtil::GetFriendlyTypename(PyTypeObj)));
		return nullptr;
	}
	if (!ObjectType)
	{
		ObjectType = UObject::StaticClass();
	}

	FString ObjectName;
	if (!UPyConversion::Nativize(PyNameObj, ObjectName))
	{
		UPyUtil::SetPythonError(PyExc_TypeError, InFuncName, *FString::Printf(TEXT("Failed to convert 'Name' (%s) to 'String'"), *UPyUtil::GetFriendlyTypename(PyNameObj)));
		return nullptr;
	}

	UObject* ObjectOuter = nullptr;
	if (PyOuterObj && !UPyConversion::Nativize(PyOuterObj, ObjectOuter))
	{
		UPyUtil::SetPythonError(PyExc_TypeError, InFuncName, *FString::Printf(TEXT("Failed to convert 'Outer' (%s) to 'Object'"), *UPyUtil::GetFriendlyTypename(PyOuterObj)));
		return nullptr;
	}

	bool bFollowRedirectors = true;
	if (PyFollowRedirectorsObj && !UPyConversion::Nativize(PyFollowRedirectorsObj, bFollowRedirectors))
	{
		UPyUtil::SetPythonError(PyExc_TypeError, InFuncName, *FString::Printf(TEXT("Failed to convert 'FollowRedirectors' (%s) to 'bool'"), *UPyUtil::GetFriendlyTypename(PyFollowRedirectorsObj)));
		return nullptr;
	}

	UObject* PotentialObject = InFunc(UObject::StaticClass(), ObjectOuter, *ObjectName);

	// Follow any redirectors
	if (bFollowRedirectors)
	{
		while (UObjectRedirector* Redirector = Cast<UObjectRedirector>(PotentialObject))
		{
			PotentialObject = Redirector->DestinationObject;
		}
	}

	// Make sure the object is of the correct type
	if (PotentialObject && !PotentialObject->IsA(ObjectType))
	{
		PotentialObject = nullptr;
	}

	return UPyConversion::Pythonize(PotentialObject);
}

PyObject* CallNewObject(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	UClass* ObjectType = nullptr;
	UObject* ObjectOuter = GetTransientPackage();
	FName ObjectName;
	UClass* ObjectBaseType = nullptr;

	// Parse the args
	{
		PyObject* PyTypeObj = nullptr;
		PyObject* PyOuterObj = nullptr;
		PyObject* PyNameObj = nullptr;
		PyObject* PyBaseTypeObj = nullptr;

		static const char* ArgsKwdList[] = { "Type", "Outer", "Name", "BaseType", nullptr };
		if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "O|OOO:NewObject", (char**)ArgsKwdList, &PyTypeObj, &PyOuterObj, &PyNameObj, &PyBaseTypeObj))
		{
			return nullptr;
		}

		if (!UPyConversion::NativizeClass(PyTypeObj, ObjectType, UObject::StaticClass()))
		{
			UPyUtil::SetPythonError(PyExc_TypeError, TEXT("NewObject"), *FString::Printf(TEXT("Failed to convert 'Type' (%s) to 'Class'"), *UPyUtil::GetFriendlyTypename(PyTypeObj)));
			return nullptr;
		}

		// If PyOuterObj is None, Nativize() will convert 'PyOuterObj=None' into 'nullptr'. That will overwrite the desired default value 'transient package'.
		if (PyOuterObj && PyOuterObj != Py_None && !UPyConversion::Nativize(PyOuterObj, ObjectOuter))
		{
			UPyUtil::SetPythonError(PyExc_TypeError, TEXT("NewObject"), *FString::Printf(TEXT("Failed to convert 'Outer' (%s) to 'Object'"), *UPyUtil::GetFriendlyTypename(PyOuterObj)));
			return nullptr;
		}

		if (PyNameObj && !UPyConversion::Nativize(PyNameObj, ObjectName))
		{
			UPyUtil::SetPythonError(PyExc_TypeError, TEXT("NewObject"), *FString::Printf(TEXT("Failed to convert 'Name' (%s) to 'Name'"), *UPyUtil::GetFriendlyTypename(PyNameObj)));
			return nullptr;
		}

		if (PyBaseTypeObj && !UPyConversion::NativizeClass(PyBaseTypeObj, ObjectBaseType, UObject::StaticClass()))
		{
			UPyUtil::SetPythonError(PyExc_TypeError, TEXT("NewObject"), *FString::Printf(TEXT("Failed to convert 'BaseType' (%s) to 'Class'"), *UPyUtil::GetFriendlyTypename(PyBaseTypeObj)));
			return nullptr;
		}
	}

	// Create the instance
	UObject* ObjectInstance = UPyUtil::NewObject(ObjectType, ObjectOuter, ObjectName, ObjectBaseType, TEXT("NewObject"));
	if (!ObjectInstance)
	{
		// UPyUtil::NewObject should have set an error state
		return nullptr;
	}

	return UPyConversion::Pythonize(ObjectInstance);
}

PyObject* CallFindObject(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	return FindOrLoadObjectImpl(InSelf, InArgs, InKwds, TEXT("FindObject"), [](UClass* ObjectType, UObject* ObjectOuter, const TCHAR* ObjectName)
	{
		UObject* Object = nullptr;
		Py_BEGIN_ALLOW_THREADS
		Object = ::StaticFindObject(ObjectType, ObjectOuter, ObjectName);
		Py_END_ALLOW_THREADS
		return Object;
	});
}

PyObject* CallLoadObject(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	return FindOrLoadObjectImpl(InSelf, InArgs, InKwds, TEXT("LoadObject"), [](UClass* ObjectType, UObject* ObjectOuter, const TCHAR* ObjectName)
	{
		UObject* Object = nullptr;
		Py_BEGIN_ALLOW_THREADS
		Object = ::StaticLoadObject(ObjectType, ObjectOuter, ObjectName);
		Py_END_ALLOW_THREADS
		return Object;
	});
}

PyObject* CallLoadClass(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	PyObject* PyNameObj = nullptr;
	PyObject* PyTypeObj = nullptr;
	PyObject* PyOuterObj = nullptr;

	static const char *ArgsKwdList[] = { "Name", "Type", "Outer", nullptr };
	if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "O|OO:LoadClass", (char**)ArgsKwdList, &PyNameObj, &PyTypeObj, &PyOuterObj))
	{
		return nullptr;
	}

	FString ObjectName;
	if (!UPyConversion::Nativize(PyNameObj, ObjectName))
	{
		UPyUtil::SetPythonError(PyExc_TypeError, TEXT("LoadClass"), *FString::Printf(TEXT("Failed to convert 'name' (%s) to 'String'"), *UPyUtil::GetFriendlyTypename(PyNameObj)));
		return nullptr;
	}

	UClass* ObjectType = nullptr;
	if (PyTypeObj && !UPyConversion::NativizeClass(PyTypeObj, ObjectType, nullptr))
	{
		UPyUtil::SetPythonError(PyExc_TypeError, TEXT("LoadClass"), *FString::Printf(TEXT("Failed to convert 'type' (%s) to 'Class'"), *UPyUtil::GetFriendlyTypename(PyTypeObj)));
		return nullptr;
	}
	if (!ObjectType)
	{
		ObjectType = UObject::StaticClass();
	}

	UObject* ObjectOuter = nullptr;
	if (PyOuterObj && !UPyConversion::Nativize(PyOuterObj, ObjectOuter))
	{
		UPyUtil::SetPythonError(PyExc_TypeError, TEXT("LoadClass"), *FString::Printf(TEXT("Failed to convert 'outer' (%s) to 'Object'"), *UPyUtil::GetFriendlyTypename(PyOuterObj)));
		return nullptr;
	}

	UClass* PotentialClass = nullptr;
	Py_BEGIN_ALLOW_THREADS
	PotentialClass = ::StaticLoadClass(ObjectType, ObjectOuter, *ObjectName);
	Py_END_ALLOW_THREADS
	return UPyConversion::PythonizeClass(PotentialClass);
}

PyObject* FindOrLoadAssetImpl(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds, const TCHAR* InFuncName, const TFunctionRef<UObject*(UClass*, UObject*, const TCHAR*)>& InFunc)
{
	PyObject* PyNameObj = nullptr;
	PyObject* PyTypeObj = nullptr;
	PyObject* PyFollowRedirectorsObj = nullptr;

	static const char *ArgsKwdList[] = { "Name", "Type", "FollowRedirectors", nullptr };
	if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, TCHAR_TO_UTF8(*FString::Printf(TEXT("O|OO:%s"), InFuncName)), (char**)ArgsKwdList, &PyNameObj, &PyTypeObj, &PyFollowRedirectorsObj))
	{
		return nullptr;
	}

	FString ObjectName;
	if (!UPyConversion::Nativize(PyNameObj, ObjectName))
	{
		UPyUtil::SetPythonError(PyExc_TypeError, InFuncName, *FString::Printf(TEXT("Failed to convert 'name' (%s) to 'String'"), *UPyUtil::GetFriendlyTypename(PyNameObj)));
		return nullptr;
	}

	UClass* ObjectType = nullptr;
	if (PyTypeObj && !UPyConversion::NativizeClass(PyTypeObj, ObjectType, nullptr))
	{
		UPyUtil::SetPythonError(PyExc_TypeError, InFuncName, *FString::Printf(TEXT("Failed to convert 'type' (%s) to 'Class'"), *UPyUtil::GetFriendlyTypename(PyTypeObj)));
		return nullptr;
	}
	if (!ObjectType)
	{
		ObjectType = UObject::StaticClass();
	}

	bool bFollowRedirectors = true;
	if (PyFollowRedirectorsObj && !UPyConversion::Nativize(PyFollowRedirectorsObj, bFollowRedirectors))
	{
		UPyUtil::SetPythonError(PyExc_TypeError, InFuncName, *FString::Printf(TEXT("Failed to convert 'follow_redirectors' (%s) to 'bool'"), *UPyUtil::GetFriendlyTypename(PyFollowRedirectorsObj)));
		return nullptr;
	}

	UObject* PotentialAsset = InFunc(UObject::StaticClass(), nullptr, *ObjectName);

	// If we found a package, try and get the primary asset from it
	if (UPackage* FoundPackage = Cast<UPackage>(PotentialAsset))
	{
		PotentialAsset = InFunc(UObject::StaticClass(), FoundPackage, *FPackageName::GetShortName(FoundPackage));
	}

	// Follow any redirectors
	if (bFollowRedirectors)
	{
		while (UObjectRedirector* Redirector = Cast<UObjectRedirector>(PotentialAsset))
		{
			PotentialAsset = Redirector->DestinationObject;
		}
	}

	// Make sure the object is an asset of the correct type
	if (PotentialAsset && (!PotentialAsset->IsAsset() || !PotentialAsset->IsA(ObjectType)))
	{
		PotentialAsset = nullptr;
	}

	return UPyConversion::Pythonize(PotentialAsset);
}

PyObject* CallFindAsset(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	return FindOrLoadAssetImpl(InSelf, InArgs, InKwds, TEXT("FindAsset"), [](UClass* ObjectType, UObject* ObjectOuter, const TCHAR* ObjectName)
	{
		UObject* Object = nullptr;
		Py_BEGIN_ALLOW_THREADS
		Object = ::StaticFindObject(ObjectType, ObjectOuter, ObjectName);
		Py_END_ALLOW_THREADS
		return Object;
	});
}

PyObject* CallLoadAsset(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	return FindOrLoadAssetImpl(InSelf, InArgs, InKwds, TEXT("LoadAsset"), [](UClass* ObjectType, UObject* ObjectOuter, const TCHAR* ObjectName)
	{
		UObject* Object = nullptr;
		Py_BEGIN_ALLOW_THREADS
		Object = ::StaticLoadObject(ObjectType, ObjectOuter, ObjectName);
		Py_END_ALLOW_THREADS
		return Object;
	});
}

PyObject* FindOrLoadPackageImpl(PyObject* InSelf, PyObject* InArgs, const TCHAR* InFuncName, const TFunctionRef<UPackage*(const TCHAR*)>& InFunc)
{
	PyObject* PyNameObj = nullptr;

	if (!PyArg_ParseTuple(InArgs, TCHAR_TO_UTF8(*FString::Printf(TEXT("O:%s"), InFuncName)), &PyNameObj))
	{
		return nullptr;
	}

	FString PackageName;
	if (!UPyConversion::Nativize(PyNameObj, PackageName))
	{
		UPyUtil::SetPythonError(PyExc_TypeError, InFuncName, *FString::Printf(TEXT("Failed to convert 'name' (%s) to 'String'"), *UPyUtil::GetFriendlyTypename(PyNameObj)));
		return nullptr;
	}

	return UPyConversion::Pythonize(InFunc(*PackageName));
}

PyObject* CallFindPackage(PyObject* InSelf, PyObject* InArgs)
{
	return FindOrLoadPackageImpl(InSelf, InArgs, TEXT("FindPackage"), [](const TCHAR* PackageName)
	{
		UPackage* Package = nullptr;
		Py_BEGIN_ALLOW_THREADS
		Package = ::FindPackage(nullptr, PackageName);
		Py_END_ALLOW_THREADS
		return Package;
	});
}

PyObject* CallLoadPackage(PyObject* InSelf, PyObject* InArgs)
{
	return FindOrLoadPackageImpl(InSelf, InArgs, TEXT("LoadPackage"), [](const TCHAR* PackageName)
	{
		UPackage* Package = nullptr;
		Py_BEGIN_ALLOW_THREADS
		Package = ::LoadPackage(nullptr, PackageName, LOAD_None);
		Py_END_ALLOW_THREADS
		return Package;
	});
}

PyObject* CallGetDefaultObject(PyObject* InSelf, PyObject* InArgs)
{
	PyObject* PyObj = nullptr;
	if (!PyArg_ParseTuple(InArgs, "O:GetDefaultObject", &PyObj))
	{
		return nullptr;
	}

	UClass* Class = nullptr;
	if (!UPyConversion::NativizeClass(PyObj, Class, nullptr))
	{
		return nullptr;
	}

	UObject* CDO = Class ? ::GetMutableDefault<UObject>(Class) : nullptr;
	return UPyConversion::Pythonize(CDO);
}

PyObject* CallGenerateClass(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	FUPyUClassDecorator* PyClassDecorator = FUPyUClassDecorator::New(&UPyUClassDecoratorType, InArgs, InKwds);
	if (PyClassDecorator)
	{
		PyClassDecorator->Init(PyClassDecorator, InArgs, InKwds);
		return (PyObject*)PyClassDecorator;
	}
	return nullptr;
}

PyObject* CallGenerateStruct(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	FUPyUStructDecorator* PyStructDecorator = FUPyUStructDecorator::New(&UPyUStructDecoratorType, InArgs, InKwds);
	if (PyStructDecorator)
	{
		PyStructDecorator->Init(PyStructDecorator, InArgs, InKwds);
		return (PyObject*)PyStructDecorator;
	}
	return nullptr;
}

PyObject* CallGenerateEnum(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	FUPyUEnumDecorator* PyEnumDecorator = FUPyUEnumDecorator::New(&UPyUEnumDecoratorType, InArgs, InKwds);
	if (PyEnumDecorator)
	{
		PyEnumDecorator->Init(PyEnumDecorator, InArgs, InKwds);
		return (PyObject*)PyEnumDecorator;
	}
	return nullptr;
}

PyObject* CallGenerateValue(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	FUPyUValueDef* PyUValue = FUPyUValueDef::New(&UPyUValueDefType);
	if (PyUValue)
	{
		if (PyUValue->PyInit(PyUValue, InArgs, InKwds) == 0)
		{
			return (PyObject*)PyUValue;
		}
		Py_CLEAR(PyUValue);
	}
	return nullptr;
}

PyObject* CallGenerateProperty(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	FUPyFPropertyDef* PyProperty = FUPyFPropertyDef::New(&UPyFPropertyDefType);
	if (PyProperty)
	{
		if (PyProperty->PyInit(PyProperty, InArgs, InKwds) == 0)
		{
			return (PyObject*)PyProperty;
		}
		Py_CLEAR(PyProperty);
	}
	return nullptr;
}

PyObject* CallGenerateFunction(PyObject* InSelf, PyObject* InArgs, PyObject* InKwds)
{
	FUPyUFunctionDecorator* PyFunctionDecorator = FUPyUFunctionDecorator::New(&UPyUFunctionDecoratorType, InArgs, InKwds);
	if (PyFunctionDecorator)
	{
		PyFunctionDecorator->Init(PyFunctionDecorator, InArgs, InKwds);
		return (PyObject*)PyFunctionDecorator;
	}
	return nullptr;
}

PyObject* CallFlushGeneratedTypeReinstancing(PyObject* InSelf)
{
	Py_BEGIN_ALLOW_THREADS
	FUPyWrapperTypeReinstancer::Get().ProcessPending();
	Py_END_ALLOW_THREADS

	Py_RETURN_NONE;
}

PyObject* CallReloadModule(PyObject* InSelf, PyObject* InArgs)
{
	PyObject* PyModuleOrName = nullptr;
	if (!PyArg_ParseTuple(InArgs, "O:ReloadModule", &PyModuleOrName))
	{
		return nullptr;
	}

	FUPyObjectPtr PyModule;
	if (PyUnicode_Check(PyModuleOrName))
	{
		const FString ModuleName = UPyUtil::PyObjectToUEString(PyModuleOrName);
		PyModule = FUPyObjectPtr::StealReference(PyImport_ImportModule(TCHAR_TO_UTF8(*ModuleName)));
		if (!PyModule)
		{
			return nullptr;
		}
	}
	else if (PyModule_Check(PyModuleOrName))
	{
		PyModule = FUPyObjectPtr::NewReference(PyModuleOrName);
	}
	else
	{
		UPyUtil::SetPythonError(PyExc_TypeError, TEXT("ReloadModule"), *FString::Printf(TEXT("Parameter must be a module or module name string, not '%s'"), *UPyUtil::GetFriendlyTypename(PyModuleOrName)));
		return nullptr;
	}

	FUPyObjectPtr PyImportLib = FUPyObjectPtr::StealReference(PyImport_ImportModule("importlib"));
	if (!PyImportLib)
	{
		return nullptr;
	}

	FUPyObjectPtr PyReloadFunc = FUPyObjectPtr::StealReference(PyObject_GetAttrString(PyImportLib, "reload"));
	if (!PyReloadFunc)
	{
		return nullptr;
	}

	FUPyObjectPtr PyReloadedModule = FUPyObjectPtr::StealReference(PyObject_CallFunctionObjArgs(PyReloadFunc.GetPtr(), PyModule.GetPtr(), nullptr));
	if (!PyReloadedModule)
	{
		return nullptr;
	}

	Py_BEGIN_ALLOW_THREADS
	FUPyWrapperTypeReinstancer::Get().ProcessPending();
	Py_END_ALLOW_THREADS

	return PyReloadedModule.Release();
}

PyObject* CallCollectGarbage(PyObject* InSelf)
{
	Py_BEGIN_ALLOW_THREADS
	::CollectGarbage(IsRunningCommandlet() ? RF_NoFlags : GARBAGE_COLLECTION_KEEPFLAGS);
	Py_END_ALLOW_THREADS

	Py_RETURN_NONE;
}

PyObject* CallGetTypeFromClass(PyObject* InSelf, PyObject* InArgs)
{
	PyObject* PyObj = nullptr;
	if (!PyArg_ParseTuple(InArgs, "O:GetTypeFromClass", &PyObj))
	{
		return nullptr;
	}
	check(PyObj);

	UClass* Class = nullptr;
	if (!UPyConversion::NativizeClass(PyObj, Class, nullptr))
	{
		UPyUtil::SetPythonError(PyExc_TypeError, TEXT("GetTypeFromClass"), *FString::Printf(TEXT("Parameter must be a 'Class' not '%s'"), *UPyUtil::GetFriendlyTypename(PyObj)));
		return nullptr;
	}

	const PyTypeObject* PyType = FUPyWrapperTypeRegistry::Get().GetWrappedClassType(Class);
	Py_INCREF(PyType);
	return (PyObject*)PyType;
}

PyObject* CallGetTypeFromStruct(PyObject* InSelf, PyObject* InArgs)
{
	PyObject* PyObj = nullptr;
	if (!PyArg_ParseTuple(InArgs, "O:GetTypeFromStruct", &PyObj))
	{
		return nullptr;
	}
	check(PyObj);

	UScriptStruct* Struct = nullptr;
	if (!UPyConversion::NativizeStruct(PyObj, Struct, nullptr))
	{
		UPyUtil::SetPythonError(PyExc_TypeError, TEXT("GetTypeFromStruct"), *FString::Printf(TEXT("Parameter must be a 'Struct' not '%s'"), *UPyUtil::GetFriendlyTypename(PyObj)));
		return nullptr;
	}

	const PyTypeObject* PyType = FUPyWrapperTypeRegistry::Get().GetWrappedStructType(Struct);
	Py_INCREF(PyType);
	return (PyObject*)PyType;
}

PyObject* CallGetTypeFromEnum(PyObject* InSelf, PyObject* InArgs)
{
	PyObject* PyObj = nullptr;
	if (!PyArg_ParseTuple(InArgs, "O:GetTypeFromEnum", &PyObj))
	{
		return nullptr;
	}
	check(PyObj);

	UEnum* Enum = nullptr;
	if (!UPyConversion::Nativize(PyObj, Enum))
	{
		UPyUtil::SetPythonError(PyExc_TypeError, TEXT("GetTypeFromEnum"), *FString::Printf(TEXT("Parameter must be a 'Enum' not '%s'"), *UPyUtil::GetFriendlyTypename(PyObj)));
		return nullptr;
	}

	const PyTypeObject* PyType = FUPyWrapperTypeRegistry::Get().GetWrappedEnumType(Enum);
	if (!PyType)
	{
		UPyUtil::SetPythonError(PyExc_TypeError, TEXT("GetTypeFromEnum"), *FString::Printf(TEXT("Enum '%s' does not have a Python wrapper type"), *Enum->GetPathName()));
		return nullptr;
	}

	Py_INCREF(PyType);
	return (PyObject*)PyType;
}

PyObject* CallConvertAbsolutePathApp(PyObject* InSelf, PyObject* InArgs)
{
	PyObject* PyObj = nullptr;
	if (PyArg_ParseTuple(InArgs, "O:ConvertAbsolutePathApp", &PyObj))
	{
		const FString Path = UPyUtil::PyObjectToUEString(PyObj);
		const FString AbsolutePath = IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(*Path);
		return UPyConversion::Pythonize(AbsolutePath);
	}

	return nullptr;
}

PyObject* CallGetContentDir()
{
	return UPyConversion::Pythonize(FPaths::ProjectContentDir());
}

PyObject* CallGetGameSavedDir()
{
	return UPyConversion::Pythonize(FPaths::ProjectSavedDir());
}

PyObject* CallIsEditor()
{
	return UPyConversion::Pythonize(GIsEditor);
}

PyMethodDef UEPyMethodDefs[] = {
	{ "Log", UPyCFunctionCast(&CallLog), METH_VARARGS, UPyDoc_STR("Log(msg: str) -> None -- log the given argument as information in the LogPython category") },
	{ "LogWarning", UPyCFunctionCast(&CallLogWarning), METH_VARARGS, UPyDoc_STR("LogWarning(msg: str) -> None -- log the given argument as a warning in the LogPython category") },
	{ "LogError", UPyCFunctionCast(&CallLogError), METH_VARARGS, UPyDoc_STR("LogError(msg: str) -> None -- log the given argument as an error in the LogPython category") },
	{ "LogFatal", UPyCFunctionCast(&CallLogFatal), METH_VARARGS, UPyDoc_STR("LogFatal(msg: str) -> None -- log the given argument as a fatal error in the LogPython category") },
	{ "LogDisplay", UPyCFunctionCast(&CallLogDisplay), METH_VARARGS, UPyDoc_STR("LogDisplay(msg: str) -> None -- log the given argument as a display message in the LogPython category") },
	{ "LogVerbose", UPyCFunctionCast(&CallLogVerbose), METH_VARARGS, UPyDoc_STR("LogVerbose(msg: str) -> None -- log the given argument as a verbose message in the LogPython category") },
	{ "LogFlush", UPyCFunctionCast(&CallLogFlush), METH_NOARGS, UPyDoc_STR("LogFlush() -> None -- flush the log to disk.") },
	// { "reload", UPyCFunctionCast(&Reload), METH_VARARGS, "reload(module: str) -> None -- reload the given Unreal Python module." },
	// { "load_module", UPyCFunctionCast(&LoadModule), METH_VARARGS, "load_module(module: str) -> None -- load the given Unreal module and generate any Python code for its reflected types" },
	{ "NewObject", UPyCFunctionCast(&CallNewObject), METH_VARARGS | METH_KEYWORDS, UPyDoc_STR("NewObject(Type: Union[Class, type], Outer: Optional[Object]=None, Name: str=\"\", BaseType: Optional[Object]=None) -> Object -- create an Unreal object of the given class (and optional outer and name), optionally validating its type") },
	{ "FindObject", UPyCFunctionCast(&CallFindObject), METH_VARARGS | METH_KEYWORDS, UPyDoc_STR("FindObject(Type: Union[Class, type], Name: str, Outer: Optional[Object]=None, FollowRedirectors: bool=True) -> Object | None -- find an already loaded Unreal object with the given outer and name, optionally validating its type") },
	{ "LoadObject", UPyCFunctionCast(&CallLoadObject), METH_VARARGS | METH_KEYWORDS, UPyDoc_STR("LoadObject(Type: Union[Class, type], Name: str, Outer: Optional[Object]=None, FollowRedirectors: bool=True) -> Object | None -- load an Unreal object with the given outer and name, optionally validating its type") },
	{ "LoadClass", UPyCFunctionCast(&CallLoadClass), METH_VARARGS | METH_KEYWORDS, UPyDoc_STR("LoadClass(Name: str, Type: Union[Class, type]=Object.StaticClass(), Outer: Optional[Object]=None) -> Class | None -- load an Unreal class with the given outer and name, optionally validating its base type") },
	{ "FindAsset", UPyCFunctionCast(&CallFindAsset), METH_VARARGS | METH_KEYWORDS, UPyDoc_STR("FindAsset(Name: str, Type: Union[Class, type]=Object.StaticClass(), FollowRedirectors: bool=True) -> Object | None -- find an already loaded Unreal asset with the given name, optionally validating its type") },
	{ "LoadAsset", UPyCFunctionCast(&CallLoadAsset), METH_VARARGS | METH_KEYWORDS, UPyDoc_STR("LoadAsset(Name: str, Type: Union[Class, type]=Object.StaticClass(), FollowRedirectors: bool=True) -> Object | None -- load an Unreal asset with the given name, optionally validating its type") },
	{ "FindPackage", UPyCFunctionCast(&CallFindPackage), METH_VARARGS, UPyDoc_STR("FindPackage(Name: str) -> Optional[Package] -- find an already loaded Unreal package with the given name") },
	{ "LoadPackage", UPyCFunctionCast(&CallLoadPackage), METH_VARARGS, UPyDoc_STR("LoadPackage(Name: str) -> Optional[Package] -- load an Unreal package with the given name") },
	{ "GetDefaultObject", UPyCFunctionCast(&CallGetDefaultObject), METH_VARARGS, UPyDoc_STR("GetDefaultObject(Type: Union[Class, type]) -> Object | None -- get the Unreal class default object (CDO) of the given type") },
	// { "purge_object_references", UPyCFunctionCast(&PurgeObjectReferences), METH_VARARGS | METH_KEYWORDS, "purge_object_references(obj: Object, include_inners: bool = True) -> None -- purge all references to the given Unreal object from any living Python objects" },
	{ "uclass", UPyCFunctionCast(&CallGenerateClass), METH_VARARGS | METH_KEYWORDS, UPyDoc_STR("uclass() -> None -- generate an Unreal class for the given Python type") },
	{ "ustruct", UPyCFunctionCast(&CallGenerateStruct), METH_VARARGS | METH_KEYWORDS, UPyDoc_STR("ustruct() -> None -- generate an Unreal struct for the given Python type") },
	{ "uenum", UPyCFunctionCast(&CallGenerateEnum), METH_VARARGS | METH_KEYWORDS, UPyDoc_STR("uenum() -> None -- generate an Unreal enum for the given Python type") },
	{ "uvalue", UPyCFunctionCast(&CallGenerateValue), METH_VARARGS | METH_KEYWORDS, UPyDoc_STR("uvalue(Val: int, Meta: Optional[dict[str, Any]]=None) -> ValueDef -- generate an Unreal const value form Python") },
	{ "uproperty", UPyCFunctionCast(&CallGenerateProperty), METH_VARARGS | METH_KEYWORDS, UPyDoc_STR("uproperty(Type: type, Meta: Optional[dict[str, Any]]=None, Getter: Optional[str]=None, Setter: Optional[str]=None) -> PropertyDef -- generate an Unreal FProperty field form Python") },
	{ "ufunction", UPyCFunctionCast(&CallGenerateFunction), METH_VARARGS | METH_KEYWORDS, UPyDoc_STR("ufunction(Meta: Optional[dict[str, Any]]=None, Ret: Optional[type]=None, Params: Optional[list[type]]=None, Override: Optional[bool]=None, Static: Optional[bool]=None, Pure: Optional[bool]=None, Getter: Optional[bool]=None, Setter: Optional[bool]=None) -> FunctionDef -- generate an Unreal FProperty field form Python") },
	{ "FlushGeneratedTypeReinstancing", UPyCFunctionCast(&CallFlushGeneratedTypeReinstancing), METH_NOARGS, UPyDoc_STR("FlushGeneratedTypeReinstancing() -> None -- flush any pending reinstancing requests for Python generated types") },
	{ "ReloadModule", UPyCFunctionCast(&CallReloadModule), METH_VARARGS, UPyDoc_STR("ReloadModule(module_or_name: Union[module, str]) -> module -- reload a Python module and flush pending generated type reinstancing") },
	{ "CollectGarbage", UPyCFunctionCast(&CallCollectGarbage), METH_NOARGS, UPyDoc_STR("CollectGarbage() -> None -- run Unreal garbage collection") },
	{ "GetTypeFromClass", UPyCFunctionCast(&CallGetTypeFromClass), METH_VARARGS, UPyDoc_STR("GetTypeFromClass(Class_: Class) -> type -- get the best matching Python type for the given Unreal class") },
	{ "GetTypeFromStruct", UPyCFunctionCast(&CallGetTypeFromStruct), METH_VARARGS, UPyDoc_STR("GetTypeFromStruct(Struct_: Struct) -> type -- get the best matching Python type for the given Unreal struct") },
	{ "GetTypeFromEnum", UPyCFunctionCast(&CallGetTypeFromEnum), METH_VARARGS, UPyDoc_STR("GetTypeFromEnum(Enum_: Enum) -> type -- get the best matching Python type for the given Unreal enum") },
	// { "register_python_shutdown_callback", UPyCFunctionCast(&RegisterPythonShutdownCallback), METH_VARARGS, "register_python_shutdown_callback(callable: Callable[[], None]) -> object -- register the given callable (with no input arguments) as a callback to execute immediately before Python shutdown"},
	// { "unregister_python_shutdown_callback", UPyCFunctionCast(&UnregisterPythonShutdownCallback), METH_VARARGS, "unregister_python_shutdown_callback(handle: object) -> None -- unregister the given handle from a previous call to register_python_shutdown_callback"},
	// { "NSLOCTEXT", UPyCFunctionCast(&CreateLocalizedText), METH_VARARGS, "NSLOCTEXT(ns: str, key: str, source: str) -> Text -- create a localized Text from the given namespace, key, and source string" },
	// { "LOCTABLE", UPyCFunctionCast(&CreateLocalizedTextFromStringTable), METH_VARARGS, "LOCTABLE(id: Union[Name, str], key: str) -> Text -- get a localized Text from the given string table id and key" },
	{ "IsEditor", UPyCFunctionCast(&CallIsEditor), METH_NOARGS, UPyDoc_STR("IsEditor() -> bool -- tells if the editor is running or not") },
	{ "GetContentDir", UPyCFunctionCast(&CallGetContentDir), METH_NOARGS, UPyDoc_STR("GetContentDir() -> str -- get the content directory of the project") },
	{ "GetGameSavedDir", UPyCFunctionCast(&CallGetGameSavedDir), METH_NOARGS, UPyDoc_STR("GetGameSavedDir() -> str -- get the saved directory of the project") },
	{ "ConvertAbsolutePathApp", UPyCFunctionCast(&CallConvertAbsolutePathApp), METH_VARARGS, UPyDoc_STR("ConvertAbsolutePathApp(path: str) -> str -- convert a path to an absolute path suitable for external applications") },
	// { "get_interpreter_executable_path", UPyCFunctionCast(&GetInterpreterExecutablePath), METH_NOARGS, "get_interpreter_executable_path() -> str -- get the path to the Python interpreter executable of the Python SDK this plugin was compiled against" },
	{nullptr}
};
