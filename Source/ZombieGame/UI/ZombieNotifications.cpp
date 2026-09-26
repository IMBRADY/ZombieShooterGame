#include "ZombieNotifications.h"
#include "Characters/Player/ZombiePlayerController.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

namespace ZombieNotifications
{
	void Notify(AActor* Context, const FText& Message, const FLinearColor& Color)
	{
		const APawn* Pawn = Cast<APawn>(Context);
		AZombiePlayerController* Controller = Pawn
			? Cast<AZombiePlayerController>(Pawn->GetController())
			: Cast<AZombiePlayerController>(Context);

		if (Controller && !Message.IsEmpty())
		{
			Controller->ClientNotify(Message, Color);
		}
	}

	void NotifyAll(const UObject* WorldContext, const FText& Message, const FLinearColor& Color)
	{
		const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
		if (!World)
		{
			return;
		}

		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			Notify(It->Get(), Message, Color);
		}
	}
}
