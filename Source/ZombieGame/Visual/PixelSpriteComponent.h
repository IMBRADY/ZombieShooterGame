#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "PixelSpriteComponent.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class USpriteSheetDataAsset;

/**
 * A flat, top-down pixel-art sprite: a ground-aligned quad whose material plays a sprite sheet
 * animation from the engine clock.
 *
 * Switching animation is one handful of material parameters; nothing ticks while it plays. Hit
 * flashes and fades are likewise material-side (flash is the only thing with a timer, to clear it).
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ZOMBIEGAME_API UPixelSpriteComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	UPixelSpriteComponent();

	void SetSpriteSheet(USpriteSheetDataAsset* InSheet, float ScaleMultiplier = 1.0f);
	USpriteSheetDataAsset* GetSpriteSheet() const { return Sheet; }

	/** Starts the named animation. Re-requesting the playing animation is a no-op unless bRestart. */
	void PlayAnimation(FName AnimationName, bool bRestart = false);
	FName GetCurrentAnimation() const { return CurrentAnimation; }

	/** Holds a single frame of the sheet - held-weapon overlays, icons, static props. */
	void ShowFrame(int32 FrameIndex);
	float GetAnimationDuration(FName AnimationName) const;

	void SetTint(const FLinearColor& Tint);

	/** Briefly washes the sprite toward FlashColor - hit feedback. */
	void Flash(const FLinearColor& FlashColor, float Duration);

	/** Fades the sprite out over Duration seconds from now (0 restores full opacity). */
	void FadeOut(float Duration);

	/** Rotates the sheet's own "facing" (art is drawn facing +X) to a world yaw. */
	void SetFacingYaw(float WorldYawDegrees);

protected:
	virtual void OnRegister() override;

	UPROPERTY(EditDefaultsOnly, Category = "Sprite")
	TSoftObjectPtr<UMaterialInterface> SpriteMaterial;

	/**
	 * Yaw between the art's facing and the quad's local +X. Sprite art is authored facing +X, so
	 * this only compensates for the quad mesh's own UV orientation.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Sprite")
	float ArtYawOffset = 0.0f;

private:
	UMaterialInstanceDynamic* EnsureMaterial();
	void ApplyAnimationParameters(const FName AnimationName);
	void ClearFlash();

	UPROPERTY(Transient)
	TObjectPtr<USpriteSheetDataAsset> Sheet;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SpriteMID;

	FName CurrentAnimation;
	FTimerHandle FlashTimer;
	float TimeOffset = 0.0f;
};
