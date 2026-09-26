#include "ZombieCharacter.h"
#include "AI/ZombieAIController.h"
#include "Audio/ZombieAudioSubsystem.h"
#include "Characters/Player/ZombiePlayerCharacter.h"
#include "Characters/Zombies/Abilities/ZombieAbilityComponent.h"
#include "Characters/Zombies/ZombieArchetypeDataAsset.h"
#include "Characters/Zombies/ZombieEnemyManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/DamageComponent.h"
#include "Components/HealthComponent.h"
#include "Components/StatusEffectComponent.h"
#include "Core/ZombieDamageTypes.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Visual/PixelSpriteComponent.h"
#include "Visual/ZombieEffectsSubsystem.h"
#include "ZombieGame.h"

namespace
{
	constexpr float BaseCapsuleRadius = 34.0f;
	constexpr float SpriteHeightAboveFeet = 10.0f;
	constexpr float AnimationUpdateInterval = 0.15f;
	constexpr float WalkAnimationSpeedThreshold = 25.0f;

	/** One scream at a time across the whole horde, so a wave spotting you is a cue, not noise. */
	constexpr double AlertSoundCooldown = 1.2;
	double GLastAlertSoundTime = -100.0;
}

AZombieCharacter::AZombieCharacter()
{
	// No per-frame work: perception, the Behavior Tree and timers drive everything
	// (prompt.txt "No unnecessary Tick functions").
	PrimaryActorTick.bCanEverTick = false;

	AIControllerClass = AZombieAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	GetCharacterMovement()->bUseControllerDesiredRotation = false;

	// Avoidance tuning only - it is switched on in BeginPlay, not here. SetAvoidanceEnabled needs a
	// character owner and a world to register with the avoidance manager, and has neither during
	// construction; calling it here leaves avoidance flagged on but unregistered.
	GetCharacterMovement()->AvoidanceConsiderationRadius = 220.0f;
	GetCharacterMovement()->AvoidanceWeight = 0.5f;

	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));

	BodySprite = CreateDefaultSubobject<UPixelSpriteComponent>(TEXT("BodySprite"));
	BodySprite->SetupAttachment(GetCapsuleComponent());
	BodySprite->SetRelativeLocation(FVector(0.0f, 0.0f, -GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() + SpriteHeightAboveFeet));

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	DamageComponent = CreateDefaultSubobject<UDamageComponent>(TEXT("DamageComponent"));
	StatusEffectComponent = CreateDefaultSubobject<UStatusEffectComponent>(TEXT("StatusEffectComponent"));
	AbilityComponent = CreateDefaultSubobject<UZombieAbilityComponent>(TEXT("AbilityComponent"));
}

void AZombieCharacter::InitializeFromArchetype(const UZombieArchetypeDataAsset* InArchetype, const FZombieDifficultyScaling& Scaling, bool bInGrantsRewards)
{
	if (!InArchetype)
	{
		UE_LOG(LogZombieGame, Error, TEXT("Zombie '%s' spawned without an archetype; it will use base defaults."), *GetName());
		return;
	}

	Archetype = InArchetype;
	bGrantsRewards = bInGrantsRewards;
	DamageMultiplier = FMath::Max(Scaling.DamageMultiplier, 0.0f);
	ScaledAttackDamage = InArchetype->BaseAttackDamage * DamageMultiplier;
	RewardMultiplier = FMath::Max(Scaling.RewardMultiplier, 0.0f);

	HealthComponent->SetMaxHealth(InArchetype->BaseMaxHealth * FMath::Max(Scaling.HealthMultiplier, 0.01f), true);

	for (const TPair<FGameplayTag, float>& Resistance : InArchetype->DamageResistances)
	{
		DamageComponent->SetResistance(Resistance.Key, Resistance.Value);
	}

	GetCapsuleComponent()->SetCapsuleRadius(BaseCapsuleRadius * InArchetype->BodyScale);
	AbilityComponent->Initialize(InArchetype->Abilities, InArchetype->PhaseHealthThresholds);
	RefreshMoveSpeed();
}

void AZombieCharacter::BeginPlay()
{
	Super::BeginPlay();

	HealthComponent->OnDeath.AddDynamic(this, &AZombieCharacter::HandleDeath);
	DamageComponent->OnDamageReceived.AddUObject(this, &AZombieCharacter::HandleDamageReceived);
	StatusEffectComponent->OnStatusEffectsChanged.AddUObject(this, &AZombieCharacter::HandleStatusEffectsChanged);
	AbilityComponent->OnPhaseChanged.AddUObject(this, &AZombieCharacter::HandlePhaseChanged);

	// Every chasing zombie descends the same shared flow field, so without local avoidance a horde
	// converges into one overlapping column instead of spreading across the corridor.
	if (bUseLocalAvoidance)
	{
		GetCharacterMovement()->SetAvoidanceEnabled(true);
	}

	ApplyArchetypeVisuals();
	AbilityComponent->NotifySpawned();

	GetWorldTimerManager().SetTimer(AnimationTimer, this, &AZombieCharacter::UpdateLocomotionAnimation, AnimationUpdateInterval, true,
		FMath::FRandRange(0.0f, AnimationUpdateInterval));
	GetWorldTimerManager().SetTimer(IdleSoundTimer, this, &AZombieCharacter::PlayIdleSound, FMath::FRandRange(4.0f, 12.0f), false);
}

void AZombieCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearAllTimersForObject(this);
	Super::EndPlay(EndPlayReason);
}

void AZombieCharacter::ApplyArchetypeVisuals()
{
	if (!Archetype)
	{
		return;
	}

	BodySprite->SetSpriteSheet(Archetype->SpriteSheet, Archetype->BodyScale);
	BodySprite->SetTint(Archetype->Tint);
	BodySprite->PlayAnimation(TEXT("Walk"), true);
}

void AZombieCharacter::RefreshMoveSpeed()
{
	const float BaseSpeed = Archetype ? Archetype->MoveSpeed : 180.0f;
	GetCharacterMovement()->MaxWalkSpeed = BaseSpeed * PhaseSpeedMultiplier * StatusEffectComponent->GetMoveSpeedMultiplier();
}

void AZombieCharacter::UpdateLocomotionAnimation()
{
	const UWorld* World = GetWorld();
	if (IsDead() || !World || World->GetTimeSeconds() < ActionAnimationEndsAt)
	{
		return;
	}

	const bool bMoving = GetVelocity().SizeSquared2D() > FMath::Square(WalkAnimationSpeedThreshold);
	BodySprite->PlayAnimation(bMoving ? FName(TEXT("Walk")) : FName(TEXT("Idle")));
}

void AZombieCharacter::PlayActionAnimation(FName AnimationName, float Duration)
{
	if (IsDead() || AnimationName.IsNone())
	{
		return;
	}

	BodySprite->PlayAnimation(AnimationName, true);
	ActionAnimationEndsAt = GetWorld()->GetTimeSeconds() + FMath::Max(Duration, BodySprite->GetAnimationDuration(AnimationName));
}

float AZombieCharacter::GetAttackRange() const
{
	const float ArchetypeRange = Archetype ? Archetype->AttackRange : 160.0f;
	return ArchetypeRange + GetCapsuleComponent()->GetScaledCapsuleRadius();
}

int32 AZombieCharacter::GetMoneyReward() const
{
	const int32 BaseReward = Archetype ? Archetype->MoneyReward : 0;
	return bGrantsRewards ? FMath::RoundToInt(BaseReward * RewardMultiplier) : 0;
}

bool AZombieCharacter::IsBoss() const
{
	return Archetype && Archetype->Tier == EZombieClassTier::Boss;
}

bool AZombieCharacter::IsDead() const
{
	return bDeathHandled || (HealthComponent && HealthComponent->IsDead());
}

void AZombieCharacter::PerformAttack(AActor* Target)
{
	if (!Target || IsDead())
	{
		return;
	}

	UGameplayStatics::ApplyDamage(Target, ScaledAttackDamage, GetController(), this, UDamageType_Melee::StaticClass());
	if (UZombieAudioSubsystem* Audio = UZombieAudioSubsystem::Get(this))
	{
		Audio->PlaySoundAtLocation(Archetype ? Archetype->AttackSound : FZombieSoundSpec(), GetActorLocation());
	}
}

void AZombieCharacter::PlayAlertSound()
{
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	if (!Archetype || Now - GLastAlertSoundTime < AlertSoundCooldown)
	{
		return;
	}

	GLastAlertSoundTime = Now;
	if (UZombieAudioSubsystem* Audio = UZombieAudioSubsystem::Get(this))
	{
		Audio->PlaySoundAtLocation(Archetype->AlertSound, GetActorLocation());
	}
}

void AZombieCharacter::PlayIdleSound()
{
	if (IsDead())
	{
		return;
	}

	if (UZombieAudioSubsystem* Audio = UZombieAudioSubsystem::Get(this))
	{
		Audio->PlaySoundAtLocation(Archetype ? Archetype->IdleSound : FZombieSoundSpec(), GetActorLocation());
	}
	GetWorldTimerManager().SetTimer(IdleSoundTimer, this, &AZombieCharacter::PlayIdleSound, FMath::FRandRange(6.0f, 16.0f), false);
}

void AZombieCharacter::PlayRiseEffect()
{
	BodySprite->Flash(FLinearColor(0.6f, 0.2f, 1.0f), 0.5f);
	if (UZombieEffectsSubsystem* Effects = UZombieEffectsSubsystem::Get(this))
	{
		Effects->PlayEffect(Archetype ? Archetype->DeathEffect : FZombieEffectSpec(), GetActorLocation());
	}
}

