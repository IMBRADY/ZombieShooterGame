#include "RoomModule.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/CollisionProfile.h"
#include "Rooms/RoomTemplates/RoomThemeDataAsset.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "ZombieGame.h"

namespace
{
	// The engine's basic cube is a 100-unit cube centred on its origin, and the plane a 100-unit
	// square, so an instance scale of 1.0 covers exactly 100 world units on each axis.
	constexpr float BasicShapeExtent = 100.0f;

	/** Debris lies a hair above the floor and below blood decals. */
	constexpr float DecorationHeight = 0.6f;

	UInstancedStaticMeshComponent* CreateInstancedMeshComponent(AActor& Owner, USceneComponent* Parent, const TCHAR* Name,
		UStaticMesh* Mesh, bool bCollides)
	{
		UInstancedStaticMeshComponent* Component = Owner.CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		Component->SetupAttachment(Parent);

		// Movable, even though a room never moves once built: Static mobility means "final at level
		// load", and these components are created at runtime. With Static the collision bodies for
		// instances added after registration come out half-built - whole rooms end up with visible
		// but non-solid floors - so runtime-assembled geometry has to be Movable.
		Component->SetMobility(EComponentMobility::Movable);
		Component->SetCollisionProfileName(bCollides ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
		Component->SetCanEverAffectNavigation(bCollides);
		Component->NumCustomDataFloats = 1;
		if (Mesh)
		{
			Component->SetStaticMesh(Mesh);
		}
		return Component;
	}
}

ARoomModule::ARoomModule()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMeshFinder(TEXT("/Engine/BasicShapes/Plane.Plane"));
	UStaticMesh* CubeMesh = CubeMeshFinder.Succeeded() ? CubeMeshFinder.Object : nullptr;
	UStaticMesh* PlaneMesh = PlaneMeshFinder.Succeeded() ? PlaneMeshFinder.Object : nullptr;

	FloorMeshes = CreateInstancedMeshComponent(*this, SceneRoot, TEXT("FloorMeshes"), CubeMesh, true);
	WallMeshes = CreateInstancedMeshComponent(*this, SceneRoot, TEXT("WallMeshes"), CubeMesh, true);
	ObstacleMeshes = CreateInstancedMeshComponent(*this, SceneRoot, TEXT("ObstacleMeshes"), CubeMesh, true);
	DecorationMeshes = CreateInstancedMeshComponent(*this, SceneRoot, TEXT("DecorationMeshes"), PlaneMesh, false);
	DecorationMeshes->SetCastShadow(false);
}

void ARoomModule::BeginPlay()
{
	Super::BeginPlay();

	if (bFlickers && RoomLight)
	{
		GetWorldTimerManager().SetTimer(FlickerTimer, this, &ARoomModule::Flicker, FMath::FRandRange(0.5f, 3.0f), false);
	}
}

FVector ARoomModule::GetTileCentre(const FIntPoint& Cell) const
{
	return FVector((Cell.X + 0.5f) * TileSize, (Cell.Y + 0.5f) * TileSize, 0.0f);
}

void ARoomModule::AddTileInstance(UInstancedStaticMeshComponent* Component, const FIntPoint& Cell,
	float Height, float ZCentre, float FootprintRatio, float Variant, float YawDegrees)
{
	if (!Component)
	{
		return;
	}

	const FVector Centre = GetTileCentre(Cell) + FVector(0.0f, 0.0f, ZCentre);
	const float HorizontalScale = (TileSize * FootprintRatio) / BasicShapeExtent;
	const FVector Scale(HorizontalScale, HorizontalScale, Height / BasicShapeExtent);

	const int32 Index = Component->AddInstance(FTransform(FRotator(0.0f, YawDegrees, 0.0f), Centre, Scale));
	if (Variant >= 0.0f)
	{
		Component->SetCustomDataValue(Index, 0, Variant, false);
	}
}

void ARoomModule::ApplyTheme(const URoomThemeDataAsset* Theme)
{
	if (!Theme)
	{
		return;
	}

	FloorMeshes->SetMaterial(0, Theme->FloorMaterial);
	WallMeshes->SetMaterial(0, Theme->WallMaterial);
	ObstacleMeshes->SetMaterial(0, Theme->ObstacleMaterial);
	DecorationMeshes->SetMaterial(0, Theme->DecorationMaterial);
}

void ARoomModule::RecordSealedDoorway(const FRoomGrid& Grid, int32 DoorwayIndex)
{
	const FRoomDoorway& Doorway = Grid.Doorways[DoorwayIndex];
	const FIntPoint Outward = RoomDirection::ToOffset(Doorway.Direction);
	const FVector Inward(-Outward.X, -Outward.Y, 0.0f);

	const FVector Location = GetActorLocation() + GetTileCentre(Doorway.Cell);
	SealedDoorways.Add(FTransform(Inward.Rotation(), Location));
}

