#include "DamageComponent.h"
#include "Components/HealthComponent.h"
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

void UDamageComponent::HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
	if (Damage <= 0.0f)
	{
		return;
	}

	LastDamageInstigator = InstigatedBy;

	if (CachedHealthComponent)
	{
		CachedHealthComponent->ApplyDamage(Damage);
	}
}
