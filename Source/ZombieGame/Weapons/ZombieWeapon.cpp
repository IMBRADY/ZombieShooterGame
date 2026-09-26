#include "ZombieWeapon.h"
#include "Audio/ZombieAudioSubsystem.h"
#include "Core/ZombieStatSource.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "Perception/AISense_Hearing.h"
#include "TimerManager.h"
#include "Visual/ZombieEffectsSubsystem.h"
#include "Weapons/WeaponDataAsset.h"
#include "Weapons/WeaponFireMode.h"
#include "Weapons/WeaponRaritySettings.h"
#include "Weapons/WeaponStatsCalculator.h"
#include "ZombieGame.h"

AZombieWeapon::AZombieWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicatingMovement(false);

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void AZombieWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AZombieWeapon, Definition);
	DOREPLIFETIME(AZombieWeapon, Rarity);
	DOREPLIFETIME(AZombieWeapon, UpgradeLevel);
	DOREPLIFETIME(AZombieWeapon, AmmoInMagazine);
	DOREPLIFETIME(AZombieWeapon, ReserveAmmo);
	DOREPLIFETIME(AZombieWeapon, bReloading);
}

void AZombieWeapon::InitializeFromInstance(const FWeaponInstanceData& Instance)
{
	Definition = Instance.Definition.LoadSynchronous();
	Rarity = Instance.Rarity;
	UpgradeLevel = FMath::Clamp(Instance.UpgradeLevel, 0, 1);

	if (!Definition)
	{
		UE_LOG(LogZombieGame, Error, TEXT("Weapon '%s' could not load its definition '%s'."), *GetName(), *Instance.Definition.ToString());
		return;
	}

	RefreshStats();

	AmmoInMagazine = Instance.AmmoInMagazine < 0 ? EffectiveStats.MagazineSize : FMath::Min(Instance.AmmoInMagazine, EffectiveStats.MagazineSize);
	ReserveAmmo = Instance.ReserveAmmo < 0 ? EffectiveStats.MaxReserveAmmo : FMath::Min(Instance.ReserveAmmo, EffectiveStats.MaxReserveAmmo);
	BroadcastAmmo();
}

FWeaponInstanceData AZombieWeapon::ToInstanceData() const
{
	FWeaponInstanceData Instance;
	Instance.Definition = Definition.Get();
	Instance.Rarity = Rarity;
	Instance.UpgradeLevel = UpgradeLevel;
	Instance.AmmoInMagazine = AmmoInMagazine;
	Instance.ReserveAmmo = ReserveAmmo;
	return Instance;
}

void AZombieWeapon::RefreshStats()
{
	if (!Definition)
	{
		return;
	}

	const UWeaponRaritySettings* RaritySettings = UWeaponRaritySettings::GetOrLoadDefault();
	const IZombieStatSource* Modifiers = ZombieStats::FindStatSource(this);

	EffectiveStats = FWeaponStatsCalculator::Compute(*Definition, Rarity, UpgradeLevel, *RaritySettings, Modifiers);

	EffectiveHitEffects.Reset();
	FWeaponStatsCalculator::GatherHitEffects(*Definition, Rarity, Modifiers, EffectiveHitEffects);

	AmmoInMagazine = FMath::Min(AmmoInMagazine, EffectiveStats.MagazineSize);
	ReserveAmmo = FMath::Min(ReserveAmmo, EffectiveStats.MaxReserveAmmo);
	BroadcastAmmo();
}

bool AZombieWeapon::IsAutomatic() const
{
	return Definition && Definition->bAutomatic;
}

EWeaponFireResult AZombieWeapon::TryFire(const FVector& AimDirection)
{
	UWorld* World = GetWorld();
	if (!Definition || !Definition->FireMode || !World)
	{
		return EWeaponFireResult::NoDefinition;
	}

	if (bReloading)
	{
		return EWeaponFireResult::Reloading;
	}

	const double Now = World->GetTimeSeconds();
	if (Now - LastFireTime < 1.0 / EffectiveStats.ShotsPerSecond)
	{
		return EWeaponFireResult::Cooldown;
	}

	if (AmmoInMagazine <= 0)
	{
		// "Reloads automatically if ammo is completely depleted and spare magazines are available."
		if (!StartReload())
		{
			if (UZombieAudioSubsystem* Audio = UZombieAudioSubsystem::Get(this))
			{
				Audio->PlaySoundAtLocation(Definition->EmptySound, GetActorLocation());
			}
			LastFireTime = Now;
		}
		return EWeaponFireResult::EmptyMagazine;
	}

	LastFireTime = Now;
	--AmmoInMagazine;

	APawn* OwnerPawn = GetInstigator();
	FWeaponFireContext Context;
	Context.Weapon = this;
	Context.Definition = Definition;
	Context.InstigatorPawn = OwnerPawn;
	Context.InstigatorController = OwnerPawn ? OwnerPawn->GetController() : nullptr;
	Context.Direction = AimDirection.GetSafeNormal2D();
	Context.Origin = GetMuzzleLocation(Context.Direction);
	Context.Stats = EffectiveStats;
	Context.HitEffects = EffectiveHitEffects;
	Context.TracerColor = IsUpgraded() ? Definition->UpgradeBonus.UpgradedTracerColor : Definition->TracerColor;

	GetDefault<UWeaponFireMode>(Definition->FireMode)->Fire(Context);
	PlayFireFeedback(Context.Origin, Context.Direction);
	BroadcastAmmo();

	if (AmmoInMagazine <= 0)
	{
		StartReload();
	}

	return EWeaponFireResult::Fired;
}

