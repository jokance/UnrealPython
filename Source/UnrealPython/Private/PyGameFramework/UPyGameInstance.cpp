
#include "PyGameFramework/UPyGameInstance.h"
#include "Core/UPyGIL.h"
#include "Core/UPyConversion.h"

UUPyGameInstance::UUPyGameInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, GameInstanceModuleName(TEXT("game_instance"))
	, GameInstanceFactoryFunctionName(TEXT("create_game_instance"))
	, bIsInitialized(false)
{
}

void UUPyGameInstance::CallGameInstanceFunction(const char* FunctionName, PyObject* PyArgs)
{
	CallPythonObjectFunction(GameInstanceObject.GetPtr(), FunctionName, PyArgs);
}

void UUPyGameInstance::CallPythonObjectFunction(PyObject* Target, const char* FunctionName, PyObject* PyArgs)
{
	FUPyScopedGIL GIL;

	if (!Target)
	{
		return;
	}

	FUPyObjectPtr PyFunc = GetCallableAttribute(Target, FunctionName);
	if (!PyFunc)
	{
		return;
	}

	FUPyObjectPtr Result = FUPyObjectPtr::StealReference(PyObject_CallObject(PyFunc, PyArgs));
	if (!Result)
	{
		if (PyErr_Occurred())
		{
			PyErr_Print();
		}
		UE_LOG(LogUnrealPython, Error, TEXT("Failed to call Python GameInstance function '%s.%s'."), *GameInstanceModuleName, UTF8_TO_TCHAR(FunctionName));
	}
}

bool UUPyGameInstance::EnsureModuleLoaded()
{
	if (GameInstanceModuleName.IsEmpty())
	{
		return false;
	}

	if (GameInstanceModule)
	{
		return true;
	}

	const FString ModuleNameStr = GameInstanceModuleName;
	GameInstanceModule = FUPyObjectPtr::StealReference(PyImport_ImportModule(TCHAR_TO_UTF8(*ModuleNameStr)));
	if (!GameInstanceModule)
	{
		if (PyErr_Occurred())
		{
			PyErr_Print();
		}
		UE_LOG(LogUnrealPython, Warning, TEXT("Failed to import Python module '%s'. Game instance callbacks will not be executed."), *ModuleNameStr);
		return false;
	}

	return true;
}

bool UUPyGameInstance::CreateGameInstanceObject()
{
	if (GameInstanceObject)
	{
		return true;
	}

	if (!EnsureModuleLoaded())
	{
		return false;
	}

	if (GameInstanceFactoryFunctionName.IsEmpty())
	{
		UE_LOG(LogUnrealPython, Error, TEXT("GameInstanceFactoryFunctionName is empty for Python module '%s'."), *GameInstanceModuleName);
		return false;
	}

	FUPyObjectPtr PyFactory = GetCallableAttribute(GameInstanceModule, TCHAR_TO_UTF8(*GameInstanceFactoryFunctionName));
	if (!PyFactory)
	{
		return false;
	}

	PyObject* PySelf = UPyConversion::PythonizeObject(this);
	if (!PySelf)
	{
		UE_LOG(LogUnrealPython, Error, TEXT("Failed to pythonize game instance for '%s.%s'."), *GameInstanceModuleName, *GameInstanceFactoryFunctionName);
		return false;
	}

	FUPyObjectPtr PyArgs = FUPyObjectPtr::StealReference(PyTuple_Pack(1, PySelf));
	Py_DECREF(PySelf);
	if (!PyArgs)
	{
		if (PyErr_Occurred())
		{
			PyErr_Print();
		}
		UE_LOG(LogUnrealPython, Error, TEXT("Failed to create arguments for Python function '%s.%s'."), *GameInstanceModuleName, *GameInstanceFactoryFunctionName);
		return false;
	}

	GameInstanceObject = FUPyObjectPtr::StealReference(PyObject_CallObject(PyFactory, PyArgs));
	if (!GameInstanceObject)
	{
		if (PyErr_Occurred())
		{
			PyErr_Print();
		}
		UE_LOG(LogUnrealPython, Error, TEXT("Failed to call Python function '%s.%s'."), *GameInstanceModuleName, *GameInstanceFactoryFunctionName);
		return false;
	}

	if (GameInstanceObject == Py_None)
	{
		UE_LOG(LogUnrealPython, Error, TEXT("Python function '%s.%s' returned None."), *GameInstanceModuleName, *GameInstanceFactoryFunctionName);
		GameInstanceObject.Reset();
		return false;
	}

	return true;
}

