
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Components/ProgressBar.h"
#include "Styling/SlateColor.h"
#include "Styling/StyleColors.h"
#include "Styling/SlateTypes.h"
#include "UPyEditorScriptExportHelperLibrary.generated.h"

class ATriggerBase;
class USkeletalMeshComponent;
class ACharacter;
class UEditableTextBox;
class UWidget;

// Only use for static script export helper functions. DO NOT write runtime code here.
UCLASS()
class UUPyEditorScriptExportHelperLibrary: public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

#if WITH_EDITOR
	// ============================================
	// FVector exposed for scripting
	// ============================================

	/** A zero vector (0,0,0) */
	UFUNCTION(BlueprintPure, meta = (UPyScriptConstant = "ZeroVector"))
	static FVector Vector_ZeroVector(const FVector& Host);

	/** One vector (1,1,1) */
	UFUNCTION(BlueprintPure, meta = (UPyScriptConstant = "OneVector"))
	static FVector Vector_OneVector(const FVector& Host);

	/**
	 * A zero vector (0,0,1)
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptConstant = "UpVector"))
	static FVector Vector_UpVector(const FVector& Host);

	/**
	 * A zero vector (1,0,0)
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptConstant = "ForwardVector"))
	static FVector Vector_ForwardVector(const FVector& Host);

	/**
	 * A zero vector (0,1,0)
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptConstant = "RightVector"))
	static FVector Vector_RightVector(const FVector& Host);


	/**
	 * Calculate cross product between this and another vector.
	 *
	 * @param V The other vector.
	 * @return The cross product.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Cross", UPyScriptOperator = "^"))
	static FVector Vector_Cross(const FVector& Host, const FVector& V);

	/**
	 * Calculate the cross product of two vectors.
	 *
	 * @param A The first vector.
	 * @param B The second vector.
	 * @return The cross product.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "CrossProduct", UPyScriptStatic))
	static FVector Vector_CrossProduct(const FVector& Host, const FVector& A, const FVector& B);

	/**
	 * Calculate the dot product between this and another vector.
	 *
	 * @param V The other vector.
	 * @return The dot product.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Dot", UPyScriptOperator = "|"))
	static double Vector_Dot(const FVector& Host, const FVector& V);

	/**
	 * Calculate the dot product of two vectors.
	 *
	 * @param A The first vector.
	 * @param B The second vector.
	 * @return The dot product.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "DotProduct", UPyScriptStatic))
	static double Vector_DotProduct(const FVector& Host, const FVector& A, const FVector& B);

	/**
	 * Gets the result of component-wise addition of this and another vector.
	 *
	 * @param V The vector to add to this.
	 * @return The result of vector addition.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "+;+="))
	static FVector Vector_AddVector(const FVector& Host, const FVector& V);

	/**
	 * Gets the result of adding to each component of the vector.
	 *
	 * @param Bias How much to add to each component.
	 * @return The result of addition.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "+"))
	static FVector Vector_AddFloat(const FVector& Host, double Bias);

	/**
	 * Gets the result of component-wise subtraction of this by another vector.
	 *
	 * @param V The vector to subtract from this.
	 * @return The result of vector subtraction.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "-;-="))
	static FVector Vector_SubtractVector(const FVector& Host, const FVector& V);

	/**
	 * Gets the result of subtracting from each component of the vector.
	 *
	 * @param Bias How much to subtract from each component.
	 * @return The result of subtraction.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "-"))
	static FVector Vector_SubtractFloat(const FVector& Host, double Bias);

	/**
	* Check against another vector for equality.
	*
	* @param V The vector to check against.
	* @return true if the vectors are equal, false otherwise.
	*/
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "=="))
	static bool Vector_Equal(const FVector& Host, const FVector& V);

	/**
	 * Check against another vector for inequality.
	 *
	 * @param V The vector to check against.
	 * @return true if the vectors are not equal, false otherwise.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "!="))
	static bool Vector_NotEqual(const FVector& Host, const FVector& V);

	/**
	 * Scales the vector.
	 *
	 * @param Scale Amount to scale this vector by.
	 * @return Copy of the vector after scaling.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "*;*="))
	static FVector Vector_MultiplyFloat(const FVector& Host, double Scale);

	/**
	 * Multiplies the vector with another vector, using component-wise multiplication.
	 *
	 * @param V What to multiply this vector with.
	 * @return Copy of the vector after multiplication.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "*;*="))
	static FVector Vector_MultiplyVector(const FVector& Host, const FVector& V);

	/**
	 * Divides the vector by a number.
	 *
	 * @param Scale What to divide this vector by.
	 * @return Copy of the vector after division.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "/;/="))
	static FVector Vector_DivideFloat(const FVector& Host, double Scale);

	/**
	 * Divides the vector by another vector, using component-wise division.
	 *
	 * @param V What to divide vector by.
	 * @return Copy of the vector after division.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "/;/="))
	static FVector Vector_DivideVector(const FVector& Host, const FVector& V);

	/**
	 * Get a negated copy of the vector.
	 *
	 * @return A negated copy of the vector.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "neg"))
	static FVector Vector_Negated(const FVector& Host);

	/**
	 * Get the length (magnitude) of this vector.
	 *
	 * @return The length of this vector.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Size"))
	static double Vector_Size(const FVector& Host);

	/**
	 * Get the length of the 2D components of this vector.
	 *
	 * @return The 2D length of this vector.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Size2D"))
	static double Vector_Size2D(const FVector& Host);

	/**
	 * Get the squared length of this vector.
	 *
	 * @return The squared length of this vector.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "SizeSquared"))
	static double Vector_SizeSquared(const FVector& Host);

	/**
	 * Gets a normalized copy of the vector, checking it is safe to do so based on the length.
	 * Returns zero vector if vector length is too small to safely normalize.
	 *
	 * @param Tolerance Minimum squared length of vector for normalization.
	 * @return A normalized copy if safe, (0,0,0) otherwise.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetSafeNormal"))
	static FVector Vector_GetSafeNormal(const FVector& Host, double Tolerance = 1.e-8);

	/**
	 * Calculate the distance between two vectors.
	 *
	 * @param V1 The first vector.
	 * @param V2 The second vector.
	 * @return The distance between the two vectors.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Dist", UPyScriptStatic))
	static double Vector_Dist(const FVector& Host, const FVector& V1, const FVector& V2);

	/**
	 * Euclidean distance between two points in the XY plane.
	 *
	 * @param V1 The first point.
	 * @param V2 The second point.
	 * @return The distance between two points in the XY plane.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Dist2D", UPyScriptStatic))
	static double Vector_Dist2D(const FVector& Host, const FVector& V1, const FVector& V2);

	/**
	 * Calculate the squared distance between two vectors.
	 *
	 * @param V1 The first vector.
	 * @param V2 The second vector.
	 * @return The squared distance between the two vectors.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "DistSquared", UPyScriptStatic))
	static double Vector_DistSquared(const FVector& Host, const FVector& V1, const FVector& V2);

	/**
	 * Checks whether all components of the vector are exactly zero.
	 *
	 * @return true if the vector is exactly zero, false otherwise.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "IsZero"))
	static bool Vector_IsZero(const FVector& Host);

	/**
	 * Checks whether vector is near to zero within a specified tolerance.
	 *
	 * @param Tolerance Error tolerance.
	 * @return true if the vector is near to zero, false otherwise.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "IsNearlyZero"))
	static bool Vector_IsNearlyZero(const FVector& Host, double Tolerance = 1.e-4);

	/**
	 * Checks whether vector is normalized.
	 *
	 * @return true if the vector is normalized, false otherwise.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "IsNormalized"))
	static bool Vector_IsNormalized(const FVector& Host);

	/**
	 * Gets a copy of this vector projected onto the input vector.
	 *
	 * @param V Vector to project onto, does not assume it is normalized.
	 * @return Projected vector.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "ProjectOnTo"))
	static FVector Vector_ProjectOnTo(const FVector& Host, const FVector& V);

	/**
	 * Gets a copy of this vector projected onto the input vector, which is assumed to be unit length.
	 *
	 * @param Normal Vector to project onto (assumed to be unit length).
	 * @return Projected vector.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "ProjectOnToNormal"))
	static FVector Vector_ProjectOnToNormal(const FVector& Host, const FVector& Normal);

	/**
	 * Create a copy of this vector, with its magnitude clamped between Min and Max.
	 *
	 * @param Min Minimum magnitude.
	 * @param Max Maximum magnitude.
	 * @return A copy of this vector with clamped magnitude.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetClampedToSize"))
	static FVector Vector_GetClampedToSize(const FVector& Host, double Min, double Max);

	/**
	 * Create a copy of this vector, with the maximum magnitude clamped to Max.
	 *
	 * @param Max Maximum magnitude.
	 * @return A copy of this vector with clamped magnitude.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetClampedToMaxSize"))
	static FVector Vector_GetClampedToMaxSize(const FVector& Host, double Max);

	/**
	 * Get a copy of this vector with absolute value of each component.
	 *
	 * @return A copy of this vector with absolute value of each component.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetAbs"))
	static FVector Vector_GetAbs(const FVector& Host);

	/**
	 * Mirror a vector about a normal vector.
	 *
	 * @param MirrorNormal Normal vector to mirror about.
	 * @return Mirrored vector.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "MirrorByVector"))
	static FVector Vector_MirrorByVector(const FVector& Host, const FVector& MirrorNormal);

	/**
	 * Rotates around Axis (assumes Axis.Size() == 1).
	 *
	 * @param AngleDeg Angle to rotate (in degrees).
	 * @param Axis Axis to rotate around.
	 * @return Rotated Vector.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "RotateAngleAxis"))
	static FVector Vector_RotateAngleAxis(const FVector& Host, double AngleDeg, const FVector& Axis);

	/**
	 * Return the FRotator orientation corresponding to the direction in which the vector points.
	 * Sets Yaw and Pitch to the proper numbers, and sets Roll to zero because the roll can't be determined from a vector.
	 *
	 * @return FRotator from the Vector's direction, without any roll.
	 * @see ToOrientationQuat()
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "ToOrientationRotator"))
	static FRotator Vector_ToOrientationRotator(const FVector& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void Vector_Init1(const FVector& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void Vector_Init2(const FVector& Host, double InF);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void Vector_Init3(const FVector& Host, int32 InI);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void Vector_Init4(const FVector& Host, double InX, double InY, double InZ);

	// ============================================
	// FVector2D exposed for scripting
	// ============================================

	/** A zero vector (0,0) */
	UFUNCTION(BlueprintPure, meta = (UPyScriptConstant = "ZeroVector"))
	static FVector2D Vector2D_ZeroVector(const FVector2D& Host);

	/** One vector (1,1) */
	UFUNCTION(BlueprintPure, meta = (UPyScriptConstant = "UnitVector"))
	static FVector2D Vector2D_UnitVector(const FVector2D& Host);

	/** Unit vector (1,1) */
	UFUNCTION(BlueprintPure, meta = (UPyScriptConstant = "Unit45Deg"))
	static FVector2D Vector2D_Unit45Deg(const FVector2D& Host);

	/**
	 * Calculate the cross product of two vectors.
	 *
	 * @param A The first vector.
	 * @param B The second vector.
	 * @return The cross product.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "CrossProduct", UPyScriptStatic))
	static double Vector2D_CrossProduct(const FVector2D& Host, const FVector2D& A, const FVector2D& B);

	/**
	 * Calculate the dot product of two vectors.
	 *
	 * @param A The first vector.
	 * @param B The second vector.
	 * @return The dot product.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "DotProduct", UPyScriptStatic))
	static double Vector2D_DotProduct(const FVector2D& Host, const FVector2D& A, const FVector2D& B);

	/**
	 * Calculate the distance between two vectors.
	 *
	 * @param V1 The first vector.
	 * @param V2 The second vector.
	 * @return The distance between the two vectors.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Distance", UPyScriptStatic))
	static double Vector2D_Distance(const FVector2D& Host, const FVector2D& V1, const FVector2D& V2);

	/**
	 * Calculate the squared distance between two vectors.
	 *
	 * @param V1 The first vector.
	 * @param V2 The second vector.
	 * @return The squared distance between the two vectors.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "DistSquared", UPyScriptStatic))
	static double Vector2D_DistSquared(const FVector2D& Host, const FVector2D& V1, const FVector2D& V2);

	/**
	 * Gets the result of component-wise addition of this and another vector.
	 *
	 * @param V The vector to add to this.
	 * @return The result of vector addition.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "+;+="))
	static FVector2D Vector2D_AddVector(const FVector2D& Host, const FVector2D& V);

	/**
	 * Gets the result of adding to each component of the vector.
	 *
	 * @param Bias How much to add to each component.
	 * @return The result of addition.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "+"))
	static FVector2D Vector2D_AddFloat(const FVector2D& Host, double Bias);

	/**
	 * Gets the result of component-wise subtraction of this by another vector.
	 *
	 * @param V The vector to subtract from this.
	 * @return The result of vector subtraction.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "-;-="))
	static FVector2D Vector2D_SubtractVector(const FVector2D& Host, const FVector2D& V);

	/**
	 * Gets the result of subtracting from each component of the vector.
	 *
	 * @param Bias How much to subtract from each component.
	 * @return The result of subtraction.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "-"))
	static FVector2D Vector2D_SubtractFloat(const FVector2D& Host, double Bias);

	/**
	 * Multiplies the vector with another vector, using component-wise multiplication.
	 *
	 * @param V What to multiply this vector with.
	 * @return Copy of the vector after multiplication.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "*;*="))
	static FVector2D Vector2D_MultiplyVector(const FVector2D& Host, const FVector2D& V);

	/**
	 * Scales the vector.
	 *
	 * @param Scale Amount to scale this vector by.
	 * @return Copy of the vector after scaling.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "*;*="))
	static FVector2D Vector2D_MultiplyFloat(const FVector2D& Host, double Scale);

	/**
	 * Divides the vector by another vector, using component-wise division.
	 *
	 * @param V What to divide vector by.
	 * @return Copy of the vector after division.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "/;/="))
	static FVector2D Vector2D_DivideVector(const FVector2D& Host, const FVector2D& V);

	/**
	 * Divides the vector by a number.
	 *
	 * @param Scale What to divide this vector by.
	 * @return Copy of the vector after division.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "/;/="))
	static FVector2D Vector2D_DivideFloat(const FVector2D& Host, double Scale);

	/**
	 * Get a negated copy of the vector.
	 *
	 * @return A negated copy of the vector.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "neg"))
	static FVector2D Vector2D_Negated(const FVector2D& Host);

	/**
	* Check against another vector for equality.
	*
	* @param V The vector to check against.
	* @return true if the vectors are equal, false otherwise.
	*/
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "=="))
	static bool Vector2D_Equal(const FVector2D& Host, const FVector2D& V);

	/**
	 * Check against another vector for inequality.
	 *
	 * @param V The vector to check against.
	 * @return true if the vectors are not equal, false otherwise.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "!="))
	static bool Vector2D_NotEqual(const FVector2D& Host, const FVector2D& V);

	/**
	 * Calculate the dot product between this and another vector.
	 *
	 * @param V The other vector.
	 * @return The dot product.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Dot", UPyScriptOperator = "|"))
	static double Vector2D_Dot(const FVector2D& Host, const FVector2D& V);

	/**
	 * Calculate cross product between this and another vector.
	 *
	 * @param V The other vector.
	 * @return The cross product.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "^"))
	static double Vector2D_Cross(const FVector2D& Host, const FVector2D& V);

	/**
	 * Get the length (magnitude) of this vector.
	 *
	 * @return The length of this vector.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Size"))
	static double Vector2D_Size(const FVector2D& Host);

	/**
	 * Get the squared length of this vector.
	 *
	 * @return The squared length of this vector.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "SizeSquared"))
	static double Vector2D_SizeSquared(const FVector2D& Host);

	/**
	 * Gets a normalized copy of the vector, checking it is safe to do so based on the length.
	 * Returns zero vector if vector length is too small to safely normalize.
	 *
	 * @param Tolerance Minimum squared length of vector for normalization.
	 * @return A normalized copy if safe, (0,0) otherwise.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetSafeNormal"))
	static FVector2D Vector2D_GetSafeNormal(const FVector2D& Host, double Tolerance = 1.e-8);

	/**
	 * Normalize this vector in-place if it is large enough, set it to (0,0) otherwise.
	 *
	 * @param Tolerance Minimum squared length of vector for normalization.
	 * @see GetSafeNormal()
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Normalize"))
	static void Vector2D_Normalize(FVector2D& Host, double Tolerance = 1.e-8);

	/**
	 * Checks whether vector is near to zero within a specified tolerance.
	 *
	 * @param Tolerance Error tolerance.
	 * @return true if the vector is near to zero, false otherwise.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "IsNearlyZero"))
	static bool Vector2D_IsNearlyZero(const FVector2D& Host, double Tolerance = 1.e-4);

	/**
	 * Checks whether all components of the vector are exactly zero.
	 *
	 * @return true if the vector is exactly zero, false otherwise.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "IsZero"))
	static bool Vector2D_IsZero(const FVector2D& Host);

	/**
	 * Get a copy of this vector with absolute value of each component.
	 *
	 * @return A copy of this vector with absolute value of each component.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetAbs"))
	static FVector2D Vector2D_GetAbs(const FVector2D& Host);

	/**
	 * Rotates around axis (0,0,1)
	 *
	 * @param AngleDeg Angle to rotate (in degrees).
	 * @return Rotated Vector.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetRotated"))
	static FVector2D Vector2D_GetRotated(const FVector2D& Host, double AngleDeg);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void Vector2D_Init1(const FVector2D& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void Vector2D_Init2(const FVector2D& Host, double InX, double InY);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void Vector2D_Init3(const FVector2D& Host, const FVector2D& InV);

	// ============================================
	// FRotator exposed for scripting
	// ============================================

	/** A rotator of zero degrees on each axis. */
	UFUNCTION(BlueprintPure, meta = (UPyScriptConstant = "ZeroRotator"))
	static FRotator Rotator_ZeroRotator(const FRotator& Host);

	/**
	 * Get the result of adding a rotator to this.
	 *
	 * @param R The other rotator.
	 * @return The result of adding a rotator to this.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "+;+="))
	static FRotator Rotator_Add(const FRotator& Host, const FRotator& R);

	/**
	 * Get the result of subtracting a rotator from this.
	 *
	 * @param R The other rotator.
	 * @return The result of subtracting a rotator from this.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "-;-="))
	static FRotator Rotator_Subtract(const FRotator& Host, const FRotator& R);

	/**
	 * Get the result of scaling this rotator.
	 *
	 * @param Scale The scaling factor.
	 * @return The result of scaling.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "*;*="))
	static FRotator Rotator_Multiply(const FRotator& Host, double Scale);

	/**
	 * Checks whether two rotators are identical. This checks each component for exact equality.
	 *
	 * @param R The other rotator.
	 * @return true if two rotators are identical, otherwise false.
	 * @see Equals()
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "=="))
	static bool Rotator_Equal(const FRotator& Host, const FRotator& R);

	/**
	 * Checks whether two rotators are different.
	 *
	 * @param R The other rotator.
	 * @return true if two rotators are different, otherwise false.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "!="))
	static bool Rotator_NotEqual(const FRotator& Host, const FRotator& R);

	/**
	 * Checks whether rotator is nearly zero within specified tolerance, when treated as an orientation.
	 * This means that TRotator(0, 0, 360) is "zero", because it is the same final orientation as the zero rotator.
	 *
	 * @param Tolerance Error Tolerance.
	 * @return true if rotator is nearly zero, within specified tolerance, otherwise false.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "IsNearlyZero"))
	static bool Rotator_IsNearlyZero(const FRotator& Host, double Tolerance = 1.e-4);

	/**
	 * Checks whether this has exactly zero rotation, when treated as an orientation.
	 * This means that TRotator(0, 0, 360) is "zero", because it is the same final orientation as the zero rotator.
	 *
	 * @return true if this has exactly zero rotation, otherwise false.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "IsZero"))
	static bool Rotator_IsZero(const FRotator& Host);

	/**
	 * Convert a rotation into a unit vector facing in its direction.
	 *
	 * @return Rotation as a unit direction vector.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Vector"))
	static FVector Rotator_Vector(const FRotator& Host);

	/**
	 * Returns the inverse of the rotator.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetInverse"))
	static FRotator Rotator_GetInverse(const FRotator& Host);

	/**
	 * Rotate a vector rotated by this rotator.
	 *
	 * @param V The vector to rotate.
	 * @return The rotated vector.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "RotateVector"))
	static FVector Rotator_RotateVector(const FRotator& Host, const FVector& V);

	/**
	 * Returns the vector rotated by the inverse of this rotator.
	 *
	 * @param V The vector to rotate.
	 * @return The rotated vector.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "UnrotateVector"))
	static FVector Rotator_UnrotateVector(const FRotator& Host, const FVector& V);

	/**
	 * Gets the rotation values so they fall within the range [0,360]
	 *
	 * @return Clamped version of rotator.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Clamp"))
	static FRotator Rotator_Clamp(const FRotator& Host);

	/**
	 * Create a copy of this rotator and normalize, removes all winding and creates the "shortest route" rotation.
	 *
	 * @return Normalized copy of this rotator
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetNormalized"))
	static FRotator Rotator_GetNormalized(const FRotator& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void Rotator_Init1(const FRotator& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void Rotator_Init2(const FRotator& Host, double InF);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void Rotator_Init3(const FRotator& Host, double InPitch, double InYaw, double InRoll);

	// ============================================
	// FTransform exposed for scripting
	// ============================================

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetLocation"))
	static FVector Transform_GetLocation(const FTransform& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "SetLocation"))
	static void Transform_SetLocation(const FTransform& Host, const FVector& InLocation);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "AddToTranslation"))
	static void Transform_AddToTranslation(FTransform& Host, const FVector& DeltaTranslation);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Rotator"))
	static FRotator Transform_GetRotation(const FTransform& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetScale3D"))
	static FVector Transform_GetScale3D(const FTransform& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "SetScale3D"))
	static void Transform_SetScale3D(const FTransform& Host, const FVector& InScale3D);

	/** The identity transform (Rotation: 0, Translation: 0, Scale: 1) */
	UFUNCTION(BlueprintPure, meta = (UPyScriptConstant = "Identity"))
	static FTransform Transform_Identity(const FTransform& Host);

	/**
	 * Inverts the transform.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Inverse"))
	static FTransform Transform_Inverse(const FTransform& Host);

	/** Set this transform to the weighted blend of the supplied two transforms. */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Blend"))
	static void Transform_Blend(const FTransform& Host, const FTransform& Atom1, const FTransform& Atom2, float Alpha);

	/** Set this Transform to the weighted blend of it and the supplied Transform. */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "BlendWith"))
	static void Transform_BlendWith(const FTransform& Host, const FTransform& OtherAtom, float Alpha);

	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "*;*="))
	static FTransform Transform_Multiply(const FTransform& Host, const FTransform& Other);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "TransformPosition"))
	static FVector Transform_TransformPosition(const FTransform& Host, const FVector& V);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "TransformPositionNoScale"))
	static FVector Transform_TransformPositionNoScale(const FTransform& Host, const FVector& V);

	/** Inverts the transform and then transforms V - correctly handles scaling in this transform. */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "InverseTransformPosition"))
	static FVector Transform_InverseTransformPosition(const FTransform& Host, const FVector& V);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "InverseTransformPositionNoScale"))
	static FVector Transform_InverseTransformPositionNoScale(const FTransform& Host, const FVector& V);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "TransformVector"))
	static FVector Transform_TransformVector(const FTransform& Host, const FVector& V);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "TransformVectorNoScale"))
	static FVector Transform_TransformVectorNoScale(const FTransform& Host, const FVector& V);

	/**
	 *	Transform a direction vector by the inverse of this matrix - will not take into account translation part.
	 *	If you want to transform a surface normal (or plane) and correctly account for non-uniform scaling you should use TransformByUsingAdjointT with adjoint of matrix inverse.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "InverseTransformVector"))
	static FVector Transform_InverseTransformVector(const FTransform& Host, const FVector& V);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "InverseTransformVectorNoScale"))
	static FVector Transform_InverseTransformVectorNoScale(const FTransform& Host, const FVector& V);

	/*******************************************************************************************
	 * The below 2 functions are the ones to get delta transform and return TTransform<T> format that can be concatenated
	 * Inverse itself can't concatenate with VQS format(since VQS always transform from S->Q->T, where inverse happens from T(-1)->Q(-1)->S(-1))
	 * So these 2 provides ways to fix this
	 * GetRelativeTransform returns this*Other(-1) and parameter is Other(not Other(-1))
	 * GetRelativeTransformReverse returns this(-1)*Other, and parameter is Other.
	 *******************************************************************************************/
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetRelativeTransform"))
	static FTransform Transform_GetRelativeTransform(const FTransform& Host, const FTransform& Other);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetRelativeTransformReverse"))
	static FTransform Transform_GetRelativeTransformReverse(const FTransform& Host, const FTransform& Other);

	/**
	 * Set current transform and the relative to ParentTransform.
	 * Equates to This = This->GetRelativeTransform(Parent), but saves the intermediate TTransform<T> storage and copy.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "SetToRelativeTransform"))
	static void Transform_SetToRelativeTransform(FTransform& Host, const FTransform& ParentTransform);

	/**
	 * Normalize the rotation component of this transformation
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "NormalizeRotation"))
	static void Transform_NormalizeRotation(FTransform& Host);

	/**
	 *
	 * @return true if the rotation component is normalized, and false otherwise.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "IsRotationNormalized"))
	static bool Transform_IsRotationNormalized(const FTransform& Host);

	/** Calculate the determinant */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetDeterminant"))
	static double Transform_GetDeterminant(const FTransform& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void Transform_Init1(const FTransform& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void Transform_Init2(const FTransform& Host, const FVector& InTranslation);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void Transform_Init3(const FTransform& Host, const FRotator& InRotation);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void Transform_Init4(const FTransform& Host, const FRotator& InRotation, const FVector& InTranslation, const FVector& InScale3D);

	// ============================================
	// FQuat exposed for scripting
	// ============================================

	/** Identity quaternion. */
	UFUNCTION(BlueprintPure, meta = (UPyScriptConstant = "Identity"))
	static FQuat Quat_Identity(const FQuat& Host);

	/**
	 * Gets the result of adding a Quaternion to this.
	 * This is a component-wise addition; composing quaternions should be done via multiplication.
	 *
	 * @param Q The Quaternion to add.
	 * @return The result of addition.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "+;+="))
	static FQuat Quat_Add(const FQuat& Host, const FQuat& Q);

	/**
	 * Gets the result of subtracting a Quaternion to this.
	 * This is a component-wise subtraction; composing quaternions should be done via multiplication.
	 *
	 * @param Q The Quaternion to subtract.
	 * @return The result of subtraction.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "-;-="))
	static FQuat Quat_Subtract(const FQuat& Host, const FQuat& Q);

	/**
	 * Gets the result of multiplying this by another quaternion (this * Q).
	 *
	 * Order matters when composing quaternions: C = A * B will yield a quaternion C that logically
	 * first applies B then A to any subsequent transformation (right first, then left).
	 *
	 * @param Q The Quaternion to multiply this by.
	 * @return The result of multiplication (this * Q).
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "*;*="))
	static FQuat Quat_Multiply(const FQuat& Host, const FQuat& Q);

	/**
	 * Rotate a vector by this quaternion.
	 *
	 * @param V the vector to be rotated
	 * @return vector after rotation
	 * @see RotateVector
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "*"))
	static FVector Quat_MultiplyVector(const FQuat& Host, const FVector& V);

	/**
	 * Checks whether another Quaternion is equal to this, within specified tolerance.
	 *
	 * @param Q The other Quaternion.
	 * @param Tolerance Error tolerance for comparison with other Quaternion.
	 * @return true if two Quaternions are equal, within specified tolerance, otherwise false.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Equals"))
	static bool Quat_Equals(const FQuat& Host, const FQuat& Q, double Tolerance = 1.e-4);

	/**
	 * Checks whether this Quaternion is an Identity Quaternion.
	 * Assumes Quaternion tested is normalized.
	 *
	 * @param Tolerance Error tolerance for comparison with Identity Quaternion.
	 * @return true if Quaternion is a normalized Identity Quaternion.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "IsIdentity"))
	static bool Quat_IsIdentity(const FQuat& Host, double Tolerance = 1.e-4);

	/**
	 * Convert a vector of floating-point Euler angles (in degrees) into a Quaternion.
	 *
	 * @param Euler the Euler angles
	 * @return constructed TQuat
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "MakeFromEuler", UPyScriptStatic))
	static FQuat Quat_MakeFromEuler(const FQuat& Host, const FVector& Euler);

	/** Convert a Quaternion into floating-point Euler angles (in degrees). */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Euler"))
	static FVector Quat_Euler(const FQuat& Host);

	/**
	 * Normalize this quaternion if it is large enough.
	 * If it is too small, returns an identity quaternion.
	 *
	 * @param Tolerance Minimum squared length of quaternion for normalization.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Normalize"))
	static void Quat_Normalize(FQuat& Host, double Tolerance = 1.e-8);

	/**
	 * Get a normalized copy of this quaternion.
	 * If it is too small, returns an identity quaternion.
	 *
	 * @param Tolerance Minimum squared length of quaternion for normalization.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetNormalized"))
	static FQuat Quat_GetNormalized(const FQuat& Host, double Tolerance = 1.e-8);

	// Return true if this quaternion is normalized
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "IsNormalized"))
	static bool Quat_IsNormalized(const FQuat& Host);

	/**
	 * Get the length of this quaternion.
	 *
	 * @return The length of this quaternion.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Size"))
	static double Quat_Size(const FQuat& Host);

	/**
	 * Get the length squared of this quaternion.
	 *
	 * @return The length of this quaternion.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "SizeSquared"))
	static double Quat_SizeSquared(const FQuat& Host);

	/**
	 * Get the angle in radians of this quaternion
	 * @warning : The rotation returned may not be the shortest version.
	 *            You may wish to call GetShortestArcWith(FQuat::Identity) first.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetAngle"))
	static double Quat_GetAngle(const FQuat& Host);

	/**
	 * Get the axis and angle of rotation of this quaternion
	 *
	 * @param Axis{out] Axis of rotation
	 * @param Angle{out] Angle of the quaternion in radians
	 * @warning : Requires this quaternion to be normalized.
	 * @warning : The rotation returned may not be the shortest version.
	 *            You may wish to call GetShortestArcWith(FQuat::Identity) first.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "ToAxisAndAngle"))
	static void Quat_ToAxisAndAngle(const FQuat& Host, FVector& Axis, double& Angle);

	/**
	 * Get the axis of rotation of the Quaternion.
	 * This is the axis around which rotation occurs to transform the canonical coordinate system to the target orientation.
	 * For the identity Quaternion which has no such rotation, TVector<T>(1,0,0) is returned.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetRotationAxis"))
	static FVector Quat_GetRotationAxis(const FQuat& Host);

	/**
	 * Rotate a vector by this quaternion.
	 *
	 * @param V the vector to be rotated
	 * @return vector after rotation
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "RotateVector"))
	static FVector Quat_RotateVector(const FQuat& Host, const FVector& V);

	/**
	 * Rotate a vector by the inverse of this quaternion.
	 *
	 * @param V the vector to be rotated
	 * @return vector after rotation by the inverse of this quaternion.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "UnrotateVector"))
	static FVector Quat_UnrotateVector(const FQuat& Host, const FVector& V);

	/**
	 * @return inverse of this quaternion
	 * @warning : Requires this quaternion to be normalized.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Inverse"))
	static FQuat Quat_Inverse(const FQuat& Host);

	/** Get the forward direction (X axis) after it has been rotated by this Quaternion. */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetAxisX"))
	static FVector Quat_GetAxisX(const FQuat& Host);

	/** Get the right direction (Y axis) after it has been rotated by this Quaternion. */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetAxisY"))
	static FVector Quat_GetAxisY(const FQuat& Host);

	/** Get the up direction (Z axis) after it has been rotated by this Quaternion. */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetAxisZ"))
	static FVector Quat_GetAxisZ(const FQuat& Host);

	/** Get the forward direction (X axis) after it has been rotated by this Quaternion. */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetForwardVector"))
	static FVector Quat_GetForwardVector(const FQuat& Host);

	/** Get the right direction (Y axis) after it has been rotated by this Quaternion. */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetRightVector"))
	static FVector Quat_GetRightVector(const FQuat& Host);

	/** Get the up direction (Z axis) after it has been rotated by this Quaternion. */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetUpVector"))
	static FVector Quat_GetUpVector(const FQuat& Host);

	/** Convert a rotation into a unit vector facing in its direction. Equivalent to GetForwardVector(). */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Vector"))
	static FVector Quat_Vector(const FQuat& Host);

	/** Get the TRotator<T> representation of this Quaternion. */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Rotator"))
	static FRotator Quat_Rotator(const FQuat& Host);

	/** Get the TMatrix<T> representation of this Quaternion. */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "ToMatrix"))
	static FMatrix Quat_ToMatrix(const FQuat& Host);

	/**
	 * Get the twist angle (in radians) for a specified axis
	 *
	 * @param TwistAxis Axis to use for decomposition
	 * @return Twist angle (in radians)
	 * @warning assumes normalized quaternion and twist axis
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetTwistAngle"))
	static double Quat_GetTwistAngle(const FQuat& Host, const FVector& TwistAxis);

	/**
	 * Decompose this Quaternion into swing and twist components
	 *
	 * @param InTwistAxis Axis to use for decomposition
	 * @param OutSwing swing component quaternion
	 * @param OutTwist Twist component quaternion
	 * @warning assumes normalized quaternion and twist axis
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "ToSwingTwist"))
	static void Quat_ToSwingTwist(const FQuat& Host, const FVector& InTwistAxis, FQuat& OutSwing, FQuat& OutTwist);

	/** Find the angular distance between two rotation quaternions (in radians) */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "AngularDistance"))
	static double Quat_AngularDistance(const FQuat& Host, const FQuat& Q);

	/**
	 * Generates the 'smallest' (geodesic) rotation between two vectors of arbitrary length.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "FindBetween", UPyScriptStatic))
	static FQuat Quat_FindBetween(const FQuat& Host, const FVector& Vector1, const FVector& Vector2);

	/**
	 * Generates the 'smallest' (geodesic) rotation between two normals (assumed to be unit length).
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "FindBetweenNormals", UPyScriptStatic))
	static FQuat Quat_FindBetweenNormals(const FQuat& Host, const FVector& Normal1, const FVector& Normal2);

	/**
	 * Generates the 'smallest' (geodesic) rotation between two vectors of arbitrary length.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "FindBetweenVectors", UPyScriptStatic))
	static FQuat Quat_FindBetweenVectors(const FQuat& Host, const FVector& Vector1, const FVector& Vector2);

	/**
	 * Error measure (angle) between two quaternions, ranged [0..1].
	 * Returns the hypersphere-angle between two quaternions; alignment shouldn't matter, though
	 * @note normalized input is expected.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Error", UPyScriptStatic))
	static double Quat_Error(const FQuat& Host, const FQuat& Q1, const FQuat& Q2);

	/**
	 * TQuat<T>::Error with auto-normalization.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "ErrorAutoNormalize", UPyScriptStatic))
	static double Quat_ErrorAutoNormalize(const FQuat& Host, const FQuat& A, const FQuat& B);

	/**
	 * Fast Linear Quaternion Interpolation.
	 * Result is NOT normalized.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "FastLerp", UPyScriptStatic))
	static FQuat Quat_FastLerp(const FQuat& Host, const FQuat& A, const FQuat& B, const double Alpha);

	/**
	 * Bi-Linear Quaternion Interpolation.
	 * Result is NOT normalized.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "FastBilerp", UPyScriptStatic))
	static FQuat Quat_FastBilerp(const FQuat& Host, const FQuat& P00, const FQuat& P10, const FQuat& P01, const FQuat& P11, double FracX, double FracY);

	/** Spherical interpolation. Will correct alignment. Result is normalized. */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Slerp", UPyScriptStatic))
	static FQuat Quat_Slerp(const FQuat& Host, const FQuat& Quat1, const FQuat& Quat2, double Slerp);

	/** Spherical interpolation. Will correct alignment. Result is NOT normalized. */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Slerp_NotNormalized", UPyScriptStatic))
	static FQuat Quat_Slerp_NotNormalized(const FQuat& Host, const FQuat& Quat1, const FQuat& Quat2, double Slerp);

	/**
	 * Simpler Slerp that doesn't do any checks for 'shortest distance' etc.
	 * We need this for the cubic interpolation stuff so that the multiple Slerps dont go in different directions.
	 * Result is normalized.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "SlerpFullPath", UPyScriptStatic))
	static FQuat Quat_SlerpFullPath(const FQuat& Host, const FQuat& quat1, const FQuat& quat2, double Alpha);

	/**
	 * Simpler Slerp that doesn't do any checks for 'shortest distance' etc.
	 * We need this for the cubic interpolation stuff so that the multiple Slerps dont go in different directions.
	 * Result is NOT normalized.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "SlerpFullPath_NotNormalized", UPyScriptStatic))
	static FQuat Quat_SlerpFullPath_NotNormalized(const FQuat& Host, const FQuat& quat1, const FQuat& quat2, double Alpha);

	/**
	 * Given a start and end quaternion and their tangents, interpolate between them.
	 * Result is normalized.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Squad", UPyScriptStatic))
	static FQuat Quat_Squad(const FQuat& Host, const FQuat& quat1, const FQuat& tang1, const FQuat& quat2, const FQuat& tang2, double Alpha);

	/**
	 * Simpler Squad that doesn't do any checks for 'shortest distance' etc.
	 * Result is normalized.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "SquadFullPath", UPyScriptStatic))
	static FQuat Quat_SquadFullPath(const FQuat& Host, const FQuat& quat1, const FQuat& tang1, const FQuat& quat2, const FQuat& tang2, double Alpha);

	/**
	 * Calculate tangents for spline interpolation.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "CalcTangents", UPyScriptStatic))
	static void Quat_CalcTangents(const FQuat& Host, const FQuat& PrevP, const FQuat& P, const FQuat& NextP, double Tension, FQuat& OutTan);

	/**
	 * @return quaternion with W=0 and V=theta*v.
	 * @warning : The rotation returned may not be the shortest version.
	 *            You may wish to call GetShortestArcWith(FQuat::Identity) first.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Log"))
	static FQuat Quat_Log(const FQuat& Host);

	/**
	 * @note Exp should really only be used after Log.
	 * Assumes a quaternion with W=0 and V=theta*v (where |v| = 1).
	 * Exp(q) = (sin(theta)*v, cos(theta))
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Exp"))
	static FQuat Quat_Exp(const FQuat& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void Quat_Init1(const FQuat& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void Quat_Init2(const FQuat& Host, const FVector& Axis, double AngleRad);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void Quat_Init3(const FQuat& Host, const FMatrix& M);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void Quat_Init4(const FQuat& Host, const FRotator& R);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void Quat_Init5(const FQuat& Host, double InX, double InY, double InZ, double InW);

	// ============================================
	// FMatrix exposed for scripting
	// ============================================

	/** Identity matrix. */
	UFUNCTION(BlueprintPure, meta = (UPyScriptConstant = "Identity"))
	static FMatrix Matrix_Identity(const FMatrix& Host);

	/**
	 * Matrix multiplication.
	 *
	 * @param Other The matrix to multiply with.
	 * @return The result of multiplication.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "*;*="))
	static FMatrix Matrix_Multiply(const FMatrix& Host, const FMatrix& Other);

	/**
	 * Matrix addition.
	 *
	 * @param Other The matrix to add.
	 * @return The result of addition.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "+;+="))
	static FMatrix Matrix_Add(const FMatrix& Host, const FMatrix& Other);

	/**
	 * Matrix scaling.
	 *
	 * @param Scale The scale factor.
	 * @return The result of scaling.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "*;*="))
	static FMatrix Matrix_MultiplyFloat(const FMatrix& Host, float Scale);

	/**
	 * Transform a location - will take into account translation part of the FMatrix.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "TransformPosition"))
	static FVector Matrix_TransformPosition(const FMatrix& Host, const FVector& V);

	/**
	 * Inverts the matrix and then transforms V - correctly handles scaling in this matrix.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "InverseTransformPosition"))
	static FVector Matrix_InverseTransformPosition(const FMatrix& Host, const FVector& V);

	/**
	 * Transform a direction vector - will not take into account translation part of the FMatrix.
	 * If you want to transform a surface normal (or plane) and correctly account for non-uniform scaling you should use TransformByUsingAdjointT with adjoint of matrix inverse.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "TransformVector"))
	static FVector Matrix_TransformVector(const FMatrix& Host, const FVector& V);

	/**
	 * Transform a direction vector by the inverse of this matrix - will not take into account translation part.
	 * If you want to transform a surface normal (or plane) and correctly account for non-uniform scaling you should use TransformByUsingAdjointT with adjoint of matrix inverse.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "InverseTransformVector"))
	static FVector Matrix_InverseTransformVector(const FMatrix& Host, const FVector& V);

	/**
	 * Transpose.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetTransposed"))
	static FMatrix Matrix_GetTransposed(const FMatrix& Host);

	/**
	 * @return determinant of this matrix.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Determinant"))
	static double Matrix_Determinant(const FMatrix& Host);

	/**
	 * @return the determinant of the 3x3 rotation matrix
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "RotDeterminant"))
	static double Matrix_RotDeterminant(const FMatrix& Host);

	/**
	 * Fast path, doesn't check for nil matrices in debug.
	 * Returns the inverse of the matrix.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "InverseFast"))
	static FMatrix Matrix_InverseFast(const FMatrix& Host);

	/**
	 * Fast path, doesn't check for nil matrices in debug.
	 * Returns the inverse of the matrix.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Inverse"))
	static FMatrix Matrix_Inverse(const FMatrix& Host);

	/**
	 * TransposeAdjoint
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "TransposeAdjoint"))
	static FMatrix Matrix_TransposeAdjoint(const FMatrix& Host);

	/**
	 * Remove any scaling from this matrix (ie magnitude of each row is 1) and return the 3D scale vector that was initially present.
	 *
	 * @return The 3D scale vector that was initially present.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "RemoveScaling"))
	static void Matrix_RemoveScaling(FMatrix& Host, double Tolerance = 1.e-8);

	/**
	 * Returns the matrix with any scaling removed.
	 *
	 * @return The matrix with any scaling removed.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetMatrixWithoutScale"))
	static FMatrix Matrix_GetMatrixWithoutScale(const FMatrix& Host, double Tolerance = 1.e-8);

	/**
	 * Extract scaling from the matrix.
	 *
	 * @return The scaling vector.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetScaleVector"))
	static FVector Matrix_GetScaleVector(const FMatrix& Host, double Tolerance = 1.e-8);

	/**
	 * Extract scaling from the matrix.
	 *
	 * @return The scaling vector.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetScaledAxis"))
	static FVector Matrix_GetScaledAxis(const FMatrix& Host, EAxis::Type Axis);

	/**
	 * get axis of this matrix scaled by the scale of the matrix
	 *
	 * @param Axis The axis to get.
	 * @return The axis of this matrix scaled by the scale of the matrix.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetUnitAxis"))
	static FVector Matrix_GetUnitAxis(const FMatrix& Host, EAxis::Type Axis);

	/**
	 * Set an axis of this matrix.
	 *
	 * @param Axis The axis to set.
	 * @param Value The value to set the axis to.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "SetAxis"))
	static void Matrix_SetAxis(FMatrix& Host, EAxis::Type Axis, const FVector& Value);

	/**
	 * Set the origin of the coordinate system to the given vector
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "SetOrigin"))
	static void Matrix_SetOrigin(FMatrix& Host, const FVector& NewOrigin);

	/**
	 * Get the origin of the coordinate system
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetOrigin"))
	static FVector Matrix_GetOrigin(const FMatrix& Host);

	/**
	 * Get the column at the given index.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetColumn"))
	static FVector Matrix_GetColumn(const FMatrix& Host, int32 Index);

	/**
	 * Set the column at the given index.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "SetColumn"))
	static void Matrix_SetColumn(FMatrix& Host, int32 Index, const FVector& Value);

	/**
	 * Get the rotator representation of this matrix.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Rotator"))
	static FRotator Matrix_Rotator(const FMatrix& Host);

	/**
	 * Get the quaternion representation of this matrix.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "ToQuat"))
	static FQuat Matrix_ToQuat(const FMatrix& Host);

	/**
	 * Get the string representation of this matrix.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "ToString"))
	static FString Matrix_ToString(const FMatrix& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void Matrix_Init1(const FMatrix& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void Matrix_Init2(const FMatrix& Host, const FVector& InX, const FVector& InY, const FVector& InZ, const FVector& InW);

	// ============================================
	// FColor exposed for scripting
	// ============================================

	/** White color (255, 255, 255, 255) */
	UFUNCTION(BlueprintPure, meta = (UPyScriptConstant = "White"))
	static FColor Color_White(const FColor& Host);

	/** Black color (0, 0, 0, 255) */
	UFUNCTION(BlueprintPure, meta = (UPyScriptConstant = "Black"))
	static FColor Color_Black(const FColor& Host);

	/** Transparent color (0, 0, 0, 0) */
	UFUNCTION(BlueprintPure, meta = (UPyScriptConstant = "Transparent"))
	static FColor Color_Transparent(const FColor& Host);

	/** Red color (255, 0, 0, 255) */
	UFUNCTION(BlueprintPure, meta = (UPyScriptConstant = "Red"))
	static FColor Color_Red(const FColor& Host);

	/** Green color (0, 255, 0, 255) */
	UFUNCTION(BlueprintPure, meta = (UPyScriptConstant = "Green"))
	static FColor Color_Green(const FColor& Host);

	/** Blue color (0, 0, 255, 255) */
	UFUNCTION(BlueprintPure, meta = (UPyScriptConstant = "Blue"))
	static FColor Color_Blue(const FColor& Host);

	/** Yellow color (255, 255, 0, 255) */
	UFUNCTION(BlueprintPure, meta = (UPyScriptConstant = "Yellow"))
	static FColor Color_Yellow(const FColor& Host);

	/** Cyan color (0, 255, 255, 255) */
	UFUNCTION(BlueprintPure, meta = (UPyScriptConstant = "Cyan"))
	static FColor Color_Cyan(const FColor& Host);

	/** Magenta color (255, 0, 255, 255) */
	UFUNCTION(BlueprintPure, meta = (UPyScriptConstant = "Magenta"))
	static FColor Color_Magenta(const FColor& Host);

	/** Orange color (243, 156, 18, 255) */
	UFUNCTION(BlueprintPure, meta = (UPyScriptConstant = "Orange"))
	static FColor Color_Orange(const FColor& Host);

	/** Purple color (169, 7, 228, 255) */
	UFUNCTION(BlueprintPure, meta = (UPyScriptConstant = "Purple"))
	static FColor Color_Purple(const FColor& Host);

	/** Turquoise color (26, 188, 156, 255) */
	UFUNCTION(BlueprintPure, meta = (UPyScriptConstant = "Turquoise"))
	static FColor Color_Turquoise(const FColor& Host);

	/** Silver color (189, 195, 199, 255) */
	UFUNCTION(BlueprintPure, meta = (UPyScriptConstant = "Silver"))
	static FColor Color_Silver(const FColor& Host);

	/** Emerald color (46, 204, 113, 255) */
	UFUNCTION(BlueprintPure, meta = (UPyScriptConstant = "Emerald"))
	static FColor Color_Emerald(const FColor& Host);

	/**
	 * Component-wise addition.
	 *
	 * @param C The color to add.
	 * @return The result of addition.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "+="))
	static FColor Color_Add(const FColor& Host, const FColor& C);

	/**
	 * Compares two colors for equality.
	 *
	 * @param C The color to compare against.
	 * @return true if the colors are equal, false otherwise.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "=="))
	static bool Color_Equal(const FColor& Host, const FColor& C);

	/**
	 * Compares two colors for inequality.
	 *
	 * @param C The color to compare against.
	 * @return true if the colors are not equal, false otherwise.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "!="))
	static bool Color_NotEqual(const FColor& Host, const FColor& C);

	/**
	 * Creates a color from a string hex representation.
	 *
	 * @param HexString The string hex representation.
	 * @return The color.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "FromHex", UPyScriptStatic))
	static FColor Color_FromHex(const FColor& Host, const FString& HexString);

	/**
	 * Makes a random color.
	 *
	 * @return A random color.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "MakeRandomColor", UPyScriptStatic))
	static FColor Color_MakeRandomColor(const FColor& Host);

	/**
	 * Makes a color red->green with the passed scalar (0..1).
	 *
	 * @param Scalar Scalar value (0..1).
	 * @return The color.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "MakeRedToGreenColorFromScalar", UPyScriptStatic))
	static FColor Color_MakeRedToGreenColorFromScalar(const FColor& Host, float Scalar);

	/**
	 * Converts temperature in Kelvins of a black body radiator to RGB chromaticity.
	 *
	 * @param Temp Temperature in Kelvins.
	 * @return The color.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "MakeFromColorTemperature", UPyScriptStatic))
	static FColor Color_MakeFromColorTemperature(const FColor& Host, float Temp);

	/**
	 * Creates a new FColor with the same RGB components but a different Alpha.
	 *
	 * @param Alpha The new Alpha value.
	 * @return A new FColor with the updated Alpha.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "WithAlpha"))
	static FColor Color_WithAlpha(const FColor& Host, uint8 Alpha);

	/**
	 * Converts this color value to a hexadecimal string.
	 *
	 * @return The hexadecimal string.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "ToHex"))
	static FString Color_ToHex(const FColor& Host);

	/**
	 * Converts this color value to a string.
	 *
	 * @return The string representation.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "ToString"))
	static FString Color_ToString(const FColor& Host);

	/**
	 * Gets the color in a packed int32 format packed in the order ARGB.
	 *
	 * @return The packed color.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "ToPackedARGB"))
	static int32 Color_ToPackedARGB(const FColor& Host);

	/**
	 * Gets the color in a packed int32 format packed in the order ABGR.
	 *
	 * @return The packed color.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "ToPackedABGR"))
	static int32 Color_ToPackedABGR(const FColor& Host);

	/**
	 * Gets the color in a packed int32 format packed in the order RGBA.
	 *
	 * @return The packed color.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "ToPackedRGBA"))
	static int32 Color_ToPackedRGBA(const FColor& Host);

	/**
	 * Gets the color in a packed int32 format packed in the order BGRA.
	 *
	 * @return The packed color.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "ToPackedBGRA"))
	static int32 Color_ToPackedBGRA(const FColor& Host);

	/**
	 * Default constructor.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void Color_Init1(const FColor& Host);

	/**
	 * Constructor with RGBA components.
	 *
	 * @param InR Red component.
	 * @param InG Green component.
	 * @param InB Blue component.
	 * @param InA Alpha component.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void Color_Init2(const FColor& Host, uint8 InR, uint8 InG, uint8 InB, uint8 InA = 255);

	/**
	 * Constructor with packed int32 color.
	 *
	 * @param InColor Packed int32 color.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void Color_Init3(const FColor& Host, int32 InColor);

	// ============================================
	// FLinearColor exposed for scripting
	// ============================================

	/** White color (1, 1, 1, 1) */
	UFUNCTION(BlueprintPure, meta = (UPyScriptConstant = "White"))
	static FLinearColor LinearColor_White(const FLinearColor& Host);

	/** Black color (0, 0, 0, 1) */
	UFUNCTION(BlueprintPure, meta = (UPyScriptConstant = "Black"))
	static FLinearColor LinearColor_Black(const FLinearColor& Host);

	/** Gray color (0.5, 0.5, 0.5, 1) */
	UFUNCTION(BlueprintPure, meta = (UPyScriptConstant = "Gray"))
	static FLinearColor LinearColor_Gray(const FLinearColor& Host);

	/** Red color (1, 0, 0, 1) */
	UFUNCTION(BlueprintPure, meta = (UPyScriptConstant = "Red"))
	static FLinearColor LinearColor_Red(const FLinearColor& Host);

	/** Green color (0, 1, 0, 1) */
	UFUNCTION(BlueprintPure, meta = (UPyScriptConstant = "Green"))
	static FLinearColor LinearColor_Green(const FLinearColor& Host);

	/** Blue color (0, 0, 1, 1) */
	UFUNCTION(BlueprintPure, meta = (UPyScriptConstant = "Blue"))
	static FLinearColor LinearColor_Blue(const FLinearColor& Host);

	/** Yellow color (1, 1, 0, 1) */
	UFUNCTION(BlueprintPure, meta = (UPyScriptConstant = "Yellow"))
	static FLinearColor LinearColor_Yellow(const FLinearColor& Host);

	/** Transparent color (0, 0, 0, 0) */
	UFUNCTION(BlueprintPure, meta = (UPyScriptConstant = "Transparent"))
	static FLinearColor LinearColor_Transparent(const FLinearColor& Host);

	/**
	 * Element-wise addition.
	 *
	 * @param Color The color to add.
	 * @return The result of addition.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "+;+="))
	static FLinearColor LinearColor_Add(const FLinearColor& Host, const FLinearColor& Color);

	/**
	 * Element-wise subtraction.
	 *
	 * @param Color The color to subtract.
	 * @return The result of subtraction.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "-;-="))
	static FLinearColor LinearColor_Subtract(const FLinearColor& Host, const FLinearColor& Color);

	/**
	 * Element-wise multiplication.
	 *
	 * @param Color The color to multiply.
	 * @return The result of multiplication.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "*;*="))
	static FLinearColor LinearColor_Multiply(const FLinearColor& Host, const FLinearColor& Color);

	/**
	 * Element-wise multiplication by a scalar.
	 *
	 * @param Scalar The scalar to multiply.
	 * @return The result of multiplication.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "*;*="))
	static FLinearColor LinearColor_MultiplyFloat(const FLinearColor& Host, float Scalar);

	/**
	 * Element-wise division.
	 *
	 * @param Color The color to divide.
	 * @return The result of division.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "/;/="))
	static FLinearColor LinearColor_Divide(const FLinearColor& Host, const FLinearColor& Color);

	/**
	 * Element-wise division by a scalar.
	 *
	 * @param Scalar The scalar to divide.
	 * @return The result of division.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "/;/="))
	static FLinearColor LinearColor_DivideFloat(const FLinearColor& Host, float Scalar);

	/**
	 * Compares two colors for equality.
	 *
	 * @param Color The color to compare against.
	 * @return true if the colors are equal, false otherwise.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "=="))
	static bool LinearColor_Equal(const FLinearColor& Host, const FLinearColor& Color);

	/**
	 * Compares two colors for inequality.
	 *
	 * @param Color The color to compare against.
	 * @return true if the colors are not equal, false otherwise.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "!="))
	static bool LinearColor_NotEqual(const FLinearColor& Host, const FLinearColor& Color);

	/**
	 * Converts an FColor to a linear color.
	 *
	 * @param Color The FColor to convert.
	 * @return The linear color.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "FromSRGBColor", UPyScriptStatic))
	static FLinearColor LinearColor_FromSRGBColor(const FLinearColor& Host, const FColor& Color);

	/**
	 * Converts a linear color to an FColor.
	 *
	 * @param bSRGB Whether to convert to sRGB.
	 * @return The FColor.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "ToFColor"))
	static FColor LinearColor_ToFColor(const FLinearColor& Host, bool bSRGB);

	/**
	 * Clamps the color values between 0 and 1.
	 *
	 * @return The clamped color.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetClamped"))
	static FLinearColor LinearColor_GetClamped(const FLinearColor& Host);

	/**
	 * Returns a desaturated color.
	 *
	 * @param Desaturation The desaturation amount.
	 * @return The desaturated color.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Desaturate"))
	static FLinearColor LinearColor_Desaturate(const FLinearColor& Host, float Desaturation);

	/**
	 * Returns the luminance of the color.
	 *
	 * @return The luminance.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetLuminance"))
	static float LinearColor_GetLuminance(const FLinearColor& Host);

	/**
	 * Returns true if the color is almost black.
	 *
	 * @return true if almost black.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "IsAlmostBlack"))
	static bool LinearColor_IsAlmostBlack(const FLinearColor& Host);

	/**
	 * Returns true if the color is near equal to another color.
	 *
	 * @param Color The other color.
	 * @param Tolerance The tolerance.
	 * @return true if near equal.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Equals"))
	static bool LinearColor_Equals(const FLinearColor& Host, const FLinearColor& Color, float Tolerance = 1.e-4f);

	/**
	 * Returns the Euclidean distance between two colors.
	 *
	 * @param C1 The first color.
	 * @param C2 The second color.
	 * @return The distance.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Dist", UPyScriptStatic))
	static float LinearColor_Dist(const FLinearColor& Host, const FLinearColor& C1, const FLinearColor& C2);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void LinearColor_Init1(const FLinearColor& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void LinearColor_Init2(const FLinearColor& Host, float R, float G, float B, float A = 1.0f);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void LinearColor_Init3(const FLinearColor& Host, const FColor& Color);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void LinearColor_Init4(const FLinearColor& Host, const FVector& Vector);

	// ============================================
	// FSlateColor exposed for scripting
	// ============================================

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void SlateColor_Init1(const FSlateColor& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void SlateColor_Init2(const FSlateColor& Host, const FLinearColor& InColor);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void SlateColor_Init3(const FSlateColor& Host, const FColor& InColor);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void SlateColor_Init4(const FSlateColor& Host, EStyleColor InStyleColor);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetSpecifiedColor"))
	static FLinearColor SlateColor_GetSpecifiedColor(const FSlateColor& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "IsColorSpecified"))
	static bool SlateColor_IsColorSpecified(const FSlateColor& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "UseForeground", UPyScriptStatic))
	static FSlateColor SlateColor_UseForeground(const FSlateColor& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "UseSubduedForeground", UPyScriptStatic))
	static FSlateColor SlateColor_UseSubduedForeground(const FSlateColor& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "=="))
	static bool SlateColor_Equal(const FSlateColor& Host, const FSlateColor& Other);

	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "!="))
	static bool SlateColor_NotEqual(const FSlateColor& Host, const FSlateColor& Other);

	// ============================================
	// FKey exposed for scripting
	// ============================================

	/**
	 * Returns true if the key is valid.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "IsValid"))
	static bool Key_IsValid(const FKey& Host);

	/**
	 * Returns true if the key is a modifier key: Ctrl, Shift, Alt, Cmd, Menu, CapsLock
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "IsModifierKey"))
	static bool Key_IsModifierKey(const FKey& Host);

	/**
	 * Returns true if the key is a gamepad button
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "IsGamepadKey"))
	static bool Key_IsGamepadKey(const FKey& Host);

	/**
	 * Returns true if the key is a mouse button
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "IsMouseButton"))
	static bool Key_IsMouseButton(const FKey& Host);

	/**
	 * Returns true if the key is a touch input
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "IsTouch"))
	static bool Key_IsTouch(const FKey& Host);

	/**
	 * Returns the display name of the key.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetDisplayName"))
	static FText Key_GetDisplayName(const FKey& Host);

	/**
	 * Returns the name of the key.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetFName"))
	static FName Key_GetFName(const FKey& Host);

	/**
	 * Returns the string representation of the key.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "ToString"))
	static FString Key_ToString(const FKey& Host);

	/**
	 * Equality operator.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "=="))
	static bool Key_Equal(const FKey& Host, const FKey& Other);

	/**
	 * Inequality operator.
	 */
	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "!="))
	static bool Key_NotEqual(const FKey& Host, const FKey& Other);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void Key_Init1(const FKey& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void Key_Init2(const FKey& Host, FName InName);

	// ============================================
	// FGuid exposed for scripting
	// ============================================

	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "=="))
	static bool Guid_Equal(const FGuid& Host, const FGuid& Other);

	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "!="))
	static bool Guid_NotEqual(const FGuid& Host, const FGuid& Other);

	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "<"))
	static bool Guid_Less(const FGuid& Host, const FGuid& Other);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "IsValid"))
	static bool Guid_IsValid(const FGuid& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Invalidate"))
	static void Guid_Invalidate(FGuid& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "ToString"))
	static FString Guid_ToString(const FGuid& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "NewGuid", UPyScriptStatic))
	static FGuid Guid_NewGuid(const FGuid& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Parse", UPyScriptStatic))
	static bool Guid_Parse(const FGuid& Host, const FString& GuidString, FGuid& OutGuid);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void Guid_Init1(const FGuid& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void Guid_Init2(const FGuid& Host, int32 A, int32 B, int32 C, int32 D);

	// ============================================
	// FBox2D exposed for scripting
	// ============================================

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetCenter"))
	static FVector2D Box2D_GetCenter(const FBox2D& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetSize"))
	static FVector2D Box2D_GetSize(const FBox2D& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetExtent"))
	static FVector2D Box2D_GetExtent(const FBox2D& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetArea"))
	static double Box2D_GetArea(const FBox2D& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "ComputeSquaredDistanceToPoint"))
	static double Box2D_ComputeSquaredDistanceToPoint(const FBox2D& Host, const FVector2D& Point);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "ExpandBy"))
	static FBox2D Box2D_ExpandBy(const FBox2D& Host, double W);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "IsInside"))
	static bool Box2D_IsInside(const FBox2D& Host, const FVector2D& TestPoint);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Intersect"))
	static bool Box2D_Intersect(const FBox2D& Host, const FBox2D& Other);

	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "+"))
	static FBox2D Box2D_ShiftBy(const FBox2D& Host, const FVector2D& Offset);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "ToString"))
	static FString Box2D_ToString(const FBox2D& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "=="))
	static bool Box2D_Equal(const FBox2D& Host, const FBox2D& Other);

	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "!="))
	static bool Box2D_NotEqual(const FBox2D& Host, const FBox2D& Other);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void Box2D_Init1(const FBox2D& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void Box2D_Init2(const FBox2D& Host, const FVector2D& InMin, const FVector2D& InMax);

	// ============================================
	// FBox exposed for scripting
	// ============================================

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetCenter"))
	static FVector Box_GetCenter(const FBox& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetSize"))
	static FVector Box_GetSize(const FBox& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetExtent"))
	static FVector Box_GetExtent(const FBox& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetVolume"))
	static double Box_GetVolume(const FBox& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "ComputeSquaredDistanceToPoint"))
	static double Box_ComputeSquaredDistanceToPoint(const FBox& Host, const FVector& Point);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "ExpandBy"))
	static FBox Box_ExpandBy(const FBox& Host, double W);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "IsInside"))
	static bool Box_IsInside(const FBox& Host, const FVector& TestPoint);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Intersect"))
	static bool Box_Intersect(const FBox& Host, const FBox& Other);

	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "+"))
	static FBox Box_ShiftBy(const FBox& Host, const FVector& Offset);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "MoveTo"))
	static FBox Box_MoveTo(const FBox& Host, const FVector& Destination);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "ToString"))
	static FString Box_ToString(const FBox& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "=="))
	static bool Box_Equal(const FBox& Host, const FBox& Other);

	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "!="))
	static bool Box_NotEqual(const FBox& Host, const FBox& Other);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void Box_Init1(const FBox& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void Box_Init2(const FBox& Host, const FVector& InMin, const FVector& InMax);

	// ============================================
	// FSoftObjectPath exposed for scripting
	// ============================================

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "ToString"))
	static FString SoftObjectPath_ToString(const FSoftObjectPath& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetAssetPathString"))
	static FString SoftObjectPath_GetAssetPathString(const FSoftObjectPath& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetLongPackageName"))
	static FString SoftObjectPath_GetLongPackageName(const FSoftObjectPath& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetAssetName"))
	static FString SoftObjectPath_GetAssetName(const FSoftObjectPath& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "IsValid"))
	static bool SoftObjectPath_IsValid(const FSoftObjectPath& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Reset"))
	static void SoftObjectPath_Reset(FSoftObjectPath& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "=="))
	static bool SoftObjectPath_Equal(const FSoftObjectPath& Host, const FSoftObjectPath& Other);

	UFUNCTION(BlueprintPure, meta=(UPyScriptOperator = "!="))
	static bool SoftObjectPath_NotEqual(const FSoftObjectPath& Host, const FSoftObjectPath& Other);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void SoftObjectPath_Init1(const FSoftObjectPath& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void SoftObjectPath_Init2(const FSoftObjectPath& Host, const FString& PathString);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void SoftObjectPath_Init3(const FSoftObjectPath& Host, const UObject* InObject);

	// ============================================
	// FSoftClassPath exposed for scripting
	// ============================================

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void SoftClassPath_Init1(const FSoftClassPath& Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void SoftClassPath_Init2(const FSoftClassPath& Host, const FString& PathString);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "__init__"))
	static void SoftClassPath_Init3(const FSoftClassPath& Host, const UClass* InClass);

	// ============================================
	// ACharacter exposed for scripting
	// ============================================

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetMesh"))
	static USkeletalMeshComponent* Character_GetMesh(ACharacter* Host);

	// ============================================
	// ATriggerBase exposed for scripting
	// ============================================
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetCollisionComponent"))
	static UShapeComponent* TriggerBase_GetCollisionComponent(ATriggerBase* Host);

	// ============================================
	// UProgressBar exposed for scripting
	// ============================================
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetPercent"))
	static float ProgressBar_GetPercent(UProgressBar* Host);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetBarFillStyle"))
	static EProgressBarFillStyle::Type ProgressBar_GetBarFillStyle(UProgressBar* Host);

	UFUNCTION(BlueprintCallable, meta=(UPyScriptMethod = "SetBarFillStyle"))
	static void ProgressBar_SetBarFillStyle(UProgressBar* Host, EProgressBarFillStyle::Type InBarFillStyle);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetBarFillType"))
	static EProgressBarFillType::Type ProgressBar_GetBarFillType(UProgressBar* Host);

	UFUNCTION(BlueprintCallable, meta=(UPyScriptMethod = "SetBarFillType"))
	static void ProgressBar_SetBarFillType(UProgressBar* Host, EProgressBarFillType::Type InBarFillType);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetBorderPadding"))
	static FVector2D ProgressBar_GetBorderPadding(UProgressBar* Host);

	UFUNCTION(BlueprintCallable, meta=(UPyScriptMethod = "SetBorderPadding"))
	static void ProgressBar_SetBorderPadding(UProgressBar* Host, FVector2D InBorderPadding);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetWidgetStyle"))
	static FProgressBarStyle ProgressBar_GetWidgetStyle(UProgressBar* Host);

	UFUNCTION(BlueprintCallable, meta=(UPyScriptMethod = "SetWidgetStyle"))
	static void ProgressBar_SetWidgetStyle(UProgressBar* Host, FProgressBarStyle InWidgetStyle);

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "UseMarquee"))
	static bool ProgressBar_UseMarquee(UProgressBar* Host);

	// ============================================
	// UEditableTextBox exposed for scripting
	// ============================================
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetHintText"))
	static FText EditableTextBox_GetHintText(UEditableTextBox* Host);

	// ============================================
	// UGameViewportSubsystem exposed for scripting
	// ============================================

	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "Get", UPyScriptStatic))
	static UGameViewportSubsystem* GameViewportSubsystem_Get(UGameViewportSubsystem* Host);

	// ============================================
	// UWidget exposed for scripting
	// ============================================
	UFUNCTION(BlueprintPure, meta=(UPyScriptMethod = "GetRenderTransform"))
	static const FWidgetTransform& Widget_GetRenderTransform(UWidget* Host);
#endif
};

