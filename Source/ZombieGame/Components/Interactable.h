#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Implemented by anything the player can press Interact on: the exit door, the shop terminal,
 * weapon and perk pickups, treasure chests. InteractionComponent finds the nearest one; the HUD
 * shows its prompt.
 */
class ZOMBIEGAME_API IInteractable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	void Interact(AActor* Interactor);

	/** Short verb phrase for the HUD prompt, e.g. "Open shop" or "Take Shotgun (Rare)". */
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	FText GetInteractionPrompt(AActor* Interactor) const;

	/** False hides the prompt and ignores the key - e.g. the exit door while still locked. */
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	bool CanInteract(AActor* Interactor) const;
};
