#pragma once

#include "CoreMinimal.h"
#include "Core/ZombieStatTypes.h"
#include "Weapons/WeaponTypes.h"

class IZombieStatSource;
class UWeaponDataAsset;
class UWeaponRaritySettings;

/**
 * Turns a weapon's base stats into the stats it actually fires with:
 *
 *     base (Data Asset) -> rarity tier -> one-time upgrade -> owner's perks
 *
 * Plain C++ with no world or actor dependency, so the damage/crit/perk-stacking maths the spec
 * wants covered by Automation Tests (ARCHITECTURE.md 17) can be tested directly.
 */
class ZOMBIEGAME_API FWeaponStatsCalculator
{
public:
	static FWeaponStats Compute(const UWeaponDataAsset& Definition, EWeaponRarity Rarity, int32 UpgradeLevel,
		const UWeaponRaritySettings& RaritySettings, const IZombieStatSource* Modifiers);

	/** Status effects the weapon's rounds carry: its own, its Legendary extras, and perk-granted ones. */
	static void GatherHitEffects(const UWeaponDataAsset& Definition, EWeaponRarity Rarity,
		const IZombieStatSource* Modifiers, TArray<FHitEffectSpec>& OutEffects);

	/** Shop price of a weapon at a rarity. */
	static int32 GetPurchasePrice(const UWeaponDataAsset& Definition, EWeaponRarity Rarity, const UWeaponRaritySettings& RaritySettings);

	/** Price of the one-time upgrade - "fairly expensive", scaling with rarity. */
	static int32 GetUpgradePrice(const UWeaponDataAsset& Definition, EWeaponRarity Rarity, const UWeaponRaritySettings& RaritySettings);

	/** What the shop pays for the weapon, counting any upgrade already bought for it. */
	static int32 GetSellPrice(const UWeaponDataAsset& Definition, EWeaponRarity Rarity, int32 UpgradeLevel, const UWeaponRaritySettings& RaritySettings);
};
