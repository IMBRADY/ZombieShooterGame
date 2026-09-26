#pragma once

#include "CoreMinimal.h"
#include "UI/ZombieWidgetBase.h"
#include "ZombieSettingRowWidget.generated.h"

class UCheckBox;
class USlider;
class UTextBlock;

DECLARE_DELEGATE_OneParam(FOnSettingRowChanged, float /*Value*/);

/**
 * One labelled option on the settings screen - a slider (0..1 or a custom range) or an on/off
 * toggle. Forwards changes through a native delegate so the settings screen can bind each row to
 * a field with a lambda instead of one UFUNCTION per option.
 */
UCLASS()
class ZOMBIEGAME_API UZombieSettingRowWidget : public UZombieWidgetBase
{
	GENERATED_BODY()

public:
	void InitSlider(const FText& Label, float Value, float Min, float Max, bool bShowPercent);
	void InitToggle(const FText& Label, bool bValue);

	FOnSettingRowChanged OnChanged;

	UWidget* GetFocusTarget() const;

protected:
	virtual void BuildWidget(UWidgetTree& Tree) override;

private:
	UFUNCTION() void HandleSliderChanged(float Value);
	UFUNCTION() void HandleToggleChanged(bool bIsChecked);

	void RefreshValueText(float Value);

	UPROPERTY(Transient) TObjectPtr<UTextBlock> LabelText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ValueText;
	UPROPERTY(Transient) TObjectPtr<USlider> Slider;
	UPROPERTY(Transient) TObjectPtr<UCheckBox> Toggle;

	FText PendingLabel;
	float PendingValue = 0.0f;
	float RangeMin = 0.0f;
	float RangeMax = 1.0f;
	bool bIsToggle = false;
	bool bPercent = true;
};
