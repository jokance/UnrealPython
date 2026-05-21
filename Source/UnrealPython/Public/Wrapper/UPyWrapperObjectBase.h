
#pragma once

#include "UPyCommon.h"
#include "Wrapper/UPyWrapperBase.h"
#include "DynamicTypes/UPyGeneratedWrappedType.h"
#include "Core/UPyConversion.h"
#include "Core/UPyConversionResult.h"
#include "UObject/PropertyAccessUtil.h"

/** Python type for FUPyWrapperObjectBase */
extern UNREALPYTHON_API PyTypeObject UPyWrapperObjectBaseType;

/** Initialize the PyWrapperObjectBase types and add them to the given Python module */
void InitializeUPyWrapperObjectBase(UPyGenUtil::FNativePythonModule& ModuleInfo);

/** Type for all Unreal exposed object instances */
struct UNREALPYTHON_API FUPyWrapperObjectBase : public FUPyWrapperBase
{
	/** Wrapped object instance */
	TObjectPtr<UObject> ObjectInstance;

	/** New this wrapper instance (called via tp_new for Python, or directly in C++) */
	static FUPyWrapperObjectBase* New(PyTypeObject* InType);

	/** Free this wrapper instance (called via tp_dealloc for Python) */
	static void Free(FUPyWrapperObjectBase* InSelf);

	/** Initialize this wrapper instance to the given value (called via tp_init for Python, or directly in C++) */
	static int Init(FUPyWrapperObjectBase* InSelf, UObject* InValue);

	/** Deinitialize this wrapper instance (called via Init and Free to restore the instance to its New state) */
	static void Deinit(FUPyWrapperObjectBase* InSelf);

	/** Called to validate the internal state of this wrapper instance prior to operating on it (should be called by all functions that expect to operate on an initialized type; will set an error state on failure) */
	static bool ValidateInternalState(FUPyWrapperObjectBase* InSelf);

	/** Cast the given Python object to this wrapped type (returns a new reference) */
	static FUPyWrapperObjectBase* CastPyObject(PyObject* InPyObject, FUPyConversionResult* OutCastResult = nullptr);

	/** Cast the given Python object to this wrapped type, or attempt to convert the type into a new wrapped instance (returns a new reference) */
	static FUPyWrapperObjectBase* CastPyObject(PyObject* InPyObject, PyTypeObject* InType, FUPyConversionResult* OutCastResult = nullptr);

	/** Get a property value from this instance (called via generated code) */
	static PyObject* GetPropertyValue(FUPyWrapperObjectBase* InSelf, const UPyGenUtil::FGeneratedWrappedProperty& InPropDef, const char* InPythonAttrName);

	/** Set a property value on this instance (called via generated code) */
	static int SetPropertyValue(FUPyWrapperObjectBase* InSelf, PyObject* InValue, const UPyGenUtil::FGeneratedWrappedProperty& InPropDef, const char* InPythonAttrName, const EPropertyAccessChangeNotifyMode InNotifyMode = EPropertyAccessChangeNotifyMode::Never, const uint64 InReadOnlyFlags = PropertyAccessUtil::RuntimeReadOnlyFlags);

	/** Call a named getter function on this class using the given instance (called via generated code) */
	static PyObject* CallGetterFunction(FUPyWrapperObjectBase* InSelf, const UPyGenUtil::FGeneratedWrappedFunction& InFuncDef);

	/** Call a named setter function on this class using the given instance (called via generated code) */
	static int CallSetterFunction(FUPyWrapperObjectBase* InSelf, PyObject* InValue, const UPyGenUtil::FGeneratedWrappedFunction& InFuncDef);

	/** Call a function on this class (called via generated code) */
	static PyObject* CallFunction(PyTypeObject* InType, const UPyGenUtil::FGeneratedWrappedFunction& InFuncDef, const char* InPythonFuncName);

	/** Call a function on this class (called via generated code) */
	static PyObject* CallFunction(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds, const UPyGenUtil::FGeneratedWrappedFunction& InFuncDef, const char* InPythonFuncName);

	/** Call a function on this class using the given instance (called via generated code) */
	static PyObject* CallFunction(FUPyWrapperObjectBase* InSelf, const UPyGenUtil::FGeneratedWrappedFunction& InFuncDef, const char* InPythonFuncName);

	/** Call a function on this class using the given instance (called via generated code) */
	static PyObject* CallFunction(FUPyWrapperObjectBase* InSelf, PyObject* InArgs, PyObject* InKwds, const UPyGenUtil::FGeneratedWrappedFunction& InFuncDef, const char* InPythonFuncName);

	/** Call a function on this instance (CallFunction internal use only) */
	static PyObject* CallFunction_Impl(UObject* InObj, const UPyGenUtil::FGeneratedWrappedFunction& InFuncDef, const char* InPythonFuncName, const TCHAR* InErrorCtxt);

	/** Call a function on this instance (CallFunction internal use only) */
	static PyObject* CallFunction_Impl(UObject* InObj, PyObject* InArgs, PyObject* InKwds, const UPyGenUtil::FGeneratedWrappedFunction& InFuncDef, const char* InPythonFuncName, const TCHAR* InErrorCtxt);

	/** Implementation of the "call" logic for a Python class method with no arguments (internal Python bindings use only) */
	static PyObject* CallClassMethodNoArgs_Impl(PyTypeObject* InType, void* InClosure);

	/** Implementation of the "call" logic for a Python class method with arguments (internal Python bindings use only) */
	static PyObject* CallClassMethodWithArgs_Impl(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds, void* InClosure);

	/** Implementation of the "call" logic for a Python method with no arguments (internal Python bindings use only) */
	static PyObject* CallMethodNoArgs_Impl(FUPyWrapperObjectBase* InSelf, void* InClosure);

	/** Implementation of the "call" logic for a Python method with arguments (internal Python bindings use only) */
	static PyObject* CallMethodWithArgs_Impl(FUPyWrapperObjectBase* InSelf, PyObject* InArgs, PyObject* InKwds, void* InClosure);

	/** Call a dynamic function on this instance (CallFunction internal use only) */
	static PyObject* CallDynamicFunction_Impl(FUPyWrapperObjectBase* InSelf, PyObject* InArgs, PyObject* InKwds, const UPyGenUtil::FGeneratedWrappedFunction& InFuncDef, const UPyGenUtil::FGeneratedWrappedMethodParameter& InSelfParam, const char* InPythonFuncName);

	/** Implementation of the "call" logic for a dynamic Python method with no arguments (internal Python bindings use only) */
	static PyObject* CallDynamicMethodNoArgs_Impl(FUPyWrapperObjectBase* InSelf, void* InClosure);

	/** Implementation of the "call" logic for a dynamic Python method with arguments (internal Python bindings use only) */
	static PyObject* CallDynamicMethodWithArgs_Impl(FUPyWrapperObjectBase* InSelf, PyObject* InArgs, PyObject* InKwds, void* InClosure);

	/** Implementation of the "getter" logic for a Python descriptor reading from an object property (internal Python bindings use only) */
	static PyObject* Getter_Impl(FUPyWrapperObjectBase* InSelf, void* InClosure);

	/** Implementation of the "setter" logic for a Python descriptor writing to an object property (internal Python bindings use only) */
	static int Setter_Impl(FUPyWrapperObjectBase* InSelf, PyObject* InValue, void* InClosure);
};

typedef TUPyPtr<FUPyWrapperObjectBase> FUPyWrapperObjectBasePtr;
