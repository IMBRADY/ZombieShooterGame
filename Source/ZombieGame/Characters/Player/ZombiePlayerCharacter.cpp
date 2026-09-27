#include "ZombiePlayerCharacter.h"
#include "Audio/ZombieAudioSubsystem.h"
#include "Camera/CameraComponent.h"
#include "Characters/Player/ZombieInputConfig.h"
#include "Components/CapsuleComponent.h"
#include "Components/DamageComponent.h"
#include "Components/HealthComponent.h"
#include "Components/InteractionComponent.h"
#include "Components/InventoryComponent.h"
#include "Components/ScreenShakeComponent.h"
#include "Components/StaminaComponent.h"
#include "Components/StatusEffectComponent.h"
#include "Components/WeaponComponent.h"
#include "Core/ZombieGameInstance.h"
#include "Core/ZombieGameplayTags.h"
#include "Core/ZombiePlayerState.h"
#include "Core/ZombieStatSource.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"
#include "Visual/PixelSpriteComponent.h"
#include "Visual/SpriteSheetDataAsset.h"
#include "Weapons/WeaponDataAsset.h"
#include "Weapons/ZombieWeapon.h"

namespace
{
	/** Sprites sit just above the floor so walls and cover correctly occlude them. */
	constexpr float SpriteHeightAboveFeet = 12.0f;
	constexpr float WalkAnimationSpeedThreshold = 20.0f;
}

AZombiePlayerCharacter::AZombiePlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 1600.0f;
	CameraBoom->SetRelativeRotation(FRotator(CameraPitch, 0.0f, 0.0f));
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;

	TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCamera->ProjectionMode = ECameraProjectionMode::Orthographic;
	TopDownCamera->OrthoWidth = CameraOrthoWidth;
	TopDownCamera->bUsePawnControlRotation = false;

	const float FeetOffset = -GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() + SpriteHeightAboveFeet;

	BodySprite = CreateDefaultSubobject<UPixelSpriteComponent>(TEXT("BodySprite"));
	BodySprite->SetupAttachment(GetCapsuleComponent());
	BodySprite->SetRelativeLocation(FVector(0.0f, 0.0f, FeetOffset));

	WeaponSprite = CreateDefaultSubobject<UPixelSpriteComponent>(TEXT("WeaponSprite"));
	WeaponSprite->SetupAttachment(GetCapsuleComponent());
	WeaponSprite->SetRelativeLocation(FVector(0.0f, 0.0f, FeetOffset + 1.0f));

	BodySheet = TSoftObjectPtr<USpriteSheetDataAsset>(FSoftObjectPath(TEXT("/Game/Sprites/Characters/SS_Player.SS_Player")));
	WeaponSheet = TSoftObjectPtr<USpriteSheetDataAsset>(FSoftObjectPath(TEXT("/Game/Sprites/Characters/SS_PlayerWeapons.SS_PlayerWeapons")));

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	StaminaComponent = CreateDefaultSubobject<UStaminaComponent>(TEXT("StaminaComponent"));
	DamageComponent = CreateDefaultSubobject<UDamageComponent>(TEXT("DamageComponent"));
	InteractionComponent = CreateDefaultSubobject<UInteractionComponent>(TEXT("InteractionComponent"));
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("InventoryComponent"));
	WeaponComponent = CreateDefaultSubobject<UWeaponComponent>(TEXT("WeaponComponent"));
	StatusEffectComponent = CreateDefaultSubobject<UStatusEffectComponent>(TEXT("StatusEffectComponent"));
	ScreenShakeComponent = CreateDefaultSubobject<UScreenShakeComponent>(TEXT("ScreenShakeComponent"));

	InputConfig = CreateDefaultSubobject<UZombieInputConfig>(TEXT("InputConfig"));
}

void AZombiePlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	BodySprite->SetSpriteSheet(BodySheet.LoadSynchronous());
	BodySprite->PlayAnimation(TEXT("Idle"));
	WeaponSprite->SetSpriteSheet(WeaponSheet.LoadSynchronous());

	HealthComponent->OnDeath.AddDynamic(this, &AZombiePlayerCharacter::HandleDeath);
	DamageComponent->OnDamageReceived.AddUObject(this, &AZombiePlayerCharacter::HandleDamageReceived);
	InventoryComponent->OnActiveWeaponChanged.AddUObject(this, &AZombiePlayerCharacter::HandleActiveWeaponChanged);
	WeaponComponent->OnWeaponFired.AddUObject(this, &AZombiePlayerCharacter::HandleWeaponFired);
	StatusEffectComponent->OnStatusEffectsChanged.AddUObject(this, &AZombiePlayerCharacter::HandleStatusEffectsChanged);

	HandleActiveWeaponChanged(InventoryComponent->GetActiveWeapon());
	HandleStatsChanged();
}

void AZombiePlayerCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(InputConfig->GameplayContext, 0);
		}
	}
}

void AZombiePlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!Input)
	{
		return;
	}

	Input->BindAction(InputConfig->Move, ETriggerEvent::Triggered, this, &AZombiePlayerCharacter::HandleMove);
	Input->BindAction(InputConfig->AimStick, ETriggerEvent::Triggered, this, &AZombiePlayerCharacter::HandleAimStick);
	Input->BindAction(InputConfig->Sprint, ETriggerEvent::Started, this, &AZombiePlayerCharacter::HandleSprintStarted);
	Input->BindAction(InputConfig->Sprint, ETriggerEvent::Completed, this, &AZombiePlayerCharacter::HandleSprintStopped);
	Input->BindAction(InputConfig->Fire, ETriggerEvent::Started, this, &AZombiePlayerCharacter::HandleFireStarted);
	Input->BindAction(InputConfig->Fire, ETriggerEvent::Completed, this, &AZombiePlayerCharacter::HandleFireStopped);
	Input->BindAction(InputConfig->Reload, ETriggerEvent::Started, this, &AZombiePlayerCharacter::HandleReload);
	Input->BindAction(InputConfig->Interact, ETriggerEvent::Started, this, &AZombiePlayerCharacter::HandleInteract);
	Input->BindAction(InputConfig->WeaponSlot, ETriggerEvent::Started, this, &AZombiePlayerCharacter::HandleWeaponSlot);
	Input->BindAction(InputConfig->CycleWeapon, ETriggerEvent::Started, this, &AZombiePlayerCharacter::HandleCycleWeapon);
	Input->BindAction(InputConfig->Melee, ETriggerEvent::Started, this, &AZombiePlayerCharacter::HandleMelee);
}

void AZombiePlayerCharacter::OnPlayerStateChanged(APlayerState* NewPlayerState, APlayerState* OldPlayerState)
{
	Super::OnPlayerStateChanged(NewPlayerState, OldPlayerState);

	if (IZombieStatSource* OldSource = Cast<IZombieStatSource>(BoundStatSource.Get()))
	{
		OldSource->OnStatsChanged().Remove(StatsChangedHandle);
	}
	BoundStatSource.Reset();

	if (IZombieStatSource* Source = ZombieStats::FindStatSource(this))
	{
		StatsChangedHandle = Source->OnStatsChanged().AddUObject(this, &AZombiePlayerCharacter::HandleStatsChanged);
		BoundStatSource = Cast<UObject>(Source);
	}
	HandleStatsChanged();
}

void AZombiePlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bDead)
	{
		return;
	}

	UpdateMovementSpeed();
	UpdateMouseAim();
	UpdateSpriteAnimation();
}

void AZombiePlayerCharacter::UpdateMovementSpeed()
{
	const float Base = StaminaComponent->IsSprinting() ? SprintSpeed : WalkSpeed;
	GetCharacterMovement()->MaxWalkSpeed = Base * MoveSpeedMultiplier * StatusEffectComponent->GetMoveSpeedMultiplier();
}

void AZombiePlayerCharacter::UpdateSpriteAnimation()
{
	const bool bMoving = GetVelocity().SizeSquared2D() > FMath::Square(WalkAnimationSpeedThreshold);
	BodySprite->PlayAnimation(bMoving ? FName(TEXT("Walk")) : FName(TEXT("Idle")));
}

