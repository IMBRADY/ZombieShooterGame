#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ZombieAchievementSubsystem.generated.h"

class UAchievementDataAsset;
struct FZombieRunStats;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnZombieAchievementUnlocked, const UAchievementDataAsset* /*Achievement*/);

/**
 * Evaluates achievement definitions against statistics, records unlocks in the meta save, and
 * forwards each unlock to the platform through the OnlineSubsystem achievements interface - which
 * is where Steam comes in, with no change here (ARCHITECTURE.md 16).
 */
UCLASS()
class ZOMBIEGAME_API UZombieAchievementSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UZombieAchievementSubsystem* Get(const UObject* WorldContext);

	/**
	 * Checks every locked achievement against lifetime statistics plus, optionally, the run in
	 * progress (for single-run achievements and so lifetime ones unlock the moment they're earned).
	 */
	void Evaluate(const FZombieRunStats* LiveRun = nullptr, int32 LiveSector = 0);

	void GetAllAchievements(TArray<const UAchievementDataAsset*>& OutAchievements) const;
	bool IsUnlocked(const UAchievementDataAsset* Achievement) const;

	/** Current progress toward the threshold, for the statistics screen. */
	int32 GetLifetimeProgress(const UAchievementDataAsset* Achievement) const;

	FOnZombieAchievementUnlocked OnAchievementUnlocked;

private:
	int32 GetStatValue(const UAchievementDataAsset& Achievement, const FZombieRunStats* LiveRun, int32 LiveSector) const;
	void Unlock(const UAchievementDataAsset& Achievement);
	void ReportToPlatform(const UAchievementDataAsset& Achievement) const;
};
