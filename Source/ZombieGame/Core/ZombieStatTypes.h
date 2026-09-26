#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ZombieStatTypes.generated.h"

UENUM(BlueprintType)
enum class EStatModifierOp : uint8
{
	/** Added to the base value before any multiplier. */
	Add			UMETA(DisplayName = "Add"),

	/** Fractional change: 0.15 means +15%. Multipliers from every source add together. */
	Multiply	UMETA(DisplayName = "Multiply (fraction)")
};

/** One stat change, identified by tag - "+15% Stat.Move.Speed", "+1 Stat.Weapon.Pierce". */
USTRUCT(BlueprintType)
struct FStatModifier
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stat", meta = (Categories = "Stat"))
	FGameplayTag Stat;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stat")
	EStatModifierOp Op = EStatModifierOp::Multiply;

	/** Per tier for perks; a flat value everywhere else. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stat")
	float Value = 0.0f;
};

/**
 * Everything a set of modifiers says about one stat, reduced to two numbers so any consumer can
 * apply it without knowing where the modifiers came from.
 */
struct FStatModifierTotals
{
	float Additive = 0.0f;
	float MultiplierFraction = 0.0f;

	/** (Base + Additive) * (1 + MultiplierFraction), never negative. */
	float Apply(float BaseValue) const
	{
		return FMath::Max((BaseValue + Additive) * (1.0f + MultiplierFraction), 0.0f);
	}

	void Accumulate(const FStatModifier& Modifier, float Scale)
	{
		if (Modifier.Op == EStatModifierOp::Add)
		{
			Additive += Modifier.Value * Scale;
		}
		else
		{
			MultiplierFraction += Modifier.Value * Scale;
		}
	}
};

/**
 * A status effect some damage can apply on hit - "25% chance to set the target Burning". The
 * effect itself (damage per second, duration, tint) lives in its own Status Effect Data Asset,
 * found by tag, so a new damage-over-time type is content rather than code.
 */
USTRUCT(BlueprintType)
struct FHitEffectSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Effect", meta = (Categories = "Status"))
	FGameplayTag StatusTag;

	/** 0..1 probability per hit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Effect", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Chance = 1.0f;

	/** Scales the status effect's damage per second. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hit Effect", meta = (ClampMin = "0.0"))
	float Potency = 1.0f;
};
