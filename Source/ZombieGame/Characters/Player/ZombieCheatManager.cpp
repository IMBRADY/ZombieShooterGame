#include "ZombieCheatManager.h"
#include "Characters/Player/ZombiePlayerController.h"
#include "Characters/Zombies/ZombieCharacter.h"
#include "Characters/Zombies/ZombieEnemyManager.h"
#include "Components/DamageComponent.h"
#include "Components/HealthComponent.h"
#include "Components/InventoryComponent.h"
#include "Core/Achievements/ZombieAchievementSubsystem.h"
#include "Core/ZombieGameState.h"
#include "Core/ZombiePlayerState.h"
#include "Core/ZombieRunController.h"
#include "Core/ZombieDamageTypes.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "Loot/LootPickup.h"
#include "Shop/ShopTransactionComponent.h"
#include "Shop/ZombieShopState.h"
#include "TimerManager.h"
#include "Weapons/ZombieWeapon.h"
#include "ZombieGame.h"

APawn* UZombieCheatManager::GetPlayerPawn() const
{
	const APlayerController* PC = GetOuterAPlayerController();
	return PC ? PC->GetPawn() : nullptr;
}

void UZombieCheatManager::ZombieGod()
{
	bGodMode = !bGodMode;
	if (UDamageComponent* Damage = GetPlayerPawn() ? GetPlayerPawn()->FindComponentByClass<UDamageComponent>() : nullptr)
	{
		Damage->SetIncomingDamageMultiplier(bGodMode ? 0.0f : 1.0f);
	}
	UE_LOG(LogZombieGame, Log, TEXT("[Cheat] God mode %s"), bGodMode ? TEXT("on") : TEXT("off"));
}

void UZombieCheatManager::ZombieKillAll()
{
	UZombieEnemyManager* Enemies = UZombieEnemyManager::Get(GetWorld());
	APlayerController* PC = GetOuterAPlayerController();
	APawn* Pawn = GetPlayerPawn();
	if (!Enemies || !PC)
	{
		return;
	}

	// Kills are credited to the player's drawn weapon, exactly as real shots would be.
	UInventoryComponent* Inventory = Pawn ? Pawn->FindComponentByClass<UInventoryComponent>() : nullptr;
	AActor* Causer = Inventory && Inventory->GetActiveWeapon() ? static_cast<AActor*>(Inventory->GetActiveWeapon()) : Pawn;

	const TArray<TObjectPtr<AZombieCharacter>> Snapshot = Enemies->GetLiveZombies();
	for (AZombieCharacter* Zombie : Snapshot)
	{
		if (IsValid(Zombie) && !Zombie->IsDead())
		{
			UGameplayStatics::ApplyDamage(Zombie, 100000.0f, PC, Causer, UDamageType_Bullet::StaticClass());
		}
	}
}

void UZombieCheatManager::ZombieGiveMoney(int32 Amount)
{
	if (AZombiePlayerState* PlayerState = GetOuterAPlayerController()->GetPlayerState<AZombiePlayerState>())
	{
		PlayerState->AddMoney(Amount, false);
	}
}

void UZombieCheatManager::ZombieCollectKey()
{
	APawn* Pawn = GetPlayerPawn();
	for (TActorIterator<ALootPickup> It(GetWorld()); It && Pawn; ++It)
	{
		if (!It->IsHidden() && It->GetLoot().Type == ELootRewardType::Key)
		{
			Pawn->TeleportTo(It->GetActorLocation() + FVector(0.0f, 0.0f, 60.0f), Pawn->GetActorRotation());
			UE_LOG(LogZombieGame, Log, TEXT("[Cheat] Moved to the key at %s"), *It->GetActorLocation().ToString());
			return;
		}
	}
}

void UZombieCheatManager::ZombieSmokeTest(int32 TargetSector)
{
	SmokeTestTarget = FMath::Max(TargetSector, 2);
	SmokeTestTicks = 0;
	if (!bGodMode)
	{
		ZombieGod();
	}
	UE_LOG(LogZombieGame, Log, TEXT("[SmokeTest] Started: running the loop until sector %d."), SmokeTestTarget);
	GetWorld()->GetTimerManager().SetTimer(SmokeTestTimer, FTimerDelegate::CreateUObject(this, &UZombieCheatManager::SmokeTestStep), 1.0f, true);
}

