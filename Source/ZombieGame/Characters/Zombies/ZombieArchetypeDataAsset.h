#pragma once

#include "CoreMinimal.h"
#include "Audio/ZombieAudioTypes.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Visual/ZombieEffectTypes.h"
#include "ZombieArchetypeDataAsset.generated.h"

class AZombieCharacter;
class UBehaviorTree;
class ULootTableDataAsset;
class USpriteSheetDataAsset;
class UZombieAbility;

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

/** Which shared Behavior Tree shape an archetype runs. */
UENUM(BlueprintType)
enum class EZombieBehaviorProfile : uint8
{
	/** Close the distance and swing (Common, Runner, Tank, Exploder, Poison, Armored). */
	Melee		UMETA(DisplayName = "Melee"),
	/** Hang back at a preferred range and use abilities; retreat when crowded (Lobber, Necromancer). */
	Ranged		UMETA(DisplayName = "Ranged / Support"),
	/** Always hunting, phase-driven ability use - the separate boss tree. */
	Boss		UMETA(DisplayName = "Boss")
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
 * A new enemy is a new Data Asset: stats, spawn economy, perception, resistances, abilities and
 * art. Only a genuinely new *ability* needs C++ - one UZombieAbility subclass - never a new
 * character class (ARCHITECTURE.md 5 and 6).
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

	/** Fraction of damage ignored per damage tag - the Armored zombie's "reduced bullet damage". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (Categories = "Damage"))
	TMap<FGameplayTag, float> DamageResistances;

	// --- Behaviour ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behaviour")
	EZombieBehaviorProfile BehaviorProfile = EZombieBehaviorProfile::Melee;

	/** Ranged/support profiles try to hold this distance from their target. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behaviour", meta = (ClampMin = "0.0"))
	float PreferredRange = 0.0f;

	/** Special behaviours (throwing, exploding, reviving, boss attacks) as instanced, data-authored abilities. */
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Behaviour")
	TArray<TObjectPtr<UZombieAbility>> Abilities;

	/** Health fractions at which a boss enters its next phase, highest first (e.g. 0.66, 0.33). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behaviour")
	TArray<float> PhaseHealthThresholds;

	/** Speed multiplier applied in each phase after the first. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behaviour", meta = (ClampMin = "0.1"))
	float PhaseSpeedMultiplier = 1.2f;

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

	// --- Rewards ---

	/** Extra drops beyond money - rare health, ammo, the occasional weapon. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rewards")
	TObjectPtr<ULootTableDataAsset> LootTable;

	// --- Presentation ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<USpriteSheetDataAsset> SpriteSheet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FLinearColor Tint = FLinearColor::White;

	/** Scales the capsule (hitbox) and the sprite together. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual", meta = (ClampMin = "0.1"))
	float BodyScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FZombieEffectSpec HitEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FZombieEffectSpec DeathEffect;

	/** Blood pool left on the floor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	FZombieEffectSpec CorpseDecal;

	// --- Audio ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	FZombieSoundSpec IdleSound;

	/** Played when it first spots a player - the zombie scream. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	FZombieSoundSpec AlertSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	FZombieSoundSpec AttackSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	FZombieSoundSpec HurtSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	FZombieSoundSpec DeathSound;

	// --- AI ---

	/**
	 * Optional editor-authored Behavior Tree. When unset the archetype uses the shared tree for its
	 * profile, assembled in C++ by UZombieAIAssetSubsystem; assigning an asset overrides it.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeOverride;
};
