#include "WeaponFireMode_Melee.h"
#include "Characters/Zombies/ZombieArchetypeDataAsset.h"
#include "Characters/Zombies/ZombieCharacter.h"
#include "CollisionQueryParams.h"
#include "Components/HealthComponent.h"
#include "Core/ZombieDamageTypes.h"
#include "Core/ZombieTeams.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Visual/ZombieEffectsSubsystem.h"
#include "Weapons/WeaponDataAsset.h"
#include "Weapons/ZombieHitResolver.h"
#include "Weapons/ZombieWeapon.h"

void UWeaponFireMode_Melee::Fire(const FWeaponFireContext& Context) const
{
	AActor* Target = Context.Weapon ? FindTarget(Context) : nullptr;
	if (!Target)
	{
		return;
	}

	bool bExecute = false;
	FZombieHitRequest Request;
	Request.Target = Target;
	Request.BaseDamage = ResolveDamage(Context, *Target, bExecute);
	Request.CritChance = bExecute ? 0.0f : Context.Stats.CritChance;
	Request.CritMultiplier = Context.Stats.CritMultiplier;
	Request.DamageType = UDamageType_Melee::StaticClass();
	Request.InstigatorController = Context.InstigatorController;
	Request.DamageCauser = Context.Weapon;
	Request.HitLocation = Target->GetActorLocation();
	Request.ShotDirection = Context.Direction;
	Request.HitEffects = &Context.HitEffects;
	FZombieHitResolver::ApplyHit(Request);

	if (UZombieEffectsSubsystem* Effects = UZombieEffectsSubsystem::Get(Context.Weapon))
	{
		Effects->PlayEffect(Context.Definition->ImpactEffect, Context.Weapon->ToVisualShotHeight(Target->GetActorLocation()),
			Context.Direction.Rotation().Yaw);
	}
}

AActor* UWeaponFireMode_Melee::FindTarget(const FWeaponFireContext& Context) const
{
	UWorld* World = Context.Weapon->GetWorld();
	const AActor* Attacker = Context.InstigatorPawn ? static_cast<const AActor*>(Context.InstigatorPawn) : Context.Weapon;
	if (!World || !Attacker)
	{
		return nullptr;
	}

	const FVector Origin = Attacker->GetActorLocation();
	const float Reach = Context.Stats.Range;
	const float MinDot = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(Context.Definition->MeleeArcHalfAngle, 1.0f, 180.0f)));

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(Reach));

	AActor* Best = nullptr;
	float BestScore = TNumericLimits<float>::Max();

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Candidate = Overlap.GetActor();
		if (!Candidate || Candidate == Attacker || !ZombieTeams::AreEnemies(Context.InstigatorPawn, Candidate))
		{
			continue;
		}

		const UHealthComponent* Health = Candidate->FindComponentByClass<UHealthComponent>();
		if (Health && Health->IsDead())
		{
			continue;
		}

		// Reach is measured to the target's edge, so a big body is as stabbable as a small one.
		const FVector ToCandidate = Candidate->GetActorLocation() - Origin;
		const float EdgeDistance = ToCandidate.Size2D() - Candidate->GetSimpleCollisionRadius();
		const float Alignment = FVector::DotProduct(ToCandidate.GetSafeNormal2D(), Context.Direction.GetSafeNormal2D());
		if (EdgeDistance > Reach || (Alignment < MinDot && EdgeDistance > 0.0f))
		{
			continue;
		}

		FCollisionQueryParams Params(SCENE_QUERY_STAT(ZombieMeleeLOS), false);
		Params.AddIgnoredActor(Attacker);
		Params.AddIgnoredActor(Candidate);
		if (World->LineTraceTestByObjectType(Origin, Candidate->GetActorLocation(), FCollisionObjectQueryParams(ECC_WorldStatic), Params))
		{
			continue;
		}

		// Nearest wins, with a penalty for being off the aim line so you stab what you point at.
		const float Score = FMath::Max(EdgeDistance, 0.0f) + (1.0f - Alignment) * Reach;
		if (Score < BestScore)
		{
			BestScore = Score;
			Best = Candidate;
		}
	}
	return Best;
}

float UWeaponFireMode_Melee::ResolveDamage(const FWeaponFireContext& Context, const AActor& Target, bool& bOutExecute)
{
	bOutExecute = false;

	const AZombieCharacter* Zombie = Cast<AZombieCharacter>(&Target);
	const UZombieArchetypeDataAsset* Archetype = Zombie ? Zombie->GetArchetype() : nullptr;
	const UHealthComponent* Health = Target.FindComponentByClass<UHealthComponent>();
	if (Archetype && Health && Context.Definition->OneHitKillTiers.Contains(Archetype->Tier))
	{
		// Far more than it has left, so no resistance or armour can leave it standing.
		bOutExecute = true;
		return FMath::Max(Context.Stats.Damage, (Health->GetHealth() + Health->GetArmor()) * 10.0f + 1.0f);
	}
	return Context.Stats.Damage;
}
