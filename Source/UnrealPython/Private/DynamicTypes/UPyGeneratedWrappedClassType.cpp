// Copyright Epic Games, Inc. All Rights Reserved.

#include "DynamicTypes/UPyGeneratedWrappedClassType.h"
#include "Core/UPyGIL.h"
#include "Wrapper/UPyWrapperTypeRegistry.h"

void FUPyGeneratedWrappedClassType::Finalize_PreReady()
{
	FGeneratedWrappedType::Finalize_PreReady();

	Methods.Finalize();

	GetSets.Finalize();
	PyType.tp_getset = GetSets.PyGetSets.GetData();

	Constants.Finalize();
}

void FUPyGeneratedWrappedClassType::Finalize_PostReady()
{
	FGeneratedWrappedType::Finalize_PostReady();

	// Execute Python code within this block
	{
		FUPyScopedGIL GIL;
		FUPyMethodWithClosureDef::AddMethods(Methods.PyMethods.GetData(), &PyType);
		FUPyConstantDef::AddConstantsToType(Constants.PyConstants.GetData(), &PyType);
	}
}
