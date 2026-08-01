#include "StaminaComponent.h"
#include "Net/UnrealNetwork.h"

UStaminaComponent::UStaminaComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UStaminaComponent::BeginPlay()
{
	Super::BeginPlay();

	Stamina = MaxStamina;
}

void UStaminaComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UStaminaComponent, Stamina);
}

void UStaminaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	const float OldStamina = Stamina;

	if (bIsSprinting)
	{
		Stamina = FMath::Max(0.0f, Stamina - DrainPerSecond * DeltaTime);
		if (Stamina <= 0.0f)
		{
			bIsSprinting = false;
			TimeSinceSprintStopped = 0.0f;
		}
	}
	else
	{
		TimeSinceSprintStopped += DeltaTime;
		if (TimeSinceSprintStopped >= RechargeDelay && Stamina < MaxStamina)
		{
			Stamina = FMath::Min(MaxStamina, Stamina + RechargePerSecond * DeltaTime);
		}
	}

	if (!FMath::IsNearlyEqual(Stamina, OldStamina))
	{
		OnStaminaChanged.Broadcast(Stamina, MaxStamina);
	}
}

void UStaminaComponent::SetSprinting(bool bNewSprinting)
{
	const bool bWantsToSprint = bNewSprinting && CanSprint();

	if (bWantsToSprint == bIsSprinting)
	{
		return;
	}

	bIsSprinting = bWantsToSprint;

	if (!bIsSprinting)
	{
		TimeSinceSprintStopped = 0.0f;
	}
}

void UStaminaComponent::OnRep_Stamina()
{
	OnStaminaChanged.Broadcast(Stamina, MaxStamina);
}
