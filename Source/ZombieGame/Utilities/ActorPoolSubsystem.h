#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ActorPoolSubsystem.generated.h"

USTRUCT()
struct FActorPoolBucket
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<AActor>> Available;
};

/**
 * Object pooling for the high-churn actors the spec calls out - projectiles, effects, pickups
 * (ARCHITECTURE.md 8). Spawning and destroying hundreds of these per second is exactly the cost the
 * spec's "thousands of projectiles" target cannot afford; recycling them makes it a transform write.
 *
 * Released actors are hidden and non-colliding, never destroyed, until the world tears down.
 */
UCLASS()
class ZOMBIEGAME_API UActorPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** A recycled instance if one is free, otherwise a freshly spawned one. Never null for a valid class. */
	AActor* Acquire(TSubclassOf<AActor> ActorClass, const FTransform& Transform, AActor* Owner = nullptr, APawn* Instigator = nullptr);

	template <typename TActor>
	TActor* Acquire(TSubclassOf<TActor> ActorClass, const FTransform& Transform, AActor* Owner = nullptr, APawn* Instigator = nullptr)
	{
		return Cast<TActor>(Acquire(TSubclassOf<AActor>(ActorClass), Transform, Owner, Instigator));
	}

	/** Returns an actor to its pool. Safe to call on an actor that is already released. */
	void Release(AActor* Actor);

	static UActorPoolSubsystem* Get(const UObject* WorldContext);

private:
	UPROPERTY(Transient)
	TMap<TObjectPtr<UClass>, FActorPoolBucket> Pools;

	UPROPERTY(Transient)
	TSet<TObjectPtr<AActor>> ReleasedActors;
};
