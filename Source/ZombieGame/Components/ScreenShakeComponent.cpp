#include "ScreenShakeComponent.h"
#include "Camera/CameraComponent.h"
#include "Core/ZombieGameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UScreenShakeComponent::UScreenShakeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UScreenShakeComponent::BeginPlay()
{
	Super::BeginPlay();

	Camera = GetOwner() ? GetOwner()->FindComponentByClass<UCameraComponent>() : nullptr;
	if (Camera)
	{
		RestLocation = Camera->GetRelativeLocation();
	}
}

void UScreenShakeComponent::AddTrauma(float Amount)
{
	const UZombieGameInstance* GameInstance = GetWorld() ? Cast<UZombieGameInstance>(GetWorld()->GetGameInstance()) : nullptr;
	if (!Camera || Amount <= 0.0f || (GameInstance && !GameInstance->GetUserSettings().bScreenShake))
	{
		return;
	}

	Trauma = FMath::Clamp(Trauma + Amount, 0.0f, 1.0f);
	SetComponentTickEnabled(true);
}

void UScreenShakeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	Trauma = FMath::Max(Trauma - DecayPerSecond * DeltaTime, 0.0f);
	NoiseTime += DeltaTime * Frequency;

	const float Strength = Trauma * Trauma * MaxOffset;
	const FVector Offset(
		0.0f,
		FMath::PerlinNoise1D(NoiseTime) * Strength,
		FMath::PerlinNoise1D(NoiseTime + 57.3f) * Strength);

	if (Camera)
	{
		Camera->SetRelativeLocation(RestLocation + Offset);
	}

	if (Trauma <= 0.0f)
	{
		if (Camera)
		{
			Camera->SetRelativeLocation(RestLocation);
		}
		SetComponentTickEnabled(false);
	}
}
