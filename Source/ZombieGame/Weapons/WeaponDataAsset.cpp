#include "WeaponDataAsset.h"

const FPrimaryAssetType UWeaponDataAsset::AssetType = TEXT("Weapon");

FPrimaryAssetId UWeaponDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(AssetType, GetFName());
}
