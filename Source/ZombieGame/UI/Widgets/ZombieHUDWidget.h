#pragma once

#include "CoreMinimal.h"
#include "UI/ZombieWidgetBase.h"
#include "ZombieHUDWidget.generated.h"

class AZombiePlayerState;
class UCanvasPanelSlot;
class UDamageType;
class UImage;
class UTextBlock;
class UVerticalBox;
class UZombieObjectiveWidget;
class UZombieVitalsWidget;
class UZombieWeaponPanelWidget;

/** One line on the notification feed and when it should go. */
USTRUCT()
struct FHUDNotification
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UTextBlock> Text;

	double ExpiresAt = 0.0;
};

/**
 * The in-sector HUD: vitals, weapon panel, sector objective and boss bar, interaction prompt,
 * notification feed and a damage vignette. Composes the sub-widgets and hands each the gameplay
 * object it observes; re-targets them automatically when the pawn or player state changes.
 */
UCLASS()
class ZOMBIEGAME_API UZombieHUDWidget : public UZombieWidgetBase
{
	GENERATED_BODY()

public:
	void PushNotification(const FText& Message, const FLinearColor& Color);

protected:
	virtual void BuildWidget(UWidgetTree& Tree) override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void RetargetIfNeeded();
	void UpdatePrompt();
	void UpdateGamepadCrosshair();
	void UpdateNotifications();
	void UpdateVignette(float DeltaTime);
	void HandleDamageReceived(float Amount, AActor* Causer, const UDamageType* DamageType);

	UPROPERTY(Transient) TObjectPtr<UZombieVitalsWidget> Vitals;
	UPROPERTY(Transient) TObjectPtr<UZombieWeaponPanelWidget> WeaponPanel;
	UPROPERTY(Transient) TObjectPtr<UZombieObjectiveWidget> Objective;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PromptText;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> NotificationFeed;
	UPROPERTY(Transient) TObjectPtr<UImage> Vignette;
	UPROPERTY(Transient) TObjectPtr<UImage> PadCrosshair;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanelSlot> PadCrosshairSlot;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> FrameRateText;

	UPROPERTY(Transient)
	TArray<FHUDNotification> Notifications;

	TWeakObjectPtr<APawn> ObservedPawn;
	TWeakObjectPtr<AZombiePlayerState> ObservedPlayerState;
	TWeakObjectPtr<AActor> ObservedGameState;
	FDelegateHandle DamageHandle;
	float VignetteStrength = 0.0f;
};
