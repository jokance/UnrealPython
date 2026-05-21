
#pragma once

#include "CoreTypes.h"

/** States that can be applied to a Python conversion result */
enum class EUPyConversionResultState : uint8
{
	/** Conversion failed */
	Failure,
	/** Conversion succeeded */
	Success,
	/** Conversion succeeded, but type coercion occurred */
	SuccessWithCoercion,
};

/** The result of attempting a Python conversion */
class FUPyConversionResult
{
public:
	/** Default constructor */
	FUPyConversionResult()
		: State(EUPyConversionResultState::Failure)
	{
	}

	/** Construct from a specific state */
	explicit FUPyConversionResult(const EUPyConversionResultState InState)
		: State(InState)
	{
	}

	/** Factory for a result set to the Failure state */
	FORCEINLINE static FUPyConversionResult Failure()
	{
		return FUPyConversionResult(EUPyConversionResultState::Failure);
	}

	/** Factory for a result set to the Success state */
	FORCEINLINE static FUPyConversionResult Success()
	{
		return FUPyConversionResult(EUPyConversionResultState::Success);
	}

	/** Factory for a result set to the SuccessWithCoercion state */
	FORCEINLINE static FUPyConversionResult SuccessWithCoercion()
	{
		return FUPyConversionResult(EUPyConversionResultState::SuccessWithCoercion);
	}

	/** Is this result in a successful state? */
	explicit operator bool() const
	{
		return Succeeded();
	}

	/** Is this result in a successful state? */
	bool Succeeded() const
	{
		return State != EUPyConversionResultState::Failure;
	}

	/** Is this result in a failure state? */
	bool Failed() const
	{
		return State == EUPyConversionResultState::Failure;
	}

	/** Get the current result state */
	EUPyConversionResultState GetState() const
	{
		return State;
	}

	/** Set the result state */
	void SetState(const EUPyConversionResultState InState)
	{
		State = InState;
	}

private:
	/** Current state of this result */
	EUPyConversionResultState State;
};

/** Helper function to set the value of an optional conversion result */
FORCEINLINE void SetOptionalUPyConversionResult(const FUPyConversionResult& InResult, FUPyConversionResult* OutResult)
{
	if (OutResult)
	{
		*OutResult = InResult;
	}
}
