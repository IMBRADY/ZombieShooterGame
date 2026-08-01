#include "BTTask_ZombieClearBlackboardValue.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_ZombieClearBlackboardValue::UBTTask_ZombieClearBlackboardValue(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = TEXT("Clear Blackboard Value");
}

void UBTTask_ZombieClearBlackboardValue::Configure(FName KeyName)
{
	BlackboardKey.SelectedKeyName = KeyName;
	NodeName = FString::Printf(TEXT("Clear %s"), *KeyName.ToString());
}

EBTNodeResult::Type UBTTask_ZombieClearBlackboardValue::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Blackboard)
	{
		return EBTNodeResult::Failed;
	}

	Blackboard->ClearValue(BlackboardKey.SelectedKeyName);
	return EBTNodeResult::Succeeded;
}

FString UBTTask_ZombieClearBlackboardValue::GetStaticDescription() const
{
	return FString::Printf(TEXT("Clear %s"), *BlackboardKey.SelectedKeyName.ToString());
}
