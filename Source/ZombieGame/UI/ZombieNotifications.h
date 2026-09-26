#pragma once

#include "CoreMinimal.h"

class AActor;

/**
 * Gameplay code's way to put a message in front of a player ("Perk: Speed Boost", "Exit unlocked")
 * without knowing anything about widgets: it routes to the owning player controller, which
 * forwards to its UI manager on whichever machine that player is on.
 */
namespace ZombieNotifications
{
	/** Context is the player's pawn or controller. */
	ZOMBIEGAME_API void Notify(AActor* Context, const FText& Message, const FLinearColor& Color = FLinearColor(0.88f, 0.9f, 0.82f));

	/** Sends to every player. */
	ZOMBIEGAME_API void NotifyAll(const UObject* WorldContext, const FText& Message, const FLinearColor& Color = FLinearColor(0.88f, 0.9f, 0.82f));
}
