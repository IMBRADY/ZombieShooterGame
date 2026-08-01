#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ZombieCharacter.generated.h"

class AController;
class AZombieCharacter;
class UDamageComponent;
class UHealthComponent;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;
class UZombieArchetypeDataAsset;
struct FZombieDifficultyScaling;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnZombieDied, AZombieCharacter* /*Zombie*/, AController* /*Killer*/);

/**
 * The pawn every zombie uses.
 *
 * It holds no per-archetype behaviour and no per-archetype numbers: everything that makes a
 * Runner different from a Tank arrives through a UZombieArchetypeDataAsset applied at spawn, and
 * everything about *deciding what to do* lives in the Behavior Tree driven by AZombieAIController.
 * What is left here is only what a body owns - its components, its stats, and how it dies.
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
	void InitializeFromArchetype(const UZombieArchetypeDataAsset* InArchetype, const FZombieDifficultyScaling& Scaling);

	const UZombieArchetypeDataAsset* GetArchetype() const { return Archetype; }

	float GetAttackRange() const;
	float GetAttackDamage() const { return ScaledAttackDamage; }
	int32 GetMoneyReward() const;

	bool IsDead() const;

	/**
	 * Applies this zombie's melee damage to a target through Unreal's standard damage path, so it
	 * lands in the target's own DamageComponent exactly like weapon damage does.
	 */
	void PerformAttack(AActor* Target);

	/** Raised once, on the authority, after the zombie's health reaches zero. */
	FOnZombieDied OnZombieDied;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UDamageComponent> DamageComponent;

	/** Greybox stand-in until real sprite/flipbook art exists, matching the player's placeholder. */
	UPROPERTY(VisibleAnywhere, Category = "Visual")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	/** How long the corpse stays before being cleaned up. */
	UPROPERTY(EditDefaultsOnly, Category = "Zombie", meta = (ClampMin = "0.0"))
	float CorpseLifetime = 4.0f;

private:
	UFUNCTION()
	void HandleDeath();

	void ApplyArchetypeVisuals();

	UPROPERTY(Transient)
	TObjectPtr<const UZombieArchetypeDataAsset> Archetype;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BodyMaterial;

	float ScaledAttackDamage = 0.0f;
	float RewardMultiplier = 1.0f;
	bool bDeathHandled = false;
};
