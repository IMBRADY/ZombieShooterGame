#include "ZombieRunState.h"
#include "Components/HealthComponent.h"
#include "Components/InventoryComponent.h"
#include "Core/ZombiePlayerState.h"
#include "Core/ZombieRunSettings.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Perks/PerkComponent.h"
#include "Weapons/WeaponDataAsset.h"

void FZombieRunState::Capture(const AController& Player, FZombieRunSaveData& OutData)
{
	const APawn* Pawn = Player.GetPawn();
	const AZombiePlayerState* PlayerState = Player.GetPlayerState<AZombiePlayerState>();

	if (const UHealthComponent* Health = Pawn ? Pawn->FindComponentByClass<UHealthComponent>() : nullptr)
	{
		OutData.Health = Health->GetHealth();
		OutData.Armor = Health->GetArmor();
	}

	if (const UInventoryComponent* Inventory = Pawn ? Pawn->FindComponentByClass<UInventoryComponent>() : nullptr)
	{
		OutData.Weapons.Reset();
		Inventory->ExportWeapons(OutData.Weapons);
		OutData.ActiveWeaponIndex = Inventory->GetActiveIndex();
		OutData.WeaponSlots = Inventory->GetSlotCount();
	}

	if (PlayerState)
	{
		OutData.Money = PlayerState->GetMoney();
		OutData.Stats = PlayerState->GetRunStats();
		OutData.Perks.Reset();
		if (const UPerkComponent* Perks = PlayerState->GetPerks())
		{
			Perks->ExportRecords(OutData.Perks);
		}
	}
}

void FZombieRunState::Apply(AController& Player, const FZombieRunSaveData& Data)
{
	APawn* Pawn = Player.GetPawn();
	AZombiePlayerState* PlayerState = Player.GetPlayerState<AZombiePlayerState>();

	// Perks first: they change maximum health and magazine sizes, which the values restored below
	// are then clamped against.
	if (PlayerState)
	{
		if (UPerkComponent* Perks = PlayerState->GetPerks())
		{
			Perks->ImportRecords(Data.Perks);
		}
		PlayerState->SetMoney(Data.Money);
		PlayerState->SetRunStats(Data.Stats);
	}

	if (UInventoryComponent* Inventory = Pawn ? Pawn->FindComponentByClass<UInventoryComponent>() : nullptr)
	{
		Inventory->SetSlotCount(Data.WeaponSlots);
		Inventory->ImportWeapons(Data.Weapons, Data.ActiveWeaponIndex);
	}

	if (UHealthComponent* Health = Pawn ? Pawn->FindComponentByClass<UHealthComponent>() : nullptr)
	{
		Health->RestoreState(Data.Health, Data.Armor);
	}
}

void FZombieRunState::ApplyStartingLoadout(AController& Player, const UZombieRunSettings& Settings, int32 StartingSlots)
{
	FZombieRunSaveData Loadout;
	Loadout.Money = Settings.StartingMoney;
	Loadout.WeaponSlots = StartingSlots;
	Loadout.Health = 100.0f;
	Loadout.Armor = 0.0f;

	for (UWeaponDataAsset* Weapon : Settings.StartingWeapons)
	{
		if (Weapon)
		{
			FWeaponInstanceData& Instance = Loadout.Weapons.AddDefaulted_GetRef();
			Instance.Definition = Weapon;
		}
	}

	Apply(Player, Loadout);
}
