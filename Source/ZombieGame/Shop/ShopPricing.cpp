#include "ShopPricing.h"
#include "Perks/PerkDataAsset.h"
#include "Shop/ShopSettings.h"
#include "Weapons/WeaponDataAsset.h"
#include "Weapons/WeaponRaritySettings.h"
#include "Weapons/WeaponStatsCalculator.h"
#include "Weapons/ZombieWeapon.h"

float FShopPricing::GetInflation(const UShopSettings& Settings, int32 Sector)
{
	return 1.0f + FMath::Max(Sector - 1, 0) * Settings.PriceIncreasePerSector;
}

int32 FShopPricing::Inflate(const UShopSettings& Settings, float BasePrice, int32 Sector)
{
	return FMath::Max(FMath::RoundToInt(BasePrice * GetInflation(Settings, Sector)), 0);
}

int32 FShopPricing::GetOfferPrice(const UShopSettings& Settings, const FShopOffer& Offer, int32 Sector, int32 OwnedPerkTier)
{
	if (Offer.Type == EShopOfferType::Perk)
	{
		return Offer.Perk ? Inflate(Settings, static_cast<float>(Offer.Perk->GetCostForNextTier(OwnedPerkTier)), Sector) : 0;
	}

	const UWeaponDataAsset* Weapon = Offer.Weapon.Definition.LoadSynchronous();
	return Weapon
		? Inflate(Settings, static_cast<float>(FWeaponStatsCalculator::GetPurchasePrice(*Weapon, Offer.Weapon.Rarity, *UWeaponRaritySettings::GetOrLoadDefault())), Sector)
		: 0;
}

int32 FShopPricing::GetServicePrice(const UShopSettings& Settings, EShopService Service, int32 Sector)
{
	switch (Service)
	{
	case EShopService::Armor:		return Inflate(Settings, static_cast<float>(Settings.ArmorPrice), Sector);
	case EShopService::MedKit:		return Inflate(Settings, static_cast<float>(Settings.MedKitPrice), Sector);
	case EShopService::MysteryBox:	return Inflate(Settings, static_cast<float>(Settings.MysteryBoxPrice), Sector);
	default:						return 0;
	}
}

int32 FShopPricing::GetSlotUpgradePrice(const UShopSettings& Settings, int32 CurrentSlots, int32 Sector)
{
	const int32 ExtraSlotsOwned = FMath::Max(CurrentSlots - Settings.StartingSlots, 0);
	const float Base = Settings.SlotUpgradeBasePrice * FMath::Pow(Settings.SlotUpgradeGrowth, static_cast<float>(ExtraSlotsOwned));
	return Inflate(Settings, Base, Sector);
}

int32 FShopPricing::GetAmmoRefillPrice(const UShopSettings& Settings, const AZombieWeapon& Weapon, int32 Sector)
{
	const UWeaponDataAsset* Definition = Weapon.GetDefinition();
	const FWeaponStats& Stats = Weapon.GetStats();
	const int32 Capacity = Stats.MagazineSize + Stats.MaxReserveAmmo;
	const int32 Missing = Capacity - (Weapon.GetAmmoInMagazine() + Weapon.GetReserveAmmo());
	if (!Definition || Capacity <= 0 || Missing <= 0)
	{
		return 0;
	}

	const float MissingFraction = static_cast<float>(Missing) / static_cast<float>(Capacity);
	return FMath::Max(Inflate(Settings, Definition->AmmoRefillPrice * MissingFraction, Sector), 1);
}

int32 FShopPricing::GetUpgradePrice(const UShopSettings& Settings, const AZombieWeapon& Weapon, int32 Sector)
{
	const UWeaponDataAsset* Definition = Weapon.GetDefinition();
	return Definition
		? Inflate(Settings, static_cast<float>(FWeaponStatsCalculator::GetUpgradePrice(*Definition, Weapon.GetRarity(), *UWeaponRaritySettings::GetOrLoadDefault())), Sector)
		: 0;
}

int32 FShopPricing::GetSellPrice(const AZombieWeapon& Weapon)
{
	const UWeaponDataAsset* Definition = Weapon.GetDefinition();
	return Definition
		? FWeaponStatsCalculator::GetSellPrice(*Definition, Weapon.GetRarity(), Weapon.GetUpgradeLevel(), *UWeaponRaritySettings::GetOrLoadDefault())
		: 0;
}
