#include "HealthComponent.h"
#include "Net/UnrealNetwork.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	Health = MaxHealth;
	Armor = 0.0f;
	bIsDead = false;
}

void UHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHealthComponent, Health);
	DOREPLIFETIME(UHealthComponent, Armor);
}

float UHealthComponent::ApplyDamage(float DamageAmount)
{
	if (DamageAmount <= 0.0f || bIsDead)
	{
		return 0.0f;
	}

	float RemainingDamage = DamageAmount;

	if (Armor > 0.0f)
	{
		const float ArmorAbsorbed = FMath::Min(Armor, RemainingDamage);
		Armor -= ArmorAbsorbed;
		RemainingDamage -= ArmorAbsorbed;
		OnArmorChanged.Broadcast(Armor, MaxArmor);
	}

	if (RemainingDamage > 0.0f)
	{
		const float OldHealth = Health;
		Health = FMath::Max(0.0f, Health - RemainingDamage);
		OnHealthChanged.Broadcast(Health, MaxHealth, Health - OldHealth);

		if (Health <= 0.0f && !bIsDead)
		{
			bIsDead = true;
			OnDeath.Broadcast();
		}
	}

	return RemainingDamage;
}

void UHealthComponent::SetMaxHealth(float NewMaxHealth, bool bRefill)
{
	MaxHealth = FMath::Max(NewMaxHealth, 1.0f);

	const float OldHealth = Health;
	Health = bRefill ? MaxHealth : FMath::Min(Health, MaxHealth);

	if (Health > 0.0f)
	{
		bIsDead = false;
	}

	OnHealthChanged.Broadcast(Health, MaxHealth, Health - OldHealth);
}

void UHealthComponent::AddArmor(float Amount)
{
	if (Amount <= 0.0f)
	{
		return;
	}

	Armor = FMath::Clamp(Armor + Amount, 0.0f, MaxArmor);
	OnArmorChanged.Broadcast(Armor, MaxArmor);
}

void UHealthComponent::Heal(float Amount)
{
	if (Amount <= 0.0f || bIsDead)
	{
		return;
	}

	const float OldHealth = Health;
	Health = FMath::Clamp(Health + Amount, 0.0f, MaxHealth);
	OnHealthChanged.Broadcast(Health, MaxHealth, Health - OldHealth);
}

void UHealthComponent::OnRep_Health()
{
	OnHealthChanged.Broadcast(Health, MaxHealth, 0.0f);
}

void UHealthComponent::OnRep_Armor()
{
	OnArmorChanged.Broadcast(Armor, MaxArmor);
}
