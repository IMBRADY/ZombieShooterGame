#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ZombieGameState.generated.h"

class AZombieShopState;

/** Where the current sector is in its lifecycle - drives the HUD objective and music. */
UENUM(BlueprintType)
enum class ESectorPhase : uint8
{
	/** Level built, waiting for the first spawn. */
	Starting,
	/** Zombies are being spawned or are still alive. */
	Combat,
	/** Budget spent and the field cleared; the key has dropped and must be collected. */
	KeyDropped,
	/** Key collected; the exit door is open. */
	ExitUnlocked,
	/** In the between-sector shop. */
	Intermission,
	/** Every player is dead. */
	GameOver
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSectorChanged, int32, NewSector);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSectorPhaseChanged, ESectorPhase /*NewPhase*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnActiveBossChanged, AActor* /*Boss*/);
DECLARE_MULTICAST_DELEGATE(FOnEncounterStateChanged);

/**
 * Shared, replicated run state everyone observes (prompt.txt "GameState tracks: Current Sector,
 * Difficulty, Active Zombies, Spawn Budget, Remaining Enemies"), plus the sector phase and the
 * active boss so the HUD's objective and boss bar are purely event-driven.
 */
UCLASS()
class ZOMBIEGAME_API AZombieGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	int32 GetCurrentSector() const { return CurrentSector; }
	float GetDifficultyLevel() const { return DifficultyLevel; }
	int32 GetSpawnBudget() const { return SpawnBudget; }
	int32 GetRemainingEnemies() const { return RemainingEnemies; }
	const TArray<TObjectPtr<AActor>>& GetActiveZombies() const { return ActiveZombies; }
	ESectorPhase GetSectorPhase() const { return SectorPhase; }
	AActor* GetActiveBoss() const { return ActiveBoss; }
	bool IsBossSector() const { return bBossSector; }

	void AdvanceToSector(int32 NewSector, bool bIsBossSector);
	void SetDifficultyLevel(float NewDifficultyLevel);
	void SetSpawnBudget(int32 NewBudget);
	void SetRemainingEnemies(int32 NewRemainingEnemies);
	void AddActiveZombie(AActor* Zombie);
	void RemoveActiveZombie(AActor* Zombie);
	void SetSectorPhase(ESectorPhase NewPhase);
	void SetActiveBoss(AActor* Boss);

	/** The intermission shop's shared stock, while one is open. */
	AZombieShopState* GetShop() const { return Shop; }
	void SetShop(AZombieShopState* InShop) { Shop = InShop; }

	UPROPERTY(BlueprintAssignable)
	FOnSectorChanged OnSectorChanged;

	FOnSectorPhaseChanged OnSectorPhaseChanged;
	FOnActiveBossChanged OnActiveBossChanged;
	FOnEncounterStateChanged OnEncounterStateChanged;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_CurrentSector)
	int32 CurrentSector = 0;

	UPROPERTY(Replicated)
	bool bBossSector = false;

	UPROPERTY(Replicated)
	float DifficultyLevel = 1.0f;

	UPROPERTY(ReplicatedUsing = OnRep_EncounterState)
	int32 SpawnBudget = 0;

	UPROPERTY(ReplicatedUsing = OnRep_EncounterState)
	int32 RemainingEnemies = 0;

	UPROPERTY(Replicated)
	TArray<TObjectPtr<AActor>> ActiveZombies;

	UPROPERTY(ReplicatedUsing = OnRep_SectorPhase)
	ESectorPhase SectorPhase = ESectorPhase::Starting;

	UPROPERTY(ReplicatedUsing = OnRep_ActiveBoss)
	TObjectPtr<AActor> ActiveBoss;

	UPROPERTY(Replicated)
	TObjectPtr<AZombieShopState> Shop;

	UFUNCTION() void OnRep_CurrentSector();
	UFUNCTION() void OnRep_EncounterState();
	UFUNCTION() void OnRep_SectorPhase();
	UFUNCTION() void OnRep_ActiveBoss();
};
