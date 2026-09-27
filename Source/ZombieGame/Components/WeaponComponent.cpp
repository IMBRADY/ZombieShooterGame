#include "WeaponComponent.h"
#include "Components/InventoryComponent.h"
#include "Core/ZombieGameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "Weapons/ZombieWeapon.h"

namespace
{
	/** Retry spacing while waiting on a reload or an input-buffered semi-auto shot. */
	constexpr float WaitRetryInterval = 0.03f;

	/** Dry-fire clicks are rate limited so a held trigger doesn't machine-gun the empty sound. */
	constexpr float EmptyRetryInterval = 0.3f;
}

UWeaponComponent::UWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UWeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	Inventory = GetOwner() ? GetOwner()->FindComponentByClass<UInventoryComponent>() : nullptr;
	if (Inventory)
	{
		Inventory->OnActiveWeaponChanged.AddUObject(this, &UWeaponComponent::HandleActiveWeaponChanged);
		HandleActiveWeaponChanged(Inventory->GetActiveWeapon());
	}
}

AZombieWeapon* UWeaponComponent::GetActiveWeapon() const
{
	return Inventory ? Inventory->GetActiveWeapon() : nullptr;
}

void UWeaponComponent::HandleActiveWeaponChanged(AZombieWeapon* NewWeapon)
{
	if (AZombieWeapon* Previous = BoundWeapon.Get())
	{
		Previous->OnFired.Remove(FiredHandle);
	}

	BoundWeapon = NewWeapon;
	if (NewWeapon)
	{
		FiredHandle = NewWeapon->OnFired.AddUObject(this, &UWeaponComponent::HandleWeaponFired);
	}
}

void UWeaponComponent::HandleWeaponFired(float ShakeStrength)
{
	OnWeaponFired.Broadcast(ShakeStrength);
}

void UWeaponComponent::StartFire()
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		SetTriggerHeld(true);
	}
	else
	{
		ServerSetTriggerHeld(true);
	}
}

void UWeaponComponent::StopFire()
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		SetTriggerHeld(false);
	}
	else
	{
		ServerSetTriggerHeld(false);
	}
}

void UWeaponComponent::ServerSetTriggerHeld_Implementation(bool bHeld)
{
	SetTriggerHeld(bHeld);
}

void UWeaponComponent::SetTriggerHeld(bool bHeld)
{
	bTriggerHeld = bHeld;
	if (!bHeld)
	{
		bShotFiredThisPress = false;
		return;
	}

	UWorld* World = GetWorld();
	if (World && !World->GetTimerManager().IsTimerActive(RefireTimer))
	{
		FireTick();
	}
}

void UWeaponComponent::FireTick()
{
	UWorld* World = GetWorld();
	AZombieWeapon* Weapon = GetActiveWeapon();
	if (!World || !Weapon || !bTriggerHeld || bBlocked)
	{
		return;
	}

	const UZombieGameInstance* GameInstance = Cast<UZombieGameInstance>(World->GetGameInstance());
	const bool bHoldToRepeat = Weapon->IsAutomatic()
		|| bAutoFireSemiAutomatic
		|| (GameInstance && GameInstance->GetUserSettings().bAutoFireSemiAutomatic);

	if (!bHoldToRepeat && bShotFiredThisPress)
	{
		return;
	}

	float NextAttempt = WaitRetryInterval;
	switch (Weapon->TryFire(GetOwner()->GetActorForwardVector()))
	{
	case EWeaponFireResult::Fired:
		bShotFiredThisPress = true;
		if (!bHoldToRepeat)
		{
			return;
		}
		NextAttempt = 1.0f / FMath::Max(Weapon->GetStats().ShotsPerSecond, 0.1f);
		break;

	case EWeaponFireResult::EmptyMagazine:
		// Every gun is dry: pull the knife instead of clicking, and let this press use it.
		if (Inventory && Inventory->GetMeleeWeapon() && Inventory->AreAllGunsEmpty())
		{
			Inventory->EquipMelee();
			NextAttempt = WaitRetryInterval;
			break;
		}
		bShotFiredThisPress = true;
		NextAttempt = EmptyRetryInterval;
		break;

	case EWeaponFireResult::NoDefinition:
		return;

	default:
		// Cooldown or reloading: keep the press buffered and try again shortly.
		break;
	}

	World->GetTimerManager().SetTimer(RefireTimer, this, &UWeaponComponent::FireTick, NextAttempt, false);
}

void UWeaponComponent::Reload()
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		ServerReload();
		return;
	}

	if (AZombieWeapon* Weapon = GetActiveWeapon())
	{
		Weapon->StartReload();
	}
}

void UWeaponComponent::ServerReload_Implementation()
{
	Reload();
}

void UWeaponComponent::SelectSlot(int32 SlotIndex)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		ServerSelectSlot(SlotIndex);
		return;
	}

	if (Inventory)
	{
		Inventory->EquipSlot(SlotIndex);
	}
}

void UWeaponComponent::ServerSelectSlot_Implementation(int32 SlotIndex)
{
	SelectSlot(SlotIndex);
}

void UWeaponComponent::SelectMelee()
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		ServerSelectMelee();
		return;
	}

	if (Inventory)
	{
		Inventory->EquipMelee();
	}
}

void UWeaponComponent::ServerSelectMelee_Implementation()
{
	SelectMelee();
}

void UWeaponComponent::CycleWeapon(int32 Direction)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		ServerCycleWeapon(Direction);
		return;
	}

	if (Inventory)
	{
		Inventory->CycleWeapon(Direction);
	}
}

void UWeaponComponent::ServerCycleWeapon_Implementation(int32 Direction)
{
	CycleWeapon(Direction);
}

void UWeaponComponent::SetWeaponsBlocked(bool bInBlocked)
{
	bBlocked = bInBlocked;
	if (bBlocked)
	{
		bTriggerHeld = false;
		bShotFiredThisPress = false;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(RefireTimer);
		}
	}
}
