#include "WeaponFireMode_Projectile.h"
#include "Core/ZombieDamageTypes.h"
#include "Weapons/Projectile/ZombieProjectile.h"
#include "Weapons/WeaponDataAsset.h"
#include "Weapons/ZombieWeapon.h"

void UWeaponFireMode_Projectile::Fire(const FWeaponFireContext& Context) const
{
	if (!Context.Weapon || !Context.Definition)
	{
		return;
	}

	FZombieProjectileLaunch Launch;
	Launch.Spec = Context.Definition->Projectile;
	Launch.Damage = Context.Stats.Damage;
	Launch.CritChance = Context.Stats.CritChance;
	Launch.CritMultiplier = Context.Stats.CritMultiplier;
	Launch.Pierce = Context.Stats.Pierce;
	Launch.Ricochet = Context.Stats.Ricochet;
	Launch.ExplosionRadius = Context.Stats.ExplosionRadius;
	Launch.DamageType = UDamageType_Bullet::StaticClass();
	Launch.HitEffects = Context.HitEffects;
	Launch.InstigatorController = Context.InstigatorController;
	Launch.SourceActor = Context.Weapon;

	// A projectile's lifetime is capped by the weapon's range, so range means the same thing for
	// hitscan and projectile guns.
	const float Speed = FMath::Max(Launch.Spec.Speed, 1.0f);
	Launch.Spec.Lifetime = FMath::Min(Launch.Spec.Lifetime, Context.Stats.Range / Speed);

	for (int32 Pellet = 0; Pellet < FMath::Max(Context.Stats.PelletsPerShot, 1); ++Pellet)
	{
		Launch.Velocity = ApplySpread(Context.Direction, Context.Stats.SpreadDegrees) * Speed;
		AZombieProjectile::SpawnFromPool(Context.Weapon->GetWorld(), Context.Origin, Launch);
	}
}
