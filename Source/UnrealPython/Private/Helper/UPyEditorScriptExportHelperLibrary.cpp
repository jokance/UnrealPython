
#if WITH_EDITOR

#include "Helper/UPyEditorScriptExportHelperLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/ProgressBar.h"
#include "Components/EditableTextBox.h"
#include "Styling/SlateColor.h"
#include "Styling/StyleColors.h"

FVector UUPyEditorScriptExportHelperLibrary::Vector_ZeroVector(const FVector& A)
{
	return FVector::ZeroVector;
}

FVector UUPyEditorScriptExportHelperLibrary::Vector_OneVector(const FVector& A)
{
	return FVector::ZeroVector;
}

FVector UUPyEditorScriptExportHelperLibrary::Vector_UpVector(const FVector& Host)
{
	return FVector::UpVector;
}

FVector UUPyEditorScriptExportHelperLibrary::Vector_ForwardVector(const FVector& Host)
{
	return FVector::ForwardVector;
}

FVector UUPyEditorScriptExportHelperLibrary::Vector_RightVector(const FVector& Host)
{
	return FVector::RightVector;
}

FVector UUPyEditorScriptExportHelperLibrary::Vector_Cross(const FVector& A, const FVector& B)
{
	return FVector::ZeroVector;
}

FVector UUPyEditorScriptExportHelperLibrary::Vector_CrossProduct(const FVector& Host, const FVector& A, const FVector& B)
{
	return FVector::ZeroVector;
}

double UUPyEditorScriptExportHelperLibrary::Vector_Dot(const FVector& A, const FVector& B)
{
	return 0.0;
}

double UUPyEditorScriptExportHelperLibrary::Vector_DotProduct(const FVector& Host, const FVector& A, const FVector& B)
{
	return 0.0;
}

FVector UUPyEditorScriptExportHelperLibrary::Vector_AddVector(const FVector& A, const FVector& B)
{
	return FVector::ZeroVector;
}

FVector UUPyEditorScriptExportHelperLibrary::Vector_AddFloat(const FVector& A, double Bias)
{
	return FVector::ZeroVector;
}

FVector UUPyEditorScriptExportHelperLibrary::Vector_SubtractVector(const FVector& A, const FVector& B)
{
	return FVector::ZeroVector;
}

FVector UUPyEditorScriptExportHelperLibrary::Vector_SubtractFloat(const FVector& A, double Bias)
{
	return FVector::ZeroVector;
}

bool UUPyEditorScriptExportHelperLibrary::Vector_Equal(const FVector& A, const FVector& B)
{
	return false;
}

bool UUPyEditorScriptExportHelperLibrary::Vector_NotEqual(const FVector& A, const FVector& B)
{
	return false;
}

FVector UUPyEditorScriptExportHelperLibrary::Vector_MultiplyFloat(const FVector& A, double Scale)
{
	return FVector::ZeroVector;
}

FVector UUPyEditorScriptExportHelperLibrary::Vector_MultiplyVector(const FVector& A, const FVector& B)
{
	return FVector::ZeroVector;
}

FVector UUPyEditorScriptExportHelperLibrary::Vector_DivideFloat(const FVector& A, double Scale)
{
	return FVector::ZeroVector;
}

FVector UUPyEditorScriptExportHelperLibrary::Vector_DivideVector(const FVector& A, const FVector& B)
{
	return FVector::ZeroVector;
}

FVector UUPyEditorScriptExportHelperLibrary::Vector_Negated(const FVector& A)
{
	return FVector::ZeroVector;
}

double UUPyEditorScriptExportHelperLibrary::Vector_Size(const FVector& A)
{
	return 0.0;
}

double UUPyEditorScriptExportHelperLibrary::Vector_Size2D(const FVector& A)
{
	return 0.0;
}

double UUPyEditorScriptExportHelperLibrary::Vector_SizeSquared(const FVector& A)
{
	return 0.0;
}

FVector UUPyEditorScriptExportHelperLibrary::Vector_GetSafeNormal(const FVector& A, double Tolerance)
{
	return FVector::ZeroVector;
}

double UUPyEditorScriptExportHelperLibrary::Vector_Dist(const FVector& Host, const FVector& V1, const FVector& V2)
{
	return FVector::Dist(V1, V2);
}

double UUPyEditorScriptExportHelperLibrary::Vector_Dist2D(const FVector& Host, const FVector& V1, const FVector& V2)
{
	return FVector::Dist2D(V1, V2);
}

double UUPyEditorScriptExportHelperLibrary::Vector_DistSquared(const FVector& Host, const FVector& V1, const FVector& V2)
{
	return FVector::DistSquared(V1, V2);
}


bool UUPyEditorScriptExportHelperLibrary::Vector_IsZero(const FVector& A)
{
	return false;
}

bool UUPyEditorScriptExportHelperLibrary::Vector_IsNearlyZero(const FVector& A, double Tolerance)
{
	return false;
}

bool UUPyEditorScriptExportHelperLibrary::Vector_IsNormalized(const FVector& A)
{
	return false;
}

FVector UUPyEditorScriptExportHelperLibrary::Vector_ProjectOnTo(const FVector& V, const FVector& A)
{
	return FVector::ZeroVector;
}

FVector UUPyEditorScriptExportHelperLibrary::Vector_ProjectOnToNormal(const FVector& V, const FVector& Normal)
{
	return FVector::ZeroVector;
}

FVector UUPyEditorScriptExportHelperLibrary::Vector_GetClampedToSize(const FVector& A, double Min, double Max)
{
	return FVector::ZeroVector;
}

FVector UUPyEditorScriptExportHelperLibrary::Vector_GetClampedToMaxSize(const FVector& A, double Max)
{
	return FVector::ZeroVector;
}

FVector UUPyEditorScriptExportHelperLibrary::Vector_GetAbs(const FVector& A)
{
	return FVector::ZeroVector;
}

FVector UUPyEditorScriptExportHelperLibrary::Vector_MirrorByVector(const FVector& A, const FVector& MirrorNormal)
{
	return FVector::ZeroVector;
}

FVector UUPyEditorScriptExportHelperLibrary::Vector_RotateAngleAxis(const FVector& V, double AngleDeg, const FVector& Axis)
{
	return FVector::ZeroVector;
}

FRotator UUPyEditorScriptExportHelperLibrary::Vector_ToOrientationRotator(const FVector& InVec)
{
	return FRotator::ZeroRotator;
}

void UUPyEditorScriptExportHelperLibrary::Vector_Init1(const FVector& InVec)
{
}

void UUPyEditorScriptExportHelperLibrary::Vector_Init2(const FVector& InVec, double InF)
{
}

void UUPyEditorScriptExportHelperLibrary::Vector_Init3(const FVector& InVec, int32 InI)
{
}

void UUPyEditorScriptExportHelperLibrary::Vector_Init4(const FVector& InVec, double InX, double InY, double InZ)
{
}

FVector2D UUPyEditorScriptExportHelperLibrary::Vector2D_ZeroVector(const FVector2D& Host)
{
	return FVector2D::ZeroVector;
}

FVector2D UUPyEditorScriptExportHelperLibrary::Vector2D_UnitVector(const FVector2D& Host)
{
	return FVector2D::UnitVector;
}

