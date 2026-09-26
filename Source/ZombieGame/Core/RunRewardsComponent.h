#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RunRewardsComponent.generated.h"

class AController;
class AZombieCharacter;

/**
 * The Loot System's run-side half: turns kills into rewards. Credits the killer's statistics
 * (kills, favourite weapon, bosses), drops the zombie's money and any loot-table extras, drops the
 * sector key, and tidies pickups away between sectors. Owned by the GameMode so rewards are
 * decided on the authority only.
 */
UCLASS(ClassGroup = (Custom))
class ZOMBIEGAME_API URunRewardsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URunRewardsComponent();

	void HandleEnemyDied(AZombieCharacter* Zombie, AController* Killer, int32 Sector);

	/** Drops the sector key where the last zombie fell. */
	void DropSectorKey(const FVector& Location);

	/** Pulls every coin still on the floor to the nearest player - no backtracking after a clear. */
	void SweepMoneyToPlayers();

	/** Returns every pickup to the pool (sector teardown). */
	void ClearPickups();

private:
	void CreditKill(const AZombieCharacter& Zombie, AController* Killer) const;
	void DropRewards(const AZombieCharacter& Zombie, int32 Sector) const;
	FText ResolveWeaponName(const AZombieCharacter& Zombie) const;
};
