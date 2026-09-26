#pragma once

#include "CoreMinimal.h"
#include "UI/ZombieWidgetBase.h"
#include "ZombieVitalsWidget.generated.h"

class AZombiePlayerState;
class UHealthComponent;
class UProgressBar;
class UStaminaComponent;
class UTextBlock;

/** Health, armor, stamina and money - top-left of the HUD. Purely event-driven. */
UCLASS()
class ZOMBIEGAME_API UZombieVitalsWidget : public UZombieWidgetBase
{
	GENERATED_BODY()

public:
	/** Rebinds to a new pawn/player state (either may be null). */
	void Observe(APawn* Pawn, AZombiePlayerState* PlayerState);

protected:
	virtual void BuildWidget(UWidgetTree& Tree) override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION() void HandleHealthChanged(float NewHealth, float MaxHealth, float Delta);
	UFUNCTION() void HandleArmorChanged(float NewArmor, float MaxArmor);
	UFUNCTION() void HandleStaminaChanged(float NewStamina, float MaxStamina);
	UFUNCTION() void HandleMoneyChanged(int32 NewMoney);

	void Unbind();

	UPROPERTY(Transient) TObjectPtr<UProgressBar> HealthBar;
	UPROPERTY(Transient) TObjectPtr<UProgressBar> ArmorBar;
	UPROPERTY(Transient) TObjectPtr<UProgressBar> StaminaBar;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HealthText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ArmorText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> MoneyText;

	TWeakObjectPtr<UHealthComponent> BoundHealth;
	TWeakObjectPtr<UStaminaComponent> BoundStamina;
	TWeakObjectPtr<AZombiePlayerState> BoundPlayerState;
};
