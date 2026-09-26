#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "Shop/ShopTypes.h"
#include "ZombieShopState.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnShopStockChanged);

/**
 * The Shop Manager's shared state for one intermission: the stock on the shelf and what has sold.
 * Replicated, because in co-op every player browses the same shelf; purchases go through each
 * player's own UShopTransactionComponent, which validates against this.
 */
UCLASS(NotPlaceable)
class ZOMBIEGAME_API AZombieShopState : public AInfo
{
	GENERATED_BODY()

public:
	AZombieShopState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Rolls this intermission's stock. The same seed always produces the same shelf. */
	void GenerateStock(int32 InSector, int32 Seed, const TArray<int32>& AlreadyPurchased);

	const TArray<FShopOffer>& GetOffers() const { return Offers; }
	const FShopOffer* FindOffer(int32 OfferId) const;
	int32 GetSector() const { return Sector; }

	/** Weapons sell out once bought; perks stay on the shelf for their next tier. */
	void MarkPurchased(int32 OfferId);
	const TArray<int32>& GetPurchasedOfferIds() const { return PurchasedOfferIds; }

	FOnShopStockChanged OnStockChanged;

private:
	UFUNCTION()
	void OnRep_Offers();

	UPROPERTY(ReplicatedUsing = OnRep_Offers)
	TArray<FShopOffer> Offers;

	UPROPERTY(Replicated)
	int32 Sector = 1;

	TArray<int32> PurchasedOfferIds;
};
