#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_ZombieCombatState.generated.h"

class AZombieCharacter;
class UBlackboardComponent;

/**
 * Keeps the combat-relevant Blackboard keys honest while a zombie has a target: drops the target
 * once it dies or disappears, and maintains the distance flags the combat branches are gated on -
 * in attack reach, too close (ranged zombies back off), at preferred range (ranged zombies hold) -
 * plus whether any of its abilities is ready to fire.
 *
 * This is the one thing that genuinely has to be sampled over time (distance changes continuously
 * as both parties move), so it runs on a service interval rather than the zombie's Tick - which
 * stays disabled entirely.
 */
UCLASS()
class ZOMBIEGAME_API UBTService_ZombieCombatState : public UBTService
{
	GENERATED_UCLASS_BODY()

	/**
	 * @param bInAlwaysHunt	Bosses never lose interest: with no target they lock onto the nearest player.
	 */
	void Configure(FName InTargetActorKey, FName InInAttackRangeKey, float InInterval, bool bInAlwaysHunt = false);

	virtual FString GetStaticServiceDescription() const override;

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName TargetActorKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName InAttackRangeKey;

	UPROPERTY(EditAnywhere, Category = "Behaviour")
	bool bAlwaysHunt = false;

private:
	static AActor* FindNearestPlayer(const AZombieCharacter& Zombie);
	static bool IsTargetAlive(const AActor* Target);
	static void ClearCombatKeys(UBlackboardComponent& Blackboard, FName TargetKey, FName RangeKey);
};
