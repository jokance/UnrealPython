
#pragma once

#include "UPyCommon.h"
#include "Core/UPyPtr.h"

/** Utility to handle taking a releasing the Python GIL within a scope */
class UNREALPYTHON_API FUPyScopedGIL
{
public:
	/** Constructor - take the GIL */
	FUPyScopedGIL()
		: GILState(PyGILState_Ensure())
	{
	}

	/** Destructor - release the GIL */
	~FUPyScopedGIL()
	{
		PyGILState_Release(GILState);
	}

	/** Non-copyable */
	FUPyScopedGIL(const FUPyScopedGIL&) = delete;
	FUPyScopedGIL& operator=(const FUPyScopedGIL&) = delete;

private:
	/** Internal GIL state */
	PyGILState_STATE GILState;
};

/** Wrapper of a TUPyPtr that can be safely copied/moved/destroyed by code where the GIL might not currently be held (eg, a lambda bound to a delegate) */
template <typename TPythonType>
class TUPyAutoGILPtr
{
public:
	TUPyAutoGILPtr() = default;

	explicit TUPyAutoGILPtr(TUPyPtr<TPythonType>&& InPtr)
		: Ptr(MoveTemp(InPtr))
	{
	}

	TUPyAutoGILPtr(const TUPyAutoGILPtr& InOther)
	{
		// This may be called after Python has already shut down
		if (Py_IsInitialized())
		{
			FUPyScopedGIL GIL;
			Ptr = InOther.Ptr;
		}
	}

	TUPyAutoGILPtr(TUPyAutoGILPtr&& InOther)
	{
		// This may be called after Python has already shut down
		if (Py_IsInitialized())
		{
			FUPyScopedGIL GIL;
			Ptr = MoveTemp(InOther.Ptr);
		}
	}

	~TUPyAutoGILPtr()
	{
		// This may be called after Python has already shut down
		if (Py_IsInitialized())
		{
			FUPyScopedGIL GIL;
			Ptr.Reset();
		}
		else
		{
			// Release ownership if Python has been shut down to avoid attempting to delete the object (which is already dead)
			Ptr.Release();
		}
	}

	TUPyAutoGILPtr& operator=(const TUPyAutoGILPtr& InOther)
	{
		if (this != &InOther)
		{
			// This may be called after Python has already shut down
			if (Py_IsInitialized())
			{
				FUPyScopedGIL GIL;
				Ptr = InOther.Ptr;
			}
			else
			{
				// Release ownership if Python has been shut down to avoid attempting to delete the object (which is already dead)
				Ptr.Release();
			}
		}
		return *this;
	}

	TUPyAutoGILPtr& operator=(TUPyAutoGILPtr&& InOther)
	{
		if (this != &InOther)
		{
			// This may be called after Python has already shut down
			if (Py_IsInitialized())
			{
				FUPyScopedGIL GIL;
				Ptr = MoveTemp(InOther.Ptr);
			}
			else
			{
				// Release ownership if Python has been shut down to avoid attempting to delete the object (which is already dead)
				Ptr.Release();
			}
		}
		return *this;
	}

	TUPyPtr<TPythonType>& Get()
	{
		return Ptr;
	}

	const TUPyPtr<TPythonType>& Get() const
	{
		return Ptr;
	}

private:
	TUPyPtr<TPythonType> Ptr;
};

typedef TUPyAutoGILPtr<PyObject> FUPyAutoGILPtr;
