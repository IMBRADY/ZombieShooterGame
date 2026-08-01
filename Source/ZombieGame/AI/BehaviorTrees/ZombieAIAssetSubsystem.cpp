#include "ZombieAIAssetSubsystem.h"
#include "AI/Blackboard/ZombieBlackboardKeys.h"
#include "AI/Decorators/BTDecorator_ZombieBlackboardKeySet.h"
#include "AI/Services/BTService_ZombieCombatState.h"
#include "AI/Tasks/BTTask_ZombieAttack.h"
#include "AI/Tasks/BTTask_ZombieClearBlackboardValue.h"
#include "AI/Tasks/BTTask_ZombieFindRoamLocation.h"
#include "AI/Tasks/BTTask_ZombieMoveTo.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/Composites/BTComposite_Selector.h"
#include "BehaviorTree/Composites/BTComposite_Sequence.h"
#include "BehaviorTree/Tasks/BTTask_Wait.h"
#include "Characters/Zombies/ZombieArchetypeDataAsset.h"
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

	UBTTask_Wait* MakeWaitTask(UBehaviorTree& Tree, const TCHAR* NodeId, float Seconds, float Deviation)
	{
		UBTTask_Wait* Wait = NewObject<UBTTask_Wait>(&Tree, UBTTask_Wait::StaticClass(), FName(NodeId));
		Wait->WaitTime = Seconds;
		Wait->RandomDeviation = Deviation;
		return Wait;
	}

	UBTDecorator_ZombieBlackboardKeySet* MakeKeySetDecorator(UBehaviorTree& Tree, const TCHAR* NodeId,
		FName KeyName, EBTFlowAbortMode::Type AbortMode)
	{
		UBTDecorator_ZombieBlackboardKeySet* Decorator =
			NewObject<UBTDecorator_ZombieBlackboardKeySet>(&Tree, UBTDecorator_ZombieBlackboardKeySet::StaticClass(), FName(NodeId));
		Decorator->Configure(KeyName, /*bRequireUnset=*/false, AbortMode);
		return Decorator;
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

	return SharedBlackboard;
}

UBTCompositeNode* UZombieAIAssetSubsystem::BuildCombatBranch(UBehaviorTree& Tree) const
{
	UBTComposite_Selector* Combat = NewObject<UBTComposite_Selector>(&Tree, UBTComposite_Selector::StaticClass(), TEXT("Sel_Combat"));
	Combat->NodeName = TEXT("Combat");

	UBTTask_ZombieAttack* Attack = NewObject<UBTTask_ZombieAttack>(&Tree, UBTTask_ZombieAttack::StaticClass(), TEXT("Task_Attack"));
	Attack->Configure(ZombieBlackboardKeys::TargetActor);

	// Both: interrupt the chase the moment the target is in reach, and abandon the swing if it
	// steps back out of it.
	AttachTask(*Combat, Attack,
		MakeKeySetDecorator(Tree, TEXT("Dec_InAttackRange"), ZombieBlackboardKeys::InAttackRange, EBTFlowAbortMode::Both));

	UBTTask_ZombieMoveTo* Chase = NewObject<UBTTask_ZombieMoveTo>(&Tree, UBTTask_ZombieMoveTo::StaticClass(), TEXT("Task_Chase"));
	Chase->Configure(ZombieBlackboardKeys::TargetActor, /*AcceptableRadius=*/80.0f, /*bChaseMovingGoal=*/true);
	AttachTask(*Combat, Chase);

	return Combat;
}

UBTCompositeNode* UZombieAIAssetSubsystem::BuildInvestigateBranch(UBehaviorTree& Tree) const
{
	UBTComposite_Sequence* Investigate = NewObject<UBTComposite_Sequence>(&Tree, UBTComposite_Sequence::StaticClass(), TEXT("Seq_Investigate"));
	Investigate->NodeName = TEXT("Investigate Noise");

	UBTTask_ZombieMoveTo* MoveToNoise = NewObject<UBTTask_ZombieMoveTo>(&Tree, UBTTask_ZombieMoveTo::StaticClass(), TEXT("Task_MoveToNoise"));
	MoveToNoise->Configure(ZombieBlackboardKeys::InvestigateLocation, /*AcceptableRadius=*/100.0f, /*bChaseMovingGoal=*/false);
	AttachTask(*Investigate, MoveToNoise);

	AttachTask(*Investigate, MakeWaitTask(Tree, TEXT("Task_LookAround"), 1.5f, 0.5f));

	UBTTask_ZombieClearBlackboardValue* Forget =
		NewObject<UBTTask_ZombieClearBlackboardValue>(&Tree, UBTTask_ZombieClearBlackboardValue::StaticClass(), TEXT("Task_ForgetNoise"));
	Forget->Configure(ZombieBlackboardKeys::InvestigateLocation);
	AttachTask(*Investigate, Forget);

	return Investigate;
}

