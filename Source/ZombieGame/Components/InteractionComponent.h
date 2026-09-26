#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFocusedInteractableChanged, AActor*, NewFocus);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ZOMBIEGAME_API UInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractionComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void TryInteract();

	AActor* GetFocusedInteractable() const { return FocusedInteractable; }

	/** The focused interactable's prompt, or empty when nothing is in reach. */
	FText GetFocusedPrompt() const;

	/** Re-evaluates focus now rather than at the next check - after an interaction changes state. */
	void RefreshFocus();

	UPROPERTY(BlueprintAssignable)
	FOnFocusedInteractableChanged OnFocusedInteractableChanged;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float InteractionRange = 200.0f;

private:
	UPROPERTY()
	TObjectPtr<AActor> FocusedInteractable;

	AActor* FindBestInteractable() const;
};
