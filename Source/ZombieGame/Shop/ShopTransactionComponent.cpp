#include "ShopTransactionComponent.h"
#include "Audio/ZombieAudioSubsystem.h"
#include "Components/HealthComponent.h"
#include "Components/InventoryComponent.h"
#include "Core/Save/ZombieSaveGames.h"
#include "Core/Save/ZombieSaveSubsystem.h"
#include "Core/ZombieGameState.h"
#include "Core/ZombiePlayerState.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Loot/LootResolver.h"
#include "Loot/LootTableDataAsset.h"
#include "Perks/PerkComponent.h"
#include "Perks/PerkDataAsset.h"
#include "Shop/ShopPricing.h"
#include "Shop/ShopSettings.h"
#include "Shop/ZombieShopState.h"
#include "Weapons/WeaponDataAsset.h"
#include "Weapons/WeaponRaritySettings.h"
#include "Weapons/WeaponStatsCalculator.h"
#include "Weapons/ZombieWeapon.h"

#define LOCTEXT_NAMESPACE "ZombieShop"

UShopTransactionComponent::UShopTransactionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

APawn* UShopTransactionComponent::GetPawn() const
{
	const AController* Controller = Cast<AController>(GetOwner());
	return Controller ? Controller->GetPawn() : nullptr;
}

AZombiePlayerState* UShopTransactionComponent::GetZombiePlayerState() const
{
	const AController* Controller = Cast<AController>(GetOwner());
	return Controller ? Controller->GetPlayerState<AZombiePlayerState>() : nullptr;
}

AZombieShopState* UShopTransactionComponent::GetShop() const
{
	const AZombieGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AZombieGameState>() : nullptr;
	return GameState ? GameState->GetShop() : nullptr;
}

bool UShopTransactionComponent::IsShopOpen() const
{
	const AZombieGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AZombieGameState>() : nullptr;
	return GameState && GameState->GetSectorPhase() == ESectorPhase::Intermission && GameState->GetShop();
}

void UShopTransactionComponent::BuyOffer(int32 OfferId)
{
	ServerBuyOffer(OfferId);
}

void UShopTransactionComponent::BuyService(EShopService Service, int32 Slot)
{
	ServerBuyService(Service, Slot);
}

void UShopTransactionComponent::ServerBuyOffer_Implementation(int32 OfferId)
{
	FText Message;
	const bool bSuccess = IsShopOpen() && ExecuteOffer(OfferId, Message);
	Report(bSuccess, Message.IsEmpty() ? LOCTEXT("Closed", "The shop is closed.") : Message);
}

void UShopTransactionComponent::ServerBuyService_Implementation(EShopService Service, int32 Slot)
{
	FText Message;
	const bool bSuccess = IsShopOpen() && ExecuteService(Service, Slot, Message);
	Report(bSuccess, Message.IsEmpty() ? LOCTEXT("Closed", "The shop is closed.") : Message);
}

void UShopTransactionComponent::Report(bool bSuccess, const FText& Message)
{
	if (UZombieAudioSubsystem* Audio = UZombieAudioSubsystem::Get(this))
	{
		Audio->PlayNamedSound2D(bSuccess ? TEXT("UI.Buy") : TEXT("UI.Denied"));
	}
	ClientTransactionResult(bSuccess, Message);
}

void UShopTransactionComponent::ClientTransactionResult_Implementation(bool bSuccess, const FText& Message)
{
	OnTransactionResult.Broadcast(bSuccess, Message);
}

bool UShopTransactionComponent::Charge(int32 Price, FText& OutMessage) const
{
	AZombiePlayerState* PlayerState = GetZombiePlayerState();
	if (!PlayerState || !PlayerState->SpendMoney(Price))
	{
		OutMessage = LOCTEXT("TooPoor", "Not enough money.");
		return false;
	}
	return true;
}

