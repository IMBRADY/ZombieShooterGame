#include "ZombieGameMode.h"
#include "Audio/ZombieAudioSubsystem.h"
#include "Characters/Player/ZombiePlayerCharacter.h"
#include "Characters/Player/ZombiePlayerController.h"
#include "Characters/Zombies/ZombieArchetypeDataAsset.h"
#include "Characters/Zombies/ZombieCharacter.h"
#include "Characters/Zombies/ZombieEnemyManager.h"
#include "Core/Achievements/ZombieAchievementSubsystem.h"
#include "Core/RunRewardsComponent.h"
#include "Core/Save/ZombieRunState.h"
#include "Core/Save/ZombieSaveGames.h"
#include "Core/Save/ZombieSaveSubsystem.h"
#include "Core/SpawnDirectorComponent.h"
#include "Core/ZombieGameState.h"
#include "Core/ZombiePlayerState.h"
#include "Core/ZombieRunSettings.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Rooms/Procedural/SectorGeneratorComponent.h"
#include "Rooms/ZombieLevelProp.h"
#include "Shop/ShopSettings.h"
#include "Shop/ZombieShopState.h"
#include "TimerManager.h"
#include "UI/ZombieNotifications.h"
#include "Utilities/ZombiePrimaryAssetLoader.h"
#include "ZombieGame.h"

#define LOCTEXT_NAMESPACE "ZombieRun"

AZombieGameMode::AZombieGameMode()
{
	GameStateClass = AZombieGameState::StaticClass();
	PlayerStateClass = AZombiePlayerState::StaticClass();
	PlayerControllerClass = AZombiePlayerController::StaticClass();
	DefaultPawnClass = AZombiePlayerCharacter::StaticClass();

	SectorGenerator = CreateDefaultSubobject<USectorGeneratorComponent>(TEXT("SectorGenerator"));
	SpawnDirector = CreateDefaultSubobject<USpawnDirectorComponent>(TEXT("SpawnDirector"));
	Rewards = CreateDefaultSubobject<URunRewardsComponent>(TEXT("Rewards"));
}

void AZombieGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	UZombieSaveSubsystem* Saves = UZombieSaveSubsystem::Get(this);
	const bool bContinue = UGameplayStatics::ParseOption(Options, TEXT("Run")) == TEXT("Continue");

	if (bContinue && Saves && Saves->LoadRun(PendingRestore))
	{
		bHasPendingRestore = true;
		bResumeInIntermission = PendingRestore.bInIntermission;
		RunSeed = PendingRestore.RunSeed;
		CurrentSector = PendingRestore.Sector;
		UE_LOG(LogZombieGame, Log, TEXT("Resuming run %d at sector %d%s."), RunSeed, CurrentSector, bResumeInIntermission ? TEXT(" (intermission)") : TEXT(""));
	}
	else
	{
		RunSeed = FMath::Rand();
		CurrentSector = 1;
		if (Saves)
		{
			Saves->DeleteRun();
			++Saves->GetMeta()->Lifetime.RunsStarted;
			Saves->SaveMeta();
		}
	}

	// InitGame runs before any player logs in, which is exactly when the level has to exist: the
	// generated start room is what ChoosePlayerStart hands back for the very first spawn.
	if (bResumeInIntermission)
	{
		SectorGenerator->GenerateIntermission(CurrentSector, GetSeedForSector(CurrentSector) ^ 0x1e7);
	}
	else
	{
		BuildSector(CurrentSector);
	}
}

void AZombieGameMode::BeginPlay()
{
	Super::BeginPlay();

	SpawnDirector->OnSectorCleared.AddUObject(this, &AZombieGameMode::HandleSectorCleared);
	SpawnDirector->OnBossSpawned.AddUObject(this, &AZombieGameMode::HandleBossSpawned);
	if (UZombieEnemyManager* Enemies = UZombieEnemyManager::Get(this))
	{
		Enemies->OnEnemyDied.AddUObject(this, &AZombieGameMode::HandleEnemyDied);
	}

	GetWorldTimerManager().SetTimer(RunClockTimer, this, &AZombieGameMode::TickRunClock, 1.0f, true);

	if (bResumeInIntermission)
	{
		EnterIntermission();
	}
	else
	{
		StartEncounter(CurrentSector);
	}
}

