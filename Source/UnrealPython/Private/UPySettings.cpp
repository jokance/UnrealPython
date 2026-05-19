// Copyright Epic Games, Inc. All Rights Reserved.

#include "UPySettings.h"
#include "Misc/Paths.h"

#define LOCTEXT_NAMESPACE "UnrealPythonSettings"

UUPySettings::UUPySettings()
{
	CategoryName = TEXT("Plugins");
}

#if WITH_EDITOR
void UUPySettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName MemberPropertyName = (PropertyChangedEvent.MemberProperty != nullptr) ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

	if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(UUPySettings, AdditionalPaths))
	{
		for (FDirectoryPath& DirPath : AdditionalPaths)
		{
			if (!DirPath.Path.IsEmpty() && !FPaths::IsRelative(DirPath.Path))
			{
				FPaths::MakePathRelativeTo(DirPath.Path, *FPaths::ProjectDir());
			}
		}
	}
}

FText UUPySettings::GetSectionText() const
{
	return LOCTEXT("UnrealPythonSettingsDisplayName", "Unreal Python");
}
#endif

#undef LOCTEXT_NAMESPACE
