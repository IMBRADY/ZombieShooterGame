#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_ZombieAttack.generated.h"

/** Per-instance state for one swing; template nodes are shared, so nothing lives on the node. */
struct FZombieAttackTaskMemory
{
	float ElapsedTime = 0.0f;
	bool bDamageApplied = false;
};

/**
 * One melee swing against the Blackboard's target: wind up, land the hit if the target is still in
 * reach, then hold for the rest of the archetype's attack interval before succeeding.
 *
 * Timings and damage come from the zombie's archetype Data Asset, so a slower, harder-hitting
 * variant is content, not code.
 */
UCLASS()
class ZOMBIEGAME_API UBTTask_ZombieAttack : public UBTTask_BlackboardBase
{
	GENERATED_UCLASS_BODY()

	void Configure(FName TargetKeyName);

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual uint16 GetInstanceMemorySize() const override;
	virtual FString GetStaticDescription() const override;

protected:
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

private:
	/** Resolves the pawn and the current target, or returns false if either is gone. */
	static bool ResolveCombatants(UBehaviorTreeComponent& OwnerComp, FName TargetKeyName,
		class AZombieCharacter*& OutZombie, AActor*& OutTarget);
};
