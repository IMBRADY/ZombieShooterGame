#include "WeaponRaritySettings.h"
#include "ZombieGame.h"

const FPrimaryAssetType UWeaponRaritySettings::AssetType = TEXT("WeaponRaritySettings");
const TCHAR* UWeaponRaritySettings::DefaultAssetPath = TEXT("/Game/DataAssets/Weapons/DA_WeaponRarity.DA_WeaponRarity");

namespace
{
	FWeaponRarityTier MakeTier(const TCHAR* Name, const FLinearColor& Color, float Damage, float FireRate, float Reload,
		float Crit, int32 Pierce, float Price, float BaseWeight, float WeightPerSector)
	{
		FWeaponRarityTier Tier;
		Tier.DisplayName = FText::FromString(Name);
		Tier.Color = Color;
		Tier.DamageMultiplier = Damage;
		Tier.FireRateMultiplier = FireRate;
		Tier.ReloadTimeMultiplier = Reload;
		Tier.BonusCritChance = Crit;
		Tier.BonusPierce = Pierce;
		Tier.PriceMultiplier = Price;
		Tier.UpgradeCostMultiplier = Price;
		Tier.BaseRollWeight = BaseWeight;
		Tier.RollWeightPerSector = WeightPerSector;
		return Tier;
	}
}

UWeaponRaritySettings::UWeaponRaritySettings()
{
	Tiers.Add(EWeaponRarity::Common, MakeTier(TEXT("Common"), FLinearColor(0.78f, 0.78f, 0.78f), 1.0f, 1.0f, 1.0f, 0.0f, 0, 1.0f, 1.0f, -0.04f));
	Tiers.Add(EWeaponRarity::Rare, MakeTier(TEXT("Rare"), FLinearColor(0.25f, 0.55f, 1.0f), 1.2f, 1.08f, 0.92f, 0.03f, 0, 1.6f, 0.45f, 0.04f));
	Tiers.Add(EWeaponRarity::Epic, MakeTier(TEXT("Epic"), FLinearColor(0.7f, 0.3f, 1.0f), 1.45f, 1.15f, 0.82f, 0.06f, 1, 2.5f, 0.12f, 0.03f));
	Tiers.Add(EWeaponRarity::Legendary, MakeTier(TEXT("Legendary"), FLinearColor(1.0f, 0.62f, 0.1f), 1.8f, 1.25f, 0.7f, 0.1f, 1, 4.0f, 0.02f, 0.01f));
}

FPrimaryAssetId UWeaponRaritySettings::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(AssetType, GetFName());
}

const UWeaponRaritySettings* UWeaponRaritySettings::GetOrLoadDefault()
{
	static TWeakObjectPtr<const UWeaponRaritySettings> Cached;
	if (Cached.IsValid())
	{
		return Cached.Get();
	}

	if (const UWeaponRaritySettings* Loaded = Cast<UWeaponRaritySettings>(FSoftObjectPath(DefaultAssetPath).TryLoad()))
	{
		Cached = Loaded;
		return Loaded;
	}

	UE_LOG(LogZombieGame, Warning, TEXT("Weapon rarity settings '%s' missing; using class defaults."), DefaultAssetPath);
	return GetDefault<UWeaponRaritySettings>();
}

FWeaponRarityTier UWeaponRaritySettings::GetTier(EWeaponRarity Rarity) const
{
	if (const FWeaponRarityTier* Tier = Tiers.Find(Rarity))
	{
		return *Tier;
	}
	return FWeaponRarityTier();
}

EWeaponRarity UWeaponRaritySettings::RollRarity(int32 Sector, FRandomStream& Random, EWeaponRarity MinimumRarity) const
{
	const float SectorsCleared = static_cast<float>(FMath::Max(Sector - 1, 0));

	TArray<TPair<EWeaponRarity, float>> Weighted;
	float Total = 0.0f;
	for (const TPair<EWeaponRarity, FWeaponRarityTier>& Entry : Tiers)
	{
		if (Entry.Key < MinimumRarity)
		{
			continue;
		}

		const float Weight = FMath::Max(Entry.Value.BaseRollWeight + Entry.Value.RollWeightPerSector * SectorsCleared, 0.0f);
		if (Weight > 0.0f)
		{
			Weighted.Emplace(Entry.Key, Weight);
			Total += Weight;
		}
	}

	if (Weighted.Num() == 0)
	{
		return MinimumRarity;
	}

	// Stable order regardless of map iteration, so a seeded roll is reproducible.
	Weighted.Sort([](const TPair<EWeaponRarity, float>& Lhs, const TPair<EWeaponRarity, float>& Rhs) { return Lhs.Key < Rhs.Key; });

	float Roll = Random.FRandRange(0.0f, Total);
	for (const TPair<EWeaponRarity, float>& Entry : Weighted)
	{
		Roll -= Entry.Value;
		if (Roll <= 0.0f)
		{
			return Entry.Key;
		}
	}
	return Weighted.Last().Key;
}