FVector2D UUPyEditorScriptExportHelperLibrary::Vector2D_Unit45Deg(const FVector2D& Host)
{
	return FVector2D::Unit45Deg;
}

double UUPyEditorScriptExportHelperLibrary::Vector2D_CrossProduct(const FVector2D& Host, const FVector2D& A, const FVector2D& B)
{
	return FVector2D::CrossProduct(A, B);
}

double UUPyEditorScriptExportHelperLibrary::Vector2D_DotProduct(const FVector2D& Host, const FVector2D& A, const FVector2D& B)
{
	return FVector2D::DotProduct(A, B);
}

double UUPyEditorScriptExportHelperLibrary::Vector2D_Distance(const FVector2D& Host, const FVector2D& V1, const FVector2D& V2)
{
	return FVector2D::Distance(V1, V2);
}

double UUPyEditorScriptExportHelperLibrary::Vector2D_DistSquared(const FVector2D& Host, const FVector2D& V1, const FVector2D& V2)
{
	return FVector2D::DistSquared(V1, V2);
}

FVector2D UUPyEditorScriptExportHelperLibrary::Vector2D_AddVector(const FVector2D& Host, const FVector2D& V)
{
	return Host + V;
}

FVector2D UUPyEditorScriptExportHelperLibrary::Vector2D_AddFloat(const FVector2D& Host, double Bias)
{
	return Host + Bias;
}

FVector2D UUPyEditorScriptExportHelperLibrary::Vector2D_SubtractVector(const FVector2D& Host, const FVector2D& V)
{
	return Host - V;
}

FVector2D UUPyEditorScriptExportHelperLibrary::Vector2D_SubtractFloat(const FVector2D& Host, double Bias)
{
	return Host - Bias;
}

FVector2D UUPyEditorScriptExportHelperLibrary::Vector2D_MultiplyVector(const FVector2D& Host, const FVector2D& V)
{
	return Host * V;
}

FVector2D UUPyEditorScriptExportHelperLibrary::Vector2D_MultiplyFloat(const FVector2D& Host, double Scale)
{
	return Host * Scale;
}

FVector2D UUPyEditorScriptExportHelperLibrary::Vector2D_DivideVector(const FVector2D& Host, const FVector2D& V)
{
	return Host / V;
}

FVector2D UUPyEditorScriptExportHelperLibrary::Vector2D_DivideFloat(const FVector2D& Host, double Scale)
{
	return Host / Scale;
}

FVector2D UUPyEditorScriptExportHelperLibrary::Vector2D_Negated(const FVector2D& Host)
{
	return -Host;
}

bool UUPyEditorScriptExportHelperLibrary::Vector2D_Equal(const FVector2D& Host, const FVector2D& V)
{
	return Host == V;
}

bool UUPyEditorScriptExportHelperLibrary::Vector2D_NotEqual(const FVector2D& Host, const FVector2D& V)
{
	return Host != V;
}

double UUPyEditorScriptExportHelperLibrary::Vector2D_Dot(const FVector2D& Host, const FVector2D& V)
{
	return Host | V;
}

double UUPyEditorScriptExportHelperLibrary::Vector2D_Cross(const FVector2D& Host, const FVector2D& V)
{
	return Host ^ V;
}

double UUPyEditorScriptExportHelperLibrary::Vector2D_Size(const FVector2D& Host)
{
	return Host.Size();
}

double UUPyEditorScriptExportHelperLibrary::Vector2D_SizeSquared(const FVector2D& Host)
{
	return Host.SizeSquared();
}

FVector2D UUPyEditorScriptExportHelperLibrary::Vector2D_GetSafeNormal(const FVector2D& Host, double Tolerance)
{
	return Host.GetSafeNormal(Tolerance);
}

void UUPyEditorScriptExportHelperLibrary::Vector2D_Normalize(FVector2D& Host, double Tolerance)
{
	Host.Normalize(Tolerance);
}

bool UUPyEditorScriptExportHelperLibrary::Vector2D_IsNearlyZero(const FVector2D& Host, double Tolerance)
{
	return Host.IsNearlyZero(Tolerance);
}

bool UUPyEditorScriptExportHelperLibrary::Vector2D_IsZero(const FVector2D& Host)
{
	return Host.IsZero();
}

FVector2D UUPyEditorScriptExportHelperLibrary::Vector2D_GetAbs(const FVector2D& Host)
{
	return Host.GetAbs();
}

FVector2D UUPyEditorScriptExportHelperLibrary::Vector2D_GetRotated(const FVector2D& Host, double AngleDeg)
{
	return Host.GetRotated(AngleDeg);
}

void UUPyEditorScriptExportHelperLibrary::Vector2D_Init1(const FVector2D& Host)
{
}

void UUPyEditorScriptExportHelperLibrary::Vector2D_Init2(const FVector2D& Host, double InX, double InY)
{
}

void UUPyEditorScriptExportHelperLibrary::Vector2D_Init3(const FVector2D& Host, const FVector2D& InV)
{
}

FRotator UUPyEditorScriptExportHelperLibrary::Rotator_ZeroRotator(const FRotator& Host)
{
	return FRotator::ZeroRotator;
}

FRotator UUPyEditorScriptExportHelperLibrary::Rotator_Add(const FRotator& Host, const FRotator& R)
{
	return Host + R;
}

FRotator UUPyEditorScriptExportHelperLibrary::Rotator_Subtract(const FRotator& Host, const FRotator& R)
{
	return Host - R;
}

FRotator UUPyEditorScriptExportHelperLibrary::Rotator_Multiply(const FRotator& Host, double Scale)
{
	return Host * Scale;
}

bool UUPyEditorScriptExportHelperLibrary::Rotator_Equal(const FRotator& Host, const FRotator& R)
{
	return Host == R;
}

bool UUPyEditorScriptExportHelperLibrary::Rotator_NotEqual(const FRotator& Host, const FRotator& R)
{
	return Host != R;
}

bool UUPyEditorScriptExportHelperLibrary::Rotator_IsNearlyZero(const FRotator& Host, double Tolerance)
{
	return Host.IsNearlyZero(Tolerance);
}

bool UUPyEditorScriptExportHelperLibrary::Rotator_IsZero(const FRotator& Host)
{
	return Host.IsZero();
}

FVector UUPyEditorScriptExportHelperLibrary::Rotator_Vector(const FRotator& Host)
{
	return Host.Vector();
}

FRotator UUPyEditorScriptExportHelperLibrary::Rotator_GetInverse(const FRotator& Host)
{
	return Host.GetInverse();
}

FVector UUPyEditorScriptExportHelperLibrary::Rotator_RotateVector(const FRotator& Host, const FVector& V)
{
	return Host.RotateVector(V);
}

FVector UUPyEditorScriptExportHelperLibrary::Rotator_UnrotateVector(const FRotator& Host, const FVector& V)
{
	return Host.UnrotateVector(V);
}

FRotator UUPyEditorScriptExportHelperLibrary::Rotator_Clamp(const FRotator& Host)
{
	return Host.Clamp();
}

FRotator UUPyEditorScriptExportHelperLibrary::Rotator_GetNormalized(const FRotator& Host)
{
	return Host.GetNormalized();
}

void UUPyEditorScriptExportHelperLibrary::Rotator_Init1(const FRotator& Host)
{
}

