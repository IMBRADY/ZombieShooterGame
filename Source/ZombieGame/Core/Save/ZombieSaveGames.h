#pragma once

#include "CoreMinimal.h"
#include "Core/Save/ZombieRunTypes.h"
#include "Core/Save/ZombieUserSettings.h"
#include "GameFramework/SaveGame.h"
#include "ZombieSaveGames.generated.h"

/** Lifetime numbers across every run - the Statistics screen and most achievements read these. */
USTRUCT(BlueprintType)
struct FZombieLifetimeStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Stats") int32 RunsStarted = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Stats") int32 Deaths = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Stats") int32 TotalKills = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Stats") int32 BossesDefeated = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Stats") int32 SectorsCleared = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Stats") int32 HighestSector = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Stats") int32 MoneyEarned = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Stats") int32 PerksBought = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Stats") int32 LegendariesFound = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Stats") float PlaySeconds = 0.0f;
};

USTRUCT(BlueprintType)
struct FZombieHighScore
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Score") int32 Sector = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Score") int32 Kills = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Score") int32 MoneyEarned = 0;
	UPROPERTY(BlueprintReadOnly, Category = "Score") float Seconds = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Score") FDateTime Date;
};

/**
 * The permanent save: settings, statistics, achievements, unlocks and high scores (ARCHITECTURE.md
 * 11). Survives death - it is the only thing that does, since runs have no permanent progression.
 */
UCLASS()
class ZOMBIEGAME_API UZombieMetaSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** Bumped whenever the layout changes incompatibly; older saves are migrated or reset. */
	static constexpr int32 CurrentVersion = 1;

	UPROPERTY()
	int32 Version = CurrentVersion;

	UPROPERTY()
	FZombieUserSettings Settings;

	UPROPERTY()
	FZombieLifetimeStats Lifetime;

	/** Best runs, best first. */
	UPROPERTY()
	TArray<FZombieHighScore> HighScores;

	UPROPERTY()
	TArray<FName> UnlockedAchievements;

	UPROPERTY()
	TArray<FName> Unlocks;
};

/** The mid-run checkpoint. Deleted when the run ends - death is permanent. */
UCLASS()
class ZOMBIEGAME_API UZombieRunSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr int32 CurrentVersion = 1;

	UPROPERTY()
	int32 Version = CurrentVersion;

	UPROPERTY()
	FZombieRunSaveData Run;
};
