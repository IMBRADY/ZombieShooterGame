#pragma once

#include "CoreMinimal.h"
#include "Characters/Zombies/ZombieArchetypeDataAsset.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ZombieAIAssetSubsystem.generated.h"

class UBehaviorTree;
class UBTCompositeNode;
class UBlackboardData;

/**
 * Owns the shared zombie Blackboard and the Behavior Trees - one per behaviour profile (melee,
 * ranged/support, boss).
 *
 * The trees are assembled in C++ rather than authored as .uasset files, for the same reason the
 * player's Enhanced Input actions are: AI behaviour is gameplay logic, and gameplay logic belongs
 * in source control as source, reviewable in a diff. The nodes themselves are ordinary Behavior
 * Tree nodes - Unreal's own composites, MoveTo and Wait, plus this project's tasks - so this is a
 * different *authoring* route to a normal tree, not a different runtime.
 *
 * The seam back to editor authoring stays open: an archetype with BehaviorTreeOverride set uses
 * that asset instead, with no code change.
 *
 * Lives on the GameInstance so each tree is built once per session and shared by every zombie of
 * its profile, and so the objects stay referenced for as long as they matter.
 */
UCLASS()
class ZOMBIEGAME_API UZombieAIAssetSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** The archetype's own tree if it has one, otherwise the shared tree for its profile. */
	UBehaviorTree* GetBehaviorTreeFor(const UZombieArchetypeDataAsset* Archetype);

private:
	UBlackboardData* GetOrBuildBlackboard();
	UBehaviorTree* GetOrBuildTree(EZombieBehaviorProfile Profile);

	/** Chase, swing and use abilities (melee and boss profiles). */
	UBTCompositeNode* BuildMeleeCombatBranch(UBehaviorTree& Tree) const;

	/** Hold at preferred range, back off when crowded, use abilities (ranged/support profile). */
	UBTCompositeNode* BuildRangedCombatBranch(UBehaviorTree& Tree) const;

	/** Walk to a noise and look around before losing interest. */
	UBTCompositeNode* BuildInvestigateBranch(UBehaviorTree& Tree) const;

	/** Idle wandering - the default state when nothing has been seen or heard. */
	UBTCompositeNode* BuildRoamBranch(UBehaviorTree& Tree) const;

	UPROPERTY(Transient)
	TObjectPtr<UBlackboardData> SharedBlackboard;

	UPROPERTY(Transient)
	TMap<EZombieBehaviorProfile, TObjectPtr<UBehaviorTree>> Trees;
};
