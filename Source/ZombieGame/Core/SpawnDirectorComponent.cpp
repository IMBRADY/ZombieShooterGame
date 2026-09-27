#include "SpawnDirectorComponent.h"
#include "Characters/Zombies/ZombieCharacter.h"
#include "Characters/Zombies/ZombieEnemyManager.h"
#include "Core/SpawnDirectorSettings.h"
#include "Core/ZombieGameState.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "Utilities/ZombiePrimaryAssetLoader.h"
#include "ZombieGame.h"

namespace
{
	/** How often rooms are checked for being cleared - cheap, and nobody notices half a second. */
	constexpr float SpawnAreaCheckInterval = 0.5f;

	/** Standing in a doorway counts as being in the room. */
	constexpr float SpawnAreaVisitTolerance = 60.0f;
}

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

void USpawnDirectorComponent::BeginSector(int32 Sector, const TArray<FZombieSpawnArea>& InSpawnAreas, const UZombieArchetypeDataAsset* BossArchetype)
{
	StopSector();

	USpawnDirectorSettings* Settings = ResolveSettings();
	UZombieEnemyManager* Enemies = UZombieEnemyManager::Get(this);
	if (!Settings || !Enemies)
	{
		return;
	}

	if (InSpawnAreas.Num() == 0)
	{
		UE_LOG(LogZombieGame, Error, TEXT("Sector %d has no zombie spawn points; nothing will spawn."), Sector);
		return;
	}

	TArray<UZombieArchetypeDataAsset*> EligibleArchetypes;
	GatherEligibleArchetypes(Sector, EligibleArchetypes);

	CurrentSector = Sector;
	SpawnAreas = InSpawnAreas;
	AreaVisited.Init(false, SpawnAreas.Num());
	AreaCleared.Init(false, SpawnAreas.Num());
	SpawnBlockedSeconds = 0.0f;
	PendingBoss = BossArchetype;
	bSectorActive = true;
	GetWorld()->GetTimerManager().SetTimer(AreaCheckTimer, this, &USpawnDirectorComponent::UpdateClearedAreas, SpawnAreaCheckInterval, true);
	Enemies->SetSectorScaling(Settings->GetScalingForSector(Sector));

	BuildSpawnQueue(Sector, Settings->GetBudgetForSector(Sector), EligibleArchetypes);
	PublishEncounterState();

	if (SpawnQueue.Num() == 0 && !PendingBoss)
	{
		// Nothing affordable ever came out of the budget - treat the sector as immediately clear
		// rather than leaving the run stuck waiting on zombies that will never arrive.
		UE_LOG(LogZombieGame, Warning, TEXT("Sector %d produced an empty spawn queue; clearing immediately."), Sector);
		CheckSectorCleared(SpawnAreas[0].Bounds.GetCenter());
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

void USpawnDirectorComponent::GatherPlayerLocations(TArray<FVector>& OutLocations) const
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (const APawn* Pawn = It->IsValid() ? It->Get()->GetPawn() : nullptr)
		{
			OutLocations.Add(Pawn->GetActorLocation());
		}
	}
}

bool USpawnDirectorComponent::IsOnScreenForAnyPlayer(const FVector& Location, float Margin) const
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		const APlayerController* PlayerController = It->Get();
		if (!PlayerController || !PlayerController->IsLocalController() || !PlayerController->PlayerCameraManager)
		{
			continue;
		}

		// Tested in camera space against the real view rather than by projecting to pixels, so it
		// also works with no viewport at all (headless runs) and for the angled orthographic camera.
		const FMinimalViewInfo& View = PlayerController->PlayerCameraManager->GetCameraCacheView();
		const FVector Local = View.Rotation.UnrotateVector(Location - View.Location);

		float AspectRatio = View.AspectRatio > 0.0f ? View.AspectRatio : 16.0f / 9.0f;
		int32 ViewportWidth = 0;
		int32 ViewportHeight = 0;
		PlayerController->GetViewportSize(ViewportWidth, ViewportHeight);
		if (ViewportWidth > 0 && ViewportHeight > 0)
		{
			AspectRatio = static_cast<float>(ViewportWidth) / static_cast<float>(ViewportHeight);
		}

		float HalfWidth = 0.0f;
		if (View.ProjectionMode == ECameraProjectionMode::Orthographic)
		{
			HalfWidth = View.OrthoWidth * 0.5f;
		}
		else if (Local.X > 0.0f)
		{
			HalfWidth = Local.X * FMath::Tan(FMath::DegreesToRadians(View.FOV * 0.5f));
		}
		else
		{
			continue;
		}

		const float HalfHeight = HalfWidth / AspectRatio;
		if (FMath::Abs(Local.Y) <= HalfWidth + Margin && FMath::Abs(Local.Z) <= HalfHeight + Margin)
		{
			return true;
		}
	}
	return false;
}

