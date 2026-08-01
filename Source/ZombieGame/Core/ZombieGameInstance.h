#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "ZombieGameInstance.generated.h"

UCLASS()
class ZOMBIEGAME_API UZombieGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
};
