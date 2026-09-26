#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "DamageComponent.generated.h"

class UHealthComponent;
class UDamageType;
class AController;

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnDamageReceived, float /*Amount*/, AActor* /*Causer*/, const UDamageType* /*DamageType*/);

/**
 * Routes Unreal's standard damage (AActor::OnTakeAnyDamage) into the owner's HealthComponent,
 * applying resistances on the way. Keeps "how damage arrives" separate from "how health/armor
 * respond", so modifiers plug in here without touching HealthComponent:
 *
 *  - per-owner resistances by damage tag (the Armored zombie's reduced bullet damage), and
 *  - stat-driven resistances for players (the Explosion Resistance perk).
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ZOMBIEGAME_API UDamageComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDamageComponent();

	/**
	 * Controller responsible for the most recent damage this actor took, or null if it was not
	 * attributable (world damage, environmental hazards). Kill credit - money, kill count, and
	 * "favourite weapon" statistics - is resolved from this rather than assuming one player.
	 */
	AController* GetLastDamageInstigator() const { return LastDamageInstigator.Get(); }

	/** The weapon/projectile/zombie that dealt the most recent damage. */
	AActor* GetLastDamageCauser() const { return LastDamageCauser.Get(); }

	/** Fraction (0..1) of damage with this tag that is ignored. */
	void SetResistance(const FGameplayTag& DamageTag, float Fraction);

	/** Damage multiplier for incoming damage; the Tank-style damage sponges and invulnerable phases. */
	void SetIncomingDamageMultiplier(float Multiplier) { IncomingDamageMultiplier = FMath::Max(Multiplier, 0.0f); }

	/** Fires after resistances, with the damage actually handed to the health component. */
	FOnDamageReceived OnDamageReceived;

protected:
	virtual void BeginPlay() override;

private:
	float GetResistanceFor(const UDamageType* DamageType) const;

	UPROPERTY()
	TObjectPtr<UHealthComponent> CachedHealthComponent;

	TWeakObjectPtr<AController> LastDamageInstigator;
	TWeakObjectPtr<AActor> LastDamageCauser;
	TMap<FGameplayTag, float> Resistances;
	float IncomingDamageMultiplier = 1.0f;

	UFUNCTION()
	void HandleTakeAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser);
};
