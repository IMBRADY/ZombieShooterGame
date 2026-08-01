#include "BTTask_ZombieAttack.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/Zombies/ZombieArchetypeDataAsset.h"
#include "Characters/Zombies/ZombieCharacter.h"

UBTTask_ZombieAttack::UBTTask_ZombieAttack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = TEXT("Zombie Attack");
	bNotifyTick = true;
}

void UBTTask_ZombieAttack::Configure(FName TargetKeyName)
{
	BlackboardKey.SelectedKeyName = TargetKeyName;
}

uint16 UBTTask_ZombieAttack::GetInstanceMemorySize() const
{
	return sizeof(FZombieAttackTaskMemory);
}

bool UBTTask_ZombieAttack::ResolveCombatants(UBehaviorTreeComponent& OwnerComp, FName TargetKeyName,
	AZombieCharacter*& OutZombie, AActor*& OutTarget)
{
	OutZombie = nullptr;
	OutTarget = nullptr;

	const AAIController* Controller = OwnerComp.GetAIOwner();
	const UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Controller || !Blackboard)
	{
		return false;
	}

	OutZombie = Cast<AZombieCharacter>(Controller->GetPawn());
	OutTarget = Cast<AActor>(Blackboard->GetValueAsObject(TargetKeyName));

	return OutZombie != nullptr && !OutZombie->IsDead() && IsValid(OutTarget);
}

EBTNodeResult::Type UBTTask_ZombieAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AZombieCharacter* Zombie = nullptr;
	AActor* Target = nullptr;
	if (!ResolveCombatants(OwnerComp, BlackboardKey.SelectedKeyName, Zombie, Target))
	{
		return EBTNodeResult::Failed;
	}

	// Face the target for the wind-up so the swing reads on screen before it lands.
	FVector Facing = Target->GetActorLocation() - Zombie->GetActorLocation();
	Facing.Z = 0.0f;
	if (!Facing.IsNearlyZero())
	{
		Zombie->SetActorRotation(Facing.Rotation());
	}

	FZombieAttackTaskMemory* Memory = CastInstanceNodeMemory<FZombieAttackTaskMemory>(NodeMemory);
	Memory->ElapsedTime = 0.0f;
	Memory->bDamageApplied = false;

	return EBTNodeResult::InProgress;
}

void UBTTask_ZombieAttack::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FZombieAttackTaskMemory* Memory = CastInstanceNodeMemory<FZombieAttackTaskMemory>(NodeMemory);
	Memory->ElapsedTime += DeltaSeconds;

	AZombieCharacter* Zombie = nullptr;
	AActor* Target = nullptr;
	if (!ResolveCombatants(OwnerComp, BlackboardKey.SelectedKeyName, Zombie, Target))
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	const UZombieArchetypeDataAsset* Archetype = Zombie->GetArchetype();
	const float Windup = Archetype ? Archetype->AttackWindup : 0.45f;
	const float Interval = Archetype ? Archetype->AttackInterval : 1.4f;

	if (!Memory->bDamageApplied && Memory->ElapsedTime >= Windup)
	{
		Memory->bDamageApplied = true;

		// Re-check reach at the moment of impact: backing off during the wind-up is the whole
		// point of having one.
		const float Reach = Zombie->GetAttackRange();
		if (FVector::DistSquared(Zombie->GetActorLocation(), Target->GetActorLocation()) <= FMath::Square(Reach))
		{
			Zombie->PerformAttack(Target);
		}
	}

	if (Memory->ElapsedTime >= FMath::Max(Interval, Windup))
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

FString UBTTask_ZombieAttack::GetStaticDescription() const
{
	return FString::Printf(TEXT("Attack %s"), *BlackboardKey.SelectedKeyName.ToString());
}
