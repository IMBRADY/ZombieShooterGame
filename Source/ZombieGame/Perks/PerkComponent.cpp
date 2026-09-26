#include "PerkComponent.h"
#include "Net/UnrealNetwork.h"
#include "Perks/PerkDataAsset.h"

UPerkComponent::UPerkComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UPerkComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UPerkComponent, OwnedPerks);
}

FStatModifierTotals UPerkComponent::GetStatTotals(const FGameplayTag& Stat) const
{
	FStatModifierTotals Totals;
	for (const FOwnedPerk& Owned : OwnedPerks)
	{
		if (!Owned.Perk)
		{
			continue;
		}

		for (const FStatModifier& Modifier : Owned.Perk->ModifiersPerTier)
		{
			if (Modifier.Stat.MatchesTagExact(Stat))
			{
				Totals.Accumulate(Modifier, static_cast<float>(Owned.Tier));
			}
		}
	}
	return Totals;
}

void UPerkComponent::GetHitEffects(TArray<FHitEffectSpec>& OutEffects) const
{
	for (const FOwnedPerk& Owned : OwnedPerks)
	{
		if (!Owned.Perk)
		{
			continue;
		}

		for (FHitEffectSpec Effect : Owned.Perk->HitEffectsPerTier)
		{
			Effect.Chance = FMath::Min(Effect.Chance * Owned.Tier, 1.0f);
			OutEffects.Add(Effect);
		}
	}
}

bool UPerkComponent::AddPerkTier(UPerkDataAsset* Perk)
{
	if (!Perk || GetOwnerRole() != ROLE_Authority)
	{
		return false;
	}

	FOwnedPerk* Owned = OwnedPerks.FindByPredicate([Perk](const FOwnedPerk& Entry) { return Entry.Perk == Perk; });
	if (!Owned)
	{
		Owned = &OwnedPerks.AddDefaulted_GetRef();
		Owned->Perk = Perk;
	}

	if (Owned->Tier >= Perk->MaxTier)
	{
		return false;
	}

	++Owned->Tier;
	StatsChangedEvent.Broadcast();
	return true;
}

int32 UPerkComponent::GetTier(const UPerkDataAsset* Perk) const
{
	const FOwnedPerk* Owned = OwnedPerks.FindByPredicate([Perk](const FOwnedPerk& Entry) { return Entry.Perk == Perk; });
	return Owned ? Owned->Tier : 0;
}

bool UPerkComponent::IsMaxed(const UPerkDataAsset* Perk) const
{
	return Perk && GetTier(Perk) >= Perk->MaxTier;
}

void UPerkComponent::ExportRecords(TArray<FOwnedPerkRecord>& OutRecords) const
{
	for (const FOwnedPerk& Owned : OwnedPerks)
	{
		FOwnedPerkRecord& Record = OutRecords.AddDefaulted_GetRef();
		Record.Perk = Owned.Perk.Get();
		Record.Tier = Owned.Tier;
	}
}

void UPerkComponent::ImportRecords(const TArray<FOwnedPerkRecord>& Records)
{
	OwnedPerks.Reset();
	for (const FOwnedPerkRecord& Record : Records)
	{
		if (UPerkDataAsset* Perk = Record.Perk.LoadSynchronous())
		{
			FOwnedPerk& Owned = OwnedPerks.AddDefaulted_GetRef();
			Owned.Perk = Perk;
			Owned.Tier = FMath::Clamp(Record.Tier, 1, Perk->MaxTier);
		}
	}
	StatsChangedEvent.Broadcast();
}

void UPerkComponent::OnRep_OwnedPerks()
{
	StatsChangedEvent.Broadcast();
}
