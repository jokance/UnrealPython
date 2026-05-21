
#include "UPyCommon.h"
#include "Utils/UPyUtil.h"
#include "Wrapper/UPyWrapperSet.h"
#include "Wrapper/UPyWrapperTypeFactory.h"


// This is for type hinting, to make the class appears as a generic (https://peps.python.org/pep-0560/) and support hinting like 's: unreal.Set[int] = unreal.Set(int)'

struct FMethods_WrapperSet
{
	static PyObject* GetClassItem(PyObject* Type, PyObject* Item)
	{
		Py_INCREF(Type);
		return Type;
	}

	static PyObject* Cast(PyTypeObject* InType, PyObject* InArgs, PyObject* InKwds)
	{
		PyObject* PyTypeObj = nullptr;
		PyObject* PyObj = nullptr;

		static const char *ArgsKwdList[] = { "type", "obj", nullptr };
		if (PyArg_ParseTupleAndKeywords(InArgs, InKwds, "OO:call", (char**)ArgsKwdList, &PyTypeObj, &PyObj))
		{
			UPyUtil::FPropertyDef SetElementDef;
			if (UPyUtil::ValidateContainerTypeParam(PyTypeObj, SetElementDef, "type", *UPyUtil::GetErrorContext(InType)) != 0)
			{
				return nullptr;
			}

			PyObject* PyCastResult = (PyObject*)FUPyWrapperSet::CastPyObject(PyObj, InType, SetElementDef);
			if (!PyCastResult)
			{
				UPyUtil::SetPythonError(PyExc_TypeError, InType, *FString::Printf(TEXT("Cannot cast type '%s' to '%s'"), *UPyUtil::GetFriendlyTypename(PyObj), *UPyUtil::GetFriendlyTypename(InType)));
			}
			return PyCastResult;
		}

		return nullptr;
	}

