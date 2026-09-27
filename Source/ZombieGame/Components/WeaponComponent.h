#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WeaponComponent.generated.h"

class AZombieWeapon;
class UInventoryComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnOwnerWeaponFired, float /*ShakeStrength*/);

/**
 * The Weapon System: turns trigger/reload/slot intents into actions on whichever gun the inventory
 * has drawn. Holds only input state (is the trigger held) - ammo, fire rate and reload rules all
 * belong to the weapon actor.
 *
 * Intents go through Server RPCs, so the same component works unchanged for a remote player once
 * multiplayer exists; in single player the RPCs execute locally.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ZOMBIEGAME_API UWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWeaponComponent();

	void StartFire();
	void StopFire();
	void Reload();
	void SelectSlot(int32 SlotIndex);
	void SelectMelee();
	void CycleWeapon(int32 Direction);

	/** Disables firing entirely (shop open, dead, paused). */
	void SetWeaponsBlocked(bool bBlocked);

	AZombieWeapon* GetActiveWeapon() const;

	/** Raised for every shot the owner fires - camera shake, recoil, HUD kick. */
	FOnOwnerWeaponFired OnWeaponFired;

protected:
	virtual void BeginPlay() override;

	/** Holding fire on a semi-automatic gun keeps shooting (accessibility option). */
	UPROPERTY(EditDefaultsOnly, Category = "Weapons")
	bool bAutoFireSemiAutomatic = false;

private:
	UFUNCTION(Server, Reliable)
	void ServerSetTriggerHeld(bool bHeld);

	UFUNCTION(Server, Reliable)
	void ServerReload();

	UFUNCTION(Server, Reliable)
	void ServerSelectSlot(int32 SlotIndex);

	UFUNCTION(Server, Reliable)
	void ServerCycleWeapon(int32 Direction);

	UFUNCTION(Server, Reliable)
	void ServerSelectMelee();

	void SetTriggerHeld(bool bHeld);

	/** One trigger evaluation; reschedules itself while the trigger is held on an automatic gun. */
	void FireTick();

	void HandleActiveWeaponChanged(AZombieWeapon* NewWeapon);
	void HandleWeaponFired(float ShakeStrength);

	UPROPERTY(Transient)
	TObjectPtr<UInventoryComponent> Inventory;

	TWeakObjectPtr<AZombieWeapon> BoundWeapon;
	FDelegateHandle FiredHandle;
	FTimerHandle RefireTimer;
	bool bTriggerHeld = false;
	bool bShotFiredThisPress = false;
	bool bBlocked = false;
};
