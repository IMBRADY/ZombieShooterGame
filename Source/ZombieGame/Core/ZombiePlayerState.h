#pragma once

#include "CoreMinimal.h"
#include "Core/Save/ZombieRunTypes.h"
#include "GameFramework/PlayerState.h"
#include "ZombiePlayerState.generated.h"

class UPerkComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMoneyChanged, int32, NewMoney);
DECLARE_MULTICAST_DELEGATE(FOnRunStatsChanged);

/**
 * Per-player run state: money, statistics and perks (prompt.txt "PlayerState tracks: Money,
 * Statistics, Kills, Perks, Inventory references"). Kept per player even in single player, so a
 * second player later is additive.
 *
 * Perks are a component (the Perk Manager) rather than fields here, which keeps this class about
 * bookkeeping and the perk rules in one testable place.
 */
UCLASS()
class ZOMBIEGAME_API AZombiePlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AZombiePlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	int32 GetMoney() const { return Money; }

	/** Adds money. Earned money (kills, pickups) counts toward statistics; refunds and sales don't. */
	void AddMoney(int32 Amount, bool bCountsAsEarned = true);
	bool SpendMoney(int32 Amount);

	/** Replaces the balance outright - used when restoring a saved run. */
	void SetMoney(int32 NewMoney);

	const FZombieRunStats& GetRunStats() const { return RunStats; }
	void SetRunStats(const FZombieRunStats& Stats);

	void RecordKill(const FText& WeaponName);
	void RecordBossDefeated();
	void RecordDamageTaken(float Amount);
	void RecordSectorCleared();
	void RecordElapsedTime(float Seconds);

	UPerkComponent* GetPerks() const { return Perks; }

	UPROPERTY(BlueprintAssignable)
	FOnMoneyChanged OnMoneyChanged;

	FOnRunStatsChanged OnRunStatsChanged;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UPerkComponent> Perks;

	UPROPERTY(ReplicatedUsing = OnRep_Money)
	int32 Money = 0;

	UPROPERTY(ReplicatedUsing = OnRep_RunStats)
	FZombieRunStats RunStats;

	UFUNCTION()
	void OnRep_Money();

	UFUNCTION()
	void OnRep_RunStats();
};
