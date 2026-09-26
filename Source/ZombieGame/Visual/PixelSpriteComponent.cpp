#include "PixelSpriteComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "Visual/SpriteSheetDataAsset.h"
#include "ZombieGame.h"

namespace PixelSpriteParams
{
	// Parameter names of /Game/Materials/M_PixelSprite (authored by Content/Python/author_content.py).
	static const FName SpriteSheet(TEXT("SpriteSheet"));
	static const FName Columns(TEXT("Columns"));
	static const FName Rows(TEXT("Rows"));
	static const FName StartFrame(TEXT("StartFrame"));
	static const FName FrameCount(TEXT("FrameCount"));
	static const FName FPS(TEXT("FPS"));
	static const FName StartTime(TEXT("StartTime"));
	static const FName Loop(TEXT("Loop"));
	static const FName Tint(TEXT("Tint"));
	static const FName FlashColor(TEXT("FlashColor"));
	static const FName FlashAmount(TEXT("FlashAmount"));
	static const FName FadeStartTime(TEXT("FadeStartTime"));
	static const FName FadeDuration(TEXT("FadeDuration"));
}

namespace
{
	// The engine plane is a 100x100 quad; scale 1.0 covers 100 world units.
	constexpr float PlaneExtent = 100.0f;
}

UPixelSpriteComponent::UPixelSpriteComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneFinder(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneFinder.Succeeded())
	{
		SetStaticMesh(PlaneFinder.Object);
	}

	SpriteMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Materials/M_PixelSprite.M_PixelSprite")));

	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
	SetCanEverAffectNavigation(false);
	CastShadow = false;
	bReceivesDecals = false;
	SetMobility(EComponentMobility::Movable);
}

void UPixelSpriteComponent::OnRegister()
{
	Super::OnRegister();

	// Deliberately unscaled by the owner: a Tank archetype scales its sprite through
	// SetSpriteSheet, not by inheriting whatever scale the capsule happens to have.
	SetUsingAbsoluteScale(true);

	// Attached sprites face wherever their owner faces; the offset only corrects the quad's UVs.
	if (GetAttachParent())
	{
		SetRelativeRotation(FRotator(0.0f, ArtYawOffset, 0.0f));
	}

	// Each instance starts its loops at a different phase, so a horde doesn't shamble in lockstep.
	TimeOffset = FMath::FRandRange(0.0f, 1.0f);
}

UMaterialInstanceDynamic* UPixelSpriteComponent::EnsureMaterial()
{
	if (SpriteMID)
	{
		return SpriteMID;
	}

	UMaterialInterface* BaseMaterial = SpriteMaterial.LoadSynchronous();
	if (!BaseMaterial)
	{
		UE_LOG(LogZombieGame, Error, TEXT("%s: sprite material '%s' is missing."), *GetName(), *SpriteMaterial.ToString());
		return nullptr;
	}

	SpriteMID = UMaterialInstanceDynamic::Create(BaseMaterial, this);
	SetMaterial(0, SpriteMID);
	SpriteMID->SetScalarParameterValue(PixelSpriteParams::FlashAmount, 0.0f);
	SpriteMID->SetScalarParameterValue(PixelSpriteParams::FadeDuration, 0.0f);
	SpriteMID->SetVectorParameterValue(PixelSpriteParams::Tint, FLinearColor::White);
	return SpriteMID;
}

void UPixelSpriteComponent::SetSpriteSheet(USpriteSheetDataAsset* InSheet, float ScaleMultiplier)
{
	Sheet = InSheet;
	UMaterialInstanceDynamic* MID = EnsureMaterial();
	if (!Sheet || !MID)
	{
		SetVisibility(false);
		return;
	}

	SetVisibility(true);
	MID->SetTextureParameterValue(PixelSpriteParams::SpriteSheet, Sheet->Texture);
	MID->SetScalarParameterValue(PixelSpriteParams::Columns, static_cast<float>(Sheet->Columns));
	MID->SetScalarParameterValue(PixelSpriteParams::Rows, static_cast<float>(Sheet->Rows));

	const float QuadScale = (Sheet->WorldSize * FMath::Max(ScaleMultiplier, 0.01f)) / PlaneExtent;
	SetWorldScale3D(FVector(QuadScale, QuadScale, 1.0f));

	const FName Restart = CurrentAnimation.IsNone() ? FName(TEXT("Idle")) : CurrentAnimation;
	CurrentAnimation = NAME_None;
	PlayAnimation(Restart, true);
}

