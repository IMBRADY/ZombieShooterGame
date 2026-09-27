#include "SectorGeneratorComponent.h"
#include "AI/ZombieFlowFieldSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "Rooms/Procedural/SectorGenerationSettings.h"
#include "Rooms/Procedural/SectorNavigationGrid.h"
#include "Rooms/RoomModule.h"
#include "Rooms/RoomTemplates/RoomTemplateDataAsset.h"
#include "Rooms/RoomTemplates/RoomThemeDataAsset.h"
#include "Rooms/ZombieLevelProp.h"
#include "Utilities/ZombiePrimaryAssetLoader.h"
#include "ZombieGame.h"

USectorGeneratorComponent::USectorGeneratorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	SettingsAsset = TSoftObjectPtr<USectorGenerationSettings>(
		FSoftObjectPath(TEXT("/Game/DataAssets/Rooms/DA_SectorGeneration.DA_SectorGeneration")));

	RoomModuleClass = ARoomModule::StaticClass();
	ExitDoorClass = AZombieExitDoor::StaticClass();
	TreasureChestClass = AZombieTreasureChest::StaticClass();
	ShopTerminalClass = AZombieShopTerminal::StaticClass();
}

USectorGenerationSettings* USectorGeneratorComponent::ResolveSettings() const
{
	if (USectorGenerationSettings* Settings = SettingsAsset.LoadSynchronous())
	{
		return Settings;
	}

	UE_LOG(LogZombieGame, Error, TEXT("Sector generation settings asset '%s' could not be loaded."), *SettingsAsset.ToString());
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
			UE_LOG(LogZombieGame, Warning, TEXT("Skipping room template '%s': %s"), *Template->GetName(), *Template->GetLayoutError());
			continue;
		}
		OutTemplates.Add(Template);
	}

	// Stable ordering: the Asset Manager's enumeration order is not guaranteed, and a seeded
	// layout has to reproduce exactly (save/resume, and any bug report quoting a seed).
	OutTemplates.Sort([](const URoomTemplateDataAsset& Lhs, const URoomTemplateDataAsset& Rhs) { return Lhs.GetName() < Rhs.GetName(); });
}

const URoomThemeDataAsset* USectorGeneratorComponent::PickTheme(int32 Sector, FRandomStream& Random) const
{
	TArray<URoomThemeDataAsset*> Themes;
	FZombiePrimaryAssetLoader::LoadAllOfType(URoomThemeDataAsset::AssetType, Themes);
	Themes.RemoveAll([Sector](const URoomThemeDataAsset* Theme) { return !Theme || Theme->MinSector > Sector || Theme->SelectionWeight <= 0.0f; });
	Themes.Sort([](const URoomThemeDataAsset& Lhs, const URoomThemeDataAsset& Rhs) { return Lhs.GetName() < Rhs.GetName(); });

	float Total = 0.0f;
	for (const URoomThemeDataAsset* Theme : Themes)
	{
		Total += Theme->SelectionWeight;
	}

	float Roll = Random.FRandRange(0.0f, Total);
	for (const URoomThemeDataAsset* Theme : Themes)
	{
		Roll -= Theme->SelectionWeight;
		if (Roll <= 0.0f)
		{
			return Theme;
		}
	}
	return Themes.Num() > 0 ? Themes.Last() : nullptr;
}

bool USectorGeneratorComponent::GenerateSector(int32 Sector, int32 Seed, bool bBossSector)
{
	USectorGenerationSettings* Settings = ResolveSettings();
	TArray<URoomTemplateDataAsset*> Templates;
	GatherRoomTemplates(Templates);
	if (!Settings || Templates.Num() == 0)
	{
		UE_LOG(LogZombieGame, Error, TEXT("Sector %d: no settings or usable room templates; cannot generate."), Sector);
		return false;
	}

	FSectorLayoutParams Params;
	Params.Seed = Seed;
	Params.Sector = Sector;
	Params.TargetRoomCount = Settings->GetRoomCountForSector(Sector);
	Params.MinCombatRooms = Settings->MinCombatRooms;
	Params.bRequireBossRoom = bBossSector;

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

	FRandomStream ThemeRandom(Seed ^ 0x5eed);
	if (!SpawnRooms(Layout, *Settings, PickTheme(Sector, ThemeRandom)))
	{
		return false;
	}

	PlacePlayerStart(Layout);
	PlaceSectorFixtures(Layout, Sector);
	PublishNavigationGrid(Layout, Settings->TileSize);

	UE_LOG(LogZombieGame, Log, TEXT("Sector %d generated: %d rooms, %d zombie spawn points, boss arena %s, bounds %s."),
		Sector, SpawnedRooms.Num(), ZombieSpawnLocations.Num(), BossTrigger ? TEXT("yes") : TEXT("no"), *SectorBounds.ToString());
	return true;
}

