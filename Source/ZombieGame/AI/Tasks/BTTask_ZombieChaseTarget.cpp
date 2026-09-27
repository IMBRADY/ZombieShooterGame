#include "BTTask_ZombieChaseTarget.h"
#include "AIController.h"
#include "AI/ZombieAISettings.h"
#include "AI/ZombieFlowFieldSubsystem.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/Zombies/ZombieArchetypeDataAsset.h"
#include "Characters/Zombies/ZombieCharacter.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"

UBTTask_ZombieChaseTarget::UBTTask_ZombieChaseTarget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = TEXT("Chase Target (Flow Field)");
	bNotifyTick = true;
	bNotifyTaskFinished = true;
}

void UBTTask_ZombieChaseTarget::Configure(FName TargetKeyName)
{
	BlackboardKey.SelectedKeyName = TargetKeyName;
}

EBTNodeResult::Type UBTTask_ZombieChaseTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FZombieChaseTaskMemory* Memory = CastInstanceNodeMemory<FZombieChaseTaskMemory>(NodeMemory);
	*Memory = FZombieChaseTaskMemory();

	const AAIController* Controller = OwnerComp.GetAIOwner();
	if (const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr)
	{
		Memory->LastCheckLocation = Pawn->GetActorLocation();
	}
	return EBTNodeResult::InProgress;
}

uint16 UBTTask_ZombieChaseTarget::GetInstanceMemorySize() const
{
	return sizeof(FZombieChaseTaskMemory);
}

void UBTTask_ZombieChaseTarget::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	// A recovery path must not outlive the chase - the attack or roam branch that follows owns movement.
	FZombieChaseTaskMemory* Memory = CastInstanceNodeMemory<FZombieChaseTaskMemory>(NodeMemory);
	AAIController* Controller = OwnerComp.GetAIOwner();
	if (Memory->RecoveryRemaining > 0.0f && Controller)
	{
		Controller->StopMovement();
	}
	Memory->RecoveryRemaining = 0.0f;

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

bool UBTTask_ZombieChaseTarget::UpdateStuckRecovery(AAIController& Controller, const AZombieCharacter& Zombie, AActor& Target,
	float StopDistance, FZombieChaseTaskMemory& Memory, float DeltaSeconds) const
{
	const UZombieAISettings* Settings = UZombieAISettings::GetOrLoadDefault();
	if (!Settings)
	{
		return false;
	}

	if (Memory.RecoveryRemaining > 0.0f)
	{
		Memory.RecoveryRemaining -= DeltaSeconds;
		if (Memory.RecoveryRemaining > 0.0f && Controller.GetMoveStatus() != EPathFollowingStatus::Idle)
		{
			return true;
		}

		// Recovery over (or the path already finished): back to the shared field.
		Controller.StopMovement();
		Memory.RecoveryRemaining = 0.0f;
		Memory.LastCheckLocation = Zombie.GetActorLocation();
		Memory.SinceLastCheck = 0.0f;
		return false;
	}

	Memory.SinceLastCheck += DeltaSeconds;
	if (Memory.SinceLastCheck < Settings->StuckCheckInterval)
	{
		return false;
	}

	const bool bStuck = FVector::DistSquared2D(Zombie.GetActorLocation(), Memory.LastCheckLocation) < FMath::Square(Settings->StuckDistanceThreshold);
	Memory.LastCheckLocation = Zombie.GetActorLocation();
	Memory.SinceLastCheck = 0.0f;

	// Deliberately slow zombies (slowed, rooted) are not "stuck" - only ones that are trying to move.
	if (!bStuck || Zombie.GetCharacterMovement()->GetMaxSpeed() < Settings->StuckDistanceThreshold)
	{
		return false;
	}

	const EPathFollowingRequestResult::Type Request = Controller.MoveToActor(&Target, FMath::Max(StopDistance * 0.8f, 10.0f),
		/*bStopOnOverlap=*/true, /*bUsePathfinding=*/true, /*bCanStrafe=*/false, /*FilterClass=*/nullptr, /*bAllowPartialPath=*/true);
	if (Request == EPathFollowingRequestResult::RequestSuccessful)
	{
		Memory.RecoveryRemaining = Settings->StuckRecoverySeconds;
		return true;
	}
	return false;
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
	AAIController* Controller = OwnerComp.GetAIOwner();
	const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	AZombieCharacter* Zombie = Controller ? Cast<AZombieCharacter>(Controller->GetPawn()) : nullptr;
	AActor* Target = Blackboard ? Cast<AActor>(Blackboard->GetValueAsObject(BlackboardKey.SelectedKeyName)) : nullptr;

	if (!Zombie || Zombie->IsDead() || !IsValid(Target))
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	// Arriving is the attack branch's cue; the in-range decorator normally interrupts this task
	// first, but succeeding here keeps the tree correct if the service hasn't ticked yet. Ranged
	// zombies stop short, at the distance they prefer to fight from.
	const UZombieArchetypeDataAsset* Archetype = Zombie->GetArchetype();
	const bool bRanged = Archetype && Archetype->BehaviorProfile == EZombieBehaviorProfile::Ranged && Archetype->PreferredRange > 0.0f;
	const float StopDistance = bRanged ? Archetype->PreferredRange * 0.9f : Zombie->GetAttackRange();
	if (FVector::DistSquared2D(Zombie->GetActorLocation(), Target->GetActorLocation()) <= FMath::Square(StopDistance))
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	FZombieChaseTaskMemory* Memory = CastInstanceNodeMemory<FZombieChaseTaskMemory>(NodeMemory);
	if (UpdateStuckRecovery(*Controller, *Zombie, *Target, StopDistance, *Memory, DeltaSeconds))
	{
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