bool UShopTransactionComponent::ExecuteOffer(int32 OfferId, FText& OutMessage)
{
	AZombieShopState* Shop = GetShop();
	const FShopOffer* Offer = Shop ? Shop->FindOffer(OfferId) : nullptr;
	APawn* Pawn = GetPawn();
	AZombiePlayerState* PlayerState = GetZombiePlayerState();
	if (!Offer || !Pawn || !PlayerState || Offer->bSoldOut)
	{
		OutMessage = LOCTEXT("SoldOut", "Sold out.");
		return false;
	}

	const UShopSettings* Settings = UShopSettings::GetOrLoadDefault();
	UPerkComponent* Perks = PlayerState->GetPerks();

	if (Offer->Type == EShopOfferType::Perk)
	{
		if (!Offer->Perk || !Perks || Perks->IsMaxed(Offer->Perk))
		{
			OutMessage = LOCTEXT("PerkMaxed", "Already at maximum tier.");
			return false;
		}

		const int32 Price = FShopPricing::GetOfferPrice(*Settings, *Offer, Shop->GetSector(), Perks->GetTier(Offer->Perk));
		if (!Charge(Price, OutMessage))
		{
			return false;
		}
		Perks->AddPerkTier(Offer->Perk);
		Shop->MarkPurchased(OfferId);

		if (UZombieSaveSubsystem* Saves = UZombieSaveSubsystem::Get(this))
		{
			++Saves->GetMeta()->Lifetime.PerksBought;
		}
		OutMessage = FText::Format(LOCTEXT("PerkBought", "{0} is now tier {1}."), Offer->Perk->DisplayName, FText::AsNumber(Perks->GetTier(Offer->Perk)));
		return true;
	}

	const int32 Price = FShopPricing::GetOfferPrice(*Settings, *Offer, Shop->GetSector(), 0);
	if (!Charge(Price, OutMessage))
	{
		return false;
	}

	// A full inventory trades the gun in hand for the new one, crediting its resale value.
	FResolvedLoot Weapon;
	Weapon.Type = ELootRewardType::Weapon;
	Weapon.Weapon = Offer->Weapon;

	UInventoryComponent* Inventory = Pawn->FindComponentByClass<UInventoryComponent>();
	const AZombieWeapon* Traded = (Inventory && !Inventory->HasFreeSlot()) ? Inventory->GetActiveGun() : nullptr;
	const int32 TradeValue = Traded ? FShopPricing::GetSellPrice(*Traded) : 0;

	FText Description;
	FResolvedLoot Displaced;
	if (!FLootResolver::Grant(Weapon, Pawn, Description, &Displaced))
	{
		PlayerState->AddMoney(Price, false);
		OutMessage = LOCTEXT("NoRoom", "Couldn't take the weapon.");
		return false;
	}

	PlayerState->AddMoney(TradeValue, false);
	Shop->MarkPurchased(OfferId);
	OutMessage = TradeValue > 0
		? FText::Format(LOCTEXT("Traded", "Bought {0} (traded in for ${1})."), Description, FText::AsNumber(TradeValue))
		: FText::Format(LOCTEXT("Bought", "Bought {0}."), Description);
	return true;
}

bool UShopTransactionComponent::ExecuteService(EShopService Service, int32 Slot, FText& OutMessage)
{
	const UShopSettings* Settings = UShopSettings::GetOrLoadDefault();
	AZombieShopState* Shop = GetShop();
	APawn* Pawn = GetPawn();
	if (!Shop || !Pawn)
	{
		return false;
	}

	const int32 Sector = Shop->GetSector();
	UHealthComponent* Health = Pawn->FindComponentByClass<UHealthComponent>();
	UInventoryComponent* Inventory = Pawn->FindComponentByClass<UInventoryComponent>();

	switch (Service)
	{
	case EShopService::Armor:
		if (!Health || Health->IsFullArmor())
		{
			OutMessage = LOCTEXT("ArmorFull", "Armor is already full.");
			return false;
		}
		if (!Charge(FShopPricing::GetServicePrice(*Settings, Service, Sector), OutMessage))
		{
			return false;
		}
		Health->AddArmor(static_cast<float>(Settings->ArmorAmount));
		OutMessage = FText::Format(LOCTEXT("ArmorBought", "+{0} armor."), FText::AsNumber(Settings->ArmorAmount));
		return true;

	case EShopService::MedKit:
		if (!Health || Health->IsFullHealth())
		{
			OutMessage = LOCTEXT("HealthFull", "Health is already full.");
			return false;
		}
		if (!Charge(FShopPricing::GetServicePrice(*Settings, Service, Sector), OutMessage))
		{
			return false;
		}
		Health->Heal(static_cast<float>(Settings->MedKitHealAmount));
		OutMessage = FText::Format(LOCTEXT("Healed", "+{0} health."), FText::AsNumber(Settings->MedKitHealAmount));
		return true;

	case EShopService::SlotUpgrade:
		if (!Inventory || !Inventory->CanAddSlot())
		{
			OutMessage = LOCTEXT("SlotsMaxed", "No more slots can be added.");
			return false;
		}
		if (!Charge(FShopPricing::GetSlotUpgradePrice(*Settings, Inventory->GetSlotCount(), Sector), OutMessage))
		{
			return false;
		}
		Inventory->AddSlot();
		OutMessage = FText::Format(LOCTEXT("SlotAdded", "You can now carry {0} guns."), FText::AsNumber(Inventory->GetSlotCount()));
		return true;

	case EShopService::MysteryBox:
		return ExecuteMysteryBox(FShopPricing::GetServicePrice(*Settings, Service, Sector), OutMessage);

	default:
		return ExecuteWeaponService(Service, Slot, Sector, OutMessage);
	}
}

