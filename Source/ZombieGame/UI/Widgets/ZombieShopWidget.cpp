#include "ZombieShopWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Characters/Player/ZombiePlayerController.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/InventoryComponent.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/ZombieGameState.h"
#include "Core/ZombiePlayerState.h"
#include "Perks/PerkComponent.h"
#include "Perks/PerkDataAsset.h"
#include "Shop/ShopPricing.h"
#include "Shop/ShopSettings.h"
#include "Shop/ShopTransactionComponent.h"
#include "Shop/ZombieShopState.h"
#include "UI/Widgets/ZombieActionButtonWidget.h"
#include "UI/ZombieUIStyle.h"
#include "Weapons/WeaponDataAsset.h"
#include "Weapons/WeaponRaritySettings.h"
#include "Weapons/WeaponStatsCalculator.h"
#include "Weapons/ZombieWeapon.h"

#define LOCTEXT_NAMESPACE "ZombieShop"

namespace
{
	FText DescribeStats(const FWeaponStats& Stats)
	{
		const FString Pellets = Stats.PelletsPerShot > 1 ? FString::Printf(TEXT("x%d"), Stats.PelletsPerShot) : FString();
		return FText::FromString(FString::Printf(TEXT("DMG %d%s  ROF %.1f/s  MAG %d  CRIT %d%%"),
			FMath::RoundToInt(Stats.Damage), *Pellets, Stats.ShotsPerSecond, Stats.MagazineSize, FMath::RoundToInt(Stats.CritChance * 100.0f)));
	}

	UVerticalBox* MakeColumn(UWidgetTree& Tree, UHorizontalBox& Row, const FText& Title, UVerticalBox*& OutList)
	{
		UVerticalBox* Column = Tree.ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Column->AddChildToVerticalBox(ZombieUI::MakeText(Tree, Title, 16, ZombieUI::MoneyColor))->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));

		OutList = Tree.ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		UScrollBox* Scroll = Tree.ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
		Scroll->AddChild(OutList);
		Column->AddChildToVerticalBox(ZombieUI::MakeSized(Tree, Scroll, 400.0f, 470.0f));

		UHorizontalBoxSlot* ChildSlot = Row.AddChildToHorizontalBox(ZombieUI::MakePanel(Tree, Column, FMargin(14.0f), ZombieUI::PanelLight));
		ChildSlot->SetPadding(FMargin(6.0f));
		return Column;
	}
}

void UZombieShopWidget::BuildWidget(UWidgetTree& Tree)
{
	UVerticalBox* Root = Tree.ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

	UHorizontalBox* Header = Tree.ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UHorizontalBoxSlot* TitleSlot = Header->AddChildToHorizontalBox(ZombieUI::MakeText(Tree, LOCTEXT("Title", "INTERMISSION MARKET"), 28, ZombieUI::Accent));
	TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	MoneyText = ZombieUI::MakeText(Tree, FText::GetEmpty(), 24, ZombieUI::MoneyColor);
	Header->AddChildToHorizontalBox(MoneyText)->SetVerticalAlignment(VAlign_Center);
	Root->AddChildToVerticalBox(Header)->SetPadding(FMargin(6.0f));

	UHorizontalBox* Columns = Tree.ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UVerticalBox* Offers = nullptr;
	UVerticalBox* Supplies = nullptr;
	UVerticalBox* Weapons = nullptr;
	MakeColumn(Tree, *Columns, LOCTEXT("ForSale", "FOR SALE"), Offers);
	MakeColumn(Tree, *Columns, LOCTEXT("Supplies", "SUPPLIES"), Supplies);
	MakeColumn(Tree, *Columns, LOCTEXT("YourGuns", "YOUR GUNS"), Weapons);
	OffersColumn = Offers;
	SuppliesColumn = Supplies;
	WeaponsColumn = Weapons;
	Root->AddChildToVerticalBox(Columns);

	StatusText = ZombieUI::MakeText(Tree, LOCTEXT("Welcome", "No enemies here. Take your time - every purchase counts."), 13, ZombieUI::TextDimColor);
	Root->AddChildToVerticalBox(StatusText)->SetPadding(FMargin(8.0f, 10.0f));

	UHorizontalBox* Footer = Tree.ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UButton* Close = ZombieUI::MakeButton(Tree, LOCTEXT("Close", "CLOSE"), 16);
	Close->OnClicked.AddDynamic(this, &UZombieShopWidget::HandleClose);
	UButton* Leave = ZombieUI::MakeButton(Tree, LOCTEXT("Leave", "LEAVE SECTOR >>"), 16);
	Leave->OnClicked.AddDynamic(this, &UZombieShopWidget::HandleLeaveSector);
	Footer->AddChildToHorizontalBox(ZombieUI::MakeSized(Tree, Close, 220.0f, -1.0f))->SetPadding(FMargin(6.0f));
	UHorizontalBoxSlot* Spacer = Footer->AddChildToHorizontalBox(Tree.ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass()));
	Spacer->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	Footer->AddChildToHorizontalBox(ZombieUI::MakeSized(Tree, Leave, 280.0f, -1.0f))->SetPadding(FMargin(6.0f));
	Root->AddChildToVerticalBox(Footer);
	CloseButton = Close;

	MakeBackdrop(Tree, ZombieUI::MakePanel(Tree, Root, FMargin(20.0f)), 0.7f);
}

void UZombieShopWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindSources();
	Refresh();
}

void UZombieShopWidget::NativeDestruct()
{
	UnbindSources();
	Super::NativeDestruct();
}

AZombieShopState* UZombieShopWidget::GetShop() const
{
	const AZombieGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AZombieGameState>() : nullptr;
	return GameState ? GameState->GetShop() : nullptr;
}

UInventoryComponent* UZombieShopWidget::GetInventory() const
{
	const APawn* Pawn = GetOwningPlayerPawn();
	return Pawn ? Pawn->FindComponentByClass<UInventoryComponent>() : nullptr;
}

void UZombieShopWidget::BindSources()
{
	APlayerController* PC = GetOwningPlayer();
	if (AZombiePlayerState* PlayerState = PC ? PC->GetPlayerState<AZombiePlayerState>() : nullptr)
	{
		BoundPlayerState = PlayerState;
		PlayerState->OnMoneyChanged.AddDynamic(this, &UZombieShopWidget::HandleMoneyChanged);
		if (UPerkComponent* Perks = PlayerState->GetPerks())
		{
			BoundPerks = Perks;
			PerksHandle = Perks->OnStatsChanged().AddUObject(this, &UZombieShopWidget::Refresh);
		}
	}

	if (AZombieShopState* Shop = GetShop())
	{
		BoundShop = Shop;
		StockHandle = Shop->OnStockChanged.AddUObject(this, &UZombieShopWidget::Refresh);
	}

	if (UInventoryComponent* Inventory = GetInventory())
	{
		BoundInventory = Inventory;
		InventoryHandle = Inventory->OnInventoryChanged.AddUObject(this, &UZombieShopWidget::Refresh);
	}

	if (const AZombiePlayerController* ZombiePC = Cast<AZombiePlayerController>(PC))
	{
		TransactionHandle = ZombiePC->GetShopTransactions()->OnTransactionResult.AddUObject(this, &UZombieShopWidget::HandleTransactionResult);
	}
}

void UZombieShopWidget::UnbindSources()
{
	if (AZombiePlayerState* PlayerState = BoundPlayerState.Get())
	{
		PlayerState->OnMoneyChanged.RemoveDynamic(this, &UZombieShopWidget::HandleMoneyChanged);
	}
	if (UPerkComponent* Perks = Cast<UPerkComponent>(BoundPerks.Get()))
	{
		Perks->OnStatsChanged().Remove(PerksHandle);
	}
	if (AZombieShopState* Shop = BoundShop.Get())
	{
		Shop->OnStockChanged.Remove(StockHandle);
	}
	if (UInventoryComponent* Inventory = BoundInventory.Get())
	{
		Inventory->OnInventoryChanged.Remove(InventoryHandle);
	}
	if (const AZombiePlayerController* ZombiePC = Cast<AZombiePlayerController>(GetOwningPlayer()))
	{
		ZombiePC->GetShopTransactions()->OnTransactionResult.Remove(TransactionHandle);
	}
}

void UZombieShopWidget::HandleMoneyChanged(int32 NewMoney)
{
	Refresh();
}

