
#pragma once

#include "UPyCommon.h"
#include "Core/UPyPtr.h"
#include "Wrapper/UPyWrapperBasic.h"
#include "Utils/UPyUtil.h"
#include "Core/UPyConversion.h"

/** Python type for FUPyDelegateHandle */
extern PyTypeObject UPyDelegateHandleType;

/** Python type for FUPyScopedSlowTask */
extern PyTypeObject UPyScopedSlowTaskType;

/** Python type for FUPyObjectIterator */
extern PyTypeObject UPyObjectIteratorType;

/** Python type for FUPyClassIterator */
extern PyTypeObject UPyClassIteratorType;

/** Python type for FUPyStructIterator */
extern PyTypeObject UPyStructIteratorType;

/** Python type for FUPyTypeIterator */
extern PyTypeObject UPyTypeIteratorType;

/** Python type for FUPyUValueDef */
extern PyTypeObject UPyUValueDefType;

/** Python type for FUPyFPropertyDef */
extern PyTypeObject UPyFPropertyDefType;

/** Python type for FUPyUFunctionDef */
extern PyTypeObject UPyUFunctionDefType;

void InitializeUPyDelegateHandle(UPyGenUtil::FNativePythonModule& ModuleInfo);
void InitializeUPyScopedSlowTask(UPyGenUtil::FNativePythonModule& ModuleInfo);
void InitializeUPyObjectIterator(UPyGenUtil::FNativePythonModule& ModuleInfo);
void InitializeUPyClassIterator(UPyGenUtil::FNativePythonModule& ModuleInfo);
void InitializeUPyStructIterator(UPyGenUtil::FNativePythonModule& ModuleInfo);
void InitializeUPyTypeIterator(UPyGenUtil::FNativePythonModule& ModuleInfo);
void InitializeUPyUValueDef(UPyGenUtil::FNativePythonModule& ModuleInfo);
void InitializeUPyFPropertyDef(UPyGenUtil::FNativePythonModule& ModuleInfo);
void InitializeUPyUFunctionDef(UPyGenUtil::FNativePythonModule& ModuleInfo);

/** Type for all Unreal exposed FDelegateHandle instances */
struct FUPyDelegateHandle : public TUPyWrapperBasic<FDelegateHandle, FUPyDelegateHandle>
{
	typedef TUPyWrapperBasic<FDelegateHandle, FUPyDelegateHandle> Super;

	/** Create and initialize a new wrapper instance from the given native instance */
	static FUPyDelegateHandle* CreateInstance(const FDelegateHandle& InValue);

	/** Cast the given Python object to this wrapped type (returns a new reference) */
	static FUPyDelegateHandle* CastPyObject(PyObject* InPyObject);
};

/** Type used to create and managed a scoped slow task in Python */
struct FUPyScopedSlowTask
{
	/** Common Python Object */
	PyObject_HEAD

	/** Internal slow task instance (created lazily due to having a custom constructor) */
	FSlowTask* SlowTask;

	/** New this instance (called via tp_new for Python, or directly in C++) */
	static FUPyScopedSlowTask* New(PyTypeObject* InType);

	/** Free this instance (called via tp_dealloc for Python) */
	static void Free(FUPyScopedSlowTask* InSelf);

	/** Initialize this instance (called via tp_init for Python, or directly in C++) */
	static int Init(FUPyScopedSlowTask* InSelf, const float InAmountOfWork, const FText& InDefaultMessage, const bool InEnabled);

	/** Deinitialize this instance (called via Init and Free to restore the instance to its New state) */
	static void Deinit(FUPyScopedSlowTask* InSelf);

	/** Called to validate the internal state of this instance prior to operating on it (should be called by all functions that expect to operate on an initialized type; will set an error state on failure) */
	static bool ValidateInternalState(FUPyScopedSlowTask* InSelf);
};

/** Type for iterating Unreal Object instances */
template <typename ObjectType, typename SelfType>
struct TUPyObjectIterator
{
	/** Common Python Object */
	PyObject_HEAD

	/** Internal iterator instance (created lazily due to having a custom constructor) */
	FPersistentThreadSafeObjectIterator* Iterator;

	/** Optional value used when filtering the iterator */
	ObjectType* IteratorFilter;

	/** New this instance (called via tp_new for Python, or directly in C++) */
	static SelfType* New(PyTypeObject* InType)
	{
		SelfType* Self = (SelfType*)InType->tp_alloc(InType, 0);
		if (Self)
		{
			Self->Iterator = nullptr;
			Self->IteratorFilter = nullptr;
		}
		return Self;
	}

	/** Free this instance (called via tp_dealloc for Python) */
	static void Free(SelfType* InSelf)
	{
		Deinit(InSelf);
		Py_TYPE(InSelf)->tp_free((PyObject*)InSelf);
	}