void UUPyEditorScriptExportHelperLibrary::Rotator_Init2(const FRotator& Host, double InF)
{
}

void UUPyEditorScriptExportHelperLibrary::Rotator_Init3(const FRotator& Host, double InPitch, double InYaw, double InRoll)
{
}

FVector UUPyEditorScriptExportHelperLibrary::Transform_GetLocation(const FTransform& InTrans)
{
	return FVector::ZeroVector;
}

void UUPyEditorScriptExportHelperLibrary::Transform_SetLocation(const FTransform& InTrans, const FVector& InLocation)
{
}

void UUPyEditorScriptExportHelperLibrary::Transform_AddToTranslation(FTransform& Host, const FVector& DeltaTranslation)
{
	Host.AddToTranslation(DeltaTranslation);
}

FRotator UUPyEditorScriptExportHelperLibrary::Transform_GetRotation(const FTransform& InTrans)
{
	return InTrans.Rotator();
}

FVector UUPyEditorScriptExportHelperLibrary::Transform_GetScale3D(const FTransform& InTrans)
{
	return FVector::ZeroVector;
}

void UUPyEditorScriptExportHelperLibrary::Transform_SetScale3D(const FTransform& InTrans, const FVector& InScale3D)
{
}

FTransform UUPyEditorScriptExportHelperLibrary::Transform_Identity(const FTransform& A)
{
	return FTransform::Identity;
}

FTransform UUPyEditorScriptExportHelperLibrary::Transform_Inverse(const FTransform& InTrans)
{
	return InTrans.Inverse();
}

void UUPyEditorScriptExportHelperLibrary::Transform_Blend(const FTransform& Host, const FTransform& Atom1, const FTransform& Atom2, float Alpha)
{

}

void UUPyEditorScriptExportHelperLibrary::Transform_BlendWith(const FTransform& Atom1, const FTransform& Atom2, float Alpha)
{

}

FTransform UUPyEditorScriptExportHelperLibrary::Transform_Multiply(const FTransform& A, const FTransform& B)
{
	return A * B;
}

FVector UUPyEditorScriptExportHelperLibrary::Transform_TransformPosition(const FTransform& InTrans, const FVector& V)
{
	return InTrans.TransformPosition(V);
}

FVector UUPyEditorScriptExportHelperLibrary::Transform_TransformPositionNoScale(const FTransform& InTrans, const FVector& V)
{
	return InTrans.TransformPositionNoScale(V);
}

FVector UUPyEditorScriptExportHelperLibrary::Transform_InverseTransformPosition(const FTransform& InTrans, const FVector& V)
{
	return InTrans.InverseTransformPosition(V);
}

FVector UUPyEditorScriptExportHelperLibrary::Transform_InverseTransformPositionNoScale(const FTransform& InTrans, const FVector& V)
{
	return InTrans.InverseTransformPositionNoScale(V);
}

FVector UUPyEditorScriptExportHelperLibrary::Transform_TransformVector(const FTransform& InTrans, const FVector& V)
{
	return InTrans.TransformVector(V);
}

FVector UUPyEditorScriptExportHelperLibrary::Transform_TransformVectorNoScale(const FTransform& InTrans, const FVector& V)
{
	return InTrans.TransformVectorNoScale(V);
}

FVector UUPyEditorScriptExportHelperLibrary::Transform_InverseTransformVector(const FTransform& InTrans, const FVector& V)
{
	return InTrans.InverseTransformVector(V);
}

FVector UUPyEditorScriptExportHelperLibrary::Transform_InverseTransformVectorNoScale(const FTransform& InTrans, const FVector& V)
{
	return InTrans.InverseTransformVectorNoScale(V);
}

FTransform UUPyEditorScriptExportHelperLibrary::Transform_GetRelativeTransform(const FTransform& A, const FTransform& B)
{
	return A.GetRelativeTransform(B);
}

FTransform UUPyEditorScriptExportHelperLibrary::Transform_GetRelativeTransformReverse(const FTransform& A, const FTransform& B)
{
	return A.GetRelativeTransformReverse(B);
}

void UUPyEditorScriptExportHelperLibrary::Transform_SetToRelativeTransform(FTransform& A, const FTransform& ParentTransform)
{
	A.SetToRelativeTransform(ParentTransform);
}

void UUPyEditorScriptExportHelperLibrary::Transform_NormalizeRotation(FTransform& InTrans)
{
	InTrans.NormalizeRotation();
}

bool UUPyEditorScriptExportHelperLibrary::Transform_IsRotationNormalized(const FTransform& InTrans)
{
	return InTrans.IsRotationNormalized();
}

double UUPyEditorScriptExportHelperLibrary::Transform_GetDeterminant(const FTransform& InTrans)
{
	return InTrans.GetDeterminant();
}

void UUPyEditorScriptExportHelperLibrary::Transform_Init1(const FTransform& InTrans)
{
}

void UUPyEditorScriptExportHelperLibrary::Transform_Init2(const FTransform& InTrans, const FVector& InTranslation)
{
}

void UUPyEditorScriptExportHelperLibrary::Transform_Init3(const FTransform& InTrans, const FRotator& InRotation)
{
}

void UUPyEditorScriptExportHelperLibrary::Transform_Init4(const FTransform& InTrans, const FRotator& InRotation, const FVector& InTranslation, const FVector& InScale3D)
{
}

FQuat UUPyEditorScriptExportHelperLibrary::Quat_Identity(const FQuat& Host)
{
	return FQuat::Identity;
}

FQuat UUPyEditorScriptExportHelperLibrary::Quat_Add(const FQuat& Host, const FQuat& Q)
{
	return Host + Q;
}

FQuat UUPyEditorScriptExportHelperLibrary::Quat_Subtract(const FQuat& Host, const FQuat& Q)
{
	return Host - Q;
}

FQuat UUPyEditorScriptExportHelperLibrary::Quat_Multiply(const FQuat& Host, const FQuat& Q)
{
	return Host * Q;
}

FVector UUPyEditorScriptExportHelperLibrary::Quat_MultiplyVector(const FQuat& Host, const FVector& V)
{
	return Host * V;
}

bool UUPyEditorScriptExportHelperLibrary::Quat_Equals(const FQuat& Host, const FQuat& Q, double Tolerance)
{
	return Host.Equals(Q, Tolerance);
}

bool UUPyEditorScriptExportHelperLibrary::Quat_IsIdentity(const FQuat& Host, double Tolerance)
{
	return Host.IsIdentity(Tolerance);
}

FVector UUPyEditorScriptExportHelperLibrary::Quat_Euler(const FQuat& Host)
{
	return Host.Euler();
}

void UUPyEditorScriptExportHelperLibrary::Quat_Normalize(FQuat& Host, double Tolerance)
{
	Host.Normalize(Tolerance);
}

FQuat UUPyEditorScriptExportHelperLibrary::Quat_GetNormalized(const FQuat& Host, double Tolerance)
{
	return Host.GetNormalized(Tolerance);
}

bool UUPyEditorScriptExportHelperLibrary::Quat_IsNormalized(const FQuat& Host)
{
	return Host.IsNormalized();
}

double UUPyEditorScriptExportHelperLibrary::Quat_Size(const FQuat& Host)
{
	return Host.Size();
}

