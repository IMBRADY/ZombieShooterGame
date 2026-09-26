#include "ZombieStatisticsWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/Achievements/AchievementDataAsset.h"
#include "Core/Achievements/ZombieAchievementSubsystem.h"
#include "Core/Save/ZombieSaveGames.h"
#include "Core/Save/ZombieSaveSubsystem.h"
#include "UI/ZombieUIStyle.h"

#define LOCTEXT_NAMESPACE "ZombieMenus"

void UZombieStatisticsWidget::BuildWidget(UWidgetTree& Tree)
{
	UVerticalBox* Column = Tree.ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Column->AddChildToVerticalBox(ZombieUI::MakeText(Tree, LOCTEXT("StatsTitle", "RECORDS"), 30, ZombieUI::Accent))->SetHorizontalAlignment(HAlign_Center);

	BuildStatistics(Tree, *Column);
	BuildHighScores(Tree, *Column);
	BuildAchievements(Tree, *Column);

	BackButton = ZombieUI::MakeButton(Tree, LOCTEXT("Back", "BACK"), 18);
	BackButton->OnClicked.AddDynamic(this, &UZombieStatisticsWidget::HandleBack);
	UVerticalBoxSlot* BackSlot = Column->AddChildToVerticalBox(ZombieUI::MakeSized(Tree, BackButton, 260.0f, -1.0f));
	BackSlot->SetHorizontalAlignment(HAlign_Center);
	BackSlot->SetPadding(FMargin(0.0f, 20.0f, 0.0f, 0.0f));

	UScrollBox* Scroll = Tree.ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	Scroll->AddChild(Column);
	MakeBackdrop(Tree, ZombieUI::MakePanel(Tree, ZombieUI::MakeSized(Tree, Scroll, 820.0f, 760.0f), FMargin(28.0f)), 0.75f);
}

void UZombieStatisticsWidget::AddLine(UWidgetTree& Tree, UVerticalBox& Column, const FText& Label, const FText& Value, const FLinearColor& Color) const
{
	UHorizontalBox* Row = Tree.ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	Row->AddChildToHorizontalBox(ZombieUI::MakeSized(Tree, ZombieUI::MakeText(Tree, Label, 13, Color), 460.0f, -1.0f));
	Row->AddChildToHorizontalBox(ZombieUI::MakeText(Tree, Value, 13, ZombieUI::TextColor));
	Column.AddChildToVerticalBox(Row)->SetPadding(FMargin(0.0f, 2.0f));
}

void UZombieStatisticsWidget::BuildStatistics(UWidgetTree& Tree, UVerticalBox& Column) const
{
	const UZombieSaveSubsystem* Saves = UZombieSaveSubsystem::Get(this);
	const UZombieMetaSaveGame* Meta = Saves ? Saves->GetMeta() : nullptr;
	if (!Meta)
	{
		return;
	}

	const FZombieLifetimeStats& Life = Meta->Lifetime;
	Column.AddChildToVerticalBox(ZombieUI::MakeText(Tree, LOCTEXT("Lifetime", "LIFETIME"), 16, ZombieUI::MoneyColor))->SetPadding(FMargin(0, 16, 0, 6));
	AddLine(Tree, Column, LOCTEXT("Runs", "Runs started"), FText::AsNumber(Life.RunsStarted), ZombieUI::TextDimColor);
	AddLine(Tree, Column, LOCTEXT("Deaths", "Deaths"), FText::AsNumber(Life.Deaths), ZombieUI::TextDimColor);
	AddLine(Tree, Column, LOCTEXT("Kills", "Zombies killed"), FText::AsNumber(Life.TotalKills), ZombieUI::TextDimColor);
	AddLine(Tree, Column, LOCTEXT("Bosses", "Bosses defeated"), FText::AsNumber(Life.BossesDefeated), ZombieUI::TextDimColor);
	AddLine(Tree, Column, LOCTEXT("Highest", "Highest sector"), FText::AsNumber(Life.HighestSector), ZombieUI::TextDimColor);
	AddLine(Tree, Column, LOCTEXT("Cleared", "Sectors cleared"), FText::AsNumber(Life.SectorsCleared), ZombieUI::TextDimColor);
	AddLine(Tree, Column, LOCTEXT("Money", "Money earned"), FText::AsNumber(Life.MoneyEarned), ZombieUI::TextDimColor);
	AddLine(Tree, Column, LOCTEXT("Played", "Time played"), ZombieUI::FormatTime(Life.PlaySeconds), ZombieUI::TextDimColor);
}

