#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ZombieArchetypeDataAsset.generated.h"

class AZombieCharacter;
class UBehaviorTree;

/**
 * Which slice of the spawn pool an archetype belongs to. The design brief describes the director
 * choosing "what portions of total zombie pool will constitute each class", so the tier is what
 * the per-sector mix is expressed in - not the individual archetype.
 */
UENUM(BlueprintType)
enum class EZombieClassTier : uint8
{
	Low		UMETA(DisplayName = "Low class"),
	Medium	UMETA(DisplayName = "Medium class"),
	High	UMETA(DisplayName = "High class"),
	Boss	UMETA(DisplayName = "Boss")
};

/**
 * Per-sector scaling applied on top of an archetype's base numbers. The design brief scales
 * zombie HP, damage and reward every sector; keeping them together means adding another scaled
 * axis later touches one struct rather than every call site.
 */
USTRUCT(BlueprintType)
struct FZombieDifficultyScaling
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
	float HealthMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
	float DamageMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
	float RewardMultiplier = 1.0f;
};

/**
 * Everything that makes one kind of zombie different from another.
 *
 * A new enemy is a new Data Asset: stats, spawn economy, perception ranges and (optionally) a
 * different Behavior Tree. Only a genuinely new *behaviour* needs C++ - and then only a new BT
 * task, not a new character class (ARCHITECTURE.md 5 and 6).
 */
UCLASS(BlueprintType)
class ZOMBIEGAME_API UZombieArchetypeDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	static const FPrimaryAssetType AssetType;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText DisplayName;

	/** Leave unset to use the base zombie character; set it only for archetypes needing new code. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	TSoftClassPtr<AZombieCharacter> ZombieClass;

	// --- Spawn economy (Procedural Director, ARCHITECTURE.md 13) ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawning")
	EZombieClassTier Tier = EZombieClassTier::Low;

	/** What one of these costs out of the sector's difficulty budget. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawning", meta = (ClampMin = "1"))
	int32 SpawnCost = 1;

	/** Relative likelihood within its tier once the tier has been chosen. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawning", meta = (ClampMin = "0.0"))
	float SelectionWeight = 1.0f;

	/** Sector this archetype starts appearing in. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawning", meta = (ClampMin = "1"))
	int32 MinSector = 1;

	// --- Combat ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "1.0"))
	float BaseMaxHealth = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0.0"))
	float BaseAttackDamage = 12.0f;

	/** Seconds between attacks once in range. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0.1"))
	float AttackInterval = 1.4f;

	/** Wind-up before an attack lands, so the player can read it and back off. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0.0"))
	float AttackWindup = 0.45f;

	/** Distance from the target at which the attack can be thrown. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0.0"))
	float AttackRange = 160.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "1.0"))
	float MoveSpeed = 180.0f;

	/** Money dropped on death, before any difficulty reward multiplier. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0"))
	int32 MoneyReward = 10;

	// --- Perception (AI Perception component config, per archetype) ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Perception", meta = (ClampMin = "0.0"))
	float SightRadius = 1400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Perception", meta = (ClampMin = "0.0"))
	float LoseSightRadius = 1900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Perception", meta = (ClampMin = "1.0", ClampMax = "180.0"))
	float PeripheralVisionHalfAngle = 80.0f;

	/** "Gunshots attract zombies very well" - hearing range is deliberately larger than sight. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Perception", meta = (ClampMin = "0.0"))
	float HearingRange = 3000.0f;

	/** How long a lost target stays remembered before the zombie gives up and roams again. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Perception", meta = (ClampMin = "0.0"))
	float MemorySeconds = 6.0f;

	// --- Presentation (greybox until real sprite/flipbook art exists) ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FLinearColor DebugTint = FLinearColor(0.15f, 0.45f, 0.15f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (ClampMin = "0.1"))
	float BodyScale = 1.0f;

	// --- AI ---

	/**
	 * Optional editor-authored Behavior Tree. When unset the archetype uses the shared zombie tree
	 * assembled in C++ by UZombieAIAssetSubsystem, so behaviour is source-controlled with the rest
	 * of the gameplay code; assigning an asset here overrides it with no code change.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeOverride;
};