bool USectorGeneratorComponent::GenerateIntermission(int32 Sector, int32 Seed)
{
	USectorGenerationSettings* Settings = ResolveSettings();
	TArray<URoomTemplateDataAsset*> Templates;
	GatherRoomTemplates(Templates);

	const int32 ShopIndex = Templates.IndexOfByPredicate([](const URoomTemplateDataAsset* Template)
	{
		return Template->GetRoomType() == ERoomType::Intermission;
	});
	if (!Settings || ShopIndex == INDEX_NONE)
	{
		UE_LOG(LogZombieGame, Error, TEXT("No intermission room template found; cannot build the shop."));
		return false;
	}

	FSectorRoomCandidate Candidate;
	Candidate.Grid = &Templates[ShopIndex]->GetGrid();
	Candidate.Type = ERoomType::Intermission;
	Candidate.SourceIndex = ShopIndex;

	const FSectorLayout Layout = FSectorLayoutBuilder::BuildSingleRoom(Candidate, Seed);
	FRandomStream ThemeRandom(Seed);
	if (Layout.IsEmpty() || !SpawnRooms(Layout, *Settings, PickTheme(Sector, ThemeRandom)))
	{
		return false;
	}

	PlacePlayerStart(Layout);
	PublishNavigationGrid(Layout, Settings->TileSize);

	const ARoomModule* Room = SpawnedRooms[0];
	SpawnFixture<AZombieShopTerminal>(ShopTerminalClass, FTransform(Room->GetCentreLocation()));
	if ((ExitDoor = SpawnExitDoorIn(*Room)) != nullptr)
	{
		ExitDoor->SetLeadsToNextSector(true);
		ExitDoor->SetUnlocked(true);
	}
	return true;
}

bool USectorGeneratorComponent::SpawnRooms(const FSectorLayout& Layout, const USectorGenerationSettings& Settings, const URoomThemeDataAsset* Theme)
{
	ClearSector();

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	for (int32 RoomIndex = 0; RoomIndex < Layout.Rooms.Num(); ++RoomIndex)
	{
		const FSectorRoomPlacement& Placement = Layout.Rooms[RoomIndex];
		const FTransform RoomTransform(FRotator::ZeroRotator,
			FVector(Placement.Origin.X * Settings.TileSize, Placement.Origin.Y * Settings.TileSize, 0.0f));

		// Deferred so the tile instances exist before the components register. Adding instances to
		// an already-registered ISM leaves collision bodies half-created (CLAUDE.md gotcha #6).
		ARoomModule* Room = World->SpawnActorDeferred<ARoomModule>(RoomModuleClass ? RoomModuleClass.Get() : ARoomModule::StaticClass(),
			RoomTransform, GetOwner(), nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Room)
		{
			UE_LOG(LogZombieGame, Error, TEXT("Failed to spawn room module actor."));
			ClearSector();
			return false;
		}

		Room->BuildFromPlacement(Placement, Settings.TileSize, Theme, HashCombine(GetTypeHash(Layout.Seed), GetTypeHash(RoomIndex)));
		Room->FinishSpawning(RoomTransform);
		SpawnedRooms.Add(Room);

		ZombieSpawnLocations.Append(Room->GetZombieSpawnLocations());
		SectorBounds += Room->GetWorldBounds();

		if (Room->GetZombieSpawnLocations().Num() > 0)
		{
			FZombieSpawnArea& Area = ZombieSpawnAreas.AddDefaulted_GetRef();
			Area.Bounds = Room->GetWorldBounds();
			Area.SpawnPoints = Room->GetZombieSpawnLocations();
		}
	}
	return true;
}

