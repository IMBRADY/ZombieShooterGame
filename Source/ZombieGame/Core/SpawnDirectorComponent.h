#pragma once

#include "CoreMinimal.h"
#include "Characters/Zombies/ZombieArchetypeDataAsset.h"
#include "Components/ActorComponent.h"
#include "SpawnDirectorComponent.generated.h"

class AController;
class AZombieCharacter;
class USpawnDirectorSettings;

DECLARE_MULTICAST_DELEGATE(FOnSectorCleared);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnZombieKilled, AZombieCharacter* /*Zombie*/, AController* /*Killer*/);

/**
 * The Procedural Director.
 *
 *     difficulty budget -> zombie cost table -> weighted selection -> spawn queue
 *
 * Nothing here rolls "a random zombie": a sector is given a budget, the director spends it on
 * archetypes it can afford, and the resulting queue is what drains into the world over time. That
 * is what makes encounters varied but bounded, and what makes "sector cleared" a well-defined
 * event - budget spent *and* nothing left alive.
 *
 * Lives on the GameMode (run logic) and is deliberately ignorant of how the level was built: it is
 * handed spawn points and does not care whether they came from a generated sector or a hand-placed
 * test map.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ZOMBIEGAME_API USpawnDirectorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USpawnDirectorComponent();

	/** Computes the sector's budget, fills the spawn queue, and starts draining it. */
	void BeginSector(int32 Sector, const TArray<FVector>& SpawnPoints);

	/** Halts spawning and destroys anything still alive - used when a run ends or restarts. */
	void StopSector();

	int32 GetQueuedZombieCount() const { return SpawnQueue.Num(); }
	int32 GetLiveZombieCount() const { return LiveZombies.Num(); }

	/** Budget spent *and* every zombie dead - the design's sector completion condition. */
	FOnSectorCleared OnSectorCleared;

	/** Raised for each kill so the GameMode can pay out money and statistics. */
	FOnZombieKilled OnZombieKilled;

protected:
	/**
	 * Difficulty curve. Soft reference with a conventional default path, so a missing or renamed
	 * asset is a logged error rather than a hard dependency in the GameMode's constructor.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Spawning")
	TSoftObjectPtr<USpawnDirectorSettings> SettingsAsset;

private:
	USpawnDirectorSettings* ResolveSettings() const;

	/** Spends the budget on affordable archetypes, weighted by tier mix and per-archetype weight. */
	void BuildSpawnQueue(int32 Sector, int32 Budget, const TArray<UZombieArchetypeDataAsset*>& Archetypes);

	/** Timer-driven, never Tick: one spawn attempt per interval while the queue has entries. */
	void SpawnNextZombie();

	bool TrySelectSpawnLocation(FVector& OutLocation) const;

	void HandleZombieDied(AZombieCharacter* Zombie, AController* Killer);

	/** Mirrors queue/live counts onto the replicated GameState for the HUD to observe. */
	void PublishEncounterState() const;

	void ScheduleNextSpawn();

	UPROPERTY(Transient)
	TArray<TObjectPtr<UZombieArchetypeDataAsset>> SpawnQueue;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AZombieCharacter>> LiveZombies;

	TArray<FVector> AvailableSpawnPoints;
	FZombieDifficultyScaling CurrentScaling;
	FTimerHandle SpawnTimer;
	int32 CurrentSector = 0;
	bool bSectorActive = false;
};
