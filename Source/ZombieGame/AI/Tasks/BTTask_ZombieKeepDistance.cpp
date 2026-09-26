#include "BTTask_ZombieKeepDistance.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/Zombies/ZombieArchetypeDataAsset.h"
#include "Characters/Zombies/ZombieCharacter.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"

namespace
{
	/** How far ahead a retreat checks for a wall before committing to a direction. */
	constexpr float WallProbeDistance = 180.0f;
}

UBTTask_ZombieKeepDistance::UBTTask_ZombieKeepDistance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = TEXT("Keep Distance");
	bNotifyTick = true;
}

void UBTTask_ZombieKeepDistance::Configure(FName TargetKeyName, float InMaxDuration)
{
	BlackboardKey.SelectedKeyName = TargetKeyName;
	MaxDuration = FMath::Max(InMaxDuration, 0.1f);
}

uint16 UBTTask_ZombieKeepDistance::GetInstanceMemorySize() const
{
	return sizeof(FZombieKeepDistanceMemory);
}

EBTNodeResult::Type UBTTask_ZombieKeepDistance::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FZombieKeepDistanceMemory* Memory = CastInstanceNodeMemory<FZombieKeepDistanceMemory>(NodeMemory);
	Memory->Elapsed = 0.0f;
	Memory->StrafeSign = FMath::RandBool() ? 1.0f : -1.0f;
	return EBTNodeResult::InProgress;
}

void UBTTask_ZombieKeepDistance::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FZombieKeepDistanceMemory* Memory = CastInstanceNodeMemory<FZombieKeepDistanceMemory>(NodeMemory);
	Memory->Elapsed += DeltaSeconds;

	const AAIController* Controller = OwnerComp.GetAIOwner();
	AZombieCharacter* Zombie = Controller ? Cast<AZombieCharacter>(Controller->GetPawn()) : nullptr;
	const AActor* Target = Cast<AActor>(OwnerComp.GetBlackboardComponent()->GetValueAsObject(BlackboardKey.SelectedKeyName));
	const UZombieArchetypeDataAsset* Archetype = Zombie ? Zombie->GetArchetype() : nullptr;
	if (!Zombie || Zombie->IsDead() || !Target || !Archetype)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	const FVector Away = (Zombie->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D();
	const float Distance = FVector::Dist2D(Zombie->GetActorLocation(), Target->GetActorLocation());
	if (Distance >= Archetype->PreferredRange * 0.85f || Memory->Elapsed >= MaxDuration)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		return;
	}

	// Straight back if there's room; otherwise slide along the wall to one side.
	FVector Direction = Away;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ZombieRetreatProbe), false, Zombie);
	if (Zombie->GetWorld()->LineTraceTestByChannel(Zombie->GetActorLocation(), Zombie->GetActorLocation() + Away * WallProbeDistance, ECC_WorldStatic, Params))
	{
		Direction = FVector::CrossProduct(Away, FVector::UpVector) * Memory->StrafeSign;
	}

	Zombie->AddMovementInput(Direction, 1.0f);
}

FString UBTTask_ZombieKeepDistance::GetStaticDescription() const
{
	return FString::Printf(TEXT("Back away from %s to preferred range"), *BlackboardKey.SelectedKeyName.ToString());
}
