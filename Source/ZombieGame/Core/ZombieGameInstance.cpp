#include "ZombieGameInstance.h"
#include "Core/Save/ZombieSaveGames.h"
#include "Core/Save/ZombieSaveSubsystem.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "OnlineSubsystem.h"
#include "ZombieGame.h"

const TCHAR* UZombieGameInstance::MainMenuMap = TEXT("/Game/Maps/L_MainMenu");
const TCHAR* UZombieGameInstance::SectorMap = TEXT("/Game/Maps/L_TestSector");

void UZombieGameInstance::Init()
{
	Super::Init();

	InitializeOnlinePlatform();

	// The save subsystem has loaded the profile by now; settings apply from the first frame.
	if (!IsDedicatedServerInstance())
	{
		ApplyDisplaySettings(GetUserSettings());
	}

	UE_LOG(LogZombieGame, Log, TEXT("ZombieGameInstance initialized"));
}

const FZombieUserSettings& UZombieGameInstance::GetUserSettings() const
{
	const UZombieSaveSubsystem* Saves = GetSubsystem<UZombieSaveSubsystem>();
	const UZombieMetaSaveGame* Meta = Saves ? Saves->GetMeta() : nullptr;
	return Meta ? Meta->Settings : FallbackSettings;
}

void UZombieGameInstance::SetUserSettings(const FZombieUserSettings& NewSettings, bool bPersist)
{
	UZombieSaveSubsystem* Saves = GetSubsystem<UZombieSaveSubsystem>();
	if (UZombieMetaSaveGame* Meta = Saves ? Saves->GetMeta() : nullptr)
	{
		Meta->Settings = NewSettings;
		if (bPersist)
		{
			Saves->SaveMeta();
		}
	}
	else
	{
		FallbackSettings = NewSettings;
	}

	ApplyDisplaySettings(NewSettings);
	OnUserSettingsChanged.Broadcast();
}

void UZombieGameInstance::ApplyDisplaySettings(const FZombieUserSettings& Settings) const
{
	UGameUserSettings* EngineSettings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
	if (!EngineSettings || GIsEditor)
	{
		// In the editor the window belongs to the editor; only packaged/standalone games apply these.
		return;
	}

	EngineSettings->SetFullscreenMode(Settings.bFullscreen ? EWindowMode::WindowedFullscreen : EWindowMode::Windowed);
	EngineSettings->SetVSyncEnabled(Settings.bVSync);
	EngineSettings->ApplySettings(false);
}

void UZombieGameInstance::InitializeOnlinePlatform() const
{
	// Steam (or any other platform) is enabled purely by config: the game only ever talks to the
	// OnlineSubsystem interfaces, never to a platform SDK directly (ARCHITECTURE.md 16).
	const IOnlineSubsystem* Online = IOnlineSubsystem::Get();
	UE_LOG(LogZombieGame, Log, TEXT("Online platform: %s"), Online ? *Online->GetSubsystemName().ToString() : TEXT("none"));
}

void UZombieGameInstance::ReturnToMainMenu()
{
	UGameplayStatics::OpenLevel(this, FName(MainMenuMap));
}

void UZombieGameInstance::StartRun(bool bContinueSavedRun)
{
	UGameplayStatics::OpenLevel(this, FName(SectorMap), true, bContinueSavedRun ? TEXT("Run=Continue") : TEXT("Run=New"));
}
