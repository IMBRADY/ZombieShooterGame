#include "ZombieCrosshairWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "UI/ZombieUIStyle.h"

void UZombieCrosshairWidget::BuildWidget(UWidgetTree& Tree)
{
	UCanvasPanel* Canvas = Tree.ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	Tree.RootWidget = Canvas;

	UTexture2D* Texture = Cast<UTexture2D>(FSoftObjectPath(TEXT("/Game/UI/Textures/T_Crosshair.T_Crosshair")).TryLoad());
	UImage* Reticle = ZombieUI::MakeImage(Tree, Texture, FVector2D(Size, Size), ZombieUI::Accent);

	// The cursor hotspot is the widget's origin, so the reticle is centred on it.
	UCanvasPanelSlot* ChildSlot = Canvas->AddChildToCanvas(Reticle);
	ChildSlot->SetPosition(FVector2D(-Size * 0.5f, -Size * 0.5f));
	ChildSlot->SetSize(FVector2D(Size, Size));
}
