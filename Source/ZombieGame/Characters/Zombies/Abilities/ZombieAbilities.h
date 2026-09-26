#pragma once

#include "CoreMinimal.h"
#include "Characters/Zombies/Abilities/ZombieAbility.h"
#include "Core/ZombieStatTypes.h"
#include "Weapons/Projectile/ZombieProjectileTypes.h"
#include "ZombieAbilities.generated.h"

class AZombieCharacter;
class UZombieArchetypeDataAsset;
class UZombieHazardDataAsset;

/** Throws or lobs projectiles at the target - the Lobber's acid, a boss's volley, a necromancer's bolt. */
UCLASS(BlueprintType, meta = (DisplayName = "Throw Projectile"))
class ZOMBIEGAME_API UZombieAbility_Projectile : public UZombieAbility
{
	GENERATED_BODY()

protected:
	virtual void ActivateInternal(AZombieCharacter& Owner, AActor* Target) override;

	UPROPERTY(EditAnywhere, Category = "Projectile")
	FZombieProjectileSpec Projectile;

	/** Damage as a multiple of the zombie's (difficulty-scaled) attack damage. */
	UPROPERTY(EditAnywhere, Category = "Projectile", meta = (ClampMin = "0.0"))
	float DamageMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Projectile", meta = (ClampMin = "1"))
	int32 Count = 1;

	/** Total fan angle across Count projectiles. */
	UPROPERTY(EditAnywhere, Category = "Projectile", meta = (ClampMin = "0.0"))
	float FanDegrees = 0.0f;

	/** Arcs onto where the target stands instead of flying straight. */
	UPROPERTY(EditAnywhere, Category = "Projectile")
	bool bLobbed = false;

	/** Scatter around the aim point for lobbed shots, so standing still isn't the only answer. */
	UPROPERTY(EditAnywhere, Category = "Projectile", meta = (ClampMin = "0.0"))
	float LobScatter = 80.0f;

	UPROPERTY(EditAnywhere, Category = "Projectile", meta = (ClampMin = "0.0"))
	float ExplosionRadius = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Projectile")
	TArray<FHitEffectSpec> HitEffects;

private:
	FVector ComputeLobVelocity(const AZombieCharacter& Owner, const FVector& Start, const FVector& Goal) const;
};

/**
 * Blows up on death (always) and, optionally, on reaching its target - the Exploder. The blast
 * hurts zombies too, so luring a pack around one is a real tactic.
 */
UCLASS(BlueprintType, meta = (DisplayName = "Explode"))
class ZOMBIEGAME_API UZombieAbility_Explode : public UZombieAbility
{
	GENERATED_BODY()

public:
	virtual bool IsActivatable() const override { return bDetonateOnContact; }
	virtual void OnOwnerDied(AZombieCharacter& Owner) override;

protected:
	virtual void ActivateInternal(AZombieCharacter& Owner, AActor* Target) override;

	UPROPERTY(EditAnywhere, Category = "Explosion")
	bool bDetonateOnContact = true;

	UPROPERTY(EditAnywhere, Category = "Explosion", meta = (ClampMin = "0.0"))
	float Radius = 320.0f;

	UPROPERTY(EditAnywhere, Category = "Explosion", meta = (ClampMin = "0.0"))
	float Damage = 45.0f;

	UPROPERTY(EditAnywhere, Category = "Explosion")
	FZombieEffectSpec ExplosionEffect;

	UPROPERTY(EditAnywhere, Category = "Explosion")
	FZombieEffectSpec ScorchDecal;

	UPROPERTY(EditAnywhere, Category = "Explosion")
	FZombieSoundSpec ExplosionSound;

private:
	bool bExploded = false;
};

/** Leaves hazards behind as it walks, and a bigger one where it dies - the Poison zombie. */
UCLASS(BlueprintType, meta = (DisplayName = "Hazard Trail"))
class ZOMBIEGAME_API UZombieAbility_HazardTrail : public UZombieAbility
{
	GENERATED_BODY()

public:
	virtual bool IsActivatable() const override { return false; }
	virtual void OnOwnerSpawned(AZombieCharacter& Owner) override;
	virtual void OnOwnerDied(AZombieCharacter& Owner) override;

protected:
	virtual void ActivateInternal(AZombieCharacter& Owner, AActor* Target) override {}

	UPROPERTY(EditAnywhere, Category = "Hazard")
	TObjectPtr<UZombieHazardDataAsset> TrailHazard;

	UPROPERTY(EditAnywhere, Category = "Hazard", meta = (ClampMin = "0.5"))
	float TrailInterval = 2.5f;

