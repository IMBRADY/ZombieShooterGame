#pragma once

#include "CoreMinimal.h"
#include "Core/Save/ZombieUserSettings.h"
#include "Engine/GameInstance.h"
#include "ZombieGameInstance.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnUserSettingsChanged);

/**
 * Session-lifetime owner of the things that outlive a level: user settings (and applying them),
 * the platform online subsystem, and the global managers, which are Game Instance Subsystems
 * (save, achievements, AI assets) so they need no wiring here.
 */
UCLASS()
class ZOMBIEGAME_API UZombieGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	const FZombieUserSettings& GetUserSettings() const;

	/**
	 * Stores and applies new settings, then notifies listeners (audio, HUD, camera). bPersist
	 * writes them to disk; a settings screen previews live while dragging and persists on close.
	 */
	void SetUserSettings(const FZombieUserSettings& NewSettings, bool bPersist = true);

	FOnUserSettingsChanged OnUserSettingsChanged;

	/** Opens the main menu level. */
	void ReturnToMainMenu();

	/** Starts a run - fresh, or resuming the saved checkpoint. */
	void StartRun(bool bContinueSavedRun);

	static const TCHAR* MainMenuMap;
	static const TCHAR* SectorMap;

private:
	/** Window mode, vsync and frame-rate display - the settings the engine itself owns. */
	void ApplyDisplaySettings(const FZombieUserSettings& Settings) const;

	/** Logs which online platform is active; achievements/cloud/rich presence route through it. */
	void InitializeOnlinePlatform() const;

	FZombieUserSettings FallbackSettings;
};
