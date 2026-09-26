#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ShopSettings.generated.h"

class ULootTableDataAsset;

/**
 * The intermission shop's economy, as data: what's stocked, what everything costs, and how prices
 * climb as the run goes deeper. "Every shop visit should feel important" is tuned here.
 */
UCLASS(BlueprintType)
class ZOMBIEGAME_API UShopSettings : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	static const FPrimaryAssetType AssetType;
	static const TCHAR* DefaultAssetPath;
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	static const UShopSettings* GetOrLoadDefault();

	// --- Stock ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stock", meta = (ClampMin = "0"))
	int32 MinWeaponOffers = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stock", meta = (ClampMin = "0"))
	int32 MaxWeaponOffers = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stock", meta = (ClampMin = "0"))
	int32 MinPerkOffers = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stock", meta = (ClampMin = "0"))
	int32 MaxPerkOffers = 3;

	// --- Supplies ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Supplies", meta = (ClampMin = "0"))
	int32 ArmorAmount = 50;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Supplies", meta = (ClampMin = "0"))
	int32 ArmorPrice = 175;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Supplies", meta = (ClampMin = "0"))
	int32 MedKitHealAmount = 50;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Supplies", meta = (ClampMin = "0"))
	int32 MedKitPrice = 150;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Supplies", meta = (ClampMin = "0"))
	int32 MysteryBoxPrice = 450;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Supplies")
	TObjectPtr<ULootTableDataAsset> MysteryBoxTable;

	/** First extra gun slot; each further slot costs SlotUpgradeGrowth times the last. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Supplies", meta = (ClampMin = "0"))
	int32 SlotUpgradeBasePrice = 900;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Supplies", meta = (ClampMin = "1.0"))
	float SlotUpgradeGrowth = 2.0f;

	/** Slots the player starts a run with - the design's "inventory of 3 guns". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Supplies", meta = (ClampMin = "1"))
	int32 StartingSlots = 3;

	// --- Inflation ---

	/** Every price rises by this fraction per sector cleared, so money never stops being tight. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Economy", meta = (ClampMin = "0.0"))
	float PriceIncreasePerSector = 0.06f;
};
