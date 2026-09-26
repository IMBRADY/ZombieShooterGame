#include "ZombieDeathScreenWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/ZombieGameInstance.h"
#include "UI/ZombieUIStyle.h"

#define LOCTEXT_NAMESPACE "ZombieMenus"

void UZombieDeathScreenWidget::BuildWidget(UWidgetTree& Tree)
{
	UVerticalBox* Column = Tree.ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

	Column->AddChildToVerticalBox(ZombieUI::MakeText(Tree, LOCTEXT("Dead", "YOU DIED"), 48, ZombieUI::Danger))->SetHorizontalAlignment(HAlign_Center);
	Column->AddChildToVerticalBox(ZombieUI::MakeText(Tree, LOCTEXT("RunOver", "The run is over. Nothing carries over - try again."), 12, ZombieUI::TextDimColor))
		->SetHorizontalAlignment(HAlign_Center);

	StatList = Tree.ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Column->AddChildToVerticalBox(StatList)->SetPadding(FMargin(0.0f, 24.0f));

	MenuButton = ZombieUI::MakeButton(Tree, LOCTEXT("Menu", "RETURN TO MAIN MENU"), 18);
	MenuButton->OnClicked.AddDynamic(this, &UZombieDeathScreenWidget::HandleReturnToMenu);
	Column->AddChildToVerticalBox(ZombieUI::MakeSized(Tree, MenuButton, 380.0f, -1.0f))->SetHorizontalAlignment(HAlign_Center);

	MakeBackdrop(Tree, ZombieUI::MakePanel(Tree, Column, FMargin(40.0f)), 0.8f);
}

void UZombieDeathScreenWidget::AddStat(const FText& Label, const FText& Value)
{
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Row->AddChildToHorizontalBox(ZombieUI::MakeSized(*WidgetTree, ZombieUI::MakeText(*WidgetTree, Label, 16, ZombieUI::TextDimColor), 300.0f, -1.0f));
	Row->AddChildToHorizontalBox(ZombieUI::MakeText(*WidgetTree, Value, 16, ZombieUI::TextColor));
	StatList->AddChildToVerticalBox(Row)->SetPadding(FMargin(0.0f, 4.0f));
}

void UZombieDeathScreenWidget::ShowSummary(const FZombieRunStats& Stats, int32 SectorReached)
{
	if (!StatList)
	{
		return;
	}

	StatList->ClearChildren();
	const FText Favorite = Stats.GetFavoriteWeapon();

	AddStat(LOCTEXT("Sector", "Sector reached"), FText::AsNumber(SectorReached));
	AddStat(LOCTEXT("Money", "Money earned"), FText::Format(LOCTEXT("MoneyValue", "${0}"), FText::AsNumber(Stats.MoneyEarned)));
	AddStat(LOCTEXT("Kills", "Kills"), FText::AsNumber(Stats.Kills));
	AddStat(LOCTEXT("Bosses", "Bosses defeated"), FText::AsNumber(Stats.BossesDefeated));
	AddStat(LOCTEXT("Damage", "Damage taken"), FText::AsNumber(FMath::RoundToInt(Stats.DamageTaken)));
	AddStat(LOCTEXT("Favorite", "Favourite weapon"), Favorite.IsEmpty() ? LOCTEXT("None", "-") : Favorite);
	AddStat(LOCTEXT("Time", "Total time"), ZombieUI::FormatTime(Stats.ElapsedSeconds));
}

UWidget* UZombieDeathScreenWidget::GetInitialFocus() const
{
	return MenuButton;
}

void UZombieDeathScreenWidget::HandleReturnToMenu()
{
	PlayUISound(TEXT("UI.Click"));
	if (UZombieGameInstance* GameInstance = Cast<UZombieGameInstance>(GetGameInstance()))
	{
		GameInstance->ReturnToMainMenu();
	}
}

#undef LOCTEXT_NAMESPACE
