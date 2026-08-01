#include "ZombieAIController.h"
#include "AI/BehaviorTrees/ZombieAIAssetSubsystem.h"
#include "AI/Blackboard/ZombieBlackboardKeys.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/Zombies/ZombieArchetypeDataAsset.h"
#include "Characters/Zombies/ZombieCharacter.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "ZombieGame.h"

AZombieAIController::AZombieAIController()
{
	PrimaryActorTick.bCanEverTick = false;

	UAIPerceptionComponent* Perception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("ZombiePerception"));

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));

	// Without a team/affiliation system everything is "neutral", so neutrals must be detectable or
	// the zombie perceives nothing at all.
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;

	Perception->ConfigureSense(*SightConfig);
	Perception->ConfigureSense(*HearingConfig);
	Perception->SetDominantSense(SightConfig->GetSenseImplementation());

	SetPerceptionComponent(*Perception);
}

void AZombieAIController::ApplyArchetypePerception(const UZombieArchetypeDataAsset* Archetype)
{
	if (!Archetype || !SightConfig || !HearingConfig)
	{
		return;
	}

	SightConfig->SightRadius = Archetype->SightRadius;
	SightConfig->LoseSightRadius = FMath::Max(Archetype->LoseSightRadius, Archetype->SightRadius);
	SightConfig->PeripheralVisionAngleDegrees = Archetype->PeripheralVisionHalfAngle;
	SightConfig->SetMaxAge(Archetype->MemorySeconds);

	HearingConfig->HearingRange = Archetype->HearingRange;
	HearingConfig->SetMaxAge(Archetype->MemorySeconds);

	MemorySeconds = Archetype->MemorySeconds;

	if (UAIPerceptionComponent* Perception = GetAIPerceptionComponent())
	{
		Perception->ConfigureSense(*SightConfig);
		Perception->ConfigureSense(*HearingConfig);
		Perception->RequestStimuliListenerUpdate();
	}
}

void AZombieAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	const AZombieCharacter* Zombie = Cast<AZombieCharacter>(InPawn);
	if (!Zombie)
	{
		UE_LOG(LogZombieGame, Error, TEXT("%s possessed a pawn that is not a zombie; AI will not run."), *GetName());
		return;
	}

	ApplyArchetypePerception(Zombie->GetArchetype());

	if (UAIPerceptionComponent* Perception = GetAIPerceptionComponent())
	{
		Perception->OnTargetPerceptionUpdated.AddDynamic(this, &AZombieAIController::HandleTargetPerceptionUpdated);
	}

	UZombieAIAssetSubsystem* AIAssets = GetGameInstance() ? GetGameInstance()->GetSubsystem<UZombieAIAssetSubsystem>() : nullptr;
	UBehaviorTree* BehaviorTree = AIAssets ? AIAssets->GetBehaviorTreeFor(Zombie->GetArchetype()) : nullptr;

	if (!BehaviorTree || !RunBehaviorTree(BehaviorTree))
	{
		UE_LOG(LogZombieGame, Error, TEXT("%s failed to start the zombie behavior tree."), *GetName());
	}
}

void AZombieAIController::OnUnPossess()
{
	if (UAIPerceptionComponent* Perception = GetAIPerceptionComponent())
	{
		Perception->OnTargetPerceptionUpdated.RemoveDynamic(this, &AZombieAIController::HandleTargetPerceptionUpdated);
	}

	AbandonCombat();

	Super::OnUnPossess();
}

void AZombieAIController::AbandonCombat()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ForgetTargetTimer);
	}

	if (UBlackboardComponent* BlackboardComponent = GetBlackboardComponent())
	{
		BlackboardComponent->ClearValue(ZombieBlackboardKeys::TargetActor);
		BlackboardComponent->ClearValue(ZombieBlackboardKeys::InvestigateLocation);
		BlackboardComponent->SetValueAsBool(ZombieBlackboardKeys::InAttackRange, false);
	}

	if (BrainComponent)
	{
		BrainComponent->StopLogic(TEXT("Zombie died"));
	}
}

bool AZombieAIController::IsHostileTarget(const AActor* Actor)
{
	const APawn* Pawn = Cast<APawn>(Actor);
	return Pawn != nullptr && Pawn->IsPlayerControlled();
}

void AZombieAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!GetBlackboardComponent())
	{
		return;
	}

	if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
	{
		HandleSightUpdate(Actor, Stimulus);
	}
	else if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>())
	{
		HandleHearingUpdate(Stimulus);
	}
}

void AZombieAIController::HandleSightUpdate(AActor* Actor, const FAIStimulus& Stimulus)
{
	UBlackboardComponent* BlackboardComponent = GetBlackboardComponent();
	if (!IsHostileTarget(Actor))
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		GetWorld()->GetTimerManager().ClearTimer(ForgetTargetTimer);
		BlackboardComponent->SetValueAsObject(ZombieBlackboardKeys::TargetActor, Actor);
		BlackboardComponent->ClearValue(ZombieBlackboardKeys::InvestigateLocation);
		return;
	}

	if (BlackboardComponent->GetValueAsObject(ZombieBlackboardKeys::TargetActor) != Actor)
	{
		return;
	}

	// Lost sight: head for where they were last seen, and only forget them once memory expires.
	BlackboardComponent->SetValueAsVector(ZombieBlackboardKeys::InvestigateLocation, Stimulus.StimulusLocation);
	GetWorld()->GetTimerManager().SetTimer(ForgetTargetTimer, this, &AZombieAIController::ForgetTarget,
		FMath::Max(MemorySeconds, 0.1f), false);
}

void AZombieAIController::HandleHearingUpdate(const FAIStimulus& Stimulus)
{
	if (!Stimulus.WasSuccessfullySensed())
	{
		return;
	}

	// "Gunshots attract zombies very well": a noise is worth investigating even mid-chase, but it
	// must never displace a target the zombie can actually see.
	UBlackboardComponent* BlackboardComponent = GetBlackboardComponent();
	if (BlackboardComponent->GetValueAsObject(ZombieBlackboardKeys::TargetActor) == nullptr)
	{
		BlackboardComponent->SetValueAsVector(ZombieBlackboardKeys::InvestigateLocation, Stimulus.StimulusLocation);
	}
}

void AZombieAIController::ForgetTarget()
{
	if (UBlackboardComponent* BlackboardComponent = GetBlackboardComponent())
	{
		BlackboardComponent->ClearValue(ZombieBlackboardKeys::TargetActor);
		BlackboardComponent->SetValueAsBool(ZombieBlackboardKeys::InAttackRange, false);
	}
}
