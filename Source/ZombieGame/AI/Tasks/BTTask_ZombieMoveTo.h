#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_MoveTo.h"
#include "BTTask_ZombieMoveTo.generated.h"

/**
 * The engine's MoveTo task, configurable from C++.
 *
 * Pathfinding, partial paths, goal tracking and abort handling are all Unreal's (per prompt.txt,
 * "avoid reinventing features already provided by Unreal Engine") - the only thing added here is
 * a way to set the Blackboard key and reach tolerance without an editor graph.
 */
UCLASS()
class ZOMBIEGAME_API UBTTask_ZombieMoveTo : public UBTTask_MoveTo
{
	GENERATED_UCLASS_BODY()

	/**
	 * @param KeyName			Blackboard key holding the goal (actor or vector).
	 * @param InAcceptableRadius	How close counts as arrived.
	 * @param bChaseMovingGoal	Re-path as the goal moves, and react to the key changing.
	 */
	void Configure(FName KeyName, float InAcceptableRadius, bool bChaseMovingGoal);
};
