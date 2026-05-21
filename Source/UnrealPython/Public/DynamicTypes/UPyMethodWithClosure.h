
#pragma once

#include "UPyCommon.h"
#include "CoreMinimal.h"


/** Python type for FUPyCFunctionWithClosureObject */
extern PyTypeObject UPyCFunctionWithClosureType;

/** Python type for FUPyMethodWithClosureDescrObject */
extern PyTypeObject UPyMethodWithClosureDescrType;

/** Python type for FUPyMethodWithClosureDescrObject (for class methods) */
extern PyTypeObject UPyClassMethodWithClosureDescrType;

/** Initialize the PyMethodWithClosure types */
void InitializeUPyMethodWithClosure();

/** Shutdown the PyMethodWithClosure types */
void ShutdownUPyMethodWithClosure();

/** C function pointers used by FUPyMethodWithClosureDef */
typedef PyObject*(*UPyCFunctionWithClosure)(PyObject*, PyObject*, void*);
typedef PyObject*(*UPyCFunctionWithClosureAndKeywords)(PyObject*, PyObject*, PyObject*, void*);
typedef PyObject*(*UPyCFunctionWithClosureAndNoArgs)(PyObject*, void*);

/** Cast a function pointer to UPyCFunctionWithClosure (via a void* to avoid a compiler warning) */
#define UPyCFunctionWithClosureCast(FUNCPTR) (UPyCFunctionWithClosure)(void*)(FUNCPTR)

/** Definition for a method with a closure (equivalent to PyMethodDef) */
struct FUPyMethodWithClosureDef
{
	/* The name of the method */
	const char* MethodName;

	/* The C function this method should call */
	UPyCFunctionWithClosure MethodCallback;

	/* Combination of METH_ flags describing how the C function should be called */
	int MethodFlags;

	/* The method documentation, or nullptr */
	const char* MethodDoc;

	/** The closure passed to the C function, or nullptr */
	void* MethodClosure;

	/** Add a singular method to the given type */
	static bool AddMethod(FUPyMethodWithClosureDef* InMethod, PyTypeObject* InType);

	/** Add the given null-terminated table of methods to the given type */
	static bool AddMethods(FUPyMethodWithClosureDef* InMethods, PyTypeObject* InType);

	/** Create a method descriptor for the given definition */
	static PyObject* NewMethodDescriptor(PyTypeObject* InType, FUPyMethodWithClosureDef* InDef);

	/** Call the method with the given self and arguments */
	static PyObject* Call(FUPyMethodWithClosureDef* InDef, PyObject* InSelf, PyObject* InArgs, PyObject* InKwds);
};

/** Python object for the bound callable form of method with a closure (equivalent to PyCFunctionObject) */
struct FUPyCFunctionWithClosureObject
{
	/** Common Python Object */
	PyObject_HEAD
	
	/* Definition of the method to call */
	FUPyMethodWithClosureDef* MethodDef;
	
	/* The 'self' argument passed to the C function, or nullptr */
    PyObject* SelfArg;
	
	/* The __module__ attribute, can be anything */
    PyObject* ModuleAttr;

	/** New this instance */
	static FUPyCFunctionWithClosureObject* New(FUPyMethodWithClosureDef* InMethod, PyObject* InSelf, PyObject* InModule);

	/** Free this instance */
	static void Free(FUPyCFunctionWithClosureObject* InSelf);

	/** Handle a Python GC traverse */
	static int GCTraverse(FUPyCFunctionWithClosureObject* InSelf, visitproc InVisit, void* InArg);
	
	/** Handle a Python GC clear */
	static int GCClear(FUPyCFunctionWithClosureObject* InSelf);
	
	/** Get a string representing this instance */
	static PyObject* Str(FUPyCFunctionWithClosureObject* InSelf);
};

/** Python object for the descriptor of a method with a closure (equivalent to PyMethodDescrObject) */
struct FUPyMethodWithClosureDescrObject
{
	/** Common Python Object */
	PyObject_HEAD

	/** Type of the owner object */
	PyTypeObject* OwnerType;

	/** Name of the method being described */
	PyObject* MethodName;

	/* Definition of the method to call */
	FUPyMethodWithClosureDef* MethodDef;

	/** New a method */
	static FUPyMethodWithClosureDescrObject* NewMethod(PyTypeObject* InType, FUPyMethodWithClosureDef* InMethod);

	/** New a class method */
	static FUPyMethodWithClosureDescrObject* NewClassMethod(PyTypeObject* InType, FUPyMethodWithClosureDef* InMethod);

	/** New this instance */
	static FUPyMethodWithClosureDescrObject* NewImpl(PyTypeObject* InDescrType, PyTypeObject* InType, FUPyMethodWithClosureDef* InMethod);

	/** Free this instance */
	static void Free(FUPyMethodWithClosureDescrObject* InSelf);

	/** Handle a Python GC traverse */
	static int GCTraverse(FUPyMethodWithClosureDescrObject* InSelf, visitproc InVisit, void* InArg);

	/** Handle a Python GC clear */
	static int GCClear(FUPyMethodWithClosureDescrObject* InSelf);

	/** Get a string representing this instance */
	static PyObject* Str(FUPyMethodWithClosureDescrObject* InSelf);

	/** Safely get the name of the method (if known) */
	static FString GetMethodName(FUPyMethodWithClosureDescrObject* InSelf);
};
