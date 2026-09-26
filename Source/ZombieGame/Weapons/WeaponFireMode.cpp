#include "WeaponFireMode.h"

FVector UWeaponFireMode::ApplySpread(const FVector& Direction, float SpreadDegrees)
{
	// Top-down game: spread is purely horizontal, so a shot never dives into the floor.
	const float YawOffset = FMath::FRandRange(-SpreadDegrees, SpreadDegrees);
	return Direction.GetSafeNormal2D().RotateAngleAxis(YawOffset, FVector::UpVector);
}
