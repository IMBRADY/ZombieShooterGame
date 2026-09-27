#pragma once

#include "CoreMinimal.h"
#include "Weapons/WeaponFireMode.h"
#include "WeaponFireMode_Melee.generated.h"

/**
 * A close-quarters blow: strikes the single enemy in front of the attacker - within the weapon's
 * Range and inside its arc (UWeaponDataAsset::MeleeArcHalfAngle), with nothing solid in between -
 * favouring whoever is nearest to the aim line. Never hits more than one target.
 *
 * Zombie classes listed in the weapon's OneHitKillTiers die to a single blow whatever their
 * sector-scaled health: a shank should always drop a shambler, in sector 1 or sector 20.
 */
UCLASS(BlueprintType)
class ZOMBIEGAME_API UWeaponFireMode_Melee : public UWeaponFireMode
{
	GENERATED_BODY()

public:
	virtual void Fire(const FWeaponFireContext& Context) const override;

private:
	/** The enemy this blow lands on, or null if nobody is in reach. */
	AActor* FindTarget(const FWeaponFireContext& Context) const;

	/** Damage for this blow: the weapon's, or enough to finish a one-hit-kill class outright. */
	static float ResolveDamage(const FWeaponFireContext& Context, const AActor& Target, bool& bOutExecute);
};
