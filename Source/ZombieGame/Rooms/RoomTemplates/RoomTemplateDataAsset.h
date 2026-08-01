#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Rooms/Procedural/RoomGrid.h"
#include "Rooms/RoomTypes.h"
#include "RoomTemplateDataAsset.generated.h"

/**
 * A handcrafted room module: a designer-authored tile grid plus the rules for when the sector
 * generator may use it. Adding a new room to the game is a new Data Asset - never a code change
 * (prompt.txt "Data Driven Design", ARCHITECTURE.md 5 and 7).
 *
 * Layout characters, one string per row:
 *   '#' wall   '.' floor   'D' doorway (must sit on a module edge)   'O' obstacle / cover
 *   'S' zombie spawn point   'P' player start   ' ' outside the module (lets modules interlock)
 */
UCLASS(BlueprintType)
class ZOMBIEGAME_API URoomTemplateDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	static const FPrimaryAssetType AssetType;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	ERoomType GetRoomType() const { return RoomType; }
	float GetSelectionWeight() const { return SelectionWeight; }
	int32 GetMinSector() const { return MinSector; }
	bool AllowsRotation() const { return bAllowRotation; }

	/** Parsed layout. Empty if the layout failed to parse - check IsUsable() first. */
	const FRoomGrid& GetGrid() const;

	/** True when the layout parsed and satisfies the module rules (see FRoomGrid::Validate). */
	bool IsUsable() const;

	/** Reason the module is unusable, for logging. Empty when it is usable. */
	const FString& GetLayoutError() const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room")
	ERoomType RoomType = ERoomType::Combat;

	/** One string per row. See the class comment for the tile alphabet. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room")
	TArray<FString> LayoutRows;

	/** Relative likelihood of this module being picked over other eligible ones. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room", meta = (ClampMin = "0.0"))
	float SelectionWeight = 1.0f;

	/** Sector at which this module starts appearing, so later sectors can introduce new layouts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room", meta = (ClampMin = "1"))
	int32 MinSector = 1;

	/** Clear for modules that only read correctly in their authored orientation. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room")
	bool bAllowRotation = true;

private:
	void RebuildGrid() const;

	mutable FRoomGrid ParsedGrid;
	mutable FString LayoutError;
	mutable bool bGridUsable = false;
	mutable bool bGridBuilt = false;
};