void UPixelSpriteComponent::PlayAnimation(FName AnimationName, bool bRestart)
{
	if (!bRestart && AnimationName == CurrentAnimation)
	{
		return;
	}

	CurrentAnimation = AnimationName;
	ApplyAnimationParameters(AnimationName);
}

void UPixelSpriteComponent::ApplyAnimationParameters(const FName AnimationName)
{
	UMaterialInstanceDynamic* MID = EnsureMaterial();
	const UWorld* World = GetWorld();
	if (!Sheet || !MID || !World)
	{
		return;
	}

	const FSpriteAnimation Animation = Sheet->FindAnimation(AnimationName);

	// Loops get a per-instance phase offset; one-shots must start exactly on their first frame.
	const float Offset = Animation.bLoop ? TimeOffset : 0.0f;

	MID->SetScalarParameterValue(PixelSpriteParams::StartFrame, static_cast<float>(Animation.StartFrame));
	MID->SetScalarParameterValue(PixelSpriteParams::FrameCount, static_cast<float>(Animation.FrameCount));
	MID->SetScalarParameterValue(PixelSpriteParams::FPS, Animation.FramesPerSecond);
	MID->SetScalarParameterValue(PixelSpriteParams::StartTime, World->GetTimeSeconds() - Offset);
	MID->SetScalarParameterValue(PixelSpriteParams::Loop, Animation.bLoop ? 1.0f : 0.0f);
}

void UPixelSpriteComponent::ShowFrame(int32 FrameIndex)
{
	UMaterialInstanceDynamic* MID = EnsureMaterial();
	if (!MID)
	{
		return;
	}

	CurrentAnimation = NAME_None;
	MID->SetScalarParameterValue(PixelSpriteParams::StartFrame, static_cast<float>(FMath::Max(FrameIndex, 0)));
	MID->SetScalarParameterValue(PixelSpriteParams::FrameCount, 1.0f);
	MID->SetScalarParameterValue(PixelSpriteParams::Loop, 0.0f);
}

float UPixelSpriteComponent::GetAnimationDuration(FName AnimationName) const
{
	return Sheet ? Sheet->FindAnimation(AnimationName).GetDuration() : 0.0f;
}

void UPixelSpriteComponent::SetTint(const FLinearColor& Tint)
{
	if (UMaterialInstanceDynamic* MID = EnsureMaterial())
	{
		MID->SetVectorParameterValue(PixelSpriteParams::Tint, Tint);
	}
}

void UPixelSpriteComponent::Flash(const FLinearColor& FlashColor, float Duration)
{
	UMaterialInstanceDynamic* MID = EnsureMaterial();
	UWorld* World = GetWorld();
	if (!MID || !World)
	{
		return;
	}

	MID->SetVectorParameterValue(PixelSpriteParams::FlashColor, FlashColor);
	MID->SetScalarParameterValue(PixelSpriteParams::FlashAmount, 1.0f);
	World->GetTimerManager().SetTimer(FlashTimer, this, &UPixelSpriteComponent::ClearFlash, FMath::Max(Duration, 0.01f), false);
}

void UPixelSpriteComponent::ClearFlash()
{
	if (SpriteMID)
	{
		SpriteMID->SetScalarParameterValue(PixelSpriteParams::FlashAmount, 0.0f);
	}
}

void UPixelSpriteComponent::FadeOut(float Duration)
{
	UMaterialInstanceDynamic* MID = EnsureMaterial();
	const UWorld* World = GetWorld();
	if (!MID || !World)
	{
		return;
	}

	MID->SetScalarParameterValue(PixelSpriteParams::FadeStartTime, World->GetTimeSeconds());
	MID->SetScalarParameterValue(PixelSpriteParams::FadeDuration, FMath::Max(Duration, 0.0f));
}

void UPixelSpriteComponent::SetFacingYaw(float WorldYawDegrees)
{
	SetWorldRotation(FRotator(0.0f, WorldYawDegrees + ArtYawOffset, 0.0f));
}
