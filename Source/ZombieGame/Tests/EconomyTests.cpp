#include "Tests/ZombieTestFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Core/SpawnDirectorSettings.h"
#include "Perks/PerkDataAsset.h"
#include "Shop/ShopPricing.h"
#include "Shop/ShopSettings.h"
#include "Weapons/WeaponDataAsset.h"
#include "Weapons/WeaponRaritySettings.h"
#include "Weapons/WeaponStatsCalculator.h"

// Every test builds its own transient Data Assets with known numbers, so rebalancing the shipped
// content never breaks a test - only a change to the maths does.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPerkTierCostTest, "ZombieGame.Economy.PerkTierCostsGrowExponentially", ZombieTest::Flags)
bool FPerkTierCostTest::RunTest(const FString& Parameters)
{
	UPerkDataAsset* Perk = NewObject<UPerkDataAsset>();
	Perk->BaseCost = 200;
	Perk->CostGrowth = 2.0f;

	TestEqual(TEXT("first tier is the base cost"), Perk->GetCostForNextTier(0), 200);
	TestEqual(TEXT("second tier doubles"), Perk->GetCostForNextTier(1), 400);
	TestEqual(TEXT("fourth tier"), Perk->GetCostForNextTier(3), 1600);

	// "Each tier is exponentially more expensive": the ratio between tiers is constant, not the gap.
	for (int32 Tier = 1; Tier < 5; ++Tier)
	{
		TestTrue(TEXT("each tier costs more than the last"), Perk->GetCostForNextTier(Tier) > Perk->GetCostForNextTier(Tier - 1));
	}

	// A designer typo below 1 must not make perks cheaper as they level.
	Perk->CostGrowth = 0.5f;
	TestEqual(TEXT("growth below 1 is clamped"), Perk->GetCostForNextTier(4), 200);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopInflationTest, "ZombieGame.Economy.ShopPricesInflatePerSector", ZombieTest::Flags)
