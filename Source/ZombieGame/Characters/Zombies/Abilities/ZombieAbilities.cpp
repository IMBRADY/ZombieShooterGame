#include "ZombieAbilities.h"
#include "Audio/ZombieAudioSubsystem.h"
#include "Characters/Zombies/Abilities/ZombieHazardZone.h"
#include "Characters/Zombies/ZombieCharacter.h"
#include "Characters/Zombies/ZombieEnemyManager.h"
#include "Components/HealthComponent.h"
#include "Core/ZombieDamageTypes.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "Visual/ZombieEffectsSubsystem.h"
#include "Weapons/Projectile/ZombieProjectile.h"
#include "Weapons/ZombieHitResolver.h"

namespace
{
	void PlayBlast(AZombieCharacter& Owner, const FVector& Location, const FZombieEffectSpec& Effect, const FZombieEffectSpec& Decal,
		const FZombieSoundSpec& Sound, float Shake)
	{
		if (UZombieEffectsSubsystem* Effects = UZombieEffectsSubsystem::Get(&Owner))
		{
			Effects->PlayEffect(Effect, Location);
			Effects->SpawnDecal(Decal, FVector(Location.X, Location.Y, 0.0f));
		}
		if (UZombieAudioSubsystem* Audio = UZombieAudioSubsystem::Get(&Owner))
		{
			Audio->PlayCreatureSoundAtLocation(Sound, Location);
		}
		Owner.ShakeNearbyPlayers(Location, Shake);
	}
}

// --- Projectile ---------------------------------------------------------------------------------

FVector UZombieAbility_Projectile::ComputeLobVelocity(const AZombieCharacter& Owner, const FVector& Start, const FVector& Goal) const
{
	// Ballistic solve for a fixed flight time: horizontal speed covers the distance in T, vertical
	// speed makes gravity bring it down exactly on the goal.
	const float Gravity = Owner.GetWorld()->GetGravityZ() * FMath::Max(Projectile.GravityScale, 0.01f);
	const FVector Delta = Goal - Start;
	const float FlightTime = FMath::Clamp(Delta.Size2D() / FMath::Max(Projectile.Speed, 1.0f), 0.45f, 1.6f);

	FVector Velocity = Delta / FlightTime;
	Velocity.Z = (Delta.Z - 0.5f * Gravity * FlightTime * FlightTime) / FlightTime;
	return Velocity;
}

void UZombieAbility_Projectile::ActivateInternal(AZombieCharacter& Owner, AActor* Target)
{
	if (!Target)
	{
		return;
	}

	FZombieProjectileLaunch Launch;
	Launch.Spec = Projectile;
	Launch.Damage = Owner.GetAttackDamage() * DamageMultiplier;
	Launch.ExplosionRadius = ExplosionRadius;
	Launch.DamageType = UDamageType_Poison::StaticClass();
	Launch.HitEffects = HitEffects;
	Launch.InstigatorController = Owner.GetController();
	Launch.SourceActor = &Owner;

	const FVector Start = Owner.GetActorLocation() + Owner.GetActorForwardVector() * 50.0f;
	const FVector ToTarget = (Target->GetActorLocation() - Start).GetSafeNormal2D();

	for (int32 Shot = 0; Shot < Count; ++Shot)
	{
		const float Alpha = Count > 1 ? static_cast<float>(Shot) / (Count - 1) - 0.5f : 0.0f;
		const FVector Direction = ToTarget.RotateAngleAxis(Alpha * FanDegrees, FVector::UpVector);

		if (bLobbed)
		{
			const float Distance = FVector::Dist2D(Start, Target->GetActorLocation());
			FVector Goal = Start + Direction * Distance + FVector(FMath::FRandRange(-LobScatter, LobScatter), FMath::FRandRange(-LobScatter, LobScatter), 0.0f);
			Goal.Z = 0.0f;
			Launch.Velocity = ComputeLobVelocity(Owner, Start, Goal);
			Launch.Spec.Lifetime = 4.0f;
		}
		else
		{
			Launch.Velocity = Direction * Projectile.Speed;
		}

		AZombieProjectile::SpawnFromPool(Owner.GetWorld(), Start, Launch);
	}
}

// --- Explode ------------------------------------------------------------------------------------

void UZombieAbility_Explode::ActivateInternal(AZombieCharacter& Owner, AActor* Target)
{
	// Self-destruct: dying triggers the blast through OnOwnerDied, so both routes share one path.
	if (UHealthComponent* Health = Owner.FindComponentByClass<UHealthComponent>())
	{
		Health->ApplyDamage(Health->GetHealth() + Health->GetArmor() + 1.0f);
	}
}

