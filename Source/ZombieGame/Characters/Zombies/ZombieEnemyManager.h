#pragma once

#include "CoreMinimal.h"
#include "Characters/Zombies/ZombieArchetypeDataAsset.h"
#include "Subsystems/WorldSubsystem.h"
#include "ZombieEnemyManager.generated.h"

class AController;
class AZombieCharacter;

/** How a particular zombie enters the world. */
struct FZombieSpawnOptions
{
	/** Revived and summoned zombies still count toward clearing the sector but pay out nothing. */
	bool bGrantsRewards = true;
	bool bIsBoss = false;
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnEnemyDied, AZombieCharacter* /*Zombie*/, AController* /*Killer*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnEnemySpawned, AZombieCharacter* /*Zombie*/);

/**
 * The Enemy Manager: the one place zombies are created, tracked and cleaned up, whoever asked
 * for them - the Spawn Director's queue, a boss summoning minions, a necromancer raising corpses.
 * Keeps the GameState's live-zombie list honest and remembers recent corpses for revival.
 *
 * The Spawn Director decides *what* and *when*; this only knows *how*.
 */
UCLASS()
class ZOMBIEGAME_API UZombieEnemyManager : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UZombieEnemyManager* Get(const UObject* WorldContext);

	/** Difficulty scaling applied to everything spawned from now on (set per sector). */
	void SetSectorScaling(const FZombieDifficultyScaling& InScaling) { Scaling = InScaling; }
	const FZombieDifficultyScaling& GetSectorScaling() const { return Scaling; }

	AZombieCharacter* SpawnZombie(const UZombieArchetypeDataAsset* Archetype, const FVector& Location, const FZombieSpawnOptions& Options);

	int32 GetLiveCount() const { return LiveZombies.Num(); }
	const TArray<TObjectPtr<AZombieCharacter>>& GetLiveZombies() const { return LiveZombies; }

	/** Remembers a corpse a necromancer may later raise. */
	void RegisterCorpse(AZombieCharacter* Corpse);

	bool HasCorpseNear(const FVector& Location, float Radius) const;

	/** Raises up to MaxCount corpses within Radius as fresh (reward-less) zombies. Returns how many rose. */
	int32 ReviveCorpsesNear(const FVector& Location, float Radius, int32 MaxCount);

	/** Destroys every live zombie and corpse - sector teardown and run end. */
	void ClearAll();

	FOnEnemyDied OnEnemyDied;
	FOnEnemySpawned OnEnemySpawned;

private:
	void HandleZombieDied(AZombieCharacter* Zombie, AController* Killer);
	void SyncGameState(AZombieCharacter* Zombie, bool bAdded) const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AZombieCharacter>> LiveZombies;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AZombieCharacter>> Corpses;

	FZombieDifficultyScaling Scaling;
};