void AZombieCharacter::ShakeNearbyPlayers(const FVector& Origin, float Strength) const
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AZombiePlayerCharacter* Player = It->IsValid() ? Cast<AZombiePlayerCharacter>(It->Get()->GetPawn()) : nullptr;
		if (Player)
		{
			const float Falloff = 1.0f - FMath::Clamp(FVector::Dist2D(Origin, Player->GetActorLocation()) / 1600.0f, 0.0f, 1.0f);
			Player->AddScreenShake(Strength * Falloff);
		}
	}
}

void AZombieCharacter::HandleDamageReceived(float Amount, AActor* Causer, const UDamageType* DamageType)
{
	BodySprite->Flash(FLinearColor::White, 0.08f);

	if (Archetype && HealthComponent)
	{
		AbilityComponent->UpdatePhase(HealthComponent->GetHealthFraction());
	}

	if (UZombieEffectsSubsystem* Effects = UZombieEffectsSubsystem::Get(this))
	{
		Effects->PlayEffect(Archetype ? Archetype->HitEffect : FZombieEffectSpec(), GetActorLocation());
	}

	const double Now = GetWorld()->GetTimeSeconds();
	if (Now - LastHurtSoundTime > 0.35 && !IsDead())
	{
		LastHurtSoundTime = Now;
		if (UZombieAudioSubsystem* Audio = UZombieAudioSubsystem::Get(this))
		{
			Audio->PlaySoundAtLocation(Archetype ? Archetype->HurtSound : FZombieSoundSpec(), GetActorLocation());
		}
	}
}

void AZombieCharacter::HandleStatusEffectsChanged()
{
	RefreshMoveSpeed();
	const FLinearColor BaseTint = Archetype ? Archetype->Tint : FLinearColor::White;
	BodySprite->SetTint(StatusEffectComponent->HasAnyStatus() ? BaseTint * StatusEffectComponent->GetDisplayTint() : BaseTint);
}

void AZombieCharacter::HandlePhaseChanged(int32 NewPhase)
{
	// Each boss phase is faster and announces itself.
	PhaseSpeedMultiplier = FMath::Pow(Archetype ? Archetype->PhaseSpeedMultiplier : 1.0f, static_cast<float>(NewPhase));
	RefreshMoveSpeed();
	BodySprite->Flash(FLinearColor(1.0f, 0.2f, 0.1f), 0.6f);
	PlayActionAnimation(TEXT("Attack"), 0.8f);
	ShakeNearbyPlayers(GetActorLocation(), 0.5f);

	if (UZombieAudioSubsystem* Audio = UZombieAudioSubsystem::Get(this))
	{
		Audio->PlaySoundAtLocation(Archetype ? Archetype->AlertSound : FZombieSoundSpec(), GetActorLocation());
	}
}

void AZombieCharacter::HandleDeath()
{
	if (bDeathHandled)
	{
		return;
	}
	bDeathHandled = true;

	AController* Killer = DamageComponent ? DamageComponent->GetLastDamageInstigator() : nullptr;

	// Stop being a combat participant immediately; the corpse lingers as feedback (and for revival).
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetWorldTimerManager().ClearTimer(AnimationTimer);
	GetWorldTimerManager().ClearTimer(IdleSoundTimer);
	StatusEffectComponent->ClearAll();

	if (AController* OwnController = GetController())
	{
		OwnController->UnPossess();
		OwnController->Destroy();
	}

	BodySprite->PlayAnimation(TEXT("Death"), true);
	BodySprite->SetTint(Archetype ? Archetype->Tint * 0.8f : FLinearColor::Gray);

	if (UZombieEffectsSubsystem* Effects = UZombieEffectsSubsystem::Get(this))
	{
		Effects->PlayEffect(Archetype ? Archetype->DeathEffect : FZombieEffectSpec(), GetActorLocation());
		Effects->SpawnDecal(Archetype ? Archetype->CorpseDecal : FZombieEffectSpec(), FVector(GetActorLocation().X, GetActorLocation().Y, 0.0f));
	}
	if (UZombieAudioSubsystem* Audio = UZombieAudioSubsystem::Get(this))
	{
		Audio->PlaySoundAtLocation(Archetype ? Archetype->DeathSound : FZombieSoundSpec(), GetActorLocation());
	}

	AbilityComponent->NotifyDied();
	OnZombieDied.Broadcast(this, Killer);

	if (!IsBoss())
	{
		if (UZombieEnemyManager* Enemies = UZombieEnemyManager::Get(this))
		{
			Enemies->RegisterCorpse(this);
		}
	}

	FTimerHandle FadeTimer;
	GetWorldTimerManager().SetTimer(FadeTimer, FTimerDelegate::CreateWeakLambda(this, [this]() { BodySprite->FadeOut(1.5f); }),
		FMath::Max(CorpseLifetime - 1.5f, 0.1f), false);
	SetLifeSpan(FMath::Max(CorpseLifetime, 0.1f));
}
