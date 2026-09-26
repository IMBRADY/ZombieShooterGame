#include "ShopSettings.h"
#include "ZombieGame.h"

const FPrimaryAssetType UShopSettings::AssetType = TEXT("ShopSettings");
const TCHAR* UShopSettings::DefaultAssetPath = TEXT("/Game/DataAssets/Shop/DA_Shop.DA_Shop");

FPrimaryAssetId UShopSettings::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(AssetType, GetFName());
}

const UShopSettings* UShopSettings::GetOrLoadDefault()
{
	static TWeakObjectPtr<const UShopSettings> Cached;
	if (Cached.IsValid())
	{
		return Cached.Get();
	}

	if (const UShopSettings* Loaded = Cast<UShopSettings>(FSoftObjectPath(DefaultAssetPath).TryLoad()))
	{
		Cached = Loaded;
		return Loaded;
	}

	UE_LOG(LogZombieGame, Warning, TEXT("Shop settings '%s' missing; using class defaults."), DefaultAssetPath);
	return GetDefault<UShopSettings>();
}
