#include "ZombiePlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "Components/HealthComponent.h"
#include "Components/StaminaComponent.h"
#include "Components/DamageComponent.h"
#include "Components/InteractionComponent.h"

AZombiePlayerCharacter::AZombiePlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 1200.0f;
	CameraBoom->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;

	TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCamera->ProjectionMode = ECameraProjectionMode::Orthographic;
	TopDownCamera->OrthoWidth = 2000.0f;
	TopDownCamera->bUsePawnControlRotation = false;

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	StaminaComponent = CreateDefaultSubobject<UStaminaComponent>(TEXT("StaminaComponent"));
	DamageComponent = CreateDefaultSubobject<UDamageComponent>(TEXT("DamageComponent"));
	InteractionComponent = CreateDefaultSubobject<UInteractionComponent>(TEXT("InteractionComponent"));

	MoveAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_Move"));
	MoveAction->ValueType = EInputActionValueType::Axis2D;

	SprintAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_Sprint"));
	SprintAction->ValueType = EInputActionValueType::Boolean;

	InteractAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_Interact"));
	InteractAction->ValueType = EInputActionValueType::Boolean;

	PauseAction = CreateDefaultSubobject<UInputAction>(TEXT("IA_Pause"));
	PauseAction->ValueType = EInputActionValueType::Boolean;

	DefaultMappingContext = CreateDefaultSubobject<UInputMappingContext>(TEXT("IMC_Default"));

	// D: raw axis lands on X by default, which is exactly "right" here - no modifier needed.
	DefaultMappingContext->MapKey(MoveAction, EKeys::D);
	{
		// Modifiers are UObjects created during CDO construction - must go through
		// CreateDefaultSubobject (unique name required), not NewObject, or the engine fatals
		// with "NewObject with empty name can't be used to create default subobjects".
		FEnhancedActionKeyMapping& Mapping = DefaultMappingContext->MapKey(MoveAction, EKeys::A);
		Mapping.Modifiers.Add(CreateDefaultSubobject<UInputModifierNegate>(TEXT("MoveNegateA")));
	}
	{
		// W/S: swizzle the raw X output onto Y so they drive forward/back instead of left/right.
		FEnhancedActionKeyMapping& Mapping = DefaultMappingContext->MapKey(MoveAction, EKeys::W);
		UInputModifierSwizzleAxis* Swizzle = CreateDefaultSubobject<UInputModifierSwizzleAxis>(TEXT("MoveSwizzleW"));
		Swizzle->Order = EInputAxisSwizzle::YXZ;
		Mapping.Modifiers.Add(Swizzle);
	}
	{
		FEnhancedActionKeyMapping& Mapping = DefaultMappingContext->MapKey(MoveAction, EKeys::S);
		UInputModifierSwizzleAxis* Swizzle = CreateDefaultSubobject<UInputModifierSwizzleAxis>(TEXT("MoveSwizzleS"));
		Swizzle->Order = EInputAxisSwizzle::YXZ;
		Mapping.Modifiers.Add(Swizzle);
		Mapping.Modifiers.Add(CreateDefaultSubobject<UInputModifierNegate>(TEXT("MoveNegateS")));
	}

	DefaultMappingContext->MapKey(SprintAction, EKeys::LeftShift);
	DefaultMappingContext->MapKey(InteractAction, EKeys::E);
	DefaultMappingContext->MapKey(PauseAction, EKeys::Escape);
}

void AZombiePlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->bShowMouseCursor = true;

		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void AZombiePlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AZombiePlayerCharacter::HandleMove);
		EnhancedInput->BindAction(SprintAction, ETriggerEvent::Started, this, &AZombiePlayerCharacter::HandleSprintStarted);
		EnhancedInput->BindAction(SprintAction, ETriggerEvent::Completed, this, &AZombiePlayerCharacter::HandleSprintStopped);
		EnhancedInput->BindAction(SprintAction, ETriggerEvent::Canceled, this, &AZombiePlayerCharacter::HandleSprintStopped);
		EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &AZombiePlayerCharacter::HandleInteract);
		EnhancedInput->BindAction(PauseAction, ETriggerEvent::Started, this, &AZombiePlayerCharacter::HandlePause);
	}
}

void AZombiePlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (StaminaComponent)
	{
		GetCharacterMovement()->MaxWalkSpeed = StaminaComponent->IsSprinting() ? SprintSpeed : WalkSpeed;
	}

	UpdateAimRotation();
}

void AZombiePlayerCharacter::HandleMove(const FInputActionValue& Value)
{
	// World-space, not actor-relative: aim (mouse) and movement (WASD) are decoupled, twin-stick
	// style, so movement must not rotate with the character's facing.
	const FVector2D MoveInput = Value.Get<FVector2D>();

	if (!FMath::IsNearlyZero(MoveInput.X))
	{
		AddMovementInput(FVector::RightVector, MoveInput.X);
	}
	if (!FMath::IsNearlyZero(MoveInput.Y))
	{
		AddMovementInput(FVector::ForwardVector, MoveInput.Y);
	}
}

void AZombiePlayerCharacter::HandleSprintStarted(const FInputActionValue& Value)
{
	if (StaminaComponent)
	{
		StaminaComponent->SetSprinting(true);
	}
}

void AZombiePlayerCharacter::HandleSprintStopped(const FInputActionValue& Value)
{
	if (StaminaComponent)
	{
		StaminaComponent->SetSprinting(false);
	}
}

void AZombiePlayerCharacter::HandleInteract(const FInputActionValue& Value)
{
	if (InteractionComponent)
	{
		InteractionComponent->TryInteract();
	}
}

void AZombiePlayerCharacter::HandlePause(const FInputActionValue& Value)
{
	const bool bNewPaused = !UGameplayStatics::IsGamePaused(this);
	UGameplayStatics::SetGamePaused(this, bNewPaused);
}

void AZombiePlayerCharacter::UpdateAimRotation()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	FVector WorldLocation, WorldDirection;
	if (!PC->DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
	{
		return;
	}

	if (FMath::IsNearlyZero(WorldDirection.Z))
	{
		return;
	}

	const float ActorZ = GetActorLocation().Z;
	const float T = (ActorZ - WorldLocation.Z) / WorldDirection.Z;
	const FVector AimPoint = WorldLocation + WorldDirection * T;

	FVector LookDirection = AimPoint - GetActorLocation();
	LookDirection.Z = 0.0f;

	if (!LookDirection.IsNearlyZero())
	{
		SetActorRotation(LookDirection.Rotation());
	}
}
