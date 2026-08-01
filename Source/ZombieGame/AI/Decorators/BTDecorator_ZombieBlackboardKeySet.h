#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Decorators/BTDecorator_BlackboardBase.h"
#include "BTDecorator_ZombieBlackboardKeySet.generated.h"

/**
 * Passes while a Blackboard key holds a value ("is this zombie chasing something?", "does it have
 * a noise to investigate?"), and aborts the running branch the moment that changes.
 *
 * The engine's own UBTDecorator_Blackboard covers this, but every knob it needs configuring
 * through is protected and editor-graph-driven; this project assembles its tree in C++ (see
 * UZombieAIAssetSubsystem), so it needs a node it can configure from code.
 */
UCLASS()
class ZOMBIEGAME_API UBTDecorator_ZombieBlackboardKeySet : public UBTDecorator_BlackboardBase
{
	GENERATED_UCLASS_BODY()

	/**
	 * @param KeyName		Blackboard key to watch.
	 * @param bRequireUnset	Invert: pass while the key is *empty*.
	 * @param AbortMode		Which running branches this decorator may interrupt when it flips.
	 */
	void Configure(FName KeyName, bool bRequireUnset, EBTFlowAbortMode::Type AbortMode);

	virtual FString GetStaticDescription() const override;

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
};
