#include "Tests/ZombieTestFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/HealthComponent.h"
#include "Core/Save/ZombieRunTypes.h"
#include "Core/ZombieGameplayTags.h"
#include "Perks/PerkComponent.h"
#include "Perks/PerkDataAsset.h"
#include "Weapons/WeaponDataAsset.h"
#include "Weapons/WeaponRaritySettings.h"
#include "Weapons/WeaponStatsCalculator.h"

namespace
{
	UWeaponDataAsset* MakePistol()
	{
		UWeaponDataAsset* Weapon = NewObject<UWeaponDataAsset>();
		Weapon->BaseStats.Damage = 20.0f;
		Weapon->BaseStats.ShotsPerSecond = 4.0f;
		Weapon->BaseStats.MagazineSize = 10;
		Weapon->BaseStats.MaxReserveAmmo = 50;
		Weapon->BaseStats.ReloadTime = 2.0f;
		Weapon->BaseStats.CritChance = 0.1f;
		Weapon->BaseStats.Pierce = 0;
		Weapon->UpgradeBonus.DamageMultiplier = 1.5f;
		Weapon->UpgradeBonus.MagazineMultiplier = 1.3f;
		Weapon->UpgradeBonus.ReloadTimeMultiplier = 0.5f;
		return Weapon;
	}

	UWeaponRaritySettings* MakeRarityTable()
	{
		UWeaponRaritySettings* Rarity = NewObject<UWeaponRaritySettings>();
		FWeaponRarityTier Legendary;
		Legendary.DamageMultiplier = 2.0f;
		Legendary.FireRateMultiplier = 1.25f;
		Legendary.ReloadTimeMultiplier = 0.5f;
		Legendary.BonusCritChance = 0.1f;
		Legendary.BonusPierce = 1;
		Rarity->Tiers = { { EWeaponRarity::Common, FWeaponRarityTier() }, { EWeaponRarity::Legendary, Legendary } };
		return Rarity;
	}

	UPerkDataAsset* MakePerk(const FGameplayTag& Stat, EStatModifierOp Op, float ValuePerTier, int32 MaxTier = 5)
	{
		UPerkDataAsset* Perk = NewObject<UPerkDataAsset>();
		Perk->MaxTier = MaxTier;
		FStatModifier& Modifier = Perk->ModifiersPerTier.AddDefaulted_GetRef();
		Modifier.Stat = Stat;
		Modifier.Op = Op;
		Modifier.Value = ValuePerTier;
		return Perk;
	}

	/** A perk loadout the way a save restores it - the path that doesn't need network authority. */
	UPerkComponent* MakePerks(const TArray<TPair<UPerkDataAsset*, int32>>& Owned)
	{
		UPerkComponent* Perks = NewObject<UPerkComponent>();
		TArray<FOwnedPerkRecord> Records;
		for (const TPair<UPerkDataAsset*, int32>& Entry : Owned)
		{
			FOwnedPerkRecord& Record = Records.AddDefaulted_GetRef();
			Record.Perk = Entry.Key;
			Record.Tier = Entry.Value;
		}
		Perks->ImportRecords(Records);
		return Perks;
	}
}

