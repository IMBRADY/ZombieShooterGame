#pragma once

#include "CoreMinimal.h"
#include "Core/ZombieStatTypes.h"
#include "UObject/Interface.h"
#include "ZombieStatSource.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnZombieStatsChanged);

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UZombieStatSource : public UInterface
{
	GENERATED_BODY()
};

/**
 * Anything that modifies a player's stats - today the perk component, later possibly armour sets
 * or temporary buffs. Consumers (movement, stamina, weapons, economy) only ever talk to this
 * interface, so a new source of modifiers never touches the systems being modified.
 */
class ZOMBIEGAME_API IZombieStatSource
{
	GENERATED_BODY()

public:
	virtual FStatModifierTotals GetStatTotals(const FGameplayTag& Stat) const = 0;

	/** Status effects every hit this player lands may apply (fire bullets, poison bullets, ...). */
	virtual void GetHitEffects(TArray<FHitEffectSpec>& OutEffects) const = 0;

	/** Raised whenever any modifier changes, so cached derived values can be refreshed. */
	virtual FOnZombieStatsChanged& OnStatsChanged() = 0;
};

/** Resolves an actor (pawn, weapon, projectile) to the stat source that modifies it. */
namespace ZombieStats
{
	/** The owning player's stat source, or null for actors no player owns (zombies, the world). */
	ZOMBIEGAME_API IZombieStatSource* FindStatSource(const AActor* Actor);

	/** BaseValue with every modifier on Stat applied; BaseValue itself when nothing modifies it. */
	ZOMBIEGAME_API float Resolve(const AActor* Actor, const FGameplayTag& Stat, float BaseValue);
}
