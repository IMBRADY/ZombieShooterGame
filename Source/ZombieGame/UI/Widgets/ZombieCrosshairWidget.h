#pragma once

#include "CoreMinimal.h"
#include "UI/ZombieWidgetBase.h"
#include "ZombieCrosshairWidget.generated.h"

/**
 * The aiming reticle. Installed as the viewport's software cursor for the Crosshairs cursor type,
 * so during gameplay the mouse cursor *is* the crosshair.
 */
UCLASS()
class ZOMBIEGAME_API UZombieCrosshairWidget : public UZombieWidgetBase
{
	GENERATED_BODY()

public:
	static constexpr float Size = 40.0f;

protected:
	virtual void BuildWidget(UWidgetTree& Tree) override;
};
