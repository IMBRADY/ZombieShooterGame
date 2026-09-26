#include "ZombieProjectile.h"
#include "Audio/ZombieAudioSubsystem.h"
#include "Characters/Zombies/Abilities/ZombieHazardZone.h"
#include "Components/SphereComponent.h"
#include "Core/ZombieDamageTypes.h"
#include "Core/ZombieTeams.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "TimerManager.h"
#include "Utilities/ActorPoolSubsystem.h"
#include "Visual/PixelSpriteComponent.h"
#include "Visual/ZombieEffectsSubsystem.h"
#include "Weapons/ZombieHitResolver.h"

AZombieProjectile::AZombieProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(14.0f);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Collision->SetGenerateOverlapEvents(true);
	Collision->SetCanEverAffectNavigation(false);
	SetRootComponent(Collision);

	Sprite = CreateDefaultSubobject<UPixelSpriteComponent>(TEXT("Sprite"));
	Sprite->SetupAttachment(Collision);

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->UpdatedComponent = Collision;
	Movement->bRotationFollowsVelocity = true;
	Movement->ProjectileGravityScale = 0.0f;
	Movement->bAutoActivate = false;
	Movement->Bounciness = 0.9f;
	Movement->Friction = 0.0f;
}

AZombieProjectile* AZombieProjectile::SpawnFromPool(UWorld* World, const FVector& Location, const FZombieProjectileLaunch& Launch)
{
	UActorPoolSubsystem* Pool = UActorPoolSubsystem::Get(World);
	if (!Pool)
	{
		return nullptr;
	}

	AActor* Source = Launch.SourceActor.Get();
	APawn* InstigatorPawn = Source ? Source->GetInstigator() : nullptr;
	AZombieProjectile* Projectile = Pool->Acquire<AZombieProjectile>(AZombieProjectile::StaticClass(),
		FTransform(Launch.Velocity.Rotation(), Location), Source, InstigatorPawn);

	if (Projectile)
	{
		Projectile->Launch(Launch);
	}
	return Projectile;
}

void AZombieProjectile::OnAcquiredFromPool()
{
	AlreadyHit.Reset();
}

void AZombieProjectile::Launch(const FZombieProjectileLaunch& InLaunch)
{
	LaunchData = InLaunch;
	PierceRemaining = InLaunch.Pierce;
	BouncesRemaining = InLaunch.Ricochet;
	SourceTeam = ZombieTeams::GetTeam(InLaunch.SourceActor.Get());
	bActive = true;

	Collision->SetSphereRadius(InLaunch.Spec.CollisionRadius);
	Collision->OnComponentBeginOverlap.AddUniqueDynamic(this, &AZombieProjectile::HandleOverlap);
	Movement->OnProjectileStop.AddUniqueDynamic(this, &AZombieProjectile::HandleStop);
	Movement->OnProjectileBounce.AddUniqueDynamic(this, &AZombieProjectile::HandleBounce);

	Sprite->SetSpriteSheet(InLaunch.Spec.Sprite, InLaunch.Spec.SpriteScale);
	Sprite->PlayAnimation(TEXT("Play"), true);

	Movement->SetUpdatedComponent(Collision);
	Movement->ProjectileGravityScale = InLaunch.Spec.GravityScale;
	Movement->bShouldBounce = BouncesRemaining > 0;
	Movement->MaxSpeed = FMath::Max(InLaunch.Velocity.Size(), InLaunch.Spec.Speed);
	Movement->Velocity = InLaunch.Velocity;
	Movement->Activate(true);

	GetWorldTimerManager().SetTimer(LifetimeTimer, this, &AZombieProjectile::Expire, FMath::Max(InLaunch.Spec.Lifetime, 0.1f), false);
}

void AZombieProjectile::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Team is captured at launch: a thrower that dies mid-flight must not turn its acid friendly.
	const ZombieTeams::ETeam OtherTeam = ZombieTeams::GetTeam(OtherActor);
	if (!bActive || !OtherActor || AlreadyHit.Contains(OtherActor) || OtherTeam == ZombieTeams::ETeam::None || OtherTeam == SourceTeam)
	{
		return;
	}

	AlreadyHit.Add(OtherActor);

	// Explosive rounds burst on the first enemy; ordinary rounds damage it and maybe keep going.
	if (LaunchData.ExplosionRadius > 0.0f || PierceRemaining <= 0)
	{
		Detonate(GetActorLocation(), OtherActor);
		return;
	}

	--PierceRemaining;
	DamageTarget(OtherActor, GetActorLocation());
}

