#include "ZombieMainMenuWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/Save/ZombieSaveSubsystem.h"
#include "Core/ZombieGameInstance.h"
#include "Engine/Texture2D.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UI/Widgets/ZombieSettingsWidget.h"
#include "UI/Widgets/ZombieStatisticsWidget.h"
#include "UI/ZombieUIManager.h"
#include "UI/ZombieUIStyle.h"

#define LOCTEXT_NAMESPACE "ZombieMenus"

void UZombieMainMenuWidget::BuildWidget(UWidgetTree& Tree)
{
	UCanvasPanel* Canvas = Tree.ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	Tree.RootWidget = Canvas;

	UImage* Background = ZombieUI::MakeImage(Tree, Cast<UTexture2D>(FSoftObjectPath(TEXT("/Game/UI/Textures/T_TitleBackground.T_TitleBackground")).TryLoad()),
		FVector2D(1920.0f, 1080.0f));
	UCanvasPanelSlot* BackgroundSlot = Canvas->AddChildToCanvas(Background);
	BackgroundSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	BackgroundSlot->SetOffsets(FMargin(0.0f));

	UVerticalBox* Column = Tree.ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Column->AddChildToVerticalBox(ZombieUI::MakeText(Tree, LOCTEXT("Title", "ROTSHOT"), 64, ZombieUI::Accent))->SetHorizontalAlignment(HAlign_Center);
	Column->AddChildToVerticalBox(ZombieUI::MakeText(Tree, LOCTEXT("Tagline", "Clear the sector. Take the key. Spend it well."), 13, ZombieUI::TextDimColor))
		->SetHorizontalAlignment(HAlign_Center);

	const UZombieSaveSubsystem* Saves = UZombieSaveSubsystem::Get(this);
	FZombieRunSaveData SavedRun;
	const bool bHasSave = Saves && Saves->LoadRun(SavedRun);

	const auto AddButton = [&](const FText& Label, UTextBlock** OutLabel = nullptr) -> UButton*
	{
		UButton* Button = ZombieUI::MakeButton(Tree, Label, 20, OutLabel);
		UVerticalBoxSlot* ChildSlot = Column->AddChildToVerticalBox(ZombieUI::MakeSized(Tree, Button, 400.0f, -1.0f));
		ChildSlot->SetHorizontalAlignment(HAlign_Center);
		ChildSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));
		return Button;
	};

	Column->AddChildToVerticalBox(ZombieUI::MakeSized(Tree, nullptr, 1.0f, 40.0f));

	ContinueButton = AddButton(bHasSave
		? FText::Format(LOCTEXT("ContinueSector", "CONTINUE  (SECTOR {0})"), FText::AsNumber(SavedRun.Sector))
		: LOCTEXT("Continue", "CONTINUE"));
	ContinueButton->SetIsEnabled(bHasSave);
	ContinueButton->OnClicked.AddDynamic(this, &UZombieMainMenuWidget::HandleContinue);

	UTextBlock* NewRunLabelBlock = nullptr;
	NewRunButton = AddButton(LOCTEXT("NewRun", "NEW RUN"), &NewRunLabelBlock);
	NewRunLabel = NewRunLabelBlock;
	NewRunButton->OnClicked.AddDynamic(this, &UZombieMainMenuWidget::HandleNewRun);

	AddButton(LOCTEXT("Settings", "SETTINGS"))->OnClicked.AddDynamic(this, &UZombieMainMenuWidget::HandleSettings);
	AddButton(LOCTEXT("Records", "RECORDS & ACHIEVEMENTS"))->OnClicked.AddDynamic(this, &UZombieMainMenuWidget::HandleRecords);
	AddButton(LOCTEXT("Quit", "QUIT"))->OnClicked.AddDynamic(this, &UZombieMainMenuWidget::HandleQuit);

	PlaceOnCanvas(Canvas, Column, FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f), FVector2D(0.0f, 20.0f));

	UTextBlock* Controls = ZombieUI::MakeText(Tree,
		LOCTEXT("Controls", "WASD move   MOUSE aim   LMB shoot   R reload   SHIFT sprint   1-3 weapons   E interact   ESC pause"),
		10, ZombieUI::TextDimColor);
	PlaceOnCanvas(Canvas, Controls, FVector2D(0.5f, 1.0f), FVector2D(0.5f, 1.0f), FVector2D(0.0f, -24.0f));
}

UWidget* UZombieMainMenuWidget::GetInitialFocus() const
{
	return (ContinueButton && ContinueButton->GetIsEnabled()) ? ContinueButton.Get() : NewRunButton.Get();
}

void UZombieMainMenuWidget::HandleContinue()
{
	PlayUISound(TEXT("UI.Click"));
	if (UZombieGameInstance* GameInstance = Cast<UZombieGameInstance>(GetGameInstance()))
	{
		GameInstance->StartRun(true);
	}
}

void UZombieMainMenuWidget::HandleNewRun()
{
	PlayUISound(TEXT("UI.Click"));

	const UZombieSaveSubsystem* Saves = UZombieSaveSubsystem::Get(this);
	if (Saves && Saves->HasRunSave() && !bConfirmingNewRun)
	{
		bConfirmingNewRun = true;
		NewRunLabel->SetText(LOCTEXT("ConfirmNewRun", "ABANDON SAVED RUN?"));
		NewRunLabel->SetColorAndOpacity(FSlateColor(ZombieUI::Danger));
		return;
	}

	if (UZombieGameInstance* GameInstance = Cast<UZombieGameInstance>(GetGameInstance()))
	{
		GameInstance->StartRun(false);
	}
}

void UZombieMainMenuWidget::HandleSettings()
{
	PlayUISound(TEXT("UI.Click"));
	if (UZombieUIManager* UI = UZombieUIManager::Get(GetOwningPlayer()))
	{
		UI->PushMenu<UZombieSettingsWidget>();
	}
}

void UZombieMainMenuWidget::HandleRecords()
{
	PlayUISound(TEXT("UI.Click"));
	if (UZombieUIManager* UI = UZombieUIManager::Get(GetOwningPlayer()))
	{
		UI->PushMenu<UZombieStatisticsWidget>();
	}
}

void UZombieMainMenuWidget::HandleQuit()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

#undef LOCTEXT_NAMESPACE
