#pragma once

#include "CoreMinimal.h"
#include "Audio/ZombieAudioTypes.h"
#include "Engine/DataAsset.h"
#include "ZombieAudioSettings.generated.h"

/**
 * The game's soundtrack and shared one-shots, as data. Weapon and zombie sounds live on their own
 * Data Assets; this holds what belongs to no single actor - music layers, ambience, UI and pickup
 * sounds - so the audio mix is retuned in one place without code.
 */
UCLASS(BlueprintType)
class ZOMBIEGAME_API UZombieAudioSettings : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	static const FPrimaryAssetType AssetType;
	static const TCHAR* DefaultAssetPath;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	static const UZombieAudioSettings* GetOrLoadDefault();

	// --- Music. Sector music is two layers crossfaded by combat intensity. ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Music")
	FZombieSoundSpec MenuMusic;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Music")
	FZombieSoundSpec ExplorationLayer;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Music")
	FZombieSoundSpec CombatLayer;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Music")
	FZombieSoundSpec BossMusic;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Music")
	FZombieSoundSpec IntermissionMusic;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Music")
	FZombieSoundSpec GameOverMusic;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ambience")
	FZombieSoundSpec AmbientLoop;

	// --- Combat intensity ---

	/** Zombies within this distance of a player count toward combat intensity. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intensity", meta = (ClampMin = "100.0"))
	float IntensityRadius = 1800.0f;

	/** Nearby zombies needed for the combat layer to reach full volume. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intensity", meta = (ClampMin = "1.0"))
	float ZombiesForFullIntensity = 6.0f;

	/** Intensity change per second - how quickly the music reacts. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intensity", meta = (ClampMin = "0.05"))
	float IntensityRiseRate = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Intensity", meta = (ClampMin = "0.05"))
	float IntensityFallRate = 0.35f;

	// --- Creature voices ---

	/**
	 * Zombie voices (groans, screams, attacks, deaths) play at full volume within this distance of
	 * the player, then fade out.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creatures", meta = (ClampMin = "0.0"))
	float CreatureFullVolumeRadius = 350.0f;

	/**
	 * Beyond this distance a zombie is silent. The fade between the two is quadratic, so a zombie
	 * halfway out is already quiet - the horde across the building is a murmur, not a wall of noise.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creatures", meta = (ClampMin = "100.0"))
	float CreatureAudibleDistance = 2200.0f;

	// --- Shared one-shots, looked up by name (UI.Click, Pickup.Money, Door.Unlock, ...) ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "One-shots")
	TMap<FName, FZombieSoundSpec> NamedSounds;
};
