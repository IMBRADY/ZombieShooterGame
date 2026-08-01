#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ZombieGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSectorChanged, int32, NewSector);

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

	void AdvanceToSector(int32 NewSector);
	void SetDifficultyLevel(float NewDifficultyLevel);
	void SetSpawnBudget(int32 NewBudget);
	void SetRemainingEnemies(int32 NewRemainingEnemies);
	void AddActiveZombie(AActor* Zombie);
	void RemoveActiveZombie(AActor* Zombie);

	UPROPERTY(BlueprintAssignable)
	FOnSectorChanged OnSectorChanged;

protected:
	UPROPERTY(Replicated)
	int32 CurrentSector = 0;

	UPROPERTY(Replicated)
	float DifficultyLevel = 1.0f;

	UPROPERTY(Replicated)
	int32 SpawnBudget = 0;

	UPROPERTY(Replicated)
	int32 RemainingEnemies = 0;

	UPROPERTY(Replicated)
	TArray<TObjectPtr<AActor>> ActiveZombies;
};