FVector AZombieWeapon::GetMuzzleLocation(const FVector& Direction) const
{
	const AActor* Carrier = GetOwner() ? GetOwner() : this;
	const float Offset = Definition ? Definition->MuzzleOffset : 60.0f;
	return Carrier->GetActorLocation() + Direction * Offset;
}

void AZombieWeapon::PlayFireFeedback(const FVector& Muzzle, const FVector& Direction)
{
	if (UZombieEffectsSubsystem* Effects = UZombieEffectsSubsystem::Get(this))
	{
		Effects->PlayEffect(Definition->MuzzleFlash, Muzzle, Direction.Rotation().Yaw);
	}

	if (UZombieAudioSubsystem* Audio = UZombieAudioSubsystem::Get(this))
	{
		Audio->PlaySoundAtLocation(Definition->FireSound, Muzzle);
	}

	// "Gunshots attract zombies very well": every shot is a hearing stimulus across NoiseRange.
	UAISense_Hearing::ReportNoiseEvent(this, Muzzle, 1.0f, GetInstigator(), EffectiveStats.NoiseRange);

	OnFired.Broadcast(Definition->ShakeStrength);
}

bool AZombieWeapon::StartReload()
{
	UWorld* World = GetWorld();
	if (!Definition || !World || bReloading || ReserveAmmo <= 0 || AmmoInMagazine >= EffectiveStats.MagazineSize)
	{
		return false;
	}

	bReloading = true;
	ReloadStartTime = World->GetTimeSeconds();
	World->GetTimerManager().SetTimer(ReloadTimer, this, &AZombieWeapon::FinishReload, EffectiveStats.ReloadTime, false);

	if (UZombieAudioSubsystem* Audio = UZombieAudioSubsystem::Get(this))
	{
		Audio->PlaySoundAtLocation(Definition->ReloadSound, GetActorLocation());
	}

	OnReloadChanged.Broadcast(true, EffectiveStats.ReloadTime);
	return true;
}

void AZombieWeapon::FinishReload()
{
	const int32 Needed = EffectiveStats.MagazineSize - AmmoInMagazine;
	const int32 Loaded = FMath::Min(Needed, ReserveAmmo);

	AmmoInMagazine += Loaded;
	ReserveAmmo -= Loaded;
	bReloading = false;

	OnReloadChanged.Broadcast(false, 0.0f);
	BroadcastAmmo();
}

void AZombieWeapon::CancelReload()
{
	if (!bReloading)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReloadTimer);
	}
	bReloading = false;
	OnReloadChanged.Broadcast(false, 0.0f);
}

float AZombieWeapon::GetReloadProgress() const
{
	const UWorld* World = GetWorld();
	if (!bReloading || !World || EffectiveStats.ReloadTime <= 0.0f)
	{
		return 0.0f;
	}
	return FMath::Clamp(static_cast<float>((World->GetTimeSeconds() - ReloadStartTime) / EffectiveStats.ReloadTime), 0.0f, 1.0f);
}

void AZombieWeapon::SetEquipped(bool bEquipped)
{
	if (!bEquipped)
	{
		CancelReload();
		return;
	}

	// Drawing an empty gun with spare ammo starts the reload straight away.
	if (AmmoInMagazine <= 0)
	{
		StartReload();
	}
}

void AZombieWeapon::Upgrade()
{
	if (UpgradeLevel > 0)
	{
		return;
	}

	UpgradeLevel = 1;
	RefreshStats();
	AmmoInMagazine = EffectiveStats.MagazineSize;
	BroadcastAmmo();
}

void AZombieWeapon::RefillAmmo()
{
	CancelReload();
	AmmoInMagazine = EffectiveStats.MagazineSize;
	ReserveAmmo = EffectiveStats.MaxReserveAmmo;
	BroadcastAmmo();
}

int32 AZombieWeapon::AddReserveAmmo(int32 Amount)
{
	const int32 Before = ReserveAmmo;
	ReserveAmmo = FMath::Clamp(ReserveAmmo + FMath::Max(Amount, 0), 0, EffectiveStats.MaxReserveAmmo);
	BroadcastAmmo();
	return ReserveAmmo - Before;
}

void AZombieWeapon::BroadcastAmmo()
{
	OnAmmoChanged.Broadcast(AmmoInMagazine, ReserveAmmo);
}
