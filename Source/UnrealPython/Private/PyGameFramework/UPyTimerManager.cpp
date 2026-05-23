
#include "PyGameFramework/UPyTimerManager.h"

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
			OnTimerCallback(TimerId);
			
			if (!bLoop)
			{
				// One-shot timer, remove from map
				ActiveTimers.Remove(TimerId);
				if (ActiveTimers.Num() == 0)
				{
					ReleaseTimerCallback();
				}
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

void UUPyTimerManager::OnTimerCallback(int32 Handle)
{
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
				PyErr_Print();
				PyErr_Clear();
			}
			UE_LOG(LogUnrealPython, Error, TEXT("UUPyTimerManager: Failed to import unreal_timer module"));
			return;
		}
		
		// Get on_timer function
		PyObject* Func = PyObject_GetAttrString(TimerModule, "on_timer");
		Py_DECREF(TimerModule);
		
		if (!Func || !PyCallable_Check(Func))
		{
			Py_XDECREF(Func);
			if (PyErr_Occurred())
			{
				PyErr_Print();
				PyErr_Clear();
			}
			UE_LOG(LogUnrealPython, Error, TEXT("UUPyTimerManager: unreal_timer.on_timer not found or not callable"));
			return;
		}
		
		OnTimerFunc = FUPyAutoGILPtr(FUPyObjectPtr::StealReference(Func));
	}
	
	// Call on_timer(handle)
	PyObject* PyHandle = PyLong_FromUnsignedLong(Handle);
	PyObject* Args = PyTuple_Pack(1, PyHandle);
	Py_DECREF(PyHandle);
	
	PyObject* Result = PyObject_CallObject(OnTimerFunc.Get(), Args);
	Py_DECREF(Args);
	
	if (!Result)
	{
		if (PyErr_Occurred())
		{
			PyErr_Print();
			PyErr_Clear();
		}
		UE_LOG(LogUnrealPython, Error, TEXT("UUPyTimerManager: Failed to call unreal_timer.on_timer (handle=%u)"), Handle);
	}
	else
	{
		Py_DECREF(Result);
	}
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
		if (ActiveTimers.Num() == 0)
		{
			ReleaseTimerCallback();
		}
		
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
	ReleaseTimerCallback();
	
	UE_LOG(LogUnrealPython, Verbose, TEXT("UUPyTimerManager: Cleared all timers"));
}

void UUPyTimerManager::ReleaseTimerCallback()
{
	OnTimerFunc = FUPyAutoGILPtr();
}
