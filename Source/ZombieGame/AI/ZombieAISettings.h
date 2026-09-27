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

	// --- Alertness ---

	/**
	 * Every zombie within this distance of a gunshot is aggravated: it hunts the shooter outright
	 * rather than merely investigating the noise. Walls do not muffle it - "gunshots attract zombies
	 * very well". Archetype hearing ranges are raised to at least this.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Alertness", meta = (ClampMin = "0.0"))
	float GunshotAlertRadius = 3200.0f;

	/**
	 * How long an aggravated zombie (shot at, or near a gunshot) keeps hunting a player it cannot
	 * see. Every further shot restarts the clock, so a zombie never calms down mid-firefight.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Alertness", meta = (ClampMin = "0.0"))
	float AggravatedMemorySeconds = 12.0f;

	/**
	 * A player this close is noticed regardless of where the zombie is facing (with a clear line
	 * between them), and a hunted player this close is never forgotten.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Alertness", meta = (ClampMin = "0.0"))
	float ProximityAwarenessRadius = 550.0f;

	// --- Chasing ---

	/**
	 * A chasing zombie that has moved less than this in StuckCheckInterval is treated as stuck
	 * (wedged on a corner, blocked by the crowd) and switches to real pathfinding for a moment.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chasing", meta = (ClampMin = "0.0"))
	float StuckDistanceThreshold = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chasing", meta = (ClampMin = "0.1"))
	float StuckCheckInterval = 0.75f;

	/** How long a stuck zombie follows a navmesh path before going back to the flow field. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Chasing", meta = (ClampMin = "0.1"))
	float StuckRecoverySeconds = 1.5f;

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
