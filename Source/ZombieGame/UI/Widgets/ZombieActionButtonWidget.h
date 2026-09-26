#pragma once

#include "CoreMinimal.h"
#include "UI/ZombieWidgetBase.h"
#include "ZombieActionButtonWidget.generated.h"

class UButton;
class UTextBlock;

/**
 * A styled button whose click is a native delegate, so list screens (the shop) can bind each
 * button to its own offer or slot with a lambda rather than one UFUNCTION per possible row.
 */
UCLASS()
class ZOMBIEGAME_API UZombieActionButtonWidget : public UZombieWidgetBase
{
	GENERATED_BODY()

public:
	void Setup(const FText& InLabel, int32 InFontSize, bool bInEnabled, float InMinWidth = 0.0f);

	FSimpleDelegate OnPressed;

	UButton* GetButton() const { return Button; }

protected:
	virtual void BuildWidget(UWidgetTree& Tree) override;

private:
	UFUNCTION()
	void HandleClicked();

	UPROPERTY(Transient) TObjectPtr<UButton> Button;

	FText Label;
	int32 FontSize = 14;
	float MinWidth = 0.0f;
	bool bEnabled = true;
};
