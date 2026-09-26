#include "DamageComponent.h"
#include "Components/HealthComponent.h"
#include "Core/ZombieDamageTypes.h"
#include "Core/ZombieGameplayTags.h"
#include "Core/ZombieStatSource.h"
#include "GameFramework/Actor.h"

UDamageComponent::UDamageComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDamageComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AActor* Owner = GetOwner())
	{
		CachedHealthComponent = Owner->FindComponentByClass<UHealthComponent>();
		Owner->OnTakeAnyDamage.AddDynamic(this, &UDamageComponent::HandleTakeAnyDamage);
	}
}

void UDamageComponent::SetResistance(const FGameplayTag& DamageTag, float Fraction)
{
	if (DamageTag.IsValid())
	{
		Resistances.Add(DamageTag, FMath::Clamp(Fraction, 0.0f, 1.0f));
	}
}

float UDamageComponent::GetResistanceFor(const UDamageType* DamageType) const
{
	const FGameplayTag DamageTag = UZombieDamageType::GetTagFor(DamageType);
	if (!DamageTag.IsValid())
	{
		return 0.0f;
	}

	float Resistance = Resistances.FindRef(DamageTag);

	// Players also resist through perks; the stat is keyed by damage kind.
	if (DamageTag.MatchesTagExact(ZombieTags::Damage_Explosion))
	{
		Resistance += ZombieStats::Resolve(GetOwner(), ZombieTags::Stat_Resist_Explosion, 0.0f);
	}
	else if (DamageTag.MatchesTagExact(ZombieTags::Damage_Poison))
	{
		Resistance += ZombieStats::Resolve(GetOwner(), ZombieTags::Stat_Resist_Poison, 0.0f);
	}

	return FMath::Clamp(Resistance, 0.0f, 0.9f);
}

void UDamageComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (Damage <= 0.0f || !CachedHealthComponent || CachedHealthComponent->IsDead())
	{
		return;
	}

	const float FinalDamage = Damage * (1.0f - GetResistanceFor(DamageType)) * IncomingDamageMultiplier;
	if (FinalDamage <= 0.0f)
	{
		return;
	}

	// Environmental ticks (a puddle nobody owns) must not steal the kill from whoever did the work.
	if (InstigatedBy)
	{
		LastDamageInstigator = InstigatedBy;
	}
	if (DamageCauser)
	{
		LastDamageCauser = DamageCauser;
	}

	CachedHealthComponent->ApplyDamage(FinalDamage);
	OnDamageReceived.Broadcast(FinalDamage, DamageCauser, DamageType);
}
