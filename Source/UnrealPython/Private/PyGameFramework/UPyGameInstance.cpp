
#include "PyGameFramework/UPyGameInstance.h"
#include "Core/UPyGIL.h"
#include "Core/UPyConversion.h"

UUPyGameInstance::UUPyGameInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, GameInstanceModuleName(TEXT("game_instance"))
	, bIsInitialized(false)
{
}

void UUPyGameInstance::CallModuleFunction(const char* FunctionName, PyObject* PyArgs)
{
	if (GameInstanceModuleName.IsEmpty())
	{
		return;
	}
	
	FUPyScopedGIL GIL;
	
	// Lazy load the module
	if (!GameInstanceModule)
	{
		const FString ModuleNameStr = GameInstanceModuleName;
		GameInstanceModule = FUPyObjectPtr::StealReference(PyImport_ImportModule(TCHAR_TO_UTF8(*ModuleNameStr)));
		if (!GameInstanceModule)
		{
			// Clear error and log, but don't treat as fatal - the module is optional
			if (PyErr_Occurred())
			{
				PyErr_Print(); // Print the traceback to the log (if redirected)
			}
			UE_LOG(LogUnrealPython, Warning, TEXT("Failed to import Python module '%s'. Game instance callbacks will not be executed."), *ModuleNameStr);
			return;
		}
	}
	
	if (!GameInstanceModule)
	{
		return;
	}
	
	// Check if the function exists
	// if (!PyObject_HasAttrString(GameInstanceModule, FunctionName))
	// {
	// 	UE_LOG(LogUnrealPython, Verbose, TEXT("Python function '%s.%s' not found, skipping."), *GameInstanceModuleName, UTF8_TO_TCHAR(FunctionName));
	// 	return;
	// }
	
	FUPyObjectPtr PyFunc = FUPyObjectPtr::StealReference(PyObject_GetAttrString(GameInstanceModule, FunctionName));
	if (!PyFunc || !PyCallable_Check(PyFunc))
	{
		UE_LOG(LogUnrealPython, Warning, TEXT("Python attribute '%s.%s' is not callable."), *GameInstanceModuleName, UTF8_TO_TCHAR(FunctionName));
		return;
	}
	
	FUPyObjectPtr Result = FUPyObjectPtr::StealReference(PyObject_CallObject(PyFunc, PyArgs));
	if (!Result)
	{
		if (PyErr_Occurred())
		{
			PyErr_Print();
		}
		UE_LOG(LogUnrealPython, Error, TEXT("Failed to call Python function '%s.%s'."), *GameInstanceModuleName, UTF8_TO_TCHAR(FunctionName));
	}
}

void UUPyGameInstance::CleanupModule()
{
	if (Py_IsInitialized())
	{
		FUPyScopedGIL GIL;
		PyTickFunction.Reset();
		GameInstanceModule.Reset();
	}
}

FUPyObjectPtr UUPyGameInstance::CacheModuleFunction(const char* FunctionName)
{
	if (!GameInstanceModule)
	{
		return FUPyObjectPtr();
	}
	
	FUPyScopedGIL GIL;
	
	if (!PyObject_HasAttrString(GameInstanceModule, FunctionName))
	{
		UE_LOG(LogUnrealPython, Verbose, TEXT("Python function '%s.%s' not found for caching."), *GameInstanceModuleName, UTF8_TO_TCHAR(FunctionName));
		return FUPyObjectPtr();
	}
	
	PyObject* PyFunc = PyObject_GetAttrString(GameInstanceModule, FunctionName);
	if (PyFunc && PyCallable_Check(PyFunc))
	{
		return FUPyObjectPtr::StealReference(PyFunc);
	}
	
	if (PyFunc)
	{
		Py_DECREF(PyFunc);
	}
	
	UE_LOG(LogUnrealPython, Warning, TEXT("Python attribute '%s.%s' is not callable."), *GameInstanceModuleName, UTF8_TO_TCHAR(FunctionName));
	return FUPyObjectPtr();
}

void UUPyGameInstance::Init()
{
	Super::Init();
	
	bIsInitialized = true;
	
	FUPyScopedGIL GIL;
	
	// Create args tuple with self as parameter
	PyObject* PySelf = UPyConversion::PythonizeObject(this);
	if (PySelf)
	{
		FUPyObjectPtr PyArgs = FUPyObjectPtr::StealReference(PyTuple_Pack(1, PySelf));
		Py_DECREF(PySelf);
		CallModuleFunction("init", PyArgs);
	}
	else
	{
		UE_LOG(LogUnrealPython, Error, TEXT("Failed to pythonize game instance for init callback."));
	}
	
	// Cache tick function and register ticker if tick function exists
	PyTickFunction = CacheModuleFunction("tick");
	if (PyTickFunction)
	{
		RegisterTicker();
	}
}

void UUPyGameInstance::Shutdown()
{
	bIsInitialized = false;
	
	UnregisterTicker();
	
	CallModuleFunction("shutdown");
	CleanupModule();
	
	Super::Shutdown();
}

void UUPyGameInstance::OnStart()
{
	Super::OnStart();
	
	CallModuleFunction("on_start");
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
		
		FUPyObjectPtr Result = FUPyObjectPtr::StealReference(PyObject_CallObject(PyTickFunction, PyArgs));
		if (!Result)
		{
			if (PyErr_Occurred())
			{
				PyErr_Print();
			}
			UE_LOG(LogUnrealPython, Error, TEXT("Failed to call Python function '%s.tick'."), *GameInstanceModuleName);
		}
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
