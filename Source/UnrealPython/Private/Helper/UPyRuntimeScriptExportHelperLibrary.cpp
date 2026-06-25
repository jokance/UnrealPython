
#include "Helper/UPyRuntimeScriptExportHelperLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "Components/EditableText.h"
#include "Engine/World.h"
#include "Misc/EngineVersionComparison.h"
#include "Styling/SlateColor.h"

void UUPyRuntimeScriptExportHelperLibrary::Transform_SetRotation(FTransform& Host, const FRotator& InRot)
{
	Host.SetRotation(InRot.Quaternion());
}

FRotator UUPyRuntimeScriptExportHelperLibrary::Transform_InverseTransformRotation(const FTransform& Host, const FRotator& R)
{
	return Host.InverseTransformRotation(R.Quaternion()).Rotator();
}

FRotator UUPyRuntimeScriptExportHelperLibrary::Transform_TransformRotation(const FTransform& Host, const FRotator& R)
{
	return Host.TransformRotation(R.Quaternion()).Rotator();
}

void UUPyRuntimeScriptExportHelperLibrary::EditableText_SetColorAndOpacity(UEditableText* Host, const FSlateColor& InColorAndOpacity)
{
	if (Host)
	{
		FEditableTextStyle Style = Host->WidgetStyle;
		Style.ColorAndOpacity = InColorAndOpacity;
		Host->SetWidgetStyle(Style);
	}
}

void UUPyRuntimeScriptExportHelperLibrary::UserWidget_AddToViewportWithAutoRemove(UUserWidget* Host, int32 ZOrder, bool bAutoRemoveOnWorldRemoved)
{
	if (Host)
	{
#if UE_VERSION_OLDER_THAN(5, 8, 0)
		if (UGameViewportSubsystem* Subsystem = UGameViewportSubsystem::Get(Host->GetWorld()))
#else
		if (UGameViewportSubsystem* Subsystem = UGameViewportSubsystem::Get())
#endif
		{
			FGameViewportWidgetSlot ViewportSlot;
			if (Subsystem->IsWidgetAdded(Host))
			{
				ViewportSlot = Subsystem->GetWidgetSlot(Host);
			}

			ViewportSlot.ZOrder = ZOrder;
			ViewportSlot.bAutoRemoveOnWorldRemoved = bAutoRemoveOnWorldRemoved;

			Subsystem->AddWidget(Host, ViewportSlot);
		}
	}
}


FVector2D UUPyRuntimeScriptExportHelperLibrary::Geometry_GetLocalSize(const FGeometry& Host)
{
	return Host.GetLocalSize();
}

FVector2D UUPyRuntimeScriptExportHelperLibrary::Geometry_GetAbsoluteSize(const FGeometry& Host)
{
	return Host.GetAbsoluteSize();
}

FVector2D UUPyRuntimeScriptExportHelperLibrary::Geometry_GetAbsolutePosition(const FGeometry& Host)
{
	return Host.GetAbsolutePosition();
}

FVector2D UUPyRuntimeScriptExportHelperLibrary::Geometry_GetAbsolutePositionAtCoordinates(const FGeometry& Host, const FVector2D& NormalCoordinates)
{
	return Host.GetAbsolutePositionAtCoordinates(NormalCoordinates);
}

FVector2D UUPyRuntimeScriptExportHelperLibrary::Geometry_GetLocalPositionAtCoordinates(const FGeometry& Host, const FVector2D& NormalCoordinates)
{
	return Host.GetLocalPositionAtCoordinates(NormalCoordinates);
}

bool UUPyRuntimeScriptExportHelperLibrary::Geometry_IsUnderLocation(const FGeometry& Host, const FVector2D& AbsoluteCoordinate)
{
	return Host.IsUnderLocation(AbsoluteCoordinate);
}

FVector2D UUPyRuntimeScriptExportHelperLibrary::Geometry_AbsoluteToLocal(const FGeometry& Host, const FVector2D& AbsoluteCoordinate)
{
	return Host.AbsoluteToLocal(AbsoluteCoordinate);
}

FVector2D UUPyRuntimeScriptExportHelperLibrary::Geometry_LocalToAbsolute(const FGeometry& Host, const FVector2D& LocalCoordinate)
{
	return Host.LocalToAbsolute(LocalCoordinate);
}

int32 UUPyRuntimeScriptExportHelperLibrary::World_GetNetMode(UWorld* Host)
{
	return Host ? static_cast<int32>(Host->GetNetMode()) : static_cast<int32>(NM_MAX);
}

FString UUPyRuntimeScriptExportHelperLibrary::World_GetNetModeName(UWorld* Host)
{
	if (!Host)
	{
		return TEXT("Unknown");
	}

	switch (Host->GetNetMode())
	{
	case NM_Standalone:
		return TEXT("Standalone");
	case NM_DedicatedServer:
		return TEXT("DedicatedServer");
	case NM_ListenServer:
		return TEXT("ListenServer");
	case NM_Client:
		return TEXT("Client");
	default:
		return TEXT("Unknown");
	}
}
