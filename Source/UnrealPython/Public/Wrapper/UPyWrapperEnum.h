
#pragma once

#include "UPyCommon.h"
#include "UObject/Class.h"
#include "Core/UPyPtr.h"
#include "Utils/UPyGenUtil.h"
#include "Utils/UPyUtil.h"
#include "Core/UPyConversionResult.h"

class UObjectRedirector;

/** Python type for FUPyWrapperEnum */
extern PyTypeObject UPyWrapperEnumType;

/** Initialize the PyWrapperEnum types and add them to the given Python module */
void InitializeUPyWrapperEnum(UPyGenUtil::FNativePythonModule& ModuleInfo);

/** Type for all Unreal exposed enum instances (an instance is created for each entry in the enum, before the enum type is locked for creating new instances) */
struct FUPyWrapperEnum
{
	/** Common Python variable-sized object header; enum instances are PyLong subtypes. */
	PyObject_VAR_HEAD

	/** New this wrapper instance (called via tp_new for Python, or directly in C++) */
	static FUPyWrapperEnum* New(PyTypeObject* InType);

	/** New this wrapper instance with the given numeric value (called directly in C++) */
	static FUPyWrapperEnum* New(PyTypeObject* InType, const int64 InEnumEntryValue);

	/** Initialize this wrapper instance (called via tp_init for Python, or directly in C++) */
	static int Init(FUPyWrapperEnum* InSelf);

	/** Initialize this wrapper instance (called directly in C++) */
	static int Init(FUPyWrapperEnum* InSelf, const int64 InEnumEntryValue, const char* InEnumEntryName);

	/** Called to validate the internal state of this wrapper instance prior to operating on it (should be called by all functions that expect to operate on an initialized type; will set an error state on failure) */
	static bool ValidateInternalState(FUPyWrapperEnum* InSelf);

	/** Cast the given Python object to this wrapped type (returns a new reference) */
	static FUPyWrapperEnum* CastPyObject(PyObject* InPyObject, FUPyConversionResult* OutCastResult = nullptr);

	/** Cast the given Python object to this wrapped type, or attempt to convert the type into a new wrapped instance (returns a new reference) */
	static FUPyWrapperEnum* CastPyObject(PyObject* InPyObject, const PyTypeObject* InType, FUPyConversionResult* OutCastResult = nullptr);

	/** Get the name of the enum entry as a string */
	static FString GetEnumEntryName(FUPyWrapperEnum* InSelf);

	/** Get the value of the enum entry as an int */
	static int64 GetEnumEntryValue(FUPyWrapperEnum* InSelf);

	/** Add the given enum entry on the given enum type (returns borrowed reference) */
	static FUPyWrapperEnum* AddEnumEntry(PyTypeObject* InType, const int64 InEnumEntryValue, const char* InEnumEntryName);
};

typedef TUPyPtr<FUPyWrapperEnum> FUPyWrapperEnumPtr;

typedef TArrayView<FUPyWrapperEnum* const> FPyWrapperEnumArrayView;

/** Iterator used with enums */
struct FUPyWrapperEnumIterator
{
	/** Common Python Object */
	PyObject_HEAD

	/** Array being iterated over */
	FPyWrapperEnumArrayView IterArray;

	/** Current iteration index */
	int32 IterIndex;

	/** New this iterator instance (called via tp_new for Python, or directly in C++) */
	static FUPyWrapperEnumIterator* New(PyTypeObject* InType)
	{
		FUPyWrapperEnumIterator* Self = (FUPyWrapperEnumIterator*)InType->tp_alloc(InType, 0);
		if (Self)
		{
			new(&Self->IterArray) FPyWrapperEnumArrayView();
			Self->IterIndex = 0;
		}
		return Self;
	}

	/** Free this iterator instance (called via tp_dealloc for Python) */
	static void Free(FUPyWrapperEnumIterator* InSelf)
	{
		Deinit(InSelf);

		InSelf->IterArray.~FPyWrapperEnumArrayView();
		Py_TYPE(InSelf)->tp_free((PyObject*)InSelf);
	}

	/** Initialize this iterator instance (called via tp_init for Python, or directly in C++) */
	static int Init(FUPyWrapperEnumIterator* InSelf, FPyWrapperEnumArrayView InIterArray)
	{
		Deinit(InSelf);

		InSelf->IterArray = InIterArray;
		InSelf->IterIndex = 0;

		return 0;
	}

	/** Deinitialize this iterator instance (called via Init and Free to restore the instance to its New state) */
	static void Deinit(FUPyWrapperEnumIterator* InSelf)
	{
		InSelf->IterArray = FPyWrapperEnumArrayView();
		InSelf->IterIndex = 0;
	}

	/** Get the iterator */
	static FUPyWrapperEnumIterator* GetIter(FUPyWrapperEnumIterator* InSelf)
	{
		Py_INCREF(InSelf);
		return InSelf;
	}

	/** Return the current value (if any) and advance the iterator */
	static PyObject* IterNext(FUPyWrapperEnumIterator* InSelf)
	{
		if (InSelf->IterArray.IsValidIndex(InSelf->IterIndex))
		{
			FUPyWrapperEnum* EnumEntry = InSelf->IterArray[InSelf->IterIndex++];
			Py_INCREF(EnumEntry);
			return (PyObject*)EnumEntry;
		}

		PyErr_SetObject(PyExc_StopIteration, Py_None);
		return nullptr;
	}
};