double UUPyEditorScriptExportHelperLibrary::Quat_SizeSquared(const FQuat& Host)
{
	return Host.SizeSquared();
}

double UUPyEditorScriptExportHelperLibrary::Quat_GetAngle(const FQuat& Host)
{
	return Host.GetAngle();
}

void UUPyEditorScriptExportHelperLibrary::Quat_ToAxisAndAngle(const FQuat& Host, FVector& Axis, double& Angle)
{
	Host.ToAxisAndAngle(Axis, Angle);
}

FVector UUPyEditorScriptExportHelperLibrary::Quat_GetRotationAxis(const FQuat& Host)
{
	return Host.GetRotationAxis();
}

FVector UUPyEditorScriptExportHelperLibrary::Quat_RotateVector(const FQuat& Host, const FVector& V)
{
	return Host.RotateVector(V);
}

FVector UUPyEditorScriptExportHelperLibrary::Quat_UnrotateVector(const FQuat& Host, const FVector& V)
{
	return Host.UnrotateVector(V);
}

FQuat UUPyEditorScriptExportHelperLibrary::Quat_Inverse(const FQuat& Host)
{
	return Host.Inverse();
}

FVector UUPyEditorScriptExportHelperLibrary::Quat_GetAxisX(const FQuat& Host)
{
	return Host.GetAxisX();
}

FVector UUPyEditorScriptExportHelperLibrary::Quat_GetAxisY(const FQuat& Host)
{
	return Host.GetAxisY();
}

FVector UUPyEditorScriptExportHelperLibrary::Quat_GetAxisZ(const FQuat& Host)
{
	return Host.GetAxisZ();
}

FVector UUPyEditorScriptExportHelperLibrary::Quat_GetForwardVector(const FQuat& Host)
{
	return Host.GetForwardVector();
}

FVector UUPyEditorScriptExportHelperLibrary::Quat_GetRightVector(const FQuat& Host)
{
	return Host.GetRightVector();
}

FVector UUPyEditorScriptExportHelperLibrary::Quat_GetUpVector(const FQuat& Host)
{
	return Host.GetUpVector();
}

FVector UUPyEditorScriptExportHelperLibrary::Quat_Vector(const FQuat& Host)
{
	return Host.Vector();
}

FRotator UUPyEditorScriptExportHelperLibrary::Quat_Rotator(const FQuat& Host)
{
	return Host.Rotator();
}

FMatrix UUPyEditorScriptExportHelperLibrary::Quat_ToMatrix(const FQuat& Host)
{
	return Host.ToMatrix();
}

double UUPyEditorScriptExportHelperLibrary::Quat_GetTwistAngle(const FQuat& Host, const FVector& TwistAxis)
{
	return Host.GetTwistAngle(TwistAxis);
}

void UUPyEditorScriptExportHelperLibrary::Quat_ToSwingTwist(const FQuat& Host, const FVector& InTwistAxis, FQuat& OutSwing, FQuat& OutTwist)
{
	Host.ToSwingTwist(InTwistAxis, OutSwing, OutTwist);
}

double UUPyEditorScriptExportHelperLibrary::Quat_AngularDistance(const FQuat& Host, const FQuat& Q)
{
	return Host.AngularDistance(Q);
}

FQuat UUPyEditorScriptExportHelperLibrary::Quat_MakeFromEuler(const FQuat& Host, const FVector& Euler)
{
	return FQuat::MakeFromEuler(Euler);
}

FQuat UUPyEditorScriptExportHelperLibrary::Quat_FindBetween(const FQuat& Host, const FVector& Vector1, const FVector& Vector2)
{
	return FQuat::FindBetween(Vector1, Vector2);
}

FQuat UUPyEditorScriptExportHelperLibrary::Quat_FindBetweenNormals(const FQuat& Host, const FVector& Normal1, const FVector& Normal2)
{
	return FQuat::FindBetweenNormals(Normal1, Normal2);
}

FQuat UUPyEditorScriptExportHelperLibrary::Quat_FindBetweenVectors(const FQuat& Host, const FVector& Vector1, const FVector& Vector2)
{
	return FQuat::FindBetweenVectors(Vector1, Vector2);
}

double UUPyEditorScriptExportHelperLibrary::Quat_Error(const FQuat& Host, const FQuat& Q1, const FQuat& Q2)
{
	return FQuat::Error(Q1, Q2);
}

double UUPyEditorScriptExportHelperLibrary::Quat_ErrorAutoNormalize(const FQuat& Host, const FQuat& A, const FQuat& B)
{
	return FQuat::ErrorAutoNormalize(A, B);
}

FQuat UUPyEditorScriptExportHelperLibrary::Quat_FastLerp(const FQuat& Host, const FQuat& A, const FQuat& B, const double Alpha)
{
	return FQuat::FastLerp(A, B, Alpha);
}

FQuat UUPyEditorScriptExportHelperLibrary::Quat_FastBilerp(const FQuat& Host, const FQuat& P00, const FQuat& P10, const FQuat& P01, const FQuat& P11, double FracX, double FracY)
{
	return FQuat::FastBilerp(P00, P10, P01, P11, FracX, FracY);
}

FQuat UUPyEditorScriptExportHelperLibrary::Quat_Slerp(const FQuat& Host, const FQuat& Quat1, const FQuat& Quat2, double Slerp)
{
	return FQuat::Slerp(Quat1, Quat2, Slerp);
}

FQuat UUPyEditorScriptExportHelperLibrary::Quat_Slerp_NotNormalized(const FQuat& Host, const FQuat& Quat1, const FQuat& Quat2, double Slerp)
{
	return FQuat::Slerp_NotNormalized(Quat1, Quat2, Slerp);
}

FQuat UUPyEditorScriptExportHelperLibrary::Quat_SlerpFullPath(const FQuat& Host, const FQuat& quat1, const FQuat& quat2, double Alpha)
{
	return FQuat::SlerpFullPath(quat1, quat2, Alpha);
}

FQuat UUPyEditorScriptExportHelperLibrary::Quat_SlerpFullPath_NotNormalized(const FQuat& Host, const FQuat& quat1, const FQuat& quat2, double Alpha)
{
	return FQuat::SlerpFullPath_NotNormalized(quat1, quat2, Alpha);
}

FQuat UUPyEditorScriptExportHelperLibrary::Quat_Squad(const FQuat& Host, const FQuat& quat1, const FQuat& tang1, const FQuat& quat2, const FQuat& tang2, double Alpha)
{
	return FQuat::Squad(quat1, tang1, quat2, tang2, Alpha);
}

FQuat UUPyEditorScriptExportHelperLibrary::Quat_SquadFullPath(const FQuat& Host, const FQuat& quat1, const FQuat& tang1, const FQuat& quat2, const FQuat& tang2, double Alpha)
{
	return FQuat::SquadFullPath(quat1, tang1, quat2, tang2, Alpha);
}

void UUPyEditorScriptExportHelperLibrary::Quat_CalcTangents(const FQuat& Host, const FQuat& PrevP, const FQuat& P, const FQuat& NextP, double Tension, FQuat& OutTan)
{
	FQuat::CalcTangents(PrevP, P, NextP, Tension, OutTan);
}

FQuat UUPyEditorScriptExportHelperLibrary::Quat_Log(const FQuat& Host)
{
	return Host.Log();
}

