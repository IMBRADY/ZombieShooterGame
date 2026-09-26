#include "ZombieHUDWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Characters/Player/ZombiePlayerCharacter.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/DamageComponent.h"
#include "Components/HealthComponent.h"
#include "Components/Image.h"
#include "Components/InteractionComponent.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Core/ZombieGameInstance.h"
#include "Core/ZombieGameState.h"
#include "Core/ZombiePlayerState.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "UI/Widgets/ZombieCrosshairWidget.h"
#include "UI/Widgets/ZombieObjectiveWidget.h"
#include "UI/Widgets/ZombieVitalsWidget.h"
#include "UI/Widgets/ZombieWeaponPanelWidget.h"
#include "UI/ZombieUIStyle.h"

#define LOCTEXT_NAMESPACE "ZombieHUD"

namespace
{
	constexpr float NotificationLifetime = 3.5f;
	constexpr int32 MaxNotifications = 6;
	constexpr float VignetteDecayPerSecond = 1.8f;
}

void UZombieHUDWidget::BuildWidget(UWidgetTree& Tree)
{
	UCanvasPanel* Canvas = Tree.ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	Tree.RootWidget = Canvas;

	// Damage vignette underneath everything else, stretched over the whole screen.
	Vignette = ZombieUI::MakeImage(Tree, Cast<UTexture2D>(FSoftObjectPath(TEXT("/Game/UI/Textures/T_Vignette.T_Vignette")).TryLoad()),
		FVector2D(64.0f, 64.0f), FLinearColor(0.8f, 0.0f, 0.0f, 0.0f));
	Vignette->SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanelSlot* VignetteSlot = Canvas->AddChildToCanvas(Vignette);
	VignetteSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	VignetteSlot->SetOffsets(FMargin(0.0f));

	Vitals = Tree.ConstructWidget<UZombieVitalsWidget>(UZombieVitalsWidget::StaticClass());
	PlaceOnCanvas(Canvas, Vitals, FVector2D(0.0f, 0.0f), FVector2D(0.0f, 0.0f), FVector2D(24.0f, 24.0f));

	Objective = Tree.ConstructWidget<UZombieObjectiveWidget>(UZombieObjectiveWidget::StaticClass());
	PlaceOnCanvas(Canvas, Objective, FVector2D(0.5f, 0.0f), FVector2D(0.5f, 0.0f), FVector2D(0.0f, 18.0f));

	WeaponPanel = Tree.ConstructWidget<UZombieWeaponPanelWidget>(UZombieWeaponPanelWidget::StaticClass());
	PlaceOnCanvas(Canvas, WeaponPanel, FVector2D(1.0f, 1.0f), FVector2D(1.0f, 1.0f), FVector2D(-24.0f, -24.0f));

	PromptText = ZombieUI::MakeText(Tree, FText::GetEmpty(), 16, ZombieUI::Accent);
	PlaceOnCanvas(Canvas, PromptText, FVector2D(0.5f, 1.0f), FVector2D(0.5f, 1.0f), FVector2D(0.0f, -150.0f));

	NotificationFeed = Tree.ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	PlaceOnCanvas(Canvas, NotificationFeed, FVector2D(1.0f, 0.0f), FVector2D(1.0f, 0.0f), FVector2D(-24.0f, 24.0f));

	FrameRateText = ZombieUI::MakeText(Tree, FText::GetEmpty(), 10, ZombieUI::TextDimColor);
	PlaceOnCanvas(Canvas, FrameRateText, FVector2D(0.0f, 1.0f), FVector2D(0.0f, 1.0f), FVector2D(12.0f, -8.0f));

	// With a gamepad there is no mouse cursor, so the HUD draws the crosshair at the aim point.
	PadCrosshair = ZombieUI::MakeImage(Tree, Cast<UTexture2D>(FSoftObjectPath(TEXT("/Game/UI/Textures/T_Crosshair.T_Crosshair")).TryLoad()),
		FVector2D(UZombieCrosshairWidget::Size, UZombieCrosshairWidget::Size), ZombieUI::Accent);
	PadCrosshair->SetVisibility(ESlateVisibility::Collapsed);
	PadCrosshairSlot = PlaceOnCanvas(Canvas, PadCrosshair, FVector2D(0.0f, 0.0f), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);
}

void UZombieHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	RetargetIfNeeded();
	UpdatePrompt();
	UpdateGamepadCrosshair();
	UpdateNotifications();
	UpdateVignette(InDeltaTime);

	const UZombieGameInstance* GameInstance = Cast<UZombieGameInstance>(GetGameInstance());
	const bool bShowFps = GameInstance && GameInstance->GetUserSettings().bShowFrameRate;
	FrameRateText->SetText(bShowFps && InDeltaTime > 0.0f
		? FText::Format(LOCTEXT("Fps", "{0} FPS"), FText::AsNumber(FMath::RoundToInt(1.0f / InDeltaTime)))
		: FText::GetEmpty());
}

