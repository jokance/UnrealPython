
#pragma once

#include "CoreMinimal.h"
#include "Core/UPyGIL.h"
#include "UPyTimerManager.generated.h"

/**
 * Manages timers that call Python callbacks
 * Uses FTSTicker for frame-based timing
 */
UCLASS()
class UNREALPYTHON_API UUPyTimerManager : public UObject
{
	GENERATED_BODY()
	
public:
	/**
	 * Set a repeating timer
	 * @param Rate - Time interval between callbacks in seconds
	 * @return Timer handle that can be used to clear the timer
	 */
	UFUNCTION(BlueprintCallable, Category = "Python|Timer")
	static int32 SetTimer(float Rate);
	
	/**
	 * Set a one-shot timer
	 * @param Delay - Time delay before the callback in seconds
	 * @return Timer handle that can be used to clear the timer
	 */
	UFUNCTION(BlueprintCallable, Category = "Python|Timer")
	static int32 SetOnceTimer(float Delay);
	
	/**
	 * Clear a timer by its handle
	 * @param Handle - The timer handle to clear
	 */
	UFUNCTION(BlueprintCallable, Category = "Python|Timer")
	static void ClearTimer(int32 Handle);
	
	/**
	 * Clear all active timers
	 */
	UFUNCTION(BlueprintCallable, Category = "Python|Timer")
	static void ClearAllTimers();

private:
	static int32 SetTimerInternal(float Rate, bool bLoop);
	static void OnTimerCallback(int32 Handle);
	static void ReleaseTimerCallback();

	static TMap<int32, FTSTicker::FDelegateHandle> ActiveTimers;
	static int32 NextTimerId;
	
	/** Cached Python on_timer function for better performance */
	static FUPyAutoGILPtr OnTimerFunc;
};
