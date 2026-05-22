
#include "UPyModuleInitializer.h"
#include "Core/UPyPtr.h"
#include "Core/UPyCore.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/UserDefinedEnum.h"
#include "StructUtils/UserDefinedStruct.h"
#include "Utils/UPyGenUtil.h"
#include "DynamicTypes/UPyConstant.h"
#include "Wrapper/UPyWrapperEnum.h"
#include "Wrapper/UPyWrapperObjectBase.h"
#include "Wrapper/UPyWrapperObject.h"
#include "Wrapper/UPyWrapperTypeRegistry.h"
#include "Wrapper/UPyWrapperStruct.h"
#include "Wrapper/UPyWrapperArray.h"
#include "Wrapper/UPyWrapperSet.h"
#include "Wrapper/UPyWrapperFieldPath.h"
#include "Wrapper/UPyWrapperFixedArray.h"
#include "Wrapper/UPyWrapperDelegate.h"
#include "Wrapper/UPyWrapperMap.h"
#include "Wrapper/AutoGen/UPyAutoGenWrapper.h"
#include "Subclassing/UPyGeneratedClass.h"
#include "Subclassing/UPyGeneratedStruct.h"
#include "Subclassing/UPyGeneratedEnum.h"

extern PyMethodDef UEPyMethodDefs[];

// ue模块类型
static PyTypeObject UPyUEModuleType = {
	PyVarObject_HEAD_INIT(nullptr, 0)
	"UPyUEModule",
};

static PyObject* Getattro2UEModule(PyObject* Self, PyObject* AttrName)
{
	PyObject* FoundPyType = UPyUEModuleType.tp_base->tp_getattro(Self, AttrName);
	if (FoundPyType)
	{
		return FoundPyType;
	}

	PyErr_Clear();

	const char* AttrStr = PyUnicode_AsUTF8(AttrName);
	FoundPyType = FUPyModuleInitializer::Get().FindOrAddPyTypeByName(AttrStr);
	if (!FoundPyType)
	{
		PyErr_Format(PyExc_AttributeError, "ue module has not attr: %s", AttrStr);
	}

	return FoundPyType;
}

FUPyModuleInitializer& FUPyModuleInitializer::Get()
{
	static FUPyModuleInitializer PyModuleInitializer;
	return PyModuleInitializer;
}

bool FUPyModuleInitializer::InitializeModules()
{
	if (bIsInitialized)
	{
		return true;
	}

	bIsInitialized = true;

	InitializeUPyConstant();

	if (!InitializeMainModule())
	{
		UE_LOG(LogUnrealPython, Error, TEXT("Failed to initialize main module"));
		return false;
	}

	if (!InitializeUEModule())
	{
		UE_LOG(LogUnrealPython, Error, TEXT("Failed to initialize ue module"));
		return false;
	}

	ConfigureModuleGlobals();

	InitializeCoreModule();

	return true;
}

void FUPyModuleInitializer::CleanupModules()
{
	PyUEDict.Reset();
	PyUEModule.Reset();
	PyMainDict.Reset();
	PyMainModule.Reset();

	bIsInitialized = false;
}

PyObject* FUPyModuleInitializer::FindOrAddPyTypeByName(const FString& AttrName)
{
	UClass* Class = nullptr;
	UScriptStruct* Struct = nullptr;
	UEnum* Enum = nullptr;

	TArray<UObject*> Objects;
	StaticFindAllObjects(Objects, nullptr, *AttrName);

	for (UObject* Object : Objects)
	{
		if (Object->IsA(UClass::StaticClass()))
		{
			Class = (UClass*)Object;
			break;
		}

		if (!Struct && Object->IsA(UScriptStruct::StaticClass()))
		{
			Struct = (UScriptStruct*)Object;
		}

		if (!Enum && Object->IsA(UEnum::StaticClass()))
		{
			Enum = (UEnum*)Object;
		}
	}

	PyObject* FoundPyType = nullptr;
	if (Class)
	{
		if (Class->HasAnyInternalFlags(EInternalObjectFlags::Native))
		{
			check(!((UObject*)Class)->IsA<UBlueprintGeneratedClass>());
			if (const PyTypeObject* PyType = FUPyWrapperTypeRegistry::Get().GetWrappedClassType(Class))
			{
				FoundPyType = (PyObject*)PyType;
				PyDict_SetItemString(PyUEDict, PyType->tp_name, FoundPyType);
			}
		}
	}
	else if (Struct)
	{
		if (Struct->HasAnyInternalFlags(EInternalObjectFlags::Native))
		{
			check(!Struct->IsA<UUserDefinedStruct>());
			if (const PyTypeObject* PyType = FUPyWrapperTypeRegistry::Get().GetWrappedStructType(Struct))
			{
				FoundPyType = (PyObject*)PyType;
				PyDict_SetItemString(PyUEDict, PyType->tp_name, FoundPyType);
			}
		}
	}
	else if (Enum)
	{
		if (Enum->HasAnyInternalFlags(EInternalObjectFlags::Native))
		{
			check(!Enum->IsA<UUserDefinedEnum>());
			if (const PyTypeObject* PyType = FUPyWrapperTypeRegistry::Get().GetWrappedEnumType(Enum))
			{
				FoundPyType = (PyObject*)PyType;
				PyDict_SetItemString(PyUEDict, PyType->tp_name, FoundPyType);
			}
		}
	}

	Py_XINCREF(FoundPyType);
	return FoundPyType;
}

