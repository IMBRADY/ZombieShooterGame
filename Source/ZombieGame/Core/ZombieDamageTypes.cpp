#include "ZombieDamageTypes.h"
#include "Core/ZombieGameplayTags.h"

FGameplayTag UZombieDamageType::GetTagFor(const UDamageType* DamageType)
{
	const UZombieDamageType* ZombieDamageType = Cast<UZombieDamageType>(DamageType);
	return ZombieDamageType ? ZombieDamageType->GetDamageTag() : FGameplayTag();
}

UDamageType_Bullet::UDamageType_Bullet()
{
	DamageTag = ZombieTags::Damage_Bullet;
}

UDamageType_Explosion::UDamageType_Explosion()
{
	DamageTag = ZombieTags::Damage_Explosion;
	bRadialDamageVelChange = true;
}

UDamageType_Fire::UDamageType_Fire()
{
	DamageTag = ZombieTags::Damage_Fire;
}

UDamageType_Poison::UDamageType_Poison()
{
	DamageTag = ZombieTags::Damage_Poison;
}

UDamageType_Melee::UDamageType_Melee()
{
	DamageTag = ZombieTags::Damage_Melee;
}
