#include "ActorPoolSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Utilities/PoolableActor.h"

UActorPoolSubsystem* UActorPoolSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UActorPoolSubsystem>() : nullptr;
}

AActor* UActorPoolSubsystem::Acquire(TSubclassOf<AActor> ActorClass, const FTransform& Transform, AActor* Owner, APawn* Instigator)
{
	UWorld* World = GetWorld();
	if (!World || !ActorClass)
	{
		return nullptr;
	}

	AActor* Actor = nullptr;
	FActorPoolBucket& Bucket = Pools.FindOrAdd(ActorClass.Get());
	while (!Actor && Bucket.Available.Num() > 0)
	{
		AActor* Candidate = Bucket.Available.Pop(EAllowShrinking::No);
		if (IsValid(Candidate))
		{
			Actor = Candidate;
		}
	}

	if (Actor)
	{
		ReleasedActors.Remove(Actor);
		Actor->SetOwner(Owner);
		Actor->SetInstigator(Instigator);
		Actor->SetActorTransform(Transform, false, nullptr, ETeleportType::ResetPhysics);
		Actor->SetActorHiddenInGame(false);
		Actor->SetActorEnableCollision(true);
	}
	else
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Owner;
		SpawnParams.Instigator = Instigator;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Actor = World->SpawnActor<AActor>(ActorClass, Transform, SpawnParams);
	}

	if (IPoolableActor* Poolable = Cast<IPoolableActor>(Actor))
	{
		Poolable->OnAcquiredFromPool();
	}

	return Actor;
}

void UActorPoolSubsystem::Release(AActor* Actor)
{
	if (!IsValid(Actor) || ReleasedActors.Contains(Actor))
	{
		return;
	}

	if (IPoolableActor* Poolable = Cast<IPoolableActor>(Actor))
	{
		Poolable->OnReleasedToPool();
	}

	Actor->SetActorHiddenInGame(true);
	Actor->SetActorEnableCollision(false);
	Actor->SetOwner(nullptr);

	ReleasedActors.Add(Actor);
	Pools.FindOrAdd(Actor->GetClass()).Available.Add(Actor);
}
