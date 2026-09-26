#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ScreenShakeComponent.generated.h"

class UCameraComponent;

/**
 * Trauma-style screen shake for the top-down camera: events add trauma, trauma decays, and the
 * camera is offset by trauma squared - small hits barely register, big explosions really kick.
 * Ticks only while shaking, and respects the player's "screen shake" accessibility setting.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ZOMBIEGAME_API UScreenShakeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UScreenShakeComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Adds 0..1 trauma. */
	void AddTrauma(float Amount);

protected:
	virtual void BeginPlay() override;

	/** Camera offset, in world units, at full trauma. */
	UPROPERTY(EditDefaultsOnly, Category = "Shake")
	float MaxOffset = 38.0f;

	/** Trauma lost per second. */
	UPROPERTY(EditDefaultsOnly, Category = "Shake")
	float DecayPerSecond = 1.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Shake")
	float Frequency = 22.0f;

private:
	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> Camera;

	FVector RestLocation = FVector::ZeroVector;
	float Trauma = 0.0f;
	float NoiseTime = 0.0f;
};
