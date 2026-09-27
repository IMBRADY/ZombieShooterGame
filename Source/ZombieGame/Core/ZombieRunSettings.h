#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ZombieRunSettings.generated.h"

class ULootTableDataAsset;
class UWeaponDataAsset;

/**
 * The shape of a run, as data: what the player starts with, how often bosses come, and the
 * pacing of the sector -> key -> exit -> intermission loop.
 */
UCLASS(BlueprintType)
class ZOMBIEGAME_API UZombieRunSettings : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	static const FPrimaryAssetType AssetType;
	static const TCHAR* DefaultAssetPath;
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	static const UZombieRunSettings* GetOrLoadDefault();

	/** Guns a fresh run starts with, in slot order. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Start")
	TArray<TObjectPtr<UWeaponDataAsset>> StartingWeapons;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Start", meta = (ClampMin = "0"))
	int32 StartingMoney = 0;

	/**
	 * The melee weapon every player always carries in its own slot, apart from the gun slots: it
	 * can't be sold, traded or dropped, and it never needs ammo - the answer to an unlucky run
	 * that leaves every gun dry.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Start")
	TObjectPtr<UWeaponDataAsset> MeleeWeapon;

	/** "Boss sectors every 5 sectors." */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bosses", meta = (ClampMin = "1"))
	int32 BossSectorInterval = 5;

	/** What a boss drops: "Legendary weapon, Rare perk, Large money bonus". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bosses")
	TObjectPtr<ULootTableDataAsset> BossRewardTable;

	/** Linear difficulty level shown in the GameState; the real curve lives in the spawn director settings. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Difficulty", meta = (ClampMin = "0.0"))
	float DifficultyIncreasePerSector = 0.15f;

	bool IsBossSector(int32 Sector) const { return BossSectorInterval > 0 && Sector > 0 && Sector % BossSectorInterval == 0; }
};