	/** Initialize this instance (called via tp_init for Python, or directly in C++) */
	static int Init(SelfType* InSelf, UClass* InClass, ObjectType* InIteratorFilter)
	{
		Deinit(InSelf);

		InSelf->Iterator = new FPersistentThreadSafeObjectIterator(InClass);
		InSelf->IteratorFilter = InIteratorFilter;

		FPersistentThreadSafeObjectIterator& Iter = *InSelf->Iterator;
		while (*Iter && !SelfType::PassesFilter(InSelf))
		{
			++Iter;
		}

		return 0;
	}

	/** Deinitialize this instance (called via Init and Free to restore the instance to its New state) */
	static void Deinit(SelfType* InSelf)
	{
		delete InSelf->Iterator;
		InSelf->Iterator = nullptr;

		InSelf->IteratorFilter = nullptr;
	}

	/** Called to validate the internal state of this instance prior to operating on it (should be called by all functions that expect to operate on an initialized type; will set an error state on failure) */
	static bool ValidateInternalState(SelfType* InSelf)
	{
		if (!InSelf->Iterator)
		{
			UPyUtil::SetPythonError(PyExc_Exception, Py_TYPE(InSelf), TEXT("Internal Error - Iterator is null!"));
			return false;
		}

		return true;
	}

	/** Get the iterator */
	static SelfType* GetIter(SelfType* InSelf)
	{
		Py_INCREF(InSelf);
		return InSelf;
	}

	/** Return the current value (if any) and advance the iterator */
	static PyObject* IterNext(SelfType* InSelf)
	{
		if (!ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		FPersistentThreadSafeObjectIterator& Iter = *InSelf->Iterator;
		if (*Iter)
		{
			PyObject* PyIterObj = SelfType::GetIterValue(InSelf);
			do
			{
				++Iter;
			}
			while (*Iter && !SelfType::PassesFilter(InSelf));
			return PyIterObj;
		}

		PyErr_SetObject(PyExc_StopIteration, Py_None);
		return nullptr;
	}

	/** Convert the current iterator value to a Python object (internal: define this in a derived type to "override" GetIterValue behavior) */
	static PyObject* GetIterValue(SelfType* InSelf)
	{
		FPersistentThreadSafeObjectIterator& Iter = *InSelf->Iterator;
		return UPyConversion::Pythonize(*Iter);
	}

	/** True if the current iterator value passes the filter (internal: define this in a derived type to "override" PassesFilter behavior) */
	static bool PassesFilter(SelfType* InSelf)
	{
		return true;
	}
};

/** Type for iterating Unreal Object instances */
struct FUPyObjectIterator : public TUPyObjectIterator<UObject, FUPyObjectIterator>
{
	typedef TUPyObjectIterator<UObject, FUPyObjectIterator> Super;
};

/** Type for iterating Unreal class types */
struct FUPyClassIterator : public TUPyObjectIterator<UClass, FUPyClassIterator>
{
	typedef TUPyObjectIterator<UClass, FUPyClassIterator> Super;

	/** True if the current iterator value passes the filter */
	static bool PassesFilter(FUPyClassIterator* InSelf);

	/** Extract the filter from the given Python object */
	static UClass* ExtractFilter(FUPyClassIterator* InSelf, PyObject* InPyFilter);
};

/** Type for iterating Unreal struct types */
struct FUPyStructIterator : public TUPyObjectIterator<UScriptStruct, FUPyStructIterator>
{
	typedef TUPyObjectIterator<UScriptStruct, FUPyStructIterator> Super;

	/** True if the current iterator value passes the filter */
	static bool PassesFilter(FUPyStructIterator* InSelf);

	/** Extract the filter from the given Python object */
	static UScriptStruct* ExtractFilter(FUPyStructIterator* InSelf, PyObject* InPyFilter);
};

/** Type for iterating Python types */
struct FUPyTypeIterator : public TUPyObjectIterator<UStruct, FUPyTypeIterator>
{
	typedef TUPyObjectIterator<UStruct, FUPyTypeIterator> Super;

	/** Convert the current iterator value to a Python object */
	static PyObject* GetIterValue(FUPyTypeIterator* InSelf);

	/** True if the current iterator value passes the filter */
	static bool PassesFilter(FUPyTypeIterator* InSelf);

	/** Extract the filter from the given Python object */
	static UStruct* ExtractFilter(FUPyTypeIterator* InSelf, PyObject* InPyFilter);
};

/** Type used to define constant values from Python */
struct FUPyUValueDef
{
	/** Common Python Object */
	PyObject_HEAD

	/** Value of this definition */
	PyObject* Value;

	/** Dictionary of meta-data associated with this value */
	PyObject* MetaData;

	/** New this instance (called via tp_new for Python, or directly in C++) */
	static FUPyUValueDef* New(PyTypeObject* InType);

	/** Free this instance (called via tp_dealloc for Python) */
	static void Free(FUPyUValueDef* InSelf);

	/** Initialize this instance (called via tp_init for Python, or directly in C++) */
	static int Init(FUPyUValueDef* InSelf, PyObject* InValue, PyObject* InMetaData);
	
	static int PyInit(FUPyUValueDef* InSelf, PyObject* InArgs, PyObject* InKwds);