	UPROPERTY(EditAnywhere, Category = "Hazard")
	TObjectPtr<UZombieHazardDataAsset> DeathHazard;

private:
	void DropTrail();

	TWeakObjectPtr<AZombieCharacter> OwnerZombie;
	FTimerHandle TrailTimer;
};

/** Raises nearby corpses as fresh zombies - the Necromancer. */
UCLASS(BlueprintType, meta = (DisplayName = "Revive Dead"))
class ZOMBIEGAME_API UZombieAbility_Revive : public UZombieAbility
{
	GENERATED_BODY()

protected:
	virtual bool CanActivateInternal(const AZombieCharacter& Owner, const AActor* Target) const override;
	virtual void ActivateInternal(AZombieCharacter& Owner, AActor* Target) override;

	UPROPERTY(EditAnywhere, Category = "Revive", meta = (ClampMin = "50.0"))
	float ReviveRadius = 900.0f;

	UPROPERTY(EditAnywhere, Category = "Revive", meta = (ClampMin = "1"))
	int32 MaxRevivedPerCast = 3;
};

/** Dashes at the target, hitting and knocking back whatever it runs into - the Butcher's charge. */
UCLASS(BlueprintType, meta = (DisplayName = "Charge"))
class ZOMBIEGAME_API UZombieAbility_Charge : public UZombieAbility
{
	GENERATED_BODY()

protected:
	virtual void ActivateInternal(AZombieCharacter& Owner, AActor* Target) override;

	UPROPERTY(EditAnywhere, Category = "Charge", meta = (ClampMin = "100.0"))
	float ChargeSpeed = 1500.0f;

	UPROPERTY(EditAnywhere, Category = "Charge", meta = (ClampMin = "0.0"))
	float Damage = 30.0f;

	UPROPERTY(EditAnywhere, Category = "Charge", meta = (ClampMin = "0.0"))
	float Knockback = 900.0f;

	/** Pause before launching - the tell. */
	UPROPERTY(EditAnywhere, Category = "Charge", meta = (ClampMin = "0.0"))
	float Windup = 0.5f;

private:
	void Launch();
	void CheckContact();
	void EndCharge();

	TWeakObjectPtr<AZombieCharacter> OwnerZombie;
	TWeakObjectPtr<AActor> ChargeTarget;
	FVector ChargeDirection = FVector::ZeroVector;
	FTimerHandle PhaseTimer;
	FTimerHandle ContactTimer;
	float SavedGroundFriction = 8.0f;
	float SavedBraking = 2048.0f;
	bool bHitLanded = false;
};

/** Telegraphed ground slam damaging everything around the zombie. */
UCLASS(BlueprintType, meta = (DisplayName = "Ground Slam"))
class ZOMBIEGAME_API UZombieAbility_Slam : public UZombieAbility
{
	GENERATED_BODY()

protected:
	virtual void ActivateInternal(AZombieCharacter& Owner, AActor* Target) override;

	UPROPERTY(EditAnywhere, Category = "Slam", meta = (ClampMin = "50.0"))
	float Radius = 380.0f;

	UPROPERTY(EditAnywhere, Category = "Slam", meta = (ClampMin = "0.0"))
	float Damage = 35.0f;

	UPROPERTY(EditAnywhere, Category = "Slam", meta = (ClampMin = "0.0"))
	float Windup = 0.7f;

	UPROPERTY(EditAnywhere, Category = "Slam")
	FZombieEffectSpec ImpactEffect;

	UPROPERTY(EditAnywhere, Category = "Slam")
	FZombieSoundSpec ImpactSound;

private:
	void Strike();

	TWeakObjectPtr<AZombieCharacter> OwnerZombie;
	FTimerHandle StrikeTimer;
};

/** Calls in minions around itself - bosses. */
UCLASS(BlueprintType, meta = (DisplayName = "Summon Minions"))
class ZOMBIEGAME_API UZombieAbility_Summon : public UZombieAbility
{
	GENERATED_BODY()

protected:
	virtual bool CanActivateInternal(const AZombieCharacter& Owner, const AActor* Target) const override;
	virtual void ActivateInternal(AZombieCharacter& Owner, AActor* Target) override;

	UPROPERTY(EditAnywhere, Category = "Summon")
	TObjectPtr<UZombieArchetypeDataAsset> MinionArchetype;

	UPROPERTY(EditAnywhere, Category = "Summon", meta = (ClampMin = "1"))
	int32 CountPerCast = 4;

	/** Summons stop while this many of its minions are still alive. */
	UPROPERTY(EditAnywhere, Category = "Summon", meta = (ClampMin = "1"))
	int32 MaxAliveMinions = 8;

private:
	TArray<TWeakObjectPtr<AActor>> Minions;
};
