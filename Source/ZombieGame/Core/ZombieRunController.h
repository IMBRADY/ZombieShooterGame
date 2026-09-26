#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ZombieRunController.generated.h"

class APawn;

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UZombieRunController : public UInterface
{
	GENERATED_BODY()
};

/**
 * What world actors (pickups, doors, trigger volumes) may tell the run about. They talk to this
 * interface - found on the authoritative GameMode - rather than to AZombieGameMode itself, so a
 * pickup or a door carries no knowledge of how the run is sequenced.
 */
class ZOMBIEGAME_API IZombieRunController
{
	GENERATED_BODY()

public:
	/** The sector key was picked up: unlock the exit. */
	virtual void NotifyKeyCollected(APawn* Collector) = 0;

	/** A player used the unlocked exit door: go to the intermission. */
	virtual void NotifyExitUsed(APawn* User) = 0;

	/** A player left the intermission through its door: build the next sector. */
	virtual void NotifyIntermissionLeft(APawn* User) = 0;

	/** A player stepped into the boss room: wake the boss. */
	virtual void NotifyBossRoomEntered(APawn* Entrant) = 0;
};