	/** Deinitialize this instance (called via Init and Free to restore the instance to its New state) */
	static void Deinit(FUPyUValueDef* InSelf);

	/** Apply the meta-data on this instance via the given predicate */
	static void ApplyMetaData(FUPyUValueDef* InSelf, const TFunctionRef<void(const FString&, const FString&)>& InPredicate);
};

/** Type used to define FProperty fields from Python */
struct FUPyFPropertyDef
{
	/** Common Python Object */
	PyObject_HEAD

	/** Type of this property */
	PyObject* PropType;

	/** Dictionary of meta-data associated with this property */
	PyObject* MetaData;

	/** Getter function to use with this property */
	FString GetterFuncName;

	/** Setter function to use with this property */
	FString SetterFuncName;

	/** Whether this property should be marked for network replication */
	bool bReplicated;

	/** Optional RepNotify function to call when this property is replicated */
	FString RepNotifyFuncName;

	/** Whether this property should be marked with a RepNotify function */
	bool bRepNotify;

	/** New this instance (called via tp_new for Python, or directly in C++) */
	static FUPyFPropertyDef* New(PyTypeObject* InType);

	/** Free this instance (called via tp_dealloc for Python) */
	static void Free(FUPyFPropertyDef* InSelf);

	/** Initialize this instance (called via tp_init for Python, or directly in C++) */
	static int Init(FUPyFPropertyDef* InSelf, PyObject* InPropType, PyObject* InMetaData, FString InGetterFuncName, FString InSetterFuncName, bool bInReplicated, FString InRepNotifyFuncName, bool bInRepNotify);

	static int PyInit(FUPyFPropertyDef* InSelf, PyObject* InArgs, PyObject* InKwds);
	
	/** Deinitialize this instance (called via Init and Free to restore the instance to its New state) */
	static void Deinit(FUPyFPropertyDef* InSelf);

	/** Apply the meta-data on this instance to the given property */
	static void ApplyMetaData(FUPyFPropertyDef* InSelf, FProperty* InProp);
};

/** Flags used to define the attributes of a UFunction field from Python */
enum class EUPyUFunctionDefFlags : uint16
{
	None = 0,
	Override = 1<<0,
	Static = 1<<1,
	Pure = 1<<2,
	Impure = 1<<3,
	Getter = 1<<4,
	Setter = 1<<5,
	Server = 1<<6,
	Client = 1<<7,
	NetMulticast = 1<<8,
	Reliable = 1<<9,
	Unreliable = 1<<10,
};
ENUM_CLASS_FLAGS(EUPyUFunctionDefFlags);

/** Type used to define UFunction fields from Python */
struct FUPyUFunctionDef
{
	/** Common Python Object */
	PyObject_HEAD

	/** Python function to call */
	PyObject* Func;

	/** Return type of this function */
	PyObject* FuncRetType;

	/** List of types for each parameter of this function */
	PyObject* FuncParamTypes;

	/** Dictionary of meta-data associated with this function */
	PyObject* MetaData;

	/** Flags used to define this function */
	EUPyUFunctionDefFlags FuncFlags;

	/** New this instance (called via tp_new for Python, or directly in C++) */
	static FUPyUFunctionDef* New(PyTypeObject* InType);

	/** Free this instance (called via tp_dealloc for Python) */
	static void Free(FUPyUFunctionDef* InSelf);

	/** Initialize this instance (called via tp_init for Python, or directly in C++) */
	static int Init(FUPyUFunctionDef* InSelf, PyObject* InFunc, PyObject* InFuncRetType, PyObject* InFuncParamTypes, PyObject* InMetaData, EUPyUFunctionDefFlags InFuncFlags);

	static int PyInit(FUPyUFunctionDef* InSelf, PyObject* InArgs, PyObject* InKwds);
	
	/** Deinitialize this instance (called via Init and Free to restore the instance to its New state) */
	static void Deinit(FUPyUFunctionDef* InSelf);

	/** Apply the meta-data on this instance to the given function */
	static void ApplyMetaData(FUPyUFunctionDef* InSelf, UFunction* InFunc);
};

typedef TUPyPtr<FUPyDelegateHandle> FUPyDelegateHandlePtr;
typedef TUPyPtr<FUPyScopedSlowTask> FUPyScopedSlowTaskPtr;
typedef TUPyPtr<FUPyObjectIterator> FUPyObjectIteratorPtr;
typedef TUPyPtr<FUPyClassIterator> FUPyClassIteratorPtr;
typedef TUPyPtr<FUPyStructIterator> FUPyStructIteratorPtr;
typedef TUPyPtr<FUPyTypeIterator> FUPyTypeIteratorPtr;
typedef TUPyPtr<FUPyUValueDef> FUPyUValueDefPtr;
typedef TUPyPtr<FUPyFPropertyDef> FUPyFPropertyDefPtr;
typedef TUPyPtr<FUPyUFunctionDef> FUPyUFunctionDefPtr;