void UZombieShopWidget::HandleTransactionResult(bool bSuccess, const FText& Message)
{
	StatusText->SetText(Message);
	StatusText->SetColorAndOpacity(FSlateColor(bSuccess ? ZombieUI::Accent : ZombieUI::Danger));
	Refresh();
}

void UZombieShopWidget::Refresh()
{
	// Deferred to the next tick: a purchase changes money, stock and inventory in one go (and may
	// be raised from inside the very button click that is about to be rebuilt).
	bRefreshPending = true;
}

void UZombieShopWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (bRefreshPending)
	{
		bRefreshPending = false;
		RebuildAll();
	}
}

void UZombieShopWidget::RebuildAll()
{
	if (!WidgetTree || !MoneyText)
	{
		return;
	}

	const AZombiePlayerState* PlayerState = BoundPlayerState.Get();
	MoneyText->SetText(FText::Format(LOCTEXT("Money", "${0}"), FText::AsNumber(PlayerState ? PlayerState->GetMoney() : 0)));

	RebuildOffers();
	RebuildSupplies();
	RebuildOwnedWeapons();
}

UVerticalBox* UZombieShopWidget::AddRow(UVerticalBox& Column, const FText& Title, const FLinearColor& TitleColor, const FText& Detail)
{
	UVerticalBox* Row = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Row->AddChildToVerticalBox(ZombieUI::MakeText(*WidgetTree, Title, 14, TitleColor));
	if (!Detail.IsEmpty())
	{
		UTextBlock* DetailText = ZombieUI::MakeText(*WidgetTree, Detail, 10, ZombieUI::TextDimColor);
		DetailText->SetAutoWrapText(true);
		Row->AddChildToVerticalBox(DetailText)->SetPadding(FMargin(0.0f, 2.0f));
	}
	Column.AddChildToVerticalBox(ZombieUI::MakePanel(*WidgetTree, Row, FMargin(8.0f)))->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	return Row;
}

UZombieActionButtonWidget* UZombieShopWidget::AddButton(UPanelWidget& Container, const FText& Label, bool bEnabled, TFunction<void()> Action)
{
	UZombieActionButtonWidget* Button = WidgetTree->ConstructWidget<UZombieActionButtonWidget>(UZombieActionButtonWidget::StaticClass());
	Button->Setup(Label, 11, bEnabled);
	Button->OnPressed.BindWeakLambda(this, MoveTemp(Action));
	Container.AddChild(Button);
	return Button;
}

void UZombieShopWidget::RebuildOffers()
{
	OffersColumn->ClearChildren();

	const AZombieShopState* Shop = BoundShop.Get();
	const AZombiePlayerState* PlayerState = BoundPlayerState.Get();
	const UPerkComponent* Perks = PlayerState ? PlayerState->GetPerks() : nullptr;
	const AZombiePlayerController* PC = Cast<AZombiePlayerController>(GetOwningPlayer());
	if (!Shop || !PlayerState || !PC)
	{
		return;
	}

	const UShopSettings* Settings = UShopSettings::GetOrLoadDefault();
	const UWeaponRaritySettings* Rarity = UWeaponRaritySettings::GetOrLoadDefault();
	UShopTransactionComponent* Transactions = PC->GetShopTransactions();

	for (const FShopOffer& Offer : Shop->GetOffers())
	{
		FText Title;
		FText Detail;
		FLinearColor Color = ZombieUI::TextColor;
		bool bAvailable = !Offer.bSoldOut;
		const int32 OwnedTier = (Offer.Type == EShopOfferType::Perk && Perks) ? Perks->GetTier(Offer.Perk) : 0;
		const int32 Price = FShopPricing::GetOfferPrice(*Settings, Offer, Shop->GetSector(), OwnedTier);

		if (Offer.Type == EShopOfferType::Weapon)
		{
			const UWeaponDataAsset* Weapon = Offer.Weapon.Definition.LoadSynchronous();
			if (!Weapon)
			{
				continue;
			}
			const FWeaponRarityTier Tier = Rarity->GetTier(Offer.Weapon.Rarity);
			Title = FText::Format(LOCTEXT("OfferWeapon", "{0} [{1}]"), Weapon->DisplayName, Tier.DisplayName);
			Color = Tier.Color;
			Detail = DescribeStats(FWeaponStatsCalculator::Compute(*Weapon, Offer.Weapon.Rarity, 0, *Rarity, nullptr));
		}
		else if (Offer.Perk)
		{
			const bool bMaxed = OwnedTier >= Offer.Perk->MaxTier;
			bAvailable = !bMaxed;
			Title = FText::Format(LOCTEXT("OfferPerk", "{0}  {1}/{2}"), Offer.Perk->DisplayName, FText::AsNumber(OwnedTier), FText::AsNumber(Offer.Perk->MaxTier));
			Color = Offer.Perk->IconTint;
			Detail = Offer.Perk->DescribeTier(FMath::Min(OwnedTier + 1, Offer.Perk->MaxTier));
		}

		UVerticalBox* Row = AddRow(*OffersColumn, Title, Color, Detail);
		const FText Label = !bAvailable
			? (Offer.bSoldOut ? LOCTEXT("SoldOut", "SOLD OUT") : LOCTEXT("Maxed", "MAXED"))
			: FText::Format(LOCTEXT("Buy", "BUY  ${0}"), FText::AsNumber(Price));
		const int32 OfferId = Offer.OfferId;
		AddButton(*Row, Label, bAvailable && PlayerState->GetMoney() >= Price, [Transactions, OfferId]() { Transactions->BuyOffer(OfferId); });
	}
}

