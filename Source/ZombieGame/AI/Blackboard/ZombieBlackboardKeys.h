#pragma once

#include "CoreMinimal.h"

/**
 * The zombie Blackboard's vocabulary, in one place.
 *
 * The AIController writes these from perception events, Behavior Tree nodes read them. Keeping
 * them as shared constants rather than string literals scattered across nodes means a renamed key
 * is a compile-time concern, not a silent "AI just stands there" bug.
 */
namespace ZombieBlackboardKeys
{
	/** Object: the actor the zombie is actively hunting. Unset means nothing is being chased. */
	ZOMBIEGAME_API extern const FName TargetActor;

	/** Vector: a noise or last-known-position worth walking over to inspect. */
	ZOMBIEGAME_API extern const FName InvestigateLocation;

	/** Vector: the wander destination picked while idle. */
	ZOMBIEGAME_API extern const FName RoamLocation;

	/** Bool: target is close enough to attack. Maintained by BTService_ZombieCombatState. */
	ZOMBIEGAME_API extern const FName InAttackRange;
}
