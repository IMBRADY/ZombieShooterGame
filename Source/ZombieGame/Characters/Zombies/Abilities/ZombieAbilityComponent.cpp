#include "ZombieAbilityComponent.h"
#include "Characters/Zombies/Abilities/ZombieAbility.h"
#include "Characters/Zombies/ZombieCharacter.h"
#include "Engine/World.h"

UZombieAbilityComponent::UZombieAbilityComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

AZombieCharacter* UZombieAbilityComponent::GetZombie() const
{
	return Cast<AZombieCharacter>(GetOwner());
}

void UZombieAbilityComponent::Initialize(const TArray<UZombieAbility*>& Templates, const TArray<float>& InPhaseThresholds)
{
	Abilities.Reset();
	NextReadyTimes.Reset();

	for (const UZombieAbility* Template : Templates)
	{
		if (Template)
		{
			// Each zombie gets its own copy: abilities may carry per-use state (a charge in flight).
			Abilities.Add(DuplicateObject<UZombieAbility>(Template, this));
		}
	}

	PhaseThresholds = InPhaseThresholds;
	PhaseThresholds.Sort(TGreater<float>());
	Phase = 0;
}

void UZombieAbilityComponent::NotifySpawned()
{
	const UWorld* World = GetWorld();
	AZombieCharacter* Zombie = GetZombie();
	if (!World || !Zombie)
	{
		return;
	}

	NextReadyTimes.SetNum(Abilities.Num());
	for (int32 Index = 0; Index < Abilities.Num(); ++Index)
	{
		NextReadyTimes[Index] = World->GetTimeSeconds() + Abilities[Index]->GetInitialDelay() * FMath::FRandRange(0.7f, 1.3f);
		Abilities[Index]->OnOwnerSpawned(*Zombie);
	}
}

void UZombieAbilityComponent::NotifyDied()
{
	if (AZombieCharacter* Zombie = GetZombie())
	{
		for (UZombieAbility* Ability : Abilities)
		{
			Ability->OnOwnerDied(*Zombie);
		}
	}
}

bool UZombieAbilityComponent::HasActivatableAbilities() const
{
	return Abilities.ContainsByPredicate([](const TObjectPtr<UZombieAbility>& Ability) { return Ability && Ability->IsActivatable(); });
}

UZombieAbility* UZombieAbilityComponent::FindReadyAbility(const AActor* Target) const
{
	const UWorld* World = GetWorld();
	const AZombieCharacter* Zombie = GetZombie();
	if (!World || !Zombie || NextReadyTimes.Num() != Abilities.Num())
	{
		return nullptr;
	}

	const double Now = World->GetTimeSeconds();
	for (int32 Index = 0; Index < Abilities.Num(); ++Index)
	{
		UZombieAbility* Ability = Abilities[Index];
		if (Ability && Now >= NextReadyTimes[Index] && Ability->CanActivate(*Zombie, Target))
		{
			return Ability;
		}
	}
	return nullptr;
}

float UZombieAbilityComponent::ActivateAbility(UZombieAbility* Ability, AActor* Target)
{
	AZombieCharacter* Zombie = GetZombie();
	const int32 Index = Abilities.IndexOfByKey(Ability);
	if (!Zombie || Index == INDEX_NONE || !GetWorld())
	{
		return 0.0f;
	}

	NextReadyTimes[Index] = GetWorld()->GetTimeSeconds() + Ability->GetCooldown();
	return Ability->Activate(*Zombie, Target);
}

void UZombieAbilityComponent::UpdatePhase(float HealthFraction)
{
	int32 NewPhase = 0;
	for (const float Threshold : PhaseThresholds)
	{
		if (HealthFraction <= Threshold)
		{
			++NewPhase;
		}
	}

	// Phases only ever advance - healing a boss (a necromancer's aura, say) doesn't reset its rage.
	if (NewPhase > Phase)
	{
		Phase = NewPhase;
		OnPhaseChanged.Broadcast(Phase);
	}
}
