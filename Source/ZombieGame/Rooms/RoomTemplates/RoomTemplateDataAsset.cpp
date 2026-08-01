#include "RoomTemplateDataAsset.h"
#include "ZombieGame.h"

const FPrimaryAssetType URoomTemplateDataAsset::AssetType = TEXT("RoomTemplate");

FPrimaryAssetId URoomTemplateDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(AssetType, GetFName());
}

void URoomTemplateDataAsset::PostLoad()
{
	Super::PostLoad();
	bGridBuilt = false;
}

#if WITH_EDITOR
void URoomTemplateDataAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	bGridBuilt = false;
	RebuildGrid();

	if (!bGridUsable)
	{
		UE_LOG(LogZombieGame, Warning, TEXT("Room template '%s' is not usable: %s"), *GetName(), *LayoutError);
	}
}
#endif

void URoomTemplateDataAsset::RebuildGrid() const
{
	bGridBuilt = true;
	bGridUsable = false;
	LayoutError.Reset();
	ParsedGrid = FRoomGrid();

	if (!ParsedGrid.ParseFrom(LayoutRows, LayoutError))
	{
		return;
	}

	bGridUsable = ParsedGrid.Validate(LayoutError);
}

const FRoomGrid& URoomTemplateDataAsset::GetGrid() const
{
	if (!bGridBuilt)
	{
		RebuildGrid();
	}

	return ParsedGrid;
}

bool URoomTemplateDataAsset::IsUsable() const
{
	if (!bGridBuilt)
	{
		RebuildGrid();
	}

	return bGridUsable;
}

const FString& URoomTemplateDataAsset::GetLayoutError() const
{
	if (!bGridBuilt)
	{
		RebuildGrid();
	}

	return LayoutError;
}
