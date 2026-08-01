#include "BTTask_ZombieChaseTarget.h"
#include "AIController.h"
#include "AI/ZombieAISettings.h"
#include "AI/ZombieFlowFieldSubsystem.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/Zombies/ZombieCharacter.h"
#include "Engine/World.h"

UBTTask_ZombieChaseTarget::UBTTask_ZombieChaseTarget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = TEXT("Chase Target (Flow Field)");
	bNotifyTick = true;
}

void UBTTask_ZombieChaseTarget::Configure(FName TargetKeyName)
{
	BlackboardKey.SelectedKeyName = TargetKeyName;
}

EBTNodeResult::Type UBTTask_ZombieChaseTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	return EBTNodeResult::InProgress;
}

FVector UBTTask_ZombieChaseTarget::ResolveChaseDirection(const AZombieCharacter& Zombie, const AActor& Target) const
{
	const FVector StraightLine = (Target.GetActorLocation() - Zombie.GetActorLocation()).GetSafeNormal2D();

	// The field is flooded from the players, so it only answers for a player target.
	const APawn* TargetPawn = Cast<APawn>(&Target);
	if (!TargetPawn || !TargetPawn->IsPlayerControlled())
	{
		return StraightLine;
	}

	const UZombieAISettings* Settings = UZombieAISettings::GetOrLoadDefault();
	if (Settings && !Settings->bUseFlowFieldForChase)
	{
		return StraightLine;
	}

	const UWorld* World = Zombie.GetWorld();
	const UZombieFlowFieldSubsystem* FlowField = World ? World->GetSubsystem<UZombieFlowFieldSubsystem>() : nullptr;

	FVector FlowDirection = FVector::ZeroVector;
	if (FlowField && FlowField->GetFlowDirection(Zombie.GetActorLocation(), FlowDirection))
	{
		return FlowDirection;
	}

	return StraightLine;
}

void UBTTask_ZombieChaseTarget::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	const AAIController* Controller = OwnerComp.GetAIOwner();
	const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	AZombieCharacter* Zombie = Controller ? Cast<AZombieCharacter>(Controller->GetPawn()) : nullptr;
	AActor* Target = Blackboard ? Cast<AActor>(Blackboard->GetValueAsObject(BlackboardKey.SelectedKeyName)) : nullptr;

	if (!Zombie || Zombie->IsDead() || !IsValid(Target))
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// Arriving is the attack branch's cue; the in-range decorator normally interrupts this task
	// first, but succeeding here keeps the tree correct if the service hasn't ticked yet.
	const float Reach = Zombie->GetAttackRange();
	if (FVector::DistSquared2D(Zombie->GetActorLocation(), Target->GetActorLocation()) <= FMath::Square(Reach))
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	const FVector ChaseDirection = ResolveChaseDirection(*Zombie, *Target);
	if (!ChaseDirection.IsNearlyZero())
	{
		Zombie->AddMovementInput(ChaseDirection, 1.0f);
	}
}

FString UBTTask_ZombieChaseTarget::GetStaticDescription() const
{
	return FString::Printf(TEXT("Chase %s via shared flow field"), *BlackboardKey.SelectedKeyName.ToString());
}
