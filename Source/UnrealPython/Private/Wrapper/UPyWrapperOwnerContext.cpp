
#include "Wrapper/UPyWrapperOwnerContext.h"
#include "Wrapper/UPyWrapperObjectBase.h"
#include "Wrapper/UPyWrapperStruct.h"
#include "Wrapper/UPyWrapperTypeFactory.h"

FUPyWrapperOwnerContext::FUPyWrapperOwnerContext()
	: OwnerObject()
	, OwnerProperty(nullptr)
{
}

FUPyWrapperOwnerContext::FUPyWrapperOwnerContext(PyObject* InOwner, const FProperty* InProp)
	: OwnerObject(FUPyObjectPtr::NewReference(InOwner))
	, OwnerProperty(InProp)
{
	checkf(!OwnerProperty || OwnerObject.IsValid(), TEXT("Owner context cannot have an owner property without an owner object"));
}

FUPyWrapperOwnerContext::FUPyWrapperOwnerContext(const FUPyObjectPtr& InOwner, const FProperty* InProp)
	: OwnerObject(InOwner)
	, OwnerProperty(InProp)
{
	checkf(!OwnerProperty || OwnerObject.IsValid(), TEXT("Owner context cannot have an owner property without an owner object"));
}

void FUPyWrapperOwnerContext::Reset()
{
	OwnerObject.Reset();
	OwnerProperty = nullptr;
}

bool FUPyWrapperOwnerContext::HasOwner() const
{
	return OwnerObject.IsValid();
}

PyObject* FUPyWrapperOwnerContext::GetOwnerObject() const
{
	return (PyObject*)OwnerObject.GetPtr();
}

UObject* FUPyWrapperOwnerContext::GetOwnerUObject() const
{
	FUPyWrapperOwnerContext OwnerContext = *this;
	if (OwnerContext.HasOwner())
	{
		PyObject* PyObj = OwnerContext.GetOwnerObject();

		if (PyObject_IsInstance(PyObj, (PyObject*)&UPyWrapperObjectBaseType) == 1)
		{
			return ((FUPyWrapperObjectBase*)PyObj)->ObjectInstance;
		}
	}

	return nullptr;
}

const FProperty* FUPyWrapperOwnerContext::GetOwnerProperty() const
{
	return OwnerProperty;
}

void FUPyWrapperOwnerContext::AssertValidConversionMethod(const EUPyConversionMethod InMethod) const
{
	::AssertValidUPyConversionOwner(GetOwnerObject(), InMethod);
}

TUniquePtr<FPropertyAccessChangeNotify> FUPyWrapperOwnerContext::BuildChangeNotify(const EPropertyAccessChangeNotifyMode InNotifyMode) const
{
#if WITH_EDITOR
	if (InNotifyMode != EPropertyAccessChangeNotifyMode::Never)
	{
		TUniquePtr<FPropertyAccessChangeNotify> ChangeNotify = MakeUnique<FPropertyAccessChangeNotify>();
		ChangeNotify->NotifyMode = InNotifyMode;

		auto AppendOwnerPropertyToChain = [&ChangeNotify](const FUPyWrapperOwnerContext& InOwnerContext) -> bool
		{
			const FProperty* LeafProp = nullptr;
			if (PyObject_IsInstance(InOwnerContext.GetOwnerObject(), (PyObject*)&UPyWrapperObjectBaseType) == 1 || PyObject_IsInstance(InOwnerContext.GetOwnerObject(), (PyObject*)&UPyWrapperStructType) == 1)
			{
				LeafProp = InOwnerContext.GetOwnerProperty();
			}

			if (LeafProp)
			{
				ChangeNotify->ChangedPropertyChain.AddHead(const_cast<FProperty*>(LeafProp));
				return true;
			}

			return false;
		};

		FUPyWrapperOwnerContext OwnerContext = *this;
		while (OwnerContext.HasOwner() && AppendOwnerPropertyToChain(OwnerContext))
		{
			PyObject* PyObj = OwnerContext.GetOwnerObject();

			if (PyObj == GetOwnerObject())
			{
				ChangeNotify->ChangedPropertyChain.SetActivePropertyNode(ChangeNotify->ChangedPropertyChain.GetHead()->GetValue());
			}

			if (PyObject_IsInstance(PyObj, (PyObject*)&UPyWrapperObjectBaseType) == 1)
			{
				// Found an object, this is the end of the chain
				ChangeNotify->ChangedObject = ((FUPyWrapperObjectBase*)PyObj)->ObjectInstance;
				ChangeNotify->ChangedPropertyChain.SetActiveMemberPropertyNode(ChangeNotify->ChangedPropertyChain.GetHead()->GetValue());
				break;
			}

			if (PyObject_IsInstance(PyObj, (PyObject*)&UPyWrapperStructType) == 1)
			{
				// Found a struct, recurse up the chain
				OwnerContext = ((FUPyWrapperStruct*)PyObj)->OwnerContext;
				continue;
			}

			// Unknown object type - just bail
			break;
		}

		// If we didn't find an object in the chain then we can't emit notifications
		if (!ChangeNotify->ChangedObject)
		{
			ChangeNotify.Reset();
		}

		return ChangeNotify;
	}
#endif
	return nullptr;
}

UObject* FUPyWrapperOwnerContext::FindChangeNotifyObject() const
{
	FUPyWrapperOwnerContext OwnerContext = *this;
	while (OwnerContext.HasOwner())
	{
		PyObject* PyObj = OwnerContext.GetOwnerObject();

		if (PyObject_IsInstance(PyObj, (PyObject*)&UPyWrapperObjectBaseType) == 1)
		{
			// Found an object, this is the end of the chain
			return ((FUPyWrapperObjectBase*)PyObj)->ObjectInstance;
		}

		if (PyObject_IsInstance(PyObj, (PyObject*)&UPyWrapperStructType) == 1)
		{
			// Found a struct, recurse up the chain
			OwnerContext = ((FUPyWrapperStruct*)PyObj)->OwnerContext;
			continue;
		}

		// Unknown object type - just bail
		break;
	}

	return nullptr;
}

void FUPyWrapperOwnerContext::AddOwnedPyProp(FUPyWrapperBase* PyProp) const
{
	if (UObject* Object = FindChangeNotifyObject())
	{
		FUPyWrapperObjectFactory::Get().AddOwnedPyProp(Object, PyProp);
	}
}

void FUPyWrapperOwnerContext::RemoveOwnedPyProp(FUPyWrapperBase* PyProp) const
{
	if (UObject* Object = FindChangeNotifyObject())
	{
		FUPyWrapperObjectFactory::Get().RemoveOwnedPyProp(Object, PyProp);
	}
}
