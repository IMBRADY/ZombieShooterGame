#pragma once

#include "CoreMinimal.h"
#include "Core/ZombieStatTypes.h"
#include "GameFramework/Actor.h"
#include "Weapons/WeaponTypes.h"
#include "ZombieWeapon.generated.h"

class UWeaponDataAsset;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnWeaponAmmoChanged, int32 /*AmmoInMagazine*/, int32 /*ReserveAmmo*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnWeaponReloadChanged, bool /*bReloading*/, float /*Duration*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnWeaponFired, float /*ShakeStrength*/);

/** Why a trigger pull did not produce a shot - the caller decides whether to click or auto-reload. */
enum class EWeaponFireResult : uint8
{
	Fired,
	Cooldown,
	Reloading,
	EmptyMagazine,
	NoDefinition
};

/**
 * The "Weapon Actor" stage of the weapon pipeline (BaseWeapon in the spec's layout).
 *
 * Owns one gun's mutable state - which Data Asset, rarity roll, upgrade, ammo, reload - and the
 * rules around a trigger pull (fire rate, magazine, auto-reload). How the round travels is
 * delegated to the Data Asset's Fire Mode class; what it does on arrival to FZombieHitResolver.
 * Nothing here is specific to any one gun.
 */
UCLASS(NotPlaceable)
class ZOMBIEGAME_API AZombieWeapon : public AActor
{
	GENERATED_BODY()

public:
	AZombieWeapon();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Sets the gun up from saved/shop data. Ammo of -1 means "full". */
	void InitializeFromInstance(const FWeaponInstanceData& Instance);
	FWeaponInstanceData ToInstanceData() const;

	/** Recomputes effective stats - call when the owner's perks change. Keeps current ammo. */
	void RefreshStats();

	/** Attempts one shot toward AimDirection (flattened to the play plane). */
	EWeaponFireResult TryFire(const FVector& AimDirection);

	/** Starts a reload if there is room in the magazine and spare ammo to fill it. */
	bool StartReload();
	void CancelReload();

	void Upgrade();
	void RefillAmmo();

	/** Adds spare rounds, capped at the reserve maximum. Returns how many were actually added. */
	int32 AddReserveAmmo(int32 Amount);

	const UWeaponDataAsset* GetDefinition() const { return Definition; }
	EWeaponRarity GetRarity() const { return Rarity; }
	int32 GetUpgradeLevel() const { return UpgradeLevel; }
	bool IsUpgraded() const { return UpgradeLevel > 0; }
	int32 GetAmmoInMagazine() const { return AmmoInMagazine; }
	int32 GetReserveAmmo() const { return ReserveAmmo; }
	bool IsReloading() const { return bReloading; }
	bool IsAutomatic() const;
	const FWeaponStats& GetStats() const { return EffectiveStats; }
	float GetReloadProgress() const;

	/** Called while the weapon is holstered/equipped, so a holstered gun never finishes a reload. */
	void SetEquipped(bool bEquipped);

	FOnWeaponAmmoChanged OnAmmoChanged;
	FOnWeaponReloadChanged OnReloadChanged;
	FOnWeaponFired OnFired;

private:
	void FinishReload();
	void BroadcastAmmo();
	void PlayFireFeedback(const FVector& Muzzle, const FVector& Direction);
	FVector GetMuzzleLocation(const FVector& Direction) const;

	UPROPERTY(Replicated)
	TObjectPtr<UWeaponDataAsset> Definition;

	UPROPERTY(Replicated)
	EWeaponRarity Rarity = EWeaponRarity::Common;

	UPROPERTY(Replicated)
	int32 UpgradeLevel = 0;

	UPROPERTY(Replicated)
	int32 AmmoInMagazine = 0;

	UPROPERTY(Replicated)
	int32 ReserveAmmo = 0;

	UPROPERTY(Replicated)
	bool bReloading = false;

	FWeaponStats EffectiveStats;
	TArray<FHitEffectSpec> EffectiveHitEffects;

	double LastFireTime = -1000.0;
	double ReloadStartTime = 0.0;
	FTimerHandle ReloadTimer;
};
