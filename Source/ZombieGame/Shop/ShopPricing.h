#pragma once

#include "CoreMinimal.h"
#include "Shop/ShopTypes.h"

class AZombieWeapon;
class UPerkDataAsset;
class UShopSettings;

/**
 * Every price in the shop, in one place and free of world state, so the economy maths the spec
 * wants tested (ARCHITECTURE.md 17: pricing, exponential perk tier costs) can be.
 */
class ZOMBIEGAME_API FShopPricing
{
public:
	/** Multiplier every price gets in the given sector. */
	static float GetInflation(const UShopSettings& Settings, int32 Sector);

	static int32 GetOfferPrice(const UShopSettings& Settings, const FShopOffer& Offer, int32 Sector, int32 OwnedPerkTier);
	static int32 GetServicePrice(const UShopSettings& Settings, EShopService Service, int32 Sector);
	static int32 GetSlotUpgradePrice(const UShopSettings& Settings, int32 CurrentSlots, int32 Sector);

	/** Prorated by how much ammo is missing; 0 when the gun is already full. */
	static int32 GetAmmoRefillPrice(const UShopSettings& Settings, const AZombieWeapon& Weapon, int32 Sector);
	static int32 GetUpgradePrice(const UShopSettings& Settings, const AZombieWeapon& Weapon, int32 Sector);
	static int32 GetSellPrice(const AZombieWeapon& Weapon);

private:
	static int32 Inflate(const UShopSettings& Settings, float BasePrice, int32 Sector);
};
