#include "ZombieGameMode.h"
#include "ZombieGameState.h"
#include "ZombiePlayerState.h"
#include "Characters/Player/ZombiePlayerCharacter.h"

AZombieGameMode::AZombieGameMode()
{
	GameStateClass = AZombieGameState::StaticClass();
	PlayerStateClass = AZombiePlayerState::StaticClass();
	DefaultPawnClass = AZombiePlayerCharacter::StaticClass();
}

void AZombieGameMode::AdvanceToNextSector()
{
	AZombieGameState* ZombieGameState = GetGameState<AZombieGameState>();
	if (!ZombieGameState)
	{
		return;
	}

	const int32 NewSector = ZombieGameState->GetCurrentSector() + 1;
	ZombieGameState->AdvanceToSector(NewSector);
	ZombieGameState->SetDifficultyLevel(1.0f + (NewSector - 1) * DifficultyIncreasePerSector);
}
