#include "BTService_ZombieCombatState.h"
#include "AI/Blackboard/ZombieBlackboardKeys.h"
#include "AI/ZombieAIController.h"
#include "AI/ZombieAISettings.h"
#include "CollisionQueryParams.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/Zombies/Abilities/ZombieAbilityComponent.h"
#include "Characters/Zombies/ZombieArchetypeDataAsset.h"
#include "Characters/Zombies/ZombieCharacter.h"
#include "Components/HealthComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

UBTService_ZombieCombatState::UBTService_ZombieCombatState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = TEXT("Zombie Combat State");

	INIT_SERVICE_NODE_NOTIFY_FLAGS();

	Interval = 0.15f;
	RandomDeviation = 0.05f;
	bCallTickOnSearchStart = true;
}

void UBTService_ZombieCombatState::Configure(FName InTargetActorKey, FName InInAttackRangeKey, float InInterval, bool bInAlwaysHunt)
{
	TargetActorKey = InTargetActorKey;
	InAttackRangeKey = InInAttackRangeKey;
	Interval = FMath::Max(InInterval, 0.01f);
	bAlwaysHunt = bInAlwaysHunt;
}

bool UBTService_ZombieCombatState::IsTargetAlive(const AActor* Target)
{
	if (!IsValid(Target))
	{
		return false;
	}
	const UHealthComponent* Health = Target->FindComponentByClass<UHealthComponent>();
	return !Health || !Health->IsDead();
}

AActor* UBTService_ZombieCombatState::FindNearestPlayer(const AZombieCharacter& Zombie)
{
	AActor* Nearest = nullptr;
	float NearestDistanceSquared = TNumericLimits<float>::Max();

	for (FConstPlayerControllerIterator It = Zombie.GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APawn* Pawn = It->IsValid() ? It->Get()->GetPawn() : nullptr;
		if (!IsTargetAlive(Pawn))
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared2D(Pawn->GetActorLocation(), Zombie.GetActorLocation());
		if (DistanceSquared < NearestDistanceSquared)
		{
			NearestDistanceSquared = DistanceSquared;
			Nearest = Pawn;
		}
	}
	return Nearest;
}

AActor* UBTService_ZombieCombatState::FindPlayerWithinReach(const AZombieCharacter& Zombie, float Radius)
{
	AActor* Nearest = FindNearestPlayer(Zombie);
	if (!Nearest || FVector::DistSquared2D(Nearest->GetActorLocation(), Zombie.GetActorLocation()) > FMath::Square(Radius))
	{
		return nullptr;
	}

	// Only through open space: a player on the far side of a wall is not "right there".
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ZombieProximityAwareness), false);
	Params.AddIgnoredActor(&Zombie);
	Params.AddIgnoredActor(Nearest);
	const bool bBlocked = Zombie.GetWorld()->LineTraceTestByObjectType(Zombie.GetActorLocation(), Nearest->GetActorLocation(),
		FCollisionObjectQueryParams(ECC_WorldStatic), Params);
	return bBlocked ? nullptr : Nearest;
}

void UBTService_ZombieCombatState::ClearCombatKeys(UBlackboardComponent& Blackboard, FName TargetKey, FName RangeKey)
{
	Blackboard.ClearValue(TargetKey);
	Blackboard.SetValueAsBool(RangeKey, false);
	Blackboard.SetValueAsBool(ZombieBlackboardKeys::AbilityReady, false);
	Blackboard.SetValueAsBool(ZombieBlackboardKeys::TooClose, false);
	Blackboard.SetValueAsBool(ZombieBlackboardKeys::InPreferredRange, false);
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

	// A player right next to a zombie is noticed whichever way it faces. Sight alone has a cone, and
	// a zombie shambling away from you should not ignore you breathing down its neck.
	if (!IsTargetAlive(Target) && !bAlwaysHunt)
	{
		const UZombieAISettings* Settings = UZombieAISettings::GetOrLoadDefault();
		AZombieAIController* ZombieController = Cast<AZombieAIController>(OwnerComp.GetAIOwner());
		AActor* Nearby = Settings ? FindPlayerWithinReach(*Zombie, Settings->ProximityAwarenessRadius) : nullptr;
		if (Nearby && ZombieController)
		{
			ZombieController->Aggravate(Nearby);
			Target = Nearby;
		}
	}

	// A dead target is not a target: without this the whole horde would keep swinging at a corpse.
	if (!IsTargetAlive(Target))
	{
		Target = bAlwaysHunt ? FindNearestPlayer(*Zombie) : nullptr;
		if (!Target)
		{
			ClearCombatKeys(*Blackboard, TargetActorKey, InAttackRangeKey);
			return;
		}
		Blackboard->SetValueAsObject(TargetActorKey, Target);
	}

	// Planar distance, matching the chase task's arrival test. A 3D distance here disagreed with it
	// whenever the two capsules' centres sat at different heights: the chase declared "arrived", the
	// attack branch said "not in range", and the zombie stood still next to the player.
	const float Distance = FVector::Dist2D(Zombie->GetActorLocation(), Target->GetActorLocation());
	Blackboard->SetValueAsBool(InAttackRangeKey, Distance <= Zombie->GetAttackRange());

	const UZombieArchetypeDataAsset* Archetype = Zombie->GetArchetype();
	const float PreferredRange = Archetype ? Archetype->PreferredRange : 0.0f;
	const bool bRanged = Archetype && Archetype->BehaviorProfile == EZombieBehaviorProfile::Ranged && PreferredRange > 0.0f;
	Blackboard->SetValueAsBool(ZombieBlackboardKeys::TooClose, bRanged && Distance < PreferredRange * 0.55f);
	Blackboard->SetValueAsBool(ZombieBlackboardKeys::InPreferredRange, bRanged && Distance <= PreferredRange);

	const UZombieAbilityComponent* Abilities = Zombie->GetAbilityComponent();
	Blackboard->SetValueAsBool(ZombieBlackboardKeys::AbilityReady, Abilities && Abilities->FindReadyAbility(Target) != nullptr);
}

FString UBTService_ZombieCombatState::GetStaticServiceDescription() const
{
	return FString::Printf(TEXT("Track %s, maintain %s, ability and range flags%s"), *TargetActorKey.ToString(), *InAttackRangeKey.ToString(),
		bAlwaysHunt ? TEXT(", always hunting") : TEXT(""));
}
