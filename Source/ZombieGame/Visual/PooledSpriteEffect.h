#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Utilities/PoolableActor.h"
#include "PooledSpriteEffect.generated.h"

class UPixelSpriteComponent;
class USpriteSheetDataAsset;

/**
 * A pooled, self-releasing flipbook - muzzle flashes, blood bursts, explosions, tracers and floor
 * decals are all one of these with different data. Lives only as long as it is shown, then goes
 * back to UActorPoolSubsystem.
 */
UCLASS(NotPlaceable)
class ZOMBIEGAME_API APooledSpriteEffect : public AActor, public IPoolableActor
{
	GENERATED_BODY()

public:
	APooledSpriteEffect();

	/**
	 * Shows the flipbook for Lifetime seconds (0 = forever, until released explicitly). FadeTime
	 * seconds before the end it starts fading out, so decals dissolve rather than pop.
	 */
	void Show(USpriteSheetDataAsset* Sheet, FName Animation, float Scale, const FLinearColor& Tint, float Lifetime, float FadeTime);

	/** Points the sprite's authored facing (+X in the art) along a world yaw. */
	void SetFacingYaw(float YawDegrees);

	/** Stretches the quad to an exact size - used for tracers, which are long and thin. */
	void SetQuadSize(const FVector2D& Size);

	virtual void OnReleasedToPool() override;

protected:
	UPROPERTY(VisibleAnywhere, Category = "Effect")
	TObjectPtr<UPixelSpriteComponent> Sprite;

private:
	void BeginFade();
	void ReturnToPool();

	FTimerHandle LifetimeTimer;
	FTimerHandle FadeTimer;
	float PendingFadeTime = 0.0f;
};
