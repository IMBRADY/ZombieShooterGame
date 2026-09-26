#pragma once

#include "CoreMinimal.h"
#include "Weapons/WeaponFireMode.h"
#include "WeaponFireMode_Hitscan.generated.h"

/**
 * Instant-hit rounds: each pellet is a trace that can pierce through enemies and ricochet off
 * walls (both perk-driven counts). A ricochet looks for a visible zombie near the bounce point
 * and heads for it, so the Ricochet perk feels like skill rather than luck.
 */
UCLASS(BlueprintType)
class ZOMBIEGAME_API UWeaponFireMode_Hitscan : public UWeaponFireMode
{
	GENERATED_BODY()

public:
	virtual void Fire(const FWeaponFireContext& Context) const override;

private:
	void FirePellet(const FWeaponFireContext& Context, const FVector& Direction) const;

	/** Redirects a bounced shot toward the nearest visible enemy, if one is close enough. */
	FVector FindRicochetDirection(const FWeaponFireContext& Context, const FVector& BouncePoint,
		const FVector& ReflectedDirection, const TArray<AActor*>& AlreadyHit) const;
};