int32 AZombieGameMode::GetSeedForSector(int32 Sector) const
{
	return static_cast<int32>(HashCombine(GetTypeHash(RunSeed), GetTypeHash(Sector)));
}

bool AZombieGameMode::BuildSector(int32 Sector)
{
	const bool bBossSector = UZombieRunSettings::GetOrLoadDefault()->IsBossSector(Sector);
	if (!SectorGenerator->GenerateSector(Sector, GetSeedForSector(Sector), bBossSector))
	{
		UE_LOG(LogZombieGame, Error, TEXT("Failed to build sector %d; the run cannot continue."), Sector);
		return false;
	}
	return true;
}

AActor* AZombieGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	if (APlayerStart* GeneratedStart = SectorGenerator->GetGeneratedPlayerStart())
	{
		return GeneratedStart;
	}

	// Falls back to whatever PlayerStart the map itself provides - useful for hand-built test maps.
	return Super::ChoosePlayerStart_Implementation(Player);
}

void AZombieGameMode::FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation)
{
	Super::FinishRestartPlayer(NewPlayer, StartRotation);

	if (!NewPlayer || !NewPlayer->GetPawn())
	{
		return;
	}

	if (bHasPendingRestore)
	{
		FZombieRunState::Apply(*NewPlayer, PendingRestore);
	}
	else
	{
		FZombieRunState::ApplyStartingLoadout(*NewPlayer, *UZombieRunSettings::GetOrLoadDefault(), UShopSettings::GetOrLoadDefault()->StartingSlots);
	}

	if (AZombiePlayerCharacter* Character = Cast<AZombiePlayerCharacter>(NewPlayer->GetPawn()))
	{
		Character->OnPlayerDied.AddUObject(this, &AZombieGameMode::HandlePlayerDied);
	}
}

void AZombieGameMode::SetPhase(ESectorPhase NewPhase)
{
	if (AZombieGameState* ZombieGameState = GetGameState<AZombieGameState>())
	{
		ZombieGameState->SetSectorPhase(NewPhase);
	}
}

void AZombieGameMode::StartEncounter(int32 Sector)
{
	const UZombieRunSettings* Settings = UZombieRunSettings::GetOrLoadDefault();
	const bool bBossSector = Settings->IsBossSector(Sector);

	if (AZombieGameState* ZombieGameState = GetGameState<AZombieGameState>())
	{
		ZombieGameState->AdvanceToSector(Sector, bBossSector);
		ZombieGameState->SetDifficultyLevel(1.0f + (Sector - 1) * Settings->DifficultyIncreasePerSector);
		ZombieGameState->SetActiveBoss(nullptr);
	}
	SetPhase(ESectorPhase::Combat);

	if (UZombieAudioSubsystem* Audio = UZombieAudioSubsystem::Get(this))
	{
		Audio->SetMusicState(EZombieMusicState::Sector);
	}

	SpawnDirector->BeginSector(Sector, SectorGenerator->GetZombieSpawnAreas(), bBossSector ? PickBossArchetype(Sector) : nullptr);

	// No arena fit this sector: the boss comes looking for the player straight away.
	if (bBossSector && !SectorGenerator->HasBossArena())
	{
		SpawnDirector->ReleaseBoss(SectorGenerator->GetBossSpawnLocation());
	}

	ZombieNotifications::NotifyAll(this, FText::Format(bBossSector ? LOCTEXT("BossSectorStart", "SECTOR {0} - something big is waiting")
		: LOCTEXT("SectorStart", "SECTOR {0}"), FText::AsNumber(Sector)), FLinearColor(0.55f, 0.85f, 0.25f));

	// A checkpoint taken as the sector begins: quitting mid-fight resumes here, never mid-fight.
	SaveCheckpoint(false);
	bHasPendingRestore = false;
}

