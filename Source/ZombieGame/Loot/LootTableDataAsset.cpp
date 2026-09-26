#include "LootTableDataAsset.h"

const FPrimaryAssetType ULootTableDataAsset::AssetType = TEXT("LootTable");

FPrimaryAssetId ULootTableDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(AssetType, GetFName());
}