void AZombieProjectile::HandleBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
	if (--BouncesRemaining <= 0)
	{
		Movement->bShouldBounce = false;
	}

	if (UZombieEffectsSubsystem* Effects = UZombieEffectsSubsystem::Get(this))
	{
		Effects->PlayEffect(LaunchData.Spec.ImpactEffect, ImpactResult.ImpactPoint);
	}
}

void AZombieProjectile::HandleStop(const FHitResult& ImpactResult)
{
	if (bActive)
	{
		Detonate(ImpactResult.ImpactPoint, nullptr);
	}
}

void AZombieProjectile::DamageTarget(AActor* Target, const FVector& Location)
{
	FZombieHitRequest Request;
	Request.Target = Target;
	Request.BaseDamage = LaunchData.Damage;
	Request.CritChance = LaunchData.CritChance;
	Request.CritMultiplier = LaunchData.CritMultiplier;
	Request.DamageType = LaunchData.DamageType;
	Request.InstigatorController = LaunchData.InstigatorController.Get();
	Request.DamageCauser = LaunchData.SourceActor.Get();
	Request.HitLocation = Location;
	Request.ShotDirection = Movement->Velocity.GetSafeNormal2D();
	Request.HitEffects = &LaunchData.HitEffects;
	FZombieHitResolver::ApplyHit(Request);
}

void AZombieProjectile::Detonate(const FVector& Location, AActor* DirectHit)
{
	bActive = false;
	UWorld* World = GetWorld();

	if (LaunchData.ExplosionRadius > 0.0f)
	{
		FZombieExplosionRequest Explosion;
		Explosion.Origin = Location;
		Explosion.Radius = LaunchData.ExplosionRadius;
		Explosion.Damage = LaunchData.Damage;
		Explosion.bFriendlyFire = LaunchData.bFriendlyFire;
		Explosion.InstigatorController = LaunchData.InstigatorController.Get();
		Explosion.DamageCauser = LaunchData.SourceActor.Get();
		Explosion.SourceTeam = SourceTeam;
		Explosion.HitEffects = &LaunchData.HitEffects;
		FZombieHitResolver::ApplyExplosion(World, Explosion);
	}
	else if (DirectHit)
	{
		DamageTarget(DirectHit, Location);
	}

	if (UZombieEffectsSubsystem* Effects = UZombieEffectsSubsystem::Get(this))
	{
		Effects->PlayEffect(LaunchData.Spec.ImpactEffect, Location);
		Effects->SpawnDecal(LaunchData.Spec.ImpactDecal, FVector(Location.X, Location.Y, 0.0f));
	}

	if (UZombieAudioSubsystem* Audio = UZombieAudioSubsystem::Get(this))
	{
		Audio->PlaySoundAtLocation(LaunchData.Spec.ImpactSound, Location);
	}

	if (World && LaunchData.Spec.ImpactHazard)
	{
		AZombieHazardZone::SpawnHazard(World, LaunchData.Spec.ImpactHazard, Location, LaunchData.SourceActor.Get());
	}

	ReturnToPool();
}

void AZombieProjectile::Expire()
{
	if (!bActive)
	{
		return;
	}

	// Explosives go off at the end of their flight; plain rounds just fizzle.
	if (LaunchData.ExplosionRadius > 0.0f || LaunchData.Spec.GravityScale > 0.0f)
	{
		Detonate(GetActorLocation(), nullptr);
		return;
	}

	bActive = false;
	ReturnToPool();
}

void AZombieProjectile::ReturnToPool()
{
	if (UActorPoolSubsystem* Pool = UActorPoolSubsystem::Get(this))
	{
		Pool->Release(this);
	}
	else
	{
		Destroy();
	}
}

void AZombieProjectile::OnReleasedToPool()
{
	bActive = false;
	GetWorldTimerManager().ClearTimer(LifetimeTimer);
	Movement->StopMovementImmediately();
	Movement->Deactivate();
	LaunchData = FZombieProjectileLaunch();
}
