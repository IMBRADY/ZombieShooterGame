#include "ZombieAIAssetSubsystem.h"
#include "AI/Blackboard/ZombieBlackboardKeys.h"
#include "AI/Decorators/BTDecorator_ZombieBlackboardKeySet.h"
#include "AI/Services/BTService_ZombieCombatState.h"
#include "AI/Tasks/BTTask_ZombieAttack.h"
#include "AI/Tasks/BTTask_ZombieChaseTarget.h"
#include "AI/Tasks/BTTask_ZombieClearBlackboardValue.h"
#include "AI/Tasks/BTTask_ZombieFindRoamLocation.h"
#include "AI/Tasks/BTTask_ZombieKeepDistance.h"
#include "AI/Tasks/BTTask_ZombieMoveTo.h"
#include "AI/Tasks/BTTask_ZombieUseAbility.h"
#include "AI/ZombieAISettings.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/Composites/BTComposite_Selector.h"
#include "BehaviorTree/Composites/BTComposite_Sequence.h"
#include "BehaviorTree/Tasks/BTTask_Wait.h"
#include "ZombieGame.h"

namespace
{
	void AddBlackboardKey(UBlackboardData& Blackboard, FName KeyName, TSubclassOf<UBlackboardKeyType> KeyTypeClass)
	{
		FBlackboardEntry Entry;
		Entry.EntryName = KeyName;
		Entry.KeyType = NewObject<UBlackboardKeyType>(&Blackboard, KeyTypeClass);
		Blackboard.Keys.Add(Entry);
	}

	void AttachComposite(UBTCompositeNode& Parent, UBTCompositeNode* Child, UBTDecorator* Decorator)
	{
		FBTCompositeChild& ChildInfo = Parent.Children.AddDefaulted_GetRef();
		ChildInfo.ChildComposite = Child;
		if (Decorator)
		{
			ChildInfo.Decorators.Add(Decorator);
		}
	}

	void AttachTask(UBTCompositeNode& Parent, UBTTaskNode* Child, UBTDecorator* Decorator = nullptr)
	{
		FBTCompositeChild& ChildInfo = Parent.Children.AddDefaulted_GetRef();
		ChildInfo.ChildTask = Child;
		if (Decorator)
		{
			ChildInfo.Decorators.Add(Decorator);
		}
	}

	template <typename TNode>
	TNode* MakeNode(UObject& Outer, const TCHAR* NodeId)
	{
		return NewObject<TNode>(&Outer, TNode::StaticClass(), FName(NodeId));
	}

	UBTTask_Wait* MakeWaitTask(UBehaviorTree& Tree, const TCHAR* NodeId, float Seconds, float Deviation)
	{
		UBTTask_Wait* Wait = MakeNode<UBTTask_Wait>(Tree, NodeId);
		Wait->WaitTime = Seconds;
		Wait->RandomDeviation = Deviation;
		return Wait;
	}

	UBTDecorator_ZombieBlackboardKeySet* MakeKeySetDecorator(UBehaviorTree& Tree, const TCHAR* NodeId,
		FName KeyName, EBTFlowAbortMode::Type AbortMode)
	{
		UBTDecorator_ZombieBlackboardKeySet* Decorator = MakeNode<UBTDecorator_ZombieBlackboardKeySet>(Tree, NodeId);
		Decorator->Configure(KeyName, /*bRequireUnset=*/false, AbortMode);
		return Decorator;
	}

	/** Fires an ability the moment one is ready, interrupting whatever lower-priority branch runs. */
	void AttachAbilityUse(UBehaviorTree& Tree, UBTCompositeNode& Combat)
	{
		UBTTask_ZombieUseAbility* UseAbility = MakeNode<UBTTask_ZombieUseAbility>(Tree, TEXT("Task_UseAbility"));
		UseAbility->Configure(ZombieBlackboardKeys::TargetActor);

		// LowerPriority only: the ability's own cooldown flips AbilityReady off the moment it fires,
		// and aborting Self on that would cancel the wind-up it just started.
		AttachTask(Combat, UseAbility,
			MakeKeySetDecorator(Tree, TEXT("Dec_AbilityReady"), ZombieBlackboardKeys::AbilityReady, EBTFlowAbortMode::LowerPriority));
	}
}

UBlackboardData* UZombieAIAssetSubsystem::GetOrBuildBlackboard()
{
	if (SharedBlackboard)
	{
		return SharedBlackboard;
	}

	SharedBlackboard = NewObject<UBlackboardData>(this, TEXT("BB_Zombie"));

	AddBlackboardKey(*SharedBlackboard, ZombieBlackboardKeys::TargetActor, UBlackboardKeyType_Object::StaticClass());
	AddBlackboardKey(*SharedBlackboard, ZombieBlackboardKeys::InvestigateLocation, UBlackboardKeyType_Vector::StaticClass());
	AddBlackboardKey(*SharedBlackboard, ZombieBlackboardKeys::RoamLocation, UBlackboardKeyType_Vector::StaticClass());
	AddBlackboardKey(*SharedBlackboard, ZombieBlackboardKeys::InAttackRange, UBlackboardKeyType_Bool::StaticClass());
	AddBlackboardKey(*SharedBlackboard, ZombieBlackboardKeys::AbilityReady, UBlackboardKeyType_Bool::StaticClass());
	AddBlackboardKey(*SharedBlackboard, ZombieBlackboardKeys::TooClose, UBlackboardKeyType_Bool::StaticClass());
	AddBlackboardKey(*SharedBlackboard, ZombieBlackboardKeys::InPreferredRange, UBlackboardKeyType_Bool::StaticClass());

	return SharedBlackboard;
}

