#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ZombieAISettings.generated.h"

/**
 * Tuning shared by every zombie's Behavior Tree.
 *
 * These are tree-level constants rather than per-archetype numbers (which live on
 * UZombieArchetypeDataAsset): one tree is shared by all zombies, so its node configuration is
 * shared too. Kept in a Data Asset so idle pacing can be retuned without recompiling.
 */
UCLASS(BlueprintType)
class ZOMBIEGAME_API UZombieAISettings : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	static const FPrimaryAssetType AssetType;

	/** Conventional path for the project's settings asset. */
	static const TCHAR* DefaultAssetPath;

	/**
	 * Loads the project's settings asset, or returns the class defaults if it is missing so AI
	 * still runs (with a logged warning) rather than failing silently.
	 */
	static const UZombieAISettings* GetOrLoadDefault();

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	// --- Idle behaviour ---

	/** How far a zombie wanders in one hop when it has nothing to chase. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Roaming", meta = (ClampMin = "100.0"))
	float RoamRadius = 800.0f;

	/** Pause after arriving at a wander point, before picking the next one. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Roaming", meta = (ClampMin = "0.0"))
	float IdleTime = 1.5f;

	/** Random spread on the idle pause, so a crowd of zombies doesn't move in lockstep. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Roaming", meta = (ClampMin = "0.0"))
	float IdleTimeDeviation = 1.0f;

	/** How long a zombie stands and looks around after reaching a noise it was investigating. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Investigating", meta = (ClampMin = "0.0"))
	float InvestigateLookAroundTime = 1.5f;

	// --- Chasing ---

	/**
	 * How often the shared player-centred flow field is re-flooded. This is the horde's whole
	 * pathfinding cost - it does not scale with zombie count.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chasing", meta = (ClampMin = "0.02"))
	float FlowFieldRebuildInterval = 0.25f;

	/**
	 * Clear to make chasing zombies fall back to individual navigation queries. Kept as a switch
	 * because it is the one setting that changes chase cost by orders of magnitude, and being able
	 * to A/B it against per-zombie pathing is worth more than the branch costs.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chasing")
	bool bUseFlowFieldForChase = true;
};
