#include "RoomThemeDataAsset.h"

const FPrimaryAssetType URoomThemeDataAsset::AssetType = TEXT("RoomTheme");

FPrimaryAssetId URoomThemeDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(AssetType, GetFName());
}
