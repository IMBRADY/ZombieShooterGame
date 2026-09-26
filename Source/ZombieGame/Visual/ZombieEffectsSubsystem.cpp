#include "ZombieEffectsSubsystem.h"
#include "Engine/World.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Utilities/ActorPoolSubsystem.h"
#include "Visual/PooledSpriteEffect.h"
#include "Visual/SpriteSheetDataAsset.h"

namespace
{
	/** Floor decals sit a hair above the floor, stacked slightly so overlapping splats don't z-fight. */
	constexpr float DecalBaseHeight = 1.5f;
	constexpr float DecalHeightJitter = 0.8f;

	const TCHAR* DefaultTracerSheetPath = TEXT("/Game/Sprites/Effects/SS_Tracer.SS_Tracer");
}

UZombieEffectsSubsystem* UZombieEffectsSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	return World ? World->GetSubsystem<UZombieEffectsSubsystem>() : nullptr;
}

void UZombieEffectsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	TracerSheet = Cast<USpriteSheetDataAsset>(FSoftObjectPath(DefaultTracerSheetPath).TryLoad());
}

APooledSpriteEffect* UZombieEffectsSubsystem::AcquireEffectActor(const FVector& Location, float YawDegrees)
{
	UActorPoolSubsystem* Pool = UActorPoolSubsystem::Get(this);
	if (!Pool)
	{
		return nullptr;
	}

	APooledSpriteEffect* Effect = Pool->Acquire<APooledSpriteEffect>(APooledSpriteEffect::StaticClass(), FTransform(Location));
	if (Effect)
	{
		Effect->SetFacingYaw(YawDegrees);
	}
	return Effect;
}

void UZombieEffectsSubsystem::PlayNiagara(const FZombieEffectSpec& Spec, const FVector& Location, float YawDegrees)
{
	UNiagaraSystem* System = Spec.NiagaraSystem.LoadSynchronous();
	if (!System)
	{
		return;
	}

	// Niagara pools its own components; AutoRelease hands them back when the burst completes.
	UNiagaraComponent* Component = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, System, Location,
		FRotator(0.0f, YawDegrees, 0.0f), FVector(Spec.Scale), true, true, ENCPoolMethod::AutoRelease);

	if (Component && !Spec.NiagaraColorParameter.IsNone())
	{
		Component->SetVariableLinearColor(Spec.NiagaraColorParameter, Spec.Tint);
	}
}

void UZombieEffectsSubsystem::PlayEffect(const FZombieEffectSpec& Spec, const FVector& Location, float YawDegrees)
{
	if (!Spec.IsSet())
	{
		return;
	}

	const float Yaw = Spec.bRandomYaw ? FMath::FRandRange(0.0f, 360.0f) : YawDegrees;
	const FVector EffectLocation = Location + FVector(0.0f, 0.0f, Spec.HeightOffset);

	if (!Spec.NiagaraSystem.IsNull())
	{
		PlayNiagara(Spec, EffectLocation, Yaw);
	}

	if (Spec.Flipbook)
	{
		if (APooledSpriteEffect* Effect = AcquireEffectActor(EffectLocation, Yaw))
		{
			const float Lifetime = Spec.Lifetime > 0.0f ? Spec.Lifetime : Spec.Flipbook->FindAnimation(Spec.Animation).GetDuration();
			Effect->Show(Spec.Flipbook, Spec.Animation, Spec.Scale, Spec.Tint, FMath::Max(Lifetime, 0.05f), 0.0f);
		}
	}
}

void UZombieEffectsSubsystem::SpawnDecal(const FZombieEffectSpec& Spec, const FVector& GroundLocation)
{
	if (!Spec.Flipbook)
	{
		return;
	}

	// Recycle the oldest decal once the budget is reached rather than growing without bound.
	ActiveDecals.RemoveAll([](const TObjectPtr<APooledSpriteEffect>& Decal)
	{
		return !IsValid(Decal) || Decal->IsHidden();
	});

	if (ActiveDecals.Num() >= MaxDecals)
	{
		if (UActorPoolSubsystem* Pool = UActorPoolSubsystem::Get(this))
		{
			Pool->Release(ActiveDecals[0]);
		}
		ActiveDecals.RemoveAt(0);
	}

	const FVector Location = GroundLocation + FVector(0.0f, 0.0f, DecalBaseHeight + FMath::FRandRange(0.0f, DecalHeightJitter) + Spec.HeightOffset);
	if (APooledSpriteEffect* Decal = AcquireEffectActor(Location, FMath::FRandRange(0.0f, 360.0f)))
	{
		const float Lifetime = Spec.Lifetime > 0.0f ? Spec.Lifetime : DecalLifetime;
		Decal->Show(Spec.Flipbook, Spec.Animation, Spec.Scale * FMath::FRandRange(0.8f, 1.2f), Spec.Tint, Lifetime, FMath::Min(3.0f, Lifetime * 0.25f));
		ActiveDecals.Add(Decal);
	}
}

void UZombieEffectsSubsystem::SpawnTracer(const FVector& Start, const FVector& End, const FLinearColor& Color, float Width, float Duration)
{
	if (!TracerSheet)
	{
		return;
	}

	const FVector Delta = End - Start;
	const float Length = Delta.Size2D();
	if (Length < 1.0f)
	{
		return;
	}

	if (APooledSpriteEffect* Tracer = AcquireEffectActor((Start + End) * 0.5f, Delta.Rotation().Yaw))
	{
		Tracer->Show(TracerSheet, TEXT("Play"), 1.0f, Color, Duration, Duration);
		Tracer->SetQuadSize(FVector2D(Length, Width));
	}
}
