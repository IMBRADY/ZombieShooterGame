#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Weapons/WeaponTypes.h"
#include "InventoryComponent.generated.h"

class AZombieWeapon;

DECLARE_MULTICAST_DELEGATE(FOnInventoryChanged);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnActiveWeaponChanged, AZombieWeapon* /*NewWeapon*/);

/**
 * The Inventory System: which guns the player carries, in which slots, and which is drawn.
 *
 * "Player has inventory of 3 guns" - three slots by default, raised by the shop's slot upgrade.
 * Owns the weapon actors' lifetime; knows nothing about firing them (UWeaponComponent does that).
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ZOMBIEGAME_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInventoryComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Puts a weapon in the first free slot and returns it, or null when every slot is full. */
	AZombieWeapon* AddWeapon(const FWeaponInstanceData& Instance);

	/** Swaps the weapon in a slot for a new one ("traded"), returning what was there. */
	FWeaponInstanceData ReplaceWeapon(int32 SlotIndex, const FWeaponInstanceData& Instance);

	/** Removes a weapon (sold). Never removes the last one - the player always has a gun. */
	bool RemoveWeapon(int32 SlotIndex, FWeaponInstanceData& OutRemoved);

	void EquipSlot(int32 SlotIndex);
	void CycleWeapon(int32 Direction);

	bool HasFreeSlot() const { return Weapons.Num() < SlotCount; }
	int32 GetSlotCount() const { return SlotCount; }
	int32 GetMaxSlotCount() const { return MaxSlotCount; }
	bool CanAddSlot() const { return SlotCount < MaxSlotCount; }
	void AddSlot();
	void SetSlotCount(int32 NewSlotCount);

	int32 GetWeaponCount() const { return Weapons.Num(); }
	AZombieWeapon* GetWeaponAt(int32 SlotIndex) const;
	AZombieWeapon* GetActiveWeapon() const { return GetWeaponAt(ActiveIndex); }
	int32 GetActiveIndex() const { return ActiveIndex; }

	/** Re-derives every carried weapon's stats (perks changed). */
	void RefreshWeaponStats();

	void ExportWeapons(TArray<FWeaponInstanceData>& OutWeapons) const;
	void ImportWeapons(const TArray<FWeaponInstanceData>& InWeapons, int32 InActiveIndex);

	FOnInventoryChanged OnInventoryChanged;
	FOnActiveWeaponChanged OnActiveWeaponChanged;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Replicated, Category = "Inventory", meta = (ClampMin = "1"))
	int32 SlotCount = 3;

	/** Ceiling for the shop's "upgrade gun slot". */
	UPROPERTY(EditDefaultsOnly, Category = "Inventory", meta = (ClampMin = "1"))
	int32 MaxSlotCount = 5;

private:
	AZombieWeapon* SpawnWeapon(const FWeaponInstanceData& Instance);
	void DestroyAllWeapons();

	UFUNCTION()
	void OnRep_Weapons();

	UFUNCTION()
	void OnRep_ActiveIndex();

	UPROPERTY(ReplicatedUsing = OnRep_Weapons)
	TArray<TObjectPtr<AZombieWeapon>> Weapons;

	UPROPERTY(ReplicatedUsing = OnRep_ActiveIndex)
	int32 ActiveIndex = 0;
};
