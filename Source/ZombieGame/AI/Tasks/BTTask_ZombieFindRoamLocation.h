#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_ZombieFindRoamLocation.generated.h"

/**
 * Picks a navigable wander destination near the zombie and writes it to a Blackboard vector key.
 *
 * Deliberately a plain navigation query rather than EQS: idle wandering has no criteria worth
 * scoring, and hundreds of zombies running environment queries would cost far more than it
 * returns. EQS is reserved for choices that actually have a best answer - Lobber stand-off
 * positions, flanking - per ARCHITECTURE.md 6.
 */
UCLASS()
class ZOMBIEGAME_API UBTTask_ZombieFindRoamLocation : public UBTTask_BlackboardBase
{
	GENERATED_UCLASS_BODY()

	void Configure(FName RoamLocationKeyName, float InRoamRadius);

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

protected:
	UPROPERTY(EditAnywhere, Category = "Roaming", meta = (ClampMin = "100.0"))
	float RoamRadius = 1200.0f;
};