void UZombieCheatManager::SmokeTestStep()
{
	const AZombieGameState* GameState = GetWorld()->GetGameState<AZombieGameState>();
	IZombieRunController* Run = Cast<IZombieRunController>(GetWorld()->GetAuthGameMode());
	APawn* Pawn = GetPlayerPawn();
	if (!GameState || !Run || !Pawn)
	{
		return;
	}

	++SmokeTestTicks;
	const AZombiePlayerState* PlayerState = GetOuterAPlayerController()->GetPlayerState<AZombiePlayerState>();
	UE_LOG(LogZombieGame, Log, TEXT("[SmokeTest] t=%d sector=%d phase=%d remaining=%d boss=%s money=%d kills=%d"),
		SmokeTestTicks, GameState->GetCurrentSector(), static_cast<int32>(GameState->GetSectorPhase()), GameState->GetRemainingEnemies(),
		GameState->GetActiveBoss() ? *GameState->GetActiveBoss()->GetName() : TEXT("none"),
		PlayerState ? PlayerState->GetMoney() : -1, PlayerState ? PlayerState->GetRunStats().Kills : -1);

	switch (GameState->GetSectorPhase())
	{
	case ESectorPhase::Combat:
		if (UInventoryComponent* Inventory = Pawn->FindComponentByClass<UInventoryComponent>())
		{
			if (AZombieWeapon* Weapon = Inventory->GetActiveWeapon())
			{
				Weapon->TryFire(Pawn->GetActorForwardVector());
			}
		}
		// Let a boss arena wake its boss, then clear the field every few seconds.
		Run->NotifyBossRoomEntered(Pawn);
		if (SmokeTestTicks % 3 == 0)
		{
			ZombieKillAll();
		}
		break;
	case ESectorPhase::KeyDropped:
		ZombieCollectKey();
		break;
	case ESectorPhase::ExitUnlocked:
		Run->NotifyExitUsed(Pawn);
		break;
	case ESectorPhase::Intermission:
		if (GameState->GetCurrentSector() + 1 > SmokeTestTarget)
		{
			SmokeTestFinish();
		}
		else
		{
			SmokeTestShop();
			Run->NotifyIntermissionLeft(Pawn);
		}
		break;
	default:
		break;
	}
}

void UZombieCheatManager::SmokeTestShop()
{
	const AZombiePlayerController* PC = Cast<AZombiePlayerController>(GetOuterAPlayerController());
	const AZombieGameState* GameState = GetWorld()->GetGameState<AZombieGameState>();
	const AZombieShopState* Shop = GameState ? GameState->GetShop() : nullptr;
	if (!PC || !Shop)
	{
		return;
	}

	ZombieGiveMoney(3000);
	UShopTransactionComponent* Transactions = PC->GetShopTransactions();
	UE_LOG(LogZombieGame, Log, TEXT("[SmokeTest] Shop has %d offers."), Shop->GetOffers().Num());
	for (const FShopOffer& Offer : Shop->GetOffers())
	{
		Transactions->BuyOffer(Offer.OfferId);
	}
	Transactions->BuyService(EShopService::Armor);
	Transactions->BuyService(EShopService::MysteryBox);
	Transactions->BuyService(EShopService::WeaponUpgrade, 0);
	Transactions->BuyService(EShopService::AmmoRefill, 0);

	const UInventoryComponent* Inventory = GetPlayerPawn()->FindComponentByClass<UInventoryComponent>();
	UE_LOG(LogZombieGame, Log, TEXT("[SmokeTest] After shopping: %d weapons, active upgraded=%d."), Inventory ? Inventory->GetWeaponCount() : 0,
		Inventory && Inventory->GetActiveWeapon() ? Inventory->GetActiveWeapon()->GetUpgradeLevel() : -1);
}

void UZombieCheatManager::SmokeTestFinish()
{
	GetWorld()->GetTimerManager().ClearTimer(SmokeTestTimer);
	UE_LOG(LogZombieGame, Log, TEXT("[SmokeTest] Target reached; ending the run by dying."));

	ZombieGod();
	if (UHealthComponent* Health = GetPlayerPawn()->FindComponentByClass<UHealthComponent>())
	{
		Health->ApplyDamage(100000.0f);
	}
}
