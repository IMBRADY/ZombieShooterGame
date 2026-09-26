#pragma once

#include "CoreMinimal.h"
#include "Core/ZombieStatTypes.h"
#include "Core/ZombieTeams.h"

class AController;
class UDamageType;

/** Everything needed to land one hit: how hard, how lucky, what it carries, and who to credit. */
struct FZombieHitRequest
{
	AActor* Target = nullptr;
	float BaseDamage = 0.0f;
	float CritChance = 0.0f;
	float CritMultiplier = 2.0f;
	TSubclassOf<UDamageType> DamageType;
	AController* InstigatorController = nullptr;

	/** The weapon or projectile - kill statistics ("favourite weapon") resolve from this. */
	AActor* DamageCauser = nullptr;

	FVector HitLocation = FVector::ZeroVector;
	FVector ShotDirection = FVector::ForwardVector;

	/** Status effects this hit may apply. Not owned. */
	const TArray<FHitEffectSpec>* HitEffects = nullptr;
};

struct FZombieHitResult
{
	float DamageDealt = 0.0f;
	bool bCritical = false;
};

/** An explosion: damage to everything in radius with line of sight, falling off with distance. */
struct FZombieExplosionRequest
{
	FVector Origin = FVector::ZeroVector;
	float Radius = 300.0f;
	float Damage = 50.0f;

	/** Damage fraction at the very edge of the radius. */
	float EdgeDamageFraction = 0.3f;

	/** Exploding zombies hurt other zombies too; a player's rocket never hurts players. */
	bool bFriendlyFire = false;

	AController* InstigatorController = nullptr;
	AActor* DamageCauser = nullptr;
	const TArray<FHitEffectSpec>* HitEffects = nullptr;

	/** Side the blast belongs to; resolved from DamageCauser when left as None. */
	ZombieTeams::ETeam SourceTeam = ZombieTeams::ETeam::None;
};

/**
 * The single path every weapon hit takes - hitscan, projectile, explosion - so critical hits,
 * on-hit status effects and life steal behave identically whatever fired them, and damage always
 * enters its target through Unreal's standard ApplyDamage (and therefore the target's own
 * DamageComponent and resistances).
 */
class ZOMBIEGAME_API FZombieHitResolver
{
public:
	static FZombieHitResult ApplyHit(const FZombieHitRequest& Request);

	/** Returns the number of actors damaged. */
	static int32 ApplyExplosion(UWorld* World, const FZombieExplosionRequest& Request);

private:
	static void ApplyHitEffects(const FZombieHitRequest& Request);
	static void ApplyLifeSteal(const FZombieHitRequest& Request, float DamageDealt);
};