void AZombiePlayerCharacter::HandleMove(const FInputActionValue& Value)
{
	if (bInputBlocked)
	{
		return;
	}

	// World-space, not actor-relative: aim and movement are decoupled, twin-stick style.
	const FVector2D MoveInput = Value.Get<FVector2D>();
	AddMovementInput(FVector::RightVector, MoveInput.X);
	AddMovementInput(FVector::ForwardVector, MoveInput.Y);
}

void AZombiePlayerCharacter::HandleAimStick(const FInputActionValue& Value)
{
	if (bInputBlocked)
	{
		return;
	}

	// Screen up is world +X and screen right is world +Y for this camera.
	const FVector2D Stick = Value.Get<FVector2D>();
	const FVector Direction(Stick.Y, Stick.X, 0.0f);
	if (!Direction.IsNearlyZero())
	{
		bGamepadAim = true;
		ApplyFacing(Direction);
		AimPoint = GetActorLocation() + Direction.GetSafeNormal() * 450.0f;
	}
}

void AZombiePlayerCharacter::UpdateMouseAim()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC || bInputBlocked)
	{
		return;
	}

	float DeltaX = 0.0f, DeltaY = 0.0f;
	PC->GetInputMouseDelta(DeltaX, DeltaY);
	if (bGamepadAim && FMath::IsNearlyZero(DeltaX) && FMath::IsNearlyZero(DeltaY))
	{
		// Stick aim stays in charge until the mouse is actually moved.
		AimPoint = GetActorLocation() + GetActorForwardVector() * 450.0f;
		return;
	}
	bGamepadAim = false;

	FVector WorldLocation, WorldDirection;
	if (!PC->DeprojectMousePositionToWorld(WorldLocation, WorldDirection) || FMath::IsNearlyZero(WorldDirection.Z))
	{
		return;
	}

	// Resolve the cursor on the plane the sprites and tracers are drawn on, not at capsule-centre
	// height. The camera looks down at an angle, so the two planes put the same screen point ~20
	// units apart on the ground: aiming on the higher one made every tracer land slightly beside
	// the crosshair, by an amount that changed with the aim direction.
	const float AimPlaneZ = GetActorLocation().Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight()
		+ AZombieWeapon::VisualShotHeightAboveFeet;
	const float T = (AimPlaneZ - WorldLocation.Z) / WorldDirection.Z;
	AimPoint = WorldLocation + WorldDirection * T;
	ApplyFacing(AimPoint - GetActorLocation());
}

void AZombiePlayerCharacter::ApplyFacing(const FVector& Direction)
{
	const FVector Flat(Direction.X, Direction.Y, 0.0f);
	if (!Flat.IsNearlyZero())
	{
		SetActorRotation(Flat.Rotation());
	}
}

void AZombiePlayerCharacter::HandleSprintStarted(const FInputActionValue& Value)
{
	const UZombieGameInstance* GameInstance = Cast<UZombieGameInstance>(GetGameInstance());
	const bool bToggle = GameInstance && GameInstance->GetUserSettings().bToggleSprint;
	StaminaComponent->SetSprinting(bToggle ? !StaminaComponent->IsSprinting() : true);
}

void AZombiePlayerCharacter::HandleSprintStopped(const FInputActionValue& Value)
{
	const UZombieGameInstance* GameInstance = Cast<UZombieGameInstance>(GetGameInstance());
	if (!GameInstance || !GameInstance->GetUserSettings().bToggleSprint)
	{
		StaminaComponent->SetSprinting(false);
	}
}

void AZombiePlayerCharacter::HandleFireStarted(const FInputActionValue& Value)
{
	if (!bInputBlocked)
	{
		WeaponComponent->StartFire();
	}
}

void AZombiePlayerCharacter::HandleFireStopped(const FInputActionValue& Value)
{
	WeaponComponent->StopFire();
}

void AZombiePlayerCharacter::HandleReload(const FInputActionValue& Value)
{
	if (!bInputBlocked)
	{
		WeaponComponent->Reload();
	}
}

void AZombiePlayerCharacter::HandleInteract(const FInputActionValue& Value)
{
	if (!bInputBlocked)
	{
		InteractionComponent->TryInteract();
	}
}

void AZombiePlayerCharacter::HandleWeaponSlot(const FInputActionValue& Value)
{
	const int32 SlotNumber = FMath::RoundToInt(Value.Get<float>());
	if (!bInputBlocked && SlotNumber >= 1)
	{
		WeaponComponent->SelectSlot(SlotNumber - 1);
	}
}

