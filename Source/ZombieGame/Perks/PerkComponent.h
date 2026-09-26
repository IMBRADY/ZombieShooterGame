#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/Save/ZombieRunTypes.h"
#include "Core/ZombieStatSource.h"
#include "PerkComponent.generated.h"

class UPerkDataAsset;

USTRUCT(BlueprintType)
struct FOwnedPerk
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Perk")
	TObjectPtr<UPerkDataAsset> Perk;

	UPROPERTY(BlueprintReadOnly, Category = "Perk")
	int32 Tier = 0;
};

/**
 * The Perk Manager, one per player, living on the PlayerState (prompt.txt: "PlayerState tracks ...
 * Perks") so perks follow the player rather than whichever pawn they are driving.
 *
 * It is the game's stat source: everything a perk changes is answered from here by tag, and
 * OnStatsChanged tells movement, stamina, health and weapons to refresh their cached values.
 * "Perks remain until death" - the component simply dies with the run.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ZOMBIEGAME_API UPerkComponent : public UActorComponent, public IZombieStatSource
{
	GENERATED_BODY()

public:
	UPerkComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// IZombieStatSource
	virtual FStatModifierTotals GetStatTotals(const FGameplayTag& Stat) const override;
	virtual void GetHitEffects(TArray<FHitEffectSpec>& OutEffects) const override;
	virtual FOnZombieStatsChanged& OnStatsChanged() override { return StatsChangedEvent; }

	/** Raises the perk one tier. Returns false if it is already maxed. Authority only. */
	bool AddPerkTier(UPerkDataAsset* Perk);

	int32 GetTier(const UPerkDataAsset* Perk) const;
	bool IsMaxed(const UPerkDataAsset* Perk) const;
	const TArray<FOwnedPerk>& GetOwnedPerks() const { return OwnedPerks; }

	void ExportRecords(TArray<FOwnedPerkRecord>& OutRecords) const;
	void ImportRecords(const TArray<FOwnedPerkRecord>& Records);

private:
	UFUNCTION()
	void OnRep_OwnedPerks();

	UPROPERTY(ReplicatedUsing = OnRep_OwnedPerks)
	TArray<FOwnedPerk> OwnedPerks;

	FOnZombieStatsChanged StatsChangedEvent;
};
