#pragma once

#include "CoreMinimal.h"
#include "UI/ZombieWidgetBase.h"
#include "ZombieMainMenuWidget.generated.h"

class UButton;
class UTextBlock;

/** Title screen: Continue / New Run / Settings / Records / Quit. */
UCLASS()
class ZOMBIEGAME_API UZombieMainMenuWidget : public UZombieMenuWidget
{
	GENERATED_BODY()

public:
	/** The title screen is the bottom of the stack - Escape does nothing here. */
	virtual bool HandleBackAction() override { return true; }
	virtual UWidget* GetInitialFocus() const override;

protected:
	virtual void BuildWidget(UWidgetTree& Tree) override;

private:
	UFUNCTION() void HandleContinue();
	UFUNCTION() void HandleNewRun();
	UFUNCTION() void HandleSettings();
	UFUNCTION() void HandleRecords();
	UFUNCTION() void HandleQuit();

	UPROPERTY(Transient) TObjectPtr<UButton> ContinueButton;
	UPROPERTY(Transient) TObjectPtr<UButton> NewRunButton;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> NewRunLabel;

	/** Starting a new run over an existing save needs a second press to confirm. */
	bool bConfirmingNewRun = false;
};
