#pragma once

#include "CoreMinimal.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/SlateColor.h"

class UBorder;
class UButton;
class UImage;
class UProgressBar;
class USizeBox;
class UTextBlock;
class UTexture2D;
class UWidget;
class UWidgetTree;

/**
 * The game's visual language for UI - palette, pixel font, and factory helpers that build styled
 * widgets inside a widget tree. Every screen is assembled from these, so the retro look is
 * defined once and a restyle is one file.
 *
 * Widgets are built in C++ (UMG, no Blueprint graphs) because the screens are simple observers of
 * gameplay state; there is no designer-only layout that would justify binary widget assets.
 */
namespace ZombieUI
{
	// --- Palette ---
	ZOMBIEGAME_API extern const FLinearColor Background;
	ZOMBIEGAME_API extern const FLinearColor Panel;
	ZOMBIEGAME_API extern const FLinearColor PanelLight;
	ZOMBIEGAME_API extern const FLinearColor TextColor;
	ZOMBIEGAME_API extern const FLinearColor TextDimColor;
	ZOMBIEGAME_API extern const FLinearColor Accent;
	ZOMBIEGAME_API extern const FLinearColor Danger;
	ZOMBIEGAME_API extern const FLinearColor HealthColor;
	ZOMBIEGAME_API extern const FLinearColor ArmorColor;
	ZOMBIEGAME_API extern const FLinearColor StaminaColor;
	ZOMBIEGAME_API extern const FLinearColor MoneyColor;

	/** The pixel font at a size, falling back to the engine font if the asset is missing. */
	ZOMBIEGAME_API FSlateFontInfo Font(int32 Size);

	ZOMBIEGAME_API UTextBlock* MakeText(UWidgetTree& Tree, const FText& Text, int32 Size, const FLinearColor& Color = TextColor, bool bShadow = true);

	/** A button with a centred label. Label is returned through OutLabel for later updates. */
	ZOMBIEGAME_API UButton* MakeButton(UWidgetTree& Tree, const FText& Label, int32 FontSize = 16, UTextBlock** OutLabel = nullptr);

	ZOMBIEGAME_API UProgressBar* MakeBar(UWidgetTree& Tree, const FLinearColor& Fill, float Height = 14.0f);

	/** Wraps Content in a size box; a non-positive dimension is left to the content. */
	ZOMBIEGAME_API USizeBox* MakeSized(UWidgetTree& Tree, UWidget* Content, float Width, float Height);

	/** A dark translucent panel with padding around a single child. */
	ZOMBIEGAME_API UBorder* MakePanel(UWidgetTree& Tree, UWidget* Content, const FMargin& Padding = FMargin(12.0f), const FLinearColor& Color = Panel);

	ZOMBIEGAME_API UImage* MakeImage(UWidgetTree& Tree, UTexture2D* Texture, const FVector2D& Size, const FLinearColor& Tint = FLinearColor::White);

	/** Formats seconds as m:ss. */
	ZOMBIEGAME_API FText FormatTime(float Seconds);
}
