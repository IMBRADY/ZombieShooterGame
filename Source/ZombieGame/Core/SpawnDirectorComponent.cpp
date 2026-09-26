#include "SpawnDirectorComponent.h"
#include "Characters/Zombies/ZombieCharacter.h"
#include "Characters/Zombies/ZombieEnemyManager.h"
#include "Core/SpawnDirectorSettings.h"
#include "Core/ZombieGameState.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "Utilities/ZombiePrimaryAssetLoader.h"
#include "ZombieGame.h"

USpawnDirectorComponent::USpawnDirectorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	SettingsAsset = TSoftObjectPtr<USpawnDirectorSettings>(
		FSoftObjectPath(TEXT("/Game/DataAssets/Zombies/DA_SpawnDirector.DA_SpawnDirector")));
}

void USpawnDirectorComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UZombieEnemyManager* Enemies = UZombieEnemyManager::Get(this))
	{
		Enemies->OnEnemyDied.AddUObject(this, &USpawnDirectorComponent::HandleEnemyDied);
	}
}

USpawnDirectorSettings* USpawnDirectorComponent::ResolveSettings() const
{
	if (USpawnDirectorSettings* Settings = SettingsAsset.LoadSynchronous())
	{
		return Settings;
	}

	UE_LOG(LogZombieGame, Error, TEXT("Spawn director settings asset '%s' could not be loaded."), *SettingsAsset.ToString());
	return nullptr;
}

void USpawnDirectorComponent::GatherEligibleArchetypes(int32 Sector, TArray<UZombieArchetypeDataAsset*>& OutArchetypes) const
{
	TArray<UZombieArchetypeDataAsset*> Discovered;
	FZombiePrimaryAssetLoader::LoadAllOfType(UZombieArchetypeDataAsset::AssetType, Discovered);

	for (UZombieArchetypeDataAsset* Archetype : Discovered)
	{
		if (Archetype && Archetype->MinSector <= Sector && Archetype->Tier != EZombieClassTier::Boss)
		{
			OutArchetypes.Add(Archetype);
		}
	}

	// Stable ordering keeps a given run seed reproducible regardless of asset enumeration order.
	OutArchetypes.Sort([](const UZombieArchetypeDataAsset& Lhs, const UZombieArchetypeDataAsset& Rhs)
	{
		return Lhs.GetName() < Rhs.GetName();
	});
}

void USpawnDirectorComponent::BeginSector(int32 Sector, const TArray<FVector>& SpawnPoints, const UZombieArchetypeDataAsset* BossArchetype)
{
	StopSector();

	USpawnDirectorSettings* Settings = ResolveSettings();
	UZombieEnemyManager* Enemies = UZombieEnemyManager::Get(this);
	if (!Settings || !Enemies)
	{
		return;
	}

	if (SpawnPoints.Num() == 0)
	{
		UE_LOG(LogZombieGame, Error, TEXT("Sector %d has no zombie spawn points; nothing will spawn."), Sector);
		return;
	}

	TArray<UZombieArchetypeDataAsset*> EligibleArchetypes;
	GatherEligibleArchetypes(Sector, EligibleArchetypes);

	CurrentSector = Sector;
	AvailableSpawnPoints = SpawnPoints;
	PendingBoss = BossArchetype;
	bSectorActive = true;
	Enemies->SetSectorScaling(Settings->GetScalingForSector(Sector));

	BuildSpawnQueue(Sector, Settings->GetBudgetForSector(Sector), EligibleArchetypes);
	PublishEncounterState();

	if (SpawnQueue.Num() == 0 && !PendingBoss)
	{
		// Nothing affordable ever came out of the budget - treat the sector as immediately clear
		// rather than leaving the run stuck waiting on zombies that will never arrive.
		UE_LOG(LogZombieGame, Warning, TEXT("Sector %d produced an empty spawn queue; clearing immediately."), Sector);
		CheckSectorCleared(SpawnPoints[0]);
		return;
	}

	ScheduleNextSpawn();
}

