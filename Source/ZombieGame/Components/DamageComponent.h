#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DamageComponent.generated.h"

class UHealthComponent;
class UDamageType;
class AController;

// Routes AActor::TakeDamage (as broadcast via OnTakeAnyDamage) to the owner's HealthComponent.
// Keeps "how damage arrives" separate from "how health/armor respond to it", so later damage
// modifiers (crits, armor-piercing, elemental types) plug in here without touching HealthComponent.
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ZOMBIEGAME_API UDamageComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDamageComponent();

	/**
	 * Controller responsible for the most recent damage this actor took, or null if it was not
	 * attributable (world damage, environmental hazards). Kill credit - money, kill count, and
	 * later "favourite weapon" statistics - is resolved from this rather than assuming one player.
	 */
	AController* GetLastDamageInstigator() const { return LastDamageInstigator.Get(); }

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	TObjectPtr<UHealthComponent> CachedHealthComponent;

	TWeakObjectPtr<AController> LastDamageInstigator;

	UFUNCTION()
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);
};
