#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "ZombieUIManager.generated.h"

class APlayerController;
class UZombieCrosshairWidget;
class UZombieHUDWidget;
class UZombieMenuWidget;

/**
 * The UI Manager: owns every screen for one local player - the HUD and a stack of menus - and the
 * input mode that goes with them. Gameplay code never creates widgets; it asks the manager to show
 * a screen, and widgets themselves only observe gameplay state (ARCHITECTURE.md 10).
 */
UCLASS()
class ZOMBIEGAME_API UZombieUIManager : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	static UZombieUIManager* Get(const APlayerController* PlayerController);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	void ShowHUD();
	void HideHUD();
	UZombieHUDWidget* GetHUD() const { return HUD; }

	/** Opens a menu on top of the stack and gives it focus. */
	UZombieMenuWidget* PushMenu(TSubclassOf<UZombieMenuWidget> MenuClass);

	template <typename TMenu>
	TMenu* PushMenu()
	{
		return Cast<TMenu>(PushMenu(TMenu::StaticClass()));
	}

	/** Closes the given menu (and anything opened above it). */
	void PopMenu(UZombieMenuWidget* Menu);
	void PopAllMenus();

	bool HasOpenMenu() const { return MenuStack.Num() > 0; }
	UZombieMenuWidget* GetTopMenu() const { return MenuStack.Num() > 0 ? MenuStack.Last().Get() : nullptr; }

	template <typename TMenu>
	TMenu* FindMenu() const
	{
		for (const TObjectPtr<UZombieMenuWidget>& Menu : MenuStack)
		{
			if (TMenu* Typed = Cast<TMenu>(Menu))
			{
				return Typed;
			}
		}
		return nullptr;
	}

	/**
	 * Escape / Start: backs out of the top menu, or opens the pause menu during gameplay. Set
	 * bAllowPauseMenu false where there is nothing to pause (the main menu level).
	 */
	void HandleBackAction();
	void SetPauseMenuAllowed(bool bAllowed) { bAllowPauseMenu = bAllowed; }

	/** A short message on the HUD's notification feed. */
	void Notify(const FText& Message, const FLinearColor& Color);

	/** Global UI scale from the accessibility settings. */
	void ApplyInterfaceScale(float Scale) const;

private:
	void RefreshInputMode();
	void EnsureCrosshair();

	/**
	 * This subsystem outlives every level, but its widgets belong to the level's player controller.
	 * Without this, the main menu or death screen left on the stack by the previous level stayed
	 * "on top" in the next one - input blocked, crosshair cursor never shown.
	 */
	void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
	void ResetForNewWorld();

	FDelegateHandle WorldCleanupHandle;

	UPROPERTY(Transient)
	TObjectPtr<UZombieHUDWidget> HUD;

	UPROPERTY(Transient)
	TObjectPtr<UZombieCrosshairWidget> Crosshair;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UZombieMenuWidget>> MenuStack;

	bool bAllowPauseMenu = true;
	bool bPausedByMenu = false;
};
