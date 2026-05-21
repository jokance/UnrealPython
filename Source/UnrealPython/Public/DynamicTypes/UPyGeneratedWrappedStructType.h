
#pragma once

#include  "DynamicTypes/UPyGeneratedWrappedType.h"

/** Stores the data needed by a runtime generated Python struct type */
struct FUPyGeneratedWrappedStructType : public UPyGenUtil::FGeneratedWrappedType, public UPyGenUtil::TGeneratedWrappedDynamicMethodsMixin<FUPyGeneratedWrappedStructType>, public UPyGenUtil::TGeneratedWrappedDynamicConstantsMixin<FUPyGeneratedWrappedStructType>
{
	FUPyGeneratedWrappedStructType() = default;
	FUPyGeneratedWrappedStructType(FUPyGeneratedWrappedStructType&&) = delete;
	FUPyGeneratedWrappedStructType(const FUPyGeneratedWrappedStructType&) = delete;
	FUPyGeneratedWrappedStructType& operator=(FUPyGeneratedWrappedStructType&&) = delete;
	FUPyGeneratedWrappedStructType& operator=(const FUPyGeneratedWrappedStructType&) = delete;

	/** Internal version of Finalize, called before readying the type with Python */
	virtual void Finalize_PreReady() override;

	/** Get/sets associated with this type */
	UPyGenUtil::FGeneratedWrappedGetSets GetSets;

	/** The doc string data for the properties associated with this type */
	// TArray<FGeneratedWrappedPropertyDoc> PropertyDocs;

	/** Tracks and logs field conflicts on this type */
	// FGeneratedWrappedFieldTracker FieldTracker;
};