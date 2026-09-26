#include "ZombieSettingRowWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CheckBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "UI/ZombieUIStyle.h"

#define LOCTEXT_NAMESPACE "ZombieMenus"

void UZombieSettingRowWidget::InitSlider(const FText& Label, float Value, float Min, float Max, bool bShowPercent)
{
	bIsToggle = false;
	PendingLabel = Label;
	PendingValue = Value;
	RangeMin = Min;
	RangeMax = FMath::Max(Max, Min + KINDA_SMALL_NUMBER);
	bPercent = bShowPercent;
}

void UZombieSettingRowWidget::InitToggle(const FText& Label, bool bValue)
{
	bIsToggle = true;
	PendingLabel = Label;
	PendingValue = bValue ? 1.0f : 0.0f;
}

void UZombieSettingRowWidget::BuildWidget(UWidgetTree& Tree)
{
	UHorizontalBox* Row = Tree.ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

	LabelText = ZombieUI::MakeText(Tree, PendingLabel, 13);
	UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(ZombieUI::MakeSized(Tree, LabelText, 300.0f, -1.0f));
	LabelSlot->SetVerticalAlignment(VAlign_Center);

	if (bIsToggle)
	{
		Toggle = Tree.ConstructWidget<UCheckBox>(UCheckBox::StaticClass());
		Toggle->SetIsChecked(PendingValue > 0.5f);
		Toggle->OnCheckStateChanged.AddDynamic(this, &UZombieSettingRowWidget::HandleToggleChanged);
		Row->AddChildToHorizontalBox(Toggle)->SetVerticalAlignment(VAlign_Center);
	}
	else
	{
		Slider = Tree.ConstructWidget<USlider>(USlider::StaticClass());
		Slider->SetMinValue(RangeMin);
		Slider->SetMaxValue(RangeMax);
		Slider->SetValue(PendingValue);
		Slider->SetStepSize((RangeMax - RangeMin) / 20.0f);
		Slider->SetSliderBarColor(ZombieUI::TextDimColor);
		Slider->SetSliderHandleColor(ZombieUI::Accent);
		Slider->OnValueChanged.AddDynamic(this, &UZombieSettingRowWidget::HandleSliderChanged);
		UHorizontalBoxSlot* SliderSlot = Row->AddChildToHorizontalBox(ZombieUI::MakeSized(Tree, Slider, 260.0f, 24.0f));
		SliderSlot->SetVerticalAlignment(VAlign_Center);
	}

	ValueText = ZombieUI::MakeText(Tree, FText::GetEmpty(), 12, ZombieUI::Accent);
	Row->AddChildToHorizontalBox(ValueText)->SetPadding(FMargin(14.0f, 0.0f, 0.0f, 0.0f));
	RefreshValueText(PendingValue);

	Tree.RootWidget = Row;
}

UWidget* UZombieSettingRowWidget::GetFocusTarget() const
{
	return bIsToggle ? static_cast<UWidget*>(Toggle) : static_cast<UWidget*>(Slider);
}

void UZombieSettingRowWidget::HandleSliderChanged(float Value)
{
	RefreshValueText(Value);
	OnChanged.ExecuteIfBound(Value);
}

void UZombieSettingRowWidget::HandleToggleChanged(bool bIsChecked)
{
	RefreshValueText(bIsChecked ? 1.0f : 0.0f);
	OnChanged.ExecuteIfBound(bIsChecked ? 1.0f : 0.0f);
	PlayUISound(TEXT("UI.Click"));
}

void UZombieSettingRowWidget::RefreshValueText(float Value)
{
	if (!ValueText)
	{
		return;
	}

	if (bIsToggle)
	{
		ValueText->SetText(Value > 0.5f ? LOCTEXT("On", "ON") : LOCTEXT("Off", "OFF"));
	}
	else if (bPercent)
	{
		ValueText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Value * 100.0f))));
	}
	else
	{
		ValueText->SetText(FText::FromString(FString::Printf(TEXT("%.2fx"), Value)));
	}
}

#undef LOCTEXT_NAMESPACE
