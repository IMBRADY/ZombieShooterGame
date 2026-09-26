#include "ZombieVitalsWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HealthComponent.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/StaminaComponent.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/ZombiePlayerState.h"
#include "GameFramework/Pawn.h"
#include "UI/ZombieUIStyle.h"

#define LOCTEXT_NAMESPACE "ZombieHUD"

namespace
{
	/** A bar with a label drawn over it. */
	UOverlay* MakeLabelledBar(UWidgetTree& Tree, UProgressBar* Bar, UTextBlock* Label, float Width, float Height)
	{
		UOverlay* Overlay = Tree.ConstructWidget<UOverlay>(UOverlay::StaticClass());
		Overlay->AddChildToOverlay(ZombieUI::MakeSized(Tree, Bar, Width, Height));
		if (Label)
		{
			UOverlaySlot* LabelSlot = Overlay->AddChildToOverlay(Label);
			LabelSlot->SetHorizontalAlignment(HAlign_Center);
			LabelSlot->SetVerticalAlignment(VAlign_Center);
		}
		return Overlay;
	}
}

void UZombieVitalsWidget::BuildWidget(UWidgetTree& Tree)
{
	UVerticalBox* Column = Tree.ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

	HealthBar = ZombieUI::MakeBar(Tree, ZombieUI::HealthColor);
	HealthText = ZombieUI::MakeText(Tree, FText::GetEmpty(), 12);
	ArmorBar = ZombieUI::MakeBar(Tree, ZombieUI::ArmorColor);
	ArmorText = ZombieUI::MakeText(Tree, FText::GetEmpty(), 9);
	StaminaBar = ZombieUI::MakeBar(Tree, ZombieUI::StaminaColor);
	MoneyText = ZombieUI::MakeText(Tree, FText::FromString(TEXT("$0")), 20, ZombieUI::MoneyColor);

	Column->AddChildToVerticalBox(MakeLabelledBar(Tree, HealthBar, HealthText, 300.0f, 24.0f))->SetPadding(FMargin(0, 0, 0, 4));
	Column->AddChildToVerticalBox(MakeLabelledBar(Tree, ArmorBar, ArmorText, 300.0f, 14.0f))->SetPadding(FMargin(0, 0, 0, 4));
	Column->AddChildToVerticalBox(MakeLabelledBar(Tree, StaminaBar, nullptr, 300.0f, 8.0f))->SetPadding(FMargin(0, 0, 0, 10));
	Column->AddChildToVerticalBox(MoneyText);

	Tree.RootWidget = ZombieUI::MakePanel(Tree, Column, FMargin(14.0f));
}

void UZombieVitalsWidget::Unbind()
{
	if (UHealthComponent* Health = BoundHealth.Get())
	{
		Health->OnHealthChanged.RemoveDynamic(this, &UZombieVitalsWidget::HandleHealthChanged);
		Health->OnArmorChanged.RemoveDynamic(this, &UZombieVitalsWidget::HandleArmorChanged);
	}
	if (UStaminaComponent* Stamina = BoundStamina.Get())
	{
		Stamina->OnStaminaChanged.RemoveDynamic(this, &UZombieVitalsWidget::HandleStaminaChanged);
	}
	if (AZombiePlayerState* PlayerState = BoundPlayerState.Get())
	{
		PlayerState->OnMoneyChanged.RemoveDynamic(this, &UZombieVitalsWidget::HandleMoneyChanged);
	}
	BoundHealth.Reset();
	BoundStamina.Reset();
	BoundPlayerState.Reset();
}

void UZombieVitalsWidget::Observe(APawn* Pawn, AZombiePlayerState* PlayerState)
{
	Unbind();

	if (UHealthComponent* Health = Pawn ? Pawn->FindComponentByClass<UHealthComponent>() : nullptr)
	{
		BoundHealth = Health;
		Health->OnHealthChanged.AddDynamic(this, &UZombieVitalsWidget::HandleHealthChanged);
		Health->OnArmorChanged.AddDynamic(this, &UZombieVitalsWidget::HandleArmorChanged);
		HandleHealthChanged(Health->GetHealth(), Health->GetMaxHealth(), 0.0f);
		HandleArmorChanged(Health->GetArmor(), Health->GetMaxArmor());
	}

	if (UStaminaComponent* Stamina = Pawn ? Pawn->FindComponentByClass<UStaminaComponent>() : nullptr)
	{
		BoundStamina = Stamina;
		Stamina->OnStaminaChanged.AddDynamic(this, &UZombieVitalsWidget::HandleStaminaChanged);
		HandleStaminaChanged(Stamina->GetStamina(), Stamina->GetMaxStamina());
	}

	if (PlayerState)
	{
		BoundPlayerState = PlayerState;
		PlayerState->OnMoneyChanged.AddDynamic(this, &UZombieVitalsWidget::HandleMoneyChanged);
		HandleMoneyChanged(PlayerState->GetMoney());
	}
}

void UZombieVitalsWidget::NativeDestruct()
{
	Unbind();
	Super::NativeDestruct();
}

void UZombieVitalsWidget::HandleHealthChanged(float NewHealth, float MaxHealth, float Delta)
{
	HealthBar->SetPercent(MaxHealth > 0.0f ? NewHealth / MaxHealth : 0.0f);
	HealthText->SetText(FText::Format(LOCTEXT("Health", "HP {0}/{1}"), FText::AsNumber(FMath::CeilToInt(NewHealth)), FText::AsNumber(FMath::RoundToInt(MaxHealth))));
}

void UZombieVitalsWidget::HandleArmorChanged(float NewArmor, float MaxArmor)
{
	ArmorBar->SetPercent(MaxArmor > 0.0f ? NewArmor / MaxArmor : 0.0f);
	ArmorText->SetText(FText::Format(LOCTEXT("Armor", "ARMOR {0}"), FText::AsNumber(FMath::CeilToInt(NewArmor))));
}

void UZombieVitalsWidget::HandleStaminaChanged(float NewStamina, float MaxStamina)
{
	StaminaBar->SetPercent(MaxStamina > 0.0f ? NewStamina / MaxStamina : 0.0f);
}

void UZombieVitalsWidget::HandleMoneyChanged(int32 NewMoney)
{
	MoneyText->SetText(FText::Format(LOCTEXT("Money", "${0}"), FText::AsNumber(NewMoney)));
}

#undef LOCTEXT_NAMESPACE
