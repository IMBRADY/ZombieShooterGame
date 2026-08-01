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
}

void UInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* NewFocus = FindBestInteractable();
	if (NewFocus != FocusedInteractable)
	{
		FocusedInteractable = NewFocus;
		OnFocusedInteractableChanged.Broadcast(FocusedInteractable);
	}
}

void UInteractionComponent::TryInteract()
{
	if (FocusedInteractable && FocusedInteractable->Implements<UInteractable>())
	{
		IInteractable::Execute_Interact(FocusedInteractable, GetOwner());
	}
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

	AActor* Best = nullptr;
	float BestDistSq = FMath::Square(InteractionRange);

	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Candidate = Overlap.GetActor();
		if (!Candidate || !Candidate->Implements<UInteractable>())
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
