#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ZombieAIAssetSubsystem.generated.h"

class UBehaviorTree;
class UBTCompositeNode;
class UBlackboardData;
class UZombieArchetypeDataAsset;

/**
 * Owns the shared zombie Blackboard and Behavior Tree.
 *
 * The tree is assembled in C++ rather than authored as a .uasset, for the same reason the player's
 * Enhanced Input actions are (Milestone 2): AI behaviour is gameplay logic, and gameplay logic
 * belongs in source control as source, reviewable in a diff. The nodes themselves are ordinary
 * Behavior Tree nodes - Unreal's own composites, MoveTo, and Wait, plus this project's tasks - so
 * this is a different *authoring* route to a normal tree, not a different runtime.
 *
 * The seam back to editor authoring stays open: an archetype with BehaviorTreeOverride set uses
 * that asset instead, with no code change.
 *
 * Lives on the GameInstance so the tree is built once per session and shared by every zombie, and
 * so the objects stay referenced (and therefore not garbage collected) for as long as they matter.
 */
UCLASS()
class ZOMBIEGAME_API UZombieAIAssetSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** The archetype's own tree if it has one, otherwise the shared zombie tree. */
	UBehaviorTree* GetBehaviorTreeFor(const UZombieArchetypeDataAsset* Archetype);

private:
	UBlackboardData* GetOrBuildBlackboard();
	UBehaviorTree* GetOrBuildSharedTree();

	/** Chase the target, swinging whenever it is in reach. */
	UBTCompositeNode* BuildCombatBranch(UBehaviorTree& Tree) const;

	/** Walk to a noise and look around before losing interest. */
	UBTCompositeNode* BuildInvestigateBranch(UBehaviorTree& Tree) const;

	/** Idle wandering - the default state when nothing has been seen or heard. */
	UBTCompositeNode* BuildRoamBranch(UBehaviorTree& Tree) const;

	UPROPERTY(Transient)
	TObjectPtr<UBlackboardData> SharedBlackboard;

	UPROPERTY(Transient)
	TObjectPtr<UBehaviorTree> SharedBehaviorTree;
};