void USpawnDirectorComponent::BuildSpawnQueue(int32 Sector, int32 Budget, const TArray<UZombieArchetypeDataAsset*>& Archetypes)
{
	USpawnDirectorSettings* Settings = ResolveSettings();
	if (!Settings)
	{
		return;
	}

	FRandomStream Random(Sector * 7919 + Budget);
	int32 RemainingBudget = Budget;

	while (RemainingBudget > 0)
	{
		// Only archetypes the remaining budget can still afford stay in the running, so the last
		// few points always resolve into cheap zombies rather than stalling the queue.
		TArray<UZombieArchetypeDataAsset*> Affordable;
		float TotalWeight = 0.0f;

		for (UZombieArchetypeDataAsset* Archetype : Archetypes)
		{
			const float Weight = Archetype->SelectionWeight * Settings->GetTierWeight(Archetype->Tier, Sector);
			if (Archetype->SpawnCost <= RemainingBudget && Weight > 0.0f)
			{
				Affordable.Add(Archetype);
				TotalWeight += Weight;
			}
		}

		if (Affordable.Num() == 0 || TotalWeight <= 0.0f)
		{
			break;
		}

		float Roll = Random.FRandRange(0.0f, TotalWeight);
		for (UZombieArchetypeDataAsset* Archetype : Affordable)
		{
			Roll -= Archetype->SelectionWeight * Settings->GetTierWeight(Archetype->Tier, Sector);
			if (Roll <= 0.0f)
			{
				SpawnQueue.Add(Archetype);
				RemainingBudget -= Archetype->SpawnCost;
				break;
			}
		}
	}

	UE_LOG(LogZombieGame, Log, TEXT("Sector %d: budget %d spent on %d zombies%s."),
		Sector, Budget, SpawnQueue.Num(), PendingBoss ? TEXT(" plus a boss") : TEXT(""));
}

void USpawnDirectorComponent::ScheduleNextSpawn()
{
	UWorld* World = GetWorld();
	USpawnDirectorSettings* Settings = ResolveSettings();
	if (!World || !Settings || !bSectorActive || SpawnQueue.Num() == 0)
	{
		return;
	}

	World->GetTimerManager().SetTimer(SpawnTimer, this, &USpawnDirectorComponent::SpawnNextZombie,
		FMath::Max(Settings->SpawnInterval, 0.05f), false);
}

bool USpawnDirectorComponent::TrySelectSpawnLocation(FVector& OutLocation) const
{
	const USpawnDirectorSettings* Settings = SettingsAsset.Get();
	const float MinDistance = Settings ? Settings->MinSpawnDistanceFromPlayer : 900.0f;

	TArray<FVector> PlayerLocations;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (const APawn* Pawn = It->IsValid() ? It->Get()->GetPawn() : nullptr)
		{
			PlayerLocations.Add(Pawn->GetActorLocation());
		}
	}

	if (AvailableSpawnPoints.Num() == 0)
	{
		return false;
	}

	// Prefer points beyond the "don't spawn in their lap" radius; if the player is standing in the
	// middle of every spawn point, fall back to the furthest rather than refusing to spawn.
	TArray<FVector> Candidates;
	FVector FurthestPoint = AvailableSpawnPoints[0];
	float FurthestDistanceSquared = -1.0f;

	for (const FVector& SpawnPoint : AvailableSpawnPoints)
	{
		float NearestPlayerDistanceSquared = TNumericLimits<float>::Max();
		for (const FVector& PlayerLocation : PlayerLocations)
		{
			NearestPlayerDistanceSquared = FMath::Min(NearestPlayerDistanceSquared, static_cast<float>(FVector::DistSquared(SpawnPoint, PlayerLocation)));
		}

		if (PlayerLocations.Num() == 0 || NearestPlayerDistanceSquared >= FMath::Square(MinDistance))
		{
			Candidates.Add(SpawnPoint);
		}

		if (NearestPlayerDistanceSquared > FurthestDistanceSquared)
		{
			FurthestDistanceSquared = NearestPlayerDistanceSquared;
			FurthestPoint = SpawnPoint;
		}
	}

	OutLocation = Candidates.Num() > 0 ? Candidates[FMath::RandRange(0, Candidates.Num() - 1)] : FurthestPoint;
	return true;
}

