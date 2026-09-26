#include "InventoryComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "Weapons/ZombieWeapon.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInventoryComponent, Weapons);
	DOREPLIFETIME(UInventoryComponent, ActiveIndex);
	DOREPLIFETIME(UInventoryComponent, SlotCount);
}

AZombieWeapon* UInventoryComponent::SpawnWeapon(const FWeaponInstanceData& Instance)
{
	UWorld* World = GetWorld();
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!World || !OwnerPawn || !Instance.IsValid())
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerPawn;
	SpawnParams.Instigator = OwnerPawn;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AZombieWeapon* Weapon = World->SpawnActor<AZombieWeapon>(AZombieWeapon::StaticClass(), OwnerPawn->GetActorTransform(), SpawnParams);
	if (Weapon)
	{
		Weapon->AttachToActor(OwnerPawn, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		Weapon->InitializeFromInstance(Instance);
	}
	return Weapon;
}

AZombieWeapon* UInventoryComponent::AddWeapon(const FWeaponInstanceData& Instance)
{
	if (!HasFreeSlot() || GetOwnerRole() != ROLE_Authority)
	{
		return nullptr;
	}

	AZombieWeapon* Weapon = SpawnWeapon(Instance);
	if (!Weapon)
	{
		return nullptr;
	}

	Weapons.Add(Weapon);
	OnInventoryChanged.Broadcast();

	// The first gun picked up is drawn immediately; later ones wait in their slot.
	if (Weapons.Num() == 1)
	{
		ActiveIndex = 0;
		Weapon->SetEquipped(true);
		OnActiveWeaponChanged.Broadcast(Weapon);
	}
	return Weapon;
}

FWeaponInstanceData UInventoryComponent::ReplaceWeapon(int32 SlotIndex, const FWeaponInstanceData& Instance)
{
	FWeaponInstanceData Previous;
	if (!Weapons.IsValidIndex(SlotIndex) || GetOwnerRole() != ROLE_Authority)
	{
		return Previous;
	}

	AZombieWeapon* NewWeapon = SpawnWeapon(Instance);
	if (!NewWeapon)
	{
		return Previous;
	}

	if (AZombieWeapon* Old = Weapons[SlotIndex])
	{
		Previous = Old->ToInstanceData();
		Old->Destroy();
	}

	Weapons[SlotIndex] = NewWeapon;
	OnInventoryChanged.Broadcast();

	if (SlotIndex == ActiveIndex)
	{
		NewWeapon->SetEquipped(true);
		OnActiveWeaponChanged.Broadcast(NewWeapon);
	}
	return Previous;
}

bool UInventoryComponent::RemoveWeapon(int32 SlotIndex, FWeaponInstanceData& OutRemoved)
{
	if (!Weapons.IsValidIndex(SlotIndex) || Weapons.Num() <= 1 || GetOwnerRole() != ROLE_Authority)
	{
		return false;
	}

	if (AZombieWeapon* Removed = Weapons[SlotIndex])
	{
		OutRemoved = Removed->ToInstanceData();
		Removed->Destroy();
	}
	Weapons.RemoveAt(SlotIndex);

	ActiveIndex = FMath::Clamp(ActiveIndex >= SlotIndex ? ActiveIndex - 1 : ActiveIndex, 0, Weapons.Num() - 1);
	OnInventoryChanged.Broadcast();
	OnActiveWeaponChanged.Broadcast(GetActiveWeapon());
	return true;
}

void UInventoryComponent::EquipSlot(int32 SlotIndex)
{
	if (!Weapons.IsValidIndex(SlotIndex) || SlotIndex == ActiveIndex)
	{
		return;
	}

	if (AZombieWeapon* Current = GetActiveWeapon())
	{
		Current->SetEquipped(false);
	}

	ActiveIndex = SlotIndex;

	if (AZombieWeapon* Next = GetActiveWeapon())
	{
		Next->SetEquipped(true);
	}
	OnActiveWeaponChanged.Broadcast(GetActiveWeapon());
}

void UInventoryComponent::CycleWeapon(int32 Direction)
{
	if (Weapons.Num() <= 1)
	{
		return;
	}
	const int32 Step = Direction >= 0 ? 1 : -1;
	EquipSlot((ActiveIndex + Step + Weapons.Num()) % Weapons.Num());
}

void UInventoryComponent::AddSlot()
{
	SetSlotCount(SlotCount + 1);
}

void UInventoryComponent::SetSlotCount(int32 NewSlotCount)
{
	SlotCount = FMath::Clamp(NewSlotCount, 1, MaxSlotCount);
	OnInventoryChanged.Broadcast();
}

AZombieWeapon* UInventoryComponent::GetWeaponAt(int32 SlotIndex) const
{
	return Weapons.IsValidIndex(SlotIndex) ? Weapons[SlotIndex].Get() : nullptr;
}

void UInventoryComponent::RefreshWeaponStats()
{
	for (AZombieWeapon* Weapon : Weapons)
	{
		if (Weapon)
		{
			Weapon->RefreshStats();
		}
	}
	OnInventoryChanged.Broadcast();
}

void UInventoryComponent::ExportWeapons(TArray<FWeaponInstanceData>& OutWeapons) const
{
	for (const AZombieWeapon* Weapon : Weapons)
	{
		if (Weapon)
		{
			OutWeapons.Add(Weapon->ToInstanceData());
		}
	}
}

void UInventoryComponent::ImportWeapons(const TArray<FWeaponInstanceData>& InWeapons, int32 InActiveIndex)
{
	DestroyAllWeapons();

	for (const FWeaponInstanceData& Instance : InWeapons)
	{
		if (Weapons.Num() >= SlotCount)
		{
			break;
		}
		if (AZombieWeapon* Weapon = SpawnWeapon(Instance))
		{
			Weapons.Add(Weapon);
		}
	}

	ActiveIndex = Weapons.Num() > 0 ? FMath::Clamp(InActiveIndex, 0, Weapons.Num() - 1) : 0;
	if (AZombieWeapon* Active = GetActiveWeapon())
	{
		Active->SetEquipped(true);
	}

	OnInventoryChanged.Broadcast();
	OnActiveWeaponChanged.Broadcast(GetActiveWeapon());
}

void UInventoryComponent::DestroyAllWeapons()
{
	for (AZombieWeapon* Weapon : Weapons)
	{
		if (IsValid(Weapon))
		{
			Weapon->Destroy();
		}
	}
	Weapons.Reset();
	ActiveIndex = 0;
}

void UInventoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		DestroyAllWeapons();
	}
	Super::EndPlay(EndPlayReason);
}

void UInventoryComponent::OnRep_Weapons()
{
	OnInventoryChanged.Broadcast();
}

void UInventoryComponent::OnRep_ActiveIndex()
{
	OnActiveWeaponChanged.Broadcast(GetActiveWeapon());
}
