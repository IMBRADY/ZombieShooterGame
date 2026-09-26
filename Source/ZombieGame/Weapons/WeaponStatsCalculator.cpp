#include "WeaponStatsCalculator.h"
#include "Core/ZombieGameplayTags.h"
#include "Core/ZombieStatSource.h"
#include "Weapons/WeaponDataAsset.h"
#include "Weapons/WeaponRaritySettings.h"

namespace
{
	FStatModifierTotals TotalsFor(const IZombieStatSource* Modifiers, const FGameplayTag& Stat)
	{
		return Modifiers ? Modifiers->GetStatTotals(Stat) : FStatModifierTotals();
	}

	void ApplyRarity(FWeaponStats& Stats, const FWeaponRarityTier& Tier)
	{
		Stats.Damage *= Tier.DamageMultiplier;
		Stats.ShotsPerSecond *= Tier.FireRateMultiplier;
		Stats.ReloadTime *= Tier.ReloadTimeMultiplier;
		Stats.CritChance += Tier.BonusCritChance;
		Stats.Pierce += Tier.BonusPierce;
	}

	void ApplyUpgrade(FWeaponStats& Stats, const FWeaponUpgradeBonus& Bonus)
	{
		Stats.Damage *= Bonus.DamageMultiplier;
		Stats.MagazineSize = FMath::CeilToInt(Stats.MagazineSize * Bonus.MagazineMultiplier);
		Stats.MaxReserveAmmo = FMath::CeilToInt(Stats.MaxReserveAmmo * Bonus.MagazineMultiplier);
		Stats.ReloadTime *= Bonus.ReloadTimeMultiplier;
	}

	void ApplyModifiers(FWeaponStats& Stats, const IZombieStatSource* Modifiers)
	{
		Stats.Damage = TotalsFor(Modifiers, ZombieTags::Stat_Weapon_Damage).Apply(Stats.Damage);
		Stats.ShotsPerSecond = TotalsFor(Modifiers, ZombieTags::Stat_Weapon_FireRate).Apply(Stats.ShotsPerSecond);

		// Reload Speed is a speed: +25% speed divides the time by 1.25, it does not add 25% time.
		const float ReloadSpeed = TotalsFor(Modifiers, ZombieTags::Stat_Weapon_ReloadSpeed).Apply(1.0f);
		Stats.ReloadTime /= FMath::Max(ReloadSpeed, 0.1f);

		const FStatModifierTotals Magazine = TotalsFor(Modifiers, ZombieTags::Stat_Weapon_MagazineSize);
		Stats.MagazineSize = FMath::RoundToInt(Magazine.Apply(static_cast<float>(Stats.MagazineSize)));
		Stats.MaxReserveAmmo = FMath::RoundToInt(Magazine.Apply(static_cast<float>(Stats.MaxReserveAmmo)));

		Stats.CritChance = TotalsFor(Modifiers, ZombieTags::Stat_Weapon_CritChance).Apply(Stats.CritChance);
		Stats.Pierce = FMath::RoundToInt(TotalsFor(Modifiers, ZombieTags::Stat_Weapon_Pierce).Apply(static_cast<float>(Stats.Pierce)));
		Stats.Ricochet = FMath::RoundToInt(TotalsFor(Modifiers, ZombieTags::Stat_Weapon_Ricochet).Apply(static_cast<float>(Stats.Ricochet)));
	}

	void Sanitise(FWeaponStats& Stats)
	{
		Stats.ShotsPerSecond = FMath::Max(Stats.ShotsPerSecond, 0.1f);
		Stats.MagazineSize = FMath::Max(Stats.MagazineSize, 1);
		Stats.MaxReserveAmmo = FMath::Max(Stats.MaxReserveAmmo, 0);
		Stats.ReloadTime = FMath::Max(Stats.ReloadTime, 0.05f);
		Stats.CritChance = FMath::Clamp(Stats.CritChance, 0.0f, 1.0f);
		Stats.Pierce = FMath::Max(Stats.Pierce, 0);
		Stats.Ricochet = FMath::Max(Stats.Ricochet, 0);
	}
}

FWeaponStats FWeaponStatsCalculator::Compute(const UWeaponDataAsset& Definition, EWeaponRarity Rarity, int32 UpgradeLevel,
	const UWeaponRaritySettings& RaritySettings, const IZombieStatSource* Modifiers)
{
	FWeaponStats Stats = Definition.BaseStats;

	ApplyRarity(Stats, RaritySettings.GetTier(Rarity));
	if (UpgradeLevel > 0)
	{
		ApplyUpgrade(Stats, Definition.UpgradeBonus);
	}
	ApplyModifiers(Stats, Modifiers);
	Sanitise(Stats);

	return Stats;
}

void FWeaponStatsCalculator::GatherHitEffects(const UWeaponDataAsset& Definition, EWeaponRarity Rarity,
	const IZombieStatSource* Modifiers, TArray<FHitEffectSpec>& OutEffects)
{
	OutEffects.Append(Definition.HitEffects);

	if (Rarity == EWeaponRarity::Legendary)
	{
		OutEffects.Append(Definition.LegendaryHitEffects);
	}

	if (Modifiers)
	{
		Modifiers->GetHitEffects(OutEffects);
	}
}

int32 FWeaponStatsCalculator::GetPurchasePrice(const UWeaponDataAsset& Definition, EWeaponRarity Rarity, const UWeaponRaritySettings& RaritySettings)
{
	return FMath::RoundToInt(Definition.BasePrice * RaritySettings.GetTier(Rarity).PriceMultiplier);
}

int32 FWeaponStatsCalculator::GetUpgradePrice(const UWeaponDataAsset& Definition, EWeaponRarity Rarity, const UWeaponRaritySettings& RaritySettings)
{
	return FMath::RoundToInt(Definition.BaseUpgradePrice * RaritySettings.GetTier(Rarity).UpgradeCostMultiplier);
}

int32 FWeaponStatsCalculator::GetSellPrice(const UWeaponDataAsset& Definition, EWeaponRarity Rarity, int32 UpgradeLevel, const UWeaponRaritySettings& RaritySettings)
{
	int32 Value = GetPurchasePrice(Definition, Rarity, RaritySettings);
	if (UpgradeLevel > 0)
	{
		Value += GetUpgradePrice(Definition, Rarity, RaritySettings);
	}
	return FMath::RoundToInt(Value * RaritySettings.SellValueFraction);
}
