#include "BTTask_ZombieFindRoamLocation.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "NavigationSystem.h"

UBTTask_ZombieFindRoamLocation::UBTTask_ZombieFindRoamLocation(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = TEXT("Find Roam Location");
}

void UBTTask_ZombieFindRoamLocation::Configure(FName RoamLocationKeyName, float InRoamRadius)
{
	BlackboardKey.SelectedKeyName = RoamLocationKeyName;
	RoamRadius = FMath::Max(InRoamRadius, 100.0f);
}

EBTNodeResult::Type UBTTask_ZombieFindRoamLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const AAIController* Controller = OwnerComp.GetAIOwner();
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (!Pawn || !Blackboard)
	{
		return EBTNodeResult::Failed;
	}

	UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(Pawn->GetWorld());
	if (!NavigationSystem)
	{
		return EBTNodeResult::Failed;
	}

	FNavLocation RoamDestination;
	if (!NavigationSystem->GetRandomReachablePointInRadius(Pawn->GetActorLocation(), RoamRadius, RoamDestination))
	{
		return EBTNodeResult::Failed;
	}

	Blackboard->SetValueAsVector(BlackboardKey.SelectedKeyName, RoamDestination.Location);
	return EBTNodeResult::Succeeded;
}

FString UBTTask_ZombieFindRoamLocation::GetStaticDescription() const
{
	return FString::Printf(TEXT("Find roam location within %.0f into %s"), RoamRadius, *BlackboardKey.SelectedKeyName.ToString());
}