const UZombieArchetypeDataAsset* AZombieGameMode::PickBossArchetype(int32 Sector) const
{
	TArray<UZombieArchetypeDataAsset*> Bosses;
	FZombiePrimaryAssetLoader::LoadAllOfType(UZombieArchetypeDataAsset::AssetType, Bosses);
	Bosses.RemoveAll([Sector](const UZombieArchetypeDataAsset* Archetype)
	{
		return !Archetype || Archetype->Tier != EZombieClassTier::Boss || Archetype->MinSector > Sector;
	});
	if (Bosses.Num() == 0)
	{
		return nullptr;
	}

	Bosses.Sort([](const UZombieArchetypeDataAsset& Lhs, const UZombieArchetypeDataAsset& Rhs) { return Lhs.GetName() < Rhs.GetName(); });
	FRandomStream Random(GetSeedForSector(Sector));
	return Bosses[Random.RandRange(0, Bosses.Num() - 1)];
}

void AZombieGameMode::HandleEnemyDied(AZombieCharacter* Zombie, AController* Killer)
{
	Rewards->HandleEnemyDied(Zombie, Killer, CurrentSector);

	AZombieGameState* ZombieGameState = GetGameState<AZombieGameState>();
	if (Zombie && Zombie->IsBoss() && ZombieGameState && ZombieGameState->GetActiveBoss() == Zombie)
	{
		ZombieGameState->SetActiveBoss(nullptr);
		ZombieNotifications::NotifyAll(this, LOCTEXT("BossDown", "BOSS DEFEATED"), FLinearColor(1.0f, 0.62f, 0.1f));
		if (UZombieAudioSubsystem* Audio = UZombieAudioSubsystem::Get(this))
		{
			Audio->SetMusicState(EZombieMusicState::Sector);
		}
		EvaluateAchievements();
	}
}

void AZombieGameMode::HandleBossSpawned(AZombieCharacter* Boss)
{
	if (AZombieGameState* ZombieGameState = GetGameState<AZombieGameState>())
	{
		ZombieGameState->SetActiveBoss(Boss);
	}
	if (UZombieAudioSubsystem* Audio = UZombieAudioSubsystem::Get(this))
	{
		Audio->SetMusicState(EZombieMusicState::Boss);
	}
}

void AZombieGameMode::NotifyBossRoomEntered(APawn* Entrant)
{
	if (SpawnDirector->IsBossPending())
	{
		SpawnDirector->ReleaseBoss(SectorGenerator->GetBossSpawnLocation());
	}
}

void AZombieGameMode::HandleSectorCleared(const FVector& LastKillLocation)
{
	UE_LOG(LogZombieGame, Log, TEXT("Sector %d cleared; dropping the key."), CurrentSector);

	SetPhase(ESectorPhase::KeyDropped);
	Rewards->DropSectorKey(LastKillLocation);
	Rewards->SweepMoneyToPlayers();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AZombiePlayerState* PlayerState = It->IsValid() ? It->Get()->GetPlayerState<AZombiePlayerState>() : nullptr)
		{
			PlayerState->RecordSectorCleared();
		}
	}

	ZombieNotifications::NotifyAll(this, LOCTEXT("Cleared", "SECTOR CLEARED - take the key"), FLinearColor(1.0f, 0.82f, 0.25f));
	if (UZombieAudioSubsystem* Audio = UZombieAudioSubsystem::Get(this))
	{
		Audio->PlayNamedSound2D(TEXT("Sector.Cleared"));
	}
	EvaluateAchievements();
}

void AZombieGameMode::NotifyKeyCollected(APawn* Collector)
{
	const AZombieGameState* ZombieGameState = GetGameState<AZombieGameState>();
	if (!ZombieGameState || ZombieGameState->GetSectorPhase() != ESectorPhase::KeyDropped)
	{
		return;
	}

	SetPhase(ESectorPhase::ExitUnlocked);
	if (AZombieExitDoor* Door = SectorGenerator->GetExitDoor())
	{
		Door->SetUnlocked(true);
	}
	if (UZombieAudioSubsystem* Audio = UZombieAudioSubsystem::Get(this))
	{
		Audio->PlayNamedSound2D(TEXT("Door.Unlock"));
	}
	ZombieNotifications::NotifyAll(this, LOCTEXT("ExitOpen", "The exit is unlocked"), FLinearColor(0.55f, 0.85f, 0.25f));
}

