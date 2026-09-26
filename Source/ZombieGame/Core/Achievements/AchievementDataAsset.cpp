#include "AchievementDataAsset.h"

const FPrimaryAssetType UAchievementDataAsset::AssetType = TEXT("Achievement");

FPrimaryAssetId UAchievementDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(AssetType, GetFName());
}
