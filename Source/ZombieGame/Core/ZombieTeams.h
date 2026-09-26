#pragma once

#include "CoreMinimal.h"

class AActor;

/**
 * Who fights whom. There are two sides - the players and the dead - and every "should this hit
 * that" question (bullets, explosions, projectiles, perception) asks here instead of casting to
 * concrete classes, so a future third faction or friendly-fire rule is one change.
 */
namespace ZombieTeams
{
	enum class ETeam : uint8
	{
		None,
		Players,
		Zombies
	};

	/** Resolves an actor, or whatever pawn owns it (a weapon, a projectile), to its side. */
	ZOMBIEGAME_API ETeam GetTeam(const AActor* Actor);

	ZOMBIEGAME_API bool AreEnemies(const AActor* A, const AActor* B);

	/** True when damage from Source should be applied to Target at all. */
	ZOMBIEGAME_API bool CanDamage(const AActor* Source, const AActor* Target, bool bFriendlyFire);
}
