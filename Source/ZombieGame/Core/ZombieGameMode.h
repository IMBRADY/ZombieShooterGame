#pragma once

#include "CoreMinimal.h"
#include "Core/Save/ZombieRunTypes.h"
#include "Core/ZombieRunController.h"
#include "GameFramework/GameModeBase.h"
#include "ZombieGameMode.generated.h"

class AController;
class AZombieCharacter;
class AZombiePlayerCharacter;
class AZombieShopState;
class URunRewardsComponent;
class USectorGeneratorComponent;
class USpawnDirectorComponent;
enum class ESectorPhase : uint8;

/**
 * Authoritative run logic: which sector is running, how hard it is, and what happens when it ends.
 *
 *     sector (clear the budget) -> key drops -> exit unlocks -> intermission shop -> next sector
 *
 * It owns the systems a run needs but implements none of them - the level comes from
 * USectorGeneratorComponent, the encounter from USpawnDirectorComponent, rewards from
 * URunRewardsComponent, saves through FZombieRunState. What is left here is sequencing.
 */
UCLASS()
class ZOMBIEGAME_API AZombieGameMode : public AGameModeBase, public IZombieRunController
{
	GENERATED_BODY()

public:
	AZombieGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void BeginPlay() override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual void FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation) override;

	// IZombieRunController
	virtual void NotifyKeyCollected(APawn* Collector) override;
	virtual void NotifyExitUsed(APawn* User) override;
	virtual void NotifyIntermissionLeft(APawn* User) override;
	virtual void NotifyBossRoomEntered(APawn* Entrant) override;

	/** Seed the whole run derives from; recorded so a run can be reproduced or resumed. */
	int32 GetRunSeed() const { return RunSeed; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Run") TObjectPtr<USectorGeneratorComponent> SectorGenerator;
	UPROPERTY(VisibleAnywhere, Category = "Run") TObjectPtr<USpawnDirectorComponent> SpawnDirector;
	UPROPERTY(VisibleAnywhere, Category = "Run") TObjectPtr<URunRewardsComponent> Rewards;

private:
	/** Generates the sector's rooms. Safe to call before the GameState exists. */
	bool BuildSector(int32 Sector);

	/** Starts the encounter for the sector already built, and writes the sector-start checkpoint. */
	void StartEncounter(int32 Sector);

	void EnterIntermission();
	void AdvanceToNextSector();

	void HandleEnemyDied(AZombieCharacter* Zombie, AController* Killer);
	void HandleSectorCleared(const FVector& LastKillLocation);
	void HandleBossSpawned(AZombieCharacter* Boss);
	void HandlePlayerDied(AZombiePlayerCharacter* Character);
	void EndRun();

	void MovePlayersToSectorStart();
	void SetPhase(ESectorPhase NewPhase);
	void SaveCheckpoint(bool bInIntermission);
	void TickRunClock();
	void EvaluateAchievements() const;

	const class UZombieArchetypeDataAsset* PickBossArchetype(int32 Sector) const;

	/** Per-sector seed derived from the run seed, so one run is reproducible end to end. */
	int32 GetSeedForSector(int32 Sector) const;

	UPROPERTY(Transient)
	TObjectPtr<AZombieShopState> ShopState;

	FZombieRunSaveData PendingRestore;
	bool bHasPendingRestore = false;
	bool bResumeInIntermission = false;
	bool bRunOver = false;

	int32 CurrentSector = 1;
	int32 RunSeed = 0;
	FTimerHandle RunClockTimer;
};
