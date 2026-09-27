#include "ZombieCheatManager.h"
#include "Characters/Player/ZombiePlayerController.h"
#include "Characters/Zombies/ZombieArchetypeDataAsset.h"
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
#include "Utilities/ZombiePrimaryAssetLoader.h"
#include "Weapons/WeaponDataAsset.h"
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
	ZombieMeleeTest();
	GetWorld()->GetTimerManager().SetTimer(SmokeTestTimer, FTimerDelegate::CreateUObject(this, &UZombieCheatManager::SmokeTestStep), 1.0f, true);
}

AZombieCharacter* UZombieCheatManager::SpawnMeleeTestZombie(EZombieClassTier Tier) const
{
	APawn* Pawn = GetPlayerPawn();
	UZombieEnemyManager* Enemies = UZombieEnemyManager::Get(GetWorld());
	if (!Pawn || !Enemies)
	{
		return nullptr;
	}

	TArray<UZombieArchetypeDataAsset*> Archetypes;
	FZombiePrimaryAssetLoader::LoadAllOfType(UZombieArchetypeDataAsset::AssetType, Archetypes);
	Archetypes.Sort([](const UZombieArchetypeDataAsset& Lhs, const UZombieArchetypeDataAsset& Rhs) { return Lhs.GetName() < Rhs.GetName(); });
	// Only plain zombies: one with abilities (an exploder detonating on contact) could die by its
	// own hand before the second stab lands.
	UZombieArchetypeDataAsset* const* Match = Archetypes.FindByPredicate([Tier](const UZombieArchetypeDataAsset* Archetype)
	{
		return Archetype && Archetype->Tier == Tier && Archetype->Abilities.Num() == 0;
	});
	if (!Match)
	{
		return nullptr;
	}

	FZombieSpawnOptions Options;
	Options.bGrantsRewards = false;
	return Enemies->SpawnZombie(*Match, Pawn->GetActorLocation() + Pawn->GetActorForwardVector() * 110.0f, Options);
}

bool UZombieCheatManager::SwingMelee() const
{
	APawn* Pawn = GetPlayerPawn();
	UInventoryComponent* Inventory = Pawn ? Pawn->FindComponentByClass<UInventoryComponent>() : nullptr;
	if (!Inventory || !Inventory->GetMeleeWeapon())
	{
		return false;
	}
	Inventory->EquipMelee();
	return Inventory->GetActiveWeapon() == Inventory->GetMeleeWeapon()
		&& Inventory->GetMeleeWeapon()->TryFire(Pawn->GetActorForwardVector()) == EWeaponFireResult::Fired;
}

void UZombieCheatManager::ZombieMeleeTest()
{
	APawn* Pawn = GetPlayerPawn();
	UInventoryComponent* Inventory = Pawn ? Pawn->FindComponentByClass<UInventoryComponent>() : nullptr;
	const AZombieWeapon* Melee = Inventory ? Inventory->GetMeleeWeapon() : nullptr;
	if (!Melee || !Melee->GetDefinition())
	{
		UE_LOG(LogZombieGame, Error, TEXT("[MeleeTest] FAIL: the player has no melee weapon."));
		return;
	}
	UE_LOG(LogZombieGame, Log, TEXT("[MeleeTest] Melee slot holds '%s' (uses ammo: %s); %d gun(s) in %d slot(s)."),
		*Melee->GetDefinition()->DisplayName.ToString(), Melee->GetDefinition()->bUsesAmmo ? TEXT("yes") : TEXT("no"),
		Inventory->GetWeaponCount(), Inventory->GetSlotCount());

	// A shambler at full, sector-scaled health must die to a single stab.
	AZombieCharacter* Shambler = SpawnMeleeTestZombie(EZombieClassTier::Low);
	const bool bSwung = SwingMelee();
	const bool bKilled = Shambler && Shambler->IsDead();
	UE_LOG(LogZombieGame, Log, TEXT("[MeleeTest] %s: one stab on a shambler (swung=%d, dead=%d)."),
		bSwung && bKilled ? TEXT("PASS") : TEXT("FAIL"), bSwung, bKilled);

	// Anything tougher takes more than one - the second check waits out the swing cooldown.
	MeleeTestTarget = SpawnMeleeTestZombie(EZombieClassTier::Medium);
	FTimerHandle Handle;
	GetWorld()->GetTimerManager().SetTimer(Handle, FTimerDelegate::CreateUObject(this, &UZombieCheatManager::MeleeTestSecondStab), 0.8f, false);
}

void UZombieCheatManager::MeleeTestSecondStab()
{
	AZombieCharacter* Target = MeleeTestTarget.Get();
	const UHealthComponent* Health = Target ? Target->FindComponentByClass<UHealthComponent>() : nullptr;
	const bool bSwung = SwingMelee();
	const bool bPass = bSwung && Health && !Health->IsDead() && Health->GetHealth() < Health->GetMaxHealth();
	UE_LOG(LogZombieGame, Log, TEXT("[MeleeTest] %s: one stab on a tougher zombie wounds it (swung=%d, health %.0f / %.0f)."),
		bPass ? TEXT("PASS") : TEXT("FAIL"), bSwung, Health ? Health->GetHealth() : -1.0f, Health ? Health->GetMaxHealth() : -1.0f);

	// Back to the gun for the rest of the run.
	if (APawn* Pawn = GetPlayerPawn())
	{
		if (UInventoryComponent* Inventory = Pawn->FindComponentByClass<UInventoryComponent>())
		{
			Inventory->EquipSlot(0);
			UE_LOG(LogZombieGame, Log, TEXT("[MeleeTest] Switched back to slot 1: melee active=%d."), Inventory->IsMeleeActive());
		}
	}
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
	if (!Transactions->OnTransactionResult.IsBoundToObject(this))
	{
		Transactions->OnTransactionResult.AddUObject(this, &UZombieCheatManager::OnSmokeTestTransaction);
	}
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
	UE_LOG(LogZombieGame, Log, TEXT("[SmokeTest] After shopping: %d weapons, slot 0 upgrade level=%d."), Inventory ? Inventory->GetWeaponCount() : 0,
		Inventory && Inventory->GetWeaponAt(0) ? Inventory->GetWeaponAt(0)->GetUpgradeLevel() : -1);
}

void UZombieCheatManager::OnSmokeTestTransaction(bool bSuccess, const FText& Message)
{
	UE_LOG(LogZombieGame, Log, TEXT("[SmokeTest] Shop %s: %s"), bSuccess ? TEXT("ok") : TEXT("denied"), *Message.ToString());
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
