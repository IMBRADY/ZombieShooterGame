#include "PerkDataAsset.h"

const FPrimaryAssetType UPerkDataAsset::AssetType = TEXT("Perk");

FPrimaryAssetId UPerkDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(AssetType, GetFName());
}

int32 UPerkDataAsset::GetCostForNextTier(int32 CurrentTier) const
{
	return FMath::RoundToInt(BaseCost * FMath::Pow(FMath::Max(CostGrowth, 1.0f), static_cast<float>(FMath::Max(CurrentTier, 0))));
}

FText UPerkDataAsset::DescribeTier(int32 Tier) const
{
	// The description's {0} is the first modifier's (or effect's) total at this tier, formatted as
	// a percentage for multipliers and fractions, and as a plain number for counts.
	float Value = 0.0f;
	bool bPercent = true;

	if (ModifiersPerTier.Num() > 0)
	{
		const FStatModifier& First = ModifiersPerTier[0];
		Value = First.Value * Tier;
		bPercent = First.Op == EStatModifierOp::Multiply || FMath::Abs(First.Value) < 1.0f;
	}
	else if (HitEffectsPerTier.Num() > 0)
	{
		Value = FMath::Min(HitEffectsPerTier[0].Chance * Tier, 1.0f);
	}

	const FString Formatted = bPercent
		? FString::Printf(TEXT("%d%%"), FMath::RoundToInt(FMath::Abs(Value) * 100.0f))
		: FString::Printf(TEXT("%d"), FMath::RoundToInt(FMath::Abs(Value)));

	return FText::Format(Description, FText::FromString(Formatted));
}
