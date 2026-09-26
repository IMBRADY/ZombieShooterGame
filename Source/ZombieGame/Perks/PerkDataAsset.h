#pragma once

#include "CoreMinimal.h"
#include "Core/ZombieStatTypes.h"
#include "Engine/DataAsset.h"
#include "PerkDataAsset.generated.h"

class UTexture2D;

/**
 * A perk as data: what it changes per tier, how many tiers, what each tier costs.
 *
 * Perks never contain code. A perk is a list of stat modifiers (by Gameplay Tag) and on-hit status
 * effects, scaled by tier - "Speed Boost" is +8% Stat.Move.Speed per tier, "Poison Bullets" is a
 * chance to apply Status.Poisoned. Consumers ask the stat source for totals by tag, so a new perk
 * that combines existing stats is purely a new asset (ARCHITECTURE.md 5).
 */
UCLASS(BlueprintType)
class ZOMBIEGAME_API UPerkDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	static const FPrimaryAssetType AssetType;
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText DisplayName;

	/** "{0}" is replaced with the value at the tier being described. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	TObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FLinearColor IconTint = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tiers", meta = (ClampMin = "1"))
	int32 MaxTier = 5;

	/** Each modifier's value is applied once per owned tier. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tiers")
	TArray<FStatModifier> ModifiersPerTier;

	/** Status effects on every hit; Chance scales with tier (capped at 100%). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tiers")
	TArray<FHitEffectSpec> HitEffectsPerTier;

	// --- Economy: "each tier is exponentially more expensive" ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Economy", meta = (ClampMin = "0"))
	int32 BaseCost = 250;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Economy", meta = (ClampMin = "1.0"))
	float CostGrowth = 1.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Economy", meta = (ClampMin = "0.0"))
	float ShopWeight = 1.0f;

	/** Rare perks only come from boss drops and the mystery box, never the shop shelf. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Economy")
	bool bRare = false;

	/** Price of going from CurrentTier to CurrentTier + 1. */
	int32 GetCostForNextTier(int32 CurrentTier) const;

	/** Human-readable summary of what the given tier provides. */
	FText DescribeTier(int32 Tier) const;
};
