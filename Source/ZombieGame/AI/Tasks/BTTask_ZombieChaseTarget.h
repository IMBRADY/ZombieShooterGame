#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_ZombieChaseTarget.generated.h"

class AZombieCharacter;

/**
 * Walks the zombie down the horde's shared flow field until its target is in reach.
 *
 * The alternative - giving every zombie its own MoveTo against a moving player - makes each
 * zombie re-path independently and repeatedly, so cost scales with the size of the horde exactly
 * where the horde is largest. Reading a field that was flooded outward from the players once for
 * everyone costs the same whether one zombie is chasing or two hundred are.
 *
 * Falls back to steering straight at the target when the field has nothing to say (target is not
 * a player, zombie stepped off the sector grid, or the field is walled off) so a zombie never
 * simply stops.
 */
UCLASS()
class ZOMBIEGAME_API UBTTask_ZombieChaseTarget : public UBTTask_BlackboardBase
{
	GENERATED_UCLASS_BODY()

	void Configure(FName TargetKeyName);

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual FString GetStaticDescription() const override;

protected:
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

private:
	/** Direction to travel this frame: flow field where available, straight line otherwise. */
	FVector ResolveChaseDirection(const AZombieCharacter& Zombie, const AActor& Target) const;
};
