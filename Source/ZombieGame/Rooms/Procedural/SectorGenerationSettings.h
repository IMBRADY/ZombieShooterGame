#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SectorGenerationSettings.generated.h"

/**
 * Designer-facing tuning for how a sector is assembled from handcrafted modules. Lives in a Data
 * Asset so pacing can be retuned without recompiling (prompt.txt "Data Driven Design").
 *
 * Which modules are available is not listed here - room templates are discovered through the
 * Asset Manager (see FZombiePrimaryAssetLoader), so adding a room is purely adding an asset.
 */
UCLASS(BlueprintType)
class ZOMBIEGAME_API USectorGenerationSettings : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	static const FPrimaryAssetType AssetType;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/** World size of one layout tile. Also the width of a one-tile corridor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sector", meta = (ClampMin = "50.0"))
	float TileSize = 250.0f;

	/** Room count in sector 1. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sector", meta = (ClampMin = "2"))
	int32 BaseRoomCount = 7;

	/** Additional rooms per sector cleared - "more generated rooms" as sectors go on. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sector", meta = (ClampMin = "0.0"))
	float ExtraRoomsPerSector = 0.75f;

	/** Hard ceiling so late sectors stay inside the navigable play area and memory budget. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sector", meta = (ClampMin = "2"))
	int32 MaxRoomCount = 16;

	/** Minimum combat arenas per sector, so a sector is never all corridors. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sector", meta = (ClampMin = "0"))
	int32 MinCombatRooms = 3;

	int32 GetRoomCountForSector(int32 Sector) const;
};
