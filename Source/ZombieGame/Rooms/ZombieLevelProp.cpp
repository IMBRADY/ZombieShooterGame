#include "ZombieLevelProp.h"
#include "Audio/ZombieAudioSubsystem.h"
#include "Characters/Player/ZombiePlayerController.h"
#include "Components/BoxComponent.h"
#include "Core/ZombieRunController.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Loot/LootPickup.h"
#include "Loot/LootResolver.h"
#include "Loot/LootTableDataAsset.h"
#include "Visual/PixelSpriteComponent.h"
#include "Visual/SpriteSheetDataAsset.h"

#define LOCTEXT_NAMESPACE "ZombieProps"

namespace
{
	IZombieRunController* GetRunController(const UObject* WorldContext)
	{
		return Cast<IZombieRunController>(UGameplayStatics::GetGameMode(WorldContext));
	}
}

// --- Base ---------------------------------------------------------------------------------------

AZombieLevelProp::AZombieLevelProp()
{
	PrimaryActorTick.bCanEverTick = false;

	InteractionVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionVolume"));
	InteractionVolume->SetBoxExtent(FVector(90.0f, 90.0f, 120.0f));
	InteractionVolume->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionVolume->SetCanEverAffectNavigation(false);
	SetRootComponent(InteractionVolume);

	Sprite = CreateDefaultSubobject<UPixelSpriteComponent>(TEXT("Sprite"));
	Sprite->SetupAttachment(InteractionVolume);

	PropSheet = TSoftObjectPtr<USpriteSheetDataAsset>(FSoftObjectPath(TEXT("/Game/Sprites/Props/SS_Props.SS_Props")));
}

void AZombieLevelProp::BeginPlay()
{
	Super::BeginPlay();
	Sprite->SetSpriteSheet(PropSheet.LoadSynchronous());
}

void AZombieLevelProp::ShowSprite(FName Animation, float Scale)
{
	Sprite->SetSpriteSheet(PropSheet.LoadSynchronous(), Scale);
	Sprite->SetRelativeLocation(FVector(0.0f, 0.0f, SpriteHeight - GetActorLocation().Z));
	Sprite->PlayAnimation(Animation, true);
}

// --- Exit door ----------------------------------------------------------------------------------

AZombieExitDoor::AZombieExitDoor()
{
	// The door is drawn on top of the sealed doorway's wall block.
	SpriteHeight = 322.0f;
	InteractionVolume->SetBoxExtent(FVector(160.0f, 160.0f, 150.0f));
}

void AZombieExitDoor::BeginPlay()
{
	Super::BeginPlay();

	FloorMarker = NewObject<UPixelSpriteComponent>(this, TEXT("FloorMarker"));
	FloorMarker->SetupAttachment(InteractionVolume);
	FloorMarker->RegisterComponent();
	FloorMarker->SetSpriteSheet(PropSheet.LoadSynchronous(), 1.2f);
	FloorMarker->SetRelativeLocation(FVector(140.0f, 0.0f, 4.0f - GetActorLocation().Z));

	SetUnlocked(bUnlocked);
}

void AZombieExitDoor::SetUnlocked(bool bInUnlocked)
{
	bUnlocked = bInUnlocked;
	ShowSprite(bUnlocked ? TEXT("DoorOpen") : TEXT("DoorLocked"), 1.6f);

	if (FloorMarker)
	{
		FloorMarker->SetVisibility(bUnlocked);
		FloorMarker->PlayAnimation(TEXT("ExitMarker"), true);
	}
}

FText AZombieExitDoor::GetInteractionPrompt_Implementation(AActor* Interactor) const
{
	return bLeadsToNextSector ? LOCTEXT("NextSector", "Leave for the next sector") : LOCTEXT("Exit", "Exit the sector");
}

bool AZombieExitDoor::CanInteract_Implementation(AActor* Interactor) const
{
	return bUnlocked;
}

void AZombieExitDoor::Interact_Implementation(AActor* Interactor)
{
	IZombieRunController* Run = GetRunController(this);
	APawn* Pawn = Cast<APawn>(Interactor);
	if (!Run || !bUnlocked)
	{
		return;
	}

	if (UZombieAudioSubsystem* Audio = UZombieAudioSubsystem::Get(this))
	{
		Audio->PlayNamedSoundAtLocation(TEXT("Door.Open"), GetActorLocation());
	}

	if (bLeadsToNextSector)
	{
		Run->NotifyIntermissionLeft(Pawn);
	}
	else
	{
		Run->NotifyExitUsed(Pawn);
	}
}

