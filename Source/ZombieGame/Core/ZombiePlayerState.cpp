#include "ZombiePlayerState.h"
#include "Net/UnrealNetwork.h"

void AZombiePlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AZombiePlayerState, Money);
	DOREPLIFETIME(AZombiePlayerState, Kills);
	DOREPLIFETIME(AZombiePlayerState, BossesDefeated);
	DOREPLIFETIME(AZombiePlayerState, DamageTaken);
}

void AZombiePlayerState::AddMoney(int32 Amount)
{
	if (Amount <= 0)
	{
		return;
	}

	Money += Amount;
	OnMoneyChanged.Broadcast(Money);
}

bool AZombiePlayerState::SpendMoney(int32 Amount)
{
	if (Amount <= 0 || Amount > Money)
	{
		return false;
	}

	Money -= Amount;
	OnMoneyChanged.Broadcast(Money);
	return true;
}

void AZombiePlayerState::AddKill()
{
	++Kills;
}

void AZombiePlayerState::AddBossDefeated()
{
	++BossesDefeated;
}

void AZombiePlayerState::AddDamageTaken(float Amount)
{
	if (Amount > 0.0f)
	{
		DamageTaken += Amount;
	}
}