void ARoomModule::BuildFromPlacement(const FSectorRoomPlacement& Placement, float InTileSize, const URoomThemeDataAsset* Theme, int32 DecorationSeed)
{
	TileSize = FMath::Max(InTileSize, 1.0f);
	RoomType = Placement.Type;
	FRandomStream Random(DecorationSeed);

	ZombieSpawnLocations.Reset();
	PlayerStartLocations.Reset();
	SealedDoorways.Reset();
	ApplyTheme(Theme);

	const FRoomGrid& Grid = Placement.Grid;

	// Doorways the sector graph did not connect are sealed, so the module never opens onto the
	// void - this is what keeps "no inaccessible spaces" true once modules are assembled.
	TSet<FIntPoint> OpenDoorCells;
	for (int32 DoorwayIndex = 0; DoorwayIndex < Grid.Doorways.Num(); ++DoorwayIndex)
	{
		if (Placement.ConnectedDoorways.Contains(DoorwayIndex))
		{
			OpenDoorCells.Add(Grid.Doorways[DoorwayIndex].Cell);
		}
		else
		{
			RecordSealedDoorway(Grid, DoorwayIndex);
		}
	}

	const int32 ObstacleVariants = Theme ? FMath::Max(Theme->ObstacleVariants, 1) : 1;
	const int32 DecorationVariants = Theme ? FMath::Max(Theme->DecorationVariants, 1) : 1;
	const float DecorationChance = Theme ? Theme->DecorationChance : 0.0f;
	const FVector2D GridCentre(Grid.Size.X * 0.5f, Grid.Size.Y * 0.5f);
	float BestCentreDistance = TNumericLimits<float>::Max();

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
			const FVector TileWorld = GetActorLocation() + GetTileCentre(Cell) + FVector(0.0f, 0.0f, PawnSpawnHeight);

			switch (Tile)
			{
			case ERoomTile::Obstacle:
				AddTileInstance(ObstacleMeshes, Cell, ObstacleHeight, ObstacleHeight * 0.5f, ObstacleFootprintRatio,
					static_cast<float>(Random.RandRange(0, ObstacleVariants - 1)), 90.0f * Random.RandRange(0, 3));
				break;
			case ERoomTile::ZombieSpawn:
				ZombieSpawnLocations.Add(TileWorld);
				break;
			case ERoomTile::PlayerStart:
				PlayerStartLocations.Add(TileWorld);
				break;
			case ERoomTile::Floor:
				if (Random.FRand() < DecorationChance)
				{
					AddTileInstance(DecorationMeshes, Cell, 1.0f, DecorationHeight, Random.FRandRange(0.5f, 0.9f),
						static_cast<float>(Random.RandRange(0, DecorationVariants - 1)), Random.FRandRange(0.0f, 360.0f));
				}
				if (FVector2D::DistSquared(FVector2D(X + 0.5f, Y + 0.5f), GridCentre) < BestCentreDistance)
				{
					BestCentreDistance = FVector2D::DistSquared(FVector2D(X + 0.5f, Y + 0.5f), GridCentre);
					CentreLocation = TileWorld;
				}
				break;
			default:
				break;
			}
		}
	}

	const FVector Extent(Grid.Size.X * TileSize, Grid.Size.Y * TileSize, WallHeight);
	WorldBounds = FBox(GetActorLocation() - FVector(0.0f, 0.0f, FloorThickness), GetActorLocation() + Extent);

	// Arenas and landmarks get a lamp; corridors stay dark, which is most of the building's mood.
	if (Theme && RoomType != ERoomType::Hallway && RoomType != ERoomType::DeadEnd)
	{
		AddLight(*Theme, Random);
	}
}

void ARoomModule::AddLight(const URoomThemeDataAsset& Theme, FRandomStream& Random)
{
	RoomLight = NewObject<UPointLightComponent>(this, TEXT("RoomLight"));
	RoomLight->SetupAttachment(SceneRoot);
	RoomLight->SetMobility(EComponentMobility::Movable);
	RoomLight->SetWorldLocation(FVector(CentreLocation.X, CentreLocation.Y, WallHeight * 0.85f));
	RoomLight->SetLightColor(Theme.LightColor);
	RoomLight->SetIntensity(Theme.LightIntensity);
	RoomLight->SetAttenuationRadius(Theme.LightRadius);
	RoomLight->SetCastShadows(false);
	AddInstanceComponent(RoomLight);

	BaseLightIntensity = Theme.LightIntensity;
	bFlickers = Random.FRand() < Theme.FlickerChance;
}

void ARoomModule::Flicker()
{
	if (!RoomLight)
	{
		return;
	}

	// Mostly on, occasionally stuttering off - a failing tube, not a disco.
	const bool bDip = RoomLight->Intensity >= BaseLightIntensity * 0.9f && FMath::FRand() < 0.6f;
	RoomLight->SetIntensity(bDip ? BaseLightIntensity * FMath::FRandRange(0.05f, 0.4f) : BaseLightIntensity);
	GetWorldTimerManager().SetTimer(FlickerTimer, this, &ARoomModule::Flicker,
		bDip ? FMath::FRandRange(0.04f, 0.18f) : FMath::FRandRange(0.6f, 4.0f), false);
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
