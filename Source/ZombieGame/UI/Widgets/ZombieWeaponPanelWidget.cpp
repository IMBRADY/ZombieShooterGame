#include "ZombieWeaponPanelWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/InventoryComponent.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/Pawn.h"
#include "UI/ZombieUIStyle.h"
#include "Weapons/WeaponDataAsset.h"
#include "Weapons/WeaponRaritySettings.h"
#include "Weapons/ZombieWeapon.h"

#define LOCTEXT_NAMESPACE "ZombieHUD"

void UZombieWeaponPanelWidget::BuildWidget(UWidgetTree& Tree)
{
	UVerticalBox* Column = Tree.ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

	WeaponName = ZombieUI::MakeText(Tree, FText::GetEmpty(), 16);
	WeaponName->SetJustification(ETextJustify::Right);

	UHorizontalBox* AmmoRow = Tree.ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	AmmoText = ZombieUI::MakeText(Tree, FText::GetEmpty(), 34);
	ReserveText = ZombieUI::MakeText(Tree, FText::GetEmpty(), 16, ZombieUI::TextDimColor);
	AmmoRow->AddChildToHorizontalBox(AmmoText)->SetVerticalAlignment(VAlign_Bottom);
	UHorizontalBoxSlot* ReserveSlot = AmmoRow->AddChildToHorizontalBox(ReserveText);
	ReserveSlot->SetVerticalAlignment(VAlign_Bottom);
	ReserveSlot->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 6.0f));

	StatusText = ZombieUI::MakeText(Tree, FText::GetEmpty(), 12, ZombieUI::Danger);
	StatusText->SetJustification(ETextJustify::Right);

	ReloadBar = ZombieUI::MakeBar(Tree, ZombieUI::Accent);
	ReloadBar->SetVisibility(ESlateVisibility::Hidden);

	SlotRow = Tree.ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

	Column->AddChildToVerticalBox(WeaponName)->SetHorizontalAlignment(HAlign_Right);
	Column->AddChildToVerticalBox(AmmoRow)->SetHorizontalAlignment(HAlign_Right);
	Column->AddChildToVerticalBox(StatusText)->SetHorizontalAlignment(HAlign_Right);
	Column->AddChildToVerticalBox(ZombieUI::MakeSized(Tree, ReloadBar, 260.0f, 8.0f))->SetPadding(FMargin(0.0f, 4.0f));
	Column->AddChildToVerticalBox(SlotRow)->SetHorizontalAlignment(HAlign_Right);

	Tree.RootWidget = ZombieUI::MakePanel(Tree, Column, FMargin(14.0f));
}

void UZombieWeaponPanelWidget::Unbind()
{
	if (UInventoryComponent* Inventory = BoundInventory.Get())
	{
		Inventory->OnInventoryChanged.Remove(InventoryHandle);
		Inventory->OnActiveWeaponChanged.Remove(ActiveHandle);
	}
	if (AZombieWeapon* Weapon = BoundWeapon.Get())
	{
		Weapon->OnAmmoChanged.Remove(AmmoHandle);
		Weapon->OnReloadChanged.Remove(ReloadHandle);
	}
	BoundInventory.Reset();
	BoundWeapon.Reset();
}

void UZombieWeaponPanelWidget::Observe(APawn* Pawn)
{
	Unbind();

	UInventoryComponent* Inventory = Pawn ? Pawn->FindComponentByClass<UInventoryComponent>() : nullptr;
	if (Inventory)
	{
		BoundInventory = Inventory;
		InventoryHandle = Inventory->OnInventoryChanged.AddUObject(this, &UZombieWeaponPanelWidget::HandleInventoryChanged);
		ActiveHandle = Inventory->OnActiveWeaponChanged.AddUObject(this, &UZombieWeaponPanelWidget::HandleActiveWeaponChanged);
	}
	HandleActiveWeaponChanged(Inventory ? Inventory->GetActiveWeapon() : nullptr);
}

void UZombieWeaponPanelWidget::NativeDestruct()
{
	Unbind();
	Super::NativeDestruct();
}

