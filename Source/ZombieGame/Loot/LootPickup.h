#pragma once

#include "CoreMinimal.h"
#include "Components/Interactable.h"
#include "GameFramework/Actor.h"
#include "Loot/LootTypes.h"
#include "Utilities/PoolableActor.h"
#include "LootPickup.generated.h"

class UPixelSpriteComponent;
class UProjectileMovementComponent;
class USphereComponent;
class USpriteSheetDataAsset;

/**
 * Anything lying on the floor waiting to be picked up - money, health, armor, ammo, the sector
 * key, weapons and perks. One pooled actor type driven by the FResolvedLoot it carries.
 *
 * Money and consumables are collected by touch, and money is pulled toward any player inside the
 * pickup radius ("automatically collected within pickup radius"); weapons and perks wait for an
 * explicit Interact so nobody swaps their gun by accident.
 */
UCLASS(NotPlaceable)
class ZOMBIEGAME_API ALootPickup : public AActor, public IInteractable, public IPoolableActor
{
	GENERATED_BODY()

public:
	ALootPickup();

	/** Puts loot on the floor at Location, scattered slightly so drops don't stack exactly. */
	static ALootPickup* SpawnLoot(UWorld* World, const FResolvedLoot& Loot, const FVector& Location, bool bScatter = true);

	void Setup(const FResolvedLoot& InLoot);
	const FResolvedLoot& GetLoot() const { return Loot; }

	/** Sends this pickup flying to a player - the end-of-sector money sweep uses this. */
	void AttractTo(APawn* Target);

	// IInteractable
	virtual void Interact_Implementation(AActor* Interactor) override;
	virtual FText GetInteractionPrompt_Implementation(AActor* Interactor) const override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;

	// IPoolableActor
	virtual void OnAcquiredFromPool() override;
	virtual void OnReleasedToPool() override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Pickup")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, Category = "Pickup")
	TObjectPtr<UPixelSpriteComponent> Sprite;

	UPROPERTY(VisibleAnywhere, Category = "Pickup")
	TObjectPtr<UProjectileMovementComponent> Movement;

	UPROPERTY(EditDefaultsOnly, Category = "Pickup")
	TSoftObjectPtr<USpriteSheetDataAsset> PickupSheet;

	/** Base radius money is pulled in from; the pickup-radius perk adds to it. */
	UPROPERTY(EditDefaultsOnly, Category = "Pickup")
	float BaseMagnetRadius = 260.0f;

private:
	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Grants the loot to Collector; returns to the pool on success. */
	bool TryCollect(APawn* Collector);

	/** Periodic check for players inside the magnet radius (money only). */
	void CheckMagnet();

	FName GetAnimationName() const;
	void ReturnToPool();

	UPROPERTY(Transient)
	FResolvedLoot Loot;

	FTimerHandle MagnetTimer;
	bool bCollected = false;
};
