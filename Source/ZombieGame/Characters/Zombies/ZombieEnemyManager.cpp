#include "ZombieEnemyManager.h"
#include "Characters/Zombies/ZombieCharacter.h"
#include "Core/ZombieGameState.h"
#include "Engine/World.h"
#include "ZombieGame.h"

UZombieEnemyManager* UZombieEnemyManager::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UZombieEnemyManager>() : nullptr;
}

AZombieCharacter* UZombieEnemyManager::SpawnZombie(const UZombieArchetypeDataAsset* Archetype, const FVector& Location, const FZombieSpawnOptions& Options)
{
	UWorld* World = GetWorld();
	if (!World || !Archetype)
	{
		return nullptr;
	}

	UClass* ZombieClass = Archetype->ZombieClass.IsNull() ? AZombieCharacter::StaticClass() : Archetype->ZombieClass.LoadSynchronous();
	const FTransform SpawnTransform(FRotator(0.0f, FMath::FRandRange(0.0f, 360.0f), 0.0f), Location);

	// Deferred so the archetype is applied before BeginPlay: the pawn is never briefly alive with
	// default stats, and its AI controller reads final numbers on possession.
	AZombieCharacter* Zombie = World->SpawnActorDeferred<AZombieCharacter>(ZombieClass ? ZombieClass : AZombieCharacter::StaticClass(),
		SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!Zombie)
	{
		UE_LOG(LogZombieGame, Warning, TEXT("Failed to spawn zombie '%s'."), *Archetype->GetName());
		return nullptr;
	}

	Zombie->InitializeFromArchetype(Archetype, Scaling, Options.bGrantsRewards);
	Zombie->OnZombieDied.AddUObject(this, &UZombieEnemyManager::HandleZombieDied);
	Zombie->FinishSpawning(SpawnTransform);

	LiveZombies.Add(Zombie);
	SyncGameState(Zombie, true);
	OnEnemySpawned.Broadcast(Zombie);
	return Zombie;
}

void UZombieEnemyManager::HandleZombieDied(AZombieCharacter* Zombie, AController* Killer)
{
	LiveZombies.Remove(Zombie);
	SyncGameState(Zombie, false);
	OnEnemyDied.Broadcast(Zombie, Killer);
}

void UZombieEnemyManager::SyncGameState(AZombieCharacter* Zombie, bool bAdded) const
{
	AZombieGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AZombieGameState>() : nullptr;
	if (!GameState)
	{
		return;
	}

	if (bAdded)
	{
		GameState->AddActiveZombie(Zombie);
	}
	else
	{
		GameState->RemoveActiveZombie(Zombie);
	}
}

void UZombieEnemyManager::RegisterCorpse(AZombieCharacter* Corpse)
{
	Corpses.RemoveAll([](const TWeakObjectPtr<AZombieCharacter>& Entry) { return !Entry.IsValid(); });
	if (Corpse)
	{
		Corpses.Add(Corpse);
	}
}

bool UZombieEnemyManager::HasCorpseNear(const FVector& Location, float Radius) const
{
	const float RadiusSquared = FMath::Square(Radius);
	return Corpses.ContainsByPredicate([&Location, RadiusSquared](const TWeakObjectPtr<AZombieCharacter>& Corpse)
	{
		return Corpse.IsValid() && FVector::DistSquared2D(Corpse->GetActorLocation(), Location) <= RadiusSquared;
	});
}

int32 UZombieEnemyManager::ReviveCorpsesNear(const FVector& Location, float Radius, int32 MaxCount)
{
	const float RadiusSquared = FMath::Square(Radius);
	int32 Revived = 0;

	for (int32 Index = Corpses.Num() - 1; Index >= 0 && Revived < MaxCount; --Index)
	{
		AZombieCharacter* Corpse = Corpses[Index].Get();
		if (!Corpse || FVector::DistSquared2D(Corpse->GetActorLocation(), Location) > RadiusSquared)
		{
			continue;
		}

		const UZombieArchetypeDataAsset* Archetype = Corpse->GetArchetype();
		const FVector CorpseLocation = Corpse->GetActorLocation() + FVector(0.0f, 0.0f, 20.0f);
		Corpses.RemoveAt(Index);
		Corpse->Destroy();

		FZombieSpawnOptions Options;
		Options.bGrantsRewards = false;
		if (AZombieCharacter* Risen = SpawnZombie(Archetype, CorpseLocation, Options))
		{
			Risen->PlayRiseEffect();
			++Revived;
		}
	}
	return Revived;
}

void UZombieEnemyManager::ClearAll()
{
	const TArray<TObjectPtr<AZombieCharacter>> Snapshot = LiveZombies;
	for (AZombieCharacter* Zombie : Snapshot)
	{
		if (IsValid(Zombie))
		{
			SyncGameState(Zombie, false);
			Zombie->Destroy();
		}
	}
	LiveZombies.Reset();

	for (const TWeakObjectPtr<AZombieCharacter>& Corpse : Corpses)
	{
		if (Corpse.IsValid())
		{
			Corpse->Destroy();
		}
	}
	Corpses.Reset();
}