void UZombieStatisticsWidget::BuildHighScores(UWidgetTree& Tree, UVerticalBox& Column) const
{
	const UZombieSaveSubsystem* Saves = UZombieSaveSubsystem::Get(this);
	const UZombieMetaSaveGame* Meta = Saves ? Saves->GetMeta() : nullptr;
	if (!Meta)
	{
		return;
	}

	Column.AddChildToVerticalBox(ZombieUI::MakeText(Tree, LOCTEXT("Best", "BEST RUNS"), 16, ZombieUI::MoneyColor))->SetPadding(FMargin(0, 16, 0, 6));
	if (Meta->HighScores.Num() == 0)
	{
		AddLine(Tree, Column, LOCTEXT("NoRuns", "No finished runs yet."), FText::GetEmpty(), ZombieUI::TextDimColor);
	}

	for (int32 Index = 0; Index < Meta->HighScores.Num(); ++Index)
	{
		const FZombieHighScore& Score = Meta->HighScores[Index];
		AddLine(Tree, Column,
			FText::Format(LOCTEXT("Rank", "#{0}  Sector {1}"), FText::AsNumber(Index + 1), FText::AsNumber(Score.Sector)),
			FText::Format(LOCTEXT("ScoreDetail", "{0} kills  ${1}  {2}"), FText::AsNumber(Score.Kills), FText::AsNumber(Score.MoneyEarned), ZombieUI::FormatTime(Score.Seconds)),
			ZombieUI::TextDimColor);
	}
}

void UZombieStatisticsWidget::BuildAchievements(UWidgetTree& Tree, UVerticalBox& Column) const
{
	const UZombieAchievementSubsystem* Achievements = UZombieAchievementSubsystem::Get(this);
	if (!Achievements)
	{
		return;
	}

	TArray<const UAchievementDataAsset*> All;
	Achievements->GetAllAchievements(All);

	int32 UnlockedCount = 0;
	for (const UAchievementDataAsset* Achievement : All)
	{
		UnlockedCount += Achievements->IsUnlocked(Achievement) ? 1 : 0;
	}

	Column.AddChildToVerticalBox(ZombieUI::MakeText(Tree,
		FText::Format(LOCTEXT("AchievementsTitle", "ACHIEVEMENTS  {0}/{1}"), FText::AsNumber(UnlockedCount), FText::AsNumber(All.Num())),
		16, ZombieUI::MoneyColor))->SetPadding(FMargin(0, 16, 0, 6));

	for (const UAchievementDataAsset* Achievement : All)
	{
		const bool bUnlocked = Achievements->IsUnlocked(Achievement);
		const bool bSecret = Achievement->bHidden && !bUnlocked;
		const FText Name = bSecret ? LOCTEXT("Secret", "???") : Achievement->DisplayName;
		const FText Detail = bSecret ? LOCTEXT("SecretDetail", "Hidden achievement")
			: bUnlocked ? Achievement->Description
			: FText::Format(LOCTEXT("Progress", "{0}  ({1}/{2})"), Achievement->Description,
				FText::AsNumber(FMath::Min(Achievements->GetLifetimeProgress(Achievement), Achievement->Threshold)), FText::AsNumber(Achievement->Threshold));

		AddLine(Tree, Column, Name, Detail, bUnlocked ? ZombieUI::Accent : ZombieUI::TextDimColor);
	}
}

UWidget* UZombieStatisticsWidget::GetInitialFocus() const
{
	return BackButton;
}

void UZombieStatisticsWidget::HandleBack()
{
	PlayUISound(TEXT("UI.Click"));
	CloseMenu();
}

#undef LOCTEXT_NAMESPACE