void AZombieGameMode::NotifyExitUsed(APawn* User)
{
	const AZombieGameState* ZombieGameState = GetGameState<AZombieGameState>();
	if (ZombieGameState && ZombieGameState->GetSectorPhase() == ESectorPhase::ExitUnlocked)
	{
		EnterIntermission();
	}
}

void AZombieGameMode::EnterIntermission()
{
	SpawnDirector->StopSector();
	Rewards->ClearPickups();

	// A resumed intermission was already built in InitGame; a fresh one replaces the sector.
	if (!bResumeInIntermission && !SectorGenerator->GenerateIntermission(CurrentSector, GetSeedForSector(CurrentSector) ^ 0x1e7))
	{
		UE_LOG(LogZombieGame, Error, TEXT("Could not build the intermission; skipping straight to the next sector."));
		AdvanceToNextSector();
		return;
	}
	bResumeInIntermission = false;

	MovePlayersToSectorStart();

	if (!ShopState)
	{
		ShopState = GetWorld()->SpawnActor<AZombieShopState>();
	}
	ShopState->GenerateStock(CurrentSector + 1, GetSeedForSector(CurrentSector) ^ 0x5409, bHasPendingRestore ? PendingRestore.PurchasedOfferIds : TArray<int32>());

	if (AZombieGameState* ZombieGameState = GetGameState<AZombieGameState>())
	{
		ZombieGameState->AdvanceToSector(CurrentSector, false);
		ZombieGameState->SetShop(ShopState);
		ZombieGameState->SetActiveBoss(nullptr);
	}
	SetPhase(ESectorPhase::Intermission);

	if (UZombieAudioSubsystem* Audio = UZombieAudioSubsystem::Get(this))
	{
		Audio->SetMusicState(EZombieMusicState::Intermission);
	}

	SaveCheckpoint(true);
	bHasPendingRestore = false;

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AZombiePlayerController* Controller = Cast<AZombiePlayerController>(It->Get()))
		{
			Controller->ClientOpenShop();
		}
	}
}

void AZombieGameMode::NotifyIntermissionLeft(APawn* User)
{
	const AZombieGameState* ZombieGameState = GetGameState<AZombieGameState>();
	if (!ZombieGameState || ZombieGameState->GetSectorPhase() != ESectorPhase::Intermission)
	{
		return;
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AZombiePlayerController* Controller = Cast<AZombiePlayerController>(It->Get()))
		{
			Controller->ClientCloseShop();
		}
	}

	AdvanceToNextSector();
}

void AZombieGameMode::AdvanceToNextSector()
{
	SpawnDirector->StopSector();
	Rewards->ClearPickups();

	if (AZombieGameState* ZombieGameState = GetGameState<AZombieGameState>())
	{
		ZombieGameState->SetShop(nullptr);
	}
	if (ShopState)
	{
		ShopState->Destroy();
		ShopState = nullptr;
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
	APlayerStart* SectorStart = SectorGenerator->GetGeneratedPlayerStart();
	if (!World || !SectorStart)
	{
		return;
	}

	// Existing pawns are relocated rather than respawned: health, money, inventory and perks all
	// persist across sectors, so the run state has to survive the transition intact.
	for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PlayerController = Iterator->Get();
		if (APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr)
		{
			Pawn->TeleportTo(SectorStart->GetActorLocation(), Pawn->GetActorRotation());
		}
	}
}

