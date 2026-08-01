#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ZombieGameMode.generated.h"

class AController;
class AZombieCharacter;
class USectorGeneratorComponent;
class USpawnDirectorComponent;

/**
 * Authoritative run logic: which sector is running, how hard it is, and what happens when it ends.
 *
 * It owns the two systems a sector needs but implements neither - the level comes from
 * USectorGeneratorComponent, the encounter from USpawnDirectorComponent. What is left here is only
 * the sequencing between them, plus paying out rewards.
 */
UCLASS()
class ZOMBIEGAME_API AZombieGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AZombieGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void BeginPlay() override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	/** Builds the next sector's level and encounter, and moves the players into it. */
	void AdvanceToNextSector();

	/** Seed the whole run derives from; recorded so a run can be reproduced or resumed. */
	int32 GetRunSeed() const { return RunSeed; }

protected:
	/** Generates the sector's rooms. Safe to call before the GameState exists. */
	bool BuildSector(int32 Sector);

	/** Starts the encounter for the sector already built, and syncs the GameState. */
	void StartEncounter(int32 Sector);

	void HandleSectorCleared();
	void HandleZombieKilled(AZombieCharacter* Zombie, AController* Killer);

	void MovePlayersToSectorStart();

	UPROPERTY(VisibleAnywhere, Category = "Sector")
	TObjectPtr<USectorGeneratorComponent> SectorGenerator;

	UPROPERTY(VisibleAnywhere, Category = "Sector")
	TObjectPtr<USpawnDirectorComponent> SpawnDirector;

	UPROPERTY(EditDefaultsOnly, Category = "Progression")
	float DifficultyIncreasePerSector = 0.15f;

	/**
	 * Until the intermission shop exists, a cleared sector rolls straight into the next one so the
	 * run keeps going. Once the exit door and shop land they take over this hand-off; this is the
	 * seam they plug into, not a stand-in for the progression itself.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Progression")
	bool bAutoAdvanceOnSectorCleared = true;

	UPROPERTY(EditDefaultsOnly, Category = "Progression", meta = (ClampMin = "0.0"))
	float SectorTransitionDelay = 5.0f;

private:
	/** Per-sector seed derived from the run seed, so one run is reproducible end to end. */
	int32 GetSeedForSector(int32 Sector) const;

	int32 CurrentSector = 1;
	int32 RunSeed = 0;
	FTimerHandle SectorTransitionTimer;
};
