#pragma once

#include "CoreMinimal.h"
#include "UI/ZombieWidgetBase.h"
#include "ZombiePauseMenuWidget.generated.h"

class UButton;

/** Resume / Settings / Save & Quit / Quit Game. Pauses the world while open. */
UCLASS()
class ZOMBIEGAME_API UZombiePauseMenuWidget : public UZombieMenuWidget
{
	GENERATED_BODY()

public:
	virtual bool PausesGame() const override { return true; }
	virtual UWidget* GetInitialFocus() const override;

protected:
	virtual void BuildWidget(UWidgetTree& Tree) override;

private:
	UFUNCTION() void HandleResume();
	UFUNCTION() void HandleSettings();
	UFUNCTION() void HandleQuitToMenu();
	UFUNCTION() void HandleQuitGame();

	UPROPERTY(Transient)
	TObjectPtr<UButton> ResumeButton;
};
