#include "LootPickup.h"
#include "Audio/ZombieAudioSubsystem.h"
#include "Components/SphereComponent.h"
#include "Core/ZombieGameplayTags.h"
#include "Core/ZombieStatSource.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Loot/LootResolver.h"
#include "TimerManager.h"
#include "UI/ZombieNotifications.h"
#include "Utilities/ActorPoolSubsystem.h"
#include "Visual/PixelSpriteComponent.h"
#include "Visual/SpriteSheetDataAsset.h"

#define LOCTEXT_NAMESPACE "ZombieLoot"

namespace
{
	constexpr float PickupHeight = 40.0f;
	constexpr float MagnetCheckInterval = 0.15f;
	constexpr int32 LargeMoneyThreshold = 60;
}

ALootPickup::ALootPickup()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(55.0f);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Collision->SetGenerateOverlapEvents(true);
	Collision->SetCanEverAffectNavigation(false);
	SetRootComponent(Collision);

	Sprite = CreateDefaultSubobject<UPixelSpriteComponent>(TEXT("Sprite"));
	Sprite->SetupAttachment(Collision);
	Sprite->SetRelativeLocation(FVector(0.0f, 0.0f, -PickupHeight + 6.0f));

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->bAutoActivate = false;
	Movement->ProjectileGravityScale = 0.0f;
	Movement->bIsHomingProjectile = true;
	Movement->HomingAccelerationMagnitude = 9000.0f;
	Movement->MaxSpeed = 2200.0f;
	Movement->InitialSpeed = 0.0f;

	PickupSheet = TSoftObjectPtr<USpriteSheetDataAsset>(FSoftObjectPath(TEXT("/Game/Sprites/Pickups/SS_Pickups.SS_Pickups")));
}

ALootPickup* ALootPickup::SpawnLoot(UWorld* World, const FResolvedLoot& Loot, const FVector& Location, bool bScatter)
{
	UActorPoolSubsystem* Pool = UActorPoolSubsystem::Get(World);
	if (!Pool)
	{
		return nullptr;
	}

	FVector SpawnLocation(Location.X, Location.Y, PickupHeight);
	if (bScatter)
	{
		SpawnLocation += FVector(FMath::FRandRange(-45.0f, 45.0f), FMath::FRandRange(-45.0f, 45.0f), 0.0f);
	}

	ALootPickup* Pickup = Pool->Acquire<ALootPickup>(ALootPickup::StaticClass(), FTransform(SpawnLocation));
	if (Pickup)
	{
		Pickup->Setup(Loot);
	}
	return Pickup;
}

void ALootPickup::BeginPlay()
{
	Super::BeginPlay();
	Collision->OnComponentBeginOverlap.AddUniqueDynamic(this, &ALootPickup::HandleOverlap);
}

void ALootPickup::OnAcquiredFromPool()
{
	bCollected = false;
}

void ALootPickup::Setup(const FResolvedLoot& InLoot)
{
	Loot = InLoot;
	bCollected = false;

	Sprite->SetSpriteSheet(PickupSheet.LoadSynchronous());
	Sprite->PlayAnimation(GetAnimationName(), true);
	Sprite->SetFacingYaw(0.0f);

	if (Loot.Type == ELootRewardType::Money)
	{
		GetWorldTimerManager().SetTimer(MagnetTimer, this, &ALootPickup::CheckMagnet, MagnetCheckInterval, true,
			FMath::FRandRange(0.0f, MagnetCheckInterval));
	}

	// Something may already be standing on the drop spot.
	TArray<AActor*> Overlapping;
	Collision->GetOverlappingActors(Overlapping, APawn::StaticClass());
	for (AActor* Actor : Overlapping)
	{
		if (Loot.IsAutoCollected() && TryCollect(Cast<APawn>(Actor)))
		{
			return;
		}
	}
}

