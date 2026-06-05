
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
	FUPyScopedGIL GIL;

	if (!EnsureModuleLoaded())
	{
		return;
	}

	FUPyObjectPtr CallbackTarget = ResolveCallbackTarget();
	if (!CallbackTarget)
	{
		return;
	}

	FUPyObjectPtr PyFunc = GetCallableAttribute(CallbackTarget, FunctionName);
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
		UE_LOG(LogUnrealPython, Error, TEXT("Failed to call Python function '%s.%s'."), *GameInstanceModuleName, UTF8_TO_TCHAR(FunctionName));
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

FUPyObjectPtr UUPyGameInstance::ResolveCallbackTarget()
{
	if (!GameInstanceModule)
	{
		return FUPyObjectPtr();
	}

	if (PyObject_HasAttrString(GameInstanceModule, "get_game_instance"))
	{
		FUPyObjectPtr PyGetter = FUPyObjectPtr::StealReference(PyObject_GetAttrString(GameInstanceModule, "get_game_instance"));
		if (PyGetter && PyCallable_Check(PyGetter))
		{
			FUPyObjectPtr PyObject = FUPyObjectPtr::StealReference(PyObject_CallObject(PyGetter, nullptr));
			if (!PyObject)
			{
				if (PyErr_Occurred())
				{
					PyErr_Print();
				}
				UE_LOG(LogUnrealPython, Error, TEXT("Failed to call Python function '%s.get_game_instance'."), *GameInstanceModuleName);
				return FUPyObjectPtr();
			}

			if (PyObject != Py_None)
			{
				return PyObject;
			}
		}
		else if (PyGetter)
		{
			UE_LOG(LogUnrealPython, Warning, TEXT("Python attribute '%s.get_game_instance' is not callable."), *GameInstanceModuleName);
		}
	}

	if (PyObject_HasAttrString(GameInstanceModule, "game_instance"))
	{
		FUPyObjectPtr PyObject = FUPyObjectPtr::StealReference(PyObject_GetAttrString(GameInstanceModule, "game_instance"));
		if (PyObject && PyObject != Py_None)
		{
			return PyObject;
		}
	}

	return FUPyObjectPtr::NewReference(GameInstanceModule);
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

bool UUPyGameInstance::HasModuleFunction(const char* FunctionName)
{
	FUPyScopedGIL GIL;
	if (!EnsureModuleLoaded())
	{
		return false;
	}

	FUPyObjectPtr CallbackTarget = ResolveCallbackTarget();
	if (!CallbackTarget)
	{
		return false;
	}

	return static_cast<bool>(GetCallableAttribute(CallbackTarget, FunctionName, false));
}

void UUPyGameInstance::CleanupModule()
{
	if (Py_IsInitialized())
	{
		FUPyScopedGIL GIL;
		GameInstanceModule.Reset();
	}
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
	
	if (HasModuleFunction("tick"))
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
		CallModuleFunction("tick", PyArgs);
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
