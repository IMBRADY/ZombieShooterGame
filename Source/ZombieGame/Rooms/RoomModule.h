#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Rooms/Procedural/SectorLayoutBuilder.h"
#include "RoomModule.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMesh;

/**
 * The in-world realisation of one handcrafted room module.
 *
 * It only stamps out the tiles a designer authored in the module's Room Template Data Asset - it
 * never invents layout. Repeated floor/wall/obstacle pieces go through Instanced Static Mesh
 * components so a whole sector costs a handful of draw calls (ARCHITECTURE.md 8).
 */
UCLASS()
class ZOMBIEGAME_API ARoomModule : public AActor
{
	GENERATED_BODY()

public:
	ARoomModule();

	/**
	 * Builds this module's geometry from a placement produced by FSectorLayoutBuilder. Doorways
	 * the layout actually connected are left open; the rest are sealed back up as wall.
	 */
	void BuildFromPlacement(const FSectorRoomPlacement& Placement, float InTileSize);

	/** World-space points a zombie may be spawned at, from the module's 'S' tiles. */
	const TArray<FVector>& GetZombieSpawnLocations() const { return ZombieSpawnLocations; }

	/** World-space point the player starts at, from the module's 'P' tile. */
	bool GetPlayerStartLocation(FVector& OutLocation) const;

	/** World-space bounds of the module's footprint, including wall thickness. */
	FBox GetWorldBounds() const { return WorldBounds; }

	ERoomType GetRoomType() const { return RoomType; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, Category = "Room")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Room")
	TObjectPtr<UInstancedStaticMeshComponent> FloorMeshes;

	UPROPERTY(VisibleAnywhere, Category = "Room")
	TObjectPtr<UInstancedStaticMeshComponent> WallMeshes;

	UPROPERTY(VisibleAnywhere, Category = "Room")
	TObjectPtr<UInstancedStaticMeshComponent> ObstacleMeshes;

	/** Height of a wall tile. Also the ceiling clearance the top-down camera looks past. */
	UPROPERTY(EditDefaultsOnly, Category = "Room")
	float WallHeight = 320.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Room")
	float FloorThickness = 24.0f;

	/** Obstacles are waist-high cover, not full walls - they block movement, not sight lines. */
	UPROPERTY(EditDefaultsOnly, Category = "Room")
	float ObstacleHeight = 120.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Room")
	float ObstacleFootprintRatio = 0.7f;

	/** Vertical offset applied to spawn/start points so pawns are placed above the floor. */
	UPROPERTY(EditDefaultsOnly, Category = "Room")
	float PawnSpawnHeight = 100.0f;

private:
	void AddTileInstance(UInstancedStaticMeshComponent* Component, const FIntPoint& Cell, float Height, float ZCentre, float FootprintRatio);

	FVector GetTileCentre(const FIntPoint& Cell) const;

	TArray<FVector> ZombieSpawnLocations;
	TArray<FVector> PlayerStartLocations;

	FBox WorldBounds = FBox(ForceInit);
	float TileSize = 250.0f;
	ERoomType RoomType = ERoomType::Combat;
};