void UZombieShopWidget::RebuildSupplies()
{
	SuppliesColumn->ClearChildren();

	const AZombieShopState* Shop = BoundShop.Get();
	const AZombiePlayerState* PlayerState = BoundPlayerState.Get();
	const AZombiePlayerController* PC = Cast<AZombiePlayerController>(GetOwningPlayer());
	const UInventoryComponent* Inventory = BoundInventory.Get();
	if (!Shop || !PlayerState || !PC)
	{
		return;
	}

	const UShopSettings* Settings = UShopSettings::GetOrLoadDefault();
	UShopTransactionComponent* Transactions = PC->GetShopTransactions();
	const int32 Sector = Shop->GetSector();

	const auto AddSupply = [&](const FText& Title, const FText& Detail, int32 Price, bool bAvailable, EShopService Service)
	{
		UVerticalBox* Row = AddRow(*SuppliesColumn, Title, ZombieUI::TextColor, Detail);
		AddButton(*Row, FText::Format(LOCTEXT("BuySupply", "BUY  ${0}"), FText::AsNumber(Price)),
			bAvailable && PlayerState->GetMoney() >= Price, [Transactions, Service]() { Transactions->BuyService(Service); });
	};

	AddSupply(LOCTEXT("Armor", "Armor Plate"), FText::Format(LOCTEXT("ArmorDetail", "+{0} armor. Absorbs damage before health."), FText::AsNumber(Settings->ArmorAmount)),
		FShopPricing::GetServicePrice(*Settings, EShopService::Armor, Sector), true, EShopService::Armor);
	AddSupply(LOCTEXT("MedKit", "Med Kit"), FText::Format(LOCTEXT("MedDetail", "Restores {0} health. Health never regenerates on its own."), FText::AsNumber(Settings->MedKitHealAmount)),
		FShopPricing::GetServicePrice(*Settings, EShopService::MedKit, Sector), true, EShopService::MedKit);
	AddSupply(LOCTEXT("Box", "Mystery Box"), LOCTEXT("BoxDetail", "A random weapon, ammo, money, armor or healing. Small chance of a Legendary or a perk."),
		FShopPricing::GetServicePrice(*Settings, EShopService::MysteryBox, Sector), true, EShopService::MysteryBox);

	if (Inventory)
	{
		AddSupply(LOCTEXT("Slot", "Extra Gun Slot"),
			FText::Format(LOCTEXT("SlotDetail", "Carry one more gun ({0}/{1})."), FText::AsNumber(Inventory->GetSlotCount()), FText::AsNumber(Inventory->GetMaxSlotCount())),
			FShopPricing::GetSlotUpgradePrice(*Settings, Inventory->GetSlotCount(), Sector), Inventory->CanAddSlot(), EShopService::SlotUpgrade);
	}
}