UBTCompositeNode* UZombieAIAssetSubsystem::BuildRoamBranch(UBehaviorTree& Tree) const
{
	UBTComposite_Sequence* Roam = NewObject<UBTComposite_Sequence>(&Tree, UBTComposite_Sequence::StaticClass(), TEXT("Seq_Roam"));
	Roam->NodeName = TEXT("Roam");

	UBTTask_ZombieFindRoamLocation* FindRoamLocation =
		NewObject<UBTTask_ZombieFindRoamLocation>(&Tree, UBTTask_ZombieFindRoamLocation::StaticClass(), TEXT("Task_FindRoamLocation"));
	FindRoamLocation->Configure(ZombieBlackboardKeys::RoamLocation, /*RoamRadius=*/1200.0f);
	AttachTask(*Roam, FindRoamLocation);

	UBTTask_ZombieMoveTo* MoveToRoamLocation = NewObject<UBTTask_ZombieMoveTo>(&Tree, UBTTask_ZombieMoveTo::StaticClass(), TEXT("Task_Wander"));
	MoveToRoamLocation->Configure(ZombieBlackboardKeys::RoamLocation, /*AcceptableRadius=*/60.0f, /*bChaseMovingGoal=*/false);
	AttachTask(*Roam, MoveToRoamLocation);

	// Idle beat between wanders, so a quiet sector doesn't look like a conveyor belt.
	AttachTask(*Roam, MakeWaitTask(Tree, TEXT("Task_Idle"), 2.5f, 1.5f));

	return Roam;
}

UBehaviorTree* UZombieAIAssetSubsystem::GetOrBuildSharedTree()
{
	if (SharedBehaviorTree)
	{
		return SharedBehaviorTree;
	}

	SharedBehaviorTree = NewObject<UBehaviorTree>(this, TEXT("BT_Zombie"));
	SharedBehaviorTree->BlackboardAsset = GetOrBuildBlackboard();

	// Priority order is the whole design: a seen target beats a heard noise, which beats idling.
	UBTComposite_Selector* Root = NewObject<UBTComposite_Selector>(SharedBehaviorTree, UBTComposite_Selector::StaticClass(), TEXT("Sel_Root"));
	Root->NodeName = TEXT("Zombie Root");

	UBTService_ZombieCombatState* CombatState =
		NewObject<UBTService_ZombieCombatState>(SharedBehaviorTree, UBTService_ZombieCombatState::StaticClass(), TEXT("Svc_CombatState"));
	CombatState->Configure(ZombieBlackboardKeys::TargetActor, ZombieBlackboardKeys::InAttackRange, 0.15f);
	Root->Services.Add(CombatState);

	AttachComposite(*Root, BuildCombatBranch(*SharedBehaviorTree),
		MakeKeySetDecorator(*SharedBehaviorTree, TEXT("Dec_HasTarget"), ZombieBlackboardKeys::TargetActor, EBTFlowAbortMode::LowerPriority));

	AttachComposite(*Root, BuildInvestigateBranch(*SharedBehaviorTree),
		MakeKeySetDecorator(*SharedBehaviorTree, TEXT("Dec_HasNoise"), ZombieBlackboardKeys::InvestigateLocation, EBTFlowAbortMode::LowerPriority));

	AttachComposite(*Root, BuildRoamBranch(*SharedBehaviorTree), nullptr);

	SharedBehaviorTree->RootNode = Root;

	UE_LOG(LogZombieGame, Log, TEXT("Zombie behavior tree assembled in code: %d root branches, %d blackboard keys."),
		Root->Children.Num(), SharedBlackboard->Keys.Num());

	return SharedBehaviorTree;
}

UBehaviorTree* UZombieAIAssetSubsystem::GetBehaviorTreeFor(const UZombieArchetypeDataAsset* Archetype)
{
	if (Archetype && Archetype->BehaviorTreeOverride)
	{
		return Archetype->BehaviorTreeOverride;
	}

	return GetOrBuildSharedTree();
}
