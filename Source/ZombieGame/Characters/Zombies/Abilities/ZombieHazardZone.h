#pragma once

#include "CoreMinimal.h"
#include "Audio/ZombieAudioTypes.h"
#include "Core/ZombieTeams.h"
#include "Engine/DataAsset.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "ZombieHazardZone.generated.h"

class UPixelSpriteComponent;
class USphereComponent;
class USpriteSheetDataAsset;

/** A lingering ground hazard as data - poison puddles, acid splashes. */
UCLASS(BlueprintType)
class ZOMBIEGAME_API UZombieHazardDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hazard")
	TObjectPtr<USpriteSheetDataAsset> Sprite;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hazard", meta = (ClampMin = "10.0"))
	float Radius = 140.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hazard", meta = (ClampMin = "0.1"))
	float Lifetime = 6.0f;

	/** Status applied to enemies standing in it, re-applied each interval. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hazard", meta = (Categories = "Status"))
	FGameplayTag StatusTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hazard", meta = (ClampMin = "0.0"))
	float Potency = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hazard", meta = (ClampMin = "0.1"))
	float ApplyInterval = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hazard")
	FZombieSoundSpec SpawnSound;
};

/**
 * A puddle on the floor that hurts whoever stands in it (enemies of whoever left it). Checks its
 * occupants on a timer, never ticks, and fades away when its lifetime runs out.
 */
UCLASS(NotPlaceable)
class ZOMBIEGAME_API AZombieHazardZone : public AActor
{
	GENERATED_BODY()

public:
	AZombieHazardZone();

	static AZombieHazardZone* SpawnHazard(UWorld* World, const UZombieHazardDataAsset* Definition, const FVector& GroundLocation,
		AActor* Source);

	void Setup(const UZombieHazardDataAsset* InDefinition, ZombieTeams::ETeam InSourceTeam);

protected:
	UPROPERTY(VisibleAnywhere, Category = "Hazard")
	TObjectPtr<USphereComponent> Area;

	UPROPERTY(VisibleAnywhere, Category = "Hazard")
	TObjectPtr<UPixelSpriteComponent> Sprite;

private:
	void ApplyToOccupants();

	UPROPERTY(Transient)
	TObjectPtr<const UZombieHazardDataAsset> Definition;

	ZombieTeams::ETeam SourceTeam = ZombieTeams::ETeam::None;
	FTimerHandle ApplyTimer;
};
