#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ZombieCharacter.generated.h"

class AController;
class AZombieCharacter;
class UDamageComponent;
class UDamageType;
class UHealthComponent;
class UPixelSpriteComponent;
class UStatusEffectComponent;
class UZombieAbilityComponent;
class UZombieArchetypeDataAsset;
struct FZombieDifficultyScaling;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnZombieDied, AZombieCharacter* /*Zombie*/, AController* /*Killer*/);

/**
 * The pawn every zombie uses, bosses included.
 *
 * It holds no per-archetype behaviour and no per-archetype numbers: everything that makes a
 * Runner different from a Necromancer arrives through a UZombieArchetypeDataAsset applied at spawn
 * (stats, resistances, abilities, art), and everything about *deciding what to do* lives in the
 * Behavior Tree driven by AZombieAIController. What is left here is what a body owns: its
 * components, its presentation, and how it dies.
 */
UCLASS()
class ZOMBIEGAME_API AZombieCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AZombieCharacter();

	/**
	 * Applies an archetype and the current sector's difficulty scaling. Call between
	 * SpawnActorDeferred and FinishSpawning so the pawn is never briefly alive with default stats.
	 */
	void InitializeFromArchetype(const UZombieArchetypeDataAsset* InArchetype, const FZombieDifficultyScaling& Scaling, bool bInGrantsRewards);

	const UZombieArchetypeDataAsset* GetArchetype() const { return Archetype; }
	UZombieAbilityComponent* GetAbilityComponent() const { return AbilityComponent; }

	float GetAttackRange() const;
	float GetAttackDamage() const { return ScaledAttackDamage; }
	float GetDamageMultiplier() const { return DamageMultiplier; }
	float GetRewardMultiplier() const { return RewardMultiplier; }
	int32 GetMoneyReward() const;
	bool GrantsRewards() const { return bGrantsRewards; }
	bool IsBoss() const;
	bool IsDead() const;

	/**
	 * Applies this zombie's melee damage to a target through Unreal's standard damage path, so it
	 * lands in the target's own DamageComponent exactly like weapon damage does.
	 */
	void PerformAttack(AActor* Target);

	/** Plays a one-off sprite animation (attack, cast) for Duration, then returns to walking. */
	void PlayActionAnimation(FName AnimationName, float Duration);

	/** The scream when it first spots a player. Rate-limited so a horde doesn't wall of sound. */
	void PlayAlertSound();

	/** Visual for rising from the dead (revived) or being summoned. */
	void PlayRiseEffect();

	/** Camera kick for players near an impact - explosions, slams. */
	void ShakeNearbyPlayers(const FVector& Origin, float Strength) const;

	/** Raised once, on the authority, after the zombie's health reaches zero. */
	FOnZombieDied OnZombieDied;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UHealthComponent> HealthComponent;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UDamageComponent> DamageComponent;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UStatusEffectComponent> StatusEffectComponent;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UZombieAbilityComponent> AbilityComponent;

	UPROPERTY(VisibleAnywhere, Category = "Visual")
	TObjectPtr<UPixelSpriteComponent> BodySprite;

	/** How long the corpse stays (and can be revived) before being cleaned up. */
	UPROPERTY(EditDefaultsOnly, Category = "Zombie", meta = (ClampMin = "0.0"))
	float CorpseLifetime = 12.0f;

	/** RVO avoidance, so a horde following one shared flow field spreads out instead of stacking. */
	UPROPERTY(EditDefaultsOnly, Category = "Zombie")
	bool bUseLocalAvoidance = true;

private:
	UFUNCTION()
	void HandleDeath();

	void HandleDamageReceived(float Amount, AActor* Causer, const UDamageType* DamageType);
	void HandleStatusEffectsChanged();
	void HandlePhaseChanged(int32 NewPhase);

	void ApplyArchetypeVisuals();
	void RefreshMoveSpeed();
	void UpdateLocomotionAnimation();
	void PlayIdleSound();

	UPROPERTY(Transient)
	TObjectPtr<const UZombieArchetypeDataAsset> Archetype;

	float ScaledAttackDamage = 0.0f;
	float DamageMultiplier = 1.0f;
	float RewardMultiplier = 1.0f;
	float PhaseSpeedMultiplier = 1.0f;
	double ActionAnimationEndsAt = 0.0;
	double LastHurtSoundTime = -10.0;
	bool bGrantsRewards = true;
	bool bDeathHandled = false;

	FTimerHandle AnimationTimer;
	FTimerHandle IdleSoundTimer;
};
