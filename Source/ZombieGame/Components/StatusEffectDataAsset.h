#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Visual/ZombieEffectTypes.h"
#include "StatusEffectDataAsset.generated.h"

class UDamageType;

/**
 * A damage-over-time condition - Burning, Poisoned - as data. Hits carry only the status's tag;
 * everything about how it behaves lives here, so "Fire Damage" and "Poison Bullets" perks, the
 * Poison zombie's puddles and a future Frost effect are all content (ARCHITECTURE.md 5).
 */
UCLASS(BlueprintType)
class ZOMBIEGAME_API UStatusEffectDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	static const FPrimaryAssetType AssetType;
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/** Resolves a status tag to its definition through the Asset Manager. Null if none is authored. */
	static const UStatusEffectDataAsset* FindByTag(const FGameplayTag& StatusTag);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status", meta = (Categories = "Status"))
	FGameplayTag StatusTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
	FText DisplayName;

	/** Per stack, before the applying hit's potency. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status", meta = (ClampMin = "0.0"))
	float DamagePerSecond = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status", meta = (ClampMin = "0.1"))
	float Duration = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status", meta = (ClampMin = "1"))
	int32 MaxStacks = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status")
	TSubclassOf<UDamageType> DamageType;

	/** Movement speed while affected; 1 leaves it alone, 0.7 slows by 30%. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status", meta = (ClampMin = "0.1", ClampMax = "2.0"))
	float MoveSpeedMultiplier = 1.0f;

	/** Colour the victim's sprite is tinted while affected. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	FLinearColor Tint = FLinearColor::White;

	/** Shown on each damage tick. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
	FZombieEffectSpec TickEffect;
};
