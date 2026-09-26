#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SpriteSheetDataAsset.generated.h"

class UTexture2D;

/** A run of frames on a sprite sheet, played by the sprite material with no per-frame CPU work. */
USTRUCT(BlueprintType)
struct FSpriteAnimation
{
	GENERATED_BODY()

	/** Index of the first frame, counting left-to-right then top-to-bottom across the sheet. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite", meta = (ClampMin = "0"))
	int32 StartFrame = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite", meta = (ClampMin = "1"))
	int32 FrameCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite", meta = (ClampMin = "0.1"))
	float FramesPerSecond = 8.0f;

	/** Non-looping animations hold their last frame (deaths, one-shot effects). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite")
	bool bLoop = true;

	float GetDuration() const { return FrameCount / FMath::Max(FramesPerSecond, 0.1f); }
};

/**
 * A pixel-art sprite sheet and the named animations on it.
 *
 * This is the project's flipbook: characters, pickups and effects are flat quads whose material
 * picks a frame from the sheet using the engine clock, so hundreds of animated zombies cost no
 * game-thread time at all (ARCHITECTURE.md 2 and 8). New art is a new texture and a new one of
 * these - no code.
 */
UCLASS(BlueprintType)
class ZOMBIEGAME_API USpriteSheetDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite")
	TObjectPtr<UTexture2D> Texture;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite", meta = (ClampMin = "1"))
	int32 Columns = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite", meta = (ClampMin = "1"))
	int32 Rows = 1;

	/** Edge length of one frame in world units once placed on its quad. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite", meta = (ClampMin = "1.0"))
	float WorldSize = 160.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sprite")
	TMap<FName, FSpriteAnimation> Animations;

	/** The named animation, or a single still of frame 0 if the sheet does not define it. */
	FSpriteAnimation FindAnimation(FName Name) const;
};
