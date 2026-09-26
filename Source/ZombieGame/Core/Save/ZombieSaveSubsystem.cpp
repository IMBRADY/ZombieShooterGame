#include "ZombieSaveSubsystem.h"
#include "Core/Save/ZombieSaveGames.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "ZombieGame.h"

const FString UZombieSaveSubsystem::MetaSlotName = TEXT("ZombieMeta");
const FString UZombieSaveSubsystem::RunSlotName = TEXT("ZombieRun");

UZombieSaveSubsystem* UZombieSaveSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UZombieSaveSubsystem>() : nullptr;
}

void UZombieSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UGameplayStatics::DoesSaveGameExist(MetaSlotName, UserIndex))
	{
		Meta = Cast<UZombieMetaSaveGame>(UGameplayStatics::LoadGameFromSlot(MetaSlotName, UserIndex));
	}

	if (!Meta || Meta->Version != UZombieMetaSaveGame::CurrentVersion)
	{
		if (Meta)
		{
			UE_LOG(LogZombieGame, Warning, TEXT("Meta save version %d is not %d; starting a fresh profile."),
				Meta->Version, UZombieMetaSaveGame::CurrentVersion);
		}
		Meta = Cast<UZombieMetaSaveGame>(UGameplayStatics::CreateSaveGameObject(UZombieMetaSaveGame::StaticClass()));
		SaveMeta();
	}
}

void UZombieSaveSubsystem::SaveMeta()
{
	if (!Meta)
	{
		return;
	}

	if (!UGameplayStatics::SaveGameToSlot(Meta, MetaSlotName, UserIndex))
	{
		UE_LOG(LogZombieGame, Error, TEXT("Failed to write the meta save."));
	}
	OnMetaSaveChanged.Broadcast();
}

void UZombieSaveSubsystem::RecordRunEnded(const FZombieRunSaveData& FinalState)
{
	if (!Meta)
	{
		return;
	}

	FZombieLifetimeStats& Lifetime = Meta->Lifetime;
	Lifetime.Deaths += 1;
	Lifetime.TotalKills += FinalState.Stats.Kills;
	Lifetime.BossesDefeated += FinalState.Stats.BossesDefeated;
	Lifetime.SectorsCleared += FinalState.Stats.SectorsCleared;
	Lifetime.HighestSector = FMath::Max(Lifetime.HighestSector, FinalState.Sector);
	Lifetime.MoneyEarned += FinalState.Stats.MoneyEarned;
	Lifetime.PlaySeconds += FinalState.Stats.ElapsedSeconds;

	FZombieHighScore Score;
	Score.Sector = FinalState.Sector;
	Score.Kills = FinalState.Stats.Kills;
	Score.MoneyEarned = FinalState.Stats.MoneyEarned;
	Score.Seconds = FinalState.Stats.ElapsedSeconds;
	Score.Date = FDateTime::Now();

	Meta->HighScores.Add(Score);
	Meta->HighScores.Sort([](const FZombieHighScore& Lhs, const FZombieHighScore& Rhs)
	{
		return Lhs.Sector != Rhs.Sector ? Lhs.Sector > Rhs.Sector : Lhs.Kills > Rhs.Kills;
	});
	if (Meta->HighScores.Num() > MaxHighScores)
	{
		Meta->HighScores.SetNum(MaxHighScores);
	}

	SaveMeta();
}

bool UZombieSaveSubsystem::HasRunSave() const
{
	return UGameplayStatics::DoesSaveGameExist(RunSlotName, UserIndex);
}

bool UZombieSaveSubsystem::LoadRun(FZombieRunSaveData& OutRun) const
{
	const UZombieRunSaveGame* Save = HasRunSave()
		? Cast<UZombieRunSaveGame>(UGameplayStatics::LoadGameFromSlot(RunSlotName, UserIndex))
		: nullptr;

	if (!Save || Save->Version != UZombieRunSaveGame::CurrentVersion)
	{
		return false;
	}

	OutRun = Save->Run;
	return true;
}

void UZombieSaveSubsystem::SaveRun(const FZombieRunSaveData& Run)
{
	UZombieRunSaveGame* Save = Cast<UZombieRunSaveGame>(UGameplayStatics::CreateSaveGameObject(UZombieRunSaveGame::StaticClass()));
	Save->Run = Run;
	Save->Run.SavedAt = FDateTime::Now();

	// Asynchronous: checkpoints happen at sector transitions, which must not hitch.
	UGameplayStatics::AsyncSaveGameToSlot(Save, RunSlotName, UserIndex);
}

void UZombieSaveSubsystem::DeleteRun()
{
	if (HasRunSave())
	{
		UGameplayStatics::DeleteGameInSlot(RunSlotName, UserIndex);
	}
}
