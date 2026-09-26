#pragma once

#include "CoreMinimal.h"
#include "ZombieEffectTypes.generated.h"

class UNiagaraSystem;
class USpriteSheetDataAsset;

/**
 * One visual effect, described as data: a Niagara system, a pixel-art flipbook, or both.
 *
 * Weapons, zombies and pickups all carry these instead of hardcoded effect classes, so a muzzle
 * flash or a blood spray is swapped by editing a Data Asset. Niagara is used where it adds
 * something a flipbook can't (particle bursts with physics); flipbooks keep the retro pixel look.
 */
USTRUCT(BlueprintType)
struct FZombieEffectSpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	TSoftObjectPtr<UNiagaraSystem> NiagaraSystem;

	/** Niagara user parameter the Tint is written to, when the system exposes one. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	FName NiagaraColorParameter = TEXT("User.Color");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	TObjectPtr<USpriteSheetDataAsset> Flipbook;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	FName Animation = TEXT("Play");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect", meta = (ClampMin = "0.01"))
	float Scale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	FLinearColor Tint = FLinearColor::White;

	/** Seconds the flipbook stays up; 0 means "exactly as long as its animation". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect", meta = (ClampMin = "0.0"))
	float Lifetime = 0.0f;

	/** Height above the given location, so ground effects sit on the floor and bursts at chest height. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	float HeightOffset = 0.0f;

	/** Random yaw each time, so repeated splats and bursts don't look stamped. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	bool bRandomYaw = true;

	bool IsSet() const { return Flipbook != nullptr || !NiagaraSystem.IsNull(); }
};
