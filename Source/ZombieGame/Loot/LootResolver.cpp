#include "LootResolver.h"
#include "Components/HealthComponent.h"
#include "Components/InventoryComponent.h"
#include "Core/Save/ZombieSaveGames.h"
#include "Core/Save/ZombieSaveSubsystem.h"
#include "Core/ZombieGameplayTags.h"
#include "Core/ZombiePlayerState.h"
#include "Core/ZombieRunController.h"
#include "Core/ZombieStatSource.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Loot/LootTableDataAsset.h"
#include "Perks/PerkComponent.h"
#include "Perks/PerkDataAsset.h"
#include "Utilities/ZombiePrimaryAssetLoader.h"
#include "Weapons/WeaponDataAsset.h"
#include "Weapons/WeaponRaritySettings.h"
#include "Weapons/ZombieWeapon.h"

#define LOCTEXT_NAMESPACE "ZombieLoot"

namespace
{
	template <typename TAsset, typename TPredicate>
	TAsset* PickWeightedAsset(FPrimaryAssetType Type, FRandomStream& Random, TPredicate&& WeightOf)
	{
		TArray<TAsset*> Assets;
		FZombiePrimaryAssetLoader::LoadAllOfType(Type, Assets);

		// Stable order so seeded rolls reproduce regardless of asset registry enumeration.
		Assets.Sort([](const TAsset& Lhs, const TAsset& Rhs) { return Lhs.GetName() < Rhs.GetName(); });

		float Total = 0.0f;
		for (TAsset* Asset : Assets)
		{
			Total += WeightOf(*Asset);
		}
		if (Total <= 0.0f)
		{
			return nullptr;
		}

		float Roll = Random.FRandRange(0.0f, Total);
		for (TAsset* Asset : Assets)
		{
			Roll -= WeightOf(*Asset);
			if (Roll <= 0.0f && WeightOf(*Asset) > 0.0f)
			{
				return Asset;
			}
		}
		return nullptr;
	}
}

UWeaponDataAsset* FLootResolver::PickRandomWeapon(int32 Sector, FRandomStream& Random, bool bShopOnly)
{
	return PickWeightedAsset<UWeaponDataAsset>(UWeaponDataAsset::AssetType, Random, [Sector, bShopOnly](const UWeaponDataAsset& Weapon)
	{
		const bool bEligible = Weapon.MinSector <= Sector && (bShopOnly ? Weapon.bAvailableInShop : Weapon.bCanDropAsLoot);
		return bEligible ? FMath::Max(Weapon.ShopWeight, 0.05f) : 0.0f;
	});
}

UPerkDataAsset* FLootResolver::PickRandomPerk(FRandomStream& Random, bool bRareOnly)
{
	return PickWeightedAsset<UPerkDataAsset>(UPerkDataAsset::AssetType, Random, [bRareOnly](const UPerkDataAsset& Perk)
	{
		return (!bRareOnly || Perk.bRare) ? FMath::Max(Perk.ShopWeight, 0.05f) : 0.0f;
	});
}

bool FLootResolver::ResolveEntry(const FLootEntry& Entry, int32 Sector, float RewardMultiplier, FRandomStream& Random, FResolvedLoot& OutLoot)
{
	OutLoot = FResolvedLoot();
	OutLoot.Type = Entry.Type;

	const float Scale = Entry.bScaleWithRewards ? FMath::Max(RewardMultiplier, 0.0f) : 1.0f;
	OutLoot.Amount = FMath::RoundToInt(Random.RandRange(Entry.MinAmount, FMath::Max(Entry.MinAmount, Entry.MaxAmount)) * Scale);

	if (Entry.Type == ELootRewardType::Weapon)
	{
		UWeaponDataAsset* Weapon = Entry.Weapon ? Entry.Weapon.Get() : PickRandomWeapon(Sector, Random, false);
		if (!Weapon)
		{
			return false;
		}
		OutLoot.Weapon.Definition = Weapon;
		OutLoot.Weapon.Rarity = UWeaponRaritySettings::GetOrLoadDefault()->RollRarity(Sector, Random, Entry.MinimumRarity);
	}
	else if (Entry.Type == ELootRewardType::Perk)
	{
		OutLoot.Perk = Entry.Perk ? Entry.Perk.Get() : PickRandomPerk(Random, Entry.bRarePerkOnly);
		if (!OutLoot.Perk)
		{
			return false;
		}
	}

	return true;
}

void FLootResolver::Roll(const ULootTableDataAsset& Table, int32 Sector, float RewardMultiplier, FRandomStream& Random, TArray<FResolvedLoot>& OutLoot)
{
	float Total = Table.NothingWeight;
	for (const FLootEntry& Entry : Table.Entries)
	{
		Total += FMath::Max(Entry.Weight, 0.0f);
	}
	if (Total <= 0.0f)
	{
		return;
	}

	for (int32 RollIndex = 0; RollIndex < Table.Rolls; ++RollIndex)
	{
		float Roll = Random.FRandRange(0.0f, Total) - Table.NothingWeight;
		if (Roll <= 0.0f)
		{
			continue;
		}

		for (const FLootEntry& Entry : Table.Entries)
		{
			Roll -= FMath::Max(Entry.Weight, 0.0f);
			if (Roll <= 0.0f)
			{
				FResolvedLoot Loot;
				if (ResolveEntry(Entry, Sector, RewardMultiplier, Random, Loot))
				{
					OutLoot.Add(Loot);
				}
				break;
			}
		}
	}
}

