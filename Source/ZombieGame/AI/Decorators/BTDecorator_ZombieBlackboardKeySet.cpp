#include "BTDecorator_ZombieBlackboardKeySet.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"

UBTDecorator_ZombieBlackboardKeySet::UBTDecorator_ZombieBlackboardKeySet(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = TEXT("Blackboard Key Is Set");
}

void UBTDecorator_ZombieBlackboardKeySet::Configure(FName KeyName, bool bRequireUnset, EBTFlowAbortMode::Type AbortMode)
{
	BlackboardKey.SelectedKeyName = KeyName;
	FlowAbortMode = AbortMode;
	SetIsInversed(bRequireUnset);
	NodeName = FString::Printf(TEXT("%s %s"), *KeyName.ToString(), bRequireUnset ? TEXT("is not set") : TEXT("is set"));
}

bool UBTDecorator_ZombieBlackboardKeySet::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Blackboard || !BlackboardKey.IsSet())
	{
		return false;
	}

	const FBlackboard::FKey KeyID = BlackboardKey.GetSelectedKeyID();

	if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Object::StaticClass())
	{
		return IsValid(Blackboard->GetValue<UBlackboardKeyType_Object>(KeyID));
	}

	if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Vector::StaticClass())
	{
		return Blackboard->IsVectorValueSet(KeyID);
	}

	if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Bool::StaticClass())
	{
		return Blackboard->GetValue<UBlackboardKeyType_Bool>(KeyID);
	}

	return false;
}

FString UBTDecorator_ZombieBlackboardKeySet::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s: %s %s"), *Super::GetStaticDescription(),
		*BlackboardKey.SelectedKeyName.ToString(), IsInversed() ? TEXT("is not set") : TEXT("is set"));
}