FQuat UUPyEditorScriptExportHelperLibrary::Quat_Exp(const FQuat& Host)
{
	return Host.Exp();
}

void UUPyEditorScriptExportHelperLibrary::Quat_Init1(const FQuat& Host)
{
}

void UUPyEditorScriptExportHelperLibrary::Quat_Init2(const FQuat& Host, const FVector& Axis, double AngleRad)
{
}

void UUPyEditorScriptExportHelperLibrary::Quat_Init3(const FQuat& Host, const FMatrix& M)
{
}

void UUPyEditorScriptExportHelperLibrary::Quat_Init4(const FQuat& Host, const FRotator& R)
{
}

void UUPyEditorScriptExportHelperLibrary::Quat_Init5(const FQuat& Host, double InX, double InY, double InZ, double InW)
{
}

FMatrix UUPyEditorScriptExportHelperLibrary::Matrix_Identity(const FMatrix& Host)
{
	return FMatrix::Identity;
}

FMatrix UUPyEditorScriptExportHelperLibrary::Matrix_Multiply(const FMatrix& Host, const FMatrix& Other)
{
	return FMatrix::Identity;
}

FMatrix UUPyEditorScriptExportHelperLibrary::Matrix_Add(const FMatrix& Host, const FMatrix& Other)
{
	return FMatrix::Identity;
}

FMatrix UUPyEditorScriptExportHelperLibrary::Matrix_MultiplyFloat(const FMatrix& Host, float Scale)
{
	return FMatrix::Identity;
}

FVector UUPyEditorScriptExportHelperLibrary::Matrix_TransformPosition(const FMatrix& Host, const FVector& V)
{
	return FVector::ZeroVector;
}

FVector UUPyEditorScriptExportHelperLibrary::Matrix_InverseTransformPosition(const FMatrix& Host, const FVector& V)
{
	return FVector::ZeroVector;
}

FVector UUPyEditorScriptExportHelperLibrary::Matrix_TransformVector(const FMatrix& Host, const FVector& V)
{
	return FVector::ZeroVector;
}

FVector UUPyEditorScriptExportHelperLibrary::Matrix_InverseTransformVector(const FMatrix& Host, const FVector& V)
{
	return FVector::ZeroVector;
}

FMatrix UUPyEditorScriptExportHelperLibrary::Matrix_GetTransposed(const FMatrix& Host)
{
	return FMatrix::Identity;
}

double UUPyEditorScriptExportHelperLibrary::Matrix_Determinant(const FMatrix& Host)
{
	return 0.0;
}

double UUPyEditorScriptExportHelperLibrary::Matrix_RotDeterminant(const FMatrix& Host)
{
	return 0.0;
}

FMatrix UUPyEditorScriptExportHelperLibrary::Matrix_InverseFast(const FMatrix& Host)
{
	return FMatrix::Identity;
}

FMatrix UUPyEditorScriptExportHelperLibrary::Matrix_Inverse(const FMatrix& Host)
{
	return FMatrix::Identity;
}

FMatrix UUPyEditorScriptExportHelperLibrary::Matrix_TransposeAdjoint(const FMatrix& Host)
{
	return FMatrix::Identity;
}

void UUPyEditorScriptExportHelperLibrary::Matrix_RemoveScaling(FMatrix& Host, double Tolerance)
{
}

FMatrix UUPyEditorScriptExportHelperLibrary::Matrix_GetMatrixWithoutScale(const FMatrix& Host, double Tolerance)
{
	return FMatrix::Identity;
}

FVector UUPyEditorScriptExportHelperLibrary::Matrix_GetScaleVector(const FMatrix& Host, double Tolerance)
{
	return FVector::ZeroVector;
}

FVector UUPyEditorScriptExportHelperLibrary::Matrix_GetScaledAxis(const FMatrix& Host, EAxis::Type Axis)
{
	return FVector::ZeroVector;
}

FVector UUPyEditorScriptExportHelperLibrary::Matrix_GetUnitAxis(const FMatrix& Host, EAxis::Type Axis)
{
	return FVector::ZeroVector;
}

void UUPyEditorScriptExportHelperLibrary::Matrix_SetAxis(FMatrix& Host, EAxis::Type Axis, const FVector& Value)
{
}

void UUPyEditorScriptExportHelperLibrary::Matrix_SetOrigin(FMatrix& Host, const FVector& NewOrigin)
{
}

FVector UUPyEditorScriptExportHelperLibrary::Matrix_GetOrigin(const FMatrix& Host)
{
	return FVector::ZeroVector;
}

FVector UUPyEditorScriptExportHelperLibrary::Matrix_GetColumn(const FMatrix& Host, int32 Index)
{
	return FVector::ZeroVector;
}

void UUPyEditorScriptExportHelperLibrary::Matrix_SetColumn(FMatrix& Host, int32 Index, const FVector& Value)
{
}

FRotator UUPyEditorScriptExportHelperLibrary::Matrix_Rotator(const FMatrix& Host)
{
	return FRotator::ZeroRotator;
}

FQuat UUPyEditorScriptExportHelperLibrary::Matrix_ToQuat(const FMatrix& Host)
{
	return FQuat::Identity;
}

FString UUPyEditorScriptExportHelperLibrary::Matrix_ToString(const FMatrix& Host)
{
	return FString();
}

void UUPyEditorScriptExportHelperLibrary::Matrix_Init1(const FMatrix& Host)
{
}

void UUPyEditorScriptExportHelperLibrary::Matrix_Init2(const FMatrix& Host, const FVector& InX, const FVector& InY, const FVector& InZ, const FVector& InW)
{
}

FColor UUPyEditorScriptExportHelperLibrary::Color_White(const FColor& Host)
{
	return FColor::White;
}

FColor UUPyEditorScriptExportHelperLibrary::Color_Black(const FColor& Host)
{
	return FColor::Black;
}

FColor UUPyEditorScriptExportHelperLibrary::Color_Transparent(const FColor& Host)
{
	return FColor::Transparent;
}

FColor UUPyEditorScriptExportHelperLibrary::Color_Red(const FColor& Host)
{
	return FColor::Red;
}

FColor UUPyEditorScriptExportHelperLibrary::Color_Green(const FColor& Host)
{
	return FColor::Green;
}

FColor UUPyEditorScriptExportHelperLibrary::Color_Blue(const FColor& Host)
{
	return FColor::Blue;
}

FColor UUPyEditorScriptExportHelperLibrary::Color_Yellow(const FColor& Host)
{
	return FColor::Yellow;
}

FColor UUPyEditorScriptExportHelperLibrary::Color_Cyan(const FColor& Host)
{
	return FColor::Cyan;
}

FColor UUPyEditorScriptExportHelperLibrary::Color_Magenta(const FColor& Host)
{
	return FColor::Magenta;
}

FColor UUPyEditorScriptExportHelperLibrary::Color_Orange(const FColor& Host)
{
	return FColor::Orange;
}

FColor UUPyEditorScriptExportHelperLibrary::Color_Purple(const FColor& Host)
{
	return FColor::Purple;
}

FColor UUPyEditorScriptExportHelperLibrary::Color_Turquoise(const FColor& Host)
{
	return FColor::Turquoise;
}

FColor UUPyEditorScriptExportHelperLibrary::Color_Silver(const FColor& Host)
{
	return FColor::Silver;
}

