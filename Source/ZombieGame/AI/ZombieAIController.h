#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "ZombieAIController.generated.h"

class UAISenseConfig_Hearing;
class UAISenseConfig_Sight;
class UZombieArchetypeDataAsset;
struct FAIStimulus;

/**
 * Drives one zombie: owns its perception, translates what it senses into Blackboard state, and
 * runs the Behavior Tree that decides what to do about it.
 *
 * The controller never decides behaviour itself - it only reports facts ("I can see this actor",
 * "I heard something over there"). Every decision lives in the tree, which is what keeps new
 * behaviours additive (a new BT task) instead of another branch in a growing controller.
 */
UCLASS()
class ZOMBIEGAME_API AZombieAIController : public AAIController
{
	GENERATED_BODY()

public:
	AZombieAIController();

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	/** Drops the current target and stops the tree - used when the pawn dies. */
	void AbandonCombat();

	/**
	 * Makes the zombie hunt Hostile right now, whether or not it can see them - it was shot, heard
	 * a gunshot nearby, or the player walked up behind it. The zombie keeps hunting for at least
	 * AggravatedMemorySeconds even out of sight; every further call restarts that clock.
	 */
	void Aggravate(AActor* Hostile);

	/** True for actors a zombie should hunt: player-controlled pawns, never other zombies. */
	static bool IsHostileTarget(const AActor* Actor);

	/** The tag weapons put on their noise events, so a gunshot can be told apart from other noise. */
	static const FName GunshotNoiseTag;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Perception")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY(VisibleAnywhere, Category = "Perception")
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

private:
	/** Pushes the possessed zombie's archetype ranges into the sense configs. */
	void ApplyArchetypePerception(const UZombieArchetypeDataAsset* Archetype);

	void HandleSightUpdate(AActor* Actor, const FAIStimulus& Stimulus);
	void HandleHearingUpdate(AActor* Actor, const FAIStimulus& Stimulus);

	/** Whether sight currently reports Actor - hearing and aggravation never count as seeing. */
	bool CanSee(const AActor& Actor) const;

	/** (Re)starts the countdown after which an unseen target is dropped. */
	void StartForgetTimer(float Seconds);
	void ForgetTarget();

	FTimerHandle ForgetTargetTimer;
	float MemorySeconds = 6.0f;
};
