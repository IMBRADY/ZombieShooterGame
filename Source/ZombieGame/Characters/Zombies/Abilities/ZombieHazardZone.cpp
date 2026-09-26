#include "ZombieHazardZone.h"
#include "Audio/ZombieAudioSubsystem.h"
#include "Components/SphereComponent.h"
#include "Components/StatusEffectComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "Visual/PixelSpriteComponent.h"
#include "Visual/SpriteSheetDataAsset.h"

namespace
{
	/** Puddles sit just above floor decals so they read over blood splats. */
	constexpr float HazardHeight = 3.0f;
}

AZombieHazardZone::AZombieHazardZone()
{
	PrimaryActorTick.bCanEverTick = false;

	Area = CreateDefaultSubobject<USphereComponent>(TEXT("Area"));
	Area->InitSphereRadius(140.0f);
	Area->SetCollisionObjectType(ECC_WorldDynamic);
	Area->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Area->SetCollisionResponseToAllChannels(ECR_Ignore);
	Area->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Area->SetCanEverAffectNavigation(false);
	SetRootComponent(Area);

	Sprite = CreateDefaultSubobject<UPixelSpriteComponent>(TEXT("Sprite"));
	Sprite->SetupAttachment(Area);
}

AZombieHazardZone* AZombieHazardZone::SpawnHazard(UWorld* World, const UZombieHazardDataAsset* Definition, const FVector& GroundLocation, AActor* Source)
{
	if (!World || !Definition)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector Location(GroundLocation.X, GroundLocation.Y, 60.0f);
	AZombieHazardZone* Hazard = World->SpawnActor<AZombieHazardZone>(AZombieHazardZone::StaticClass(), Location, FRotator::ZeroRotator, SpawnParams);
	if (Hazard)
	{
		Hazard->Setup(Definition, ZombieTeams::GetTeam(Source));
	}
	return Hazard;
}

void AZombieHazardZone::Setup(const UZombieHazardDataAsset* InDefinition, ZombieTeams::ETeam InSourceTeam)
{
	Definition = InDefinition;
	SourceTeam = InSourceTeam;
	if (!Definition)
	{
		Destroy();
		return;
	}

	Area->SetSphereRadius(Definition->Radius);

	// The sphere is centred at pawn height; the sprite is laid on the floor beneath it.
	Sprite->SetRelativeLocation(FVector(0.0f, 0.0f, -GetActorLocation().Z + HazardHeight));
	Sprite->SetSpriteSheet(Definition->Sprite, 1.0f);
	Sprite->SetWorldScale3D(FVector(Definition->Radius * 2.0f / 100.0f, Definition->Radius * 2.0f / 100.0f, 1.0f));
	Sprite->SetFacingYaw(FMath::FRandRange(0.0f, 360.0f));
	Sprite->PlayAnimation(TEXT("Play"), true);

	SetLifeSpan(Definition->Lifetime);
	GetWorldTimerManager().SetTimer(ApplyTimer, this, &AZombieHazardZone::ApplyToOccupants, Definition->ApplyInterval, true, 0.1f);

	// Start fading well before it disappears so the player can read that it's going.
	FTimerHandle FadeTimer;
	GetWorldTimerManager().SetTimer(FadeTimer, FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		Sprite->FadeOut(1.5f);
	}), FMath::Max(Definition->Lifetime - 1.5f, 0.1f), false);

	if (UZombieAudioSubsystem* Audio = UZombieAudioSubsystem::Get(this))
	{
		Audio->PlaySoundAtLocation(Definition->SpawnSound, GetActorLocation());
	}
}

void AZombieHazardZone::ApplyToOccupants()
{
	if (!Definition || !Definition->StatusTag.IsValid())
	{
		return;
	}

	TArray<AActor*> Occupants;
	Area->GetOverlappingActors(Occupants, APawn::StaticClass());

	for (AActor* Occupant : Occupants)
	{
		const ZombieTeams::ETeam Team = ZombieTeams::GetTeam(Occupant);
		if (Team == ZombieTeams::ETeam::None || Team == SourceTeam)
		{
			continue;
		}

		if (UStatusEffectComponent* Status = Occupant->FindComponentByClass<UStatusEffectComponent>())
		{
			Status->ApplyStatus(Definition->StatusTag, Definition->Potency, nullptr, this);
		}
	}
}