FColor UUPyEditorScriptExportHelperLibrary::Color_Emerald(const FColor& Host)
{
	return FColor::Emerald;
}

void UUPyEditorScriptExportHelperLibrary::Color_Init1(const FColor& Host)
{
}

void UUPyEditorScriptExportHelperLibrary::Color_Init2(const FColor& Host, uint8 InR, uint8 InG, uint8 InB, uint8 InA)
{
}

void UUPyEditorScriptExportHelperLibrary::Color_Init3(const FColor& Host, int32 InColor)
{
}

FColor UUPyEditorScriptExportHelperLibrary::Color_Add(const FColor& Host, const FColor& C)
{
	return FColor();
}

bool UUPyEditorScriptExportHelperLibrary::Color_Equal(const FColor& Host, const FColor& C)
{
	return Host == C;
}

bool UUPyEditorScriptExportHelperLibrary::Color_NotEqual(const FColor& Host, const FColor& C)
{
	return Host != C;
}

FColor UUPyEditorScriptExportHelperLibrary::Color_FromHex(const FColor& Host, const FString& HexString)
{
	return FColor::FromHex(HexString);
}

FColor UUPyEditorScriptExportHelperLibrary::Color_MakeRandomColor(const FColor& Host)
{
	return FColor::MakeRandomColor();
}

FColor UUPyEditorScriptExportHelperLibrary::Color_MakeRedToGreenColorFromScalar(const FColor& Host, float Scalar)
{
	return FColor::MakeRedToGreenColorFromScalar(Scalar);
}

FColor UUPyEditorScriptExportHelperLibrary::Color_MakeFromColorTemperature(const FColor& Host, float Temp)
{
	return FColor::MakeFromColorTemperature(Temp);
}

FColor UUPyEditorScriptExportHelperLibrary::Color_WithAlpha(const FColor& Host, uint8 Alpha)
{
	return Host.WithAlpha(Alpha);
}

FString UUPyEditorScriptExportHelperLibrary::Color_ToHex(const FColor& Host)
{
	return Host.ToHex();
}

FString UUPyEditorScriptExportHelperLibrary::Color_ToString(const FColor& Host)
{
	return Host.ToString();
}

int32 UUPyEditorScriptExportHelperLibrary::Color_ToPackedARGB(const FColor& Host)
{
	return Host.ToPackedARGB();
}

int32 UUPyEditorScriptExportHelperLibrary::Color_ToPackedABGR(const FColor& Host)
{
	return Host.ToPackedABGR();
}

int32 UUPyEditorScriptExportHelperLibrary::Color_ToPackedRGBA(const FColor& Host)
{
	return Host.ToPackedRGBA();
}

int32 UUPyEditorScriptExportHelperLibrary::Color_ToPackedBGRA(const FColor& Host)
{
	return Host.ToPackedBGRA();
}

FLinearColor UUPyEditorScriptExportHelperLibrary::LinearColor_White(const FLinearColor& Host)
{
	return FLinearColor::White;
}

FLinearColor UUPyEditorScriptExportHelperLibrary::LinearColor_Black(const FLinearColor& Host)
{
	return FLinearColor::Black;
}

FLinearColor UUPyEditorScriptExportHelperLibrary::LinearColor_Gray(const FLinearColor& Host)
{
	return FLinearColor::Gray;
}

FLinearColor UUPyEditorScriptExportHelperLibrary::LinearColor_Red(const FLinearColor& Host)
{
	return FLinearColor::Red;
}

FLinearColor UUPyEditorScriptExportHelperLibrary::LinearColor_Green(const FLinearColor& Host)
{
	return FLinearColor::Green;
}

FLinearColor UUPyEditorScriptExportHelperLibrary::LinearColor_Blue(const FLinearColor& Host)
{
	return FLinearColor::Blue;
}

FLinearColor UUPyEditorScriptExportHelperLibrary::LinearColor_Yellow(const FLinearColor& Host)
{
	return FLinearColor::Yellow;
}

FLinearColor UUPyEditorScriptExportHelperLibrary::LinearColor_Transparent(const FLinearColor& Host)
{
	return FLinearColor::Transparent;
}

FLinearColor UUPyEditorScriptExportHelperLibrary::LinearColor_Add(const FLinearColor& Host, const FLinearColor& Color)
{
	return Host + Color;
}

FLinearColor UUPyEditorScriptExportHelperLibrary::LinearColor_Subtract(const FLinearColor& Host, const FLinearColor& Color)
{
	return Host - Color;
}

FLinearColor UUPyEditorScriptExportHelperLibrary::LinearColor_Multiply(const FLinearColor& Host, const FLinearColor& Color)
{
	return Host * Color;
}

FLinearColor UUPyEditorScriptExportHelperLibrary::LinearColor_MultiplyFloat(const FLinearColor& Host, float Scalar)
{
	return Host * Scalar;
}

FLinearColor UUPyEditorScriptExportHelperLibrary::LinearColor_Divide(const FLinearColor& Host, const FLinearColor& Color)
{
	return Host / Color;
}

FLinearColor UUPyEditorScriptExportHelperLibrary::LinearColor_DivideFloat(const FLinearColor& Host, float Scalar)
{
	return Host / Scalar;
}

bool UUPyEditorScriptExportHelperLibrary::LinearColor_Equal(const FLinearColor& Host, const FLinearColor& Color)
{
	return Host == Color;
}

bool UUPyEditorScriptExportHelperLibrary::LinearColor_NotEqual(const FLinearColor& Host, const FLinearColor& Color)
{
	return Host != Color;
}

FLinearColor UUPyEditorScriptExportHelperLibrary::LinearColor_FromSRGBColor(const FLinearColor& Host, const FColor& Color)
{
	return FLinearColor::FromSRGBColor(Color);
}

FColor UUPyEditorScriptExportHelperLibrary::LinearColor_ToFColor(const FLinearColor& Host, bool bSRGB)
{
	return Host.ToFColor(bSRGB);
}

FLinearColor UUPyEditorScriptExportHelperLibrary::LinearColor_GetClamped(const FLinearColor& Host)
{
	return Host.GetClamped();
}

FLinearColor UUPyEditorScriptExportHelperLibrary::LinearColor_Desaturate(const FLinearColor& Host, float Desaturation)
{
	return Host.Desaturate(Desaturation);
}

float UUPyEditorScriptExportHelperLibrary::LinearColor_GetLuminance(const FLinearColor& Host)
{
	return Host.GetLuminance();
}

bool UUPyEditorScriptExportHelperLibrary::LinearColor_IsAlmostBlack(const FLinearColor& Host)
{
	return Host.IsAlmostBlack();
}

bool UUPyEditorScriptExportHelperLibrary::LinearColor_Equals(const FLinearColor& Host, const FLinearColor& Color, float Tolerance)
{
	return Host.Equals(Color, Tolerance);
}

float UUPyEditorScriptExportHelperLibrary::LinearColor_Dist(const FLinearColor& Host, const FLinearColor& C1, const FLinearColor& C2)
{
	return FLinearColor::Dist(C1, C2);
}

void UUPyEditorScriptExportHelperLibrary::LinearColor_Init1(const FLinearColor& Host)
{
}

