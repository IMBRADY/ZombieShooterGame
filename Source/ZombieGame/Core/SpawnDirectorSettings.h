#pragma once

#include "CoreMinimal.h"
#include "Characters/Zombies/ZombieArchetypeDataAsset.h"
#include "Engine/DataAsset.h"
#include "SpawnDirectorSettings.generated.h"

/**
 * The difficulty curve, as data.
 *
 * Everything the Spawn Director needs to answer "how hard is sector N" lives here, so retuning the
 * game's pacing is a Data Asset edit. There are deliberately no per-sector special cases in code -
 * the curve is parameterised, per ARCHITECTURE.md 13 ("no hardcoded per-sector if chains").
 */
UCLASS(BlueprintType)
class ZOMBIEGAME_API USpawnDirectorSettings : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	USpawnDirectorSettings();

	static const FPrimaryAssetType AssetType;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	// --- Budget ---

	/** Difficulty budget for sector 1, spent against archetype spawn costs. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Budget", meta = (ClampMin = "1"))
	int32 BaseBudget = 60;

	/** Compounding growth: budget = BaseBudget * BudgetGrowthPerSector^(sector - 1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Budget", meta = (ClampMin = "1.0"))
	float BudgetGrowthPerSector = 1.25f;

	// --- Per-sector scaling of the zombies themselves ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scaling", meta = (ClampMin = "0.0"))
	float HealthIncreasePerSector = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scaling", meta = (ClampMin = "0.0"))
	float DamageIncreasePerSector = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scaling", meta = (ClampMin = "0.0"))
	float RewardIncreasePerSector = 0.10f;

	// --- Pacing ---

	/** Seconds between spawns while the queue drains. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pacing", meta = (ClampMin = "0.05"))
	float SpawnInterval = 1.1f;

	/** Ceiling on simultaneously live zombies; the queue waits rather than dumping the sector. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pacing", meta = (ClampMin = "1"))
	int32 MaxConcurrentZombies = 40;

	/** Zombies never materialise in the player's lap. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pacing", meta = (ClampMin = "0.0"))
	float MinSpawnDistanceFromPlayer = 900.0f;

	/**
	 * Zombies never spawn on screen: a spawn point inside any player's view, grown by this margin
	 * (world units) on every side so nothing pops in right at the edge, is skipped.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pacing", meta = (ClampMin = "0.0"))
	float OffscreenSpawnMargin = 250.0f;

	/**
	 * Once a player has been in a room and no zombie is left alive in it, the room is cleared and
	 * nothing spawns there again - unless every room with spawn points is cleared, in which case
	 * the rest of the budget comes in through cleared rooms (off screen, as far away as possible)
	 * rather than leaving the sector impossible to finish.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pacing")
	bool bKeepClearedRoomsClear = true;

	/**
	 * If every spawn point is on screen for this long, the queue gives up waiting and uses the
	 * point furthest from the players - a last resort so a sector can always be finished.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pacing", meta = (ClampMin = "0.0"))
	float MaxOffscreenWaitSeconds = 10.0f;

	// --- Tier mix ---

	/**
	 * Share of the pool each zombie class takes at sector 1. The design brief describes deciding
	 * "what portions of total zombie pool will constitute each class" - this is that decision.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tier Mix")
	TMap<EZombieClassTier, float> BaseTierWeights;

	/** Added to a tier's weight per sector cleared - elites become steadily more common. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tier Mix")
	TMap<EZombieClassTier, float> TierWeightIncreasePerSector;

	int32 GetBudgetForSector(int32 Sector) const;
	FZombieDifficultyScaling GetScalingForSector(int32 Sector) const;
	float GetTierWeight(EZombieClassTier Tier, int32 Sector) const;
};
