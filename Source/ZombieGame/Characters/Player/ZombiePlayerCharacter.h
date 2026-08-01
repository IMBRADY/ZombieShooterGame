#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ZombiePlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;
class UHealthComponent;
class UStaminaComponent;
class UDamageComponent;
class UInteractionComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

UCLASS()
class ZOMBIEGAME_API AZombiePlayerCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AZombiePlayerCharacter();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> TopDownCamera;

	// Greybox placeholder so the character is visible before real sprite/flipbook art exists
	// (see ARCHITECTURE.md 2 for the intended 2.5D pixel-art-over-3D-environment direction).
	UPROPERTY(VisibleAnywhere, Category = "Visual")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UStaminaComponent> StaminaComponent;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UDamageComponent> DamageComponent;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UInteractionComponent> InteractionComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float WalkSpeed = 400.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float SprintSpeed = 700.0f;

	UPROPERTY()
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY()
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY()
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY()
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY()
	TObjectPtr<UInputAction> PauseAction;

private:
	void HandleMove(const FInputActionValue& Value);
	void HandleSprintStarted(const FInputActionValue& Value);
	void HandleSprintStopped(const FInputActionValue& Value);
	void HandleInteract(const FInputActionValue& Value);
	void HandlePause(const FInputActionValue& Value);
	void UpdateAimRotation();
};