void UZombieWeaponPanelWidget::HandleInventoryChanged()
{
	// Slots changed (bought, sold, traded, upgraded) - the drawn weapon may have been replaced.
	UInventoryComponent* Inventory = BoundInventory.Get();
	AZombieWeapon* Active = Inventory ? Inventory->GetActiveWeapon() : nullptr;
	if (Active != BoundWeapon.Get())
	{
		HandleActiveWeaponChanged(Active);
		return;
	}
	RefreshWeaponInfo();
	RebuildSlots();
}

void UZombieWeaponPanelWidget::HandleActiveWeaponChanged(AZombieWeapon* NewWeapon)
{
	if (AZombieWeapon* Previous = BoundWeapon.Get())
	{
		Previous->OnAmmoChanged.Remove(AmmoHandle);
		Previous->OnReloadChanged.Remove(ReloadHandle);
	}

	BoundWeapon = NewWeapon;
	if (NewWeapon)
	{
		AmmoHandle = NewWeapon->OnAmmoChanged.AddUObject(this, &UZombieWeaponPanelWidget::HandleAmmoChanged);
		ReloadHandle = NewWeapon->OnReloadChanged.AddUObject(this, &UZombieWeaponPanelWidget::HandleReloadChanged);
	}

	RefreshWeaponInfo();
	RebuildSlots();
}

