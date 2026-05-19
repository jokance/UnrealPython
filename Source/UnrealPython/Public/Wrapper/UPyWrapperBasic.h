// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Wrapper/UPyWrapperBase.h"

template <typename SelfType>
void InitializeUPyWrapperBasicType(PyTypeObject* InPyType, const char* InTypeDoc)
{
	struct FFuncs
	{
		static PyObject* New(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
		{
			return (PyObject*)SelfType::New(InType);
		}

		static void Dealloc(SelfType* InSelf)
		{
			SelfType::Free(InSelf);
		}

		static int Init(SelfType* InSelf, PyObject* InArgs, PyObject* InKwds)
		{
			return SelfType::Init(InSelf);
		}
	};

	InPyType->tp_base = &UPyWrapperBaseType;
	InPyType->tp_new = (newfunc)&FFuncs::New;
	InPyType->tp_dealloc = (destructor)&FFuncs::Dealloc;
	InPyType->tp_init = (initproc)&FFuncs::Init;

	InPyType->tp_flags = Py_TPFLAGS_DEFAULT;
	InPyType->tp_doc = InTypeDoc;
}

/** Base type for any Unreal exposed simple value instances (that just copy data into Python) */
template <typename ValueType, typename SelfType>
struct TUPyWrapperBasic : public FUPyWrapperBase
{
	/** The wrapped instance */
	ValueType Value;

	/** New this wrapper instance (called via tp_new for Python, or directly in C++) */
	static SelfType* New(PyTypeObject* InType)
	{
		SelfType* Self = (SelfType*)FUPyWrapperBase::New(InType);
		if (Self)
		{
			new(&Self->Value) ValueType();
		}
		return Self;
	}

	/** Free this wrapper instance (called via tp_dealloc for Python) */
	static void Free(SelfType* InSelf)
	{
		Deinit(InSelf);

		InSelf->Value.~ValueType();
		FUPyWrapperBase::Free(InSelf);
	}

	/** Initialize this wrapper instance (called via tp_init for Python, or directly in C++) */
	static int Init(SelfType* InSelf)
	{
		Deinit(InSelf);

		const int BaseInit = FUPyWrapperBase::Init(InSelf);
		if (BaseInit != 0)
		{
			return BaseInit;
		}

		SelfType::InitValue(InSelf, ValueType());
		return 0;
	}

	/** Initialize this wrapper instance to the given value (called directly in C++) */
	static int Init(SelfType* InSelf, const ValueType InValue)
	{
		Deinit(InSelf);

		const int BaseInit = FUPyWrapperBase::Init(InSelf);
		if (BaseInit != 0)
		{
			return BaseInit;
		}

		SelfType::InitValue(InSelf, InValue);
		return 0;
	}

	/** Deinitialize this wrapper instance (called via Init and Free to restore the instance to its New state) */
	static void Deinit(SelfType* InSelf)
	{
		SelfType::DeinitValue(InSelf);
	}

	/** Initialize the value of this wrapper instance (internal: define this in a derived type to "override" InitValue behavior) */
	static void InitValue(SelfType* InSelf, const ValueType InValue)
	{
		InSelf->Value = InValue;
	}

	/** Deinitialize the value of this wrapper instance (internal: define this in a derived type to "override" DeinitValue behavior) */
	static void DeinitValue(SelfType* InSelf)
	{
		InSelf->Value = ValueType();
	}
};
