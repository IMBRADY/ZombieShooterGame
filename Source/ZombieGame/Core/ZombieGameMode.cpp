#include "ZombieGameMode.h"
#include "Characters/Player/ZombiePlayerCharacter.h"
#include "Characters/Zombies/ZombieCharacter.h"
#include "Core/SpawnDirectorComponent.h"
#include "Core/ZombieGameState.h"
#include "Core/ZombiePlayerState.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Rooms/Procedural/SectorGeneratorComponent.h"
#include "ZombieGame.h"

AZombieGameMode::AZombieGameMode()
{
	GameStateClass = AZombieGameState::StaticClass();
	PlayerStateClass = AZombiePlayerState::StaticClass();
	DefaultPawnClass = AZombiePlayerCharacter::StaticClass();

	SectorGenerator = CreateDefaultSubobject<USectorGeneratorComponent>(TEXT("SectorGenerator"));
	SpawnDirector = CreateDefaultSubobject<USpawnDirectorComponent>(TEXT("SpawnDirector"));
}

void AZombieGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// InitGame runs before any player logs in, which is exactly when the level has to exist: the
	// generated start room is what ChoosePlayerStart hands back for the very first spawn.
	RunSeed = FMath::Rand();
	CurrentSector = 1;

	BuildSector(CurrentSector);
}

void AZombieGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (SpawnDirector)
	{
		SpawnDirector->OnSectorCleared.AddUObject(this, &AZombieGameMode::HandleSectorCleared);
		SpawnDirector->OnZombieKilled.AddUObject(this, &AZombieGameMode::HandleZombieKilled);
	}

	StartEncounter(CurrentSector);
}

int32 AZombieGameMode::GetSeedForSector(int32 Sector) const
{
	return static_cast<int32>(HashCombine(GetTypeHash(RunSeed), GetTypeHash(Sector)));
}

bool AZombieGameMode::BuildSector(int32 Sector)
{
	if (!SectorGenerator)
	{
		return false;
	}

	if (!SectorGenerator->GenerateSector(Sector, GetSeedForSector(Sector)))
	{
		UE_LOG(LogZombieGame, Error, TEXT("Failed to build sector %d; the run cannot continue."), Sector);
		return false;
	}

	return true;
}

void AZombieGameMode::StartEncounter(int32 Sector)
{
	if (AZombieGameState* ZombieGameState = GetGameState<AZombieGameState>())
	{
		ZombieGameState->AdvanceToSector(Sector);
		ZombieGameState->SetDifficultyLevel(1.0f + (Sector - 1) * DifficultyIncreasePerSector);
	}

	if (SpawnDirector && SectorGenerator)
	{
		SpawnDirector->BeginSector(Sector, SectorGenerator->GetZombieSpawnLocations());
	}
}

AActor* AZombieGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	if (SectorGenerator)
	{
		if (APlayerStart* GeneratedStart = SectorGenerator->GetGeneratedPlayerStart())
		{
			return GeneratedStart;
		}
	}

	// Falls back to whatever PlayerStart the map itself provides - useful for hand-built test maps.
	return Super::ChoosePlayerStart_Implementation(Player);
}

void AZombieGameMode::AdvanceToNextSector()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SectorTransitionTimer);
	}

	if (SpawnDirector)
	{
		SpawnDirector->StopSector();
	}

	const int32 NextSector = CurrentSector + 1;
	if (!BuildSector(NextSector))
	{
		return;
	}

	CurrentSector = NextSector;
	MovePlayersToSectorStart();
	StartEncounter(CurrentSector);
}

void AZombieGameMode::MovePlayersToSectorStart()
{
	UWorld* World = GetWorld();
	APlayerStart* SectorStart = SectorGenerator ? SectorGenerator->GetGeneratedPlayerStart() : nullptr;
	if (!World || !SectorStart)
	{
		return;
	}

	// Existing pawns are relocated rather than respawned: health, money and (later) inventory all
	// persist across sectors, so the run state has to survive the transition intact.
	for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PlayerController = Iterator->Get();
		if (!PlayerController)
		{
			continue;
		}

		if (APawn* Pawn = PlayerController->GetPawn())
		{
			Pawn->TeleportTo(SectorStart->GetActorLocation(), Pawn->GetActorRotation());
		}
		else
		{
			RestartPlayer(PlayerController);
		}
	}
}

void AZombieGameMode::HandleZombieKilled(AZombieCharacter* Zombie, AController* Killer)
{
	if (!Zombie)
	{
		return;
	}

	// Rewards land on per-player state even in a single-player build, so a second player later is
	// additive rather than a refactor.
	if (AZombiePlayerState* KillerState = Killer ? Killer->GetPlayerState<AZombiePlayerState>() : nullptr)
	{
		KillerState->AddKill();
		KillerState->AddMoney(Zombie->GetMoneyReward());
	}
}

void AZombieGameMode::HandleSectorCleared()
{
	UE_LOG(LogZombieGame, Log, TEXT("Sector %d cleared."), CurrentSector);

	if (!bAutoAdvanceOnSectorCleared)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(SectorTransitionTimer, this, &AZombieGameMode::AdvanceToNextSector,
			FMath::Max(SectorTransitionDelay, 0.1f), false);
	}
}