void UZombieWeaponPanelWidget::RefreshWeaponInfo()
{
	const AZombieWeapon* Weapon = BoundWeapon.Get();
	const UWeaponDataAsset* Definition = Weapon ? Weapon->GetDefinition() : nullptr;
	if (!Definition)
	{
		WeaponName->SetText(LOCTEXT("Unarmed", "UNARMED"));
		AmmoText->SetText(FText::GetEmpty());
		ReserveText->SetText(FText::GetEmpty());
		return;
	}

	if (!Definition->bUsesAmmo)
	{
		WeaponName->SetText(FText::Format(LOCTEXT("MeleeName", "{0}  [MELEE]"), Definition->DisplayName));
		WeaponName->SetColorAndOpacity(FSlateColor(ZombieUI::TextColor));
		AmmoText->SetText(LOCTEXT("NoAmmoNeeded", "--"));
		AmmoText->SetColorAndOpacity(FSlateColor(ZombieUI::TextColor));
		ReserveText->SetText(FText::GetEmpty());
		StatusText->SetText(FText::GetEmpty());
		ReloadBar->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	const FWeaponRarityTier Tier = UWeaponRaritySettings::GetOrLoadDefault()->GetTier(Weapon->GetRarity());
	const FText Name = Weapon->IsUpgraded()
		? FText::Format(LOCTEXT("UpgradedName", "{0}+"), Definition->DisplayName)
		: Definition->DisplayName;
	WeaponName->SetText(FText::Format(LOCTEXT("WeaponName", "{0}  [{1}]"), Name, Tier.DisplayName));
	WeaponName->SetColorAndOpacity(FSlateColor(Tier.Color));

	HandleAmmoChanged(Weapon->GetAmmoInMagazine(), Weapon->GetReserveAmmo());
	HandleReloadChanged(Weapon->IsReloading(), 0.0f);
}

void UZombieWeaponPanelWidget::HandleAmmoChanged(int32 AmmoInMagazine, int32 ReserveAmmo)
{
	const AZombieWeapon* Drawn = BoundWeapon.Get();
	if (Drawn && Drawn->GetDefinition() && !Drawn->GetDefinition()->bUsesAmmo)
	{
		return;
	}

	AmmoText->SetText(FText::AsNumber(AmmoInMagazine));
	AmmoText->SetColorAndOpacity(FSlateColor(AmmoInMagazine > 0 ? ZombieUI::TextColor : ZombieUI::Danger));
	ReserveText->SetText(FText::Format(LOCTEXT("Reserve", "/ {0}"), FText::AsNumber(ReserveAmmo)));

	const AZombieWeapon* Weapon = BoundWeapon.Get();
	const bool bEmpty = AmmoInMagazine <= 0 && ReserveAmmo <= 0;
	const bool bLow = Weapon && !Weapon->IsReloading() && AmmoInMagazine <= FMath::Max(Weapon->GetStats().MagazineSize / 4, 1);
	StatusText->SetText(bEmpty ? LOCTEXT("NoAmmo", "OUT OF AMMO") : bLow ? LOCTEXT("LowAmmo", "RELOAD [R]") : FText::GetEmpty());
}

void UZombieWeaponPanelWidget::HandleReloadChanged(bool bReloading, float Duration)
{
	ReloadBar->SetVisibility(bReloading ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	if (bReloading)
	{
		StatusText->SetText(LOCTEXT("Reloading", "RELOADING"));
	}
	else if (const AZombieWeapon* Weapon = BoundWeapon.Get())
	{
		HandleAmmoChanged(Weapon->GetAmmoInMagazine(), Weapon->GetReserveAmmo());
	}
}

void UZombieWeaponPanelWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const AZombieWeapon* Weapon = BoundWeapon.Get();
	if (Weapon && Weapon->IsReloading())
	{
		ReloadBar->SetPercent(Weapon->GetReloadProgress());
	}
}

void UZombieWeaponPanelWidget::RebuildSlots()
{
	SlotRow->ClearChildren();

	const UInventoryComponent* Inventory = BoundInventory.Get();
	if (!Inventory || !WidgetTree)
	{
		return;
	}

	for (int32 SlotIndex = 0; SlotIndex < Inventory->GetSlotCount(); ++SlotIndex)
	{
		const AZombieWeapon* Weapon = Inventory->GetWeaponAt(SlotIndex);
		const UWeaponDataAsset* Definition = Weapon ? Weapon->GetDefinition() : nullptr;
		const bool bActive = SlotIndex == Inventory->GetActiveIndex() && Weapon && !Inventory->IsMeleeActive();

		const FText Label = FText::Format(LOCTEXT("Slot", "{0} {1}"), FText::AsNumber(SlotIndex + 1),
			Definition ? Definition->DisplayName : LOCTEXT("EmptySlot", "--"));
		const FLinearColor Color = !Definition ? ZombieUI::TextDimColor
			: UWeaponRaritySettings::GetOrLoadDefault()->GetTier(Weapon->GetRarity()).Color;

		UTextBlock* Text = ZombieUI::MakeText(*WidgetTree, Label, 11, Color);
		UBorder* Box = ZombieUI::MakePanel(*WidgetTree, Text, FMargin(8.0f, 4.0f),
			bActive ? FLinearColor(0.25f, 0.4f, 0.12f, 0.95f) : FLinearColor(0.0f, 0.0f, 0.0f, 0.6f));
		SlotRow->AddChildToHorizontalBox(Box)->SetPadding(FMargin(4.0f, 0.0f, 0.0f, 0.0f));
	}

	if (const AZombieWeapon* Melee = Inventory->GetMeleeWeapon())
	{
		const UWeaponDataAsset* Definition = Melee->GetDefinition();
		const FText Label = FText::Format(LOCTEXT("MeleeSlot", "V {0}"), Definition ? Definition->DisplayName : LOCTEXT("MeleeFallback", "MELEE"));
		UTextBlock* Text = ZombieUI::MakeText(*WidgetTree, Label, 11, ZombieUI::TextColor);
		UBorder* Box = ZombieUI::MakePanel(*WidgetTree, Text, FMargin(8.0f, 4.0f),
			Inventory->IsMeleeActive() ? FLinearColor(0.25f, 0.4f, 0.12f, 0.95f) : FLinearColor(0.0f, 0.0f, 0.0f, 0.6f));
		SlotRow->AddChildToHorizontalBox(Box)->SetPadding(FMargin(12.0f, 0.0f, 0.0f, 0.0f));
	}
}

#undef LOCTEXT_NAMESPACE
