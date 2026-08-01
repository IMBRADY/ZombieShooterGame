#include "SpawnDirectorSettings.h"

const FPrimaryAssetType USpawnDirectorSettings::AssetType = TEXT("SpawnDirectorSettings");

USpawnDirectorSettings::USpawnDirectorSettings()
{
	// Sector 1 is almost entirely the common shambler; medium-class zombies appear as a garnish
	// and high-class ones effectively don't until the growth below has had a few sectors to run.
	BaseTierWeights.Add(EZombieClassTier::Low, 1.0f);
	BaseTierWeights.Add(EZombieClassTier::Medium, 0.25f);
	BaseTierWeights.Add(EZombieClassTier::High, 0.05f);
	BaseTierWeights.Add(EZombieClassTier::Boss, 0.0f);

	TierWeightIncreasePerSector.Add(EZombieClassTier::Low, 0.0f);
	TierWeightIncreasePerSector.Add(EZombieClassTier::Medium, 0.12f);
	TierWeightIncreasePerSector.Add(EZombieClassTier::High, 0.08f);

	// Bosses are placed by the boss system every 5 sectors, not drawn from the spawn budget.
	TierWeightIncreasePerSector.Add(EZombieClassTier::Boss, 0.0f);
}

FPrimaryAssetId USpawnDirectorSettings::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(AssetType, GetFName());
}

int32 USpawnDirectorSettings::GetBudgetForSector(int32 Sector) const
{
	const int32 SectorsCleared = FMath::Max(Sector - 1, 0);
	const float Budget = BaseBudget * FMath::Pow(FMath::Max(BudgetGrowthPerSector, 1.0f), static_cast<float>(SectorsCleared));

	return FMath::Max(FMath::RoundToInt(Budget), 1);
}

FZombieDifficultyScaling USpawnDirectorSettings::GetScalingForSector(int32 Sector) const
{
	const float SectorsCleared = static_cast<float>(FMath::Max(Sector - 1, 0));

	FZombieDifficultyScaling Scaling;
	Scaling.HealthMultiplier = 1.0f + SectorsCleared * HealthIncreasePerSector;
	Scaling.DamageMultiplier = 1.0f + SectorsCleared * DamageIncreasePerSector;
	Scaling.RewardMultiplier = 1.0f + SectorsCleared * RewardIncreasePerSector;

	return Scaling;
}

float USpawnDirectorSettings::GetTierWeight(EZombieClassTier Tier, int32 Sector) const
{
	const float SectorsCleared = static_cast<float>(FMath::Max(Sector - 1, 0));
	const float BaseWeight = BaseTierWeights.FindRef(Tier);
	const float Growth = TierWeightIncreasePerSector.FindRef(Tier);

	return FMath::Max(BaseWeight + SectorsCleared * Growth, 0.0f);
}
