#include "ZombieGameInstance.h"
#include "../ZombieGame.h"

void UZombieGameInstance::Init()
{
	Super::Init();

	UE_LOG(LogZombieGame, Log, TEXT("ZombieGameInstance initialized"));
}