FUPyObjectPtr UUPyGameInstance::GetCallableAttribute(PyObject* Target, const char* FunctionName, bool bWarnIfNotCallable)
{
	if (!Target || !PyObject_HasAttrString(Target, FunctionName))
	{
		UE_LOG(LogUnrealPython, Verbose, TEXT("Python function '%s.%s' not found, skipping."), *GameInstanceModuleName, UTF8_TO_TCHAR(FunctionName));
		return FUPyObjectPtr();
	}

	FUPyObjectPtr PyFunc = FUPyObjectPtr::StealReference(PyObject_GetAttrString(Target, FunctionName));
	if (PyFunc && PyCallable_Check(PyFunc))
	{
		return PyFunc;
	}

	if (bWarnIfNotCallable)
	{
		UE_LOG(LogUnrealPython, Warning, TEXT("Python attribute '%s.%s' is not callable."), *GameInstanceModuleName, UTF8_TO_TCHAR(FunctionName));
	}
	return FUPyObjectPtr();
}

bool UUPyGameInstance::HasGameInstanceFunction(const char* FunctionName)
{
	FUPyScopedGIL GIL;
	if (!GameInstanceObject)
	{
		return false;
	}

	return static_cast<bool>(GetCallableAttribute(GameInstanceObject, FunctionName, false));
}

void UUPyGameInstance::CleanupModule()
{
	if (Py_IsInitialized())
	{
		FUPyScopedGIL GIL;
		GameInstanceObject.Reset();
		GameInstanceModule.Reset();
	}
}

void UUPyGameInstance::CallAfterShutdown(PyObject* PythonGameInstanceObject)
{
	CallPythonObjectFunction(PythonGameInstanceObject, "after_shutdown");
}

void UUPyGameInstance::Init()
{
	Super::Init();
	
	bIsInitialized = true;

	FUPyScopedGIL GIL;

	if (CreateGameInstanceObject())
	{
		PyObject* PySelf = UPyConversion::PythonizeObject(this);
		if (!PySelf)
		{
			UE_LOG(LogUnrealPython, Error, TEXT("Failed to pythonize game instance for init callback."));
			return;
		}

		FUPyObjectPtr PyArgs = FUPyObjectPtr::StealReference(PyTuple_Pack(1, PySelf));
		Py_DECREF(PySelf);
		CallGameInstanceFunction("init", PyArgs);

		if (HasGameInstanceFunction("tick"))
		{
			RegisterTicker();
		}
	}
}

void UUPyGameInstance::Shutdown()
{
	bIsInitialized = false;
	
	UnregisterTicker();

	FUPyObjectPtr AfterShutdownObject;
	if (Py_IsInitialized() && GameInstanceObject)
	{
		FUPyScopedGIL GIL;
		AfterShutdownObject = GameInstanceObject;
	}

	CallGameInstanceFunction("shutdown");
	Super::Shutdown();
	CleanupModule();

	if (Py_IsInitialized() && AfterShutdownObject)
	{
		FUPyScopedGIL GIL;
		CallAfterShutdown(AfterShutdownObject.GetPtr());
		AfterShutdownObject.Reset();
	}
	else if (AfterShutdownObject)
	{
		AfterShutdownObject.Release();
	}
}

void UUPyGameInstance::OnStart()
{
	Super::OnStart();

	CallGameInstanceFunction("on_start");
}

bool UUPyGameInstance::Tick(float DeltaTime)
{
	if (IsEngineExitRequested())
	{
		return false;
	}
	
	FUPyScopedGIL GIL;
	
	PyObject* PyDeltaTime = nullptr;
	if (UPyConversion::Pythonize(DeltaTime, PyDeltaTime))
	{
		FUPyObjectPtr PyArgs = FUPyObjectPtr::StealReference(PyTuple_Pack(1, PyDeltaTime));
		Py_DECREF(PyDeltaTime);
		CallGameInstanceFunction("tick", PyArgs);
	}

	return true;
}

void UUPyGameInstance::RegisterTicker()
{
	if (TickerHandle.IsValid())
	{
		return;
	}
	
	TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UUPyGameInstance::Tick),
		0.0f  // Tick every frame
	);
}

void UUPyGameInstance::UnregisterTicker()
{
	if (TickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		TickerHandle.Reset();
	}
}

void UUPyGameInstance::PyGC()
{
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
}
