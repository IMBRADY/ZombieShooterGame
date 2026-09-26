#pragma once

#include "CoreMinimal.h"
#include "Core/Save/ZombieRunTypes.h"
#include "GameFramework/PlayerController.h"
#include "ZombiePlayerController.generated.h"

class UShopTransactionComponent;
class UZombieInputConfig;
struct FInputActionValue;

/**
 * The in-run player controller: owns the always-available input (pause), the player's link to
 * the shop, and the client-side entry points the server uses to drive this player's UI (open the
 * shop, show a message, show the run summary). Screens themselves belong to UZombieUIManager.
 */
UCLASS()
class ZOMBIEGAME_API AZombiePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AZombiePlayerController();

	UShopTransactionComponent* GetShopTransactions() const { return ShopTransactions; }

	UFUNCTION(Client, Reliable)
	void ClientNotify(const FText& Message, FLinearColor Color);

	UFUNCTION(Client, Reliable)
	void ClientOpenShop();

	UFUNCTION(Client, Reliable)
	void ClientCloseShop();

	UFUNCTION(Client, Reliable)
	void ClientShowRunSummary(const FZombieRunStats& Stats, int32 SectorReached);

	/** The shop's "Leave Sector" button. */
	UFUNCTION(Server, Reliable)
	void ServerLeaveIntermission();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UShopTransactionComponent> ShopTransactions;

	UPROPERTY(VisibleAnywhere, Category = "Input")
	TObjectPtr<UZombieInputConfig> InputConfig;

private:
	void HandlePause(const FInputActionValue& Value);
	void ApplyUserSettings();
	void HandleAchievementUnlocked(const class UAchievementDataAsset* Achievement);

	FDelegateHandle SettingsChangedHandle;
	FDelegateHandle AchievementHandle;
};
