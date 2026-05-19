// Copyright Epic Games, Inc. All Rights Reserved.

#include "DynamicTypes/UPyGeneratedWrappedStructType.h"

void FUPyGeneratedWrappedStructType::Finalize_PreReady()
{
	FGeneratedWrappedType::Finalize_PreReady();

	GetSets.Finalize();
	PyType.tp_getset = GetSets.PyGetSets.GetData();
}
