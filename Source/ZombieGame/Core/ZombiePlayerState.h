#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ZombiePlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMoneyChanged, int32, NewMoney);

UCLASS()
class ZOMBIEGAME_API AZombiePlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	int32 GetMoney() const { return Money; }
	int32 GetKills() const { return Kills; }
	int32 GetBossesDefeated() const { return BossesDefeated; }
	float GetDamageTaken() const { return DamageTaken; }

	void AddMoney(int32 Amount);
	bool SpendMoney(int32 Amount);
	void AddKill();
	void AddBossDefeated();
	void AddDamageTaken(float Amount);

	UPROPERTY(BlueprintAssignable)
	FOnMoneyChanged OnMoneyChanged;

protected:
	UPROPERTY(Replicated)
	int32 Money = 0;

	UPROPERTY(Replicated)
	int32 Kills = 0;

	UPROPERTY(Replicated)
	int32 BossesDefeated = 0;

	UPROPERTY(Replicated)
	float DamageTaken = 0.0f;
};
