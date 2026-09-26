#include "ZombiePauseMenuWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/ZombieGameInstance.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UI/Widgets/ZombieSettingsWidget.h"
#include "UI/ZombieUIManager.h"
#include "UI/ZombieUIStyle.h"

#define LOCTEXT_NAMESPACE "ZombieMenus"

void UZombiePauseMenuWidget::BuildWidget(UWidgetTree& Tree)
{
	UVerticalBox* Column = Tree.ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

	UTextBlock* Title = ZombieUI::MakeText(Tree, LOCTEXT("Paused", "PAUSED"), 34, ZombieUI::Accent);
	Column->AddChildToVerticalBox(Title)->SetHorizontalAlignment(HAlign_Center);

	const auto AddButton = [&](const FText& Label) -> UButton*
	{
		UButton* Button = ZombieUI::MakeButton(Tree, Label, 18);
		UVerticalBoxSlot* ChildSlot = Column->AddChildToVerticalBox(ZombieUI::MakeSized(Tree, Button, 340.0f, -1.0f));
		ChildSlot->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 0.0f));
		ChildSlot->SetHorizontalAlignment(HAlign_Center);
		return Button;
	};

	ResumeButton = AddButton(LOCTEXT("Resume", "RESUME"));
	ResumeButton->OnClicked.AddDynamic(this, &UZombiePauseMenuWidget::HandleResume);
	AddButton(LOCTEXT("Settings", "SETTINGS"))->OnClicked.AddDynamic(this, &UZombiePauseMenuWidget::HandleSettings);
	AddButton(LOCTEXT("SaveQuit", "SAVE & QUIT TO MENU"))->OnClicked.AddDynamic(this, &UZombiePauseMenuWidget::HandleQuitToMenu);
	AddButton(LOCTEXT("QuitGame", "QUIT GAME"))->OnClicked.AddDynamic(this, &UZombiePauseMenuWidget::HandleQuitGame);

	UTextBlock* Hint = ZombieUI::MakeText(Tree, LOCTEXT("SaveHint", "Your run resumes from the start of the current sector."), 10, ZombieUI::TextDimColor);
	Column->AddChildToVerticalBox(Hint)->SetPadding(FMargin(0.0f, 16.0f, 0.0f, 0.0f));

	MakeBackdrop(Tree, ZombieUI::MakePanel(Tree, Column, FMargin(36.0f)));
}

UWidget* UZombiePauseMenuWidget::GetInitialFocus() const
{
	return ResumeButton;
}

void UZombiePauseMenuWidget::HandleResume()
{
	PlayUISound(TEXT("UI.Click"));
	CloseMenu();
}

void UZombiePauseMenuWidget::HandleSettings()
{
	PlayUISound(TEXT("UI.Click"));
	if (UZombieUIManager* UI = UZombieUIManager::Get(GetOwningPlayer()))
	{
		UI->PushMenu<UZombieSettingsWidget>();
	}
}

void UZombiePauseMenuWidget::HandleQuitToMenu()
{
	PlayUISound(TEXT("UI.Click"));

	// The checkpoint was written when this sector (or intermission) began; nothing else to save.
	if (UZombieGameInstance* GameInstance = Cast<UZombieGameInstance>(GetGameInstance()))
	{
		GameInstance->ReturnToMainMenu();
	}
}

void UZombiePauseMenuWidget::HandleQuitGame()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

#undef LOCTEXT_NAMESPACE
