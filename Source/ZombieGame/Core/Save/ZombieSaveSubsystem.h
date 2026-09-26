#pragma once

#include "CoreMinimal.h"
#include "Core/Save/ZombieRunTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ZombieSaveSubsystem.generated.h"

class UZombieMetaSaveGame;
struct FZombieHighScore;

DECLARE_MULTICAST_DELEGATE(FOnMetaSaveChanged);

/**
 * The Save Manager: owns both save slots and is the only code that touches them.
 *
 * Built on Unreal's USaveGame / SaveGameToSlot, so Steam Cloud later is a matter of which folder
 * the platform syncs, not a code change (ARCHITECTURE.md 11 and 16).
 */
UCLASS()
class ZOMBIEGAME_API UZombieSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	static UZombieSaveSubsystem* Get(const UObject* WorldContext);

	// --- Meta save (always present) ---

	UZombieMetaSaveGame* GetMeta() const { return Meta; }

	/** Writes the meta save to disk and notifies listeners (achievements, statistics screen). */
	void SaveMeta();

	/** Records a finished run into lifetime statistics and the high-score table. */
	void RecordRunEnded(const FZombieRunSaveData& FinalState);

	FOnMetaSaveChanged OnMetaSaveChanged;

	// --- Run save (only while a run is in progress) ---

	bool HasRunSave() const;
	bool LoadRun(FZombieRunSaveData& OutRun) const;
	void SaveRun(const FZombieRunSaveData& Run);
	void DeleteRun();

private:
	UPROPERTY(Transient)
	TObjectPtr<UZombieMetaSaveGame> Meta;

	static const FString MetaSlotName;
	static const FString RunSlotName;
	static constexpr int32 UserIndex = 0;
	static constexpr int32 MaxHighScores = 10;
};
