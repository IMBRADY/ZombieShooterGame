#pragma once

#include "CoreMinimal.h"
#include "Core/Save/ZombieUserSettings.h"
#include "UI/ZombieWidgetBase.h"
#include "ZombieSettingsWidget.generated.h"

class UButton;
class UVerticalBox;
class UZombieSettingRowWidget;

/**
 * Audio, display and accessibility options. Changes preview live (volume is heard as the slider
 * moves) and are written to the profile when the screen closes.
 */
UCLASS()
class ZOMBIEGAME_API UZombieSettingsWidget : public UZombieMenuWidget
{
	GENERATED_BODY()

public:
	virtual bool HandleBackAction() override;
	virtual UWidget* GetInitialFocus() const override;

	/** Opened from the pause menu it keeps the game paused; from the main menu it doesn't matter. */
	virtual bool PausesGame() const override { return true; }

protected:
	virtual void BuildWidget(UWidgetTree& Tree) override;

private:
	UFUNCTION() void HandleBack();

	void AddHeader(UWidgetTree& Tree, UVerticalBox& Column, const FText& Title) const;
	UZombieSettingRowWidget* AddSlider(UWidgetTree& Tree, UVerticalBox& Column, const FText& Label, float FZombieUserSettings::* Field,
		float Min = 0.0f, float Max = 1.0f, bool bPercent = true);
	UZombieSettingRowWidget* AddToggle(UWidgetTree& Tree, UVerticalBox& Column, const FText& Label, bool FZombieUserSettings::* Field);

	/** Pushes the working copy to the game instance without writing it to disk. */
	void Preview();

	FZombieUserSettings Working;

	UPROPERTY(Transient)
	TObjectPtr<UZombieSettingRowWidget> FirstRow;
};