void USpawnDirectorComponent::SpawnNextZombie()
{
	UZombieEnemyManager* Enemies = UZombieEnemyManager::Get(this);
	const USpawnDirectorSettings* Settings = SettingsAsset.Get();
	if (!Enemies || !bSectorActive || SpawnQueue.Num() == 0)
	{
		return;
	}

	// At the cap the queue simply waits - the sector's total stays the same, its shape changes.
	const int32 MaxConcurrent = Settings ? Settings->MaxConcurrentZombies : 40;
	FVector SpawnLocation;
	if (Enemies->GetLiveCount() < MaxConcurrent && TrySelectSpawnLocation(SpawnLocation))
	{
		if (Enemies->SpawnZombie(SpawnQueue[0], SpawnLocation, FZombieSpawnOptions()))
		{
			SpawnQueue.RemoveAt(0, EAllowShrinking::No);
		}
	}

	PublishEncounterState();
	ScheduleNextSpawn();
}

AZombieCharacter* USpawnDirectorComponent::ReleaseBoss(const FVector& Location)
{
	UZombieEnemyManager* Enemies = UZombieEnemyManager::Get(this);
	if (!PendingBoss || !Enemies || !bSectorActive)
	{
		return nullptr;
	}

	FZombieSpawnOptions Options;
	Options.bIsBoss = true;
	AZombieCharacter* Boss = Enemies->SpawnZombie(PendingBoss, Location, Options);
	if (Boss)
	{
		PendingBoss = nullptr;
		OnBossSpawned.Broadcast(Boss);
		PublishEncounterState();
	}
	return Boss;
}

void USpawnDirectorComponent::HandleEnemyDied(AZombieCharacter* Zombie, AController* Killer)
{
	PublishEncounterState();
	CheckSectorCleared(Zombie ? Zombie->GetActorLocation() : FVector::ZeroVector);
}

void USpawnDirectorComponent::CheckSectorCleared(const FVector& LastKillLocation)
{
	const UZombieEnemyManager* Enemies = UZombieEnemyManager::Get(this);
	if (!bSectorActive || SpawnQueue.Num() > 0 || PendingBoss || (Enemies && Enemies->GetLiveCount() > 0))
	{
		return;
	}

	bSectorActive = false;
	UE_LOG(LogZombieGame, Log, TEXT("Sector %d cleared: budget exhausted and no zombies remain."), CurrentSector);
	OnSectorCleared.Broadcast(LastKillLocation);
}

void USpawnDirectorComponent::PublishEncounterState() const
{
	UWorld* World = GetWorld();
	AZombieGameState* GameState = World ? World->GetGameState<AZombieGameState>() : nullptr;
	const UZombieEnemyManager* Enemies = UZombieEnemyManager::Get(this);
	if (!GameState)
	{
		return;
	}

	int32 RemainingBudget = 0;
	for (const UZombieArchetypeDataAsset* Archetype : SpawnQueue)
	{
		RemainingBudget += Archetype ? Archetype->SpawnCost : 0;
	}

	GameState->SetSpawnBudget(RemainingBudget);
	GameState->SetRemainingEnemies(SpawnQueue.Num() + (Enemies ? Enemies->GetLiveCount() : 0) + (PendingBoss ? 1 : 0));
}

void USpawnDirectorComponent::StopSector()
{
	bSectorActive = false;
	PendingBoss = nullptr;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SpawnTimer);
	}
	if (UZombieEnemyManager* Enemies = UZombieEnemyManager::Get(this))
	{
		Enemies->ClearAll();
	}

	SpawnQueue.Reset();
	AvailableSpawnPoints.Reset();
	PublishEncounterState();
}
