#include "ZombieArchetypeDataAsset.h"

const FPrimaryAssetType UZombieArchetypeDataAsset::AssetType = TEXT("ZombieArchetype");

FPrimaryAssetId UZombieArchetypeDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(AssetType, GetFName());
}
