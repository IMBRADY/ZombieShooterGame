#include "BTService_ZombieCombatState.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/Zombies/ZombieCharacter.h"
#include "Components/HealthComponent.h"

UBTService_ZombieCombatState::UBTService_ZombieCombatState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = TEXT("Zombie Combat State");

	INIT_SERVICE_NODE_NOTIFY_FLAGS();

	Interval = 0.15f;
	RandomDeviation = 0.05f;
	bCallTickOnSearchStart = true;
}

void UBTService_ZombieCombatState::Configure(FName InTargetActorKey, FName InInAttackRangeKey, float InInterval)
{
	TargetActorKey = InTargetActorKey;
	InAttackRangeKey = InInAttackRangeKey;
	Interval = FMath::Max(InInterval, 0.01f);
}

void UBTService_ZombieCombatState::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	const AAIController* Controller = OwnerComp.GetAIOwner();
	const AZombieCharacter* Zombie = Controller ? Cast<AZombieCharacter>(Controller->GetPawn()) : nullptr;
	if (!Blackboard || !Zombie)
	{
		return;
	}

	AActor* Target = Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey));

	// A dead target is not a target: without this the whole horde would keep swinging at a corpse.
	bool bTargetIsValid = IsValid(Target);
	if (bTargetIsValid)
	{
		if (const UHealthComponent* TargetHealth = Target->FindComponentByClass<UHealthComponent>())
		{
			bTargetIsValid = !TargetHealth->IsDead();
		}
	}

	if (!bTargetIsValid)
	{
		Blackboard->ClearValue(TargetActorKey);
		Blackboard->SetValueAsBool(InAttackRangeKey, false);
		return;
	}

	const float Reach = Zombie->GetAttackRange();
	const bool bInRange = FVector::DistSquared(Zombie->GetActorLocation(), Target->GetActorLocation()) <= FMath::Square(Reach);

	Blackboard->SetValueAsBool(InAttackRangeKey, bInRange);
}

FString UBTService_ZombieCombatState::GetStaticServiceDescription() const
{
	return FString::Printf(TEXT("Track %s, maintain %s"), *TargetActorKey.ToString(), *InAttackRangeKey.ToString());
}
