#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "ZombieMenuGameMode.generated.h"

class UZombieInputConfig;
struct FInputActionValue;

/**
 * The title screen's rules: no pawn, no run - just the menu. Selected for L_MainMenu through
 * GameModeMapPrefixes in DefaultEngine.ini, so the map itself carries no game mode reference.
 */
UCLASS()
class ZOMBIEGAME_API AZombieMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AZombieMenuGameMode();
};

/** Shows the main menu and routes Escape/Back to it. */
UCLASS()
class ZOMBIEGAME_API AZombieMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AZombieMenuPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	UPROPERTY(VisibleAnywhere, Category = "Input")
	TObjectPtr<UZombieInputConfig> InputConfig;

private:
	void HandleBack(const FInputActionValue& Value);
};
