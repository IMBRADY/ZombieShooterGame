#include "ZombieActionButtonWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "UI/ZombieUIStyle.h"

void UZombieActionButtonWidget::Setup(const FText& InLabel, int32 InFontSize, bool bInEnabled, float InMinWidth)
{
	Label = InLabel;
	FontSize = InFontSize;
	bEnabled = bInEnabled;
	MinWidth = InMinWidth;
}

void UZombieActionButtonWidget::BuildWidget(UWidgetTree& Tree)
{
	Button = ZombieUI::MakeButton(Tree, Label, FontSize);
	Button->SetIsEnabled(bEnabled);
	Button->OnClicked.AddDynamic(this, &UZombieActionButtonWidget::HandleClicked);
	Tree.RootWidget = MinWidth > 0.0f ? static_cast<UWidget*>(ZombieUI::MakeSized(Tree, Button, MinWidth, -1.0f)) : Button;
}

void UZombieActionButtonWidget::HandleClicked()
{
	OnPressed.ExecuteIfBound();
}
