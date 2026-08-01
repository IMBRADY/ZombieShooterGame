#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"

/**
 * Discovers content by Primary Asset Type through the Asset Manager instead of hardcoded lists.
 *
 * This is what makes "add a room / a zombie / a perk without touching code" literally true: a new
 * Data Asset dropped into the scanned content directory is picked up on the next run, with no
 * registration step and no switch statement (ARCHITECTURE.md 5). Which directories are scanned is
 * configured in DefaultGame.ini under [/Script/Engine.AssetManagerSettings].
 */
class ZOMBIEGAME_API FZombiePrimaryAssetLoader
{
public:
	/**
	 * Loads every registered asset of the given Primary Asset Type.
	 *
	 * Synchronous by design: these are small metadata assets resolved once at sector setup, not
	 * the meshes/audio they point at - those stay behind soft references and load asynchronously.
	 */
	template <typename TAssetClass>
	static void LoadAllOfType(FPrimaryAssetType AssetType, TArray<TAssetClass*>& OutAssets)
	{
		TArray<UObject*> LoadedObjects;
		LoadAllOfType(AssetType, LoadedObjects);

		OutAssets.Reserve(OutAssets.Num() + LoadedObjects.Num());
		for (UObject* LoadedObject : LoadedObjects)
		{
			if (TAssetClass* TypedAsset = Cast<TAssetClass>(LoadedObject))
			{
				OutAssets.Add(TypedAsset);
			}
		}
	}

	static void LoadAllOfType(FPrimaryAssetType AssetType, TArray<UObject*>& OutAssets);
};
