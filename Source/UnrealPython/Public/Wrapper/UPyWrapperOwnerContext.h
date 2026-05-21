
#pragma once

#include "UPyCommon.h"
#include "Core/UPyConversion.h"
#include "Core/UPyPtr.h"
#include "UObject/PropertyAccessUtil.h"

struct FUPyWrapperBase;

/** Owner context information for wrapped types */
class UNREALPYTHON_API FUPyWrapperOwnerContext
{
public:
	/** Default constructor */
	FUPyWrapperOwnerContext();

	/** Construct this context from the given Python object and optional property (will create a new reference to the given object) */
	explicit FUPyWrapperOwnerContext(PyObject* InOwner, const FProperty* InProp = nullptr);

	/** Construct this context from the given Python object and optional property */
	explicit FUPyWrapperOwnerContext(const FUPyObjectPtr& InOwner, const FProperty* InProp = nullptr);

	/** Reset this context back to its default state */
	void Reset();

	/** Check to see if this context has an owner set */
	bool HasOwner() const;

	/** Get the Python object that owns the instance being wrapped (if any, borrowed reference) */
	PyObject* GetOwnerObject() const;

	UObject* GetOwnerUObject() const;

	/** Get the property on the owner object that owns the instance being wrapped (if known) */
	const FProperty* GetOwnerProperty() const;

	/** Assert that the given conversion method is valid for this owner context */
	void AssertValidConversionMethod(const EUPyConversionMethod InMethod) const;

	/** Build the property change notify that corresponds to this owner context, or null if this owner context shouldn't emit change notifications */
	TUniquePtr<FPropertyAccessChangeNotify> BuildChangeNotify(const EPropertyAccessChangeNotifyMode InNotifyMode) const;

	/** Walk the owner context chain to find a UObject owner instance that should receive change notifications (if any) */
	UObject* FindChangeNotifyObject() const;

	void AddOwnedPyProp(FUPyWrapperBase* PyProp) const;

#if WITH_EDITOR
	void UpdateOwnerProperty(const FProperty* InProp) { OwnerProperty = InProp; }
#endif

private:
	/** The Python object that owns the instance being wrapped (if any) */
	FUPyObjectPtr OwnerObject;

	/** The property on the owner object that owns the instance being wrapped (if known) */
	const FProperty* OwnerProperty;
};
