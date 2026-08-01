#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnHealthChanged, float, NewHealth, float, MaxHealth, float, Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnArmorChanged, float, NewArmor, float, MaxArmor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeath);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ZOMBIEGAME_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHealthComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	float GetHealth() const { return Health; }
	float GetMaxHealth() const { return MaxHealth; }
	float GetArmor() const { return Armor; }
	float GetMaxArmor() const { return MaxArmor; }
	bool IsDead() const { return bIsDead; }

	// Armor absorbs first, then health. Returns the amount actually applied to health.
	float ApplyDamage(float DamageAmount);

	// Re-sizes the pool - enemy health scales per sector, and perks raise the player's. bRefill is
	// for the spawn case (start at full); leave it clear to raise the ceiling without healing.
	void SetMaxHealth(float NewMaxHealth, bool bRefill);

	void AddArmor(float Amount);
	void Heal(float Amount);

	UPROPERTY(BlueprintAssignable)
	FOnHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable)
	FOnArmorChanged OnArmorChanged;

	UPROPERTY(BlueprintAssignable)
	FOnDeath OnDeath;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "Health")
	float MaxHealth = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Health")
	float MaxArmor = 100.0f;

	UPROPERTY(ReplicatedUsing = OnRep_Health)
	float Health = 0.0f;

	UPROPERTY(ReplicatedUsing = OnRep_Armor)
	float Armor = 0.0f;

	UFUNCTION()
	void OnRep_Health();

	UFUNCTION()
	void OnRep_Armor();

private:
	bool bIsDead = false;
};