bool FShopInflationTest::RunTest(const FString& Parameters)
{
	UShopSettings* Settings = NewObject<UShopSettings>();
	Settings->PriceIncreasePerSector = 0.1f;
	Settings->ArmorPrice = 100;
	Settings->MedKitPrice = 150;
	Settings->MysteryBoxPrice = 500;

	TestEqual(TEXT("no inflation in sector 1"), FShopPricing::GetInflation(*Settings, 1), 1.0f);
	TestEqual(TEXT("sector 0 is treated as sector 1"), FShopPricing::GetInflation(*Settings, 0), 1.0f);
	TestEqual(TEXT("+10% per sector cleared"), FShopPricing::GetInflation(*Settings, 6), 1.5f);

	TestEqual(TEXT("armor at sector 1"), FShopPricing::GetServicePrice(*Settings, EShopService::Armor, 1), 100);
	TestEqual(TEXT("med kit at sector 6"), FShopPricing::GetServicePrice(*Settings, EShopService::MedKit, 6), 225);
	TestEqual(TEXT("mystery box at sector 3"), FShopPricing::GetServicePrice(*Settings, EShopService::MysteryBox, 3), 600);

	// Per-weapon services are priced from the weapon, not the service table.
	TestEqual(TEXT("per-weapon services have no flat price"), FShopPricing::GetServicePrice(*Settings, EShopService::WeaponUpgrade, 1), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShopSlotPriceTest, "ZombieGame.Economy.ExtraGunSlotsGetPricier", ZombieTest::Flags)
bool FShopSlotPriceTest::RunTest(const FString& Parameters)
{
	UShopSettings* Settings = NewObject<UShopSettings>();
	Settings->PriceIncreasePerSector = 0.0f;
	Settings->StartingSlots = 3;
	Settings->SlotUpgradeBasePrice = 900;
	Settings->SlotUpgradeGrowth = 2.0f;

	TestEqual(TEXT("first extra slot"), FShopPricing::GetSlotUpgradePrice(*Settings, 3, 1), 900);
	TestEqual(TEXT("second extra slot"), FShopPricing::GetSlotUpgradePrice(*Settings, 4, 1), 1800);
	TestEqual(TEXT("third extra slot"), FShopPricing::GetSlotUpgradePrice(*Settings, 5, 1), 3600);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponPricesTest, "ZombieGame.Economy.WeaponPricesScaleWithRarity", ZombieTest::Flags)
bool FWeaponPricesTest::RunTest(const FString& Parameters)
{
	UWeaponDataAsset* Weapon = NewObject<UWeaponDataAsset>();
	Weapon->BasePrice = 200;
	Weapon->BaseUpgradePrice = 300;

	UWeaponRaritySettings* Rarity = NewObject<UWeaponRaritySettings>();
	Rarity->SellValueFraction = 0.5f;
	FWeaponRarityTier Common;
	FWeaponRarityTier Epic;
	Epic.PriceMultiplier = 3.0f;
	Epic.UpgradeCostMultiplier = 2.0f;
	Rarity->Tiers = { { EWeaponRarity::Common, Common }, { EWeaponRarity::Epic, Epic } };

	TestEqual(TEXT("common buy price"), FWeaponStatsCalculator::GetPurchasePrice(*Weapon, EWeaponRarity::Common, *Rarity), 200);
	TestEqual(TEXT("epic buy price"), FWeaponStatsCalculator::GetPurchasePrice(*Weapon, EWeaponRarity::Epic, *Rarity), 600);
	TestEqual(TEXT("upgrade cost scales with rarity"), FWeaponStatsCalculator::GetUpgradePrice(*Weapon, EWeaponRarity::Epic, *Rarity), 600);

	// Selling returns a fraction of everything paid into the gun, including its upgrade.
	TestEqual(TEXT("sell unupgraded"), FWeaponStatsCalculator::GetSellPrice(*Weapon, EWeaponRarity::Common, 0, *Rarity), 100);
	TestEqual(TEXT("sell upgraded"), FWeaponStatsCalculator::GetSellPrice(*Weapon, EWeaponRarity::Epic, 1, *Rarity), 600);
	TestTrue(TEXT("never sells for more than it cost"),
		FWeaponStatsCalculator::GetSellPrice(*Weapon, EWeaponRarity::Epic, 0, *Rarity) < FWeaponStatsCalculator::GetPurchasePrice(*Weapon, EWeaponRarity::Epic, *Rarity));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpawnBudgetTest, "ZombieGame.Director.BudgetAndScalingGrowPerSector", ZombieTest::Flags)
bool FSpawnBudgetTest::RunTest(const FString& Parameters)
{
	USpawnDirectorSettings* Director = NewObject<USpawnDirectorSettings>();
	Director->BaseBudget = 100;
	Director->BudgetGrowthPerSector = 1.5f;
	Director->HealthIncreasePerSector = 0.2f;
	Director->DamageIncreasePerSector = 0.1f;
	Director->RewardIncreasePerSector = 0.05f;

	TestEqual(TEXT("sector 1 budget is the base"), Director->GetBudgetForSector(1), 100);
	TestEqual(TEXT("budget compounds"), Director->GetBudgetForSector(3), 225);
	for (int32 Sector = 2; Sector <= 15; ++Sector)
	{
		TestTrue(TEXT("every sector has a bigger budget"), Director->GetBudgetForSector(Sector) > Director->GetBudgetForSector(Sector - 1));
	}

	const FZombieDifficultyScaling Sector6 = Director->GetScalingForSector(6);
	TestEqual(TEXT("zombie HP scales"), Sector6.HealthMultiplier, 2.0f);
	TestEqual(TEXT("zombie damage scales"), Sector6.DamageMultiplier, 1.5f);
	TestEqual(TEXT("rewards scale"), Sector6.RewardMultiplier, 1.25f);
	TestEqual(TEXT("sector 1 is unscaled"), Director->GetScalingForSector(1).HealthMultiplier, 1.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSpawnTierMixTest, "ZombieGame.Director.ElitesBecomeMoreCommon", ZombieTest::Flags)
bool FSpawnTierMixTest::RunTest(const FString& Parameters)
{
	// Uses the constructor's defaults: the shipped starting mix.
	const USpawnDirectorSettings* Director = GetDefault<USpawnDirectorSettings>();

	auto EliteShare = [Director](int32 Sector)
	{
		const float Low = Director->GetTierWeight(EZombieClassTier::Low, Sector);
		const float Medium = Director->GetTierWeight(EZombieClassTier::Medium, Sector);
		const float High = Director->GetTierWeight(EZombieClassTier::High, Sector);
		return (Medium + High) / FMath::Max(Low + Medium + High, UE_SMALL_NUMBER);
	};

	TestTrue(TEXT("sector 1 is mostly common zombies"), EliteShare(1) < 0.5f);
	TestTrue(TEXT("elite share rises by sector 5"), EliteShare(5) > EliteShare(1));
	TestTrue(TEXT("elite share keeps rising by sector 10"), EliteShare(10) > EliteShare(5));
	TestEqual(TEXT("bosses are never drawn from the budget"), Director->GetTierWeight(EZombieClassTier::Boss, 20), 0.0f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
