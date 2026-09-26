#include "ZombieSettingsWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/ZombieGameInstance.h"
#include "UI/Widgets/ZombieSettingRowWidget.h"
#include "UI/ZombieUIStyle.h"

#define LOCTEXT_NAMESPACE "ZombieMenus"

void UZombieSettingsWidget::BuildWidget(UWidgetTree& Tree)
{
	if (const UZombieGameInstance* GameInstance = Cast<UZombieGameInstance>(GetGameInstance()))
	{
		Working = GameInstance->GetUserSettings();
	}

	UVerticalBox* Column = Tree.ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Column->AddChildToVerticalBox(ZombieUI::MakeText(Tree, LOCTEXT("SettingsTitle", "SETTINGS"), 30, ZombieUI::Accent))
		->SetHorizontalAlignment(HAlign_Center);

	AddHeader(Tree, *Column, LOCTEXT("Audio", "AUDIO"));
	FirstRow = AddSlider(Tree, *Column, LOCTEXT("Master", "Master volume"), &FZombieUserSettings::MasterVolume);
	AddSlider(Tree, *Column, LOCTEXT("Music", "Music volume"), &FZombieUserSettings::MusicVolume);
	AddSlider(Tree, *Column, LOCTEXT("Effects", "Effects volume"), &FZombieUserSettings::EffectsVolume);
	AddSlider(Tree, *Column, LOCTEXT("Interface", "Interface volume"), &FZombieUserSettings::InterfaceVolume);

	AddHeader(Tree, *Column, LOCTEXT("Display", "DISPLAY"));
	AddToggle(Tree, *Column, LOCTEXT("Fullscreen", "Fullscreen"), &FZombieUserSettings::bFullscreen);
	AddToggle(Tree, *Column, LOCTEXT("VSync", "V-Sync"), &FZombieUserSettings::bVSync);
	AddToggle(Tree, *Column, LOCTEXT("Fps", "Show frame rate"), &FZombieUserSettings::bShowFrameRate);

	AddHeader(Tree, *Column, LOCTEXT("Accessibility", "ACCESSIBILITY"));
	AddToggle(Tree, *Column, LOCTEXT("Shake", "Screen shake"), &FZombieUserSettings::bScreenShake);
	AddToggle(Tree, *Column, LOCTEXT("Flash", "Damage flash"), &FZombieUserSettings::bDamageFlash);
	AddToggle(Tree, *Column, LOCTEXT("ToggleSprint", "Toggle sprint (instead of hold)"), &FZombieUserSettings::bToggleSprint);
	AddToggle(Tree, *Column, LOCTEXT("AutoFire", "Hold to fire semi-automatics"), &FZombieUserSettings::bAutoFireSemiAutomatic);
	AddSlider(Tree, *Column, LOCTEXT("UIScale", "Interface scale"), &FZombieUserSettings::InterfaceScale, 0.75f, 1.5f, false);

	UButton* Back = ZombieUI::MakeButton(Tree, LOCTEXT("Back", "BACK"), 18);
	Back->OnClicked.AddDynamic(this, &UZombieSettingsWidget::HandleBack);
	UVerticalBoxSlot* BackSlot = Column->AddChildToVerticalBox(ZombieUI::MakeSized(Tree, Back, 260.0f, -1.0f));
	BackSlot->SetHorizontalAlignment(HAlign_Center);
	BackSlot->SetPadding(FMargin(0.0f, 20.0f, 0.0f, 0.0f));

	UScrollBox* Scroll = Tree.ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	Scroll->AddChild(Column);
	MakeBackdrop(Tree, ZombieUI::MakePanel(Tree, ZombieUI::MakeSized(Tree, Scroll, 760.0f, 720.0f), FMargin(28.0f)), 0.75f);
}

void UZombieSettingsWidget::AddHeader(UWidgetTree& Tree, UVerticalBox& Column, const FText& Title) const
{
	Column.AddChildToVerticalBox(ZombieUI::MakeText(Tree, Title, 16, ZombieUI::MoneyColor))->SetPadding(FMargin(0.0f, 18.0f, 0.0f, 6.0f));
}

UZombieSettingRowWidget* UZombieSettingsWidget::AddSlider(UWidgetTree& Tree, UVerticalBox& Column, const FText& Label,
	float FZombieUserSettings::* Field, float Min, float Max, bool bPercent)
{
	UZombieSettingRowWidget* Row = Tree.ConstructWidget<UZombieSettingRowWidget>(UZombieSettingRowWidget::StaticClass());
	Row->InitSlider(Label, Working.*Field, Min, Max, bPercent);
	Row->OnChanged.BindWeakLambda(this, [this, Field](float Value)
	{
		Working.*Field = Value;
		Preview();
	});
	Column.AddChildToVerticalBox(Row)->SetPadding(FMargin(0.0f, 3.0f));
	return Row;
}

UZombieSettingRowWidget* UZombieSettingsWidget::AddToggle(UWidgetTree& Tree, UVerticalBox& Column, const FText& Label, bool FZombieUserSettings::* Field)
{
	UZombieSettingRowWidget* Row = Tree.ConstructWidget<UZombieSettingRowWidget>(UZombieSettingRowWidget::StaticClass());
	Row->InitToggle(Label, Working.*Field);
	Row->OnChanged.BindWeakLambda(this, [this, Field](float Value)
	{
		Working.*Field = Value > 0.5f;
		Preview();
	});
	Column.AddChildToVerticalBox(Row)->SetPadding(FMargin(0.0f, 3.0f));
	return Row;
}

void UZombieSettingsWidget::Preview()
{
	if (UZombieGameInstance* GameInstance = Cast<UZombieGameInstance>(GetGameInstance()))
	{
		GameInstance->SetUserSettings(Working, false);
	}
}

UWidget* UZombieSettingsWidget::GetInitialFocus() const
{
	return FirstRow ? FirstRow->GetFocusTarget() : nullptr;
}

bool UZombieSettingsWidget::HandleBackAction()
{
	HandleBack();
	return true;
}

void UZombieSettingsWidget::HandleBack()
{
	PlayUISound(TEXT("UI.Click"));
	if (UZombieGameInstance* GameInstance = Cast<UZombieGameInstance>(GetGameInstance()))
	{
		GameInstance->SetUserSettings(Working, true);
	}
	CloseMenu();
}

#undef LOCTEXT_NAMESPACE
