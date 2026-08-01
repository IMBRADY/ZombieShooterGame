#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_ZombieClearBlackboardValue.generated.h"

/**
 * Clears a Blackboard key, ending whatever branch that key was gating - used to drop an
 * investigate point once the zombie has walked over and found nothing there.
 */
UCLASS()
class ZOMBIEGAME_API UBTTask_ZombieClearBlackboardValue : public UBTTask_BlackboardBase
{
	GENERATED_UCLASS_BODY()

	void Configure(FName KeyName);

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;
};
