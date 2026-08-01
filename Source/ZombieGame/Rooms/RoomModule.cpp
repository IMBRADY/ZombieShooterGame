#include "RoomModule.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "UObject/ConstructorHelpers.h"
#include "ZombieGame.h"

namespace
{
	// The engine's basic cube is a 100-unit cube centred on its origin, so an instance scale of
	// 1.0 covers exactly 100 world units on each axis.
	constexpr float BasicCubeExtent = 100.0f;
}

ARoomModule::ARoomModule()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	// Greybox geometry: the engine cube stands in for real modular art (ARCHITECTURE.md 2).
	// Swapping in art later is a mesh assignment, not a code change.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* CubeMesh = CubeMeshFinder.Succeeded() ? CubeMeshFinder.Object : nullptr;

	const auto CreateInstancedMeshComponent = [this, CubeMesh](const TCHAR* Name) -> UInstancedStaticMeshComponent*
	{
		UInstancedStaticMeshComponent* Component = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		Component->SetupAttachment(SceneRoot);

		// Movable, even though a room never moves once built: Static mobility means "final at level
		// load", and these components are created at runtime. With Static the collision bodies for
		// instances added after registration come out half-built - whole rooms end up with visible
		// but non-solid floors - so runtime-assembled geometry has to be Movable.
		Component->SetMobility(EComponentMobility::Movable);
		Component->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		Component->SetCanEverAffectNavigation(true);
		if (CubeMesh)
		{
			Component->SetStaticMesh(CubeMesh);
		}
		return Component;
	};

	FloorMeshes = CreateInstancedMeshComponent(TEXT("FloorMeshes"));
	WallMeshes = CreateInstancedMeshComponent(TEXT("WallMeshes"));
	ObstacleMeshes = CreateInstancedMeshComponent(TEXT("ObstacleMeshes"));
}

void ARoomModule::BeginPlay()
{
	Super::BeginPlay();
}

FVector ARoomModule::GetTileCentre(const FIntPoint& Cell) const
{
	return FVector((Cell.X + 0.5f) * TileSize, (Cell.Y + 0.5f) * TileSize, 0.0f);
}

void ARoomModule::AddTileInstance(UInstancedStaticMeshComponent* Component, const FIntPoint& Cell,
	float Height, float ZCentre, float FootprintRatio)
{
	if (!Component)
	{
		return;
	}

	const FVector Centre = GetTileCentre(Cell) + FVector(0.0f, 0.0f, ZCentre);
	const float HorizontalScale = (TileSize * FootprintRatio) / BasicCubeExtent;
	const FVector Scale(HorizontalScale, HorizontalScale, Height / BasicCubeExtent);

	Component->AddInstance(FTransform(FRotator::ZeroRotator, Centre, Scale));
}

void ARoomModule::BuildFromPlacement(const FSectorRoomPlacement& Placement, float InTileSize)
{
	TileSize = FMath::Max(InTileSize, 1.0f);
	RoomType = Placement.Type;

	ZombieSpawnLocations.Reset();
	PlayerStartLocations.Reset();
	FloorMeshes->ClearInstances();
	WallMeshes->ClearInstances();
	ObstacleMeshes->ClearInstances();

	// Doorways the sector graph did not connect are sealed, so the module never opens onto the
	// void - this is what keeps "no inaccessible spaces" true once modules are assembled.
	TSet<FIntPoint> OpenDoorCells;
	for (const int32 DoorwayIndex : Placement.ConnectedDoorways)
	{
		if (Placement.Grid.Doorways.IsValidIndex(DoorwayIndex))
		{
			OpenDoorCells.Add(Placement.Grid.Doorways[DoorwayIndex].Cell);
		}
	}

	const FRoomGrid& Grid = Placement.Grid;
	for (int32 Y = 0; Y < Grid.Size.Y; ++Y)
	{
		for (int32 X = 0; X < Grid.Size.X; ++X)
		{
			const FIntPoint Cell(X, Y);
			ERoomTile Tile = Grid.GetTile(Cell);

			if (Tile == ERoomTile::Empty)
			{
				continue;
			}

			if (Tile == ERoomTile::Door)
			{
				Tile = OpenDoorCells.Contains(Cell) ? ERoomTile::Floor : ERoomTile::Wall;
			}

			if (Tile == ERoomTile::Wall)
			{
				AddTileInstance(WallMeshes, Cell, WallHeight, WallHeight * 0.5f, 1.0f);
				continue;
			}

			// Everything else is standable, so it gets a floor tile; some tiles add more on top.
			AddTileInstance(FloorMeshes, Cell, FloorThickness, -FloorThickness * 0.5f, 1.0f);

			switch (Tile)
			{
			case ERoomTile::Obstacle:
				AddTileInstance(ObstacleMeshes, Cell, ObstacleHeight, ObstacleHeight * 0.5f, ObstacleFootprintRatio);
				break;
			case ERoomTile::ZombieSpawn:
				ZombieSpawnLocations.Add(GetActorLocation() + GetTileCentre(Cell) + FVector(0.0f, 0.0f, PawnSpawnHeight));
				break;
			case ERoomTile::PlayerStart:
				PlayerStartLocations.Add(GetActorLocation() + GetTileCentre(Cell) + FVector(0.0f, 0.0f, PawnSpawnHeight));
				break;
			default:
				break;
			}
		}
	}

	const FVector Extent(Grid.Size.X * TileSize, Grid.Size.Y * TileSize, WallHeight);
	WorldBounds = FBox(GetActorLocation() - FVector(0.0f, 0.0f, FloorThickness), GetActorLocation() + Extent);

	UE_LOG(LogZombieGame, Verbose, TEXT("Room module '%s' built: %dx%d tiles, %d floor / %d wall / %d obstacle instances, %d zombie spawn points."),
		*GetName(), Grid.Size.X, Grid.Size.Y, FloorMeshes->GetInstanceCount(), WallMeshes->GetInstanceCount(),
		ObstacleMeshes->GetInstanceCount(), ZombieSpawnLocations.Num());
}

bool ARoomModule::GetPlayerStartLocation(FVector& OutLocation) const
{
	if (PlayerStartLocations.Num() == 0)
	{
		return false;
	}

	OutLocation = PlayerStartLocations[0];
	return true;
}