void UZombieAbility_Explode::OnOwnerDied(AZombieCharacter& Owner)
{
	if (bExploded)
	{
		return;
	}
	bExploded = true;

	const FVector Origin = Owner.GetActorLocation();
	FZombieExplosionRequest Explosion;
	Explosion.Origin = Origin;
	Explosion.Radius = Radius;
	Explosion.Damage = ScaleDamage(Owner, Damage);
	Explosion.bFriendlyFire = true;
	Explosion.DamageCauser = &Owner;
	FZombieHitResolver::ApplyExplosion(Owner.GetWorld(), Explosion);

	PlayBlast(Owner, Origin, ExplosionEffect, ScorchDecal, ExplosionSound, 0.7f);
}

// --- Hazard trail -------------------------------------------------------------------------------

void UZombieAbility_HazardTrail::OnOwnerSpawned(AZombieCharacter& Owner)
{
	OwnerZombie = &Owner;
	if (TrailHazard)
	{
		Owner.GetWorldTimerManager().SetTimer(TrailTimer, FTimerDelegate::CreateUObject(this, &UZombieAbility_HazardTrail::DropTrail),
			TrailInterval, true, FMath::FRandRange(0.5f, TrailInterval));
	}
}

void UZombieAbility_HazardTrail::DropTrail()
{
	AZombieCharacter* Owner = OwnerZombie.Get();
	if (!Owner || Owner->IsDead())
	{
		return;
	}

	// Only while actually moving - a zombie standing still shouldn't carpet one tile.
	if (Owner->GetVelocity().SizeSquared2D() > FMath::Square(40.0f))
	{
		AZombieHazardZone::SpawnHazard(Owner->GetWorld(), TrailHazard, Owner->GetActorLocation(), Owner);
	}
}

void UZombieAbility_HazardTrail::OnOwnerDied(AZombieCharacter& Owner)
{
	Owner.GetWorldTimerManager().ClearTimer(TrailTimer);
	AZombieHazardZone::SpawnHazard(Owner.GetWorld(), DeathHazard, Owner.GetActorLocation(), &Owner);
}

// --- Revive -------------------------------------------------------------------------------------

bool UZombieAbility_Revive::CanActivateInternal(const AZombieCharacter& Owner, const AActor* Target) const
{
	const UZombieEnemyManager* Enemies = UZombieEnemyManager::Get(&Owner);
	return Enemies && Enemies->HasCorpseNear(Owner.GetActorLocation(), ReviveRadius);
}

void UZombieAbility_Revive::ActivateInternal(AZombieCharacter& Owner, AActor* Target)
{
	if (UZombieEnemyManager* Enemies = UZombieEnemyManager::Get(&Owner))
	{
		Enemies->ReviveCorpsesNear(Owner.GetActorLocation(), ReviveRadius, MaxRevivedPerCast);
	}
}

// --- Charge -------------------------------------------------------------------------------------

void UZombieAbility_Charge::ActivateInternal(AZombieCharacter& Owner, AActor* Target)
{
	if (!Target)
	{
		return;
	}

	OwnerZombie = &Owner;
	ChargeTarget = Target;
	ChargeDirection = (Target->GetActorLocation() - Owner.GetActorLocation()).GetSafeNormal2D();
	bHitLanded = false;

	Owner.GetCharacterMovement()->StopMovementImmediately();
	Owner.GetWorldTimerManager().SetTimer(PhaseTimer, FTimerDelegate::CreateUObject(this, &UZombieAbility_Charge::Launch), FMath::Max(Windup, 0.01f), false);
}

void UZombieAbility_Charge::Launch()
{
	AZombieCharacter* Owner = OwnerZombie.Get();
	if (!Owner || Owner->IsDead())
	{
		return;
	}

	// Frictionless for the dash so the launch carries, restored when it ends.
	UCharacterMovementComponent* Movement = Owner->GetCharacterMovement();
	SavedGroundFriction = Movement->GroundFriction;
	SavedBraking = Movement->BrakingDecelerationWalking;
	Movement->GroundFriction = 0.0f;
	Movement->BrakingDecelerationWalking = 0.0f;
	Owner->LaunchCharacter(ChargeDirection * ChargeSpeed, true, false);

	const float DashTime = FMath::Max(BusyDuration - Windup, 0.1f);
	Owner->GetWorldTimerManager().SetTimer(ContactTimer, FTimerDelegate::CreateUObject(this, &UZombieAbility_Charge::CheckContact), 0.05f, true);
	Owner->GetWorldTimerManager().SetTimer(PhaseTimer, FTimerDelegate::CreateUObject(this, &UZombieAbility_Charge::EndCharge), DashTime, false);
}

