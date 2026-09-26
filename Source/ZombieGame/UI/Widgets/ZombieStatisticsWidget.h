#pragma once

#include "CoreMinimal.h"
#include "UI/ZombieWidgetBase.h"
#include "ZombieStatisticsWidget.generated.h"

class UButton;
class UVerticalBox;

/** Lifetime statistics, best runs and achievements, from the meta save. */
UCLASS()
class ZOMBIEGAME_API UZombieStatisticsWidget : public UZombieMenuWidget
{
	GENERATED_BODY()

public:
	virtual UWidget* GetInitialFocus() const override;

protected:
	virtual void BuildWidget(UWidgetTree& Tree) override;

private:
	UFUNCTION() void HandleBack();

	void BuildStatistics(UWidgetTree& Tree, UVerticalBox& Column) const;
	void BuildHighScores(UWidgetTree& Tree, UVerticalBox& Column) const;
	void BuildAchievements(UWidgetTree& Tree, UVerticalBox& Column) const;
	void AddLine(UWidgetTree& Tree, UVerticalBox& Column, const FText& Label, const FText& Value, const FLinearColor& Color) const;

	UPROPERTY(Transient) TObjectPtr<UButton> BackButton;
};
