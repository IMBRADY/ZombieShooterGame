#include "ZombiePlayerController.h"
#include "Audio/ZombieAudioSubsystem.h"
#include "Characters/Player/ZombieCheatManager.h"
#include "Characters/Player/ZombieInputConfig.h"
#include "Core/Achievements/AchievementDataAsset.h"
#include "Core/Achievements/ZombieAchievementSubsystem.h"
#include "Core/ZombieGameInstance.h"
#include "Core/ZombieRunController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/GameModeBase.h"
#include "InputActionValue.h"
#include "Shop/ShopTransactionComponent.h"
#include "UI/Widgets/ZombieDeathScreenWidget.h"
#include "UI/Widgets/ZombieShopWidget.h"
#include "UI/ZombieUIManager.h"

AZombiePlayerController::AZombiePlayerController()
{
	ShopTransactions = CreateDefaultSubobject<UShopTransactionComponent>(TEXT("ShopTransactions"));
	InputConfig = CreateDefaultSubobject<UZombieInputConfig>(TEXT("InputConfig"));
	CheatClass = UZombieCheatManager::StaticClass();
	bShowMouseCursor = true;
}

void AZombiePlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController())
	{
		return;
	}

#if !UE_BUILD_SHIPPING
	// Development console commands (god mode, the unattended smoke test) in every non-shipping build.
	AddCheats(true);
#endif

	if (UZombieAchievementSubsystem* Achievements = UZombieAchievementSubsystem::Get(this))
	{
		AchievementHandle = Achievements->OnAchievementUnlocked.AddUObject(this, &AZombiePlayerController::HandleAchievementUnlocked);
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(InputConfig->GlobalContext, 10);
	}

	if (UZombieGameInstance* GameInstance = Cast<UZombieGameInstance>(GetGameInstance()))
	{
		SettingsChangedHandle = GameInstance->OnUserSettingsChanged.AddUObject(this, &AZombiePlayerController::ApplyUserSettings);
	}
	ApplyUserSettings();

	if (UZombieUIManager* UI = UZombieUIManager::Get(this))
	{
		UI->ShowHUD();
	}
}

void AZombiePlayerController::HandleAchievementUnlocked(const UAchievementDataAsset* Achievement)
{
	if (!Achievement)
	{
		return;
	}
	ClientNotify(FText::Format(NSLOCTEXT("ZombieRun", "Achievement", "ACHIEVEMENT: {0}"), Achievement->DisplayName), FLinearColor(1.0f, 0.82f, 0.25f));
	if (UZombieAudioSubsystem* Audio = UZombieAudioSubsystem::Get(this))
	{
		Audio->PlayNamedSound2D(TEXT("UI.Achievement"));
	}
}

void AZombiePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UZombieAchievementSubsystem* Achievements = UZombieAchievementSubsystem::Get(this))
	{
		Achievements->OnAchievementUnlocked.Remove(AchievementHandle);
	}
	if (UZombieGameInstance* GameInstance = Cast<UZombieGameInstance>(GetGameInstance()))
	{
		GameInstance->OnUserSettingsChanged.Remove(SettingsChangedHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void AZombiePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(InputComponent))
	{
		Input->BindAction(InputConfig->Pause, ETriggerEvent::Started, this, &AZombiePlayerController::HandlePause);
	}
}

void AZombiePlayerController::HandlePause(const FInputActionValue& Value)
{
	if (UZombieUIManager* UI = UZombieUIManager::Get(this))
	{
		UI->HandleBackAction();
	}
}

void AZombiePlayerController::ApplyUserSettings()
{
	const UZombieGameInstance* GameInstance = Cast<UZombieGameInstance>(GetGameInstance());
	UZombieUIManager* UI = UZombieUIManager::Get(this);
	if (GameInstance && UI)
	{
		UI->ApplyInterfaceScale(GameInstance->GetUserSettings().InterfaceScale);
	}
}

void AZombiePlayerController::ClientNotify_Implementation(const FText& Message, FLinearColor Color)
{
	if (UZombieUIManager* UI = UZombieUIManager::Get(this))
	{
		UI->Notify(Message, Color);
	}
}

void AZombiePlayerController::ClientOpenShop_Implementation()
{
	UZombieUIManager* UI = UZombieUIManager::Get(this);
	if (UI && !UI->FindMenu<UZombieShopWidget>())
	{
		UI->PushMenu<UZombieShopWidget>();
	}
}

void AZombiePlayerController::ClientCloseShop_Implementation()
{
	UZombieUIManager* UI = UZombieUIManager::Get(this);
	if (UZombieShopWidget* Shop = UI ? UI->FindMenu<UZombieShopWidget>() : nullptr)
	{
		UI->PopMenu(Shop);
	}
}

void AZombiePlayerController::ClientShowRunSummary_Implementation(const FZombieRunStats& Stats, int32 SectorReached)
{
	UZombieUIManager* UI = UZombieUIManager::Get(this);
	if (!UI)
	{
		return;
	}

	UI->PopAllMenus();
	UI->SetPauseMenuAllowed(false);
	if (UZombieDeathScreenWidget* DeathScreen = UI->PushMenu<UZombieDeathScreenWidget>())
	{
		DeathScreen->ShowSummary(Stats, SectorReached);
	}
}

void AZombiePlayerController::ServerLeaveIntermission_Implementation()
{
	if (IZombieRunController* Run = Cast<IZombieRunController>(GetWorld()->GetAuthGameMode()))
	{
		Run->NotifyIntermissionLeft(GetPawn());
	}
}
