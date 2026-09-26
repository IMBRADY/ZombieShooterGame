#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ZombieWidgetBase.generated.h"

class UCanvasPanel;
class UCanvasPanelSlot;
class UWidgetTree;

/**
 * Base for every code-built widget: builds the widget tree once, on first construction, through
 * BuildWidget. Subclasses only describe layout and bind to the gameplay state they observe.
 */
UCLASS(Abstract)
class ZOMBIEGAME_API UZombieWidgetBase : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	/** Create the widget hierarchy. Called once, with the root still empty. */
	virtual void BuildWidget(UWidgetTree& Tree) PURE_VIRTUAL(UZombieWidgetBase::BuildWidget, );

	/** Adds Child to Canvas, anchored and aligned as given, at Offset from the anchor. */
	static UCanvasPanelSlot* PlaceOnCanvas(UCanvasPanel* Canvas, UWidget* Child, const FVector2D& Anchor,
		const FVector2D& Alignment, const FVector2D& Offset, bool bAutoSize = true);

	/** Plays a shared UI sound (UI.Click, UI.Buy, UI.Denied ...). */
	void PlayUISound(FName SoundName) const;
};

/**
 * A screen that takes over input - pause, settings, shop, death, main menu. The UI manager stacks
 * these: while any is open the cursor shows and the pawn stops taking gameplay input.
 */
UCLASS(Abstract)
class ZOMBIEGAME_API UZombieMenuWidget : public UZombieWidgetBase
{
	GENERATED_BODY()

public:
	/** Escape / gamepad B. Return true if handled; the default closes this menu. */
	virtual bool HandleBackAction();

	/** Whether the game world pauses while this menu is on top. */
	virtual bool PausesGame() const { return false; }

	/** Widget to focus for keyboard/gamepad navigation when the menu opens. */
	virtual UWidget* GetInitialFocus() const { return nullptr; }

	/** Closes this menu through the UI manager. */
	void CloseMenu();

protected:
	/** A full-screen dimmed backdrop with the given content centred on it. */
	UCanvasPanel* MakeBackdrop(UWidgetTree& Tree, UWidget* CentredContent, float Dim = 0.65f);
};
