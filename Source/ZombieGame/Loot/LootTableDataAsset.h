#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Loot/LootTypes.h"
#include "LootTableDataAsset.generated.h"

/**
 * A weighted loot table (prompt.txt "Loot tables must be Data Assets"). Zombie drops, treasure
 * chests, boss rewards and the Mystery Box are each just a table; the rolling rules live in
 * FLootResolver.
 */
UCLASS(BlueprintType)
class ZOMBIEGAME_API ULootTableDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	static const FPrimaryAssetType AssetType;
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	TArray<FLootEntry> Entries;

	/** Independent rolls per use. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "1"))
	int32 Rolls = 1;

	/** Weight of rolling nothing at all - how a zombie's drop table makes drops *rare*. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0.0"))
	float NothingWeight = 0.0f;
};
