#include "ZombiePlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Perks/PerkComponent.h"

AZombiePlayerState::AZombiePlayerState()
{
	Perks = CreateDefaultSubobject<UPerkComponent>(TEXT("Perks"));
}

void AZombiePlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AZombiePlayerState, Money);
	DOREPLIFETIME(AZombiePlayerState, RunStats);
}

void AZombiePlayerState::AddMoney(int32 Amount, bool bCountsAsEarned)
{
	if (Amount <= 0)
	{
		return;
	}

	Money += Amount;
	if (bCountsAsEarned)
	{
		RunStats.MoneyEarned += Amount;
		OnRunStatsChanged.Broadcast();
	}
	OnMoneyChanged.Broadcast(Money);
}

bool AZombiePlayerState::SpendMoney(int32 Amount)
{
	if (Amount < 0 || Amount > Money)
	{
		return false;
	}

	Money -= Amount;
	OnMoneyChanged.Broadcast(Money);
	return true;
}

void AZombiePlayerState::SetMoney(int32 NewMoney)
{
	Money = FMath::Max(NewMoney, 0);
	OnMoneyChanged.Broadcast(Money);
}

void AZombiePlayerState::SetRunStats(const FZombieRunStats& Stats)
{
	RunStats = Stats;
	OnRunStatsChanged.Broadcast();
}

void AZombiePlayerState::RecordKill(const FText& WeaponName)
{
	++RunStats.Kills;
	RunStats.AddWeaponKill(WeaponName);
	OnRunStatsChanged.Broadcast();
}

void AZombiePlayerState::RecordBossDefeated()
{
	++RunStats.BossesDefeated;
	OnRunStatsChanged.Broadcast();
}

void AZombiePlayerState::RecordDamageTaken(float Amount)
{
	if (Amount > 0.0f)
	{
		RunStats.DamageTaken += Amount;
		OnRunStatsChanged.Broadcast();
	}
}

void AZombiePlayerState::RecordSectorCleared()
{
	++RunStats.SectorsCleared;
	OnRunStatsChanged.Broadcast();
}

void AZombiePlayerState::RecordElapsedTime(float Seconds)
{
	// Not broadcast: this ticks up continuously and nothing needs to react to every second of it.
	RunStats.ElapsedSeconds += FMath::Max(Seconds, 0.0f);
}

void AZombiePlayerState::OnRep_Money()
{
	OnMoneyChanged.Broadcast(Money);
}

void AZombiePlayerState::OnRep_RunStats()
{
	OnRunStatsChanged.Broadcast();
}
