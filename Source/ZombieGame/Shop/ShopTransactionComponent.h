#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Shop/ShopTypes.h"
#include "ShopTransactionComponent.generated.h"

class AZombieGameState;
class AZombiePlayerState;
class AZombieShopState;
class APawn;

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnShopTransaction, bool /*bSuccess*/, const FText& /*Message*/);

/**
 * The Economy Manager's front door for one player: every purchase, sale and upgrade is requested
 * here, validated and executed on the server against the shared shop stock, then the outcome is
 * reported back to that player's UI. The shop widget never changes gameplay state itself.
 *
 * Lives on the PlayerController so it has a network connection to send RPCs through.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ZOMBIEGAME_API UShopTransactionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UShopTransactionComponent();

	void BuyOffer(int32 OfferId);

	/** Slot is the inventory slot for per-weapon services (refill, upgrade, sell); ignored otherwise. */
	void BuyService(EShopService Service, int32 Slot = INDEX_NONE);

	/** Raised on the owning client with the result of the last request. */
	FOnShopTransaction OnTransactionResult;

private:
	UFUNCTION(Server, Reliable)
	void ServerBuyOffer(int32 OfferId);

	UFUNCTION(Server, Reliable)
	void ServerBuyService(EShopService Service, int32 Slot);

	UFUNCTION(Client, Reliable)
	void ClientTransactionResult(bool bSuccess, const FText& Message);

	bool ExecuteOffer(int32 OfferId, FText& OutMessage);
	bool ExecuteService(EShopService Service, int32 Slot, FText& OutMessage);
	bool ExecuteWeaponService(EShopService Service, int32 Slot, int32 Sector, FText& OutMessage);
	bool ExecuteMysteryBox(int32 Price, FText& OutMessage);

	/** Charges the player, or explains why not. */
	bool Charge(int32 Price, FText& OutMessage) const;

	void Report(bool bSuccess, const FText& Message);

	APawn* GetPawn() const;
	AZombiePlayerState* GetZombiePlayerState() const;
	AZombieShopState* GetShop() const;
	bool IsShopOpen() const;
};
