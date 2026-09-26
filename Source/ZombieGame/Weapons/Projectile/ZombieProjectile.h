#pragma once

#include "CoreMinimal.h"
#include "Core/ZombieStatTypes.h"
#include "Core/ZombieTeams.h"
#include "GameFramework/Actor.h"
#include "Utilities/PoolableActor.h"
#include "Weapons/Projectile/ZombieProjectileTypes.h"
#include "ZombieProjectile.generated.h"

class AController;
class UDamageType;
class UPixelSpriteComponent;
class UProjectileMovementComponent;
class USphereComponent;

/** Everything a projectile needs at launch; the spec says how it flies, the rest what it does. */
struct FZombieProjectileLaunch
{
	FZombieProjectileSpec Spec;

	FVector Velocity = FVector::ZeroVector;

	float Damage = 10.0f;
	float CritChance = 0.0f;
	float CritMultiplier = 2.0f;
	int32 Pierce = 0;
	int32 Ricochet = 0;
	float ExplosionRadius = 0.0f;

	/** Explosions from this projectile also hurt its own side. */
	bool bFriendlyFire = false;

	TSubclassOf<UDamageType> DamageType;
	TArray<FHitEffectSpec> HitEffects;

	TWeakObjectPtr<AController> InstigatorController;

	/** Credited as the damage causer (the weapon, or the zombie that threw it). */
	TWeakObjectPtr<AActor> SourceActor;
};

/**
 * A pooled projectile - rockets, the Lobber's acid, boss volleys. Pawns of the other side are hit
 * by overlap (so piercing rounds keep flying); world geometry stops or bounces it. Released back to
 * UActorPoolSubsystem on impact or expiry instead of being destroyed.
 */
UCLASS(NotPlaceable)
class ZOMBIEGAME_API AZombieProjectile : public AActor, public IPoolableActor
{
	GENERATED_BODY()

public:
	AZombieProjectile();

	void Launch(const FZombieProjectileLaunch& InLaunch);

	virtual void OnAcquiredFromPool() override;
	virtual void OnReleasedToPool() override;

	/** Fires a projectile from the shared pool. Returns null only if the world is gone. */
	static AZombieProjectile* SpawnFromPool(UWorld* World, const FVector& Location, const FZombieProjectileLaunch& Launch);

protected:
	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> Movement;

	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<UPixelSpriteComponent> Sprite;

private:
	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleStop(const FHitResult& ImpactResult);

	UFUNCTION()
	void HandleBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);

	/** Damage (or blast) at a point, then back to the pool. */
	void Detonate(const FVector& Location, AActor* DirectHit);
	void DamageTarget(AActor* Target, const FVector& Location);
	void Expire();
	void ReturnToPool();

	FZombieProjectileLaunch LaunchData;
	TSet<TWeakObjectPtr<AActor>> AlreadyHit;
	ZombieTeams::ETeam SourceTeam = ZombieTeams::ETeam::None;
	int32 PierceRemaining = 0;
	int32 BouncesRemaining = 0;
	bool bActive = false;
	FTimerHandle LifetimeTimer;
};
