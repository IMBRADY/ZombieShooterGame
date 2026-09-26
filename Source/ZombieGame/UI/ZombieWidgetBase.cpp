#include "ZombieWidgetBase.h"
#include "Audio/ZombieAudioSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "UI/ZombieUIManager.h"

TSharedRef<SWidget> UZombieWidgetBase::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildWidget(*WidgetTree);
	}
	return Super::RebuildWidget();
}

UCanvasPanelSlot* UZombieWidgetBase::PlaceOnCanvas(UCanvasPanel* Canvas, UWidget* Child, const FVector2D& Anchor,
	const FVector2D& Alignment, const FVector2D& Offset, bool bAutoSize)
{
	if (!Canvas || !Child)
	{
		return nullptr;
	}

	UCanvasPanelSlot* ChildSlot = Canvas->AddChildToCanvas(Child);
	ChildSlot->SetAnchors(FAnchors(Anchor.X, Anchor.Y));
	ChildSlot->SetAlignment(Alignment);
	ChildSlot->SetPosition(Offset);
	ChildSlot->SetAutoSize(bAutoSize);
	return ChildSlot;
}

void UZombieWidgetBase::PlayUISound(FName SoundName) const
{
	if (UZombieAudioSubsystem* Audio = UZombieAudioSubsystem::Get(this))
	{
		Audio->PlayNamedSound2D(SoundName);
	}
}

bool UZombieMenuWidget::HandleBackAction()
{
	CloseMenu();
	return true;
}

void UZombieMenuWidget::CloseMenu()
{
	if (UZombieUIManager* Manager = UZombieUIManager::Get(GetOwningPlayer()))
	{
		Manager->PopMenu(this);
	}
}

UCanvasPanel* UZombieMenuWidget::MakeBackdrop(UWidgetTree& Tree, UWidget* CentredContent, float Dim)
{
	UCanvasPanel* Canvas = Tree.ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	Tree.RootWidget = Canvas;

	UBorder* Shade = Tree.ConstructWidget<UBorder>(UBorder::StaticClass());
	Shade->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, Dim));
	UCanvasPanelSlot* ShadeSlot = Canvas->AddChildToCanvas(Shade);
	ShadeSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	ShadeSlot->SetOffsets(FMargin(0.0f));

	PlaceOnCanvas(Canvas, CentredContent, FVector2D(0.5f, 0.5f), FVector2D(0.5f, 0.5f), FVector2D::ZeroVector);
	return Canvas;
}
