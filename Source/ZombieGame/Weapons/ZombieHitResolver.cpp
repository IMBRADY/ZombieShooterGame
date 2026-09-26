#include "ZombieHitResolver.h"
#include "CollisionQueryParams.h"
#include "Components/HealthComponent.h"
#include "Components/StatusEffectComponent.h"
#include "Core/ZombieDamageTypes.h"
#include "Core/ZombieGameplayTags.h"
#include "Core/ZombieStatSource.h"
#include "Core/ZombieTeams.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

FZombieHitResult FZombieHitResolver::ApplyHit(const FZombieHitRequest& Request)
{
	FZombieHitResult Result;
	if (!IsValid(Request.Target) || Request.BaseDamage <= 0.0f)
	{
		return Result;
	}

	const UHealthComponent* TargetHealth = Request.Target->FindComponentByClass<UHealthComponent>();
	if (TargetHealth && TargetHealth->IsDead())
	{
		return Result;
	}

	Result.bCritical = FMath::FRand() < Request.CritChance;
	const float Damage = Request.BaseDamage * (Result.bCritical ? FMath::Max(Request.CritMultiplier, 1.0f) : 1.0f);

	FHitResult HitInfo;
	HitInfo.Location = Request.HitLocation;
	HitInfo.ImpactPoint = Request.HitLocation;
	HitInfo.HitObjectHandle = FActorInstanceHandle(Request.Target);

	TSubclassOf<UDamageType> DamageType = Request.DamageType ? Request.DamageType : TSubclassOf<UDamageType>(UDamageType_Bullet::StaticClass());
	Result.DamageDealt = UGameplayStatics::ApplyPointDamage(Request.Target, Damage, Request.ShotDirection, HitInfo,
		Request.InstigatorController, Request.DamageCauser, DamageType);

	if (Result.DamageDealt > 0.0f)
	{
		ApplyHitEffects(Request);
		ApplyLifeSteal(Request, Result.DamageDealt);
	}

	return Result;
}

void FZombieHitResolver::ApplyHitEffects(const FZombieHitRequest& Request)
{
	if (!Request.HitEffects || Request.HitEffects->Num() == 0)
	{
		return;
	}

	UStatusEffectComponent* Status = Request.Target->FindComponentByClass<UStatusEffectComponent>();
	if (!Status)
	{
		return;
	}

	for (const FHitEffectSpec& Effect : *Request.HitEffects)
	{
		if (Effect.StatusTag.IsValid() && FMath::FRand() < Effect.Chance)
		{
			Status->ApplyStatus(Effect.StatusTag, Effect.Potency, Request.InstigatorController, Request.DamageCauser);
		}
	}
}

void FZombieHitResolver::ApplyLifeSteal(const FZombieHitRequest& Request, float DamageDealt)
{
	APawn* Shooter = Request.InstigatorController ? Request.InstigatorController->GetPawn() : nullptr;
	if (!Shooter)
	{
		return;
	}

	const float LifeStealFraction = ZombieStats::Resolve(Shooter, ZombieTags::Stat_Hit_LifeSteal, 0.0f);
	if (LifeStealFraction <= 0.0f)
	{
		return;
	}

	if (UHealthComponent* ShooterHealth = Shooter->FindComponentByClass<UHealthComponent>())
	{
		ShooterHealth->Heal(DamageDealt * LifeStealFraction);
	}
}

int32 FZombieHitResolver::ApplyExplosion(UWorld* World, const FZombieExplosionRequest& Request)
{
	if (!World || Request.Radius <= 0.0f)
	{
		return 0;
	}

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(Overlaps, Request.Origin, FQuat::Identity,
		FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(Request.Radius));

	TSet<AActor*> Damaged;
	const AActor* Source = Request.DamageCauser;
	const ZombieTeams::ETeam SourceTeam = Request.SourceTeam != ZombieTeams::ETeam::None ? Request.SourceTeam : ZombieTeams::GetTeam(Source);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Target = Overlap.GetActor();
		if (!Target || Damaged.Contains(Target))
		{
			continue;
		}

		const ZombieTeams::ETeam TargetTeam = ZombieTeams::GetTeam(Target);
		if (!Request.bFriendlyFire && SourceTeam != ZombieTeams::ETeam::None && TargetTeam == SourceTeam)
		{
			continue;
		}

		// Walls shield: a blast doesn't reach through geometry.
		FCollisionQueryParams LineOfSight(SCENE_QUERY_STAT(ZombieExplosionLOS), false);
		LineOfSight.AddIgnoredActor(Target);
		if (Source)
		{
			LineOfSight.AddIgnoredActor(Source);
		}
		const FVector TargetCentre = Target->GetActorLocation();
		if (World->LineTraceTestByChannel(Request.Origin + FVector(0, 0, 40.0f), TargetCentre, ECC_WorldStatic, LineOfSight))
		{
			continue;
		}

		const float Distance = FVector::Dist2D(Request.Origin, TargetCentre);
		const float Falloff = FMath::Lerp(1.0f, Request.EdgeDamageFraction, FMath::Clamp(Distance / Request.Radius, 0.0f, 1.0f));

		FZombieHitRequest Hit;
		Hit.Target = Target;
		Hit.BaseDamage = Request.Damage * Falloff;
		Hit.DamageType = UDamageType_Explosion::StaticClass();
		Hit.InstigatorController = Request.InstigatorController;
		Hit.DamageCauser = Request.DamageCauser;
		Hit.HitLocation = TargetCentre;
		Hit.ShotDirection = (TargetCentre - Request.Origin).GetSafeNormal2D();
		Hit.HitEffects = Request.HitEffects;

		if (ApplyHit(Hit).DamageDealt > 0.0f)
		{
			Damaged.Add(Target);
		}
	}

	return Damaged.Num();
}
