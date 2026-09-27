#include "ZombieUIManager.h"
#include "Characters/Player/ZombiePlayerCharacter.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UI/Widgets/ZombieCrosshairWidget.h"
#include "UI/Widgets/ZombieHUDWidget.h"
#include "UI/Widgets/ZombiePauseMenuWidget.h"
#include "UI/ZombieWidgetBase.h"

namespace
{
	constexpr int32 HUDZOrder = 0;
	constexpr int32 MenuBaseZOrder = 10;
}

UZombieUIManager* UZombieUIManager::Get(const APlayerController* PlayerController)
{
	const ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	return LocalPlayer ? LocalPlayer->GetSubsystem<UZombieUIManager>() : nullptr;
}

void UZombieUIManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(this, &UZombieUIManager::HandleWorldCleanup);
}

void UZombieUIManager::Deinitialize()
{
	FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
	PopAllMenus();
	HideHUD();
	Super::Deinitialize();
}

void UZombieUIManager::HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
	if (World && World->IsGameWorld())
	{
		ResetForNewWorld();
	}
}

void UZombieUIManager::ResetForNewWorld()
{
	// No RefreshInputMode here: the player controller is being torn down with its world. The next
	// level's controller sets input up again through ShowHUD / PushMenu.
	for (UZombieMenuWidget* Menu : MenuStack)
	{
		if (Menu)
		{
			Menu->RemoveFromParent();
		}
	}
	MenuStack.Reset();

	HideHUD();

	// The software cursor is registered on the viewport, which survives the level: drop the old
	// crosshair so EnsureCrosshair builds one for the new controller.
	if (Crosshair)
	{
		if (UGameViewportClient* Viewport = GetLocalPlayer() ? GetLocalPlayer()->ViewportClient : nullptr)
		{
			Viewport->SetSoftwareCursorWidget(EMouseCursor::Crosshairs, nullptr);
		}
		Crosshair = nullptr;
	}

	// Per-level state: the death screen turns the pause menu off, and must not do so for the next run.
	bAllowPauseMenu = true;
	bPausedByMenu = false;
}

void UZombieUIManager::EnsureCrosshair()
{
	APlayerController* PC = GetLocalPlayer() ? GetLocalPlayer()->GetPlayerController(GetWorld()) : nullptr;
	UGameViewportClient* Viewport = GetLocalPlayer() ? GetLocalPlayer()->ViewportClient : nullptr;
	if (Crosshair || !PC || !Viewport)
	{
		return;
	}

	// The crosshair is the mouse cursor itself (a software cursor widget), so it tracks the mouse
	// with zero latency and needs no per-frame positioning.
	Crosshair = CreateWidget<UZombieCrosshairWidget>(PC, UZombieCrosshairWidget::StaticClass());
	Viewport->SetUseSoftwareCursorWidgets(true);
	Viewport->SetSoftwareCursorWidget(EMouseCursor::Crosshairs, Crosshair);
}

void UZombieUIManager::ShowHUD()
{
	APlayerController* PC = GetLocalPlayer() ? GetLocalPlayer()->GetPlayerController(GetWorld()) : nullptr;
	if (!PC)
	{
		return;
	}

	EnsureCrosshair();

	if (!HUD)
	{
		HUD = CreateWidget<UZombieHUDWidget>(PC, UZombieHUDWidget::StaticClass());
	}
	if (HUD && !HUD->IsInViewport())
	{
		HUD->AddToPlayerScreen(HUDZOrder);
	}
	RefreshInputMode();
}

void UZombieUIManager::HideHUD()
{
	if (HUD)
	{
		HUD->RemoveFromParent();
	}
	HUD = nullptr;
}

UZombieMenuWidget* UZombieUIManager::PushMenu(TSubclassOf<UZombieMenuWidget> MenuClass)
{
	APlayerController* PC = GetLocalPlayer() ? GetLocalPlayer()->GetPlayerController(GetWorld()) : nullptr;
	if (!PC || !MenuClass)
	{
		return nullptr;
	}

	UZombieMenuWidget* Menu = CreateWidget<UZombieMenuWidget>(PC, MenuClass);
	if (!Menu)
	{
		return nullptr;
	}

	Menu->AddToPlayerScreen(MenuBaseZOrder + MenuStack.Num());
	MenuStack.Add(Menu);
	RefreshInputMode();
	return Menu;
}

void UZombieUIManager::PopMenu(UZombieMenuWidget* Menu)
{
	const int32 Index = MenuStack.IndexOfByKey(Menu);
	if (Index == INDEX_NONE)
	{
		return;
	}

	for (int32 Remove = MenuStack.Num() - 1; Remove >= Index; --Remove)
	{
		if (MenuStack[Remove])
		{
			MenuStack[Remove]->RemoveFromParent();
		}
		MenuStack.RemoveAt(Remove);
	}
	RefreshInputMode();
}

void UZombieUIManager::PopAllMenus()
{
	for (UZombieMenuWidget* Menu : MenuStack)
	{
		if (Menu)
		{
			Menu->RemoveFromParent();
		}
	}
	MenuStack.Reset();
	RefreshInputMode();
}

void UZombieUIManager::HandleBackAction()
{
	if (UZombieMenuWidget* Top = GetTopMenu())
	{
		Top->HandleBackAction();
		return;
	}

	if (bAllowPauseMenu && HUD)
	{
		PushMenu<UZombiePauseMenuWidget>();
	}
}

void UZombieUIManager::Notify(const FText& Message, const FLinearColor& Color)
{
	if (HUD)
	{
		HUD->PushNotification(Message, Color);
	}
}

void UZombieUIManager::ApplyInterfaceScale(float Scale) const
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetApplicationScale(FMath::Clamp(Scale, 0.5f, 2.0f));
	}
}

void UZombieUIManager::RefreshInputMode()
{
	APlayerController* PC = GetLocalPlayer() ? GetLocalPlayer()->GetPlayerController(GetWorld()) : nullptr;
	if (!PC)
	{
		return;
	}

	AZombiePlayerCharacter* Character = Cast<AZombiePlayerCharacter>(PC->GetPawn());
	UZombieMenuWidget* Top = GetTopMenu();

	if (Top)
	{
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		if (UWidget* Focus = Top->GetInitialFocus())
		{
			Mode.SetWidgetToFocus(Focus->TakeWidget());
		}
		PC->SetInputMode(Mode);
		PC->CurrentMouseCursor = EMouseCursor::Default;
	}
	else
	{
		FInputModeGameOnly Mode;
		Mode.SetConsumeCaptureMouseDown(false);
		PC->SetInputMode(Mode);
		PC->CurrentMouseCursor = EMouseCursor::Crosshairs;
	}
	PC->SetShowMouseCursor(true);

	if (Character)
	{
		Character->SetGameplayInputBlocked(Top != nullptr);
	}

	const bool bShouldPause = MenuStack.ContainsByPredicate([](const TObjectPtr<UZombieMenuWidget>& Menu)
	{
		return Menu && Menu->PausesGame();
	});
	if (bShouldPause != bPausedByMenu)
	{
		bPausedByMenu = bShouldPause;
		UGameplayStatics::SetGamePaused(PC, bShouldPause);
	}
}
