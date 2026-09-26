#include "InteractionComponent.h"
#include "Components/Interactable.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "CollisionShape.h"

UInteractionComponent::UInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	// Proximity only needs re-checking a few times a second, not every frame.
	PrimaryComponentTick.TickInterval = 0.1f;
}

void UInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	RefreshFocus();
}

void UInteractionComponent::RefreshFocus()
{
	AActor* NewFocus = FindBestInteractable();
	if (NewFocus != FocusedInteractable)
	{
		FocusedInteractable = NewFocus;
		OnFocusedInteractableChanged.Broadcast(FocusedInteractable);
	}
}

void UInteractionComponent::TryInteract()
{
	if (FocusedInteractable && FocusedInteractable->Implements<UInteractable>()
		&& IInteractable::Execute_CanInteract(FocusedInteractable, GetOwner()))
	{
		IInteractable::Execute_Interact(FocusedInteractable, GetOwner());
		RefreshFocus();
	}
}

FText UInteractionComponent::GetFocusedPrompt() const
{
	if (FocusedInteractable && FocusedInteractable->Implements<UInteractable>())
	{
		return IInteractable::Execute_GetInteractionPrompt(FocusedInteractable, GetOwner());
	}
	return FText::GetEmpty();
}

AActor* UInteractionComponent::FindBestInteractable() const
{
	const APawn* OwningPawn = Cast<APawn>(GetOwner());
	if (!OwningPawn)
	{
		return nullptr;
	}

	const FVector Origin = OwningPawn->GetActorLocation();

	TArray<FOverlapResult> Overlaps;
	const FCollisionShape Shape = FCollisionShape::MakeSphere(InteractionRange);
	GetWorld()->OverlapMultiByObjectType(Overlaps, Origin, FQuat::Identity, FCollisionObjectQueryParams(ECC_WorldDynamic), Shape);

	// The overlap already proves the interactable's volume is in reach (a door's volume sits in
	// the wall, well away from its actor origin); distance only ranks the candidates.
	AActor* Best = nullptr;
	float BestDistSq = TNumericLimits<float>::Max();

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Candidate = Overlap.GetActor();
		if (!Candidate || !Candidate->Implements<UInteractable>() || !IInteractable::Execute_CanInteract(Candidate, GetOwner()))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(Origin, Candidate->GetActorLocation());
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			Best = Candidate;
		}
	}

	return Best;
}
