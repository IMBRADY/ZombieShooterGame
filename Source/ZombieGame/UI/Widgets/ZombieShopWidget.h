#pragma once

#include "CoreMinimal.h"
#include "Shop/ShopTypes.h"
#include "UI/ZombieWidgetBase.h"
#include "ZombieShopWidget.generated.h"

class AZombiePlayerState;
class AZombieShopState;
class UInventoryComponent;
class UPanelWidget;
class UTextBlock;
class UVerticalBox;
class UZombieActionButtonWidget;

/**
 * The intermission shop screen. Observes the shared stock, the player's money, inventory and perks,
 * and redraws whenever any of them changes; every button only *requests* a transaction through
 * UShopTransactionComponent, which reports back the outcome shown on the status line.
 */
UCLASS()
class ZOMBIEGAME_API UZombieShopWidget : public UZombieMenuWidget
{
	GENERATED_BODY()

public:
	virtual UWidget* GetInitialFocus() const override;

protected:
	virtual void BuildWidget(UWidgetTree& Tree) override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UFUNCTION() void HandleMoneyChanged(int32 NewMoney);
	UFUNCTION() void HandleClose();
	UFUNCTION() void HandleLeaveSector();

	void HandleTransactionResult(bool bSuccess, const FText& Message);
	void Refresh();
	void RebuildAll();

	void RebuildOffers();
	void RebuildSupplies();
	void RebuildOwnedWeapons();

	/** A row: title, detail line, and one or more action buttons. Returns the buttons' row for adding more. */
	UVerticalBox* AddRow(UVerticalBox& Column, const FText& Title, const FLinearColor& TitleColor, const FText& Detail);
	UZombieActionButtonWidget* AddButton(UPanelWidget& Container, const FText& Label, bool bEnabled, TFunction<void()> Action);

	void BindSources();
	void UnbindSources();

	AZombieShopState* GetShop() const;
	UInventoryComponent* GetInventory() const;

	UPROPERTY(Transient) TObjectPtr<UTextBlock> MoneyText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> OffersColumn;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> SuppliesColumn;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> WeaponsColumn;
	UPROPERTY(Transient) TObjectPtr<UWidget> CloseButton;

	TWeakObjectPtr<AZombiePlayerState> BoundPlayerState;
	TWeakObjectPtr<AZombieShopState> BoundShop;
	TWeakObjectPtr<UInventoryComponent> BoundInventory;
	TWeakObjectPtr<UObject> BoundPerks;
	FDelegateHandle StockHandle;
	FDelegateHandle InventoryHandle;
	FDelegateHandle PerksHandle;
	FDelegateHandle TransactionHandle;
	bool bRefreshPending = false;
};
