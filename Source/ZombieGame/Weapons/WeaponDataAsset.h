#pragma once

#include "CoreMinimal.h"
#include "Audio/ZombieAudioTypes.h"
#include "Core/ZombieStatTypes.h"
#include "Engine/DataAsset.h"
#include "Visual/ZombieEffectTypes.h"
#include "Weapons/Projectile/ZombieProjectileTypes.h"
#include "Weapons/WeaponTypes.h"
#include "WeaponDataAsset.generated.h"

class UTexture2D;
class UWeaponFireMode;

/** What the one-time upgrade does to a weapon ("Damage, Magazine, Reload Speed, Visual Effects"). */
USTRUCT(BlueprintType)
struct FWeaponUpgradeBonus
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade", meta = (ClampMin = "1.0"))
	float DamageMultiplier = 1.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade", meta = (ClampMin = "1.0"))
	float MagazineMultiplier = 1.3f;

	/** Below 1 is faster. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float ReloadTimeMultiplier = 0.8f;

	/** Upgraded guns fire visibly different tracers. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Upgrade")
	FLinearColor UpgradedTracerColor = FLinearColor(0.4f, 0.9f, 1.0f);
};

/**
 * A gun, entirely as data (prompt.txt "Weapons must be Data Assets ... No weapon should require
 * code modification"). The pipeline is:
 *
 *     Weapon Actor -> Weapon Data Asset -> Fire Mode -> Effects -> Projectile or Hitscan
 *
 * A new weapon is a new one of these: its numbers, which fire mode class it uses (hitscan or
 * projectile), which status effects its rounds carry, and how it looks and sounds.
 */
UCLASS(BlueprintType)
class ZOMBIEGAME_API UWeaponDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	static const FPrimaryAssetType AssetType;
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	EWeaponCategory Category = EWeaponCategory::Pistol;

	// --- Behaviour ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Firing")
	FWeaponStats BaseStats;

	/** Holding the trigger keeps firing. Semi-automatic weapons need a click per shot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Firing")
	bool bAutomatic = false;

	/** Hitscan or projectile - the strategy the weapon actor delegates each shot to. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Firing")
	TSubclassOf<UWeaponFireMode> FireMode;

	/** Used when FireMode is a projectile mode. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Firing")
	FZombieProjectileSpec Projectile;

	/** Status effects every round carries, whatever the rarity. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Firing")
	TArray<FHitEffectSpec> HitEffects;

	/** The "unique modifier" only a Legendary roll of this weapon gets. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Firing")
	TArray<FHitEffectSpec> LegendaryHitEffects;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Firing")
	FWeaponUpgradeBonus UpgradeBonus;

	// --- Economy ---

	/** Common-rarity shop price; rarity multiplies it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Economy", meta = (ClampMin = "0"))
	int32 BasePrice = 200;

	/** Base cost of the one-time upgrade; rarity multiplies it ("cost scales with rarity"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Economy", meta = (ClampMin = "0"))
	int32 BaseUpgradePrice = 350;

	/** Price of topping the magazine and reserve back up at the shop. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Economy", meta = (ClampMin = "0"))
	int32 AmmoRefillPrice = 60;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Economy", meta = (ClampMin = "0.0"))
	float ShopWeight = 1.0f;

	/** Sector from which the shop and mystery box may offer it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Economy", meta = (ClampMin = "1"))
	int32 MinSector = 1;

	/** Clear for starting weapons or boss-only drops that should never appear in the shop. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Economy")
	bool bAvailableInShop = true;

	/** Whether random weapon drops (chests, boss rewards, the Mystery Box) can produce it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Economy")
	bool bCanDropAsLoot = true;

	// --- Presentation ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	TObjectPtr<UTexture2D> Icon;

	/** Frame of the player's held-weapon overlay sheet showing this gun. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation", meta = (ClampMin = "0"))
	int32 HeldSpriteFrame = 0;

	/** How far in front of the carrier's centre shots leave the barrel. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation", meta = (ClampMin = "0.0"))
	float MuzzleOffset = 70.0f;

	/**
	 * Extra width a hitscan round has against bodies (walls still use a thin line). Collision
	 * capsules are narrower than the sprites drawn on them, so without this a round that visibly
	 * crossed a zombie's shoulder could pass straight by its capsule.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation", meta = (ClampMin = "0.0"))
	float ShotHitRadius = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	FZombieEffectSpec MuzzleFlash;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	FZombieEffectSpec ImpactEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	bool bDrawTracers = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	FLinearColor TracerColor = FLinearColor(1.0f, 0.85f, 0.4f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation", meta = (ClampMin = "1.0"))
	float TracerWidth = 10.0f;

	/** 0..1 camera shake per shot. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ShakeStrength = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	FZombieSoundSpec FireSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	FZombieSoundSpec ReloadSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	FZombieSoundSpec EmptySound;
};
