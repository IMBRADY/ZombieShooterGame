#pragma once

#include "CoreMinimal.h"
#include "Audio/ZombieAudioTypes.h"
#include "Visual/ZombieEffectTypes.h"
#include "ZombieProjectileTypes.generated.h"

class USpriteSheetDataAsset;
class UZombieHazardDataAsset;

/**
 * How a projectile looks and flies - owned by whatever fires it (a weapon Data Asset, a zombie
 * ability). The damage it carries is not here: that comes from the firer's effective stats at the
 * moment of firing, so perks and difficulty scaling apply without the projectile knowing about them.
 */
USTRUCT(BlueprintType)
struct FZombieProjectileSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<USpriteSheetDataAsset> Sprite;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "0.05"))
	float SpriteScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "1.0"))
	float Speed = 2600.0f;

	/** 0 flies straight; above 0 the projectile is lobbed in an arc onto its aim point. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "0.0"))
	float GravityScale = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "1.0"))
	float CollisionRadius = 14.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "0.1"))
	float Lifetime = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
	FZombieEffectSpec ImpactEffect;

	/** Left on the floor where it lands - scorch marks, acid splashes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
	FZombieEffectSpec ImpactDecal;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
	FZombieSoundSpec ImpactSound;

	/** Optional hazard left where it lands, e.g. the Lobber's acid puddle. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UZombieHazardDataAsset> ImpactHazard;
};
