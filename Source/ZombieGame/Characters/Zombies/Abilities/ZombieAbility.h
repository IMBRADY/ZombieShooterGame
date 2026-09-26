#pragma once

#include "CoreMinimal.h"
#include "Audio/ZombieAudioTypes.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "Visual/ZombieEffectTypes.h"
#include "ZombieAbility.generated.h"

class AZombieCharacter;

/**
 * Something a special zombie can do beyond walking up and swinging - throw acid, explode, leave
 * poison, raise the dead, charge, slam, summon. "Special zombies add unique states": each is one of
 * these, authored inline on the archetype Data Asset with its own numbers.
 *
 * Active abilities are chosen by the Behavior Tree (a service marks one ready, a task fires it);
 * passive ones hook the owner's spawn and death. The archetype holds templates; every zombie gets
 * its own duplicated instances, so per-zombie state (a charge in flight) lives here safely.
 */
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, CollapseCategories)
class ZOMBIEGAME_API UZombieAbility : public UObject
{
	GENERATED_BODY()

public:
	/** False for purely passive abilities (poison trail, death explosion) the tree never fires. */
	virtual bool IsActivatable() const { return true; }

	/** Range, line of sight, boss phase and ability-specific checks. */
	bool CanActivate(const AZombieCharacter& Owner, const AActor* Target) const;

	/** Fires the ability. Returns how long the zombie is committed to it. */
	float Activate(AZombieCharacter& Owner, AActor* Target);

	virtual void OnOwnerSpawned(AZombieCharacter& Owner) {}
	virtual void OnOwnerDied(AZombieCharacter& Owner) {}

	float GetCooldown() const { return Cooldown; }
	float GetInitialDelay() const { return InitialDelay; }
	const FGameplayTag& GetAbilityTag() const { return AbilityTag; }

protected:
	virtual bool CanActivateInternal(const AZombieCharacter& Owner, const AActor* Target) const { return true; }
	virtual void ActivateInternal(AZombieCharacter& Owner, AActor* Target) PURE_VIRTUAL(UZombieAbility::ActivateInternal, );

	/** Scales a base damage number by the owner's current difficulty multiplier. */
	static float ScaleDamage(const AZombieCharacter& Owner, float BaseDamage);

	UPROPERTY(EditAnywhere, Category = "Ability", meta = (Categories = "Ability"))
	FGameplayTag AbilityTag;

	UPROPERTY(EditAnywhere, Category = "Ability", meta = (ClampMin = "0.0"))
	float Cooldown = 6.0f;

	/** Grace period after spawning before the first use. */
	UPROPERTY(EditAnywhere, Category = "Ability", meta = (ClampMin = "0.0"))
	float InitialDelay = 2.0f;

	UPROPERTY(EditAnywhere, Category = "Ability", meta = (ClampMin = "0.0"))
	float MinRange = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Ability", meta = (ClampMin = "0.0"))
	float MaxRange = 1200.0f;

	/** Bosses only: the phase from which this ability is unlocked (0 = from the start). */
	UPROPERTY(EditAnywhere, Category = "Ability", meta = (ClampMin = "0"))
	int32 MinPhase = 0;

	UPROPERTY(EditAnywhere, Category = "Ability")
	bool bRequiresTarget = true;

	UPROPERTY(EditAnywhere, Category = "Ability")
	bool bRequiresLineOfSight = true;

	/** How long the zombie stands committed to the ability (wind-up + recovery). */
	UPROPERTY(EditAnywhere, Category = "Ability", meta = (ClampMin = "0.0"))
	float BusyDuration = 0.8f;

	/** Sprite animation played while using it. */
	UPROPERTY(EditAnywhere, Category = "Presentation")
	FName Animation = TEXT("Attack");

	UPROPERTY(EditAnywhere, Category = "Presentation")
	FZombieSoundSpec Sound;

	UPROPERTY(EditAnywhere, Category = "Presentation")
	FZombieEffectSpec CastEffect;
};
