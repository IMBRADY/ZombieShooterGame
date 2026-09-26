#include "BTTask_ZombieUseAbility.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/Zombies/Abilities/ZombieAbility.h"
#include "Characters/Zombies/Abilities/ZombieAbilityComponent.h"
#include "Characters/Zombies/ZombieCharacter.h"

UBTTask_ZombieUseAbility::UBTTask_ZombieUseAbility(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = TEXT("Use Ready Ability");
	bNotifyTick = true;
}

void UBTTask_ZombieUseAbility::Configure(FName TargetKeyName)
{
	BlackboardKey.SelectedKeyName = TargetKeyName;
}

uint16 UBTTask_ZombieUseAbility::GetInstanceMemorySize() const
{
	return sizeof(FZombieUseAbilityMemory);
}

EBTNodeResult::Type UBTTask_ZombieUseAbility::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* Controller = OwnerComp.GetAIOwner();
	AZombieCharacter* Zombie = Controller ? Cast<AZombieCharacter>(Controller->GetPawn()) : nullptr;
	UZombieAbilityComponent* Abilities = Zombie ? Zombie->GetAbilityComponent() : nullptr;
	AActor* Target = Cast<AActor>(OwnerComp.GetBlackboardComponent()->GetValueAsObject(BlackboardKey.SelectedKeyName));
	if (!Abilities || Zombie->IsDead())
	{
		return EBTNodeResult::Failed;
	}

	UZombieAbility* Ability = Abilities->FindReadyAbility(Target);
	if (!Ability)
	{
		return EBTNodeResult::Failed;
	}

	Controller->StopMovement();

	FZombieUseAbilityMemory* Memory = CastInstanceNodeMemory<FZombieUseAbilityMemory>(NodeMemory);
	Memory->RemainingBusyTime = Abilities->ActivateAbility(Ability, Target);

	return Memory->RemainingBusyTime > 0.0f ? EBTNodeResult::InProgress : EBTNodeResult::Succeeded;
}

void UBTTask_ZombieUseAbility::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FZombieUseAbilityMemory* Memory = CastInstanceNodeMemory<FZombieUseAbilityMemory>(NodeMemory);
	Memory->RemainingBusyTime -= DeltaSeconds;

	const AAIController* Controller = OwnerComp.GetAIOwner();
	const AZombieCharacter* Zombie = Controller ? Cast<AZombieCharacter>(Controller->GetPawn()) : nullptr;
	if (!Zombie || Zombie->IsDead())
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	if (Memory->RemainingBusyTime <= 0.0f)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

FString UBTTask_ZombieUseAbility::GetStaticDescription() const
{
	return FString::Printf(TEXT("Use a ready ability on %s"), *BlackboardKey.SelectedKeyName.ToString());
}
