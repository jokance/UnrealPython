// Copyright Epic Games, Inc. All Rights Reserved.

#include "UPyCommon.h"
#include "UPyManager.h"
#include "Wrapper/UPyWrapperObjectBase.h"
#include "Utils/UPyUtil.h"
#include "Core/UPyConversion.h"
#include "Core/UPyPtr.h"
#include "Wrapper/UPyWrapperTypeRegistry.h"

struct FMethods_WrapperObjectBase
{
	static PyObject* PostInit(FUPyWrapperObjectBase* InSelf)
	{
		Py_RETURN_NONE;
	}

	static PyObject* CallIsValid(FUPyWrapperObjectBase* InSelf)
	{
		return UPyConversion::Pythonize(FUPyWrapperObjectBase::ValidateInternalState(InSelf));
	}

	static PyObject* Cast(PyTypeObject* InType, PyObject* InArgs)
	{
		PyObject* PyObj = nullptr;
		if (PyArg_ParseTuple(InArgs, "O:cast", &PyObj))
		{
			PyObject* PyCastResult = (PyObject*)FUPyWrapperObjectBase::CastPyObject(PyObj, InType);
			if (!PyCastResult)
			{
				UPyUtil::SetPythonError(PyExc_TypeError, InType, *FString::Printf(TEXT("Cannot cast type '%s' to '%s'"), *UPyUtil::GetFriendlyTypename(PyObj), *UPyUtil::GetFriendlyTypename(InType)));
			}
			return PyCastResult;
		}

		return nullptr;
	}