UBTCompositeNode* UZombieAIAssetSubsystem::BuildMeleeCombatBranch(UBehaviorTree& Tree) const
{
	UBTComposite_Selector* Combat = MakeNode<UBTComposite_Selector>(Tree, TEXT("Sel_Combat"));
	Combat->NodeName = TEXT("Combat (Melee)");

	AttachAbilityUse(Tree, *Combat);

	UBTTask_ZombieAttack* Attack = MakeNode<UBTTask_ZombieAttack>(Tree, TEXT("Task_Attack"));
	Attack->Configure(ZombieBlackboardKeys::TargetActor);

	// Both: interrupt the chase the moment the target is in reach, and abandon the swing if it
	// steps back out of it.
	AttachTask(*Combat, Attack,
		MakeKeySetDecorator(Tree, TEXT("Dec_InAttackRange"), ZombieBlackboardKeys::InAttackRange, EBTFlowAbortMode::Both));

	// Chasing goes through the horde's shared flow field rather than a per-zombie MoveTo - see
	// UZombieFlowFieldSubsystem for why.
	UBTTask_ZombieChaseTarget* Chase = MakeNode<UBTTask_ZombieChaseTarget>(Tree, TEXT("Task_Chase"));
	Chase->Configure(ZombieBlackboardKeys::TargetActor);
	AttachTask(*Combat, Chase);

	return Combat;
}

UBTCompositeNode* UZombieAIAssetSubsystem::BuildRangedCombatBranch(UBehaviorTree& Tree) const
{
	UBTComposite_Selector* Combat = MakeNode<UBTComposite_Selector>(Tree, TEXT("Sel_RangedCombat"));
	Combat->NodeName = TEXT("Combat (Ranged)");

	AttachAbilityUse(Tree, *Combat);

	UBTTask_ZombieKeepDistance* KeepDistance = MakeNode<UBTTask_ZombieKeepDistance>(Tree, TEXT("Task_KeepDistance"));
	KeepDistance->Configure(ZombieBlackboardKeys::TargetActor, 1.4f);
	AttachTask(*Combat, KeepDistance,
		MakeKeySetDecorator(Tree, TEXT("Dec_TooClose"), ZombieBlackboardKeys::TooClose, EBTFlowAbortMode::LowerPriority));

	// At range with nothing ready: hold position briefly, re-evaluating as soon as range changes.
	AttachTask(*Combat, MakeWaitTask(Tree, TEXT("Task_HoldRange"), 0.4f, 0.15f),
		MakeKeySetDecorator(Tree, TEXT("Dec_InPreferredRange"), ZombieBlackboardKeys::InPreferredRange, EBTFlowAbortMode::Both));

	UBTTask_ZombieChaseTarget* Approach = MakeNode<UBTTask_ZombieChaseTarget>(Tree, TEXT("Task_Approach"));
	Approach->Configure(ZombieBlackboardKeys::TargetActor);
	AttachTask(*Combat, Approach);

	return Combat;
}

UBTCompositeNode* UZombieAIAssetSubsystem::BuildInvestigateBranch(UBehaviorTree& Tree) const
{
	const UZombieAISettings* Settings = UZombieAISettings::GetOrLoadDefault();

	UBTComposite_Sequence* Investigate = MakeNode<UBTComposite_Sequence>(Tree, TEXT("Seq_Investigate"));
	Investigate->NodeName = TEXT("Investigate Noise");

	UBTTask_ZombieMoveTo* MoveToNoise = MakeNode<UBTTask_ZombieMoveTo>(Tree, TEXT("Task_MoveToNoise"));
	MoveToNoise->Configure(ZombieBlackboardKeys::InvestigateLocation, /*AcceptableRadius=*/100.0f, /*bChaseMovingGoal=*/false);
	AttachTask(*Investigate, MoveToNoise);

	AttachTask(*Investigate, MakeWaitTask(Tree, TEXT("Task_LookAround"), Settings->InvestigateLookAroundTime, 0.5f));

	UBTTask_ZombieClearBlackboardValue* Forget = MakeNode<UBTTask_ZombieClearBlackboardValue>(Tree, TEXT("Task_ForgetNoise"));
	Forget->Configure(ZombieBlackboardKeys::InvestigateLocation);
	AttachTask(*Investigate, Forget);

	// If the noise cannot be reached the sequence fails with the key still set - and the key's own
	// decorator then re-selects this branch every frame, so the zombie froze in place. Forgetting an
	// unreachable noise lets it fall through to roaming instead.
	UBTComposite_Selector* InvestigateOrGiveUp = MakeNode<UBTComposite_Selector>(Tree, TEXT("Sel_Investigate"));
	InvestigateOrGiveUp->NodeName = TEXT("Investigate or Give Up");
	AttachComposite(*InvestigateOrGiveUp, Investigate, nullptr);

	UBTTask_ZombieClearBlackboardValue* GiveUp = MakeNode<UBTTask_ZombieClearBlackboardValue>(Tree, TEXT("Task_GiveUpOnNoise"));
	GiveUp->Configure(ZombieBlackboardKeys::InvestigateLocation);
	AttachTask(*InvestigateOrGiveUp, GiveUp);

	return InvestigateOrGiveUp;
}

