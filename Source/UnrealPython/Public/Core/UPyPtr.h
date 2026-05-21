
#pragma once

#include "UPyCommon.h"
#include "Misc/AssertionMacros.h"


/** Wrapper that handles the ref-counting of the contained Python object */
template <typename TPythonType>
class  TUPyPtr
{
public:
	/** Create a null pointer */
	TUPyPtr()
		: Ptr(nullptr)
	{
	}

	/** Copy a pointer */
	TUPyPtr(const TUPyPtr& InOther)
		: Ptr(InOther.Ptr)
	{
		Py_XINCREF(Ptr);
	}

	/** Copy a pointer */
	template <typename TOtherPythonType>
	TUPyPtr(const TUPyPtr<TOtherPythonType>& InOther)
		: Ptr(InOther.Ptr)
	{
		Py_XINCREF(Ptr);
	}

	/** Move a pointer */
	TUPyPtr(TUPyPtr&& InOther)
		: Ptr(InOther.Ptr)
	{
		InOther.Ptr = nullptr;
	}

	/** Move a pointer */
	template <typename TOtherPythonType>
	TUPyPtr(TUPyPtr<TOtherPythonType>&& InOther)
		: Ptr(InOther.Ptr)
	{
		InOther.Ptr = nullptr;
	}

	/** Release our reference to this object */
	~TUPyPtr()
	{
		Py_XDECREF(Ptr);
	}

	/** Create a pointer from the given object, incrementing its reference count */
	static TUPyPtr NewReference(TPythonType* InPtr)
	{
		return TUPyPtr(InPtr, true);
	}

	/** Create a pointer from the given object, leaving its reference count alone */
	static TUPyPtr StealReference(TPythonType* InPtr)
	{
		return TUPyPtr(InPtr, false);
	}

	/** Copy a pointer */
	TUPyPtr& operator=(const TUPyPtr& InOther)
	{
		if (this != &InOther)
		{
			Py_XDECREF(Ptr);
			Ptr = InOther.Ptr;
			Py_XINCREF(Ptr);
		}
		return *this;
	}

	/** Copy a pointer */
	template <typename TOtherPythonType>
	TUPyPtr& operator=(const TUPyPtr<TOtherPythonType>& InOther)
	{
		if (this != &InOther)
		{
			Py_XDECREF(Ptr);
			Ptr = InOther.Ptr;
			Py_XINCREF(Ptr);
		}
		return *this;
	}

	/** Move a pointer */
	TUPyPtr& operator=(TUPyPtr&& InOther)
	{
		if (this != &InOther)
		{
			Py_XDECREF(Ptr);
			Ptr = InOther.Ptr;
			InOther.Ptr = nullptr;
		}
		return *this;
	}

	/** Move a pointer */
	template <typename TOtherPythonType>
	TUPyPtr& operator=(TUPyPtr<TOtherPythonType>&& InOther)
	{
		if (this != &InOther)
		{
			Py_XDECREF(Ptr);
			Ptr = InOther.Ptr;
			InOther.Ptr = nullptr;
		}
		return *this;
	}

	explicit operator bool() const
	{
		return Ptr != nullptr;
	}

	bool IsValid() const
	{
		return Ptr != nullptr;
	}

	operator TPythonType*() const
	{
		return Ptr;
	}

	TPythonType& operator*()
	{
		check(Ptr);
		return *Ptr;
	}

	const TPythonType& operator*() const
	{
		check(Ptr);
		return *Ptr;
	}

	TPythonType* operator->()
	{
		check(Ptr);
		return Ptr;
	}

	const TPythonType* operator->() const
	{
		check(Ptr);
		return Ptr;
	}

	TPythonType*& Get()
	{
		return Ptr;
	}

	TPythonType* GetPtr() const
	{
		return Ptr;
	}

	TPythonType* Release()
	{
		TPythonType* RetPtr = Ptr;
		Ptr = nullptr;
		return RetPtr;
	}

	void Reset()
	{
		Py_XDECREF(Ptr);
		Ptr = nullptr;
	}

private:
	TUPyPtr(TPythonType* InPtr, const bool bIncRef)
		: Ptr(InPtr)
	{
		if (bIncRef)
		{
			Py_XINCREF(Ptr);
		}
	}

	TPythonType* Ptr;
};

typedef TUPyPtr<PyObject> FUPyObjectPtr;
typedef TUPyPtr<PyTypeObject> FUPyTypeObjectPtr;

