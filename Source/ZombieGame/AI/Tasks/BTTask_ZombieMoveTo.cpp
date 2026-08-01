#include "BTTask_ZombieMoveTo.h"

UBTTask_ZombieMoveTo::UBTTask_ZombieMoveTo(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = TEXT("Move To");
}

void UBTTask_ZombieMoveTo::Configure(FName KeyName, float InAcceptableRadius, bool bChaseMovingGoal)
{
	BlackboardKey.SelectedKeyName = KeyName;
	AcceptableRadius = InAcceptableRadius;
	bTrackMovingGoal = bChaseMovingGoal;
	bObserveBlackboardValue = bChaseMovingGoal;
	bReachTestIncludesAgentRadius = true;
	bReachTestIncludesGoalRadius = true;

	// Zombies should still shamble as close as the navmesh allows when the player stands somewhere
	// unreachable, rather than failing the move outright and dropping back to roaming.
	bAllowPartialPath = true;

	NodeName = FString::Printf(TEXT("Move To %s"), *KeyName.ToString());
}
