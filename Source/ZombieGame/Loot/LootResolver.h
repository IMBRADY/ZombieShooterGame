#pragma once

#include "CoreMinimal.h"
#include "Loot/LootTypes.h"

class APawn;
class ULootTableDataAsset;
class UPerkDataAsset;
class UWeaponDataAsset;

/**
 * Rolls loot tables and hands out what they produce. One implementation behind zombie drops,
 * treasure chests, boss rewards and the Mystery Box, so "a Rare shotgun" means the same thing
 * wherever it comes from.
 */
class ZOMBIEGAME_API FLootResolver
{
public:
	/** Rolls the table's entries (RewardMultiplier scales amounts). Appends to OutLoot. */
	static void Roll(const ULootTableDataAsset& Table, int32 Sector, float RewardMultiplier, FRandomStream& Random,
		TArray<FResolvedLoot>& OutLoot);

	/** Resolves one entry into something concrete - picks the weapon, rolls its rarity, etc. */
	static bool ResolveEntry(const FLootEntry& Entry, int32 Sector, float RewardMultiplier, FRandomStream& Random,
		FResolvedLoot& OutLoot);

	/**
	 * Gives the reward to a player. Returns false when it could not be used (full health, perk
	 * already maxed) so a pickup can stay on the floor. OutDescription is for toasts.
	 */
	static bool Grant(const FResolvedLoot& Loot, APawn* Recipient, FText& OutDescription, FResolvedLoot* OutDisplaced = nullptr);

	/** Human-readable name, e.g. "Shotgun (Rare)" or "Speed Boost". */
	static FText Describe(const FResolvedLoot& Loot);

	static UWeaponDataAsset* PickRandomWeapon(int32 Sector, FRandomStream& Random, bool bShopOnly);
	static UPerkDataAsset* PickRandomPerk(FRandomStream& Random, bool bRareOnly);

private:
	static bool GrantWeapon(const FResolvedLoot& Loot, APawn& Recipient, FResolvedLoot* OutDisplaced);
	static bool GrantAmmo(int32 PercentOfReserve, APawn& Recipient);
};
