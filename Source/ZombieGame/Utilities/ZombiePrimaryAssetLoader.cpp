#include "ZombiePrimaryAssetLoader.h"
#include "ZombieGame.h"

void FZombiePrimaryAssetLoader::LoadAllOfType(FPrimaryAssetType AssetType, TArray<UObject*>& OutAssets)
{
	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	if (!AssetManager)
	{
		UE_LOG(LogZombieGame, Error, TEXT("Asset Manager unavailable; cannot discover assets of type '%s'."), *AssetType.ToString());
		return;
	}

	TArray<FPrimaryAssetId> AssetIds;
	AssetManager->GetPrimaryAssetIdList(AssetType, AssetIds);

	if (AssetIds.Num() == 0)
	{
		UE_LOG(LogZombieGame, Warning,
			TEXT("No assets registered for Primary Asset Type '%s'. Check that the Data Assets exist and that ")
			TEXT("[/Script/Engine.AssetManagerSettings] in DefaultGame.ini scans their directory."),
			*AssetType.ToString());
		return;
	}

	for (const FPrimaryAssetId& AssetId : AssetIds)
	{
		const FSoftObjectPath AssetPath = AssetManager->GetPrimaryAssetPath(AssetId);
		if (UObject* LoadedAsset = AssetPath.TryLoad())
		{
			OutAssets.Add(LoadedAsset);
		}
		else
		{
			UE_LOG(LogZombieGame, Warning, TEXT("Failed to load primary asset '%s'."), *AssetId.ToString());
		}
	}
}
