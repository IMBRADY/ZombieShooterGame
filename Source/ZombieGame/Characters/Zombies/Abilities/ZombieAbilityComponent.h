#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZombieAbilityComponent.generated.h"

class AZombieCharacter;
class UZombieAbility;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnZombiePhaseChanged, int32 /*NewPhase*/);

/**
 * Runs one zombie's abilities: owns its private copies of the archetype's ability templates,
 * tracks their cooldowns, answers "is anything ready against this target" for the Behavior Tree,
 * and drives boss phases from health thresholds.
 */
UCLASS(ClassGroup = (Custom))
class ZOMBIEGAME_API UZombieAbilityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UZombieAbilityComponent();

	/** Instantiates the archetype's abilities and phase thresholds for this zombie. */
	void Initialize(const TArray<UZombieAbility*>& Templates, const TArray<float>& InPhaseThresholds);

	/** Starts passive behaviour; call once the owner is fully spawned. */
	void NotifySpawned();
	void NotifyDied();

	/** The first activatable ability off cooldown that can be used on Target, or null. */
	UZombieAbility* FindReadyAbility(const AActor* Target) const;

	/** Fires the ability and starts its cooldown. Returns the busy duration. */
	float ActivateAbility(UZombieAbility* Ability, AActor* Target);

	bool HasActivatableAbilities() const;

	/** 0 until the first threshold is crossed, then 1, 2, ... */
	int32 GetPhase() const { return Phase; }

	/** Re-evaluates the phase from a health fraction; bosses call this as they take damage. */
	void UpdatePhase(float HealthFraction);

	FOnZombiePhaseChanged OnPhaseChanged;

private:
	AZombieCharacter* GetZombie() const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UZombieAbility>> Abilities;

	TArray<double> NextReadyTimes;
	TArray<float> PhaseThresholds;
	int32 Phase = 0;
};
