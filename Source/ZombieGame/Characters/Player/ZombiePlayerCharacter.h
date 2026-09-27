#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ZombiePlayerCharacter.generated.h"

class AZombiePlayerCharacter;
class AZombieWeapon;
class UCameraComponent;
class UDamageComponent;
class UDamageType;
class UHealthComponent;
class UInteractionComponent;
class UInventoryComponent;
class UPixelSpriteComponent;
class UScreenShakeComponent;
class USpringArmComponent;
class USpriteSheetDataAsset;
class UStaminaComponent;
class UStatusEffectComponent;
class UWeaponComponent;
class UZombieInputConfig;
struct FInputActionValue;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnPlayerCharacterDied, AZombiePlayerCharacter* /*Character*/);

/**
 * The player's body: a composition of components (health, stamina, inventory, weapons,
 * interaction, status effects) plus the top-down camera and pixel-art sprites.
 *
 * Gameplay lives in the components; this class translates input into component intents, keeps the
 * sprite in sync with movement, and re-applies perk-driven stats to its components when they change.
 */
UCLASS()
class ZOMBIEGAME_API AZombiePlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AZombiePlayerCharacter();

	virtual void Tick(float DeltaTime) override;

	/** Where the player is aiming, on the play plane - the HUD's crosshair follows this. */
	FVector GetAimPoint() const { return AimPoint; }

	/** True while aiming with a gamepad stick rather than the mouse. */
	bool IsUsingGamepadAim() const { return bGamepadAim; }

	bool IsDead() const { return bDead; }

	/** Stops all gameplay input without killing the pawn - shop menus, pause, cutscenes. */
	void SetGameplayInputBlocked(bool bBlocked);

	void AddScreenShake(float Trauma);

	FOnPlayerCharacterDied OnPlayerDied;

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void OnPlayerStateChanged(APlayerState* NewPlayerState, APlayerState* OldPlayerState) override;
	virtual void PawnClientRestart() override;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> TopDownCamera;

	// -90 = looking straight down. Kept off -90 for a slight angled top-down look (readable
	// silhouettes/depth) rather than a flat orthographic-feeling top-down.
	UPROPERTY(EditDefaultsOnly, Category = "Camera", meta = (ClampMin = "-90.0", ClampMax = "0.0"))
	float CameraPitch = -75.0f;

	/** Visible world width. Wide enough that a reasonably sized room fits on screen. */
	UPROPERTY(EditDefaultsOnly, Category = "Camera", meta = (ClampMin = "500.0"))
	float CameraOrthoWidth = 2300.0f;

	UPROPERTY(VisibleAnywhere, Category = "Visual")
	TObjectPtr<UPixelSpriteComponent> BodySprite;

	/** The held gun, drawn over the body from the weapon overlay sheet. */
	UPROPERTY(VisibleAnywhere, Category = "Visual")
	TObjectPtr<UPixelSpriteComponent> WeaponSprite;

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	TSoftObjectPtr<USpriteSheetDataAsset> BodySheet;

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	TSoftObjectPtr<USpriteSheetDataAsset> WeaponSheet;

	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UHealthComponent> HealthComponent;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UStaminaComponent> StaminaComponent;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UDamageComponent> DamageComponent;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UInteractionComponent> InteractionComponent;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UInventoryComponent> InventoryComponent;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UWeaponComponent> WeaponComponent;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UStatusEffectComponent> StatusEffectComponent;
	UPROPERTY(VisibleAnywhere, Category = "Components") TObjectPtr<UScreenShakeComponent> ScreenShakeComponent;

	UPROPERTY(VisibleAnywhere, Category = "Input")
	TObjectPtr<UZombieInputConfig> InputConfig;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float WalkSpeed = 400.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float SprintSpeed = 700.0f;

	/** Unmodified maximum health; the Extra Health perk adds to it. */
	UPROPERTY(EditDefaultsOnly, Category = "Health")
	float BaseMaxHealth = 100.0f;

private:
	void HandleMove(const FInputActionValue& Value);
	void HandleAimStick(const FInputActionValue& Value);
	void HandleSprintStarted(const FInputActionValue& Value);
	void HandleSprintStopped(const FInputActionValue& Value);
	void HandleFireStarted(const FInputActionValue& Value);
	void HandleFireStopped(const FInputActionValue& Value);
	void HandleReload(const FInputActionValue& Value);
	void HandleInteract(const FInputActionValue& Value);
	void HandleWeaponSlot(const FInputActionValue& Value);
	void HandleCycleWeapon(const FInputActionValue& Value);
	void HandleMelee(const FInputActionValue& Value);

	void UpdateMouseAim();
	void ApplyFacing(const FVector& Direction);
	void UpdateSpriteAnimation();
	void UpdateMovementSpeed();

	/** Re-applies perk-driven values to movement, stamina, health and weapons. */
	void HandleStatsChanged();
	void HandleActiveWeaponChanged(AZombieWeapon* NewWeapon);
	void HandleWeaponFired(float ShakeStrength);
	void HandleDamageReceived(float Amount, AActor* Causer, const UDamageType* DamageType);
	void HandleStatusEffectsChanged();

	UFUNCTION()
	void HandleDeath();

	FVector AimPoint = FVector::ZeroVector;
	float MoveSpeedMultiplier = 1.0f;
	bool bGamepadAim = false;
	bool bDead = false;
	bool bInputBlocked = false;
	FDelegateHandle StatsChangedHandle;
	TWeakObjectPtr<UObject> BoundStatSource;
};
