#pragma once

#include "CoreMinimal.h"
#include "Weapons/WeaponTypes.h"
#include "LootTypes.generated.h"

class UPerkDataAsset;
class UWeaponDataAsset;

/** What a drop, a chest, a boss reward or the mystery box can give. */
UENUM(BlueprintType)
enum class ELootRewardType : uint8
{
	Money,
	Health,
	Armor,
	Ammo,
	Weapon,
	Perk,
	Key
};

/** One weighted line of a loot table. */
USTRUCT(BlueprintType)
struct FLootEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	ELootRewardType Type = ELootRewardType::Money;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0.0"))
	float Weight = 1.0f;

	/** Money, health, armor or ammo amount (random in range). Ignored for weapons and perks. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0"))
	int32 MinAmount = 10;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot", meta = (ClampMin = "0"))
	int32 MaxAmount = 25;

	/** Specific weapon, or none for a random eligible one. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	TObjectPtr<UWeaponDataAsset> Weapon;

	/** Weapons roll rarity normally but never below this ("small chance: Legendary weapon"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	EWeaponRarity MinimumRarity = EWeaponRarity::Common;

	/** Specific perk, or none for a random one. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	TObjectPtr<UPerkDataAsset> Perk;

	/** Random perks come only from the rare pool ("Rare perk" boss drops). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	bool bRarePerkOnly = false;

	/** Amounts scale with the sector's reward multiplier. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	bool bScaleWithRewards = true;
};

/** A rolled reward, concrete enough to be granted or put on the floor as a pickup. */
USTRUCT(BlueprintType)
struct FResolvedLoot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Loot")
	ELootRewardType Type = ELootRewardType::Money;

	UPROPERTY(BlueprintReadOnly, Category = "Loot")
	int32 Amount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Loot")
	FWeaponInstanceData Weapon;

	UPROPERTY(BlueprintReadOnly, Category = "Loot")
	TObjectPtr<UPerkDataAsset> Perk;

	/** Collected by walking over it; weapons and perks need an explicit Interact instead. */
	bool IsAutoCollected() const { return Type != ELootRewardType::Weapon && Type != ELootRewardType::Perk; }
};
