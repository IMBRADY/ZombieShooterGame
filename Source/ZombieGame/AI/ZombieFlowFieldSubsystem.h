#pragma once

#include "CoreMinimal.h"
#include "Rooms/Procedural/SectorNavigationGrid.h"
#include "Subsystems/WorldSubsystem.h"
#include "ZombieFlowFieldSubsystem.generated.h"

/**
 * One shared "how do I get to the player" field for the whole horde.
 *
 * Rather than every zombie asking the navigation system for its own path to the same moving
 * target, the sector is flooded **outward from the players** once per rebuild: a breadth-first
 * sweep over the sector's tile grid records, for every cell, how far it is from the nearest
 * player. A zombie then just reads the downhill direction at the cell it is standing in.
 *
 * Cost is one sweep of the grid per rebuild interval regardless of how many zombies are alive -
 * so a hundred zombies cost exactly what one does, which is what makes the spec's "hundreds of
 * zombies" target reachable. It also produces better horde behaviour for free: because every
 * zombie descends the same field, they naturally fan out along different corridors into the same
 * destination instead of single-filing down one path.
 *
 * Multi-source by construction: the sweep is seeded from *every* player pawn at once, so each
 * zombie flows toward whichever player is nearest, and adding a second player needs no changes.
 */
UCLASS()
class ZOMBIEGAME_API UZombieFlowFieldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	/** Adopts a newly generated sector's walkability grid and starts rebuilding the field. */
	void SetSectorGrid(const FSectorNavigationGrid& InGrid);

	/** Drops the field - called when a sector is torn down. */
	void ClearSectorGrid();

	bool IsReady() const { return !Grid.IsEmpty() && Distances.Num() == Grid.NumCells(); }

	/**
	 * Direction to travel from WorldLocation to get closer to the nearest player.
	 * Returns false when the location is off-grid or walled off from every player, leaving the
	 * caller to fall back to steering straight at its own target.
	 */
	bool GetFlowDirection(const FVector& WorldLocation, FVector& OutDirection) const;

	/** Cells between this location and the nearest player, or INDEX_NONE if unreachable. */
	int32 GetDistanceToNearestPlayer(const FVector& WorldLocation) const;

private:
	void Rebuild();
	void StartRebuildTimer();

	/** Seeds the sweep from every player pawn currently standing on the grid. */
	void GatherGoalCells(TArray<int32>& OutGoalIndices) const;

	/** Breadth-first flood outward from the goal cells, filling Distances. */
	void FloodFill(const TArray<int32>& GoalIndices);

	/** Turns the distance field into a per-cell direction, using the local gradient. */
	void BuildDirections();

	FSectorNavigationGrid Grid;

	/** Cell distance from the nearest player, in cells. INDEX_NONE where unreachable. */
	TArray<int32> Distances;

	/** Unit direction pointing downhill (toward a player). Zero where unreachable. */
	TArray<FVector2f> Directions;

	FTimerHandle RebuildTimer;

	/** How often the field is re-flooded. The players have to move a long way to invalidate it. */
	float RebuildInterval = 0.25f;
};
