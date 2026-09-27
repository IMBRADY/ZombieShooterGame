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
 * Separately from those, a melee slot always holds the run's melee weapon (UZombieRunSettings).
 * It is never counted, sold, traded, saved or dropped as one of the guns.
 *
 * ActiveIndex always names the gun slot last drawn, even while the melee weapon is out - so a
 * shop or loot trade-in always swaps a gun, never the knife.
 *
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

	/** Draws the melee weapon. */
	void EquipMelee();

	/** Steps through the gun slots, then the melee slot, then round again. */
	void CycleWeapon(int32 Direction);

	AZombieWeapon* GetMeleeWeapon() const { return MeleeWeapon; }
	bool IsMeleeActive() const { return bMeleeActive && MeleeWeapon; }

	/** True when there is no gun with a round loaded or spare. */
	bool AreAllGunsEmpty() const;

	bool HasFreeSlot() const { return Weapons.Num() < SlotCount; }
	int32 GetSlotCount() const { return SlotCount; }
	int32 GetMaxSlotCount() const { return MaxSlotCount; }
	bool CanAddSlot() const { return SlotCount < MaxSlotCount; }
	void AddSlot();
	void SetSlotCount(int32 NewSlotCount);

	int32 GetWeaponCount() const { return Weapons.Num(); }
	AZombieWeapon* GetWeaponAt(int32 SlotIndex) const;
	/** Whatever is in the player's hands - a gun, or the melee weapon. */
	AZombieWeapon* GetActiveWeapon() const { return IsMeleeActive() ? MeleeWeapon.Get() : GetWeaponAt(ActiveIndex); }

	/** The gun slot last drawn (still meaningful while the melee weapon is out). */
	int32 GetActiveIndex() const { return ActiveIndex; }
	AZombieWeapon* GetActiveGun() const { return GetWeaponAt(ActiveIndex); }

	/** Re-derives every carried weapon's stats (perks changed). */
	void RefreshWeaponStats();

	void ExportWeapons(TArray<FWeaponInstanceData>& OutWeapons) const;
	void ImportWeapons(const TArray<FWeaponInstanceData>& InWeapons, int32 InActiveIndex);

	FOnInventoryChanged OnInventoryChanged;
	FOnActiveWeaponChanged OnActiveWeaponChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Replicated, Category = "Inventory", meta = (ClampMin = "1"))
	int32 SlotCount = 3;

	/** Ceiling for the shop's "upgrade gun slot". */
	UPROPERTY(EditDefaultsOnly, Category = "Inventory", meta = (ClampMin = "1"))
	int32 MaxSlotCount = 5;

private:
	AZombieWeapon* SpawnWeapon(const FWeaponInstanceData& Instance);
	void DestroyAllWeapons();
	void SpawnMeleeWeapon();

	/** Holsters whatever is drawn, draws the melee weapon or gun slot GunIndex, and tells listeners. */
	void SwitchTo(bool bMelee, int32 GunIndex);

	UFUNCTION()
	void OnRep_Weapons();

	UFUNCTION()
	void OnRep_ActiveIndex();

	UPROPERTY(ReplicatedUsing = OnRep_Weapons)
	TArray<TObjectPtr<AZombieWeapon>> Weapons;

	UPROPERTY(ReplicatedUsing = OnRep_ActiveIndex)
	int32 ActiveIndex = 0;

	UPROPERTY(Replicated)
	TObjectPtr<AZombieWeapon> MeleeWeapon;

	UPROPERTY(ReplicatedUsing = OnRep_ActiveIndex)
	bool bMeleeActive = false;
};