	static PyObject* Copy(FUPyWrapperSet* InSelf)
	{
		if (!FUPyWrapperSet::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		return (PyObject*)FUPyWrapperSetFactory::Get().CreateInstance(InSelf->SetInstance, InSelf->SetProp, FUPyWrapperOwnerContext(), EUPyConversionMethod::Copy);
	}

	static PyObject* Add(FUPyWrapperSet* InSelf, PyObject* InArgs)
	{
		PyObject* PyObj = nullptr;
		if (!PyArg_ParseTuple(InArgs, "O:add", &PyObj))
		{
			return nullptr;
		}

		if (FUPyWrapperSet::Add(InSelf, PyObj) != 0)
		{
			return nullptr;
		}

		Py_RETURN_NONE;
	}

	static PyObject* Discard(FUPyWrapperSet* InSelf, PyObject* InArgs)
	{
		PyObject* PyObj = nullptr;
		if (!PyArg_ParseTuple(InArgs, "O:discard", &PyObj))
		{
			return nullptr;
		}

		if (FUPyWrapperSet::Discard(InSelf, PyObj) == -1)
		{
			return nullptr;
		}

		Py_RETURN_NONE;
	}

	static PyObject* Remove(FUPyWrapperSet* InSelf, PyObject* InArgs)
	{
		PyObject* PyObj = nullptr;
		if (!PyArg_ParseTuple(InArgs, "O:remove", &PyObj))
		{
			return nullptr;
		}

		if (FUPyWrapperSet::Remove(InSelf, PyObj) != 0)
		{
			return nullptr;
		}

		Py_RETURN_NONE;
	}

	static PyObject* Pop(FUPyWrapperSet* InSelf)
	{
		return FUPyWrapperSet::Pop(InSelf);
	}

	static PyObject* Clear(FUPyWrapperSet* InSelf)
	{
		if (FUPyWrapperSet::Clear(InSelf) != 0)
		{
			return nullptr;
		}

		Py_RETURN_NONE;
	}

	static PyObject* Difference(FUPyWrapperSet* InSelf, PyObject* InArgs)
	{
		check(PyTuple_Check(InArgs));
		return (PyObject*)FUPyWrapperSet::Difference(InSelf, InArgs);
	}

	static PyObject* DifferenceUpdate(FUPyWrapperSet* InSelf, PyObject* InArgs)
	{
		check(PyTuple_Check(InArgs));

		if (FUPyWrapperSet::DifferenceUpdate(InSelf, InArgs) != 0)
		{
			return nullptr;
		}

		Py_RETURN_NONE;
	}

	static PyObject* Intersection(FUPyWrapperSet* InSelf, PyObject* InArgs)
	{
		check(PyTuple_Check(InArgs));
		return (PyObject*)FUPyWrapperSet::Intersection(InSelf, InArgs);
	}

	static PyObject* IntersectionUpdate(FUPyWrapperSet* InSelf, PyObject* InArgs)
	{
		check(PyTuple_Check(InArgs));

		if (FUPyWrapperSet::IntersectionUpdate(InSelf, InArgs) != 0)
		{
			return nullptr;
		}

		Py_RETURN_NONE;
	}

	static PyObject* SymmetricDifference(FUPyWrapperSet* InSelf, PyObject* InArgs)
	{
		PyObject* PyObj = nullptr;
		if (!PyArg_ParseTuple(InArgs, "O:symmetric_difference", &PyObj))
		{
			return nullptr;
		}

		return (PyObject*)FUPyWrapperSet::SymmetricDifference(InSelf, PyObj);
	}

	static PyObject* SymmetricDifferenceUpdate(FUPyWrapperSet* InSelf, PyObject* InArgs)
	{
		PyObject* PyObj = nullptr;
		if (!PyArg_ParseTuple(InArgs, "O:symmetric_difference_update", &PyObj))
		{
			return nullptr;
		}

		if (FUPyWrapperSet::SymmetricDifferenceUpdate(InSelf, PyObj) != 0)
		{
			return nullptr;
		}

		Py_RETURN_NONE;
	}

	static PyObject* Union(FUPyWrapperSet* InSelf, PyObject* InArgs)
	{
		check(PyTuple_Check(InArgs));
		return (PyObject*)FUPyWrapperSet::Union(InSelf, InArgs);
	}

	static PyObject* Update(FUPyWrapperSet* InSelf, PyObject* InArgs)
	{
		check(PyTuple_Check(InArgs));

		if (FUPyWrapperSet::Update(InSelf, InArgs) != 0)
		{
			return nullptr;
		}

		Py_RETURN_NONE;
	}

	static PyObject* IsDisjoint(FUPyWrapperSet* InSelf, PyObject* InArgs)
	{
		PyObject* PyObj = nullptr;
		if (!PyArg_ParseTuple(InArgs, "O:isdisjoint", &PyObj))
		{
			return nullptr;
		}

		const int IsDisjointResult = FUPyWrapperSet::IsDisjoint(InSelf, PyObj);
		if (IsDisjointResult == -1)
		{
			return nullptr;
		}

		if (IsDisjointResult == 1)
		{
			Py_RETURN_TRUE;
		}

		Py_RETURN_FALSE;
	}

	static PyObject* IsSubset(FUPyWrapperSet* InSelf, PyObject* InArgs)
	{
		PyObject* PyObj = nullptr;
		if (!PyArg_ParseTuple(InArgs, "O:issubset", &PyObj))
		{
			return nullptr;
		}

		const int IsSubsetResult = FUPyWrapperSet::IsSubset(InSelf, PyObj);
		if (IsSubsetResult == -1)
		{
			return nullptr;
		}

		if (IsSubsetResult == 1)
		{
			Py_RETURN_TRUE;
		}

		Py_RETURN_FALSE;
	}

	static PyObject* IsSuperset(FUPyWrapperSet* InSelf, PyObject* InArgs)
	{
		PyObject* PyObj = nullptr;
		if (!PyArg_ParseTuple(InArgs, "O:issuperset", &PyObj))
		{
			return nullptr;
		}

		const int IsSupersetResult = FUPyWrapperSet::IsSuperset(InSelf, PyObj);
		if (IsSupersetResult == -1)
		{
			return nullptr;
		}

		if (IsSupersetResult == 1)
		{
			Py_RETURN_TRUE;
		}

		Py_RETURN_FALSE;
	}
};


// NOTE: _ElemType = TypeVar('_ElemType') and Type/Iterable are defined by the Python typing module.
PyMethodDef SetPyMethodDefs[] = {
	{ "__class_getitem__", UPyCFunctionCast(FMethods_WrapperSet::GetClassItem), METH_O|METH_CLASS, UPyDoc_STR("__class_getitem__(cls, item: _ElemType) -- implemented for type hinting purpose only") },
	{ "Cast", UPyCFunctionCast(&FMethods_WrapperSet::Cast), METH_VARARGS | METH_KEYWORDS | METH_CLASS, UPyDoc_STR("Cast(cls, type: Type[_ElemType], obj: object) -> Set[_ElemType] -- cast the given object to this Unreal set type") },
	{ "__copy__", UPyCFunctionCast(&FMethods_WrapperSet::Copy), METH_NOARGS, UPyDoc_STR("__copy__(self) -> Set[_ElemType] -- copy this Unreal set") },
	{ "Copy", UPyCFunctionCast(&FMethods_WrapperSet::Copy), METH_NOARGS, UPyDoc_STR("Copy(self) -> Set[_ElemType] -- copy this Unreal set") },
	{ "Add", UPyCFunctionCast(&FMethods_WrapperSet::Add), METH_VARARGS, UPyDoc_STR("Add(self, value: _ElemType) -> None -- add the given value to this Unreal set if not already present") },
	{ "Discard", UPyCFunctionCast(&FMethods_WrapperSet::Discard), METH_VARARGS, UPyDoc_STR("Discard(self, value: _ElemType) -> None -- remove the given value from this Unreal set, or do nothing if not present") },
	{ "Remove", UPyCFunctionCast(&FMethods_WrapperSet::Remove), METH_VARARGS, UPyDoc_STR("Remove(self, value: _ElemType) -> None -- remove the given value from this Unreal set, or raise KeyError if not present") },
	{ "Pop", UPyCFunctionCast(&FMethods_WrapperSet::Pop), METH_NOARGS, UPyDoc_STR("Pop(self) -> _ElemType -- remove and return an arbitrary value from this Unreal set, or raise KeyError if the set is empty") },
	{ "Clear", UPyCFunctionCast(&FMethods_WrapperSet::Clear), METH_NOARGS, UPyDoc_STR("Clear(self) -> None -- remove all values from this Unreal set") },
	{ "Difference", UPyCFunctionCast(&FMethods_WrapperSet::Difference), METH_VARARGS, UPyDoc_STR("Difference(self, *iterable: Iterable[_ElemType]) -> Set[_ElemType] -- return the difference between this Unreal set and the other iterables passed for comparison (ie, all values that are in this set but not the others)") },
	{ "DifferenceUpdate", UPyCFunctionCast(&FMethods_WrapperSet::DifferenceUpdate), METH_VARARGS, UPyDoc_STR("DifferenceUpdate(self, *iterables: Iterable[_ElemType]) -> None -- make this set the difference between this Unreal set and the other iterables passed for comparison (ie, all values that are in this set but not the others)") },
	{ "Intersection", UPyCFunctionCast(&FMethods_WrapperSet::Intersection), METH_VARARGS, UPyDoc_STR("Intersection(self, *iterables: Iterable[_ElemType]) -> Set[_ElemType] -- return the intersection between this Unreal set and the other iterables passed for comparison (ie, values that are common to all of the sets)") },
	{ "IntersectionUpdate", UPyCFunctionCast(&FMethods_WrapperSet::IntersectionUpdate), METH_VARARGS, UPyDoc_STR("IntersectionUpdate(self, *iterables: Iterable[_ElemType]) -> None -- make this set the intersection between this Unreal set and the other iterables passed for comparison (ie, values that are common to all of the sets)") },
	{ "SymmetricDifference", UPyCFunctionCast(&FMethods_WrapperSet::SymmetricDifference), METH_VARARGS, UPyDoc_STR("SymmetricDifference(self, other: Iterable[_ElemType]) -> Set[_ElemType] -- return the symmetric difference between this Unreal set and another (ie, values that are in exactly one of the sets)") },
	{ "SymmetricDifferenceUpdate", UPyCFunctionCast(&FMethods_WrapperSet::SymmetricDifferenceUpdate), METH_VARARGS, UPyDoc_STR("SymmetricDifferenceUpdate(self, other: Iterable[_ElemType]) -> None -- make this set the symmetric difference between this Unreal set and another (ie, values that are in exactly one of the sets)") },
	{ "Union", UPyCFunctionCast(&FMethods_WrapperSet::Union), METH_VARARGS, UPyDoc_STR("Union(self, *iterables: Iterable[_ElemType]) -> Set[_ElemType] -- return the union between this Unreal set and the other iterables passed for comparison (ie, values that are in any set)") },
	{ "Update", UPyCFunctionCast(&FMethods_WrapperSet::Update), METH_VARARGS, UPyDoc_STR("Update(self, *iterables: Iterable[_ElemType]) -> None -- make this set the union between this Unreal set and the other iterables passed for comparison (ie, values that are in any set)") },
	{ "IsDisjoint", UPyCFunctionCast(&FMethods_WrapperSet::IsDisjoint), METH_VARARGS, UPyDoc_STR("IsDisjoint(self, other: Iterable[_ElemType]) -> bool -- return True if there is a null intersection between this Unreal set and another") },
	{ "IsSubset", UPyCFunctionCast(&FMethods_WrapperSet::IsSubset), METH_VARARGS, UPyDoc_STR("IsSubset(self, other: Iterable[_ElemType]) -> bool -- return True if another set contains this Unreal set") },
	{ "IsSuperset", UPyCFunctionCast(&FMethods_WrapperSet::IsSuperset), METH_VARARGS, UPyDoc_STR("IsSuperset(self, other: Iterable[_ElemType]) -> bool -- return True if this Unreal set contains another") },
	{ nullptr, nullptr, 0, nullptr }
};
