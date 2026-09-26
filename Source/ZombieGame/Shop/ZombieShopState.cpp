#include "ZombieShopState.h"
#include "Loot/LootResolver.h"
#include "Net/UnrealNetwork.h"
#include "Perks/PerkDataAsset.h"
#include "Shop/ShopSettings.h"
#include "Weapons/WeaponDataAsset.h"
#include "Weapons/WeaponRaritySettings.h"

AZombieShopState::AZombieShopState()
{
	bReplicates = true;
	bAlwaysRelevant = true;
}

void AZombieShopState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AZombieShopState, Offers);
	DOREPLIFETIME(AZombieShopState, Sector);
}

void AZombieShopState::GenerateStock(int32 InSector, int32 Seed, const TArray<int32>& AlreadyPurchased)
{
	const UShopSettings* Settings = UShopSettings::GetOrLoadDefault();
	const UWeaponRaritySettings* Rarity = UWeaponRaritySettings::GetOrLoadDefault();
	FRandomStream Random(Seed);

	Sector = InSector;
	Offers.Reset();
	PurchasedOfferIds = AlreadyPurchased;

	// Weapons: distinct guns, each with its own rarity roll.
	const int32 WeaponCount = Random.RandRange(Settings->MinWeaponOffers, FMath::Max(Settings->MinWeaponOffers, Settings->MaxWeaponOffers));
	TSet<const UWeaponDataAsset*> ChosenWeapons;
	for (int32 Attempt = 0; Attempt < WeaponCount * 6 && ChosenWeapons.Num() < WeaponCount; ++Attempt)
	{
		UWeaponDataAsset* Weapon = FLootResolver::PickRandomWeapon(Sector, Random, true);
		if (!Weapon || ChosenWeapons.Contains(Weapon))
		{
			continue;
		}
		ChosenWeapons.Add(Weapon);

		FShopOffer& Offer = Offers.AddDefaulted_GetRef();
		Offer.OfferId = Offers.Num() - 1;
		Offer.Type = EShopOfferType::Weapon;
		Offer.Weapon.Definition = Weapon;
		Offer.Weapon.Rarity = Rarity->RollRarity(Sector, Random);
	}

	// Perks: distinct, from the ordinary (non-rare) pool.
	const int32 PerkCount = Random.RandRange(Settings->MinPerkOffers, FMath::Max(Settings->MinPerkOffers, Settings->MaxPerkOffers));
	TSet<const UPerkDataAsset*> ChosenPerks;
	for (int32 Attempt = 0; Attempt < PerkCount * 6 && ChosenPerks.Num() < PerkCount; ++Attempt)
	{
		UPerkDataAsset* Perk = FLootResolver::PickRandomPerk(Random, false);
		if (!Perk || Perk->bRare || ChosenPerks.Contains(Perk))
		{
			continue;
		}
		ChosenPerks.Add(Perk);

		FShopOffer& Offer = Offers.AddDefaulted_GetRef();
		Offer.OfferId = Offers.Num() - 1;
		Offer.Type = EShopOfferType::Perk;
		Offer.Perk = Perk;
	}

	for (FShopOffer& Offer : Offers)
	{
		Offer.bSoldOut = Offer.Type == EShopOfferType::Weapon && PurchasedOfferIds.Contains(Offer.OfferId);
	}

	OnStockChanged.Broadcast();
}

const FShopOffer* AZombieShopState::FindOffer(int32 OfferId) const
{
	return Offers.FindByPredicate([OfferId](const FShopOffer& Offer) { return Offer.OfferId == OfferId; });
}

void AZombieShopState::MarkPurchased(int32 OfferId)
{
	PurchasedOfferIds.AddUnique(OfferId);

	FShopOffer* Offer = Offers.FindByPredicate([OfferId](const FShopOffer& Entry) { return Entry.OfferId == OfferId; });
	if (Offer && Offer->Type == EShopOfferType::Weapon)
	{
		Offer->bSoldOut = true;
	}
	OnStockChanged.Broadcast();
}

void AZombieShopState::OnRep_Offers()
{
	OnStockChanged.Broadcast();
}
