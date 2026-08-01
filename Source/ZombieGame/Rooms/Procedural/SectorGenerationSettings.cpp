#include "SectorGenerationSettings.h"

const FPrimaryAssetType USectorGenerationSettings::AssetType = TEXT("SectorGenerationSettings");

FPrimaryAssetId USectorGenerationSettings::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(AssetType, GetFName());
}

int32 USectorGenerationSettings::GetRoomCountForSector(int32 Sector) const
{
	const int32 SectorsCleared = FMath::Max(Sector - 1, 0);
	const int32 Requested = BaseRoomCount + FMath::FloorToInt(SectorsCleared * ExtraRoomsPerSector);

	return FMath::Clamp(Requested, 2, FMath::Max(MaxRoomCount, 2));
}