void UZombieShopWidget::RebuildOwnedWeapons()
{
	WeaponsColumn->ClearChildren();

	const AZombieShopState* Shop = BoundShop.Get();
	const AZombiePlayerState* PlayerState = BoundPlayerState.Get();
	const AZombiePlayerController* PC = Cast<AZombiePlayerController>(GetOwningPlayer());
	const UInventoryComponent* Inventory = BoundInventory.Get();
	if (!Shop || !PlayerState || !PC || !Inventory)
	{
		return;
	}

	const UShopSettings* Settings = UShopSettings::GetOrLoadDefault();
	const UWeaponRaritySettings* Rarity = UWeaponRaritySettings::GetOrLoadDefault();
	UShopTransactionComponent* Transactions = PC->GetShopTransactions();
	const int32 Money = PlayerState->GetMoney();

	for (int32 SlotIndex = 0; SlotIndex < Inventory->GetWeaponCount(); ++SlotIndex)
	{
		const AZombieWeapon* Weapon = Inventory->GetWeaponAt(SlotIndex);
		const UWeaponDataAsset* Definition = Weapon ? Weapon->GetDefinition() : nullptr;
		if (!Definition)
		{
			continue;
		}

		const FWeaponRarityTier Tier = Rarity->GetTier(Weapon->GetRarity());
		const FText Title = FText::Format(LOCTEXT("OwnedWeapon", "{0}{1} [{2}]"), Definition->DisplayName,
			Weapon->IsUpgraded() ? FText::FromString(TEXT("+")) : FText::GetEmpty(), Tier.DisplayName);
		const FText Detail = FText::Format(LOCTEXT("OwnedDetail", "Ammo {0} / {1}   {2}"), FText::AsNumber(Weapon->GetAmmoInMagazine()),
			FText::AsNumber(Weapon->GetReserveAmmo()), DescribeStats(Weapon->GetStats()));

		UVerticalBox* Row = AddRow(*WeaponsColumn, Title, Tier.Color, Detail);
		UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Row->AddChildToVerticalBox(Buttons);

		const int32 RefillPrice = FShopPricing::GetAmmoRefillPrice(*Settings, *Weapon, Shop->GetSector());
		const int32 UpgradePrice = FShopPricing::GetUpgradePrice(*Settings, *Weapon, Shop->GetSector());
		const int32 SellPrice = FShopPricing::GetSellPrice(*Weapon);

		AddButton(*Buttons, RefillPrice > 0 ? FText::Format(LOCTEXT("Refill", "AMMO ${0}"), FText::AsNumber(RefillPrice)) : LOCTEXT("Full", "FULL"),
			RefillPrice > 0 && Money >= RefillPrice, [Transactions, SlotIndex]() { Transactions->BuyService(EShopService::AmmoRefill, SlotIndex); });
		AddButton(*Buttons, Weapon->IsUpgraded() ? LOCTEXT("Upgraded", "UPGRADED") : FText::Format(LOCTEXT("Upgrade", "UPGRADE ${0}"), FText::AsNumber(UpgradePrice)),
			!Weapon->IsUpgraded() && Money >= UpgradePrice, [Transactions, SlotIndex]() { Transactions->BuyService(EShopService::WeaponUpgrade, SlotIndex); });
		AddButton(*Buttons, FText::Format(LOCTEXT("Sell", "SELL +${0}"), FText::AsNumber(SellPrice)),
			Inventory->GetWeaponCount() > 1, [Transactions, SlotIndex]() { Transactions->BuyService(EShopService::SellWeapon, SlotIndex); });
	}
}

UWidget* UZombieShopWidget::GetInitialFocus() const
{
	return CloseButton;
}

void UZombieShopWidget::HandleClose()
{
	PlayUISound(TEXT("UI.Click"));
	CloseMenu();
}

void UZombieShopWidget::HandleLeaveSector()
{
	PlayUISound(TEXT("UI.Click"));
	if (AZombiePlayerController* PC = Cast<AZombiePlayerController>(GetOwningPlayer()))
	{
		PC->ServerLeaveIntermission();
	}
}

#undef LOCTEXT_NAMESPACE
