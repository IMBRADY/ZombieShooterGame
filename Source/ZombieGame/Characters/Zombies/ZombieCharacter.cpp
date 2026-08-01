#include "ZombieCharacter.h"
#include "AI/ZombieAIController.h"
#include "Characters/Zombies/ZombieArchetypeDataAsset.h"
#include "Components/CapsuleComponent.h"
#include "Components/DamageComponent.h"
#include "Components/HealthComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "ZombieGame.h"

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
	// construction; calling it here leaves avoidance flagged on but unregistered, which stops the
	// pawn moving at all.
	GetCharacterMovement()->AvoidanceConsiderationRadius = 220.0f;
	GetCharacterMovement()->AvoidanceWeight = 0.5f;

	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(GetCapsuleComponent());
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaceholderMeshFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (PlaceholderMeshFinder.Succeeded())
	{
		BodyMesh->SetStaticMesh(PlaceholderMeshFinder.Object);
		BodyMesh->SetRelativeScale3D(FVector(0.6f, 0.6f, 1.76f));
	}

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
	DamageComponent = CreateDefaultSubobject<UDamageComponent>(TEXT("DamageComponent"));
}

void AZombieCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (HealthComponent)
	{
		HealthComponent->OnDeath.AddDynamic(this, &AZombieCharacter::HandleDeath);
	}

	// Every chasing zombie descends the same shared flow field, so without local avoidance a horde
	// converges into one overlapping column instead of spreading across the corridor.
	if (bUseLocalAvoidance)
	{
		GetCharacterMovement()->SetAvoidanceEnabled(true);
	}

	ApplyArchetypeVisuals();
}

void AZombieCharacter::InitializeFromArchetype(const UZombieArchetypeDataAsset* InArchetype, const FZombieDifficultyScaling& Scaling)
{
	if (!InArchetype)
	{
		UE_LOG(LogZombieGame, Error, TEXT("Zombie '%s' spawned without an archetype; it will use base defaults."), *GetName());
		return;
	}

	Archetype = InArchetype;
	ScaledAttackDamage = InArchetype->BaseAttackDamage * FMath::Max(Scaling.DamageMultiplier, 0.0f);
	RewardMultiplier = FMath::Max(Scaling.RewardMultiplier, 0.0f);

	if (HealthComponent)
	{
		HealthComponent->SetMaxHealth(InArchetype->BaseMaxHealth * FMath::Max(Scaling.HealthMultiplier, 0.01f), true);
	}

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = InArchetype->MoveSpeed;
	}

	if (BodyMesh)
	{
		BodyMesh->SetRelativeScale3D(FVector(0.6f, 0.6f, 1.76f) * InArchetype->BodyScale);
	}
}

void AZombieCharacter::ApplyArchetypeVisuals()
{
	if (!Archetype || !BodyMesh || !BodyMesh->GetStaticMesh())
	{
		return;
	}

	// Greybox tinting only - it exists so archetypes are distinguishable on screen before art
	// lands, and goes away with the placeholder mesh.
	BodyMaterial = BodyMesh->CreateDynamicMaterialInstance(0);
	if (BodyMaterial)
	{
		BodyMaterial->SetVectorParameterValue(TEXT("Color"), Archetype->DebugTint);
	}
}

float AZombieCharacter::GetAttackRange() const
{
	const float ArchetypeRange = Archetype ? Archetype->AttackRange : 160.0f;
	return ArchetypeRange + GetCapsuleComponent()->GetScaledCapsuleRadius();
}

int32 AZombieCharacter::GetMoneyReward() const
{
	const int32 BaseReward = Archetype ? Archetype->MoneyReward : 0;
	return FMath::RoundToInt(BaseReward * RewardMultiplier);
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

	UGameplayStatics::ApplyDamage(Target, ScaledAttackDamage, GetController(), this, UDamageType::StaticClass());
}

void AZombieCharacter::HandleDeath()
{
	if (bDeathHandled)
	{
		return;
	}
	bDeathHandled = true;

	AController* Killer = DamageComponent ? DamageComponent->GetLastDamageInstigator() : nullptr;

	// Stop being a combat participant immediately; the corpse lingers purely as feedback.
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();

	if (AController* OwnController = GetController())
	{
		OwnController->UnPossess();
		OwnController->Destroy();
	}

	OnZombieDied.Broadcast(this, Killer);

	SetLifeSpan(FMath::Max(CorpseLifetime, 0.1f));
}
