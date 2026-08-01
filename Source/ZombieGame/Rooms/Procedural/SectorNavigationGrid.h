#pragma once

#include "CoreMinimal.h"
#include "Rooms/Procedural/SectorLayoutBuilder.h"

/**
 * A flat walkability grid covering an assembled sector: one cell per layout tile, walkable where a
 * pawn can stand.
 *
 * Built straight from the room layout rather than queried back out of the navmesh - the layout is
 * already an exact, authoritative tile map, so this costs nothing and cannot disagree with the
 * geometry that was spawned from the same data.
 *
 * Plain C++ (no UObject, no world) so the grid and everything derived from it stays testable.
 */
struct ZOMBIEGAME_API FSectorNavigationGrid
{
	/** Lowest sector cell the grid covers; grid index 0 maps to this cell. */
	FIntPoint MinCell = FIntPoint::ZeroValue;

	/** Grid extents in cells. */
	FIntPoint Size = FIntPoint::ZeroValue;

	float TileSize = 250.0f;

	/** Row-major, Size.X * Size.Y entries. */
	TArray<bool> Walkable;

	/** Rebuilds the grid from an assembled layout. Sealed doorways count as wall. */
	void BuildFromLayout(const FSectorLayout& Layout, float InTileSize);

	void Reset();

	bool IsEmpty() const { return Walkable.Num() == 0; }

	bool IsValidCell(const FIntPoint& Cell) const
	{
		const FIntPoint Local = Cell - MinCell;
		return Local.X >= 0 && Local.Y >= 0 && Local.X < Size.X && Local.Y < Size.Y;
	}

	int32 ToIndex(const FIntPoint& Cell) const
	{
		const FIntPoint Local = Cell - MinCell;
		return Local.Y * Size.X + Local.X;
	}

	FIntPoint FromIndex(int32 Index) const
	{
		return MinCell + FIntPoint(Index % Size.X, Index / Size.X);
	}

	bool IsWalkable(const FIntPoint& Cell) const
	{
		return IsValidCell(Cell) && Walkable[ToIndex(Cell)];
	}

	FIntPoint WorldToCell(const FVector& WorldLocation) const
	{
		return FIntPoint(FMath::FloorToInt(WorldLocation.X / TileSize), FMath::FloorToInt(WorldLocation.Y / TileSize));
	}

	FVector2D CellToWorld(const FIntPoint& Cell) const
	{
		return FVector2D((Cell.X + 0.5) * TileSize, (Cell.Y + 0.5) * TileSize);
	}

	int32 NumCells() const { return Walkable.Num(); }
};
