#include "ZombieGameState.h"
#include "Net/UnrealNetwork.h"
#include "Shop/ZombieShopState.h"

void AZombieGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AZombieGameState, CurrentSector);
	DOREPLIFETIME(AZombieGameState, bBossSector);
	DOREPLIFETIME(AZombieGameState, DifficultyLevel);
	DOREPLIFETIME(AZombieGameState, SpawnBudget);
	DOREPLIFETIME(AZombieGameState, RemainingEnemies);
	DOREPLIFETIME(AZombieGameState, ActiveZombies);
	DOREPLIFETIME(AZombieGameState, SectorPhase);
	DOREPLIFETIME(AZombieGameState, ActiveBoss);
	DOREPLIFETIME(AZombieGameState, Shop);
}

void AZombieGameState::AdvanceToSector(int32 NewSector, bool bIsBossSector)
{
	CurrentSector = NewSector;
	bBossSector = bIsBossSector;
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
	if (RemainingEnemies != NewRemainingEnemies)
	{
		RemainingEnemies = NewRemainingEnemies;
		OnEncounterStateChanged.Broadcast();
	}
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

void AZombieGameState::SetSectorPhase(ESectorPhase NewPhase)
{
	if (SectorPhase != NewPhase)
	{
		SectorPhase = NewPhase;
		OnSectorPhaseChanged.Broadcast(SectorPhase);
	}
}

void AZombieGameState::SetActiveBoss(AActor* Boss)
{
	if (ActiveBoss != Boss)
	{
		ActiveBoss = Boss;
		OnActiveBossChanged.Broadcast(ActiveBoss);
	}
}

void AZombieGameState::OnRep_CurrentSector()
{
	OnSectorChanged.Broadcast(CurrentSector);
}

void AZombieGameState::OnRep_EncounterState()
{
	OnEncounterStateChanged.Broadcast();
}

void AZombieGameState::OnRep_SectorPhase()
{
	OnSectorPhaseChanged.Broadcast(SectorPhase);
}

void AZombieGameState::OnRep_ActiveBoss()
{
	OnActiveBossChanged.Broadcast(ActiveBoss);
}