// --- Shop terminal ------------------------------------------------------------------------------

void AZombieShopTerminal::BeginPlay()
{
	Super::BeginPlay();
	ShowSprite(TEXT("Terminal"), 1.5f);
}

FText AZombieShopTerminal::GetInteractionPrompt_Implementation(AActor* Interactor) const
{
	return LOCTEXT("OpenShop", "Browse the market");
}

void AZombieShopTerminal::Interact_Implementation(AActor* Interactor)
{
	const APawn* Pawn = Cast<APawn>(Interactor);
	if (AZombiePlayerController* Controller = Pawn ? Cast<AZombiePlayerController>(Pawn->GetController()) : nullptr)
	{
		Controller->ClientOpenShop();
	}
}

// --- Treasure chest -----------------------------------------------------------------------------

AZombieTreasureChest::AZombieTreasureChest()
{
	LootTable = TSoftObjectPtr<ULootTableDataAsset>(FSoftObjectPath(TEXT("/Game/DataAssets/Loot/LT_TreasureChest.LT_TreasureChest")));
}

void AZombieTreasureChest::BeginPlay()
{
	Super::BeginPlay();
	ShowSprite(TEXT("ChestClosed"), 1.1f);
}

FText AZombieTreasureChest::GetInteractionPrompt_Implementation(AActor* Interactor) const
{
	return LOCTEXT("OpenChest", "Open the chest");
}

void AZombieTreasureChest::Interact_Implementation(AActor* Interactor)
{
	const ULootTableDataAsset* Table = LootTable.LoadSynchronous();
	if (bOpened || !Table || !HasAuthority())
	{
		return;
	}

	bOpened = true;
	ShowSprite(TEXT("ChestOpen"), 1.1f);

	FRandomStream Random(FMath::Rand());
	TArray<FResolvedLoot> Loot;
	FLootResolver::Roll(*Table, Sector, 1.0f, Random, Loot);
	for (const FResolvedLoot& Item : Loot)
	{
		ALootPickup::SpawnLoot(GetWorld(), Item, GetActorLocation() + FVector(FMath::FRandRange(60.0f, 120.0f), 0.0f, 0.0f).RotateAngleAxis(FMath::FRandRange(0.0f, 360.0f), FVector::UpVector));
	}

	if (UZombieAudioSubsystem* Audio = UZombieAudioSubsystem::Get(this))
	{
		Audio->PlayNamedSoundAtLocation(TEXT("Chest.Open"), GetActorLocation());
	}
}

// --- Boss arena trigger -------------------------------------------------------------------------

AZombieBossArenaTrigger::AZombieBossArenaTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	Volume = CreateDefaultSubobject<UBoxComponent>(TEXT("Volume"));
	Volume->SetCollisionObjectType(ECC_WorldDynamic);
	Volume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Volume->SetCollisionResponseToAllChannels(ECR_Ignore);
	Volume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Volume->SetGenerateOverlapEvents(true);
	Volume->SetCanEverAffectNavigation(false);
	SetRootComponent(Volume);
}

void AZombieBossArenaTrigger::BeginPlay()
{
	Super::BeginPlay();
	Volume->OnComponentBeginOverlap.AddDynamic(this, &AZombieBossArenaTrigger::HandleOverlap);
}

void AZombieBossArenaTrigger::SetArenaBounds(const FBox& Bounds)
{
	// Inset by a tile so walking past the arena's doorway doesn't count as entering it.
	const FVector Extent = (Bounds.GetExtent() - FVector(250.0f, 250.0f, 0.0f)).ComponentMax(FVector(100.0f));
	SetActorLocation(Bounds.GetCenter());
	Volume->SetBoxExtent(Extent);
}

void AZombieBossArenaTrigger::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (bTriggered || !Pawn || !Pawn->IsPlayerControlled())
	{
		return;
	}

	bTriggered = true;
	if (IZombieRunController* Run = GetRunController(this))
	{
		Run->NotifyBossRoomEntered(Pawn);
	}
}

#undef LOCTEXT_NAMESPACE
