#include "ZombieAISettings.h"
#include "ZombieGame.h"

const FPrimaryAssetType UZombieAISettings::AssetType = TEXT("ZombieAISettings");
const TCHAR* UZombieAISettings::DefaultAssetPath = TEXT("/Game/DataAssets/Zombies/DA_ZombieAI.DA_ZombieAI");

FPrimaryAssetId UZombieAISettings::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(AssetType, GetFName());
}

const UZombieAISettings* UZombieAISettings::GetOrLoadDefault()
{
	static TWeakObjectPtr<const UZombieAISettings> CachedSettings;
	if (CachedSettings.IsValid())
	{
		return CachedSettings.Get();
	}

	TSoftObjectPtr<UZombieAISettings> SettingsAsset{ FSoftObjectPath(DefaultAssetPath) };
	if (const UZombieAISettings* Loaded = SettingsAsset.LoadSynchronous())
	{
		CachedSettings = Loaded;
		return Loaded;
	}

	UE_LOG(LogZombieGame, Warning, TEXT("Zombie AI settings asset '%s' is missing; using class defaults."), DefaultAssetPath);
	return GetDefault<UZombieAISettings>();
}
