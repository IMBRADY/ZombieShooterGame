#pragma once

#include "CoreMinimal.h"
#include "Core/Save/ZombieRunTypes.h"

class AController;
class UZombieRunSettings;

/**
 * Moves a player's run state between the live game (pawn components + player state) and the
 * FZombieRunSaveData a checkpoint stores. The one place that knows which component holds which
 * piece of the run, so saving, restoring and the fresh-run loadout can never disagree.
 */
class ZOMBIEGAME_API FZombieRunState
{
public:
	/** Reads the player's current state. Run-level fields (seed, sector) are filled by the caller. */
	static void Capture(const AController& Player, FZombieRunSaveData& OutData);

	/** Restores a checkpoint onto a freshly spawned player. */
	static void Apply(AController& Player, const FZombieRunSaveData& Data);

	/** Gives a brand-new run's player their starting guns, slots and money. */
	static void ApplyStartingLoadout(AController& Player, const UZombieRunSettings& Settings, int32 StartingSlots);
};
