#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

/**
 * The project's Gameplay Tag vocabulary, declared natively so a renamed tag is a compile error
 * rather than a silently dead Data Asset entry.
 *
 * Tags are what keep perks, weapons and status effects data-driven (ARCHITECTURE.md 5): a Data
 * Asset names the stat it modifies or the effect it applies by tag, and the systems that consume
 * them look the tag up - there is no switch over "which perk is this".
 */
namespace ZombieTags
{
	// --- Stats a perk (or any other modifier source) can change ---

	ZOMBIEGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stat_Move_Speed);
	ZOMBIEGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stat_Stamina_DrainRate);
	ZOMBIEGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stat_Health_Max);
	ZOMBIEGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stat_Weapon_Damage);
	ZOMBIEGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stat_Weapon_FireRate);
	ZOMBIEGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stat_Weapon_ReloadSpeed);
	ZOMBIEGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stat_Weapon_MagazineSize);
	ZOMBIEGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stat_Weapon_CritChance);
	ZOMBIEGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stat_Weapon_Pierce);
	ZOMBIEGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stat_Weapon_Ricochet);
	ZOMBIEGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stat_Hit_LifeSteal);
	ZOMBIEGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stat_Economy_MoneyMultiplier);
	ZOMBIEGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stat_Pickup_Radius);
	ZOMBIEGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stat_Resist_Explosion);
	ZOMBIEGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Stat_Resist_Poison);

	// --- Damage categories (resistances and statistics key off these) ---

	ZOMBIEGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Bullet);
	ZOMBIEGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Explosion);
	ZOMBIEGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Fire);
	ZOMBIEGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Poison);
	ZOMBIEGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Melee);

	// --- Status effects applied on hit (resolved to UStatusEffectDataAsset by tag) ---

	ZOMBIEGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Burning);
	ZOMBIEGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Poisoned);

	// --- Zombie abilities (Behavior Tree picks them up by tag) ---

	ZOMBIEGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_RangedThrow);
	ZOMBIEGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Revive);
	ZOMBIEGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Charge);
	ZOMBIEGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Slam);
	ZOMBIEGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Summon);
}