void UUPyEditorScriptExportHelperLibrary::LinearColor_Init2(const FLinearColor& Host, float R, float G, float B, float A)
{
}

void UUPyEditorScriptExportHelperLibrary::LinearColor_Init3(const FLinearColor& Host, const FColor& Color)
{
}

void UUPyEditorScriptExportHelperLibrary::LinearColor_Init4(const FLinearColor& Host, const FVector& Vector)
{
}

void UUPyEditorScriptExportHelperLibrary::SlateColor_Init1(const FSlateColor& Host)
{
}

void UUPyEditorScriptExportHelperLibrary::SlateColor_Init2(const FSlateColor& Host, const FLinearColor& InColor)
{
}

void UUPyEditorScriptExportHelperLibrary::SlateColor_Init3(const FSlateColor& Host, const FColor& InColor)
{
}

void UUPyEditorScriptExportHelperLibrary::SlateColor_Init4(const FSlateColor& Host, EStyleColor InStyleColor)
{
}

FLinearColor UUPyEditorScriptExportHelperLibrary::SlateColor_GetSpecifiedColor(const FSlateColor& Host)
{
	return FLinearColor::White;
}

bool UUPyEditorScriptExportHelperLibrary::SlateColor_IsColorSpecified(const FSlateColor& Host)
{
	return false;
}

FSlateColor UUPyEditorScriptExportHelperLibrary::SlateColor_UseForeground(const FSlateColor& Host)
{
	return FSlateColor();
}

FSlateColor UUPyEditorScriptExportHelperLibrary::SlateColor_UseSubduedForeground(const FSlateColor& Host)
{
	return FSlateColor();
}

bool UUPyEditorScriptExportHelperLibrary::SlateColor_Equal(const FSlateColor& Host, const FSlateColor& Other)
{
	return false;
}

bool UUPyEditorScriptExportHelperLibrary::SlateColor_NotEqual(const FSlateColor& Host, const FSlateColor& Other)
{
	return false;
}

bool UUPyEditorScriptExportHelperLibrary::Key_IsValid(const FKey& Host)
{
	return Host.IsValid();
}

bool UUPyEditorScriptExportHelperLibrary::Key_IsModifierKey(const FKey& Host)
{
	return Host.IsModifierKey();
}

bool UUPyEditorScriptExportHelperLibrary::Key_IsGamepadKey(const FKey& Host)
{
	return Host.IsGamepadKey();
}

bool UUPyEditorScriptExportHelperLibrary::Key_IsMouseButton(const FKey& Host)
{
	return Host.IsMouseButton();
}

bool UUPyEditorScriptExportHelperLibrary::Key_IsTouch(const FKey& Host)
{
	return Host.IsTouch();
}

FText UUPyEditorScriptExportHelperLibrary::Key_GetDisplayName(const FKey& Host)
{
	return Host.GetDisplayName();
}

FName UUPyEditorScriptExportHelperLibrary::Key_GetFName(const FKey& Host)
{
	return Host.GetFName();
}

FString UUPyEditorScriptExportHelperLibrary::Key_ToString(const FKey& Host)
{
	return Host.ToString();
}

bool UUPyEditorScriptExportHelperLibrary::Key_Equal(const FKey& Host, const FKey& Other)
{
	return Host == Other;
}

bool UUPyEditorScriptExportHelperLibrary::Key_NotEqual(const FKey& Host, const FKey& Other)
{
	return Host != Other;
}

void UUPyEditorScriptExportHelperLibrary::Key_Init1(const FKey& Host)
{
}

void UUPyEditorScriptExportHelperLibrary::Key_Init2(const FKey& Host, FName InName)
{
}

bool UUPyEditorScriptExportHelperLibrary::Guid_Equal(const FGuid& Host, const FGuid& Other)
{
	return Host == Other;
}

bool UUPyEditorScriptExportHelperLibrary::Guid_NotEqual(const FGuid& Host, const FGuid& Other)
{
	return Host != Other;
}

bool UUPyEditorScriptExportHelperLibrary::Guid_Less(const FGuid& Host, const FGuid& Other)
{
	return Host < Other;
}

bool UUPyEditorScriptExportHelperLibrary::Guid_IsValid(const FGuid& Host)
{
	return Host.IsValid();
}

void UUPyEditorScriptExportHelperLibrary::Guid_Invalidate(FGuid& Host)
{
	Host.Invalidate();
}

FString UUPyEditorScriptExportHelperLibrary::Guid_ToString(const FGuid& Host)
{
	return Host.ToString();
}

FGuid UUPyEditorScriptExportHelperLibrary::Guid_NewGuid(const FGuid& Host)
{
	return FGuid::NewGuid();
}

bool UUPyEditorScriptExportHelperLibrary::Guid_Parse(const FGuid& Host, const FString& GuidString, FGuid& OutGuid)
{
	return FGuid::Parse(GuidString, OutGuid);
}

void UUPyEditorScriptExportHelperLibrary::Guid_Init1(const FGuid& Host)
{
}

void UUPyEditorScriptExportHelperLibrary::Guid_Init2(const FGuid& Host, int32 A, int32 B, int32 C, int32 D)
{
}

FVector2D UUPyEditorScriptExportHelperLibrary::Box2D_GetCenter(const FBox2D& Host)
{
	return Host.GetCenter();
}

FVector2D UUPyEditorScriptExportHelperLibrary::Box2D_GetSize(const FBox2D& Host)
{
	return Host.GetSize();
}

FVector2D UUPyEditorScriptExportHelperLibrary::Box2D_GetExtent(const FBox2D& Host)
{
	return Host.GetExtent();
}

double UUPyEditorScriptExportHelperLibrary::Box2D_GetArea(const FBox2D& Host)
{
	return Host.GetArea();
}

double UUPyEditorScriptExportHelperLibrary::Box2D_ComputeSquaredDistanceToPoint(const FBox2D& Host, const FVector2D& Point)
{
	return Host.ComputeSquaredDistanceToPoint(Point);
}

FBox2D UUPyEditorScriptExportHelperLibrary::Box2D_ExpandBy(const FBox2D& Host, double W)
{
	return Host.ExpandBy(W);
}

bool UUPyEditorScriptExportHelperLibrary::Box2D_IsInside(const FBox2D& Host, const FVector2D& TestPoint)
{
	return Host.IsInside(TestPoint);
}

bool UUPyEditorScriptExportHelperLibrary::Box2D_Intersect(const FBox2D& Host, const FBox2D& Other)
{
	return Host.Intersect(Other);
}

FBox2D UUPyEditorScriptExportHelperLibrary::Box2D_ShiftBy(const FBox2D& Host, const FVector2D& Offset)
{
	return Host + Offset;
}

FString UUPyEditorScriptExportHelperLibrary::Box2D_ToString(const FBox2D& Host)
{
	return Host.ToString();
}

bool UUPyEditorScriptExportHelperLibrary::Box2D_Equal(const FBox2D& Host, const FBox2D& Other)
{
	return Host == Other;
}

bool UUPyEditorScriptExportHelperLibrary::Box2D_NotEqual(const FBox2D& Host, const FBox2D& Other)
{
	return Host != Other;
}

void UUPyEditorScriptExportHelperLibrary::Box2D_Init1(const FBox2D& Host)
{
}

