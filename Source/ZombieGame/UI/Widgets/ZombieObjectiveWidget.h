#pragma once

#include "CoreMinimal.h"
#include "Core/ZombieGameState.h"
#include "UI/ZombieWidgetBase.h"
#include "ZombieObjectiveWidget.generated.h"

class UHealthComponent;
class UProgressBar;
class UTextBlock;
class UVerticalBox;

/** Sector number, the mini objective, and the boss health bar - top-centre of the HUD. */
UCLASS()
class ZOMBIEGAME_API UZombieObjectiveWidget : public UZombieWidgetBase
{
	GENERATED_BODY()

public:
	void Observe(AZombieGameState* GameState);

protected:
	virtual void BuildWidget(UWidgetTree& Tree) override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandleSectorChanged(int32 NewSector);

	UFUNCTION()
	void HandleBossHealthChanged(float NewHealth, float MaxHealth, float Delta);

	void HandlePhaseChanged(ESectorPhase NewPhase);
	void HandleBossChanged(AActor* Boss);
	void RefreshObjective();
	void Unbind();

	UPROPERTY(Transient) TObjectPtr<UTextBlock> SectorText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ObjectiveText;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> BossPanel;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> BossName;
	UPROPERTY(Transient) TObjectPtr<UProgressBar> BossBar;

	TWeakObjectPtr<AZombieGameState> BoundGameState;
	TWeakObjectPtr<UHealthComponent> BoundBossHealth;
	FDelegateHandle PhaseHandle;
	FDelegateHandle BossHandle;
	FDelegateHandle EncounterHandle;
};
