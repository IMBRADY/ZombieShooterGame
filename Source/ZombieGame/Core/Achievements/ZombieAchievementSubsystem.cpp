#include "ZombieAchievementSubsystem.h"
#include "Core/Achievements/AchievementDataAsset.h"
#include "Core/Save/ZombieSaveGames.h"
#include "Core/Save/ZombieSaveSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Interfaces/OnlineAchievementsInterface.h"
#include "OnlineSubsystem.h"
#include "Utilities/ZombiePrimaryAssetLoader.h"
#include "ZombieGame.h"

UZombieAchievementSubsystem* UZombieAchievementSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UZombieAchievementSubsystem>() : nullptr;
}

void UZombieAchievementSubsystem::GetAllAchievements(TArray<const UAchievementDataAsset*>& OutAchievements) const
{
	TArray<UAchievementDataAsset*> Loaded;
	FZombiePrimaryAssetLoader::LoadAllOfType(UAchievementDataAsset::AssetType, Loaded);
	Loaded.Sort([](const UAchievementDataAsset& Lhs, const UAchievementDataAsset& Rhs) { return Lhs.GetName() < Rhs.GetName(); });
	OutAchievements.Append(Loaded);
}

bool UZombieAchievementSubsystem::IsUnlocked(const UAchievementDataAsset* Achievement) const
{
	const UZombieSaveSubsystem* Saves = GetGameInstance()->GetSubsystem<UZombieSaveSubsystem>();
	return Achievement && Saves && Saves->GetMeta() && Saves->GetMeta()->UnlockedAchievements.Contains(Achievement->GetFName());
}

int32 UZombieAchievementSubsystem::GetLifetimeProgress(const UAchievementDataAsset* Achievement) const
{
	return Achievement ? GetStatValue(*Achievement, nullptr, 0) : 0;
}

int32 UZombieAchievementSubsystem::GetStatValue(const UAchievementDataAsset& Achievement, const FZombieRunStats* LiveRun, int32 LiveSector) const
{
	const UZombieSaveSubsystem* Saves = GetGameInstance()->GetSubsystem<UZombieSaveSubsystem>();
	const UZombieMetaSaveGame* Meta = Saves ? Saves->GetMeta() : nullptr;
	if (!Meta)
	{
		return 0;
	}

	const FZombieLifetimeStats& Life = Meta->Lifetime;
	const bool bRun = Achievement.Scope == EAchievementScope::SingleRun;
	const bool bAddLive = LiveRun && !bRun;

	switch (Achievement.Stat)
	{
	case EAchievementStat::Kills:			return bRun ? (LiveRun ? LiveRun->Kills : 0) : Life.TotalKills + (bAddLive ? LiveRun->Kills : 0);
	case EAchievementStat::BossesDefeated:	return bRun ? (LiveRun ? LiveRun->BossesDefeated : 0) : Life.BossesDefeated + (bAddLive ? LiveRun->BossesDefeated : 0);
	case EAchievementStat::SectorsCleared:	return bRun ? (LiveRun ? LiveRun->SectorsCleared : 0) : Life.SectorsCleared + (bAddLive ? LiveRun->SectorsCleared : 0);
	case EAchievementStat::MoneyEarned:		return bRun ? (LiveRun ? LiveRun->MoneyEarned : 0) : Life.MoneyEarned + (bAddLive ? LiveRun->MoneyEarned : 0);
	case EAchievementStat::SectorReached:	return FMath::Max(bRun ? 0 : Life.HighestSector, LiveSector);
	case EAchievementStat::PerksBought:		return Life.PerksBought;
	case EAchievementStat::LegendariesFound: return Life.LegendariesFound;
	case EAchievementStat::RunsStarted:		return Life.RunsStarted;
	case EAchievementStat::Deaths:			return Life.Deaths;
	default:								return 0;
	}
}

void UZombieAchievementSubsystem::Evaluate(const FZombieRunStats* LiveRun, int32 LiveSector)
{
	TArray<const UAchievementDataAsset*> All;
	GetAllAchievements(All);

	for (const UAchievementDataAsset* Achievement : All)
	{
		if (Achievement && !IsUnlocked(Achievement) && GetStatValue(*Achievement, LiveRun, LiveSector) >= Achievement->Threshold)
		{
			Unlock(*Achievement);
		}
	}
}

void UZombieAchievementSubsystem::Unlock(const UAchievementDataAsset& Achievement)
{
	UZombieSaveSubsystem* Saves = GetGameInstance()->GetSubsystem<UZombieSaveSubsystem>();
	if (!Saves || !Saves->GetMeta())
	{
		return;
	}

	Saves->GetMeta()->UnlockedAchievements.AddUnique(Achievement.GetFName());
	Saves->SaveMeta();

	UE_LOG(LogZombieGame, Log, TEXT("Achievement unlocked: %s"), *Achievement.GetName());
	ReportToPlatform(Achievement);
	OnAchievementUnlocked.Broadcast(&Achievement);
}

void UZombieAchievementSubsystem::ReportToPlatform(const UAchievementDataAsset& Achievement) const
{
	IOnlineSubsystem* Online = IOnlineSubsystem::Get();
	IOnlineAchievementsPtr Achievements = Online ? Online->GetAchievementsInterface() : nullptr;
	const ULocalPlayer* LocalPlayer = GetGameInstance()->GetFirstGamePlayer();
	if (!Achievements.IsValid() || !LocalPlayer)
	{
		return;
	}

	const FUniqueNetIdRepl NetId = LocalPlayer->GetPreferredUniqueNetId();
	if (!NetId.IsValid())
	{
		return;
	}

	FOnlineAchievementsWriteRef Write = MakeShared<FOnlineAchievementsWrite, ESPMode::ThreadSafe>();
	Write->SetFloatStat(Achievement.GetName(), 100.0f);
	FOnlineAchievementsWriteRef WriteRef = Write;
	Achievements->WriteAchievements(*NetId, WriteRef);
}