bool UShopTransactionComponent::ExecuteWeaponService(EShopService Service, int32 Slot, int32 Sector, FText& OutMessage)
{
	const UShopSettings* Settings = UShopSettings::GetOrLoadDefault();
	APawn* Pawn = GetPawn();
	UInventoryComponent* Inventory = Pawn ? Pawn->FindComponentByClass<UInventoryComponent>() : nullptr;
	AZombieWeapon* Weapon = Inventory ? Inventory->GetWeaponAt(Slot) : nullptr;
	AZombiePlayerState* PlayerState = GetZombiePlayerState();
	if (!Weapon || !PlayerState)
	{
		OutMessage = LOCTEXT("NoWeapon", "No weapon in that slot.");
		return false;
	}

	if (Service == EShopService::AmmoRefill)
	{
		const int32 Price = FShopPricing::GetAmmoRefillPrice(*Settings, *Weapon, Sector);
		if (Price <= 0)
		{
			OutMessage = LOCTEXT("AmmoFull", "Ammo is already full.");
			return false;
		}
		if (!Charge(Price, OutMessage))
		{
			return false;
		}
		Weapon->RefillAmmo();
		OutMessage = LOCTEXT("Refilled", "Ammo refilled.");
		return true;
	}

	if (Service == EShopService::WeaponUpgrade)
	{
		if (Weapon->IsUpgraded())
		{
			OutMessage = LOCTEXT("AlreadyUpgraded", "Every weapon may be upgraded only once.");
			return false;
		}
		if (!Charge(FShopPricing::GetUpgradePrice(*Settings, *Weapon, Sector), OutMessage))
		{
			return false;
		}
		Weapon->Upgrade();
		OutMessage = LOCTEXT("Upgraded", "Weapon upgraded.");
		return true;
	}

	if (Service == EShopService::SellWeapon)
	{
		const int32 Value = FShopPricing::GetSellPrice(*Weapon);
		FWeaponInstanceData Removed;
		if (!Inventory->RemoveWeapon(Slot, Removed))
		{
			OutMessage = LOCTEXT("LastGun", "You can't sell your last gun.");
			return false;
		}
		PlayerState->AddMoney(Value, false);
		OutMessage = FText::Format(LOCTEXT("Sold", "Sold for ${0}."), FText::AsNumber(Value));
		return true;
	}

	return false;
}

bool UShopTransactionComponent::ExecuteMysteryBox(int32 Price, FText& OutMessage)
{
	const UShopSettings* Settings = UShopSettings::GetOrLoadDefault();
	APawn* Pawn = GetPawn();
	AZombiePlayerState* PlayerState = GetZombiePlayerState();
	AZombieShopState* Shop = GetShop();
	if (!Settings->MysteryBoxTable || !Pawn || !PlayerState || !Shop)
	{
		OutMessage = LOCTEXT("BoxEmpty", "The box is empty.");
		return false;
	}

	if (!Charge(Price, OutMessage))
	{
		return false;
	}

	// Re-roll a few times if the reward would be wasted (full health, maxed perk); a box that
	// still finds nothing usable pays back its own price.
	FRandomStream Random(FMath::Rand());
	for (int32 Attempt = 0; Attempt < 6; ++Attempt)
	{
		TArray<FResolvedLoot> Rolled;
		FLootResolver::Roll(*Settings->MysteryBoxTable, Shop->GetSector(), 1.0f, Random, Rolled);

		for (const FResolvedLoot& Loot : Rolled)
		{
			FText Description;
			FResolvedLoot Displaced;
			if (FLootResolver::Grant(Loot, Pawn, Description, &Displaced))
			{
				if (Displaced.Weapon.IsValid())
				{
					if (const UWeaponDataAsset* Definition = Displaced.Weapon.Definition.LoadSynchronous())
					{
						PlayerState->AddMoney(FWeaponStatsCalculator::GetSellPrice(*Definition, Displaced.Weapon.Rarity,
							Displaced.Weapon.UpgradeLevel, *UWeaponRaritySettings::GetOrLoadDefault()), false);
					}
				}
				OutMessage = FText::Format(LOCTEXT("BoxWon", "Mystery Box: {0}!"), Description);
				return true;
			}
		}
	}

	PlayerState->AddMoney(Price, false);
	OutMessage = LOCTEXT("BoxRefund", "The box was empty - refunded.");
	return true;
}

#undef LOCTEXT_NAMESPACE
