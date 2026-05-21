
#pragma once

#include "DynamicTypes/UPyGeneratedWrappedType.h"
#include "Utils/UPyGenUtil.h"
#include "Wrapper/UPyWrapperEnum.h"

/** Stores the data needed by a runtime generated Python enum entry */
struct FGeneratedWrappedEnumEntry
{
	FGeneratedWrappedEnumEntry() = default;
	FGeneratedWrappedEnumEntry(FGeneratedWrappedEnumEntry&&) = delete;
	FGeneratedWrappedEnumEntry(const FGeneratedWrappedEnumEntry&) = default;
	FGeneratedWrappedEnumEntry& operator=(FGeneratedWrappedEnumEntry&&) = delete;
	FGeneratedWrappedEnumEntry& operator=(const FGeneratedWrappedEnumEntry&) = default;

	/** Array of deprecated aliases for this enum entry, if any */
	TArray<UPyGenUtil::FUTF8Buffer> EntryDeprecatedAliases;

	/** The name of the entry */
	UPyGenUtil::FUTF8Buffer EntryName;

	/** The doc string of the entry */
	UPyGenUtil::FUTF8Buffer EntryDoc;

	/** The value of the entry */
	int64 EntryValue = 0;
};

/** Stores the data needed by a runtime generated Python enum type */
struct FUPyGeneratedWrappedEnumType : public UPyGenUtil::FGeneratedWrappedType
{
	FUPyGeneratedWrappedEnumType() = default;
	FUPyGeneratedWrappedEnumType(FUPyGeneratedWrappedEnumType&&) = delete;
	FUPyGeneratedWrappedEnumType(const FUPyGeneratedWrappedEnumType&) = delete;
	FUPyGeneratedWrappedEnumType& operator=(FUPyGeneratedWrappedEnumType&&) = delete;
	FUPyGeneratedWrappedEnumType& operator=(const FUPyGeneratedWrappedEnumType&) = delete;

	/** Internal version of Finalize, called after readying the type with Python */
	virtual void Finalize_PostReady() override;

	/** Internal version of Reset, called to remove data added to the Python type */
	virtual void Reset_CleansePyType() override;

	/** Internal version of Reset, called to reset the data on this type */
	virtual void Reset_CleanseSelf() override;

	/** Called to extract the enum entries array from the given enum */
	void ExtractEnumEntries(const UEnum* InEnum);

	/** Array of entries associated with this enum */
	TArray<FGeneratedWrappedEnumEntry> EnumEntries;

	/** Array of enum entries in the order they were added (enum entries are stored as borrowed references) */
	TArray<FUPyWrapperEnum*> PyEnumEntries;
};
