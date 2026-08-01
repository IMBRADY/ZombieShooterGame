#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UInteractable : public UInterface
{
	GENERATED_BODY()
};

// Implemented by anything InteractionComponent can find and interact with (shop kiosks, doors,
// pickups, ...). None of those exist yet; this is the contract they'll implement.
class ZOMBIEGAME_API IInteractable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	void Interact(AActor* Interactor);
};
