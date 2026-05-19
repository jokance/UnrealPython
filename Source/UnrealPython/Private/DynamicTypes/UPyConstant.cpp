// Copyright Epic Games, Inc. All Rights Reserved.

#include "DynamicTypes/UPyConstant.h"
#include "Utils/UPyUtil.h"
#include "Core/UPyPtr.h"

void InitializeUPyConstant()
{
	PyType_Ready(&UPyConstantDescrType);
}

bool FUPyConstantDef::AddConstantToType(FUPyConstantDef* InConstant, PyTypeObject* InType)
{
	return AddConstantToDict(InConstant, InType->tp_dict);
}

bool FUPyConstantDef::AddConstantsToType(FUPyConstantDef* InConstants, PyTypeObject* InType)
{
	return AddConstantsToDict(InConstants, InType->tp_dict);
}

bool FUPyConstantDef::AddConstantToModule(FUPyConstantDef* InConstant, PyObject* InModule)
{
	if (!PyModule_Check(InModule))
	{
		return false;
	}

	PyObject* ModuleDict = PyModule_GetDict(InModule);
	return AddConstantToDict(InConstant, ModuleDict);
}

bool FUPyConstantDef::AddConstantsToModule(FUPyConstantDef* InConstants, PyObject* InModule)
{
	if (!PyModule_Check(InModule))
	{
		return false;
	}
	
	PyObject* ModuleDict = PyModule_GetDict(InModule);
	return AddConstantsToDict(InConstants, ModuleDict);
}

bool FUPyConstantDef::AddConstantToDict(FUPyConstantDef* InConstant, PyObject* InDict)
{
	FUPyObjectPtr PyConstantDescr = FUPyObjectPtr::StealReference((PyObject*)FUPyConstantDescrObject::New(InConstant));
	if (!PyConstantDescr)
	{
		return false;
	}

	if (PyDict_SetItemString(InDict, InConstant->ConstantName, PyConstantDescr) != 0)
	{
		return false;
	}

	return true;
}

bool FUPyConstantDef::AddConstantsToDict(FUPyConstantDef* InConstants, PyObject* InDict)
{
	if (!PyDict_Check(InDict))
	{
		return false;
	}

	for (FUPyConstantDef* ConstantDef = InConstants; ConstantDef->ConstantName; ++ConstantDef)
	{
		if (!AddConstantToDict(ConstantDef, InDict))
		{
			return false;
		}
	}

	return true;
}

FUPyConstantDescrObject* FUPyConstantDescrObject::New(const FUPyConstantDef* InConstantDef)
{
	FUPyConstantDescrObject* Self = (FUPyConstantDescrObject*)UPyConstantDescrType.tp_alloc(&UPyConstantDescrType, 0);
	if (Self)
	{
		Self->ConstantName = PyUnicode_FromString(InConstantDef->ConstantName);
		Self->ConstantDef = InConstantDef;
	}
	return Self;
}

void FUPyConstantDescrObject::Free(FUPyConstantDescrObject* InSelf)
{
	Py_XDECREF(InSelf->ConstantName);
	InSelf->ConstantName = nullptr;

	InSelf->ConstantDef = nullptr;

	Py_TYPE(InSelf)->tp_free((PyObject*)InSelf);
}

PyTypeObject InitializeUPyConstantDescrType()
{
	struct FFuncs
	{
		static void Dealloc(FUPyConstantDescrObject* InSelf)
		{
			FUPyConstantDescrObject::Free(InSelf);
		}

		static PyObject* Str(FUPyConstantDescrObject* InSelf)
		{
			return PyUnicode_FromString("<built-in constant value>");
		}

		static PyObject* DescrGet(FUPyConstantDescrObject* InSelf, PyObject* InObj, PyObject* InType)
		{
			return InSelf->ConstantDef->ConstantGetter((PyTypeObject*)InType, InSelf->ConstantDef->ConstantContext);
		}

		static int DescrSet(FUPyConstantDescrObject* InSelf, PyObject* InObj, PyObject* InValue)
		{
			PyErr_SetString(PyExc_Exception, "Constant values are read-only");
			return -1;
		}
	};

	struct FGetSets
	{
		static PyObject* GetDoc(FUPyConstantDescrObject* InSelf, void* InClosure)
		{
			if (InSelf->ConstantDef->ConstantDoc)
			{
				return PyUnicode_FromString(InSelf->ConstantDef->ConstantDoc);
			}

			Py_RETURN_NONE;
		}
	};

	static PyMemberDef PyMembers[] = {
		{ UPyCStrCast("__name__"), T_OBJECT, STRUCT_OFFSET(FUPyConstantDescrObject, ConstantName), READONLY, nullptr },
		{ nullptr, 0, 0, 0, nullptr }
	};

	static PyGetSetDef PyGetSets[] = {
		{ UPyCStrCast("__doc__"), (getter)&FGetSets::GetDoc, nullptr, nullptr, nullptr },
		{ nullptr, nullptr, nullptr, nullptr, nullptr }
	};

	PyTypeObject PyType = {
		PyVarObject_HEAD_INIT(nullptr, 0)
		"constant_value", /* tp_name */
		sizeof(FUPyConstantDescrObject), /* tp_basicsize */
	};

	PyType.tp_dealloc = (destructor)&FFuncs::Dealloc;
	PyType.tp_str = (reprfunc)&FFuncs::Str;
	PyType.tp_repr = (reprfunc)&FFuncs::Str;
	PyType.tp_descr_get = (descrgetfunc)&FFuncs::DescrGet;
	PyType.tp_descr_set = (descrsetfunc)&FFuncs::DescrSet;
	PyType.tp_getattro = (getattrofunc)&PyObject_GenericGetAttr;

	PyType.tp_members = PyMembers;
	PyType.tp_getset = PyGetSets;

	PyType.tp_flags = Py_TPFLAGS_DEFAULT;

	return PyType;
}

PyTypeObject UPyConstantDescrType = InitializeUPyConstantDescrType();
