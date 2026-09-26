#pragma once

#include "CoreMinimal.h"
#include "Weapons/WeaponFireMode.h"
#include "WeaponFireMode_Projectile.generated.h"

/**
 * Physical rounds: one pooled AZombieProjectile per pellet, flying with the weapon Data Asset's
 * projectile spec and carrying the shot's effective stats. Rocket launchers and other explosive
 * weapons are this mode plus an ExplosionRadius - no dedicated class.
 */
UCLASS(BlueprintType)
class ZOMBIEGAME_API UWeaponFireMode_Projectile : public UWeaponFireMode
{
	GENERATED_BODY()

public:
	virtual void Fire(const FWeaponFireContext& Context) const override;
};
