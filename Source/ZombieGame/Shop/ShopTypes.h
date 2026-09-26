#pragma once

#include "CoreMinimal.h"
#include "Weapons/WeaponTypes.h"
#include "ShopTypes.generated.h"

class UPerkDataAsset;

UENUM(BlueprintType)
enum class EShopOfferType : uint8
{
	Weapon,
	Perk
};

/** Stock on the shop shelf - generated fresh each intermission ("1-3 random weapons, 1-3 random perks"). */
USTRUCT(BlueprintType)
struct FShopOffer
{
	GENERATED_BODY()

	/** Stable within one intermission, so a purchase can be referenced over the network and in saves. */
	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	int32 OfferId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	EShopOfferType Type = EShopOfferType::Weapon;

	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	FWeaponInstanceData Weapon;

	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	TObjectPtr<UPerkDataAsset> Perk;

	UPROPERTY(BlueprintReadOnly, Category = "Shop")
	bool bSoldOut = false;
};

/** The shop's fixed services - always available, priced by FShopPricing. */
UENUM(BlueprintType)
enum class EShopService : uint8
{
	Armor,
	MedKit,
	MysteryBox,
	SlotUpgrade,
	AmmoRefill,
	WeaponUpgrade,
	SellWeapon
};
