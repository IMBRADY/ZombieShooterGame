#pragma once

#include "CoreMinimal.h"
#include "RoomTypes.generated.h"

// What a room module is for. Drives selection quotas in the layout builder and, later, which
// loot/encounter rules apply to the room.
UENUM(BlueprintType)
enum class ERoomType : uint8
{
	Start		UMETA(DisplayName = "Start Room"),
	Combat		UMETA(DisplayName = "Combat Room"),
	Hallway		UMETA(DisplayName = "Hallway"),
	DeadEnd		UMETA(DisplayName = "Dead End"),
	Treasure	UMETA(DisplayName = "Treasure Room"),
	Boss		UMETA(DisplayName = "Boss Room")
};

// One character of a handcrafted room layout. The generator never invents these - a designer
// authors the tile grid in a Room Template Data Asset; the generator only decides which
// handcrafted module goes where (see ARCHITECTURE.md 7: no procedural geometry).
UENUM(BlueprintType)
enum class ERoomTile : uint8
{
	Empty			UMETA(DisplayName = "Empty (outside the module)"),
	Wall			UMETA(DisplayName = "Wall"),
	Floor			UMETA(DisplayName = "Floor"),
	Door			UMETA(DisplayName = "Doorway"),
	Obstacle		UMETA(DisplayName = "Obstacle / cover"),
	ZombieSpawn		UMETA(DisplayName = "Zombie spawn point"),
	PlayerStart		UMETA(DisplayName = "Player start")
};

// Grid axes: +X is a column step (world +X), +Y is a row step (world +Y). Row 0 of a layout is
// the lowest Y, so a layout reads top-to-bottom as increasing Y.
UENUM(BlueprintType)
enum class ERoomDirection : uint8
{
	North	UMETA(DisplayName = "North (-Y)"),
	East	UMETA(DisplayName = "East (+X)"),
	South	UMETA(DisplayName = "South (+Y)"),
	West	UMETA(DisplayName = "West (-X)")
};

// A doorway on a module's boundary, in the module's own (unrotated or rotated) cell space.
USTRUCT(BlueprintType)
struct FRoomDoorway
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, Category = "Room")
	FIntPoint Cell = FIntPoint::ZeroValue;

	UPROPERTY(VisibleAnywhere, Category = "Room")
	ERoomDirection Direction = ERoomDirection::North;

	FRoomDoorway() = default;
	FRoomDoorway(const FIntPoint& InCell, ERoomDirection InDirection)
		: Cell(InCell), Direction(InDirection) {}
};

namespace RoomDirection
{
	/** Cell offset taken by stepping one tile in the given direction. */
	inline FIntPoint ToOffset(ERoomDirection Direction)
	{
		switch (Direction)
		{
		case ERoomDirection::North:	return FIntPoint(0, -1);
		case ERoomDirection::East:	return FIntPoint(1, 0);
		case ERoomDirection::South:	return FIntPoint(0, 1);
		default:					return FIntPoint(-1, 0);
		}
	}

	inline ERoomDirection Opposite(ERoomDirection Direction)
	{
		switch (Direction)
		{
		case ERoomDirection::North:	return ERoomDirection::South;
		case ERoomDirection::East:	return ERoomDirection::West;
		case ERoomDirection::South:	return ERoomDirection::North;
		default:					return ERoomDirection::East;
		}
	}

	/** Rotates a direction by the given number of clockwise quarter turns. */
	inline ERoomDirection Rotate(ERoomDirection Direction, int32 QuarterTurns)
	{
		const int32 Rotated = (static_cast<int32>(Direction) + QuarterTurns) % 4;
		return static_cast<ERoomDirection>((Rotated + 4) % 4);
	}
}

namespace RoomTile
{
	/** Character used for this tile in a Room Template Data Asset's layout rows. */
	ZOMBIEGAME_API TCHAR ToChar(ERoomTile Tile);

	/** Parses a layout character. Returns false for characters with no tile mapping. */
	ZOMBIEGAME_API bool FromChar(TCHAR Character, ERoomTile& OutTile);

	/** True for tiles an actor can stand on (everything inside the room except walls/emptiness). */
	inline bool IsWalkable(ERoomTile Tile)
	{
		return Tile == ERoomTile::Floor
			|| Tile == ERoomTile::Door
			|| Tile == ERoomTile::ZombieSpawn
			|| Tile == ERoomTile::PlayerStart;
	}

	/** True for tiles that physically occupy a cell, i.e. that must not overlap another module. */
	inline bool IsSolidFootprint(ERoomTile Tile)
	{
		return Tile != ERoomTile::Empty;
	}
}
