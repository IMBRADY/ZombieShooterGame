#include "PooledSpriteEffect.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Utilities/ActorPoolSubsystem.h"
#include "Visual/PixelSpriteComponent.h"
#include "Visual/SpriteSheetDataAsset.h"

APooledSpriteEffect::APooledSpriteEffect()
{
	PrimaryActorTick.bCanEverTick = false;

	Sprite = CreateDefaultSubobject<UPixelSpriteComponent>(TEXT("Sprite"));
	SetRootComponent(Sprite);
}

void APooledSpriteEffect::Show(USpriteSheetDataAsset* Sheet, FName Animation, float Scale, const FLinearColor& Tint, float Lifetime, float FadeTime)
{
	UWorld* World = GetWorld();
	if (!World || !Sheet)
	{
		ReturnToPool();
		return;
	}

	Sprite->FadeOut(0.0f);
	Sprite->SetSpriteSheet(Sheet, Scale);
	Sprite->SetTint(Tint);
	Sprite->PlayAnimation(Animation, true);

	FTimerManager& Timers = World->GetTimerManager();
	Timers.ClearTimer(LifetimeTimer);
	Timers.ClearTimer(FadeTimer);

	if (Lifetime <= 0.0f)
	{
		return;
	}

	Timers.SetTimer(LifetimeTimer, this, &APooledSpriteEffect::ReturnToPool, Lifetime, false);

	PendingFadeTime = FMath::Clamp(FadeTime, 0.0f, Lifetime);
	if (PendingFadeTime > 0.0f)
	{
		Timers.SetTimer(FadeTimer, this, &APooledSpriteEffect::BeginFade, FMath::Max(Lifetime - PendingFadeTime, 0.01f), false);
	}
}

void APooledSpriteEffect::SetQuadSize(const FVector2D& Size)
{
	// Plane mesh is 100 units square.
	Sprite->SetWorldScale3D(FVector(FMath::Max(Size.X, 1.0f) / 100.0f, FMath::Max(Size.Y, 1.0f) / 100.0f, 1.0f));
}

void APooledSpriteEffect::SetFacingYaw(float YawDegrees)
{
	Sprite->SetFacingYaw(YawDegrees);
}

void APooledSpriteEffect::BeginFade()
{
	Sprite->FadeOut(PendingFadeTime);
}

void APooledSpriteEffect::ReturnToPool()
{
	if (UActorPoolSubsystem* Pool = UActorPoolSubsystem::Get(this))
	{
		Pool->Release(this);
	}
	else
	{
		Destroy();
	}
}

void APooledSpriteEffect::OnReleasedToPool()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LifetimeTimer);
		World->GetTimerManager().ClearTimer(FadeTimer);
	}
}