void UZombieAbility_Charge::CheckContact()
{
	AZombieCharacter* Owner = OwnerZombie.Get();
	AActor* Target = ChargeTarget.Get();
	if (!Owner || !Target || bHitLanded)
	{
		return;
	}

	if (FVector::DistSquared2D(Owner->GetActorLocation(), Target->GetActorLocation()) > FMath::Square(Owner->GetAttackRange()))
	{
		return;
	}

	bHitLanded = true;
	FZombieHitRequest Hit;
	Hit.Target = Target;
	Hit.BaseDamage = ScaleDamage(*Owner, Damage);
	Hit.DamageType = UDamageType_Melee::StaticClass();
	Hit.InstigatorController = Owner->GetController();
	Hit.DamageCauser = Owner;
	Hit.HitLocation = Target->GetActorLocation();
	Hit.ShotDirection = ChargeDirection;
	FZombieHitResolver::ApplyHit(Hit);

	if (ACharacter* Victim = Cast<ACharacter>(Target))
	{
		Victim->LaunchCharacter(ChargeDirection * Knockback + FVector(0.0f, 0.0f, 150.0f), true, true);
	}
	EndCharge();
}

void UZombieAbility_Charge::EndCharge()
{
	AZombieCharacter* Owner = OwnerZombie.Get();
	if (!Owner)
	{
		return;
	}

	Owner->GetWorldTimerManager().ClearTimer(ContactTimer);
	Owner->GetWorldTimerManager().ClearTimer(PhaseTimer);

	UCharacterMovementComponent* Movement = Owner->GetCharacterMovement();
	Movement->GroundFriction = SavedGroundFriction;
	Movement->BrakingDecelerationWalking = SavedBraking;
	Movement->StopMovementImmediately();
}

// --- Slam ---------------------------------------------------------------------------------------

void UZombieAbility_Slam::ActivateInternal(AZombieCharacter& Owner, AActor* Target)
{
	OwnerZombie = &Owner;
	Owner.GetCharacterMovement()->StopMovementImmediately();
	Owner.GetWorldTimerManager().SetTimer(StrikeTimer, FTimerDelegate::CreateUObject(this, &UZombieAbility_Slam::Strike), FMath::Max(Windup, 0.01f), false);
}

void UZombieAbility_Slam::Strike()
{
	AZombieCharacter* Owner = OwnerZombie.Get();
	if (!Owner || Owner->IsDead())
	{
		return;
	}

	FZombieExplosionRequest Blast;
	Blast.Origin = Owner->GetActorLocation();
	Blast.Radius = Radius;
	Blast.Damage = ScaleDamage(*Owner, Damage);
	Blast.EdgeDamageFraction = 0.6f;
	Blast.DamageCauser = Owner;
	Blast.SourceTeam = ZombieTeams::ETeam::Zombies;
	FZombieHitResolver::ApplyExplosion(Owner->GetWorld(), Blast);

	PlayBlast(*Owner, Owner->GetActorLocation(), ImpactEffect, FZombieEffectSpec(), ImpactSound, 0.6f);
}

// --- Summon -------------------------------------------------------------------------------------

bool UZombieAbility_Summon::CanActivateInternal(const AZombieCharacter& Owner, const AActor* Target) const
{
	int32 Alive = 0;
	for (const TWeakObjectPtr<AActor>& Minion : Minions)
	{
		const AZombieCharacter* Zombie = Cast<AZombieCharacter>(Minion.Get());
		Alive += (Zombie && !Zombie->IsDead()) ? 1 : 0;
	}
	return MinionArchetype && Alive < MaxAliveMinions;
}

void UZombieAbility_Summon::ActivateInternal(AZombieCharacter& Owner, AActor* Target)
{
	UZombieEnemyManager* Enemies = UZombieEnemyManager::Get(&Owner);
	if (!Enemies || !MinionArchetype)
	{
		return;
	}

	Minions.RemoveAll([](const TWeakObjectPtr<AActor>& Minion) { return !Minion.IsValid(); });

	FZombieSpawnOptions Options;
	Options.bGrantsRewards = false;
	for (int32 Index = 0; Index < CountPerCast; ++Index)
	{
		const FVector Offset = FVector(FMath::FRandRange(-1.0f, 1.0f), FMath::FRandRange(-1.0f, 1.0f), 0.0f).GetSafeNormal() * FMath::FRandRange(180.0f, 320.0f);
		if (AZombieCharacter* Minion = Enemies->SpawnZombie(MinionArchetype, Owner.GetActorLocation() + Offset, Options))
		{
			Minion->PlayRiseEffect();
			Minions.Add(Minion);
		}
	}
}
