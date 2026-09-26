#include "StatusEffectComponent.h"
#include "Components/StatusEffectDataAsset.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Visual/ZombieEffectsSubsystem.h"

UStatusEffectComponent::UStatusEffectComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UStatusEffectComponent::ApplyStatus(const FGameplayTag& StatusTag, float Potency, AController* Instigator, AActor* Causer)
{
	const UStatusEffectDataAsset* Definition = UStatusEffectDataAsset::FindByTag(StatusTag);
	UWorld* World = GetWorld();
	if (!Definition || !World || !GetOwner() || GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	FActiveStatusEffect* Existing = ActiveEffects.FindByPredicate([Definition](const FActiveStatusEffect& Effect)
	{
		return Effect.Definition == Definition;
	});

	if (Existing)
	{
		Existing->Stacks = FMath::Min(Existing->Stacks + 1, FMath::Max(Definition->MaxStacks, 1));
		Existing->Potency = FMath::Max(Existing->Potency, Potency);
		Existing->RemainingTime = Definition->Duration;
		Existing->Instigator = Instigator;
		Existing->Causer = Causer;
	}
	else
	{
		FActiveStatusEffect& Added = ActiveEffects.AddDefaulted_GetRef();
		Added.Definition = Definition;
		Added.Potency = Potency;
		Added.RemainingTime = Definition->Duration;
		Added.Instigator = Instigator;
		Added.Causer = Causer;
	}

	if (!World->GetTimerManager().IsTimerActive(TickTimer))
	{
		World->GetTimerManager().SetTimer(TickTimer, this, &UStatusEffectComponent::TickEffects, TickInterval, true);
	}

	OnStatusEffectsChanged.Broadcast();
}

void UStatusEffectComponent::TickEffects()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// Iterate over a copy: applying damage can kill the owner, which clears the effects mid-loop.
	const TArray<FActiveStatusEffect> Snapshot = ActiveEffects;
	for (const FActiveStatusEffect& Effect : Snapshot)
	{
		if (!Effect.Definition)
		{
			continue;
		}

		const float Damage = Effect.Definition->DamagePerSecond * Effect.Potency * Effect.Stacks * TickInterval;
		if (Damage > 0.0f)
		{
			UGameplayStatics::ApplyDamage(Owner, Damage, Effect.Instigator.Get(), Effect.Causer.Get(), Effect.Definition->DamageType);
		}

		if (UZombieEffectsSubsystem* Effects = UZombieEffectsSubsystem::Get(this))
		{
			Effects->PlayEffect(Effect.Definition->TickEffect, Owner->GetActorLocation());
		}
	}

	const int32 CountBefore = ActiveEffects.Num();
	for (FActiveStatusEffect& Effect : ActiveEffects)
	{
		Effect.RemainingTime -= TickInterval;
	}
	ActiveEffects.RemoveAll([](const FActiveStatusEffect& Effect) { return Effect.RemainingTime <= 0.0f || !Effect.Definition; });

	if (ActiveEffects.Num() == 0 && GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(TickTimer);
	}

	if (ActiveEffects.Num() != CountBefore)
	{
		OnStatusEffectsChanged.Broadcast();
	}
}

void UStatusEffectComponent::ClearAll()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(TickTimer);
	}

	if (ActiveEffects.Num() > 0)
	{
		ActiveEffects.Reset();
		OnStatusEffectsChanged.Broadcast();
	}
}

float UStatusEffectComponent::GetMoveSpeedMultiplier() const
{
	float Multiplier = 1.0f;
	for (const FActiveStatusEffect& Effect : ActiveEffects)
	{
		Multiplier *= Effect.Definition ? Effect.Definition->MoveSpeedMultiplier : 1.0f;
	}
	return Multiplier;
}

FLinearColor UStatusEffectComponent::GetDisplayTint() const
{
	return (ActiveEffects.Num() > 0 && ActiveEffects.Last().Definition) ? ActiveEffects.Last().Definition->Tint : FLinearColor::White;
}

void UStatusEffectComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(TickTimer);
	}
	Super::EndPlay(EndPlayReason);
}
