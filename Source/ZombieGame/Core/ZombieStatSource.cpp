#include "ZombieStatSource.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

namespace ZombieStats
{
	namespace
	{
		const APawn* ResolveOwningPawn(const AActor* Actor)
		{
			// Weapons are owned by the pawn carrying them and projectiles by the weapon that fired
			// them, so walking the owner chain reaches the pawn from any of the three.
			for (const AActor* Current = Actor; Current; Current = Current->GetOwner())
			{
				if (const APawn* Pawn = Cast<APawn>(Current))
				{
					return Pawn;
				}
			}
			return nullptr;
		}
	}

	IZombieStatSource* FindStatSource(const AActor* Actor)
	{
		const APawn* Pawn = ResolveOwningPawn(Actor);
		const APlayerState* PlayerState = Pawn ? Pawn->GetPlayerState() : nullptr;
		if (!PlayerState)
		{
			return nullptr;
		}

		for (UActorComponent* Component : PlayerState->GetComponents())
		{
			if (IZombieStatSource* Source = Cast<IZombieStatSource>(Component))
			{
				return Source;
			}
		}

		return nullptr;
	}

	float Resolve(const AActor* Actor, const FGameplayTag& Stat, float BaseValue)
	{
		const IZombieStatSource* Source = FindStatSource(Actor);
		return Source ? Source->GetStatTotals(Stat).Apply(BaseValue) : BaseValue;
	}
}
