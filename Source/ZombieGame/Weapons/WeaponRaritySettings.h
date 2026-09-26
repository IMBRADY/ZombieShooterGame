#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Weapons/WeaponTypes.h"
#include "WeaponRaritySettings.generated.h"

/** What one rarity tier does to any weapon rolled at it. */
USTRUCT(BlueprintType)
struct FWeaponRarityTier
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rarity")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rarity")
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rarity", meta = (ClampMin = "0.1"))
	float DamageMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rarity", meta = (ClampMin = "0.1"))
	float FireRateMultiplier = 1.0f;

	/** Below 1 reloads faster. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rarity", meta = (ClampMin = "0.1"))
	float ReloadTimeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rarity", meta = (ClampMin = "0.0"))
	float BonusCritChance = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rarity", meta = (ClampMin = "0"))
	int32 BonusPierce = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rarity", meta = (ClampMin = "0.0"))
	float PriceMultiplier = 1.0f;

	/** "Cost scales with rarity" for the one-time upgrade. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rarity", meta = (ClampMin = "0.0"))
	float UpgradeCostMultiplier = 1.0f;

	/** Relative chance of this tier being rolled in the shop at sector 1... */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rarity", meta = (ClampMin = "0.0"))
	float BaseRollWeight = 1.0f;

	/** ...plus this much per sector cleared, so better guns become common as the run goes deeper. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rarity")
	float RollWeightPerSector = 0.0f;
};

/**
 * The rarity table shared by every weapon: one place to rebalance "how much better is Epic than
 * Rare" without touching each weapon. Weapon-specific extras (a Legendary's unique modifier) stay
 * on the weapon's own Data Asset.
 */
UCLASS(BlueprintType)
class ZOMBIEGAME_API UWeaponRaritySettings : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UWeaponRaritySettings();

	static const FPrimaryAssetType AssetType;
	static const TCHAR* DefaultAssetPath;

	/** The project's rarity table, or class defaults (with a warning) if the asset is missing. */
	static const UWeaponRaritySettings* GetOrLoadDefault();

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rarity")
	TMap<EWeaponRarity, FWeaponRarityTier> Tiers;

	/** Sale price as a fraction of what the weapon would cost to buy. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Economy", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SellValueFraction = 0.4f;

	FWeaponRarityTier GetTier(EWeaponRarity Rarity) const;

	/** Rolls a rarity for the given sector, never below MinimumRarity. */
	EWeaponRarity RollRarity(int32 Sector, FRandomStream& Random, EWeaponRarity MinimumRarity = EWeaponRarity::Common) const;
};
