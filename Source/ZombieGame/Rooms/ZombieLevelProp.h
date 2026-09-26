#pragma once

#include "CoreMinimal.h"
#include "Components/Interactable.h"
#include "GameFramework/Actor.h"
#include "ZombieLevelProp.generated.h"

class UBoxComponent;
class UPixelSpriteComponent;
class USpriteSheetDataAsset;

/**
 * Base for the interactive fixtures the sector generator places - exit door, shop terminal,
 * treasure chest. Gives each an interaction volume InteractionComponent can find and a sprite
 * from the shared props sheet; subclasses only decide what Interact means.
 */
UCLASS(Abstract, NotPlaceable)
class ZOMBIEGAME_API AZombieLevelProp : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AZombieLevelProp();

	virtual FText GetInteractionPrompt_Implementation(AActor* Interactor) const override { return FText::GetEmpty(); }
	virtual bool CanInteract_Implementation(AActor* Interactor) const override { return true; }
	virtual void Interact_Implementation(AActor* Interactor) override {}

protected:
	virtual void BeginPlay() override;

	/** Shows a frame/animation from the props sheet. */
	void ShowSprite(FName Animation, float Scale = 1.0f);

	UPROPERTY(VisibleAnywhere, Category = "Prop")
	TObjectPtr<UBoxComponent> InteractionVolume;

	UPROPERTY(VisibleAnywhere, Category = "Prop")
	TObjectPtr<UPixelSpriteComponent> Sprite;

	UPROPERTY(EditDefaultsOnly, Category = "Prop")
	TSoftObjectPtr<USpriteSheetDataAsset> PropSheet;

	/** Height above the prop's origin the sprite is drawn at (door sprites sit on the wall top). */
	UPROPERTY(EditDefaultsOnly, Category = "Prop")
	float SpriteHeight = 8.0f;
};

/**
 * The sector exit. Locked until the sector key is collected, then opens onto the intermission. In
 * the intermission the same door leads to the next sector.
 */
UCLASS()
class ZOMBIEGAME_API AZombieExitDoor : public AZombieLevelProp
{
	GENERATED_BODY()

public:
	AZombieExitDoor();

	void SetUnlocked(bool bInUnlocked);
	bool IsUnlocked() const { return bUnlocked; }

	/** Intermission doors lead onward to the next sector rather than into the shop. */
	void SetLeadsToNextSector(bool bInLeadsToNextSector) { bLeadsToNextSector = bInLeadsToNextSector; }

	virtual FText GetInteractionPrompt_Implementation(AActor* Interactor) const override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual void Interact_Implementation(AActor* Interactor) override;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UPixelSpriteComponent> FloorMarker;

	bool bUnlocked = false;
	bool bLeadsToNextSector = false;
};

/** The intermission's shop counter: opens the market for whoever uses it. */
UCLASS()
class ZOMBIEGAME_API AZombieShopTerminal : public AZombieLevelProp
{
	GENERATED_BODY()

public:
	virtual FText GetInteractionPrompt_Implementation(AActor* Interactor) const override;
	virtual void Interact_Implementation(AActor* Interactor) override;

protected:
	virtual void BeginPlay() override;
};

/** A treasure room's chest: rolls its loot table once and spills the contents on the floor. */
UCLASS()
class ZOMBIEGAME_API AZombieTreasureChest : public AZombieLevelProp
{
	GENERATED_BODY()

public:
	AZombieTreasureChest();

	void SetSector(int32 InSector) { Sector = InSector; }

	virtual FText GetInteractionPrompt_Implementation(AActor* Interactor) const override;
	virtual bool CanInteract_Implementation(AActor* Interactor) const override { return !bOpened; }
	virtual void Interact_Implementation(AActor* Interactor) override;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "Chest")
	TSoftObjectPtr<class ULootTableDataAsset> LootTable;

private:
	int32 Sector = 1;
	bool bOpened = false;
};

/** Covers a boss arena; the first player to step inside wakes the boss. */
UCLASS(NotPlaceable)
class ZOMBIEGAME_API AZombieBossArenaTrigger : public AActor
{
	GENERATED_BODY()

public:
	AZombieBossArenaTrigger();

	void SetArenaBounds(const FBox& Bounds);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Arena")
	TObjectPtr<UBoxComponent> Volume;

private:
	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	bool bTriggered = false;
};