UBTCompositeNode* UZombieAIAssetSubsystem::BuildRoamBranch(UBehaviorTree& Tree) const
{
	const UZombieAISettings* Settings = UZombieAISettings::GetOrLoadDefault();

	UBTComposite_Sequence* Roam = MakeNode<UBTComposite_Sequence>(Tree, TEXT("Seq_Roam"));
	Roam->NodeName = TEXT("Roam");

	// Short hops with a short pause after each, rather than long treks: a zombie that has not seen
	// anything should read as milling around its patch.
	UBTTask_ZombieFindRoamLocation* FindRoamLocation = MakeNode<UBTTask_ZombieFindRoamLocation>(Tree, TEXT("Task_FindRoamLocation"));
	FindRoamLocation->Configure(ZombieBlackboardKeys::RoamLocation, Settings->RoamRadius);
	AttachTask(*Roam, FindRoamLocation);

	UBTTask_ZombieMoveTo* MoveToRoamLocation = MakeNode<UBTTask_ZombieMoveTo>(Tree, TEXT("Task_Wander"));
	MoveToRoamLocation->Configure(ZombieBlackboardKeys::RoamLocation, /*AcceptableRadius=*/60.0f, /*bChaseMovingGoal=*/false);
	AttachTask(*Roam, MoveToRoamLocation);

	AttachTask(*Roam, MakeWaitTask(Tree, TEXT("Task_Idle"), Settings->IdleTime, Settings->IdleTimeDeviation));

	return Roam;
}

UBehaviorTree* UZombieAIAssetSubsystem::GetOrBuildTree(EZombieBehaviorProfile Profile)
{
	if (TObjectPtr<UBehaviorTree>* Existing = Trees.Find(Profile))
	{
		return *Existing;
	}

	const bool bBoss = Profile == EZombieBehaviorProfile::Boss;
	const TCHAR* TreeName = bBoss ? TEXT("BT_ZombieBoss") : Profile == EZombieBehaviorProfile::Ranged ? TEXT("BT_ZombieRanged") : TEXT("BT_ZombieMelee");

	UBehaviorTree* Tree = NewObject<UBehaviorTree>(this, FName(TreeName));
	Tree->BlackboardAsset = GetOrBuildBlackboard();

	// Priority order is the whole design: a seen target beats a heard noise, which beats idling.
	UBTComposite_Selector* Root = MakeNode<UBTComposite_Selector>(*Tree, TEXT("Sel_Root"));
	Root->NodeName = TEXT("Zombie Root");

	UBTService_ZombieCombatState* CombatState = MakeNode<UBTService_ZombieCombatState>(*Tree, TEXT("Svc_CombatState"));
	CombatState->Configure(ZombieBlackboardKeys::TargetActor, ZombieBlackboardKeys::InAttackRange, 0.15f, /*bAlwaysHunt=*/bBoss);
	Root->Services.Add(CombatState);

	UBTCompositeNode* Combat = Profile == EZombieBehaviorProfile::Ranged ? BuildRangedCombatBranch(*Tree) : BuildMeleeCombatBranch(*Tree);
	AttachComposite(*Root, Combat,
		MakeKeySetDecorator(*Tree, TEXT("Dec_HasTarget"), ZombieBlackboardKeys::TargetActor, EBTFlowAbortMode::LowerPriority));

	// Bosses never idle or investigate - the service always hands them a target while a player lives.
	if (!bBoss)
	{
		AttachComposite(*Root, BuildInvestigateBranch(*Tree),
			MakeKeySetDecorator(*Tree, TEXT("Dec_HasNoise"), ZombieBlackboardKeys::InvestigateLocation, EBTFlowAbortMode::LowerPriority));
	}
	AttachComposite(*Root, BuildRoamBranch(*Tree), nullptr);

	Tree->RootNode = Root;
	Trees.Add(Profile, Tree);

	UE_LOG(LogZombieGame, Log, TEXT("Behavior tree '%s' assembled in code: %d root branches, %d blackboard keys."),
		TreeName, Root->Children.Num(), SharedBlackboard->Keys.Num());
	return Tree;
}

UBehaviorTree* UZombieAIAssetSubsystem::GetBehaviorTreeFor(const UZombieArchetypeDataAsset* Archetype)
{
	if (Archetype && Archetype->BehaviorTreeOverride)
	{
		return Archetype->BehaviorTreeOverride;
	}

	return GetOrBuildTree(Archetype ? Archetype->BehaviorProfile : EZombieBehaviorProfile::Melee);
}