FText FLootResolver::Describe(const FResolvedLoot& Loot)
{
	switch (Loot.Type)
	{
	case ELootRewardType::Money:	return FText::Format(LOCTEXT("Money", "+${0}"), FText::AsNumber(Loot.Amount));
	case ELootRewardType::Health:	return FText::Format(LOCTEXT("Health", "+{0} Health"), FText::AsNumber(Loot.Amount));
	case ELootRewardType::Armor:	return FText::Format(LOCTEXT("Armor", "+{0} Armor"), FText::AsNumber(Loot.Amount));
	case ELootRewardType::Ammo:		return LOCTEXT("Ammo", "Ammo");
	case ELootRewardType::Key:		return LOCTEXT("Key", "Sector Key");
	case ELootRewardType::Perk:
		return Loot.Perk ? FText::Format(LOCTEXT("Perk", "Perk: {0}"), Loot.Perk->DisplayName) : FText::GetEmpty();
	case ELootRewardType::Weapon:
	{
		const UWeaponDataAsset* Weapon = Loot.Weapon.Definition.LoadSynchronous();
		const FText RarityName = UWeaponRaritySettings::GetOrLoadDefault()->GetTier(Loot.Weapon.Rarity).DisplayName;
		return Weapon ? FText::Format(LOCTEXT("Weapon", "{0} ({1})"), Weapon->DisplayName, RarityName) : FText::GetEmpty();
	}
	default:
		return FText::GetEmpty();
	}
}

bool FLootResolver::GrantAmmo(int32 PercentOfReserve, APawn& Recipient)
{
	UInventoryComponent* Inventory = Recipient.FindComponentByClass<UInventoryComponent>();
	if (!Inventory)
	{
		return false;
	}

	int32 Added = 0;
	for (int32 Slot = 0; Slot < Inventory->GetWeaponCount(); ++Slot)
	{
		if (AZombieWeapon* Weapon = Inventory->GetWeaponAt(Slot))
		{
			const int32 Rounds = FMath::CeilToInt(Weapon->GetStats().MaxReserveAmmo * PercentOfReserve / 100.0f);
			Added += Weapon->AddReserveAmmo(FMath::Max(Rounds, 1));
		}
	}
	return Added > 0;
}

bool FLootResolver::GrantWeapon(const FResolvedLoot& Loot, APawn& Recipient, FResolvedLoot* OutDisplaced)
{
	UInventoryComponent* Inventory = Recipient.FindComponentByClass<UInventoryComponent>();
	if (!Inventory || !Loot.Weapon.IsValid())
	{
		return false;
	}

	if (Loot.Weapon.Rarity == EWeaponRarity::Legendary)
	{
		if (UZombieSaveSubsystem* Saves = UZombieSaveSubsystem::Get(&Recipient))
		{
			++Saves->GetMeta()->Lifetime.LegendariesFound;
		}
	}

	if (Inventory->HasFreeSlot())
	{
		return Inventory->AddWeapon(Loot.Weapon) != nullptr;
	}

	// Full: trade the gun in hand for the new one, and hand the old one back to be dropped.
	const FWeaponInstanceData Previous = Inventory->ReplaceWeapon(Inventory->GetActiveIndex(), Loot.Weapon);
	if (OutDisplaced && Previous.IsValid())
	{
		OutDisplaced->Type = ELootRewardType::Weapon;
		OutDisplaced->Weapon = Previous;
	}
	return true;
}

bool FLootResolver::Grant(const FResolvedLoot& Loot, APawn* Recipient, FText& OutDescription, FResolvedLoot* OutDisplaced)
{
	if (!Recipient)
	{
		return false;
	}

	OutDescription = Describe(Loot);
	UHealthComponent* Health = Recipient->FindComponentByClass<UHealthComponent>();
	AZombiePlayerState* PlayerState = Recipient->GetPlayerState<AZombiePlayerState>();

	switch (Loot.Type)
	{
	case ELootRewardType::Money:
	{
		if (!PlayerState)
		{
			return false;
		}
		const int32 Amount = FMath::RoundToInt(ZombieStats::Resolve(Recipient, ZombieTags::Stat_Economy_MoneyMultiplier, static_cast<float>(Loot.Amount)));
		PlayerState->AddMoney(Amount);
		OutDescription = FText::Format(LOCTEXT("MoneyGranted", "+${0}"), FText::AsNumber(Amount));
		return true;
	}
	case ELootRewardType::Health:
		if (!Health || Health->IsFullHealth())
		{
			return false;
		}
		Health->Heal(static_cast<float>(Loot.Amount));
		return true;

	case ELootRewardType::Armor:
		if (!Health || Health->IsFullArmor())
		{
			return false;
		}
		Health->AddArmor(static_cast<float>(Loot.Amount));
		return true;

	case ELootRewardType::Ammo:
		return GrantAmmo(FMath::Max(Loot.Amount, 1), *Recipient);

	case ELootRewardType::Weapon:
		return GrantWeapon(Loot, *Recipient, OutDisplaced);

	case ELootRewardType::Perk:
	{
		UPerkComponent* Perks = PlayerState ? PlayerState->GetPerks() : nullptr;
		return Perks && Perks->AddPerkTier(Loot.Perk);
	}
	case ELootRewardType::Key:
		if (IZombieRunController* Run = Cast<IZombieRunController>(UGameplayStatics::GetGameMode(Recipient)))
		{
			Run->NotifyKeyCollected(Recipient);
			return true;
		}
		return false;

	default:
		return false;
	}
}

#undef LOCTEXT_NAMESPACE
