#pragma once

#include "CoreMinimal.h"
#include "ZombieUserSettings.generated.h"

/**
 * Player-facing options. Persisted in the meta save (survives death and new runs) and applied by
 * the systems they concern - audio volumes by the audio subsystem, window settings by the game
 * instance, accessibility options by the camera and HUD.
 */
USTRUCT(BlueprintType)
struct FZombieUserSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MasterVolume = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MusicVolume = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float EffectsVolume = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float InterfaceVolume = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
	bool bFullscreen = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
	bool bVSync = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
	bool bShowFrameRate = false;

	// --- Accessibility ---

	/** Camera shake on explosions, heavy weapons and taking hits. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accessibility")
	bool bScreenShake = true;

	/** Red vignette pulse when hurt. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accessibility")
	bool bDamageFlash = true;

	/** Sprint stays on after one press of the sprint key, instead of needing it held. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accessibility")
	bool bToggleSprint = false;

	/** Multiplier on every HUD and menu element. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accessibility", meta = (ClampMin = "0.75", ClampMax = "1.5"))
	float InterfaceScale = 1.0f;

	/** Holding fire on a semi-automatic weapon keeps firing at its fire rate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Accessibility")
	bool bAutoFireSemiAutomatic = false;
};
