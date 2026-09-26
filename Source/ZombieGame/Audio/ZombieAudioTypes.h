#pragma once

#include "CoreMinimal.h"
#include "ZombieAudioTypes.generated.h"

class USoundBase;

/** Which user-facing volume slider a sound answers to. */
UENUM(BlueprintType)
enum class EZombieSoundCategory : uint8
{
	Effects	UMETA(DisplayName = "Sound Effects"),
	Music	UMETA(DisplayName = "Music"),
	UI		UMETA(DisplayName = "Interface")
};

/**
 * One sound event as data: a set of interchangeable variants plus how much to vary them.
 *
 * "Weapon variation" and "zombie variation" from the spec come from here - every playback picks a
 * random variant and detunes it slightly, so fifty identical pistol shots never sound identical.
 * The variants are USoundBase, so a SoundWave, a Sound Cue or a MetaSound all drop in unchanged.
 */
USTRUCT(BlueprintType)
struct FZombieSoundSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	TArray<TSoftObjectPtr<USoundBase>> Variants;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound", meta = (ClampMin = "0.0"))
	float Volume = 1.0f;

	/** Random pitch range around 1.0, e.g. 0.08 plays anywhere from 0.92 to 1.08. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound", meta = (ClampMin = "0.0", ClampMax = "0.5"))
	float PitchVariance = 0.06f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sound")
	EZombieSoundCategory Category = EZombieSoundCategory::Effects;

	bool IsSet() const { return Variants.Num() > 0; }
};
