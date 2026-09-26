#include "ZombieTeams.h"
#include "Characters/Zombies/ZombieCharacter.h"
#include "GameFramework/Pawn.h"

namespace ZombieTeams
{
	ETeam GetTeam(const AActor* Actor)
	{
		for (const AActor* Current = Actor; Current; Current = Current->GetOwner())
		{
			if (Current->IsA<AZombieCharacter>())
			{
				return ETeam::Zombies;
			}

			if (const APawn* Pawn = Cast<APawn>(Current))
			{
				return Pawn->IsPlayerControlled() || Pawn->GetPlayerState() ? ETeam::Players : ETeam::None;
			}
		}

		// Projectiles and hazards fired by a pawn record it as their instigator.
		if (Actor)
		{
			if (const APawn* InstigatorPawn = Actor->GetInstigator())
			{
				if (InstigatorPawn != Actor)
				{
					return GetTeam(InstigatorPawn);
				}
			}
		}

		return ETeam::None;
	}

	bool AreEnemies(const AActor* A, const AActor* B)
	{
		const ETeam TeamA = GetTeam(A);
		const ETeam TeamB = GetTeam(B);
		return TeamA != ETeam::None && TeamB != ETeam::None && TeamA != TeamB;
	}

	bool CanDamage(const AActor* Source, const AActor* Target, bool bFriendlyFire)
	{
		if (!Target)
		{
			return false;
		}

		if (bFriendlyFire)
		{
			return true;
		}

		// Environmental damage (no owning side) hurts everyone.
		return GetTeam(Source) == ETeam::None || AreEnemies(Source, Target);
	}
}
