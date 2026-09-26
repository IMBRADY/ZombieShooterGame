#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RoomThemeDataAsset.generated.h"

class UMaterialInterface;

/**
 * How a sector looks: the "abandoned building" dressing applied to every handcrafted module in it -
 * floor, wall and prop materials (pixel-art texture atlases), scattered floor debris, and lighting.
 * One theme is rolled per sector, so consecutive sectors read as different parts of the building
 * (offices, a factory floor, a lab) while every room layout stays handcrafted.
 */
UCLASS(BlueprintType)
class ZOMBIEGAME_API URoomThemeDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	static const FPrimaryAssetType AssetType;
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Theme")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Theme", meta = (ClampMin = "0.0"))
	float SelectionWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Theme", meta = (ClampMin = "1"))
	int32 MinSector = 1;

	// --- Surfaces ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surfaces")
	TObjectPtr<UMaterialInterface> FloorMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surfaces")
	TObjectPtr<UMaterialInterface> WallMaterial;

	/** Atlas material; each obstacle picks a variant through per-instance custom data. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surfaces")
	TObjectPtr<UMaterialInterface> ObstacleMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Surfaces", meta = (ClampMin = "1"))
	int32 ObstacleVariants = 4;

	// --- Random decoration ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Decoration")
	TObjectPtr<UMaterialInterface> DecorationMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Decoration", meta = (ClampMin = "1"))
	int32 DecorationVariants = 8;

	/** Chance each open floor tile gets a piece of debris ("random decoration"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Decoration", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DecorationChance = 0.14f;

	// --- Lighting ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lighting")
	FLinearColor LightColor = FLinearColor(1.0f, 0.85f, 0.6f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lighting", meta = (ClampMin = "0.0"))
	float LightIntensity = 9000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lighting", meta = (ClampMin = "0.0"))
	float LightRadius = 1600.0f;

	/** Fraction of rooms whose light flickers - the building's power is failing. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lighting", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FlickerChance = 0.35f;
};
