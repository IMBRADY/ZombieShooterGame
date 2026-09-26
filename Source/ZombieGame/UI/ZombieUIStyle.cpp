#include "ZombieUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Fonts/CompositeFont.h"
#include "Misc/Paths.h"
#include "Engine/Texture2D.h"

namespace ZombieUI
{
	const FLinearColor Background(0.02f, 0.02f, 0.025f, 0.92f);
	const FLinearColor Panel(0.04f, 0.045f, 0.05f, 0.82f);
	const FLinearColor PanelLight(0.12f, 0.13f, 0.12f, 0.95f);
	const FLinearColor TextColor(0.88f, 0.9f, 0.82f, 1.0f);
	const FLinearColor TextDimColor(0.5f, 0.53f, 0.48f, 1.0f);
	const FLinearColor Accent(0.55f, 0.85f, 0.25f, 1.0f);
	const FLinearColor Danger(0.9f, 0.18f, 0.12f, 1.0f);
	const FLinearColor HealthColor(0.82f, 0.13f, 0.11f, 1.0f);
	const FLinearColor ArmorColor(0.3f, 0.55f, 0.95f, 1.0f);
	const FLinearColor StaminaColor(0.95f, 0.8f, 0.2f, 1.0f);
	const FLinearColor MoneyColor(1.0f, 0.82f, 0.25f, 1.0f);

	FSlateFontInfo Font(int32 Size)
	{
		// The pixel font ships as a plain TTF (Content/UI/Fonts, staged as a non-asset file) and is
		// loaded as a runtime composite font; the engine font stands in if it is ever missing.
		static TSharedPtr<const FCompositeFont> PixelFont;
		static bool bResolved = false;
		if (!bResolved)
		{
			bResolved = true;
			const FString FontPath = FPaths::ProjectContentDir() / TEXT("UI/Fonts/PressStart2P-Regular.ttf");
			if (FPaths::FileExists(FontPath))
			{
				PixelFont = MakeShared<FStandaloneCompositeFont>(NAME_None, FontPath, EFontHinting::None, EFontLoadingPolicy::LazyLoad);
			}
		}

		if (PixelFont.IsValid())
		{
			// Press Start 2P is wide; sizes are authored for a regular font, so it is drawn smaller.
			return FSlateFontInfo(PixelFont, FMath::Max(FMath::RoundToInt(Size * 0.72f), 6));
		}

		static TWeakObjectPtr<UObject> EngineFont;
		if (!EngineFont.IsValid())
		{
			EngineFont = FSoftObjectPath(TEXT("/Engine/EngineFonts/Roboto.Roboto")).TryLoad();
		}
		return FSlateFontInfo(EngineFont.Get(), Size);
	}

	UTextBlock* MakeText(UWidgetTree& Tree, const FText& InText, int32 Size, const FLinearColor& Color, bool bShadow)
	{
		UTextBlock* Block = Tree.ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Block->SetText(InText);
		Block->SetFont(Font(Size));
		Block->SetColorAndOpacity(FSlateColor(Color));
		if (bShadow)
		{
			Block->SetShadowOffset(FVector2D(2.0f, 2.0f));
			Block->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));
		}
		return Block;
	}

	UButton* MakeButton(UWidgetTree& Tree, const FText& Label, int32 FontSize, UTextBlock** OutLabel)
	{
		UButton* Button = Tree.ConstructWidget<UButton>(UButton::StaticClass());

		FButtonStyle Style = Button->GetStyle();
		const auto MakeBrush = [](const FLinearColor& Color)
		{
			FSlateBrush Brush;
			Brush.TintColor = FSlateColor(Color);
			Brush.DrawAs = ESlateBrushDrawType::Box;
			Brush.Margin = FMargin(0.0f);
			return Brush;
		};
		Style.SetNormal(MakeBrush(FLinearColor(0.08f, 0.09f, 0.08f, 0.95f)));
		Style.SetHovered(MakeBrush(FLinearColor(0.2f, 0.32f, 0.12f, 1.0f)));
		Style.SetPressed(MakeBrush(FLinearColor(0.35f, 0.55f, 0.18f, 1.0f)));
		Style.SetDisabled(MakeBrush(FLinearColor(0.05f, 0.05f, 0.05f, 0.7f)));
		Style.SetNormalPadding(FMargin(14.0f, 8.0f));
		Style.SetPressedPadding(FMargin(14.0f, 9.0f, 14.0f, 7.0f));
		Button->SetStyle(Style);

		UTextBlock* LabelBlock = MakeText(Tree, Label, FontSize, TextColor);
		LabelBlock->SetJustification(ETextJustify::Center);
		Button->AddChild(LabelBlock);

		if (OutLabel)
		{
			*OutLabel = LabelBlock;
		}
		return Button;
	}

	UProgressBar* MakeBar(UWidgetTree& Tree, const FLinearColor& Fill, float Height)
	{
		UProgressBar* Bar = Tree.ConstructWidget<UProgressBar>(UProgressBar::StaticClass());

		FProgressBarStyle Style = Bar->GetWidgetStyle();
		FSlateBrush BackgroundBrush;
		BackgroundBrush.TintColor = FSlateColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.7f));
		BackgroundBrush.DrawAs = ESlateBrushDrawType::Box;
		FSlateBrush FillBrush;
		FillBrush.TintColor = FSlateColor(FLinearColor::White);
		FillBrush.DrawAs = ESlateBrushDrawType::Box;
		Style.SetBackgroundImage(BackgroundBrush);
		Style.SetFillImage(FillBrush);
		Bar->SetWidgetStyle(Style);

		Bar->SetFillColorAndOpacity(Fill);
		Bar->SetPercent(1.0f);
		return Bar;
	}

	USizeBox* MakeSized(UWidgetTree& Tree, UWidget* Content, float Width, float Height)
	{
		USizeBox* Box = Tree.ConstructWidget<USizeBox>(USizeBox::StaticClass());
		if (Width > 0.0f)
		{
			Box->SetWidthOverride(Width);
		}
		if (Height > 0.0f)
		{
			Box->SetHeightOverride(Height);
		}
		if (Content)
		{
			Box->AddChild(Content);
		}
		return Box;
	}

	UBorder* MakePanel(UWidgetTree& Tree, UWidget* Content, const FMargin& Padding, const FLinearColor& Color)
	{
		UBorder* Border = Tree.ConstructWidget<UBorder>(UBorder::StaticClass());
		Border->SetBrushColor(Color);
		Border->SetPadding(Padding);
		if (Content)
		{
			Border->SetContent(Content);
		}
		return Border;
	}

	UImage* MakeImage(UWidgetTree& Tree, UTexture2D* Texture, const FVector2D& Size, const FLinearColor& Tint)
	{
		UImage* Image = Tree.ConstructWidget<UImage>(UImage::StaticClass());
		if (Texture)
		{
			Image->SetBrushFromTexture(Texture, false);
		}
		Image->SetDesiredSizeOverride(Size);
		Image->SetColorAndOpacity(Tint);
		return Image;
	}

	FText FormatTime(float Seconds)
	{
		const int32 Total = FMath::Max(FMath::FloorToInt(Seconds), 0);
		return FText::FromString(FString::Printf(TEXT("%d:%02d"), Total / 60, Total % 60));
	}
}
