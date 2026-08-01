#include "ZombieGameState.h"
#include "Net/UnrealNetwork.h"

void AZombieGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AZombieGameState, CurrentSector);
	DOREPLIFETIME(AZombieGameState, DifficultyLevel);
	DOREPLIFETIME(AZombieGameState, SpawnBudget);
	DOREPLIFETIME(AZombieGameState, RemainingEnemies);
	DOREPLIFETIME(AZombieGameState, ActiveZombies);
}

void AZombieGameState::AdvanceToSector(int32 NewSector)
{
	CurrentSector = NewSector;
	OnSectorChanged.Broadcast(CurrentSector);
}

void AZombieGameState::SetDifficultyLevel(float NewDifficultyLevel)
{
	DifficultyLevel = NewDifficultyLevel;
}

void AZombieGameState::SetSpawnBudget(int32 NewBudget)
{
	SpawnBudget = NewBudget;
}

void AZombieGameState::SetRemainingEnemies(int32 NewRemainingEnemies)
{
	RemainingEnemies = NewRemainingEnemies;
}

void AZombieGameState::AddActiveZombie(AActor* Zombie)
{
	if (Zombie)
	{
		ActiveZombies.AddUnique(Zombie);
	}
}

void AZombieGameState::RemoveActiveZombie(AActor* Zombie)
{
	ActiveZombies.Remove(Zombie);
}
