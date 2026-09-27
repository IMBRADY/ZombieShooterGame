#include "WeaponFireMode_Hitscan.h"
#include "CollisionQueryParams.h"
#include "Core/ZombieDamageTypes.h"
#include "Core/ZombieTeams.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Visual/ZombieEffectsSubsystem.h"
#include "Weapons/WeaponDataAsset.h"
#include "Weapons/ZombieHitResolver.h"
#include "Weapons/ZombieWeapon.h"

namespace
{
	/** Safety cap on segments per pellet (pierces + bounces + friendly pass-throughs). */
	constexpr int32 MaxTraceSegments = 24;

	/** How far from a bounce point a ricochet will look for an enemy to redirect toward. */
	constexpr float RicochetSeekRadius = 900.0f;

	/**
	 * One straight stretch of a round's flight. Walls and cover stop rounds; pawns take them;
	 * pickups and triggers (WorldDynamic) don't. Walls are tested with a thin line so rounds still
	 * thread gaps and graze corners, bodies with a sphere of BodyRadius so what the player sees hit
	 * a zombie's sprite actually hits it.
	 */
	bool TraceShotSegment(const UWorld& World, const FVector& Start, const FVector& End, const FCollisionQueryParams& Params,
		float BodyRadius, FHitResult& OutHit)
	{
		FHitResult WallHit;
		const bool bHitWall = World.LineTraceSingleByObjectType(WallHit, Start, End, FCollisionObjectQueryParams(ECC_WorldStatic), Params);
		const FVector BodyTraceEnd = bHitWall ? WallHit.Location : End;

		FHitResult BodyHit;
		const FCollisionObjectQueryParams Bodies(ECC_Pawn);
		const bool bHitBody = BodyRadius > 0.0f
			? World.SweepSingleByObjectType(BodyHit, Start, BodyTraceEnd, FQuat::Identity, Bodies, FCollisionShape::MakeSphere(BodyRadius), Params)
			: World.LineTraceSingleByObjectType(BodyHit, Start, BodyTraceEnd, Bodies, Params);

		if (bHitBody)
		{
			OutHit = BodyHit;
			return true;
		}
		if (bHitWall)
		{
			OutHit = WallHit;
			return true;
		}
		return false;
	}
}

void UWeaponFireMode_Hitscan::Fire(const FWeaponFireContext& Context) const
{
	if (!Context.Weapon)
	{
		return;
	}

	for (int32 Pellet = 0; Pellet < FMath::Max(Context.Stats.PelletsPerShot, 1); ++Pellet)
	{
		FirePellet(Context, ApplySpread(Context.Direction, Context.Stats.SpreadDegrees));
	}
}