void USpawnDirectorComponent::UpdateClearedAreas()
{
	const USpawnDirectorSettings* Settings = SettingsAsset.Get();
	const UZombieEnemyManager* Enemies = UZombieEnemyManager::Get(this);
	if (!bSectorActive || !Enemies || (Settings && !Settings->bKeepClearedRoomsClear))
	{
		return;
	}

	TArray<FVector> PlayerLocations;
	GatherPlayerLocations(PlayerLocations);

	for (int32 AreaIndex = 0; AreaIndex < SpawnAreas.Num(); ++AreaIndex)
	{
		if (AreaCleared[AreaIndex])
		{
			continue;
		}

		const FBox VisitBounds = SpawnAreas[AreaIndex].Bounds.ExpandBy(SpawnAreaVisitTolerance);
		for (const FVector& PlayerLocation : PlayerLocations)
		{
			AreaVisited[AreaIndex] = AreaVisited[AreaIndex] || VisitBounds.IsInsideXY(PlayerLocation);
		}
		if (!AreaVisited[AreaIndex])
		{
			continue;
		}

		const FBox& Bounds = SpawnAreas[AreaIndex].Bounds;
		const bool bOccupied = Enemies->GetLiveZombies().ContainsByPredicate([&Bounds](const AZombieCharacter* Zombie)
		{
			return IsValid(Zombie) && !Zombie->IsDead() && Bounds.IsInsideXY(Zombie->GetActorLocation());
		});
		if (!bOccupied)
		{
			AreaCleared[AreaIndex] = true;
			UE_LOG(LogZombieGame, Log, TEXT("Sector %d: room %d cleared - no further spawns there."), CurrentSector, AreaIndex);
		}
	}
}

bool USpawnDirectorComponent::TrySelectSpawnLocation(FVector& OutLocation) const
{
	const USpawnDirectorSettings* Settings = SettingsAsset.Get();
	const float MinDistanceSquared = FMath::Square(Settings ? Settings->MinSpawnDistanceFromPlayer : 900.0f);
	const float Margin = Settings ? Settings->OffscreenSpawnMargin : 250.0f;

	TArray<FVector> PlayerLocations;
	GatherPlayerLocations(PlayerLocations);

	// Candidates come from rooms the player has not cleared - or, only once every room is cleared,
	// from any room. Points outside the "don't spawn in their lap" radius beat ones inside it.
	// Anything on screen is never a candidate.
	const bool bAllAreasCleared = !AreaCleared.Contains(false);
	TArray<FVector> Far;
	TArray<FVector> Near;

	for (int32 AreaIndex = 0; AreaIndex < SpawnAreas.Num(); ++AreaIndex)
	{
		if (AreaCleared[AreaIndex] && !bAllAreasCleared)
		{
			continue;
		}

		for (const FVector& SpawnPoint : SpawnAreas[AreaIndex].SpawnPoints)
		{
			if (IsOnScreenForAnyPlayer(SpawnPoint, Margin))
			{
				continue;
			}

			const bool bTooClose = PlayerLocations.ContainsByPredicate([&SpawnPoint, MinDistanceSquared](const FVector& PlayerLocation)
			{
				return FVector::DistSquared(SpawnPoint, PlayerLocation) < MinDistanceSquared;
			});
			(bTooClose ? Near : Far).Add(SpawnPoint);
		}
	}

	const TArray<FVector>& Pool = Far.Num() > 0 ? Far : Near;
	if (Pool.Num() == 0)
	{
		return false;
	}

	OutLocation = Pool[FMath::RandRange(0, Pool.Num() - 1)];
	return true;
}

FVector USpawnDirectorComponent::GetFurthestSpawnPoint(const TArray<FVector>& PlayerLocations) const
{
	FVector FurthestPoint = SpawnAreas[0].SpawnPoints[0];
	float FurthestDistanceSquared = -1.0f;

	for (const FZombieSpawnArea& Area : SpawnAreas)
	{
		for (const FVector& SpawnPoint : Area.SpawnPoints)
		{
			float NearestPlayerDistanceSquared = TNumericLimits<float>::Max();
			for (const FVector& PlayerLocation : PlayerLocations)
			{
				NearestPlayerDistanceSquared = FMath::Min(NearestPlayerDistanceSquared, static_cast<float>(FVector::DistSquared(SpawnPoint, PlayerLocation)));
			}

			if (NearestPlayerDistanceSquared > FurthestDistanceSquared)
			{
				FurthestDistanceSquared = NearestPlayerDistanceSquared;
				FurthestPoint = SpawnPoint;
			}
		}
	}
	return FurthestPoint;
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
	if (Enemies->GetLiveCount() < MaxConcurrent && SpawnAreas.Num() > 0)
	{
		FVector SpawnLocation;
		bool bHasLocation = TrySelectSpawnLocation(SpawnLocation);
		if (bHasLocation)
		{
			SpawnBlockedSeconds = 0.0f;
		}
		else
		{
			// Every spawn point is on screen. Wait for the camera to move on - but not forever, or a
			// player parked where they can see everything could never finish the sector.
			SpawnBlockedSeconds += Settings ? Settings->SpawnInterval : 1.0f;
			if (SpawnBlockedSeconds >= (Settings ? Settings->MaxOffscreenWaitSeconds : 10.0f))
			{
				TArray<FVector> PlayerLocations;
				GatherPlayerLocations(PlayerLocations);
				SpawnLocation = GetFurthestSpawnPoint(PlayerLocations);
				bHasLocation = true;
			}
		}

		if (bHasLocation && Enemies->SpawnZombie(SpawnQueue[0], SpawnLocation, FZombieSpawnOptions()))
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
		World->GetTimerManager().ClearTimer(AreaCheckTimer);
	}
	if (UZombieEnemyManager* Enemies = UZombieEnemyManager::Get(this))
	{
		Enemies->ClearAll();
	}

	SpawnQueue.Reset();
	SpawnAreas.Reset();
	AreaVisited.Reset();
	AreaCleared.Reset();
	PublishEncounterState();
}
