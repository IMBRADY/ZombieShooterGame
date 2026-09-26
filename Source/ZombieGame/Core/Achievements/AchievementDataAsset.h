#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AchievementDataAsset.generated.h"

/** The number an achievement watches. */
UENUM(BlueprintType)
enum class EAchievementStat : uint8
{
	Kills,
	BossesDefeated,
	SectorReached,
	SectorsCleared,
	MoneyEarned,
	PerksBought,
	LegendariesFound,
	RunsStarted,
	Deaths
};

/** Whether the threshold must be hit within one run or accumulated across all of them. */
UENUM(BlueprintType)
enum class EAchievementScope : uint8
{
	Lifetime,
	SingleRun
};

/**
 * One achievement as data: which statistic, over what scope, reaching what value. The platform
 * achievement ID (Steam API name) is the asset's name, so a new achievement is a new asset plus
 * the matching entry on the platform's side - no code.
 */
UCLASS(BlueprintType)
class ZOMBIEGAME_API UAchievementDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	static const FPrimaryAssetType AssetType;
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Achievement")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Achievement")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Achievement")
	EAchievementStat Stat = EAchievementStat::Kills;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Achievement")
	EAchievementScope Scope = EAchievementScope::Lifetime;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Achievement", meta = (ClampMin = "1"))
	int32 Threshold = 1;

	/** Hidden achievements show "???" until unlocked. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Achievement")
	bool bHidden = false;
};
