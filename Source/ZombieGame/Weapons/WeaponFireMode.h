#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Core/ZombieStatTypes.h"
#include "Weapons/WeaponTypes.h"
#include "WeaponFireMode.generated.h"

class AController;
class APawn;
class AZombieWeapon;
class UWeaponDataAsset;

/** One trigger pull, fully resolved: where from, which way, with what numbers. */
struct FWeaponFireContext
{
	AZombieWeapon* Weapon = nullptr;
	const UWeaponDataAsset* Definition = nullptr;
	APawn* InstigatorPawn = nullptr;
	AController* InstigatorController = nullptr;

	FVector Origin = FVector::ZeroVector;
	FVector Direction = FVector::ForwardVector;

	/** Effective stats after rarity, upgrade and perks. */
	FWeaponStats Stats;
	TArray<FHitEffectSpec> HitEffects;
	FLinearColor TracerColor = FLinearColor::White;
};

/**
 * The "Fire Mode" stage of the weapon pipeline: how a shot travels once the weapon has decided to
 * fire. Stateless strategy objects - the weapon Data Asset names a class and its default object
 * does the work - so a new way of shooting (a beam, a flamethrower cone) is one subclass and no
 * change to AZombieWeapon.
 */
UCLASS(Abstract, Const)
class ZOMBIEGAME_API UWeaponFireMode : public UObject
{
	GENERATED_BODY()

public:
	virtual void Fire(const FWeaponFireContext& Context) const PURE_VIRTUAL(UWeaponFireMode::Fire, );

protected:
	/** Direction randomised within the weapon's spread cone, flattened to the play plane. */
	static FVector ApplySpread(const FVector& Direction, float SpreadDegrees);
};