void UWeaponFireMode_Hitscan::FirePellet(const FWeaponFireContext& Context, const FVector& Direction) const
{
	UWorld* World = Context.Weapon->GetWorld();
	UZombieEffectsSubsystem* Effects = UZombieEffectsSubsystem::Get(World);
	if (!World)
	{
		return;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(ZombieHitscan), false);
	Params.AddIgnoredActor(Context.Weapon);
	Params.AddIgnoredActor(Context.InstigatorPawn);
	const float BodyRadius = Context.Definition->ShotHitRadius;
	const AZombieWeapon& Weapon = *Context.Weapon;

	FVector SegmentStart = Context.Origin;
	FVector TracerStart = Context.Origin;
	FVector ShotDirection = Direction;
	float RemainingRange = Context.Stats.Range;
	int32 PierceLeft = Context.Stats.Pierce;
	int32 BouncesLeft = Context.Stats.Ricochet;
	TArray<AActor*> AlreadyHit;

	for (int32 Segment = 0; Segment < MaxTraceSegments && RemainingRange > 1.0f; ++Segment)
	{
		const FVector SegmentEnd = SegmentStart + ShotDirection * RemainingRange;
		FHitResult Hit;
		if (!TraceShotSegment(*World, SegmentStart, SegmentEnd, Params, BodyRadius, Hit))
		{
			if (Effects && Context.Definition->bDrawTracers)
			{
				Effects->SpawnTracer(Weapon.ToVisualShotHeight(TracerStart), Weapon.ToVisualShotHeight(SegmentEnd), Context.TracerColor,
					Context.Definition->TracerWidth, 0.12f);
			}
			break;
		}

		RemainingRange -= Hit.Distance;
		AActor* HitActor = Hit.GetActor();

		if (Cast<APawn>(HitActor))
		{
			Params.AddIgnoredActor(HitActor);
			SegmentStart = Hit.Location;

			// Rounds pass through friendlies untouched rather than being wasted on them.
			if (!ZombieTeams::AreEnemies(Context.InstigatorPawn, HitActor))
			{
				continue;
			}

			FZombieHitRequest Request;
			Request.Target = HitActor;
			Request.BaseDamage = Context.Stats.Damage;
			Request.CritChance = Context.Stats.CritChance;
			Request.CritMultiplier = Context.Stats.CritMultiplier;
			Request.DamageType = UDamageType_Bullet::StaticClass();
			Request.InstigatorController = Context.InstigatorController;
			Request.DamageCauser = Context.Weapon;
			Request.HitLocation = Hit.Location;
			Request.ShotDirection = ShotDirection;
			Request.HitEffects = &Context.HitEffects;
			FZombieHitResolver::ApplyHit(Request);
			AlreadyHit.Add(HitActor);

			if (PierceLeft-- > 0)
			{
				continue;
			}
		}
		else if (Effects)
		{
			Effects->PlayEffect(Context.Definition->ImpactEffect, Weapon.ToVisualShotHeight(Hit.ImpactPoint));
		}

		if (Effects && Context.Definition->bDrawTracers)
		{
			Effects->SpawnTracer(Weapon.ToVisualShotHeight(TracerStart), Weapon.ToVisualShotHeight(Hit.Location), Context.TracerColor,
				Context.Definition->TracerWidth, 0.12f);
		}

		// Only walls bounce a round; a round that stopped in an enemy is spent.
		if (Cast<APawn>(HitActor) || BouncesLeft-- <= 0)
		{
			break;
		}

		const FVector Normal = FVector(Hit.ImpactNormal.X, Hit.ImpactNormal.Y, 0.0f).GetSafeNormal();
		const FVector Reflected = FMath::GetReflectionVector(ShotDirection, Normal).GetSafeNormal2D();
		SegmentStart = Hit.Location + Normal * 4.0f;
		TracerStart = SegmentStart;
		ShotDirection = FindRicochetDirection(Context, SegmentStart, Reflected, AlreadyHit);
	}
}

FVector UWeaponFireMode_Hitscan::FindRicochetDirection(const FWeaponFireContext& Context, const FVector& BouncePoint,
	const FVector& ReflectedDirection, const TArray<AActor*>& AlreadyHit) const
{
	UWorld* World = Context.Weapon->GetWorld();

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(Overlaps, BouncePoint, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(RicochetSeekRadius));

	FVector Best = ReflectedDirection;
	float BestDistanceSquared = TNumericLimits<float>::Max();

	for (const FOverlapResult& Overlap : Overlaps)
	{
		const AActor* Candidate = Overlap.GetActor();
		if (!Candidate || AlreadyHit.Contains(Candidate) || !ZombieTeams::AreEnemies(Context.InstigatorPawn, Candidate))
		{
			continue;
		}

		const FVector ToCandidate = Candidate->GetActorLocation() - BouncePoint;
		const float DistanceSquared = ToCandidate.SizeSquared2D();

		// Only targets on the reflected side of the wall, and visible from the bounce point.
		if (DistanceSquared >= BestDistanceSquared || FVector::DotProduct(ToCandidate.GetSafeNormal2D(), ReflectedDirection) < 0.2f)
		{
			continue;
		}

		FCollisionQueryParams Params(SCENE_QUERY_STAT(ZombieRicochetLOS), false);
		Params.AddIgnoredActor(Candidate);
		if (World->LineTraceTestByChannel(BouncePoint, Candidate->GetActorLocation(), ECC_WorldStatic, Params))
		{
			continue;
		}

		BestDistanceSquared = DistanceSquared;
		Best = ToCandidate.GetSafeNormal2D();
	}

	return Best;
}
