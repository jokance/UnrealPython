// Copyright Epic Games, Inc. All Rights Reserved.

#include "DynamicTypes/UPyGeneratedWrappedEnumType.h"
#include "Core/UPyGIL.h"
#include "Utils/UPyGenUtil.h"
#include "Wrapper/UPyWrapperEnum.h"

void FUPyGeneratedWrappedEnumType::Finalize_PostReady()
{
	FGeneratedWrappedType::Finalize_PostReady();

	FUPyScopedGIL GIL;
	for (const FGeneratedWrappedEnumEntry& EnumEntry : EnumEntries)
	{
		FUPyWrapperEnum* PyEnumEntry = FUPyWrapperEnum::AddEnumEntry(&PyType, EnumEntry.EntryValue, EnumEntry.EntryName.GetData(), EnumEntry.EntryDoc.GetData());
		if (PyEnumEntry)
		{
			PyEnumEntries.Add(PyEnumEntry);
		}
	}
}

void FUPyGeneratedWrappedEnumType::Reset_CleansePyType()
{
	// Execute Python code within this block
	{
		// Unregister the existing enum entries
		FUPyScopedGIL GIL;
		for (const FGeneratedWrappedEnumEntry& EnumEntry : EnumEntries)
		{
			PyDict_DelItemString(PyType.tp_dict, EnumEntry.EntryName.GetData());
		}
	}

	FGeneratedWrappedType::Reset_CleansePyType();
}

void FUPyGeneratedWrappedEnumType::Reset_CleanseSelf()
{
	EnumEntries.Reset();

	FGeneratedWrappedType::Reset_CleanseSelf();
}

void FUPyGeneratedWrappedEnumType::ExtractEnumEntries(const UEnum* InEnum)
{
	for (int32 EnumEntryIndex = 0; EnumEntryIndex < InEnum->NumEnums() - 1; ++EnumEntryIndex)
	{
		if (UPyGenUtil::ShouldExportEnumEntry(InEnum, EnumEntryIndex))
		{
			FGeneratedWrappedEnumEntry& EnumEntry = EnumEntries.AddDefaulted_GetRef();
			EnumEntry.EntryName = UPyGenUtil::TCHARToUTF8Buffer(*UPyGenUtil::GetEnumEntryPythonName(InEnum, EnumEntryIndex));
			// EnumEntry.EntryDoc = UPyGenUtil::TCHARToUTF8Buffer(*PythonizeTooltip(ParseTooltip(GetEnumEntryTooltip(InEnum, EnumEntryIndex))));
			EnumEntry.EntryValue = InEnum->GetValueByIndex(EnumEntryIndex);

			// const TArray<FString> DeprecatedEnumEntryNames = GetDeprecatedEnumEntryPythonNames(InEnum, EnumEntryIndex);
			// EnumEntry.EntryDeprecatedAliases.Reserve(DeprecatedEnumEntryNames.Num());
			// for (const FString& DeprecatedEnumEntryName : DeprecatedEnumEntryNames)
			// {
			// 	EnumEntry.EntryDeprecatedAliases.Add(TCHARToUTF8Buffer(*DeprecatedEnumEntryName));
			// }
		}
	}
}