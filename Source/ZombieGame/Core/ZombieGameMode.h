#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ZombieGameMode.generated.h"

UCLASS()
class ZOMBIEGAME_API AZombieGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AZombieGameMode();

	void AdvanceToNextSector();

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Progression")
	float DifficultyIncreasePerSector = 0.15f;
};
