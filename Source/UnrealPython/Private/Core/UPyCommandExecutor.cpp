// Copyright Epic Games, Inc. All Rights Reserved.

#include "UPyCommandExecutor.h"

#if WITH_EDITOR
#include "Core/UPyGIL.h"
#include "UnrealPython.h"
#include "Framework/Commands/InputChord.h"

FName FUnrealPythonCommandExecutor::StaticName()
{
	static const FName CmdExecName = TEXT("UnrealPython");
	return CmdExecName;
}

FName FUnrealPythonCommandExecutor::GetName() const
{
	return StaticName();
}

FText FUnrealPythonCommandExecutor::GetDisplayName() const
{
	return NSLOCTEXT("UnrealPython", "CommandExecutorDisplayName", "Unreal Python");
}

FText FUnrealPythonCommandExecutor::GetDescription() const
{
	return NSLOCTEXT("UnrealPython", "CommandExecutorDescription", "Execute Python scripts through the Unreal Python plugin.");
}

FText FUnrealPythonCommandExecutor::GetHintText() const
{
	return NSLOCTEXT("UnrealPython", "CommandExecutorHint", "Enter Python script");
}

void FUnrealPythonCommandExecutor::GetSuggestedCompletions(const TCHAR* Input, TArray<FConsoleSuggestion>& Out)
{
	// TODO: Implement auto-completion
}

void FUnrealPythonCommandExecutor::GetExecHistory(TArray<FString>& Out)
{
	IConsoleManager::Get().GetConsoleHistory(TEXT("UnrealPython"), Out);
}

bool FUnrealPythonCommandExecutor::Exec(const TCHAR* Input)
{
	IConsoleManager::Get().AddConsoleHistoryEntry(TEXT("UnrealPython"), Input);

	UE_LOG(LogUnrealPython, Log, TEXT("%s"), Input);

	FUPyScopedGIL GIL;
	PyRun_SimpleString(TCHAR_TO_UTF8(Input));
	return true;
}

bool FUnrealPythonCommandExecutor::AllowHotKeyClose() const
{
	return true;
}

bool FUnrealPythonCommandExecutor::AllowMultiLine() const
{
	return true;
}

FInputChord FUnrealPythonCommandExecutor::GetHotKey() const
{
	return FInputChord();
}

FInputChord FUnrealPythonCommandExecutor::GetIterateExecutorHotKey() const
{
	return FInputChord();
}
#endif // WITH_EDITOR
