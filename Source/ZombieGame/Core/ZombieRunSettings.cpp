#include "ZombieRunSettings.h"
#include "ZombieGame.h"

const FPrimaryAssetType UZombieRunSettings::AssetType = TEXT("RunSettings");
const TCHAR* UZombieRunSettings::DefaultAssetPath = TEXT("/Game/DataAssets/DA_RunSettings.DA_RunSettings");

FPrimaryAssetId UZombieRunSettings::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(AssetType, GetFName());
}

const UZombieRunSettings* UZombieRunSettings::GetOrLoadDefault()
{
	static TWeakObjectPtr<const UZombieRunSettings> Cached;
	if (Cached.IsValid())
	{
		return Cached.Get();
	}

	if (const UZombieRunSettings* Loaded = Cast<UZombieRunSettings>(FSoftObjectPath(DefaultAssetPath).TryLoad()))
	{
		Cached = Loaded;
		return Loaded;
	}

	UE_LOG(LogZombieGame, Warning, TEXT("Run settings '%s' missing; using class defaults (no starting weapon)."), DefaultAssetPath);
	return GetDefault<UZombieRunSettings>();
}