void USectorGeneratorComponent::PlacePlayerStart(const FSectorLayout& Layout)
{
	// The start module's 'P' tile is where the run begins; AZombieGameMode returns this from
	// ChoosePlayerStart so Unreal's own restart path puts the pawn there.
	FVector PlayerStartLocation = SectorBounds.GetCenter();
	if (SpawnedRooms.IsValidIndex(Layout.StartRoomIndex) && !SpawnedRooms[Layout.StartRoomIndex]->GetPlayerStartLocation(PlayerStartLocation))
	{
		PlayerStartLocation = SpawnedRooms[Layout.StartRoomIndex]->GetCentreLocation();
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = GetOwner();
	GeneratedPlayerStart = GetWorld()->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), PlayerStartLocation, FRotator::ZeroRotator, SpawnParams);
}

void USectorGeneratorComponent::PlaceSectorFixtures(const FSectorLayout& Layout, int32 Sector)
{
	for (int32 RoomIndex = 0; RoomIndex < SpawnedRooms.Num(); ++RoomIndex)
	{
		const ARoomModule* Room = SpawnedRooms[RoomIndex];
		if (Room->GetRoomType() == ERoomType::Treasure)
		{
			if (AZombieTreasureChest* Chest = SpawnFixture<AZombieTreasureChest>(TreasureChestClass, FTransform(Room->GetCentreLocation())))
			{
				Chest->SetSector(Sector);
			}
		}
	}

	if (SpawnedRooms.IsValidIndex(Layout.ExitRoomIndex))
	{
		const ARoomModule* ExitRoom = SpawnedRooms[Layout.ExitRoomIndex];
		ExitDoor = SpawnExitDoorIn(*ExitRoom);
		BossSpawnLocation = ExitRoom->GetCentreLocation();
	}

	if (SpawnedRooms.IsValidIndex(Layout.BossRoomIndex))
	{
		const ARoomModule* Arena = SpawnedRooms[Layout.BossRoomIndex];
		BossSpawnLocation = Arena->GetCentreLocation();
		BossTrigger = SpawnFixture<AZombieBossArenaTrigger>(AZombieBossArenaTrigger::StaticClass(), FTransform(Arena->GetWorldBounds().GetCenter()));
		if (BossTrigger)
		{
			BossTrigger->SetArenaBounds(Arena->GetWorldBounds());
		}
	}
}

AZombieExitDoor* USectorGeneratorComponent::SpawnExitDoorIn(const ARoomModule& Room)
{
	// A sealed doorway makes a natural exit; a room with every doorway connected gets a floor hatch
	// in its middle instead.
	const TArray<FTransform>& Sealed = Room.GetSealedDoorways();
	const FTransform DoorTransform = Sealed.Num() > 0 ? Sealed[0] : FTransform(Room.GetCentreLocation());
	return SpawnFixture<AZombieExitDoor>(ExitDoorClass, DoorTransform);
}

template <typename TActor>
TActor* USectorGeneratorComponent::SpawnFixture(TSubclassOf<TActor> Class, const FTransform& Transform)
{
	UWorld* World = GetWorld();
	if (!World || !Class)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = GetOwner();
	TActor* Fixture = World->SpawnActor<TActor>(Class, Transform, SpawnParams);
	if (Fixture)
	{
		SpawnedFixtures.Add(Fixture);
	}
	return Fixture;
}

void USectorGeneratorComponent::PublishNavigationGrid(const FSectorLayout& Layout, float TileSize) const
{
	// Hand the horde its shared pathing data. Built from the layout rather than read back out of
	// the navmesh: the layout is already an exact tile map of what was just spawned.
	if (UZombieFlowFieldSubsystem* FlowField = GetWorld()->GetSubsystem<UZombieFlowFieldSubsystem>())
	{
		FSectorNavigationGrid NavigationGrid;
		NavigationGrid.BuildFromLayout(Layout, TileSize);
		FlowField->SetSectorGrid(NavigationGrid);
	}
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
	for (AActor* Fixture : SpawnedFixtures)
	{
		if (IsValid(Fixture))
		{
			Fixture->Destroy();
		}
	}

	SpawnedRooms.Reset();
	SpawnedFixtures.Reset();
	ZombieSpawnLocations.Reset();
	ZombieSpawnAreas.Reset();
	SectorBounds = FBox(ForceInit);
	ExitDoor = nullptr;
	BossTrigger = nullptr;

	if (UWorld* World = GetWorld())
	{
		if (UZombieFlowFieldSubsystem* FlowField = World->GetSubsystem<UZombieFlowFieldSubsystem>())
		{
			FlowField->ClearSectorGrid();
		}
	}

	if (IsValid(GeneratedPlayerStart))
	{
		GeneratedPlayerStart->Destroy();
	}
	GeneratedPlayerStart = nullptr;
}
