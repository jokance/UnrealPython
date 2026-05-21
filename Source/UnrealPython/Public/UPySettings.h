
#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Engine/DeveloperSettings.h"
#include "UPySettings.generated.h"

UENUM()
enum class EUPyTypeHintingMode : uint8
{
	/** Turn off type hinting. */
	Off,

	/**
	 * When generating the Python stub and to some extend the Docstrings, enables type hinting (PEP 484) to get the best experience
	 * with a Python IDE auto-completion. The hinting will list the exact input types, omit type coercions and will assume all reflected
	 * unreal.Object cannot be None which is not true, but will let the function signature easy to read.
	 */
	AutoCompletion,

	/**
	 * Enables type hinting for static type checking. Hint as close as possible the real supported types including
	 * possible type coercions. Because the UE reflection API doesn't provide all the required information, some tradeoffs
	 * are required that do not always reflect the reality. For example, reflected UObject will always be marked as
	 * 'possibly None'. While this is true in some contexts, it is not true all the time.
	 */
	TypeChecker,
};

/**
 * Configure the UnrealPython plug-in.
 */
UCLASS(config=Game, defaultconfig)
class UUPySettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UUPySettings();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;

	//~ UDeveloperSettings interface
	virtual FText GetSectionText() const override;
#endif

	/** Array of additional paths to add to the Python system paths. */
	UPROPERTY(config, EditAnywhere, Category=Python, meta=(ConfigRestartRequired=true, RelativeToGameDir))
	TArray<FDirectoryPath> AdditionalPaths;

};