void AZombieGameMode::SaveCheckpoint(bool bInIntermission)
{
	UZombieSaveSubsystem* Saves = UZombieSaveSubsystem::Get(this);
	APlayerController* Player = GetWorld()->GetFirstPlayerController();
	if (!Saves || !Player || !Player->GetPawn() || bRunOver)
	{
		return;
	}

	// Single-player checkpoint: the first local player's state is the run's state.
	FZombieRunSaveData Data;
	FZombieRunState::Capture(*Player, Data);
	Data.RunSeed = RunSeed;
	Data.Sector = CurrentSector;
	Data.bInIntermission = bInIntermission;
	if (bInIntermission && ShopState)
	{
		Data.PurchasedOfferIds = ShopState->GetPurchasedOfferIds();
	}
	Saves->SaveRun(Data);
}

void AZombieGameMode::TickRunClock()
{
	if (bRunOver)
	{
		return;
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AZombiePlayerState* PlayerState = It->IsValid() ? It->Get()->GetPlayerState<AZombiePlayerState>() : nullptr)
		{
			PlayerState->RecordElapsedTime(1.0f);
		}
	}

	// The intermission checkpoint tracks purchases as they happen, so quitting can't undo them.
	const AZombieGameState* ZombieGameState = GetGameState<AZombieGameState>();
	if (ZombieGameState && ZombieGameState->GetSectorPhase() == ESectorPhase::Intermission && ShopState
		&& ShopState->GetPurchasedOfferIds().Num() != PendingRestore.PurchasedOfferIds.Num())
	{
		PendingRestore.PurchasedOfferIds = ShopState->GetPurchasedOfferIds();
		SaveCheckpoint(true);
	}
}

void AZombieGameMode::HandlePlayerDied(AZombiePlayerCharacter* Character)
{
	// The run ends when nobody is left standing.
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		const AZombiePlayerCharacter* Player = It->IsValid() ? Cast<AZombiePlayerCharacter>(It->Get()->GetPawn()) : nullptr;
		if (Player && !Player->IsDead())
		{
			return;
		}
	}

	EndRun();
}

void AZombieGameMode::EndRun()
{
	if (bRunOver)
	{
		return;
	}
	bRunOver = true;

	SetPhase(ESectorPhase::GameOver);
	if (UZombieAudioSubsystem* Audio = UZombieAudioSubsystem::Get(this))
	{
		Audio->SetMusicState(EZombieMusicState::GameOver);
	}

	// "No permanent progression": the checkpoint goes, only statistics and records remain.
	APlayerController* Player = GetWorld()->GetFirstPlayerController();
	FZombieRunSaveData Final;
	if (Player)
	{
		FZombieRunState::Capture(*Player, Final);
	}
	Final.Sector = CurrentSector;

	if (UZombieSaveSubsystem* Saves = UZombieSaveSubsystem::Get(this))
	{
		Saves->DeleteRun();
		Saves->RecordRunEnded(Final);
	}
	EvaluateAchievements();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AZombiePlayerController* Controller = Cast<AZombiePlayerController>(It->Get());
		const AZombiePlayerState* PlayerState = Controller ? Controller->GetPlayerState<AZombiePlayerState>() : nullptr;
		if (Controller && PlayerState)
		{
			const FZombieRunStats& Stats = PlayerState->GetRunStats();
			UE_LOG(LogZombieGame, Log, TEXT("Run over at sector %d: %d kills, %d bosses, $%d earned, %.0f damage taken, %.0fs."),
				CurrentSector, Stats.Kills, Stats.BossesDefeated, Stats.MoneyEarned, Stats.DamageTaken, Stats.ElapsedSeconds);
			Controller->ClientShowRunSummary(Stats, CurrentSector);
		}
	}
}

void AZombieGameMode::EvaluateAchievements() const
{
	UZombieAchievementSubsystem* Achievements = UZombieAchievementSubsystem::Get(this);
	const APlayerController* Player = GetWorld()->GetFirstPlayerController();
	const AZombiePlayerState* PlayerState = Player ? Player->GetPlayerState<AZombiePlayerState>() : nullptr;
	if (Achievements)
	{
		Achievements->Evaluate(PlayerState && !bRunOver ? &PlayerState->GetRunStats() : nullptr, CurrentSector);
	}
}

#undef LOCTEXT_NAMESPACE
