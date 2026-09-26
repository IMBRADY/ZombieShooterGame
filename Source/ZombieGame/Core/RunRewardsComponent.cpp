#include "RunRewardsComponent.h"
#include "Characters/Zombies/ZombieArchetypeDataAsset.h"
#include "Characters/Zombies/ZombieCharacter.h"
#include "Components/DamageComponent.h"
#include "Core/ZombiePlayerState.h"
#include "Core/ZombieRunSettings.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "Loot/LootPickup.h"
#include "Loot/LootResolver.h"
#include "Loot/LootTableDataAsset.h"
#include "Utilities/ActorPoolSubsystem.h"
#include "Weapons/WeaponDataAsset.h"
#include "Weapons/ZombieWeapon.h"

URunRewardsComponent::URunRewardsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

FText URunRewardsComponent::ResolveWeaponName(const AZombieCharacter& Zombie) const
{
	// The causer is the weapon for hitscan and the weapon too for projectiles (their source);
	// status-effect ticks credit whatever applied them.
	const UDamageComponent* Damage = Zombie.FindComponentByClass<UDamageComponent>();
	const AZombieWeapon* Weapon = Damage ? Cast<AZombieWeapon>(Damage->GetLastDamageCauser()) : nullptr;
	const UWeaponDataAsset* Definition = Weapon ? Weapon->GetDefinition() : nullptr;
	return Definition ? Definition->DisplayName : FText::GetEmpty();
}

void URunRewardsComponent::CreditKill(const AZombieCharacter& Zombie, AController* Killer) const
{
	AZombiePlayerState* KillerState = Killer ? Killer->GetPlayerState<AZombiePlayerState>() : nullptr;
	if (!KillerState)
	{
		return;
	}

	KillerState->RecordKill(ResolveWeaponName(Zombie));
	if (Zombie.IsBoss())
	{
		KillerState->RecordBossDefeated();
	}
}

void URunRewardsComponent::DropRewards(const AZombieCharacter& Zombie, int32 Sector) const
{
	UWorld* World = GetWorld();
	const UZombieArchetypeDataAsset* Archetype = Zombie.GetArchetype();
	if (!World || !Archetype || !Zombie.GrantsRewards())
	{
		return;
	}

	const FVector Location = Zombie.GetActorLocation();

	// "Money dropped by zombies" - one pickup per zombie, worth its (difficulty-scaled) reward.
	if (Zombie.GetMoneyReward() > 0)
	{
		FResolvedLoot Money;
		Money.Type = ELootRewardType::Money;
		Money.Amount = Zombie.GetMoneyReward();
		ALootPickup::SpawnLoot(World, Money, Location);
	}

	FRandomStream Random(FMath::Rand());
	TArray<FResolvedLoot> Extras;
	if (Archetype->LootTable)
	{
		FLootResolver::Roll(*Archetype->LootTable, Sector, Zombie.GetRewardMultiplier(), Random, Extras);
	}

	const UZombieRunSettings* RunSettings = UZombieRunSettings::GetOrLoadDefault();
	if (Zombie.IsBoss() && RunSettings->BossRewardTable)
	{
		FLootResolver::Roll(*RunSettings->BossRewardTable, Sector, Zombie.GetRewardMultiplier(), Random, Extras);
	}

	for (const FResolvedLoot& Loot : Extras)
	{
		ALootPickup::SpawnLoot(World, Loot, Location);
	}
}

void URunRewardsComponent::HandleEnemyDied(AZombieCharacter* Zombie, AController* Killer, int32 Sector)
{
	if (!Zombie)
	{
		return;
	}

	CreditKill(*Zombie, Killer);
	DropRewards(*Zombie, Sector);
}

void URunRewardsComponent::DropSectorKey(const FVector& Location)
{
	FResolvedLoot Key;
	Key.Type = ELootRewardType::Key;
	ALootPickup::SpawnLoot(GetWorld(), Key, Location, false);
}

void URunRewardsComponent::SweepMoneyToPlayers()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<ALootPickup> It(World); It; ++It)
	{
		ALootPickup* Pickup = *It;
		if (Pickup->IsHidden() || Pickup->GetLoot().Type != ELootRewardType::Money)
		{
			continue;
		}

		APawn* Nearest = nullptr;
		float NearestDistance = TNumericLimits<float>::Max();
		for (FConstPlayerControllerIterator PC = World->GetPlayerControllerIterator(); PC; ++PC)
		{
			APawn* Pawn = PC->IsValid() ? PC->Get()->GetPawn() : nullptr;
			const float Distance = Pawn ? FVector::DistSquared(Pawn->GetActorLocation(), Pickup->GetActorLocation()) : NearestDistance;
			if (Distance < NearestDistance)
			{
				NearestDistance = Distance;
				Nearest = Pawn;
			}
		}
		Pickup->AttractTo(Nearest);
	}
}

void URunRewardsComponent::ClearPickups()
{
	UWorld* World = GetWorld();
	UActorPoolSubsystem* Pool = UActorPoolSubsystem::Get(World);
	if (!World || !Pool)
	{
		return;
	}

	for (TActorIterator<ALootPickup> It(World); It; ++It)
	{
		if (!It->IsHidden())
		{
			Pool->Release(*It);
		}
	}
}