bool FUPyModuleInitializer::InitializeMainModule()
{
	PyMainModule = FUPyObjectPtr::NewReference(PyImport_ImportModule("__main__"));
	if (PyMainModule)
	{
		PyMainDict = FUPyObjectPtr::NewReference(PyModule_GetDict(PyMainModule));
		if (PyMainDict)
		{
			return true;
		}
	}
	return false;
}

bool FUPyModuleInitializer::InitializeUEModule()
{
	const char* UEModuleName = "ue";

	PyTypeObject* UEModuleType = &UPyUEModuleType;
	UEModuleType->tp_base = &PyModule_Type;
	UEModuleType->tp_flags = Py_TPFLAGS_DEFAULT;
	UEModuleType->tp_getattro = Getattro2UEModule;
	if (PyType_Ready(&UPyUEModuleType) < 0)
	{
		return false;
	}

	PyUEModule = FUPyObjectPtr::NewReference(PyImport_AddModule(UEModuleName));
	if (!PyUEModule)
	{
		return false;
	}

	PyModule_AddFunctions(PyUEModule, UEPyMethodDefs);

	Py_SET_TYPE(PyUEModule, UEModuleType);

	PyUEDict = FUPyObjectPtr::NewReference(PyModule_GetDict(PyUEModule));

	return true;
}

void FUPyModuleInitializer::InitializeCoreModule()
{
	UPyUtil::GetPythonTypeContainerSingleton().Reset(::NewObject<UPackage>(nullptr, TEXT("/Engine/UPyTypes"), RF_Public | RF_Transient));
	UPyUtil::GetPythonTypeContainerSingleton()->SetPackageFlags(PKG_ContainsScript);

	UPyGenUtil::FNativePythonModule NativePythonModule;
	NativePythonModule.PyModuleMethods = UEPyMethodDefs;

	NativePythonModule.PyModule = PyUEModule;

	InitializeUPyDelegateHandle(NativePythonModule);
	InitializeUPyScopedSlowTask(NativePythonModule);
	InitializeUPyObjectIterator(NativePythonModule);
	InitializeUPyClassIterator(NativePythonModule);
	InitializeUPyStructIterator(NativePythonModule);
	InitializeUPyTypeIterator(NativePythonModule);
	InitializeUPyUValueDef(NativePythonModule);
	InitializeUPyFPropertyDef(NativePythonModule);
	InitializeUPyUFunctionDef(NativePythonModule);

	InitializeUPyUFunctionDecorator(NativePythonModule);
	InitializeUPyUClassDecorator(NativePythonModule);
	InitializeUPyUEnumDecorator(NativePythonModule);
	InitializeUPyUStructDecorator(NativePythonModule);

	InitializeUPyWrapperBase(NativePythonModule);
	InitializeUPyWrapperObjectBase(NativePythonModule);
	InitializeUPyWrapperObject(NativePythonModule);
	InitializeUPyWrapperStruct(NativePythonModule);
	InitializeUPyWrapperEnum(NativePythonModule);
	InitializeUPyWrapperArray(NativePythonModule);
	InitializeUPyWrapperSet(NativePythonModule);
	InitializeUPyWrapperMap(NativePythonModule);
	InitializeUPyWrapperFixedArray(NativePythonModule);
	InitializeUPyWrapperFieldPath(NativePythonModule);
	InitializeUPyWrapperDelegate(NativePythonModule);

	InitializeAutoGenWrapperTypes(NativePythonModule);

	FUPyWrapperTypeRegistry::Get().RegisterNativePythonModule(MoveTemp(NativePythonModule));

	FUnrealPythonModule::OnInitializePythonWrappers.Broadcast();

	FUPyWrapperTypeRegistry::Get().StartCreateAutoWrappedTypes(NativePythonModule);
}

void FUPyModuleInitializer::ConfigureModuleGlobals()
{
#if UE_BUILD_SHIPPING
	PyDict_SetItemString(PyUEDict, "BUILD_SHIPPING", Py_True);
#else
	PyDict_SetItemString(PyUEDict, "BUILD_SHIPPING", Py_False);
#endif

	PyDict_SetItemString(PyUEDict, "GIsEditor", FUPyObjectPtr::StealReference(PyBool_FromLong(GIsEditor)));
}
