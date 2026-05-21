
#pragma once

#include "DynamicTypes/UPyGeneratedWrappedType.h"
#include "DynamicTypes/UPyGeneratedWrappedStructType.h"

/** Stores the data needed by a runtime generated Python class type */
struct FUPyGeneratedWrappedClassType : public UPyGenUtil::FGeneratedWrappedType, public UPyGenUtil::TGeneratedWrappedDynamicMethodsMixin<FUPyGeneratedWrappedStructType>, public UPyGenUtil::TGeneratedWrappedDynamicConstantsMixin<FUPyGeneratedWrappedStructType>
{
	FUPyGeneratedWrappedClassType() = default;
	FUPyGeneratedWrappedClassType(FUPyGeneratedWrappedClassType&&) = delete;
	FUPyGeneratedWrappedClassType(const FUPyGeneratedWrappedClassType&) = delete;
	FUPyGeneratedWrappedClassType& operator=(FUPyGeneratedWrappedClassType&&) = delete;
	FUPyGeneratedWrappedClassType& operator=(const FUPyGeneratedWrappedClassType&) = delete;

	/** Internal version of Finalize, called before readying the type with Python */
	virtual void Finalize_PreReady() override;

	/** Internal version of Finalize, called after readying the type with Python */
	virtual void Finalize_PostReady() override;

	/** Methods associated with this type */
	UPyGenUtil::FGeneratedWrappedMethods Methods;

	/** Get/sets associated with this type */
	UPyGenUtil::FGeneratedWrappedGetSets GetSets;

	/** Constants associated with this type */
	UPyGenUtil::FGeneratedWrappedConstants Constants;

	/** The doc string data for the properties associated with this type */
	TArray<UPyGenUtil::FGeneratedWrappedPropertyDoc> PropertyDocs;

	/** Tracks and logs field conflicts on this type */
	UPyGenUtil::FGeneratedWrappedFieldTracker FieldTracker;
};