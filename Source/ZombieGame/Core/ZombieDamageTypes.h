#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "GameplayTagContainer.h"
#include "ZombieDamageTypes.generated.h"

/**
 * Damage types carry a Gameplay Tag so resistances ("Armored zombie: reduced bullet damage",
 * the Explosion Resistance perk) can be authored as data against the tag, without the damage
 * pipeline knowing which kinds of damage exist.
 */
UCLASS(Abstract)
class ZOMBIEGAME_API UZombieDamageType : public UDamageType
{
	GENERATED_BODY()

public:
	const FGameplayTag& GetDamageTag() const { return DamageTag; }

	/** The tag of whatever damage type class a hit used; empty for non-project damage types. */
	static FGameplayTag GetTagFor(const UDamageType* DamageType);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Damage", meta = (Categories = "Damage"))
	FGameplayTag DamageTag;
};

UCLASS()
class ZOMBIEGAME_API UDamageType_Bullet : public UZombieDamageType
{
	GENERATED_BODY()
public:
	UDamageType_Bullet();
};

UCLASS()
class ZOMBIEGAME_API UDamageType_Explosion : public UZombieDamageType
{
	GENERATED_BODY()
public:
	UDamageType_Explosion();
};

UCLASS()
class ZOMBIEGAME_API UDamageType_Fire : public UZombieDamageType
{
	GENERATED_BODY()
public:
	UDamageType_Fire();
};

UCLASS()
class ZOMBIEGAME_API UDamageType_Poison : public UZombieDamageType
{
	GENERATED_BODY()
public:
	UDamageType_Poison();
};

UCLASS()
class ZOMBIEGAME_API UDamageType_Melee : public UZombieDamageType
{
	GENERATED_BODY()
public:
	UDamageType_Melee();
};
