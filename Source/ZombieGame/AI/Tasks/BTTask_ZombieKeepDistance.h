#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_ZombieKeepDistance.generated.h"

struct FZombieKeepDistanceMemory
{
	float Elapsed = 0.0f;
	float StrafeSign = 1.0f;
};

/**
 * Ranged zombies "avoid the player": back away from the target until the archetype's preferred
 * range is restored, sliding sideways along walls rather than pinning themselves into a corner.
 * Bounded in time so a cornered Lobber still gets back to throwing.
 */
UCLASS()
class ZOMBIEGAME_API UBTTask_ZombieKeepDistance : public UBTTask_BlackboardBase
{
	GENERATED_UCLASS_BODY()

	void Configure(FName TargetKeyName, float InMaxDuration);

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual uint16 GetInstanceMemorySize() const override;
	virtual FString GetStaticDescription() const override;

protected:
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, Category = "Keep Distance", meta = (ClampMin = "0.1"))
	float MaxDuration = 1.4f;
};
