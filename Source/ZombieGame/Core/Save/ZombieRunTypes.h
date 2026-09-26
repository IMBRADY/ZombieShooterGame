#pragma once

#include "CoreMinimal.h"
#include "Weapons/WeaponTypes.h"
#include "ZombieRunTypes.generated.h"

class UPerkDataAsset;

/** Kills credited to one weapon over a run - "favourite weapon" on the death screen. */
USTRUCT(BlueprintType)
struct FWeaponKillCount
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	FText WeaponName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 Kills = 0;
};

/** Everything the death screen reports, accumulated over one run. */
USTRUCT(BlueprintType)
struct FZombieRunStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 Kills = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 BossesDefeated = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float DamageTaken = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 MoneyEarned = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 SectorsCleared = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	float ElapsedSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stats")
	TArray<FWeaponKillCount> KillsByWeapon;

	void AddWeaponKill(const FText& WeaponName);

	/** Most kills wins; empty when nothing has been killed with a weapon. */
	FText GetFavoriteWeapon() const;
};

/** One perk the player owns and at which tier. */
USTRUCT(BlueprintType)
struct FOwnedPerkRecord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Perk")
	TSoftObjectPtr<UPerkDataAsset> Perk;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Perk")
	int32 Tier = 0;
};

/**
 * A run checkpoint: what the spec's mid-run save must restore - "sector, difficulty, coins,
 * inventory, etc." Taken at sector starts and at the intermission, never mid-fight.
 */
USTRUCT(BlueprintType)
struct FZombieRunSaveData
{
	GENERATED_BODY()

	UPROPERTY()
	int32 RunSeed = 0;

	/** Sector to resume in (or the one just cleared, when saved in the intermission). */
	UPROPERTY()
	int32 Sector = 1;

	UPROPERTY()
	bool bInIntermission = false;

	UPROPERTY()
	float Health = 100.0f;

	UPROPERTY()
	float Armor = 0.0f;

	UPROPERTY()
	int32 Money = 0;

	UPROPERTY()
	TArray<FWeaponInstanceData> Weapons;

	UPROPERTY()
	int32 ActiveWeaponIndex = 0;

	UPROPERTY()
	int32 WeaponSlots = 3;

	UPROPERTY()
	TArray<FOwnedPerkRecord> Perks;

	UPROPERTY()
	FZombieRunStats Stats;

	/** Shop offers already bought this intermission, so a reload can't restock them. */
	UPROPERTY()
	TArray<int32> PurchasedOfferIds;

	UPROPERTY()
	FDateTime SavedAt;
};
