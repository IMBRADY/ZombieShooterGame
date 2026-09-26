#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Visual/ZombieEffectTypes.h"
#include "ZombieEffectsSubsystem.generated.h"

class APooledSpriteEffect;
class USpriteSheetDataAsset;

/**
 * The one place gameplay code asks for something to be *shown*. Callers pass an effect spec from
 * their Data Asset; this decides Niagara vs pooled flipbook, pools the actors, and keeps floor
 * decals (blood, scorch marks) under a fixed budget so a long fight never accumulates unbounded
 * actors.
 *
 * Purely cosmetic by contract: nothing here affects gameplay, so it can be skipped entirely on a
 * dedicated server.
 */
UCLASS()
class ZOMBIEGAME_API UZombieEffectsSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UZombieEffectsSubsystem* Get(const UObject* WorldContext);

	/** A one-shot effect at a location. YawDegrees is ignored when the spec asks for random yaw. */
	void PlayEffect(const FZombieEffectSpec& Spec, const FVector& Location, float YawDegrees = 0.0f);

	/** A lingering floor mark. The oldest decal is recycled once the budget is reached. */
	void SpawnDecal(const FZombieEffectSpec& Spec, const FVector& GroundLocation);

	/** A bullet trail from Start to End that fades out over Duration. */
	void SpawnTracer(const FVector& Start, const FVector& End, const FLinearColor& Color, float Width, float Duration);

	/** Tracer art, set once from the effects settings by whoever owns the level's presentation. */
	void SetTracerSheet(USpriteSheetDataAsset* InTracerSheet) { TracerSheet = InTracerSheet; }

protected:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Budget of simultaneous floor decals. */
	int32 MaxDecals = 160;

	/** How long a decal lingers before dissolving. */
	float DecalLifetime = 45.0f;

private:
	APooledSpriteEffect* AcquireEffectActor(const FVector& Location, float YawDegrees);
	void PlayNiagara(const FZombieEffectSpec& Spec, const FVector& Location, float YawDegrees);

	UPROPERTY(Transient)
	TArray<TObjectPtr<APooledSpriteEffect>> ActiveDecals;

	UPROPERTY(Transient)
	TObjectPtr<USpriteSheetDataAsset> TracerSheet;
};
