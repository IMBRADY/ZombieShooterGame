#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_ZombieCombatState.generated.h"

/**
 * Keeps the combat-relevant Blackboard keys honest while a zombie has a target: drops the target
 * once it dies or disappears, and maintains the "close enough to swing" flag the attack branch is
 * gated on.
 *
 * This is the one thing that genuinely has to be sampled over time (distance changes continuously
 * as both parties move), so it runs on a service interval rather than the zombie's Tick - which
 * stays disabled entirely.
 */
UCLASS()
class ZOMBIEGAME_API UBTService_ZombieCombatState : public UBTService
{
	GENERATED_UCLASS_BODY()

	void Configure(FName InTargetActorKey, FName InInAttackRangeKey, float InInterval);

	virtual FString GetStaticServiceDescription() const override;

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName TargetActorKey;

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName InAttackRangeKey;
};
