#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "StatusEffectComponent.generated.h"

class AController;
class UStatusEffectDataAsset;

DECLARE_MULTICAST_DELEGATE(FOnStatusEffectsChanged);

USTRUCT()
struct FActiveStatusEffect
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<const UStatusEffectDataAsset> Definition;

	int32 Stacks = 1;
	float Potency = 1.0f;
	float RemainingTime = 0.0f;

	/** Who gets the kill if this is what finishes the victim off. */
	TWeakObjectPtr<AController> Instigator;
	TWeakObjectPtr<AActor> Causer;
};

/**
 * Damage-over-time conditions on whoever owns it (zombies and players alike). Ticks on a timer
 * only while something is active, applies its damage through the standard Unreal damage path so
 * resistances and kill credit work exactly as they do for bullets.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ZOMBIEGAME_API UStatusEffectComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStatusEffectComponent();

	/** Applies (or re-applies and stacks) the status identified by tag. No-op if none is authored. */
	void ApplyStatus(const FGameplayTag& StatusTag, float Potency, AController* Instigator, AActor* Causer);

	void ClearAll();

	bool HasAnyStatus() const { return ActiveEffects.Num() > 0; }

	/** Product of every active status's movement multiplier. */
	float GetMoveSpeedMultiplier() const;

	/** Tint of the most recently applied status, or white when unaffected. */
	FLinearColor GetDisplayTint() const;

	FOnStatusEffectsChanged OnStatusEffectsChanged;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Seconds between damage ticks. */
	UPROPERTY(EditDefaultsOnly, Category = "Status", meta = (ClampMin = "0.05"))
	float TickInterval = 0.5f;

private:
	void TickEffects();

	UPROPERTY(Transient)
	TArray<FActiveStatusEffect> ActiveEffects;

	FTimerHandle TickTimer;
};
