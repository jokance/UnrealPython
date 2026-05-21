
#pragma once

#if WITH_EDITOR
#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"

class FUnrealPythonCommandExecutor : public IConsoleCommandExecutor
{
public:
	static FName StaticName();
	virtual FName GetName() const override;
	virtual FText GetDisplayName() const override;
	virtual FText GetDescription() const override;
	virtual FText GetHintText() const override;
	virtual void GetSuggestedCompletions(const TCHAR* Input, TArray<FConsoleSuggestion>& Out) override;
	virtual void GetExecHistory(TArray<FString>& Out) override;
	virtual bool Exec(const TCHAR* Input) override;
	virtual bool AllowHotKeyClose() const override;
	virtual bool AllowMultiLine() const override;
	virtual FInputChord GetHotKey() const override;
	virtual FInputChord GetIterateExecutorHotKey() const override;
};
#endif // WITH_EDITOR
