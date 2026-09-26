#include "ZombieRunTypes.h"

void FZombieRunStats::AddWeaponKill(const FText& WeaponName)
{
	if (WeaponName.IsEmpty())
	{
		return;
	}

	FWeaponKillCount* Entry = KillsByWeapon.FindByPredicate([&WeaponName](const FWeaponKillCount& Count)
	{
		return Count.WeaponName.EqualTo(WeaponName);
	});

	if (!Entry)
	{
		Entry = &KillsByWeapon.AddDefaulted_GetRef();
		Entry->WeaponName = WeaponName;
	}
	++Entry->Kills;
}

FText FZombieRunStats::GetFavoriteWeapon() const
{
	const FWeaponKillCount* Best = nullptr;
	for (const FWeaponKillCount& Count : KillsByWeapon)
	{
		if (!Best || Count.Kills > Best->Kills)
		{
			Best = &Count;
		}
	}
	return Best ? Best->WeaponName : FText::GetEmpty();
}
