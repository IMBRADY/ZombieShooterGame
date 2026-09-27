#pragma once

#include "CoreMinimal.h"
#include "Audio/ZombieAudioTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "ZombieAudioSubsystem.generated.h"

class UAudioComponent;
class UZombieAudioSettings;

UENUM(BlueprintType)
enum class EZombieMusicState : uint8
{
	Silence,
	Menu,
	Sector,
	Boss,
	Intermission,
	GameOver
};

/**
 * The Audio Manager the spec asks for (ARCHITECTURE.md 12).
 *
 * Every sound in the game is played through here, which is what makes the player's volume sliders
 * and variation rules apply uniformly: one-shots pick a random variant and detune it; music runs as
 * layered loops whose mix follows combat intensity ("music intensity increases with combat"),
 * switching wholesale for bosses, the intermission and the game-over screen.
 *
 * World subsystem: it lives and dies with the level, and needs the world to measure how many
 * zombies are closing in on the players.
 */
UCLASS()
class ZOMBIEGAME_API UZombieAudioSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UZombieAudioSubsystem* Get(const UObject* WorldContext);

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	void PlaySoundAtLocation(const FZombieSoundSpec& Sound, const FVector& Location);

	/**
	 * A zombie's voice: like PlaySoundAtLocation, but faded by distance from the nearest player so
	 * nearby zombies are loud and distant ones faint or silent (see UZombieAudioSettings).
	 */
	void PlayCreatureSoundAtLocation(const FZombieSoundSpec& Sound, const FVector& Location);
	void PlaySound2D(const FZombieSoundSpec& Sound);

	/** Plays one of the shared one-shots from the audio settings by name ("UI.Click", ...). */
	void PlayNamedSound2D(FName SoundName);
	void PlayNamedSoundAtLocation(FName SoundName, const FVector& Location);

	void SetMusicState(EZombieMusicState NewState);
	EZombieMusicState GetMusicState() const { return MusicState; }

	/** Current 0..1 combat intensity driving the sector music mix. */
	float GetCombatIntensity() const { return CombatIntensity; }

	/** Re-reads volume sliders; called when the player changes settings. */
	void ApplyVolumeSettings();

private:
	float GetCategoryVolume(EZombieSoundCategory Category) const;

	/** 1 near the players, falling to 0 at CreatureAudibleDistance. */
	float GetCreatureDistanceGain(const FVector& Location) const;
	USoundBase* PickVariant(const FZombieSoundSpec& Sound) const;

	UAudioComponent* StartLoop(const FZombieSoundSpec& Sound);
	void StopAllMusic();
	void UpdateMix();
	float MeasureThreat() const;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> PrimaryMusic;

	/** Only used in the Sector state: the combat layer crossfaded over the exploration layer. */
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> CombatMusic;

	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> Ambience;

	EZombieMusicState MusicState = EZombieMusicState::Silence;
	float CombatIntensity = 0.0f;
	FTimerHandle MixTimer;
	FDelegateHandle SettingsChangedHandle;
};
