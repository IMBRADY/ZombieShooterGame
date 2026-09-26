#include "ZombieObjectiveWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Characters/Zombies/ZombieArchetypeDataAsset.h"
#include "Characters/Zombies/ZombieCharacter.h"
#include "Components/HealthComponent.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/ZombieUIStyle.h"

#define LOCTEXT_NAMESPACE "ZombieHUD"

void UZombieObjectiveWidget::BuildWidget(UWidgetTree& Tree)
{
	UVerticalBox* Column = Tree.ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

	SectorText = ZombieUI::MakeText(Tree, FText::GetEmpty(), 26, ZombieUI::Accent);
	SectorText->SetJustification(ETextJustify::Center);
	ObjectiveText = ZombieUI::MakeText(Tree, FText::GetEmpty(), 13);
	ObjectiveText->SetJustification(ETextJustify::Center);

	BossPanel = Tree.ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	BossName = ZombieUI::MakeText(Tree, FText::GetEmpty(), 16, ZombieUI::Danger);
	BossName->SetJustification(ETextJustify::Center);
	BossBar = ZombieUI::MakeBar(Tree, ZombieUI::Danger);
	BossPanel->AddChildToVerticalBox(BossName)->SetHorizontalAlignment(HAlign_Center);
	BossPanel->AddChildToVerticalBox(ZombieUI::MakeSized(Tree, BossBar, 560.0f, 18.0f))->SetPadding(FMargin(0.0f, 4.0f));
	BossPanel->SetVisibility(ESlateVisibility::Collapsed);

	Column->AddChildToVerticalBox(SectorText)->SetHorizontalAlignment(HAlign_Center);
	Column->AddChildToVerticalBox(ObjectiveText)->SetHorizontalAlignment(HAlign_Center);
	Column->AddChildToVerticalBox(BossPanel)->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 0.0f));

	Tree.RootWidget = Column;
}

void UZombieObjectiveWidget::Unbind()
{
	if (AZombieGameState* GameState = BoundGameState.Get())
	{
		GameState->OnSectorChanged.RemoveDynamic(this, &UZombieObjectiveWidget::HandleSectorChanged);
		GameState->OnSectorPhaseChanged.Remove(PhaseHandle);
		GameState->OnActiveBossChanged.Remove(BossHandle);
		GameState->OnEncounterStateChanged.Remove(EncounterHandle);
	}
	HandleBossChanged(nullptr);
	BoundGameState.Reset();
}

void UZombieObjectiveWidget::Observe(AZombieGameState* GameState)
{
	Unbind();
	if (!GameState)
	{
		return;
	}

	BoundGameState = GameState;
	GameState->OnSectorChanged.AddDynamic(this, &UZombieObjectiveWidget::HandleSectorChanged);
	PhaseHandle = GameState->OnSectorPhaseChanged.AddUObject(this, &UZombieObjectiveWidget::HandlePhaseChanged);
	BossHandle = GameState->OnActiveBossChanged.AddUObject(this, &UZombieObjectiveWidget::HandleBossChanged);
	EncounterHandle = GameState->OnEncounterStateChanged.AddUObject(this, &UZombieObjectiveWidget::RefreshObjective);

	HandleSectorChanged(GameState->GetCurrentSector());
	HandleBossChanged(GameState->GetActiveBoss());
}

void UZombieObjectiveWidget::NativeDestruct()
{
	Unbind();
	Super::NativeDestruct();
}

void UZombieObjectiveWidget::HandleSectorChanged(int32 NewSector)
{
	const AZombieGameState* GameState = BoundGameState.Get();
	const bool bBoss = GameState && GameState->IsBossSector();
	SectorText->SetText(FText::Format(bBoss ? LOCTEXT("BossSector", "SECTOR {0} - BOSS") : LOCTEXT("Sector", "SECTOR {0}"), FText::AsNumber(NewSector)));
	RefreshObjective();
}

void UZombieObjectiveWidget::HandlePhaseChanged(ESectorPhase NewPhase)
{
	RefreshObjective();
}

void UZombieObjectiveWidget::RefreshObjective()
{
	const AZombieGameState* GameState = BoundGameState.Get();
	if (!GameState)
	{
		return;
	}

	FText Objective;
	switch (GameState->GetSectorPhase())
	{
	case ESectorPhase::Starting:
	case ESectorPhase::Combat:
		Objective = GameState->GetActiveBoss()
			? LOCTEXT("KillBoss", "Kill the boss")
			: FText::Format(LOCTEXT("Clear", "Clear the sector - {0} remaining"), FText::AsNumber(GameState->GetRemainingEnemies()));
		break;
	case ESectorPhase::KeyDropped:		Objective = LOCTEXT("GetKey", "Pick up the sector key"); break;
	case ESectorPhase::ExitUnlocked:	Objective = LOCTEXT("GetOut", "Exit unlocked - reach the exit door"); break;
	case ESectorPhase::Intermission:	Objective = LOCTEXT("Shop", "Intermission - spend wisely, then leave"); break;
	default:							break;
	}
	ObjectiveText->SetText(Objective);
}

void UZombieObjectiveWidget::HandleBossChanged(AActor* Boss)
{
	if (UHealthComponent* Previous = BoundBossHealth.Get())
	{
		Previous->OnHealthChanged.RemoveDynamic(this, &UZombieObjectiveWidget::HandleBossHealthChanged);
	}
	BoundBossHealth.Reset();

	UHealthComponent* Health = Boss ? Boss->FindComponentByClass<UHealthComponent>() : nullptr;
	BossPanel->SetVisibility(Health ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (Health)
	{
		const AZombieCharacter* Zombie = Cast<AZombieCharacter>(Boss);
		const UZombieArchetypeDataAsset* Archetype = Zombie ? Zombie->GetArchetype() : nullptr;
		BossName->SetText(Archetype ? Archetype->DisplayName : LOCTEXT("Boss", "BOSS"));

		BoundBossHealth = Health;
		Health->OnHealthChanged.AddDynamic(this, &UZombieObjectiveWidget::HandleBossHealthChanged);
		HandleBossHealthChanged(Health->GetHealth(), Health->GetMaxHealth(), 0.0f);
	}
	RefreshObjective();
}

void UZombieObjectiveWidget::HandleBossHealthChanged(float NewHealth, float MaxHealth, float Delta)
{
	BossBar->SetPercent(MaxHealth > 0.0f ? NewHealth / MaxHealth : 0.0f);
}

#undef LOCTEXT_NAMESPACE
