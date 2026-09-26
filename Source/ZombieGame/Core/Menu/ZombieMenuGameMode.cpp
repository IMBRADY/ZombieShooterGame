#include "ZombieMenuGameMode.h"
#include "Audio/ZombieAudioSubsystem.h"
#include "Characters/Player/ZombieInputConfig.h"
#include "Core/ZombieGameInstance.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputActionValue.h"
#include "UI/Widgets/ZombieMainMenuWidget.h"
#include "UI/ZombieUIManager.h"

AZombieMenuGameMode::AZombieMenuGameMode()
{
	DefaultPawnClass = nullptr;
	PlayerControllerClass = AZombieMenuPlayerController::StaticClass();
}

AZombieMenuPlayerController::AZombieMenuPlayerController()
{
	InputConfig = CreateDefaultSubobject<UZombieInputConfig>(TEXT("InputConfig"));
	bShowMouseCursor = true;
}

void AZombieMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(InputConfig->GlobalContext, 10);
	}

	if (UZombieUIManager* UI = UZombieUIManager::Get(this))
	{
		if (const UZombieGameInstance* GameInstance = Cast<UZombieGameInstance>(GetGameInstance()))
		{
			UI->ApplyInterfaceScale(GameInstance->GetUserSettings().InterfaceScale);
		}
		UI->SetPauseMenuAllowed(false);
		UI->PushMenu<UZombieMainMenuWidget>();
	}

	if (UZombieAudioSubsystem* Audio = UZombieAudioSubsystem::Get(this))
	{
		Audio->SetMusicState(EZombieMusicState::Menu);
	}
}

void AZombieMenuPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(InputComponent))
	{
		Input->BindAction(InputConfig->Pause, ETriggerEvent::Started, this, &AZombieMenuPlayerController::HandleBack);
	}
}

void AZombieMenuPlayerController::HandleBack(const FInputActionValue& Value)
{
	if (UZombieUIManager* UI = UZombieUIManager::Get(this))
	{
		UI->HandleBackAction();
	}
}
