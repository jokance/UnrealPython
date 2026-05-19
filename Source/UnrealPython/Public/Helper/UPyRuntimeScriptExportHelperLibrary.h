// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once


#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UPyRuntimeScriptExportHelperLibrary.generated.h"

class UUserWidget;
class UGameViewportSubsystem;
class UEditableText;
class UAkAudioEvent;

UCLASS()
class UUPyRuntimeScriptExportHelperLibrary: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// ============================================
	// FTransform exposed for scripting
	// ============================================

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "SetRotation", UPyUseHelperMethod))
	static void Transform_SetRotation(FTransform& Host, const FRotator& InRot);

	/**
	 * Inverse transforms a rotation.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "InverseTransformRotation", UPyUseHelperMethod))
	static FRotator Transform_InverseTransformRotation(const FTransform& Host, const FRotator& R);

	/**
	* Transforms a rotation.
	*/
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "TransformRotation", UPyUseHelperMethod))
	static FRotator Transform_TransformRotation(const FTransform& Host, const FRotator& R);

	// ============================================
	// UEditableText exposed for scripting
	// ============================================

	UFUNCTION(BlueprintCallable, meta=(UPyScriptMethod = "SetColorAndOpacity", UPyUseHelperMethod))
	static void EditableText_SetColorAndOpacity(UEditableText* Host, const FSlateColor& InColorAndOpacity);

	// ============================================
	// UUserWidget exposed for scripting
	// ============================================

	/**
	* Adds the widget to the game's viewport.
	*/
	UFUNCTION(BlueprintCallable, meta=(UPyScriptMethod = "AddToViewportWithAutoRemove", UPyUseHelperMethod))
	static void UserWidget_AddToViewportWithAutoRemove(UUserWidget* Host, int32 ZOrder = 0, bool bAutoRemoveOnWorldRemoved = true);

	// ============================================
	// FGeometry exposed for scripting
	// ============================================
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetLocalSize", UPyUseHelperMethod))
	static FVector2D Geometry_GetLocalSize(const FGeometry& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetAbsoluteSize", UPyUseHelperMethod))
	static FVector2D Geometry_GetAbsoluteSize(const FGeometry& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetAbsolutePosition", UPyUseHelperMethod))
	static FVector2D Geometry_GetAbsolutePosition(const FGeometry& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetAbsolutePositionAtCoordinates", UPyUseHelperMethod))
	static FVector2D Geometry_GetAbsolutePositionAtCoordinates(const FGeometry& Host, const FVector2D& NormalCoordinates);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetLocalPositionAtCoordinates", UPyUseHelperMethod))
	static FVector2D Geometry_GetLocalPositionAtCoordinates(const FGeometry& Host, const FVector2D& NormalCoordinates);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "IsUnderLocation", UPyUseHelperMethod))
	static bool Geometry_IsUnderLocation(const FGeometry& Host, const FVector2D& AbsoluteCoordinate);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "AbsoluteToLocal", UPyUseHelperMethod))
	static FVector2D Geometry_AbsoluteToLocal(const FGeometry& Host, const FVector2D& AbsoluteCoordinate);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "LocalToAbsolute", UPyUseHelperMethod))
	static FVector2D Geometry_LocalToAbsolute(const FGeometry& Host, const FVector2D& LocalCoordinate);
};
