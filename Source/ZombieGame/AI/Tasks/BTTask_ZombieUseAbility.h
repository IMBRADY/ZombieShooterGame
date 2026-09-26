#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_ZombieUseAbility.generated.h"

struct FZombieUseAbilityMemory
{
	float RemainingBusyTime = 0.0f;
};

/**
 * Fires the first ready ability against the Blackboard target, then holds the zombie for the
 * ability's busy time (wind-up and recovery) before succeeding. Fails straight away if nothing is
 * ready, so the tree falls through to attacking or chasing.
 *
 * One task for every special behaviour: which abilities exist is data on the archetype.
 */
UCLASS()
class ZOMBIEGAME_API UBTTask_ZombieUseAbility : public UBTTask_BlackboardBase
{
	GENERATED_UCLASS_BODY()

	void Configure(FName TargetKeyName);

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual uint16 GetInstanceMemorySize() const override;
	virtual FString GetStaticDescription() const override;

protected:
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
