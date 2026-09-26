#pragma once

#include "CoreMinimal.h"
#include "UI/ZombieWidgetBase.h"
#include "ZombieWeaponPanelWidget.generated.h"

class AZombieWeapon;
class UHorizontalBox;
class UInventoryComponent;
class UProgressBar;
class UTextBlock;

/**
 * Current weapon, ammo, reload indicator and weapon slots - bottom-right of the HUD. Rebuilds on
 * inventory events; only the reload bar is sampled per frame, and only while a reload runs.
 */
UCLASS()
class ZOMBIEGAME_API UZombieWeaponPanelWidget : public UZombieWidgetBase
{
	GENERATED_BODY()

public:
	void Observe(APawn* Pawn);

protected:
	virtual void BuildWidget(UWidgetTree& Tree) override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;

private:
	void HandleInventoryChanged();
	void HandleActiveWeaponChanged(AZombieWeapon* NewWeapon);
	void HandleAmmoChanged(int32 AmmoInMagazine, int32 ReserveAmmo);
	void HandleReloadChanged(bool bReloading, float Duration);

	void RebuildSlots();
	void RefreshWeaponInfo();
	void Unbind();

	UPROPERTY(Transient) TObjectPtr<UTextBlock> WeaponName;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> AmmoText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ReserveText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(Transient) TObjectPtr<UProgressBar> ReloadBar;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> SlotRow;

	TWeakObjectPtr<UInventoryComponent> BoundInventory;
	TWeakObjectPtr<AZombieWeapon> BoundWeapon;
	FDelegateHandle InventoryHandle;
	FDelegateHandle ActiveHandle;
	FDelegateHandle AmmoHandle;
	FDelegateHandle ReloadHandle;
};
