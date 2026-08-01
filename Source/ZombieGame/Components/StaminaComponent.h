#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "StaminaComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStaminaChanged, float, NewStamina, float, MaxStamina);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ZOMBIEGAME_API UStaminaComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UStaminaComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	float GetStamina() const { return Stamina; }
	float GetMaxStamina() const { return MaxStamina; }
	bool CanSprint() const { return Stamina > 0.0f; }
	bool IsSprinting() const { return bIsSprinting; }

	void SetSprinting(bool bNewSprinting);

	UPROPERTY(BlueprintAssignable)
	FOnStaminaChanged OnStaminaChanged;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "Stamina")
	float MaxStamina = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Stamina")
	float DrainPerSecond = 25.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Stamina")
	float RechargeDelay = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Stamina")
	float RechargePerSecond = 20.0f;

	UPROPERTY(ReplicatedUsing = OnRep_Stamina)
	float Stamina = 0.0f;

	UFUNCTION()
	void OnRep_Stamina();

private:
	bool bIsSprinting = false;
	float TimeSinceSprintStopped = 0.0f;
};
