#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "ZombieCheatManager.generated.h"

/**
 * Development console commands (never in shipping builds - Unreal strips the cheat manager).
 *
 * Besides the usual god-mode and money cheats, ZombieSmokeTest drives a whole run unattended -
 * clear sector, take the key, exit, shop, next sector, through a boss fight, then die - logging
 * each step, so the full game loop can be verified headlessly:
 *
 *     UnrealEditor.exe ZombieGame.uproject /Game/Maps/L_TestSector -game -nullrhi -ExecCmds="ZombieSmokeTest 6"
 */
UCLASS()
class ZOMBIEGAME_API UZombieCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	UFUNCTION(Exec) void ZombieGod();
	UFUNCTION(Exec) void ZombieKillAll();
	UFUNCTION(Exec) void ZombieGiveMoney(int32 Amount);
	UFUNCTION(Exec) void ZombieCollectKey();
	UFUNCTION(Exec) void ZombieSmokeTest(int32 TargetSector);

private:
	void SmokeTestStep();
	void SmokeTestShop();
	void SmokeTestFinish();
	void OnSmokeTestTransaction(bool bSuccess, const FText& Message);
	APawn* GetPlayerPawn() const;

	FTimerHandle SmokeTestTimer;
	int32 SmokeTestTarget = 0;
	int32 SmokeTestTicks = 0;
	bool bGodMode = false;
};