void UUPyEditorScriptExportHelperLibrary::Box2D_Init2(const FBox2D& Host, const FVector2D& InMin, const FVector2D& InMax)
{
}

FVector UUPyEditorScriptExportHelperLibrary::Box_GetCenter(const FBox& Host)
{
	return Host.GetCenter();
}

FVector UUPyEditorScriptExportHelperLibrary::Box_GetSize(const FBox& Host)
{
	return Host.GetSize();
}

FVector UUPyEditorScriptExportHelperLibrary::Box_GetExtent(const FBox& Host)
{
	return Host.GetExtent();
}

double UUPyEditorScriptExportHelperLibrary::Box_GetVolume(const FBox& Host)
{
	return Host.GetVolume();
}

double UUPyEditorScriptExportHelperLibrary::Box_ComputeSquaredDistanceToPoint(const FBox& Host, const FVector& Point)
{
	return Host.ComputeSquaredDistanceToPoint(Point);
}

FBox UUPyEditorScriptExportHelperLibrary::Box_ExpandBy(const FBox& Host, double W)
{
	return Host.ExpandBy(W);
}

bool UUPyEditorScriptExportHelperLibrary::Box_IsInside(const FBox& Host, const FVector& TestPoint)
{
	return Host.IsInside(TestPoint);
}

bool UUPyEditorScriptExportHelperLibrary::Box_Intersect(const FBox& Host, const FBox& Other)
{
	return Host.Intersect(Other);
}

FBox UUPyEditorScriptExportHelperLibrary::Box_ShiftBy(const FBox& Host, const FVector& Offset)
{
	return Host + Offset;
}

FBox UUPyEditorScriptExportHelperLibrary::Box_MoveTo(const FBox& Host, const FVector& Destination)
{
	return Host.MoveTo(Destination);
}

FString UUPyEditorScriptExportHelperLibrary::Box_ToString(const FBox& Host)
{
	return Host.ToString();
}

bool UUPyEditorScriptExportHelperLibrary::Box_Equal(const FBox& Host, const FBox& Other)
{
	return Host == Other;
}

bool UUPyEditorScriptExportHelperLibrary::Box_NotEqual(const FBox& Host, const FBox& Other)
{
	return Host != Other;
}

void UUPyEditorScriptExportHelperLibrary::Box_Init1(const FBox& Host)
{
}

void UUPyEditorScriptExportHelperLibrary::Box_Init2(const FBox& Host, const FVector& InMin, const FVector& InMax)
{
}

FString UUPyEditorScriptExportHelperLibrary::SoftObjectPath_ToString(const FSoftObjectPath& Host)
{
	return Host.ToString();
}

FString UUPyEditorScriptExportHelperLibrary::SoftObjectPath_GetAssetPathString(const FSoftObjectPath& Host)
{
	return Host.GetAssetPathString();
}

FString UUPyEditorScriptExportHelperLibrary::SoftObjectPath_GetLongPackageName(const FSoftObjectPath& Host)
{
	return Host.GetLongPackageName();
}

FString UUPyEditorScriptExportHelperLibrary::SoftObjectPath_GetAssetName(const FSoftObjectPath& Host)
{
	return Host.GetAssetName();
}

bool UUPyEditorScriptExportHelperLibrary::SoftObjectPath_IsValid(const FSoftObjectPath& Host)
{
	return Host.IsValid();
}

void UUPyEditorScriptExportHelperLibrary::SoftObjectPath_Reset(FSoftObjectPath& Host)
{
	Host.Reset();
}

bool UUPyEditorScriptExportHelperLibrary::SoftObjectPath_Equal(const FSoftObjectPath& Host, const FSoftObjectPath& Other)
{
	return Host == Other;
}

bool UUPyEditorScriptExportHelperLibrary::SoftObjectPath_NotEqual(const FSoftObjectPath& Host, const FSoftObjectPath& Other)
{
	return Host != Other;
}

void UUPyEditorScriptExportHelperLibrary::SoftObjectPath_Init1(const FSoftObjectPath& Host)
{
}

void UUPyEditorScriptExportHelperLibrary::SoftObjectPath_Init2(const FSoftObjectPath& Host, const FString& PathString)
{
}

void UUPyEditorScriptExportHelperLibrary::SoftObjectPath_Init3(const FSoftObjectPath& Host, const UObject* InObject)
{
}

void UUPyEditorScriptExportHelperLibrary::SoftClassPath_Init1(const FSoftClassPath& Host)
{
}

void UUPyEditorScriptExportHelperLibrary::SoftClassPath_Init2(const FSoftClassPath& Host, const FString& PathString)
{
}

void UUPyEditorScriptExportHelperLibrary::SoftClassPath_Init3(const FSoftClassPath& Host, const UClass* InClass)
{
}

USkeletalMeshComponent* UUPyEditorScriptExportHelperLibrary::Character_GetMesh(ACharacter* Host)
{
	return nullptr;
}

UShapeComponent* UUPyEditorScriptExportHelperLibrary::TriggerBase_GetCollisionComponent(ATriggerBase* Host)
{
	return nullptr;
}

float UUPyEditorScriptExportHelperLibrary::ProgressBar_GetPercent(UProgressBar* Host)
{
	return 0.0f;
}

EProgressBarFillStyle::Type UUPyEditorScriptExportHelperLibrary::ProgressBar_GetBarFillStyle(UProgressBar* Host)
{
	return EProgressBarFillStyle::Mask;
}

void UUPyEditorScriptExportHelperLibrary::ProgressBar_SetBarFillStyle(UProgressBar* Host, EProgressBarFillStyle::Type InBarFillStyle)
{
}

EProgressBarFillType::Type UUPyEditorScriptExportHelperLibrary::ProgressBar_GetBarFillType(UProgressBar* Host)
{
	return EProgressBarFillType::LeftToRight;
}

void UUPyEditorScriptExportHelperLibrary::ProgressBar_SetBarFillType(UProgressBar* Host, EProgressBarFillType::Type InBarFillType)
{
}

FVector2D UUPyEditorScriptExportHelperLibrary::ProgressBar_GetBorderPadding(UProgressBar* Host)
{
	return FVector2D::ZeroVector;
}

void UUPyEditorScriptExportHelperLibrary::ProgressBar_SetBorderPadding(UProgressBar* Host, FVector2D InBorderPadding)
{
}

FProgressBarStyle UUPyEditorScriptExportHelperLibrary::ProgressBar_GetWidgetStyle(UProgressBar* Host)
{
	return FProgressBarStyle();
}

void UUPyEditorScriptExportHelperLibrary::ProgressBar_SetWidgetStyle(UProgressBar* Host, FProgressBarStyle InWidgetStyle)
{
}

bool UUPyEditorScriptExportHelperLibrary::ProgressBar_UseMarquee(UProgressBar* Host)
{
	return false;
}

FText UUPyEditorScriptExportHelperLibrary::EditableTextBox_GetHintText(UEditableTextBox* Host)
{
	return FText::GetEmpty();
}


UGameViewportSubsystem* UUPyEditorScriptExportHelperLibrary::GameViewportSubsystem_Get(UGameViewportSubsystem* Host)
{
	return nullptr;
}

const FWidgetTransform& UUPyEditorScriptExportHelperLibrary::Widget_GetRenderTransform(UWidget* Host)
{
	return Host->GetRenderTransform();
}

#endif