	static PyObject* IsA(FUPyWrapperObjectBase* InSelf, PyObject* InArgs)
	{
		if (!FUPyWrapperObjectBase::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		PyObject* PyClass = nullptr;
		if (!PyArg_ParseTuple(InArgs, "O:IsA", &PyClass))
		{
			return nullptr;
		}

		UClass* Class = nullptr;
		if (!UPyConversion::NativizeClass(PyClass, Class, UObject::StaticClass()))
		{
			return nullptr;
		}

		const bool bResult = InSelf->ObjectInstance->IsA(Class);
		return UPyConversion::Pythonize(bResult);
	}

	static PyObject* IsChildOf(FUPyWrapperObjectBase* InSelf, PyObject* InArgs)
	{
		if (!FUPyWrapperObjectBase::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		PyObject* PyClass = nullptr;
		if (!PyArg_ParseTuple(InArgs, "O:IsChildOf", &PyClass))
		{
			return nullptr;
		}

		UClass* Class = nullptr;
		if (!UPyConversion::NativizeClass(PyClass, Class, UObject::StaticClass()))
		{
			return nullptr;
		}

		UStruct* Struct = ::Cast<UStruct>(InSelf->ObjectInstance);
		if (!Struct)
		{
			UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Instance '%s' is not a Struct/Class"), *InSelf->ObjectInstance->GetName()));
			return nullptr;
		}

		const bool bResult = Struct->IsChildOf(Class);
		return UPyConversion::Pythonize(bResult);
	}

	static PyObject* GetDefaultObject(PyTypeObject* InType)
	{
		const UClass* Class = FUPyWrapperTypeRegistry::Get().FindClass(InType);
		UObject* CDO = Class ? Class->GetDefaultObject() : nullptr;
		return UPyConversion::Pythonize(CDO);
	}

	static PyObject* StaticClass(PyTypeObject* InType)
	{
		UClass* Class = (UClass*)FUPyWrapperTypeRegistry::Get().FindClass(InType);
		return UPyConversion::PythonizeClass(Class);
	}


	static PyObject* GetClass(FUPyWrapperObjectBase* InSelf)
	{
		if (!FUPyWrapperObjectBase::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		return UPyConversion::PythonizeClass(InSelf->ObjectInstance->GetClass());
	}

	static PyObject* GetOuter(FUPyWrapperObjectBase* InSelf)
	{
		if (!FUPyWrapperObjectBase::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		return UPyConversion::Pythonize(InSelf->ObjectInstance->GetOuter());
	}

	static PyObject* GetTypedOuter(FUPyWrapperObjectBase* InSelf, PyObject* InArgs)
	{
		if (!FUPyWrapperObjectBase::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		PyObject* PyOuterType = nullptr;
		if (!PyArg_ParseTuple(InArgs, "O:get_typed_outer", &PyOuterType))
		{
			return nullptr;
		}

		UClass* OuterType = nullptr;
		if (!UPyConversion::NativizeClass(PyOuterType, OuterType, UObject::StaticClass()))
		{
			return nullptr;
		}

		return UPyConversion::Pythonize(InSelf->ObjectInstance->GetTypedOuter(OuterType));
	}

	static PyObject* GetOutermost(FUPyWrapperObjectBase* InSelf)
	{
		if (!FUPyWrapperObjectBase::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		return UPyConversion::Pythonize(InSelf->ObjectInstance->GetOutermost());
	}

	static PyObject* IsPackageExternal(FUPyWrapperObjectBase* InSelf)
	{
		if (!FUPyWrapperObjectBase::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		const bool bIsPackageExternal = InSelf->ObjectInstance->IsPackageExternal();
		return UPyConversion::Pythonize(bIsPackageExternal);
	}

	static PyObject* GetPackage(FUPyWrapperObjectBase* InSelf)
	{
		if (!FUPyWrapperObjectBase::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		return UPyConversion::Pythonize(InSelf->ObjectInstance->GetPackage());
	}

	static PyObject* GetName(FUPyWrapperObjectBase* InSelf)
	{
		if (!FUPyWrapperObjectBase::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		return UPyConversion::Pythonize(InSelf->ObjectInstance->GetName());
	}

	static PyObject* GetFName(FUPyWrapperObjectBase* InSelf)
	{
		if (!FUPyWrapperObjectBase::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		return UPyConversion::Pythonize(InSelf->ObjectInstance->GetFName());
	}

	static PyObject* GetFullName(FUPyWrapperObjectBase* InSelf)
	{
		if (!FUPyWrapperObjectBase::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		return UPyConversion::Pythonize(InSelf->ObjectInstance->GetFullName());
	}

	static PyObject* GetPathName(FUPyWrapperObjectBase* InSelf)
	{
		if (!FUPyWrapperObjectBase::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		return UPyConversion::Pythonize(InSelf->ObjectInstance->GetPathName());
	}

	static PyObject* GetWorld(FUPyWrapperObjectBase* InSelf)
	{
		if (!FUPyWrapperObjectBase::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		return UPyConversion::Pythonize(InSelf->ObjectInstance->GetWorld());
	}

	static PyObject* Modify(FUPyWrapperObjectBase* InSelf, PyObject* InArgs)
	{
		if (!FUPyWrapperObjectBase::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		PyObject* PyAlwaysMarkDirty = nullptr;
		if (!PyArg_ParseTuple(InArgs, "|O:modify", &PyAlwaysMarkDirty))
		{
			return nullptr;
		}

		bool bAlwaysMarkDirty = true;
		if (PyAlwaysMarkDirty && !UPyConversion::Nativize(PyAlwaysMarkDirty, bAlwaysMarkDirty))
		{
			return nullptr;
		}

		const bool bResult = InSelf->ObjectInstance->Modify(bAlwaysMarkDirty);
		return UPyConversion::Pythonize(bResult);
	}

	static PyObject* Rename(FUPyWrapperObjectBase* InSelf, PyObject* InArgs, PyObject* InKwds)
	{
		if (!FUPyWrapperObjectBase::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		PyObject* PyNameObj = nullptr;
		PyObject* PyOuterObj = nullptr;

		static const char *ArgsKwdList[] = { "name", "outer", nullptr };
		if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "|OO:rename", (char**)ArgsKwdList, &PyNameObj, &PyOuterObj))
		{
			return nullptr;
		}

		FName NewName;
		if (PyNameObj && PyNameObj != Py_None && !UPyConversion::Nativize(PyNameObj, NewName))
		{
			UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'name' (%s) to 'Name'"), *UPyUtil::GetFriendlyTypename(PyNameObj)));
			return nullptr;
		}

		UObject* NewOuter = nullptr;
		if (PyOuterObj && !UPyConversion::Nativize(PyOuterObj, NewOuter))
		{
			UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'outer' (%s) to 'Object'"), *UPyUtil::GetFriendlyTypename(PyOuterObj)));
			return nullptr;
		}

		const bool bResult = InSelf->ObjectInstance->Rename(NewName.IsNone() ? nullptr : *NewName.ToString(), NewOuter);

		return UPyConversion::Pythonize(bResult);
	}

	static UPyGenUtil::FGeneratedWrappedProperty FindEditorPropertyImpl(FUPyWrapperObjectBase* InSelf, const FName InPythonPropName)
	{
		UPyGenUtil::FGeneratedWrappedProperty WrappedPropDef;

		const UClass* Class = InSelf->ObjectInstance->GetClass();
		const FName ResolvedName = InPythonPropName; // todo(hzn): metadata: FPyWrapperObjectMetaData::ResolvePropertyName(InSelf, InPythonPropName);

		const FProperty* ResolvedProp = Class->FindPropertyByName(ResolvedName);
		if (InSelf->ObjectInstance->HasAnyFlags(RF_ClassDefaultObject) && (!ResolvedProp || ResolvedProp->HasAnyPropertyFlags(CPF_Deprecated)))
		{
			// Look for a sparse member of the same name - the sparse data is treated as an extension of the class default object by the details panel
			if (const UStruct* SparseDataStruct = Class->GetSparseClassDataStruct())
			{
				if (const FProperty* SparseProp = SparseDataStruct->FindPropertyByName(ResolvedName))
				{
					ResolvedProp = SparseProp;
				}
			}
		}

		if (!ResolvedProp)
		{
			UPyUtil::SetPythonError(PyExc_Exception, InSelf, *FString::Printf(TEXT("Failed to find property '%s' for attribute '%s' on '%s'"), *ResolvedName.ToString(), *InPythonPropName.ToString(), *Class->GetName()));
			return WrappedPropDef;
		}

		// If the owner class is set then this property is from 'self', otherwise it's from the sparse class data
		const bool bPropertyIsOwnedBySelf = ResolvedProp->GetOwnerClass() != nullptr;

		TOptional<FString> PropDeprecationMessage;
		// todo(hzn): metadata
		// if (bPropertyIsOwnedBySelf)
		// {
		// 	FString PropDeprecationMessageStr;
		// 	if (FPyWrapperObjectMetaData::IsPropertyDeprecated(InSelf, InPythonPropName, &PropDeprecationMessageStr))
		// 	{
		// 		PropDeprecationMessage = MoveTemp(PropDeprecationMessageStr);
		// 	}
		// }

		if (PropDeprecationMessage.IsSet())
		{
			WrappedPropDef.SetProperty(ResolvedProp, UPyGenUtil::FGeneratedWrappedProperty::SPF_None);
			WrappedPropDef.DeprecationMessage = MoveTemp(PropDeprecationMessage);
		}
		else
		{
			WrappedPropDef.SetProperty(ResolvedProp);
		}

		return WrappedPropDef;
	}

#if WITH_EDITOR
	static PyObject* GetEditorProperty(FUPyWrapperObjectBase* InSelf, PyObject* InArgs, PyObject* InKwds)
	{
		if (!FUPyWrapperObjectBase::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		PyObject* PyNameObj = nullptr;

		static const char *ArgsKwdList[] = { "name", nullptr };
		if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "O:get_editor_property", (char**)ArgsKwdList, &PyNameObj))
		{
			return nullptr;
		}

		FName Name;
		if (!UPyConversion::Nativize(PyNameObj, Name))
		{
			UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'name' (%s) to 'Name'"), *UPyUtil::GetFriendlyTypename(PyNameObj)));
			return nullptr;
		}

		const UPyGenUtil::FGeneratedWrappedProperty WrappedPropDef = FindEditorPropertyImpl(InSelf, Name);
		if (!WrappedPropDef.Prop)
		{
			return nullptr;
		}

		// If the owner class is set then this property is from 'self', otherwise it's from the sparse class data
		const bool bPropertyIsOwnedBySelf = WrappedPropDef.Prop->GetOwnerClass() != nullptr;
		UClass* Class = InSelf->ObjectInstance->GetClass();
		if (bPropertyIsOwnedBySelf)
		{
			return UPyGenUtil::GetPropertyValue(Class, InSelf->ObjectInstance, WrappedPropDef, TCHAR_TO_UTF8(*Name.ToString()), (PyObject*)InSelf, *UPyUtil::GetErrorContext(InSelf));
		}
		else
		{
			const UScriptStruct* SparseDataStruct = Class->GetSparseClassDataStruct();
			const void* SparseData = Class->GetSparseClassData(EGetSparseClassDataMethod::ArchetypeIfNull);
			if (SparseDataStruct && SparseData)
			{
				return UPyGenUtil::GetPropertyValue(SparseDataStruct, SparseData, WrappedPropDef, TCHAR_TO_UTF8(*Name.ToString()), (PyObject*)InSelf, *UPyUtil::GetErrorContext(InSelf));
			}
			else
			{
				UPyUtil::SetPythonError(PyExc_Exception, InSelf, *FString::Printf(TEXT("Failed to get sparse data for '%s'"), *Class->GetName()));
				return nullptr;
			}
		}
	}

	static PyObject* SetEditorProperty(FUPyWrapperObjectBase* InSelf, PyObject* InArgs, PyObject* InKwds)
	{
		if (!FUPyWrapperObjectBase::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		PyObject* PyNameObj = nullptr;
		PyObject* PyValueObj = nullptr;
		PyObject* PyNotifyModeObj = nullptr;

		static const char *ArgsKwdList[] = { "name", "value", "notify_mode", nullptr };
		if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "OO|O:set_editor_property", (char**)ArgsKwdList, &PyNameObj, &PyValueObj, &PyNotifyModeObj))
		{
			return nullptr;
		}

		FName Name;
		if (!UPyConversion::Nativize(PyNameObj, Name))
		{
			UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'name' (%s) to 'Name'"), *UPyUtil::GetFriendlyTypename(PyNameObj)));
			return nullptr;
		}

		static const UEnum* PropertyAccessChangeNotifyModeEnum = FindObjectChecked<UEnum>(nullptr, TEXT("/Script/CoreUObject.EPropertyAccessChangeNotifyMode"));

		EPropertyAccessChangeNotifyMode NotifyMode = EPropertyAccessChangeNotifyMode::Default;
		if (PyNotifyModeObj && !UPyConversion::NativizeEnumEntry(PyNotifyModeObj, PropertyAccessChangeNotifyModeEnum, NotifyMode))
		{
			UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'notify_mode' (%s) to 'PropertyAccessChangeNotifyMode'"), *UPyUtil::GetFriendlyTypename(PyNotifyModeObj)));
			return nullptr;
		}

		const UPyGenUtil::FGeneratedWrappedProperty WrappedPropDef = FindEditorPropertyImpl(InSelf, Name);
		if (!WrappedPropDef.Prop)
		{
			return nullptr;
		}

		// If the object is a template, gather instances (including subclass CDOs) inheriting their property value from this template.
		// Those instances are passed into UPyGenUtil::SetPropertyValue so that they will receive the value change as well.
		TArray<void*> ArchetypeInsts;
		const bool bIsObjectTemplate = PropertyAccessUtil::IsObjectTemplate(InSelf->ObjectInstance);
		if (bIsObjectTemplate)
		{
			PropertyAccessUtil::GetArchetypeInstancesInheritingPropertyValue_AsContainerData(WrappedPropDef.Prop, InSelf->ObjectInstance, ArchetypeInsts);
		}

		// If the owner class is set then this property is from 'self', otherwise it's from the sparse class data
		const bool bPropertyIsOwnedBySelf = WrappedPropDef.Prop->GetOwnerClass() != nullptr;
		UClass* Class = InSelf->ObjectInstance->GetClass();
		if (bPropertyIsOwnedBySelf)
		{
			const TUniquePtr<FPropertyAccessChangeNotify> ChangeNotify = FUPyWrapperOwnerContext((PyObject*)InSelf, WrappedPropDef.Prop).BuildChangeNotify(NotifyMode);
			const int Result = UPyGenUtil::SetPropertyValue(Class, InSelf->ObjectInstance, PyValueObj, WrappedPropDef, TCHAR_TO_UTF8(*Name.ToString()), ChangeNotify.Get(), PropertyAccessUtil::EditorReadOnlyFlags, bIsObjectTemplate, *UPyUtil::GetErrorContext(InSelf), ArchetypeInsts);
			if (Result != 0)
			{
				return nullptr;
			}
		}
		else
		{
			const UScriptStruct* SparseDataStruct = Class->GetSparseClassDataStruct();
			void* SparseData = Class->GetOrCreateSparseClassData();
			if (SparseDataStruct && SparseData)
			{
				const TUniquePtr<FPropertyAccessChangeNotify> ChangeNotify = FUPyWrapperOwnerContext((PyObject*)InSelf, WrappedPropDef.Prop).BuildChangeNotify(NotifyMode);
				const int Result = UPyGenUtil::SetPropertyValue(SparseDataStruct, SparseData, PyValueObj, WrappedPropDef, TCHAR_TO_UTF8(*Name.ToString()), ChangeNotify.Get(), PropertyAccessUtil::EditorReadOnlyFlags, bIsObjectTemplate, *UPyUtil::GetErrorContext(InSelf), ArchetypeInsts);
				if (Result != 0)
				{
					return nullptr;
				}
			}
			else
			{
				UPyUtil::SetPythonError(PyExc_Exception, InSelf, *FString::Printf(TEXT("Failed to get sparse data for '%s'"), *Class->GetName()));
				return nullptr;
			}
		}

		Py_RETURN_NONE;
	}

	static PyObject* SetEditorProperties(FUPyWrapperObjectBase* InSelf, PyObject* InArgs, PyObject* InKwds)
	{
		if (!FUPyWrapperObjectBase::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		PyObject* PyPropertyInfoDict = nullptr;
		if (!PyArg_ParseTuple(InArgs, "O:set_editor_properties", &PyPropertyInfoDict))
		{
			return nullptr;
		}

		if (!UPyUtil::IsMappingType(PyPropertyInfoDict))
		{
			UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("'property_info' (%s) is expected to be a mapping type (eg, dict)"), *UPyUtil::GetFriendlyTypename(PyPropertyInfoDict)));
			return nullptr;
		}

		// Build up the list of properties and their new values
		typedef TTuple<FName, UPyGenUtil::FGeneratedWrappedProperty, FUPyObjectPtr> FPropertyInfoPair;
		TArray<FPropertyInfoPair> PropertyInfos;
		{
			FUPyObjectPtr PyPropertyInfoDictIter = FUPyObjectPtr::StealReference(PyObject_GetIter(PyPropertyInfoDict));
			if (PyPropertyInfoDictIter)
			{
				FUPyObjectPtr PyKeyItem;
				while ((PyKeyItem = FUPyObjectPtr::StealReference(PyIter_Next(PyPropertyInfoDictIter))))
				{
					FName Name;
					if (!UPyConversion::Nativize(PyKeyItem, Name))
					{
						UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert dict key (%s) to 'Name'"), *UPyUtil::GetFriendlyTypename(PyKeyItem)));
						return nullptr;
					}

					UPyGenUtil::FGeneratedWrappedProperty WrappedPropDef = FindEditorPropertyImpl(InSelf, Name);
					if (!WrappedPropDef.Prop)
					{
						return nullptr;
					}

					FUPyObjectPtr PyValueItem = FUPyObjectPtr::StealReference(PyObject_GetItem(PyPropertyInfoDict, PyKeyItem));
					if (ensure(PyValueItem))
					{
						PropertyInfos.Add(MakeTuple(Name, MoveTemp(WrappedPropDef), MoveTemp(PyValueItem)));
					}
				}

				// Iteration error?
				if (PyErr_Occurred())
				{
					return nullptr;
				}
			}
		}

		// At this point we know that every property we were asked to set was found, but not that the values are going to be compatible
		// Checking value coercion is tricky without actually doing the conversion, so we'll assume that this point that we need to do the change notifies
		InSelf->ObjectInstance->PreEditChange(nullptr);
		ON_SCOPE_EXIT
		{
			InSelf->ObjectInstance->PostEditChange();
		};

		// Try and set the value of each property
		UClass* Class = InSelf->ObjectInstance->GetClass();
		const bool bIsObjectTemplate = PropertyAccessUtil::IsObjectTemplate(InSelf->ObjectInstance);
		for (const FPropertyInfoPair& PropertyInfo : PropertyInfos)
		{
			const FName Name = PropertyInfo.Get<0>();
			const UPyGenUtil::FGeneratedWrappedProperty& WrappedPropDef = PropertyInfo.Get<1>();
			PyObject* PyValueObj = PropertyInfo.Get<2>().GetPtr();

			// If the object is a template, gather instances (including subclass CDOs) inheriting their property value from this template.
			// Those instances are passed into UPyGenUtil::SetPropertyValue so that they will receive the value change as well.
			// Each property can have a different set of archetype instances that are inheriting the value.
			TArray<void*> ArchetypeInsts;
			if (bIsObjectTemplate)
			{
				PropertyAccessUtil::GetArchetypeInstancesInheritingPropertyValue_AsContainerData(WrappedPropDef.Prop, InSelf->ObjectInstance, ArchetypeInsts);
			}

			// If the owner class is set then this property is from 'self', otherwise it's from the sparse class data
			const bool bPropertyIsOwnedBySelf = WrappedPropDef.Prop->GetOwnerClass() != nullptr;
			if (bPropertyIsOwnedBySelf)
			{
				const int Result = UPyGenUtil::SetPropertyValue(Class, InSelf->ObjectInstance, PyValueObj, WrappedPropDef, TCHAR_TO_UTF8(*Name.ToString()), nullptr, PropertyAccessUtil::EditorReadOnlyFlags, bIsObjectTemplate, *UPyUtil::GetErrorContext(InSelf), ArchetypeInsts);
				if (Result != 0)
				{
					return nullptr;
				}
			}
			else
			{
				const UScriptStruct* SparseDataStruct = Class->GetSparseClassDataStruct();
				void* SparseData = Class->GetOrCreateSparseClassData();
				if (SparseDataStruct && SparseData)
				{
					const int Result = UPyGenUtil::SetPropertyValue(SparseDataStruct, SparseData, PyValueObj, WrappedPropDef, TCHAR_TO_UTF8(*Name.ToString()), nullptr, PropertyAccessUtil::EditorReadOnlyFlags, bIsObjectTemplate, *UPyUtil::GetErrorContext(InSelf), ArchetypeInsts);
					if (Result != 0)
					{
						return nullptr;
					}
				}
				else
				{
					UPyUtil::SetPythonError(PyExc_Exception, InSelf, *FString::Printf(TEXT("Failed to get sparse data for '%s'"), *Class->GetName()));
					return nullptr;
				}
			}
		}

		Py_RETURN_NONE;
	}
#endif // WITH_EDITOR

	static PyObject* AddPythonOwned(FUPyWrapperObjectBase* InSelf)
	{
		UUPyManager::Get()->AddPythonOwnedObject(InSelf);
		Py_RETURN_NONE;
	}

	static PyObject* RemovePythonOwned(FUPyWrapperObjectBase* InSelf)
	{
		UUPyManager::Get()->RemovePythonOwnedObject(InSelf);
		Py_RETURN_NONE;
	}

	static PyObject* IsPythonOwned(FUPyWrapperObjectBase* InSelf)
	{
		const bool bResult = UUPyManager::Get()->IsPythonOwnedObject(InSelf);
		return UPyConversion::Pythonize(bResult);
	}

	static PyObject* AddToRoot(FUPyWrapperObjectBase* InSelf)
	{
		if (!FUPyWrapperObjectBase::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		InSelf->ObjectInstance->AddToRoot();
		Py_RETURN_NONE;
	}

	static PyObject* RemoveFromRoot(FUPyWrapperObjectBase* InSelf)
	{
		if (!FUPyWrapperObjectBase::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		InSelf->ObjectInstance->RemoveFromRoot();
		Py_RETURN_NONE;
	}

	static PyObject* CallMethod(FUPyWrapperObjectBase* InSelf, PyObject* InArgs, PyObject* InKwds)
	{
		if (!FUPyWrapperObjectBase::ValidateInternalState(InSelf))
		{
			return nullptr;
		}

		PyObject* PyNameObj = nullptr;
		PyObject* PyArgsObj = nullptr;
		PyObject* PyKwargsObj = nullptr;

		static const char* ArgsKwdList[] = { "name", "args", "kwargs", nullptr };
		if (!PyArg_ParseTupleAndKeywords(InArgs, InKwds, "O|OO:call_method", (char**)ArgsKwdList, &PyNameObj, &PyArgsObj, &PyKwargsObj))
		{
			return nullptr;
		}

		FName Name;
		if (!UPyConversion::Nativize(PyNameObj, Name))
		{
			UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("Failed to convert 'name' (%s) to 'Name'"), *UPyUtil::GetFriendlyTypename(PyNameObj)));
			return nullptr;
		}

		// Args must be a tuple
		if (PyArgsObj && !PyTuple_Check(PyArgsObj))
		{
			UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("'args' (%s) must be 'tuple'"), *UPyUtil::GetFriendlyTypename(PyArgsObj)));
			return nullptr;
		}

		// Kwargs must be a dict
		if (PyKwargsObj && !PyDict_Check(PyKwargsObj))
		{
			UPyUtil::SetPythonError(PyExc_TypeError, InSelf, *FString::Printf(TEXT("'kwargs' (%s) must be 'dict'"), *UPyUtil::GetFriendlyTypename(PyKwargsObj)));
			return nullptr;
		}

		// Find the named function we want to call
		const UFunction* Func = InSelf->ObjectInstance->GetClass()->FindFunctionByName(Name);
		if (!Func)
		{
			UPyUtil::SetPythonError(PyExc_Exception, InSelf, *FString::Printf(TEXT("Failed to find function '%s' on '%s'"), *Name.ToString(), *InSelf->ObjectInstance->GetClass()->GetName()));
			return nullptr;
		}

		// Args must always be set to a tuple, even if empty, for the parameter validation to work
		FUPyObjectPtr EmptyArgs;
		if (!PyArgsObj)
		{
			EmptyArgs = FUPyObjectPtr::StealReference(PyTuple_New(0));
			PyArgsObj = EmptyArgs;
		}

		// Build temporary glue for this function and call it
		const FString PythonFunctionName = UPyGenUtil::PythonizeName(Func->GetName(), UPyGenUtil::EPythonizeNameCase::Lower);
		UPyGenUtil::FGeneratedWrappedFunction GeneratedWrappedFunction;
		GeneratedWrappedFunction.SetFunction(Func);
		return FUPyWrapperObjectBase::CallFunction(InSelf, PyArgsObj, PyKwargsObj, GeneratedWrappedFunction, TCHAR_TO_UTF8(*PythonFunctionName));
	}
};


// NOTE: _T = TypeVar('_T') and Any/Type/Union/Mapping/Optional are defines by the Python typing module.
PyMethodDef ObjectBasePyMethodDefs[] = {
	{ UPyGenUtil::PostInitFuncName, UPyCFunctionCast(&FMethods_WrapperObjectBase::PostInit), METH_NOARGS, UPyDoc_STR("_post_init(self) -> None -- called during Unreal object initialization (equivalent to PostInitProperties in C++)") },
	{ "IsValid", UPyCFunctionCast(&FMethods_WrapperObjectBase::CallIsValid), METH_NOARGS, UPyDoc_STR("IsValid(self) -> bool -- returns true if this instance is valid (not null and not pending kill)") },
	{ "Cast", UPyCFunctionCast(&FMethods_WrapperObjectBase::Cast), METH_VARARGS | METH_CLASS, UPyDoc_STR("Cast(cls: Type[_T], object: object) -> _T -- cast the given object to this Unreal object type or raise an exception if the cast is not possible") },
	{ "IsA", UPyCFunctionCast(&FMethods_WrapperObjectBase::IsA), METH_VARARGS, UPyDoc_STR("IsA(self, type: Union[Class, type]) -> bool -- returns true if this instance is of the given type") },
	{ "IsChildOf", UPyCFunctionCast(&FMethods_WrapperObjectBase::IsChildOf), METH_VARARGS, UPyDoc_STR("IsChildOf(self, type: Union[Class, type]) -> bool -- returns true if this class is a child of the given type") },
	{ "GetDefaultObject", UPyCFunctionCast(&FMethods_WrapperObjectBase::GetDefaultObject), METH_NOARGS | METH_CLASS, UPyDoc_STR("GetDefaultObject(cls: Type[_T]) -> _T -- get the Unreal class default object (CDO) of this type") },
	{ "StaticClass", UPyCFunctionCast(&FMethods_WrapperObjectBase::StaticClass), METH_NOARGS | METH_CLASS, UPyDoc_STR("StaticClass(cls) -> Class -- get the Unreal class of this type") },
	{ "GetClass", UPyCFunctionCast(&FMethods_WrapperObjectBase::GetClass), METH_NOARGS, UPyDoc_STR("GetClass(self) -> Class -- get the Unreal class of this instance") },
	{ "GetOuter", UPyCFunctionCast(&FMethods_WrapperObjectBase::GetOuter), METH_NOARGS, UPyDoc_STR("GetOuter(self) -> Any -- get the outer object from this instance (if any)") },
	{ "GetTypedOuter", UPyCFunctionCast(&FMethods_WrapperObjectBase::GetTypedOuter), METH_VARARGS, UPyDoc_STR("GetTypedOuter(self, type: Union[Class, type]) -> Any -- get the first outer object of the given type from this instance (if any)") },
	{ "GetOutermost", UPyCFunctionCast(&FMethods_WrapperObjectBase::GetOutermost), METH_NOARGS, UPyDoc_STR("GetOutermost(self) -> Package -- get the outermost object (the package) from this instance") },
	{ "IsPackageExternal", UPyCFunctionCast(&FMethods_WrapperObjectBase::IsPackageExternal), METH_NOARGS, UPyDoc_STR("IsPackageExternal(self) -> bool -- returns true if this instance has a different package than its outer's package") },
	{ "GetPackage", UPyCFunctionCast(&FMethods_WrapperObjectBase::GetPackage), METH_NOARGS, UPyDoc_STR("GetPackage(self) -> Package -- get the package directly associated with this instance") },
	{ "GetName", UPyCFunctionCast(&FMethods_WrapperObjectBase::GetName), METH_NOARGS, UPyDoc_STR("GetName(self) -> str -- get the name of this instance") },
	// { "GetFName", UPyCFunctionCast(&FMethods_WrapperObjectBase::GetFName), METH_NOARGS, UPyDoc_STR("GetFName(self) -> str -- get the name of this instance") },
	{ "GetFullName", UPyCFunctionCast(&FMethods_WrapperObjectBase::GetFullName), METH_NOARGS, UPyDoc_STR("GetFullName(self) -> str -- get the full name (class name + full path) of this instance") },
	{ "GetPathName", UPyCFunctionCast(&FMethods_WrapperObjectBase::GetPathName), METH_NOARGS, UPyDoc_STR("GetPathName(self) -> str -- get the path name of this instance") },
	{ "GetWorld", UPyCFunctionCast(&FMethods_WrapperObjectBase::GetWorld), METH_NOARGS, UPyDoc_STR("GetWorld(self) -> Optional[World] -- get the world associated with this instance (if any)") },
	{ "Modify", UPyCFunctionCast(&FMethods_WrapperObjectBase::Modify), METH_VARARGS, UPyDoc_STR("Modify(self, always_mark_dirty: bool = True) -> bool -- inform that this instance is about to be modified (tracks changes for undo/redo if transactional)") },
	{ "Rename", UPyCFunctionCast(&FMethods_WrapperObjectBase::Rename), METH_VARARGS | METH_KEYWORDS, UPyDoc_STR("Rename(self, name: str=\"None\", outer: Optional[Object]=None) -> bool -- rename this instance and/or change its outer") },
#if WITH_EDITOR
	{ "GetEditorProperty", UPyCFunctionCast(&FMethods_WrapperObjectBase::GetEditorProperty), METH_VARARGS | METH_KEYWORDS, UPyDoc_STR("GetEditorProperty(self, name: str) -> object -- get the value of any property visible to the editor") },
	{ "SetEditorProperty", UPyCFunctionCast(&FMethods_WrapperObjectBase::SetEditorProperty), METH_VARARGS | METH_KEYWORDS, UPyDoc_STR("SetEditorProperty(self, name: str, value: object, notify_mode: EPropertyAccessChangeNotifyMode=EPropertyAccessChangeNotifyMode.Default) -> None -- set the value of any property visible to the editor, ensuring that the pre/post change notifications are called") },
	{ "SetEditorProperties", UPyCFunctionCast(&FMethods_WrapperObjectBase::SetEditorProperties), METH_VARARGS, UPyDoc_STR("SetEditorProperties(self, properties: dict[str, object]) -> None -- set the value of any properties visible to the editor (from a name->value dict), ensuring that the pre/post change notifications are called") },
#endif // WITH_EDITOR
	{ "AddPythonOwned", UPyCFunctionCast(&FMethods_WrapperObjectBase::AddPythonOwned), METH_NOARGS, UPyDoc_STR("AddPythonOwned(self) -> None -- add this object to the python owned set (prevents garbage collection until removed)") },
	{ "RemovePythonOwned", UPyCFunctionCast(&FMethods_WrapperObjectBase::RemovePythonOwned), METH_NOARGS, UPyDoc_STR("RemovePythonOwned(self) -> None -- remove this object from the python owned set") },
	{ "IsPythonOwned", UPyCFunctionCast(&FMethods_WrapperObjectBase::IsPythonOwned), METH_NOARGS, UPyDoc_STR("IsPythonOwned(self) -> bool -- returns true if this object is in the python owned set") },
	{ "AddToRoot", UPyCFunctionCast(&FMethods_WrapperObjectBase::AddToRoot), METH_NOARGS, UPyDoc_STR("AddToRoot(self) -> None -- add this object to the root set (prevents garbage collection)") },
	{ "RemoveFromRoot", UPyCFunctionCast(&FMethods_WrapperObjectBase::RemoveFromRoot), METH_NOARGS, UPyDoc_STR("RemoveFromRoot(self) -> None -- remove this object from the root set (allows garbage collection)") },
	{ "CallMethod", UPyCFunctionCast(&FMethods_WrapperObjectBase::CallMethod), METH_VARARGS | METH_KEYWORDS, UPyDoc_STR("CallMethod(self, name: str, *args: tuple[object, ...], **kwargs: dict[str, object]) -> Any -- call a method on this object via Unreal reflection using the given ordered (a tuple passed as args) or named (a dict passed as kwargs) argument data - allows calling methods that don't have Python glue") },
	{ nullptr, nullptr, 0, nullptr }
};