FName ALootPickup::GetAnimationName() const
{
	switch (Loot.Type)
	{
	case ELootRewardType::Money:	return Loot.Amount >= LargeMoneyThreshold ? TEXT("MoneyLarge") : TEXT("Money");
	case ELootRewardType::Health:	return TEXT("Health");
	case ELootRewardType::Armor:	return TEXT("Armor");
	case ELootRewardType::Ammo:		return TEXT("Ammo");
	case ELootRewardType::Key:		return TEXT("Key");
	case ELootRewardType::Perk:		return TEXT("Perk");
	case ELootRewardType::Weapon:	return Loot.Weapon.Rarity >= EWeaponRarity::Epic ? TEXT("WeaponRare") : TEXT("Weapon");
	default:						return TEXT("Money");
	}
}

void ALootPickup::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (Loot.IsAutoCollected())
	{
		TryCollect(Cast<APawn>(OtherActor));
	}
}

bool ALootPickup::TryCollect(APawn* Collector)
{
	if (bCollected || !Collector || !Collector->IsPlayerControlled() || !HasAuthority())
	{
		return false;
	}

	FText Description;
	FResolvedLoot Displaced;
	if (!FLootResolver::Grant(Loot, Collector, Description, &Displaced))
	{
		return false;
	}

	bCollected = true;

	if (UZombieAudioSubsystem* Audio = UZombieAudioSubsystem::Get(this))
	{
		static const FName SoundNames[] = { TEXT("Pickup.Money"), TEXT("Pickup.Health"), TEXT("Pickup.Armor"), TEXT("Pickup.Ammo"),
			TEXT("Pickup.Weapon"), TEXT("Pickup.Perk"), TEXT("Pickup.Key") };
		Audio->PlayNamedSoundAtLocation(SoundNames[static_cast<int32>(Loot.Type)], GetActorLocation());
	}

	// Money ticks up on the HUD by itself; everything else deserves a message.
	if (Loot.Type != ELootRewardType::Money)
	{
		ZombieNotifications::Notify(Collector, Description);
	}

	if (Displaced.Weapon.IsValid())
	{
		SpawnLoot(GetWorld(), Displaced, GetActorLocation(), true);
	}

	ReturnToPool();
	return true;
}

void ALootPickup::CheckMagnet()
{
	if (bCollected || Movement->IsActive())
	{
		return;
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APawn* Pawn = It->IsValid() ? It->Get()->GetPawn() : nullptr;
		if (!Pawn)
		{
			continue;
		}

		const float Radius = ZombieStats::Resolve(Pawn, ZombieTags::Stat_Pickup_Radius, BaseMagnetRadius);
		if (FVector::DistSquared2D(Pawn->GetActorLocation(), GetActorLocation()) <= FMath::Square(Radius))
		{
			AttractTo(Pawn);
			return;
		}
	}
}

void ALootPickup::AttractTo(APawn* Target)
{
	if (!Target || bCollected)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(MagnetTimer);
	Movement->SetUpdatedComponent(Collision);
	Movement->HomingTargetComponent = Target->GetRootComponent();
	Movement->Velocity = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal2D() * 300.0f;
	Movement->Activate(true);
}

void ALootPickup::Interact_Implementation(AActor* Interactor)
{
	TryCollect(Cast<APawn>(Interactor));
}

FText ALootPickup::GetInteractionPrompt_Implementation(AActor* Interactor) const
{
	return FText::Format(LOCTEXT("TakePrompt", "Take {0}"), FLootResolver::Describe(Loot));
}

bool ALootPickup::CanInteract_Implementation(AActor* Interactor) const
{
	return !bCollected && !Loot.IsAutoCollected() && !IsHidden();
}

void ALootPickup::ReturnToPool()
{
	if (UActorPoolSubsystem* Pool = UActorPoolSubsystem::Get(this))
	{
		Pool->Release(this);
	}
	else
	{
		Destroy();
	}
}

void ALootPickup::OnReleasedToPool()
{
	bCollected = true;
	GetWorldTimerManager().ClearTimer(MagnetTimer);
	Movement->StopMovementImmediately();
	Movement->Deactivate();
	Movement->HomingTargetComponent = nullptr;
	Loot = FResolvedLoot();
}

#undef LOCTEXT_NAMESPACE
