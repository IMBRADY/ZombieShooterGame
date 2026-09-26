#include "ZombieAudioSettings.h"
#include "ZombieGame.h"

const FPrimaryAssetType UZombieAudioSettings::AssetType = TEXT("AudioSettings");
const TCHAR* UZombieAudioSettings::DefaultAssetPath = TEXT("/Game/DataAssets/Audio/DA_Audio.DA_Audio");

FPrimaryAssetId UZombieAudioSettings::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(AssetType, GetFName());
}

const UZombieAudioSettings* UZombieAudioSettings::GetOrLoadDefault()
{
	static TWeakObjectPtr<const UZombieAudioSettings> Cached;
	if (Cached.IsValid())
	{
		return Cached.Get();
	}

	if (const UZombieAudioSettings* Loaded = Cast<UZombieAudioSettings>(FSoftObjectPath(DefaultAssetPath).TryLoad()))
	{
		Cached = Loaded;
		return Loaded;
	}

	UE_LOG(LogZombieGame, Warning, TEXT("Audio settings '%s' missing; the game will be silent."), DefaultAssetPath);
	return GetDefault<UZombieAudioSettings>();
}
