#include "StatusEffectDataAsset.h"
#include "Utilities/ZombiePrimaryAssetLoader.h"

const FPrimaryAssetType UStatusEffectDataAsset::AssetType = TEXT("StatusEffect");

FPrimaryAssetId UStatusEffectDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(AssetType, GetFName());
}

const UStatusEffectDataAsset* UStatusEffectDataAsset::FindByTag(const FGameplayTag& StatusTag)
{
	if (!StatusTag.IsValid())
	{
		return nullptr;
	}

	// A handful of tiny assets, resolved once and cached weakly so a reloaded asset is picked up.
	static TMap<FGameplayTag, TWeakObjectPtr<const UStatusEffectDataAsset>> Cache;
	if (const TWeakObjectPtr<const UStatusEffectDataAsset>* Cached = Cache.Find(StatusTag))
	{
		if (Cached->IsValid())
		{
			return Cached->Get();
		}
	}

	TArray<UStatusEffectDataAsset*> All;
	FZombiePrimaryAssetLoader::LoadAllOfType(AssetType, All);
	for (const UStatusEffectDataAsset* Asset : All)
	{
		if (Asset)
		{
			Cache.Add(Asset->StatusTag, Asset);
		}
	}

	const TWeakObjectPtr<const UStatusEffectDataAsset>* Found = Cache.Find(StatusTag);
	return Found ? Found->Get() : nullptr;
}