// --- Weapon stat pipeline: base -> rarity -> upgrade -> perks ---------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponBaseStatsTest, "ZombieGame.Combat.Weapons.CommonUnupgradedMatchesDataAsset", ZombieTest::Flags)
bool FWeaponBaseStatsTest::RunTest(const FString& Parameters)
{
	const UWeaponDataAsset* Pistol = MakePistol();
	const FWeaponStats Stats = FWeaponStatsCalculator::Compute(*Pistol, EWeaponRarity::Common, 0, *MakeRarityTable(), nullptr);

	TestEqual(TEXT("damage"), Stats.Damage, 20.0f);
	TestEqual(TEXT("fire rate"), Stats.ShotsPerSecond, 4.0f);
	TestEqual(TEXT("magazine"), Stats.MagazineSize, 10);
	TestEqual(TEXT("reserve"), Stats.MaxReserveAmmo, 50);
	TestEqual(TEXT("reload"), Stats.ReloadTime, 2.0f);
	TestEqual(TEXT("crit"), Stats.CritChance, 0.1f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponRarityUpgradeTest, "ZombieGame.Combat.Weapons.RarityAndUpgradeStack", ZombieTest::Flags)
bool FWeaponRarityUpgradeTest::RunTest(const FString& Parameters)
{
	const UWeaponDataAsset* Pistol = MakePistol();
	const UWeaponRaritySettings* Rarity = MakeRarityTable();

	const FWeaponStats Legendary = FWeaponStatsCalculator::Compute(*Pistol, EWeaponRarity::Legendary, 0, *Rarity, nullptr);
	TestEqual(TEXT("legendary damage"), Legendary.Damage, 40.0f);
	TestEqual(TEXT("legendary fire rate"), Legendary.ShotsPerSecond, 5.0f);
	TestEqual(TEXT("legendary reload"), Legendary.ReloadTime, 1.0f);
	TestEqual(TEXT("legendary crit"), Legendary.CritChance, 0.2f);
	TestEqual(TEXT("legendary pierce"), Legendary.Pierce, 1);

	const FWeaponStats Upgraded = FWeaponStatsCalculator::Compute(*Pistol, EWeaponRarity::Legendary, 1, *Rarity, nullptr);
	TestEqual(TEXT("upgrade multiplies damage on top of rarity"), Upgraded.Damage, 60.0f);
	TestEqual(TEXT("upgrade grows the magazine (rounded up)"), Upgraded.MagazineSize, 13);
	TestEqual(TEXT("upgrade grows reserve ammo with it"), Upgraded.MaxReserveAmmo, 65);
	TestEqual(TEXT("upgrade speeds the reload"), Upgraded.ReloadTime, 0.5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponPerkModifiersTest, "ZombieGame.Combat.Weapons.PerksApplyLast", ZombieTest::Flags)
bool FWeaponPerkModifiersTest::RunTest(const FString& Parameters)
{
	const UWeaponDataAsset* Pistol = MakePistol();
	const UWeaponRaritySettings* Rarity = MakeRarityTable();

	UPerkComponent* Perks = MakePerks({
		{ MakePerk(ZombieTags::Stat_Weapon_Damage, EStatModifierOp::Multiply, 0.1f), 3 },
		{ MakePerk(ZombieTags::Stat_Weapon_ReloadSpeed, EStatModifierOp::Multiply, 0.25f), 4 },
		{ MakePerk(ZombieTags::Stat_Weapon_Pierce, EStatModifierOp::Add, 1.0f), 2 },
		{ MakePerk(ZombieTags::Stat_Weapon_CritChance, EStatModifierOp::Add, 0.5f), 5 },
	});

	const FWeaponStats Stats = FWeaponStatsCalculator::Compute(*Pistol, EWeaponRarity::Common, 0, *Rarity, Perks);
	TestEqual(TEXT("+10% damage per tier at tier 3"), Stats.Damage, 26.0f);
	// Reload Speed is a speed: +100% halves the reload time rather than doubling it.
	TestEqual(TEXT("reload speed divides reload time"), Stats.ReloadTime, 1.0f);
	TestEqual(TEXT("piercing bullets add targets"), Stats.Pierce, 2);
	TestEqual(TEXT("crit chance is capped at 100%"), Stats.CritChance, 1.0f);
	return true;
}

// --- Perk stacking --------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPerkStackingTest, "ZombieGame.Combat.Perks.StackingOrderAndTotals", ZombieTest::Flags)
bool FPerkStackingTest::RunTest(const FString& Parameters)
{
	UPerkDataAsset* FlatSpeed = MakePerk(ZombieTags::Stat_Move_Speed, EStatModifierOp::Add, 20.0f);
	UPerkDataAsset* PercentSpeed = MakePerk(ZombieTags::Stat_Move_Speed, EStatModifierOp::Multiply, 0.1f);
	UPerkDataAsset* OtherSpeed = MakePerk(ZombieTags::Stat_Move_Speed, EStatModifierOp::Multiply, 0.05f);
	UPerkDataAsset* Unrelated = MakePerk(ZombieTags::Stat_Health_Max, EStatModifierOp::Add, 25.0f);

	const UPerkComponent* Perks = MakePerks({ { FlatSpeed, 2 }, { PercentSpeed, 3 }, { OtherSpeed, 2 }, { Unrelated, 1 } });
	const FStatModifierTotals Speed = Perks->GetStatTotals(ZombieTags::Stat_Move_Speed);

	TestEqual(TEXT("flat bonuses scale with tier"), Speed.Additive, 40.0f);
	// Multipliers from every source add together (+30% and +10% = +40%), rather than compounding.
	TestEqual(TEXT("percent bonuses add across perks"), Speed.MultiplierFraction, 0.4f, KINDA_SMALL_NUMBER);
	// Flat first, then percent: (600 + 40) * 1.4.
	TestEqual(TEXT("flat is applied before percent"), Speed.Apply(600.0f), 896.0f, 0.01f);
	TestEqual(TEXT("perks for other stats don't leak"), Perks->GetStatTotals(ZombieTags::Stat_Weapon_Damage).Apply(10.0f), 10.0f);

	TestEqual(TEXT("tier is tracked per perk"), Perks->GetTier(PercentSpeed), 3);
	TestEqual(TEXT("unowned perk is tier 0"), Perks->GetTier(NewObject<UPerkDataAsset>()), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPerkTierClampTest, "ZombieGame.Combat.Perks.RestoredTiersAreClamped", ZombieTest::Flags)
bool FPerkTierClampTest::RunTest(const FString& Parameters)
{
	UPerkDataAsset* Perk = MakePerk(ZombieTags::Stat_Weapon_Damage, EStatModifierOp::Multiply, 0.1f, 3);
	const UPerkComponent* Perks = MakePerks({ { Perk, 99 } });

	TestEqual(TEXT("a corrupt save can't exceed the max tier"), Perks->GetTier(Perk), 3);
	TestTrue(TEXT("and the perk reads as maxed"), Perks->IsMaxed(Perk));

	FStatModifierTotals Negative;
	FStatModifier Penalty;
	Penalty.Op = EStatModifierOp::Multiply;
	Penalty.Value = -2.0f;
	Negative.Accumulate(Penalty, 1.0f);
	TestEqual(TEXT("a stat never goes negative"), Negative.Apply(50.0f), 0.0f);
	return true;
}

// --- Damage application ---------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FArmorAbsorbsFirstTest, "ZombieGame.Combat.Health.ArmorAbsorbsBeforeHealth", ZombieTest::Flags)
bool FArmorAbsorbsFirstTest::RunTest(const FString& Parameters)
{
	UHealthComponent* Health = NewObject<UHealthComponent>();
	Health->RestoreState(100.0f, 30.0f);

	TestEqual(TEXT("fully absorbed hit reaches no health"), Health->ApplyDamage(20.0f), 0.0f);
	TestEqual(TEXT("armor took it"), Health->GetArmor(), 10.0f);
	TestEqual(TEXT("health untouched"), Health->GetHealth(), 100.0f);

	TestEqual(TEXT("overflow passes through"), Health->ApplyDamage(25.0f), 15.0f);
	TestEqual(TEXT("armor is gone"), Health->GetArmor(), 0.0f);
	TestEqual(TEXT("health took the rest"), Health->GetHealth(), 85.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHealthDeathTest, "ZombieGame.Combat.Health.DeathAndHealingRules", ZombieTest::Flags)
bool FHealthDeathTest::RunTest(const FString& Parameters)
{
	UHealthComponent* Health = NewObject<UHealthComponent>();
	Health->RestoreState(40.0f, 0.0f);

	Health->Heal(500.0f);
	TestEqual(TEXT("healing caps at max health"), Health->GetHealth(), 100.0f);
	Health->AddArmor(500.0f);
	TestEqual(TEXT("armor caps at max armor"), Health->GetArmor(), Health->GetMaxArmor());

	TestEqual(TEXT("negative damage does nothing"), Health->ApplyDamage(-10.0f), 0.0f);
	Health->ApplyDamage(10000.0f);
	TestTrue(TEXT("lethal damage kills"), Health->IsDead());
	TestEqual(TEXT("health floors at zero"), Health->GetHealth(), 0.0f);

	Health->Heal(50.0f);
	TestEqual(TEXT("the dead can't be healed"), Health->GetHealth(), 0.0f);
	TestEqual(TEXT("the dead take no further damage"), Health->ApplyDamage(10.0f), 0.0f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
