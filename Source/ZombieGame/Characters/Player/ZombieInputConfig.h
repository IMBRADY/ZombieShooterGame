#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ZombieInputConfig.generated.h"

class UInputAction;
class UInputMappingContext;

/**
 * The player's Enhanced Input actions and their default keyboard/mouse and gamepad bindings,
 * built in code so input setup lives in source control with the rest of gameplay (as it has since
 * Milestone 2), and kept out of the character so the character stays about composing components.
 *
 * Controls follow the design brief: WASD move, mouse aim, LMB shoot, R reload, Left Shift sprint,
 * 1/2/3 weapon slots, E interact, Escape pause. Controller bindings mirror them twin-stick style.
 */
UCLASS()
class ZOMBIEGAME_API UZombieInputConfig : public UObject
{
	GENERATED_BODY()

public:
	UZombieInputConfig();

	/** Pawn actions: movement, combat, interaction. Removed while the pawn is dead. */
	UPROPERTY()
	TObjectPtr<UInputMappingContext> GameplayContext;

	/** Always-on actions owned by the controller (pause). */
	UPROPERTY()
	TObjectPtr<UInputMappingContext> GlobalContext;

	UPROPERTY() TObjectPtr<UInputAction> Move;
	UPROPERTY() TObjectPtr<UInputAction> AimStick;
	UPROPERTY() TObjectPtr<UInputAction> Sprint;
	UPROPERTY() TObjectPtr<UInputAction> Fire;
	UPROPERTY() TObjectPtr<UInputAction> Reload;
	UPROPERTY() TObjectPtr<UInputAction> Interact;

	/** Axis1D whose value is the slot number (1-5), via a scalar modifier per key. */
	UPROPERTY() TObjectPtr<UInputAction> WeaponSlot;

	/** Axis1D: +1 next weapon, -1 previous (mouse wheel, shoulder buttons). */
	UPROPERTY() TObjectPtr<UInputAction> CycleWeapon;

	UPROPERTY() TObjectPtr<UInputAction> Pause;

private:
	void BindMovement();
	void BindCombat();
	void BindWeaponSlots();
	void BindGlobal();
};
