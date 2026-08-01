#include "SpawnDirectorComponent.h"
#include "Characters/Zombies/ZombieCharacter.h"
#include "Core/SpawnDirectorSettings.h"
#include "Core/ZombieGameState.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Utilities/ZombiePrimaryAssetLoader.h"
#include "ZombieGame.h"

USpawnDirectorComponent::USpawnDirectorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	SettingsAsset = TSoftObjectPtr<USpawnDirectorSettings>(
		FSoftObjectPath(TEXT("/Game/DataAssets/Zombies/DA_SpawnDirector.DA_SpawnDirector")));
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

void USpawnDirectorComponent::BeginSector(int32 Sector, const TArray<FVector>& SpawnPoints)
{
	StopSector();

	USpawnDirectorSettings* Settings = ResolveSettings();
	if (!Settings)
	{
		return;
	}

	if (SpawnPoints.Num() == 0)
	{
		UE_LOG(LogZombieGame, Error, TEXT("Sector %d has no zombie spawn points; nothing will spawn."), Sector);
		return;
	}

	TArray<UZombieArchetypeDataAsset*> DiscoveredArchetypes;
	FZombiePrimaryAssetLoader::LoadAllOfType(UZombieArchetypeDataAsset::AssetType, DiscoveredArchetypes);

	TArray<UZombieArchetypeDataAsset*> EligibleArchetypes;
	for (UZombieArchetypeDataAsset* Archetype : DiscoveredArchetypes)
	{
		if (Archetype && Archetype->MinSector <= Sector && Archetype->Tier != EZombieClassTier::Boss)
		{
			EligibleArchetypes.Add(Archetype);
		}
	}

	if (EligibleArchetypes.Num() == 0)
	{
		UE_LOG(LogZombieGame, Error, TEXT("Sector %d has no eligible zombie archetypes; nothing will spawn."), Sector);
		return;
	}

	// Stable ordering keeps a given run seed reproducible regardless of asset enumeration order.
	EligibleArchetypes.Sort([](const UZombieArchetypeDataAsset& Lhs, const UZombieArchetypeDataAsset& Rhs)
	{
		return Lhs.GetName() < Rhs.GetName();
	});

	CurrentSector = Sector;
	CurrentScaling = Settings->GetScalingForSector(Sector);
	AvailableSpawnPoints = SpawnPoints;
	bSectorActive = true;

	BuildSpawnQueue(Sector, Settings->GetBudgetForSector(Sector), EligibleArchetypes);
	PublishEncounterState();

	if (SpawnQueue.Num() == 0)
	{
		// Nothing affordable ever came out of the budget - treat the sector as immediately clear
		// rather than leaving the run stuck waiting on zombies that will never arrive.
		UE_LOG(LogZombieGame, Warning, TEXT("Sector %d produced an empty spawn queue; clearing immediately."), Sector);
		bSectorActive = false;
		OnSectorCleared.Broadcast();
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
			if (Archetype->SpawnCost > RemainingBudget)
			{
				continue;
			}

			const float Weight = Archetype->SelectionWeight * Settings->GetTierWeight(Archetype->Tier, Sector);
			if (Weight > 0.0f)
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

	UE_LOG(LogZombieGame, Log, TEXT("Sector %d: budget %d spent on %d zombies (health x%.2f, damage x%.2f)."),
		Sector, Budget, SpawnQueue.Num(), CurrentScaling.HealthMultiplier, CurrentScaling.DamageMultiplier);
}

void USpawnDirectorComponent::ScheduleNextSpawn()
{
	UWorld* World = GetWorld();
	USpawnDirectorSettings* Settings = ResolveSettings();
	if (!World || !Settings || !bSectorActive)
	{
		return;
	}

	if (SpawnQueue.Num() == 0)
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

	// Prefer points beyond the "don't spawn in their lap" radius; if the player is standing in the
	// middle of every spawn point, fall back to the furthest rather than refusing to spawn.
	TArray<FVector> Candidates;
	FVector FurthestPoint = FVector::ZeroVector;
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

	if (AvailableSpawnPoints.Num() == 0)
	{
		return false;
	}

	OutLocation = Candidates.Num() > 0 ? Candidates[FMath::RandRange(0, Candidates.Num() - 1)] : FurthestPoint;
	return true;
}

void USpawnDirectorComponent::SpawnNextZombie()
{
	UWorld* World = GetWorld();
	if (!World || !bSectorActive || SpawnQueue.Num() == 0)
	{
		return;
	}

	const USpawnDirectorSettings* Settings = SettingsAsset.Get();
	const int32 MaxConcurrent = Settings ? Settings->MaxConcurrentZombies : 40;

	// At the cap the queue simply waits - the sector's total stays the same, its shape changes.
	if (LiveZombies.Num() < MaxConcurrent)
	{
		FVector SpawnLocation = FVector::ZeroVector;
		const UZombieArchetypeDataAsset* Archetype = SpawnQueue[0];

		if (Archetype && TrySelectSpawnLocation(SpawnLocation))
		{
			UClass* ZombieClass = Archetype->ZombieClass.IsNull()
				? AZombieCharacter::StaticClass()
				: Archetype->ZombieClass.LoadSynchronous();

			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
			SpawnParams.Owner = GetOwner();

			const FTransform SpawnTransform(FRotator(0.0f, FMath::FRandRange(0.0f, 360.0f), 0.0f), SpawnLocation);

			// Deferred so the archetype is applied before BeginPlay: the pawn is never briefly
			// alive with default health, and its AI controller reads final stats on possession.
			AZombieCharacter* Zombie = World->SpawnActorDeferred<AZombieCharacter>(
				ZombieClass ? ZombieClass : AZombieCharacter::StaticClass(), SpawnTransform, GetOwner(),
				nullptr, SpawnParams.SpawnCollisionHandlingOverride);

			if (Zombie)
			{
				Zombie->InitializeFromArchetype(Archetype, CurrentScaling);
				Zombie->OnZombieDied.AddUObject(this, &USpawnDirectorComponent::HandleZombieDied);
				Zombie->FinishSpawning(SpawnTransform);

				SpawnQueue.RemoveAt(0, EAllowShrinking::No);
				LiveZombies.Add(Zombie);

				if (AZombieGameState* GameState = World->GetGameState<AZombieGameState>())
				{
					GameState->AddActiveZombie(Zombie);
				}
			}
		}
	}

	PublishEncounterState();
	ScheduleNextSpawn();
}

void USpawnDirectorComponent::HandleZombieDied(AZombieCharacter* Zombie, AController* Killer)
{
	LiveZombies.Remove(Zombie);

	if (UWorld* World = GetWorld())
	{
		if (AZombieGameState* GameState = World->GetGameState<AZombieGameState>())
		{
			GameState->RemoveActiveZombie(Zombie);
		}
	}

	OnZombieKilled.Broadcast(Zombie, Killer);
	PublishEncounterState();

	if (bSectorActive && SpawnQueue.Num() == 0 && LiveZombies.Num() == 0)
	{
		bSectorActive = false;
		UE_LOG(LogZombieGame, Log, TEXT("Sector %d cleared: budget exhausted and no zombies remain."), CurrentSector);
		OnSectorCleared.Broadcast();
	}
}

void USpawnDirectorComponent::PublishEncounterState() const
{
	UWorld* World = GetWorld();
	AZombieGameState* GameState = World ? World->GetGameState<AZombieGameState>() : nullptr;
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
	GameState->SetRemainingEnemies(SpawnQueue.Num() + LiveZombies.Num());
}

void USpawnDirectorComponent::StopSector()
{
	bSectorActive = false;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SpawnTimer);

		AZombieGameState* GameState = World->GetGameState<AZombieGameState>();
		for (AZombieCharacter* Zombie : LiveZombies)
		{
			if (IsValid(Zombie))
			{
				if (GameState)
				{
					GameState->RemoveActiveZombie(Zombie);
				}
				Zombie->Destroy();
			}
		}
	}

	LiveZombies.Reset();
	SpawnQueue.Reset();
	AvailableSpawnPoints.Reset();
}