void AZombiePlayerCharacter::HandleMelee(const FInputActionValue& Value)
{
	if (!bInputBlocked)
	{
		WeaponComponent->SelectMelee();
	}
}

void AZombiePlayerCharacter::HandleCycleWeapon(const FInputActionValue& Value)
{
	const float Direction = Value.Get<float>();
	if (!bInputBlocked && !FMath::IsNearlyZero(Direction))
	{
		WeaponComponent->CycleWeapon(Direction > 0.0f ? 1 : -1);
	}
}

void AZombiePlayerCharacter::HandleStatsChanged()
{
	MoveSpeedMultiplier = ZombieStats::Resolve(this, ZombieTags::Stat_Move_Speed, 1.0f);
	StaminaComponent->SetDrainMultiplier(ZombieStats::Resolve(this, ZombieTags::Stat_Stamina_DrainRate, 1.0f));

	// Extra Health raises the ceiling and grants the new headroom, rather than leaving it empty.
	const float OldMax = HealthComponent->GetMaxHealth();
	const float NewMax = ZombieStats::Resolve(this, ZombieTags::Stat_Health_Max, BaseMaxHealth);
	if (!FMath::IsNearlyEqual(OldMax, NewMax))
	{
		HealthComponent->SetMaxHealth(NewMax, false);
		if (NewMax > OldMax)
		{
			HealthComponent->Heal(NewMax - OldMax);
		}
	}

	InventoryComponent->RefreshWeaponStats();
}

void AZombiePlayerCharacter::HandleActiveWeaponChanged(AZombieWeapon* NewWeapon)
{
	const UWeaponDataAsset* Definition = NewWeapon ? NewWeapon->GetDefinition() : nullptr;
	WeaponSprite->SetVisibility(Definition != nullptr);
	if (Definition)
	{
		WeaponSprite->ShowFrame(Definition->HeldSpriteFrame);
	}
}

void AZombiePlayerCharacter::HandleWeaponFired(float ShakeStrength)
{
	AddScreenShake(ShakeStrength);
}

void AZombiePlayerCharacter::HandleDamageReceived(float Amount, AActor* Causer, const UDamageType* DamageType)
{
	BodySprite->Flash(FLinearColor(1.0f, 0.15f, 0.1f), 0.12f);
	if (UZombieAudioSubsystem* Audio = UZombieAudioSubsystem::Get(this))
	{
		Audio->PlayNamedSoundAtLocation(TEXT("Player.Hurt"), GetActorLocation());
	}
	AddScreenShake(FMath::Clamp(Amount / 60.0f, 0.1f, 0.6f));

	if (AZombiePlayerState* ZombiePlayerState = GetPlayerState<AZombiePlayerState>())
	{
		ZombiePlayerState->RecordDamageTaken(Amount);
	}
}

void AZombiePlayerCharacter::HandleStatusEffectsChanged()
{
	BodySprite->SetTint(StatusEffectComponent->GetDisplayTint());
}

void AZombiePlayerCharacter::AddScreenShake(float Trauma)
{
	if (IsLocallyControlled())
	{
		ScreenShakeComponent->AddTrauma(Trauma);
	}
}

void AZombiePlayerCharacter::SetGameplayInputBlocked(bool bBlocked)
{
	bInputBlocked = bBlocked;
	WeaponComponent->SetWeaponsBlocked(bBlocked || bDead);
	if (bBlocked)
	{
		StaminaComponent->SetSprinting(false);
	}
}

void AZombiePlayerCharacter::HandleDeath()
{
	if (bDead)
	{
		return;
	}
	bDead = true;

	WeaponComponent->SetWeaponsBlocked(true);
	StatusEffectComponent->ClearAll();
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	BodySprite->PlayAnimation(TEXT("Death"), true);
	if (UZombieAudioSubsystem* Audio = UZombieAudioSubsystem::Get(this))
	{
		Audio->PlayNamedSound2D(TEXT("Player.Death"));
	}
	WeaponSprite->SetVisibility(false);
	AddScreenShake(0.8f);

	OnPlayerDied.Broadcast(this);
}
