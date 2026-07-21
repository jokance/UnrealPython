
#include "PyGameFramework/UPyTimerManager.h"
#include "Utils/UPyUtil.h"

TMap<int32, FTSTicker::FDelegateHandle> UUPyTimerManager::ActiveTimers;
int32 UUPyTimerManager::NextTimerId = 1;
FUPyAutoGILPtr UUPyTimerManager::OnTimerFunc;

int32 UUPyTimerManager::SetTimer(float Rate)
{
	return SetTimerInternal(Rate, true);
}

int32 UUPyTimerManager::SetOnceTimer(float Delay)
{
	return SetTimerInternal(Delay, false);
}

int32 UUPyTimerManager::SetTimerInternal(float Rate, bool bLoop)
{
	if (Rate < 0.0f)
	{
		UE_LOG(LogUnrealPython, Warning, TEXT("UUPyTimerManager::SetTimer - Rate must be non-negative"));
		return 0;
	}
	
	int32 TimerId = NextTimerId++;
	
	// Register with ticker using the rate as the delay
	FTSTicker::FDelegateHandle TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda([bLoop, TimerId](float DeltaTime) -> bool
		{
			// Call Python unreal_timer.on_timer(handle)
			const bool bCallbackSucceeded = OnTimerCallback(TimerId);

			if (!bCallbackSucceeded || !bLoop || !ActiveTimers.Contains(TimerId))
			{
				ActiveTimers.Remove(TimerId);
				return false;
			}
			
			return true;  // Continue for looping timers
		}),
		Rate  // Delay time in seconds
	);
	
	ActiveTimers.Add(TimerId, TickerHandle);
	
	UE_LOG(LogUnrealPython, Verbose, TEXT("UUPyTimerManager: Created timer %u (rate=%.3f, loop=%d)"), TimerId, Rate, bLoop);
	
	return TimerId;
}

bool UUPyTimerManager::OnTimerCallback(int32 Handle)
{
	if (!Py_IsInitialized())
	{
		return false;
	}

	FUPyScopedGIL GIL;
	
	// Lazy cache the on_timer function
	if (!OnTimerFunc.Get())
	{
		// Import unreal_timer module
		PyObject* TimerModule = PyImport_ImportModule("unreal_timer");
		if (!TimerModule)
		{
			if (PyErr_Occurred())
			{
				UPyUtil::LogPythonError();
			}
			UE_LOG(LogUnrealPython, Error, TEXT("UUPyTimerManager: Failed to import unreal_timer module"));
			return false;
		}
		
		// Get on_timer function
		PyObject* Func = PyObject_GetAttrString(TimerModule, "on_timer");
		Py_DECREF(TimerModule);
		
		if (!Func || !PyCallable_Check(Func))
		{
			Py_XDECREF(Func);
			if (PyErr_Occurred())
			{
				UPyUtil::LogPythonError();
			}
			UE_LOG(LogUnrealPython, Error, TEXT("UUPyTimerManager: unreal_timer.on_timer not found or not callable"));
			return false;
		}
		
		OnTimerFunc = FUPyAutoGILPtr(FUPyObjectPtr::StealReference(Func));
	}
	
	// Call on_timer(handle)
	FUPyObjectPtr PyHandle = FUPyObjectPtr::StealReference(PyLong_FromLong(Handle));
	FUPyObjectPtr Args = PyHandle ? FUPyObjectPtr::StealReference(PyTuple_Pack(1, PyHandle.Get())) : FUPyObjectPtr();
	if (!Args)
	{
		UPyUtil::LogPythonError();
		return false;
	}

	FUPyObjectPtr Result = FUPyObjectPtr::StealReference(PyObject_CallObject(OnTimerFunc.Get(), Args));

	if (!Result)
	{
		if (PyErr_Occurred())
		{
			UPyUtil::LogPythonError();
		}
		UE_LOG(LogUnrealPython, Error, TEXT("UUPyTimerManager: Failed to call unreal_timer.on_timer (handle=%u)"), Handle);
		return false;
	}

	return true;
}

void UUPyTimerManager::ClearTimer(int32 Handle)
{
	if (Handle == 0)
	{
		return;
	}
	
	FTSTicker::FDelegateHandle* TickerHandle = ActiveTimers.Find(Handle);
	if (TickerHandle)
	{
		if (TickerHandle->IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(*TickerHandle);
		}
		
		ActiveTimers.Remove(Handle);
		
		UE_LOG(LogUnrealPython, Verbose, TEXT("UUPyTimerManager: Cleared timer %u"), Handle);
	}
}

void UUPyTimerManager::ClearAllTimers()
{
	for (auto& Pair : ActiveTimers)
	{
		if (Pair.Value.IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(Pair.Value);
		}
	}
	
	ActiveTimers.Empty();
	
	UE_LOG(LogUnrealPython, Verbose, TEXT("UUPyTimerManager: Cleared all timers"));
}

void UUPyTimerManager::Shutdown()
{
	ClearAllTimers();
	NextTimerId = 1;

	if (Py_IsInitialized())
	{
		FUPyScopedGIL GIL;
		OnTimerFunc.Get().Reset();
	}
	else
	{
		OnTimerFunc.Get().Release();
	}
}
