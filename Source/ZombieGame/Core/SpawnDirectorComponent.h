#pragma once

#include "CoreMinimal.h"
#include "Characters/Zombies/ZombieArchetypeDataAsset.h"
#include "Components/ActorComponent.h"
#include "SpawnDirectorComponent.generated.h"

class AController;
class AZombieCharacter;
class USpawnDirectorSettings;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnSectorCleared, const FVector& /*LastKillLocation*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnBossSpawned, AZombieCharacter* /*Boss*/);

/**
 * The Procedural Director.
 *
 *     difficulty budget -> zombie cost table -> weighted selection -> spawn queue
 *
 * Nothing here rolls "a random zombie": a sector is given a budget, the director spends it on
 * archetypes it can afford, and the resulting queue is what drains into the world over time. That
 * is what makes encounters varied but bounded, and what makes "sector cleared" a well-defined
 * event - budget spent *and* nothing left alive (and, on a boss sector, the boss dead).
 *
 * It decides what and when; the Enemy Manager does the actual spawning and tracking, so zombies
 * raised by a necromancer or summoned by a boss count toward the sector exactly like queued ones.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ZOMBIEGAME_API USpawnDirectorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USpawnDirectorComponent();

	/**
	 * Computes the sector's budget, fills the spawn queue, and starts draining it. A boss
	 * archetype, if given, is held back until ReleaseBoss is called.
	 */
	void BeginSector(int32 Sector, const TArray<FVector>& SpawnPoints, const UZombieArchetypeDataAsset* BossArchetype);

	/** Wakes the held-back boss at a location (the player walked into the boss room). */
	AZombieCharacter* ReleaseBoss(const FVector& Location);
	bool IsBossPending() const { return PendingBoss != nullptr; }

	/** Halts spawning and clears every zombie - used when a sector is torn down or the run ends. */
	void StopSector();

	int32 GetQueuedZombieCount() const { return SpawnQueue.Num(); }

	FOnSectorCleared OnSectorCleared;
	FOnBossSpawned OnBossSpawned;

protected:
	virtual void BeginPlay() override;

	/**
	 * Difficulty curve. Soft reference with a conventional default path, so a missing or renamed
	 * asset is a logged error rather than a hard dependency in the GameMode's constructor.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Spawning")
	TSoftObjectPtr<USpawnDirectorSettings> SettingsAsset;

private:
	USpawnDirectorSettings* ResolveSettings() const;

	/** Archetypes that may appear in this sector (unlocked, not bosses), in a stable order. */
	void GatherEligibleArchetypes(int32 Sector, TArray<UZombieArchetypeDataAsset*>& OutArchetypes) const;

	/** Spends the budget on affordable archetypes, weighted by tier mix and per-archetype weight. */
	void BuildSpawnQueue(int32 Sector, int32 Budget, const TArray<UZombieArchetypeDataAsset*>& Archetypes);

	/** Timer-driven, never Tick: one spawn attempt per interval while the queue has entries. */
	void SpawnNextZombie();
	void ScheduleNextSpawn();

	bool TrySelectSpawnLocation(FVector& OutLocation) const;

	void HandleEnemyDied(AZombieCharacter* Zombie, AController* Killer);
	void CheckSectorCleared(const FVector& LastKillLocation);

	/** Mirrors queue/live counts onto the replicated GameState for the HUD to observe. */
	void PublishEncounterState() const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UZombieArchetypeDataAsset>> SpawnQueue;

	UPROPERTY(Transient)
	TObjectPtr<const UZombieArchetypeDataAsset> PendingBoss;

	TArray<FVector> AvailableSpawnPoints;
	FTimerHandle SpawnTimer;
	int32 CurrentSector = 0;
	bool bSectorActive = false;
};
