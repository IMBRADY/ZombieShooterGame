#include "ZombieAbility.h"
#include "Audio/ZombieAudioSubsystem.h"
#include "Characters/Zombies/Abilities/ZombieAbilityComponent.h"
#include "Characters/Zombies/ZombieCharacter.h"
#include "CollisionQueryParams.h"
#include "Engine/World.h"
#include "Visual/ZombieEffectsSubsystem.h"

bool UZombieAbility::CanActivate(const AZombieCharacter& Owner, const AActor* Target) const
{
	if (!IsActivatable() || Owner.IsDead())
	{
		return false;
	}

	const UZombieAbilityComponent* Abilities = Owner.GetAbilityComponent();
	if (Abilities && Abilities->GetPhase() < MinPhase)
	{
		return false;
	}

	if (bRequiresTarget)
	{
		if (!Target)
		{
			return false;
		}

		const float Distance = FVector::Dist2D(Owner.GetActorLocation(), Target->GetActorLocation());
		if (Distance < MinRange || Distance > MaxRange)
		{
			return false;
		}

		if (bRequiresLineOfSight)
		{
			FCollisionQueryParams Params(SCENE_QUERY_STAT(ZombieAbilityLOS), false, &Owner);
			Params.AddIgnoredActor(Target);
			if (Owner.GetWorld()->LineTraceTestByChannel(Owner.GetActorLocation(), Target->GetActorLocation(), ECC_WorldStatic, Params))
			{
				return false;
			}
		}
	}

	return CanActivateInternal(Owner, Target);
}

float UZombieAbility::Activate(AZombieCharacter& Owner, AActor* Target)
{
	if (Target)
	{
		FVector Facing = Target->GetActorLocation() - Owner.GetActorLocation();
		Facing.Z = 0.0f;
		if (!Facing.IsNearlyZero())
		{
			Owner.SetActorRotation(Facing.Rotation());
		}
	}

	Owner.PlayActionAnimation(Animation, BusyDuration);

	if (UZombieAudioSubsystem* Audio = UZombieAudioSubsystem::Get(&Owner))
	{
		Audio->PlaySoundAtLocation(Sound, Owner.GetActorLocation());
	}
	if (UZombieEffectsSubsystem* Effects = UZombieEffectsSubsystem::Get(&Owner))
	{
		Effects->PlayEffect(CastEffect, Owner.GetActorLocation());
	}

	ActivateInternal(Owner, Target);
	return BusyDuration;
}

float UZombieAbility::ScaleDamage(const AZombieCharacter& Owner, float BaseDamage)
{
	return BaseDamage * Owner.GetDamageMultiplier();
}
