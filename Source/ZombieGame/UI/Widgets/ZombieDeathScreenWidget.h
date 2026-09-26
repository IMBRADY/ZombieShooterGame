#pragma once

#include "CoreMinimal.h"
#include "Core/Save/ZombieRunTypes.h"
#include "UI/ZombieWidgetBase.h"
#include "ZombieDeathScreenWidget.generated.h"

class UButton;
class UVerticalBox;

/**
 * The run summary shown on death: sector reached, money earned, kills, bosses defeated, damage
 * taken, favourite weapon and total time - then back to the main menu on a click.
 */
UCLASS()
class ZOMBIEGAME_API UZombieDeathScreenWidget : public UZombieMenuWidget
{
	GENERATED_BODY()

public:
	void ShowSummary(const FZombieRunStats& Stats, int32 SectorReached);

	/** There is nothing to go back to - only the button leaves this screen. */
	virtual bool HandleBackAction() override { return true; }
	virtual UWidget* GetInitialFocus() const override;

protected:
	virtual void BuildWidget(UWidgetTree& Tree) override;

private:
	UFUNCTION() void HandleReturnToMenu();

	void AddStat(const FText& Label, const FText& Value);

	UPROPERTY(Transient) TObjectPtr<UVerticalBox> StatList;
	UPROPERTY(Transient) TObjectPtr<UButton> MenuButton;
};
