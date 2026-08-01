#include "SectorGeneratorComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "Rooms/Procedural/SectorGenerationSettings.h"
#include "Rooms/Procedural/SectorLayoutBuilder.h"
#include "Rooms/RoomModule.h"
#include "Rooms/RoomTemplates/RoomTemplateDataAsset.h"
#include "Utilities/ZombiePrimaryAssetLoader.h"
#include "ZombieGame.h"

USectorGeneratorComponent::USectorGeneratorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	SettingsAsset = TSoftObjectPtr<USectorGenerationSettings>(
		FSoftObjectPath(TEXT("/Game/DataAssets/Rooms/DA_SectorGeneration.DA_SectorGeneration")));

	RoomModuleClass = ARoomModule::StaticClass();
}

USectorGenerationSettings* USectorGeneratorComponent::ResolveSettings() const
{
	if (USectorGenerationSettings* Settings = SettingsAsset.LoadSynchronous())
	{
		return Settings;
	}

	UE_LOG(LogZombieGame, Error, TEXT("Sector generation settings asset '%s' could not be loaded."),
		*SettingsAsset.ToString());
	return nullptr;
}

void USectorGeneratorComponent::GatherRoomTemplates(TArray<URoomTemplateDataAsset*>& OutTemplates) const
{
	TArray<URoomTemplateDataAsset*> Discovered;
	FZombiePrimaryAssetLoader::LoadAllOfType(URoomTemplateDataAsset::AssetType, Discovered);

	for (URoomTemplateDataAsset* Template : Discovered)
	{
		if (!Template)
		{
			continue;
		}

		if (!Template->IsUsable())
		{
			UE_LOG(LogZombieGame, Warning, TEXT("Skipping room template '%s': %s"),
				*Template->GetName(), *Template->GetLayoutError());
			continue;
		}

		OutTemplates.Add(Template);
	}

	// Stable ordering: the Asset Manager's enumeration order is not guaranteed, and a seeded
	// layout has to reproduce exactly (save/resume, and any bug report quoting a seed).
	OutTemplates.Sort([](const URoomTemplateDataAsset& Lhs, const URoomTemplateDataAsset& Rhs)
	{
		return Lhs.GetName() < Rhs.GetName();
	});
}

bool USectorGeneratorComponent::GenerateSector(int32 Sector, int32 Seed)
{
	USectorGenerationSettings* Settings = ResolveSettings();
	if (!Settings)
	{
		return false;
	}

	TArray<URoomTemplateDataAsset*> Templates;
	GatherRoomTemplates(Templates);
	if (Templates.Num() == 0)
	{
		UE_LOG(LogZombieGame, Error, TEXT("Sector %d: no usable room templates found; cannot generate."), Sector);
		return false;
	}

	FSectorLayoutParams Params;
	Params.Seed = Seed;
	Params.Sector = Sector;
	Params.TargetRoomCount = Settings->GetRoomCountForSector(Sector);
	Params.MinCombatRooms = Settings->MinCombatRooms;
	Params.Candidates.Reserve(Templates.Num());

	for (int32 Index = 0; Index < Templates.Num(); ++Index)
	{
		FSectorRoomCandidate& Candidate = Params.Candidates.AddDefaulted_GetRef();
		Candidate.Grid = &Templates[Index]->GetGrid();
		Candidate.Type = Templates[Index]->GetRoomType();
		Candidate.SelectionWeight = Templates[Index]->GetSelectionWeight();
		Candidate.MinSector = Templates[Index]->GetMinSector();
		Candidate.bAllowRotation = Templates[Index]->AllowsRotation();
		Candidate.SourceIndex = Index;
	}

	const FSectorLayout Layout = FSectorLayoutBuilder::Build(Params);
	if (Layout.IsEmpty())
	{
		UE_LOG(LogZombieGame, Error, TEXT("Sector %d: layout builder produced no valid layout for seed %d."), Sector, Seed);
		return false;
	}

	ClearSector();

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = GetOwner();

	SectorBounds = FBox(ForceInit);
	ExitRoomIndex = Layout.ExitRoomIndex;

	for (const FSectorRoomPlacement& Placement : Layout.Rooms)
	{
		const FTransform RoomTransform(FRotator::ZeroRotator,
			FVector(Placement.Origin.X * Settings->TileSize, Placement.Origin.Y * Settings->TileSize, 0.0f));

		// Deferred so the tile instances exist before the components register. Adding instances to
		// an already-registered Static-mobility ISM leaves their collision bodies half-created -
		// walls block but stretches of floor silently do not, and pawns fall through the level.
		ARoomModule* Room = World->SpawnActorDeferred<ARoomModule>(
			RoomModuleClass ? RoomModuleClass.Get() : ARoomModule::StaticClass(), RoomTransform, GetOwner(),
			nullptr, SpawnParams.SpawnCollisionHandlingOverride);

		if (!Room)
		{
			UE_LOG(LogZombieGame, Error, TEXT("Sector %d: failed to spawn room module actor."), Sector);
			ClearSector();
			return false;
		}

		Room->BuildFromPlacement(Placement, Settings->TileSize);
		Room->FinishSpawning(RoomTransform);
		SpawnedRooms.Add(Room);

		ZombieSpawnLocations.Append(Room->GetZombieSpawnLocations());
		SectorBounds += Room->GetWorldBounds();
	}

	// The start module's 'P' tile is where the run begins; AZombieGameMode returns this from
	// ChoosePlayerStart so Unreal's own restart path puts the pawn there.
	FVector PlayerStartLocation = SectorBounds.GetCenter();
	if (SpawnedRooms.IsValidIndex(Layout.StartRoomIndex))
	{
		SpawnedRooms[Layout.StartRoomIndex]->GetPlayerStartLocation(PlayerStartLocation);
	}

	GeneratedPlayerStart = World->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), PlayerStartLocation, FRotator::ZeroRotator, SpawnParams);

	UE_LOG(LogZombieGame, Log, TEXT("Sector %d generated: %d rooms, %d zombie spawn points, bounds %s."),
		Sector, SpawnedRooms.Num(), ZombieSpawnLocations.Num(), *SectorBounds.ToString());

	return true;
}

ARoomModule* USectorGeneratorComponent::GetExitRoom() const
{
	return SpawnedRooms.IsValidIndex(ExitRoomIndex) ? SpawnedRooms[ExitRoomIndex] : nullptr;
}

void USectorGeneratorComponent::ClearSector()
{
	for (ARoomModule* Room : SpawnedRooms)
	{
		if (IsValid(Room))
		{
			Room->Destroy();
		}
	}

	SpawnedRooms.Reset();
	ZombieSpawnLocations.Reset();
	SectorBounds = FBox(ForceInit);
	ExitRoomIndex = INDEX_NONE;

	if (IsValid(GeneratedPlayerStart))
	{
		GeneratedPlayerStart->Destroy();
	}
	GeneratedPlayerStart = nullptr;
}
