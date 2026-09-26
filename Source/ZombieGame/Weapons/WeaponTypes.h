#pragma once

#include "CoreMinimal.h"
#include "WeaponTypes.generated.h"

class UWeaponDataAsset;

UENUM(BlueprintType)
enum class EWeaponRarity : uint8
{
	Common		UMETA(DisplayName = "Common"),
	Rare		UMETA(DisplayName = "Rare"),
	Epic		UMETA(DisplayName = "Epic"),
	Legendary	UMETA(DisplayName = "Legendary")
};

/** What kind of gun it is - drives which held-weapon art the player shows and shop grouping. */
UENUM(BlueprintType)
enum class EWeaponCategory : uint8
{
	Pistol			UMETA(DisplayName = "Pistol"),
	SMG				UMETA(DisplayName = "SMG"),
	Shotgun			UMETA(DisplayName = "Shotgun"),
	Rifle			UMETA(DisplayName = "Rifle"),
	Sniper			UMETA(DisplayName = "Sniper"),
	RocketLauncher	UMETA(DisplayName = "Rocket Launcher"),
	Special			UMETA(DisplayName = "Special")
};

/**
 * Every number that defines how a gun behaves. A weapon Data Asset holds the base block; rarity,
 * the one-time upgrade and the owner's perks each transform it into the effective block the
 * weapon actually fires with (FWeaponStatsCalculator).
 */
USTRUCT(BlueprintType)
struct FWeaponStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0.0"))
	float Damage = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0.1"))
	float ShotsPerSecond = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "1"))
	int32 MagazineSize = 12;

	/** Spare rounds the gun can carry beyond the loaded magazine. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0"))
	int32 MaxReserveAmmo = 60;

	/** Seconds to reload. Lower is better - Reload Speed perks shrink this. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0.05"))
	float ReloadTime = 1.4f;

	/** Half-angle cone, in degrees, each pellet is scattered within. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0.0", ClampMax = "45.0"))
	float SpreadDegrees = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "1"))
	int32 PelletsPerShot = 1;

	/** 0..1. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float CritChance = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "1.0"))
	float CritMultiplier = 2.0f;

	/** Extra enemies a shot passes through after its first hit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0"))
	int32 Pierce = 0;

	/** Bounces off walls before the shot is spent. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0"))
	int32 Ricochet = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "100.0"))
	float Range = 4000.0f;

	/** Radius of the blast for explosive weapons; 0 for ordinary guns. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0.0"))
	float ExplosionRadius = 0.0f;

	/** How far away zombies hear it - "gunshots attract zombies very well". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats", meta = (ClampMin = "0.0"))
	float NoiseRange = 3500.0f;
};

/**
 * One gun the player owns, reduced to what survives between sectors, shops and save files: which
 * weapon, how good a roll it is, whether it has been upgraded, and how much ammo is left.
 */
USTRUCT(BlueprintType)
struct FWeaponInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TSoftObjectPtr<UWeaponDataAsset> Definition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	EWeaponRarity Rarity = EWeaponRarity::Common;

	/** 0 or 1 - "every weapon may be upgraded exactly once". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	int32 UpgradeLevel = 0;

	/** -1 means "fresh from the box": a full magazine and full reserve once stats are known. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	int32 AmmoInMagazine = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	int32 ReserveAmmo = -1;

	bool IsValid() const { return !Definition.IsNull(); }
};
