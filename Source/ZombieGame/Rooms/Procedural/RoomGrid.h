#pragma once

#include "CoreMinimal.h"
#include "Rooms/RoomTypes.h"

/**
 * A parsed handcrafted room module layout: the tile grid plus everything derivable from it
 * (doorways, spawn cells, obstacles).
 *
 * Deliberately a plain C++ struct with no UObject dependency so the parsing/validation rules and
 * the layout builder that consumes them are testable without loading assets or a level
 * (ARCHITECTURE.md 17).
 */
struct ZOMBIEGAME_API FRoomGrid
{
	/** Column/row extents. Size.X = columns, Size.Y = rows. */
	FIntPoint Size = FIntPoint::ZeroValue;

	/** Row-major: Tiles[Y * Size.X + X]. */
	TArray<ERoomTile> Tiles;

	TArray<FRoomDoorway> Doorways;
	TArray<FIntPoint> ZombieSpawnCells;
	TArray<FIntPoint> PlayerStartCells;
	TArray<FIntPoint> ObstacleCells;

	/**
	 * Parses a handcrafted layout, one string per row, and derives doorways/spawn cells.
	 * Returns false with a designer-readable reason if the layout is malformed.
	 */
	bool ParseFrom(const TArray<FString>& Rows, FString& OutError);

	/** Returns this grid rotated clockwise by the given number of quarter turns. */
	FRoomGrid Rotated(int32 QuarterTurns) const;

	bool IsInside(const FIntPoint& Cell) const
	{
		return Cell.X >= 0 && Cell.Y >= 0 && Cell.X < Size.X && Cell.Y < Size.Y;
	}

	ERoomTile GetTile(const FIntPoint& Cell) const
	{
		return IsInside(Cell) ? Tiles[Cell.Y * Size.X + Cell.X] : ERoomTile::Empty;
	}

	bool IsWalkable(const FIntPoint& Cell) const { return RoomTile::IsWalkable(GetTile(Cell)); }

	bool IsEmpty() const { return Tiles.Num() == 0; }

	/**
	 * Checks the rules a module must satisfy to be usable: it has at least one doorway, every
	 * doorway sits on the module boundary facing outward, and every walkable cell is reachable
	 * from every other one (no sealed-off pockets, so "no inaccessible spaces" holds per room
	 * before the sector graph is even assembled).
	 */
	bool Validate(FString& OutError) const;

private:
	/** Repopulates Doorways/ZombieSpawnCells/PlayerStartCells/ObstacleCells from Tiles. */
	bool RebuildDerivedData(FString& OutError);

	bool TryDeriveDoorway(const FIntPoint& Cell, FRoomDoorway& OutDoorway, FString& OutError) const;
};