void UZombieHUDWidget::RetargetIfNeeded()
{
	APlayerController* PC = GetOwningPlayer();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	AZombiePlayerState* PlayerState = PC ? PC->GetPlayerState<AZombiePlayerState>() : nullptr;
	AZombieGameState* GameState = GetWorld() ? GetWorld()->GetGameState<AZombieGameState>() : nullptr;

	if (Pawn != ObservedPawn.Get() || PlayerState != ObservedPlayerState.Get())
	{
		if (APawn* OldPawn = ObservedPawn.Get())
		{
			if (UDamageComponent* OldDamage = OldPawn->FindComponentByClass<UDamageComponent>())
			{
				OldDamage->OnDamageReceived.Remove(DamageHandle);
			}
		}

		ObservedPawn = Pawn;
		ObservedPlayerState = PlayerState;
		Vitals->Observe(Pawn, PlayerState);
		WeaponPanel->Observe(Pawn);

		if (UDamageComponent* Damage = Pawn ? Pawn->FindComponentByClass<UDamageComponent>() : nullptr)
		{
			DamageHandle = Damage->OnDamageReceived.AddUObject(this, &UZombieHUDWidget::HandleDamageReceived);
		}
	}

	if (GameState != ObservedGameState.Get())
	{
		ObservedGameState = GameState;
		Objective->Observe(GameState);
	}
}

void UZombieHUDWidget::UpdatePrompt()
{
	const APawn* Pawn = ObservedPawn.Get();
	const UInteractionComponent* Interaction = Pawn ? Pawn->FindComponentByClass<UInteractionComponent>() : nullptr;
	const FText Prompt = Interaction ? Interaction->GetFocusedPrompt() : FText::GetEmpty();

	const AZombiePlayerCharacter* Character = Cast<AZombiePlayerCharacter>(Pawn);
	const FText Key = (Character && Character->IsUsingGamepadAim()) ? LOCTEXT("PadKey", "[A]") : LOCTEXT("Key", "[E]");
	PromptText->SetText(Prompt.IsEmpty() ? FText::GetEmpty() : FText::Format(LOCTEXT("Prompt", "{0} {1}"), Key, Prompt));
}

void UZombieHUDWidget::UpdateGamepadCrosshair()
{
	const AZombiePlayerCharacter* Character = Cast<AZombiePlayerCharacter>(ObservedPawn.Get());
	APlayerController* PC = GetOwningPlayer();
	const bool bPad = Character && Character->IsUsingGamepadAim() && !Character->IsDead();

	PadCrosshair->SetVisibility(bPad ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (PC)
	{
		PC->SetShowMouseCursor(!bPad);
	}

	FVector2D ScreenPosition;
	if (bPad && PC && PC->ProjectWorldLocationToScreen(Character->GetAimPoint(), ScreenPosition))
	{
		const float Scale = UWidgetLayoutLibrary::GetViewportScale(this);
		PadCrosshairSlot->SetPosition(ScreenPosition / FMath::Max(Scale, 0.01f));
	}
}

void UZombieHUDWidget::PushNotification(const FText& Message, const FLinearColor& Color)
{
	if (!WidgetTree || !NotificationFeed || Message.IsEmpty())
	{
		return;
	}

	UTextBlock* Text = ZombieUI::MakeText(*WidgetTree, Message, 14, Color);
	Text->SetJustification(ETextJustify::Right);
	NotificationFeed->AddChildToVerticalBox(Text)->SetHorizontalAlignment(HAlign_Right);

	FHUDNotification& Entry = Notifications.AddDefaulted_GetRef();
	Entry.Text = Text;
	Entry.ExpiresAt = FPlatformTime::Seconds() + NotificationLifetime;

	while (Notifications.Num() > MaxNotifications)
	{
		Notifications[0].Text->RemoveFromParent();
		Notifications.RemoveAt(0);
	}
}

void UZombieHUDWidget::UpdateNotifications()
{
	// Real time, not game time: the feed keeps fading while the game is paused behind a menu.
	const double Now = FPlatformTime::Seconds();
	for (int32 Index = Notifications.Num() - 1; Index >= 0; --Index)
	{
		const double Remaining = Notifications[Index].ExpiresAt - Now;
		if (Remaining <= 0.0)
		{
			Notifications[Index].Text->RemoveFromParent();
			Notifications.RemoveAt(Index);
		}
		else if (Remaining < 0.6)
		{
			Notifications[Index].Text->SetRenderOpacity(static_cast<float>(Remaining / 0.6));
		}
	}
}

void UZombieHUDWidget::HandleDamageReceived(float Amount, AActor* Causer, const UDamageType* DamageType)
{
	const UZombieGameInstance* GameInstance = Cast<UZombieGameInstance>(GetGameInstance());
	if (!GameInstance || GameInstance->GetUserSettings().bDamageFlash)
	{
		VignetteStrength = FMath::Clamp(VignetteStrength + Amount / 40.0f, 0.25f, 0.85f);
	}
}

void UZombieHUDWidget::UpdateVignette(float DeltaTime)
{
	// Hurt flashes decay; low health keeps a faint, steady red edge as a warning.
	const APawn* Pawn = ObservedPawn.Get();
	const UHealthComponent* Health = Pawn ? Pawn->FindComponentByClass<UHealthComponent>() : nullptr;
	const float LowHealthFloor = (Health && Health->GetHealthFraction() < 0.3f && !Health->IsDead()) ? 0.3f : 0.0f;

	VignetteStrength = FMath::Max(VignetteStrength - VignetteDecayPerSecond * DeltaTime, 0.0f);
	Vignette->SetColorAndOpacity(FLinearColor(0.75f, 0.0f, 0.0f, FMath::Max(VignetteStrength, LowHealthFloor)));
}

#undef LOCTEXT_NAMESPACE
