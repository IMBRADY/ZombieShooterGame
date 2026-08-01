#include "RoomGrid.h"

namespace RoomTile
{
	TCHAR ToChar(ERoomTile Tile)
	{
		switch (Tile)
		{
		case ERoomTile::Wall:			return TEXT('#');
		case ERoomTile::Floor:			return TEXT('.');
		case ERoomTile::Door:			return TEXT('D');
		case ERoomTile::Obstacle:		return TEXT('O');
		case ERoomTile::ZombieSpawn:	return TEXT('S');
		case ERoomTile::PlayerStart:	return TEXT('P');
		default:						return TEXT(' ');
		}
	}

	bool FromChar(TCHAR Character, ERoomTile& OutTile)
	{
		switch (Character)
		{
		case TEXT('#'):	OutTile = ERoomTile::Wall;			return true;
		case TEXT('.'):	OutTile = ERoomTile::Floor;			return true;
		case TEXT('D'):	OutTile = ERoomTile::Door;			return true;
		case TEXT('O'):	OutTile = ERoomTile::Obstacle;		return true;
		case TEXT('S'):	OutTile = ERoomTile::ZombieSpawn;	return true;
		case TEXT('P'):	OutTile = ERoomTile::PlayerStart;	return true;
		case TEXT(' '):
		case TEXT('_'):	OutTile = ERoomTile::Empty;			return true;
		default:									 		return false;
		}
	}
}

bool FRoomGrid::ParseFrom(const TArray<FString>& Rows, FString& OutError)
{
	Size = FIntPoint::ZeroValue;
	Tiles.Reset();

	if (Rows.Num() < 3)
	{
		OutError = TEXT("Layout needs at least 3 rows (walls on both sides plus interior).");
		return false;
	}

	const int32 Width = Rows[0].Len();
	if (Width < 3)
	{
		OutError = TEXT("Layout needs at least 3 columns (walls on both sides plus interior).");
		return false;
	}

	Size = FIntPoint(Width, Rows.Num());
	Tiles.SetNumUninitialized(Width * Rows.Num());

	for (int32 RowIndex = 0; RowIndex < Rows.Num(); ++RowIndex)
	{
		const FString& Row = Rows[RowIndex];
		if (Row.Len() != Width)
		{
			OutError = FString::Printf(TEXT("Row %d is %d characters, expected %d - all rows must be the same length."),
				RowIndex, Row.Len(), Width);
			return false;
		}

		for (int32 ColumnIndex = 0; ColumnIndex < Width; ++ColumnIndex)
		{
			ERoomTile Tile = ERoomTile::Empty;
			if (!RoomTile::FromChar(Row[ColumnIndex], Tile))
			{
				OutError = FString::Printf(TEXT("Unrecognised tile character '%c' at row %d, column %d."),
					Row[ColumnIndex], RowIndex, ColumnIndex);
				return false;
			}

			Tiles[RowIndex * Width + ColumnIndex] = Tile;
		}
	}

	return RebuildDerivedData(OutError);
}

bool FRoomGrid::TryDeriveDoorway(const FIntPoint& Cell, FRoomDoorway& OutDoorway, FString& OutError) const
{
	// A doorway faces whichever module edge it sits on. Anything else (an interior door, or a
	// door in a corner facing two ways at once) can't be connected to unambiguously.
	const bool bOnWest = Cell.X == 0;
	const bool bOnEast = Cell.X == Size.X - 1;
	const bool bOnNorth = Cell.Y == 0;
	const bool bOnSouth = Cell.Y == Size.Y - 1;

	const int32 EdgeCount = (bOnWest ? 1 : 0) + (bOnEast ? 1 : 0) + (bOnNorth ? 1 : 0) + (bOnSouth ? 1 : 0);
	if (EdgeCount != 1)
	{
		OutError = FString::Printf(TEXT("Doorway at (%d, %d) must sit on exactly one module edge (found %d)."),
			Cell.X, Cell.Y, EdgeCount);
		return false;
	}

	ERoomDirection Direction = ERoomDirection::North;
	if (bOnWest)		{ Direction = ERoomDirection::West; }
	else if (bOnEast)	{ Direction = ERoomDirection::East; }
	else if (bOnNorth)	{ Direction = ERoomDirection::North; }
	else				{ Direction = ERoomDirection::South; }

	OutDoorway = FRoomDoorway(Cell, Direction);
	return true;
}

bool FRoomGrid::RebuildDerivedData(FString& OutError)
{
	Doorways.Reset();
	ZombieSpawnCells.Reset();
	PlayerStartCells.Reset();
	ObstacleCells.Reset();

	for (int32 Y = 0; Y < Size.Y; ++Y)
	{
		for (int32 X = 0; X < Size.X; ++X)
		{
			const FIntPoint Cell(X, Y);
			switch (GetTile(Cell))
			{
			case ERoomTile::Door:
			{
				FRoomDoorway Doorway;
				if (!TryDeriveDoorway(Cell, Doorway, OutError))
				{
					return false;
				}
				Doorways.Add(Doorway);
				break;
			}
			case ERoomTile::ZombieSpawn:	ZombieSpawnCells.Add(Cell); break;
			case ERoomTile::PlayerStart:	PlayerStartCells.Add(Cell); break;
			case ERoomTile::Obstacle:		ObstacleCells.Add(Cell); break;
			default: break;
			}
		}
	}

	return true;
}

FRoomGrid FRoomGrid::Rotated(int32 QuarterTurns) const
{
	const int32 Turns = ((QuarterTurns % 4) + 4) % 4;
	if (Turns == 0 || IsEmpty())
	{
		return *this;
	}

	FRoomGrid Result;
	Result.Size = (Turns % 2 == 0) ? Size : FIntPoint(Size.Y, Size.X);
	Result.Tiles.SetNumUninitialized(Tiles.Num());

	for (int32 Y = 0; Y < Size.Y; ++Y)
	{
		for (int32 X = 0; X < Size.X; ++X)
		{
			// One clockwise quarter turn maps (X, Y) -> (Height - 1 - Y, X).
			FIntPoint Target(X, Y);
			FIntPoint SourceSize = Size;
			for (int32 Turn = 0; Turn < Turns; ++Turn)
			{
				Target = FIntPoint(SourceSize.Y - 1 - Target.Y, Target.X);
				SourceSize = FIntPoint(SourceSize.Y, SourceSize.X);
			}

			Result.Tiles[Target.Y * Result.Size.X + Target.X] = GetTile(FIntPoint(X, Y));
		}
	}

	FString UnusedError;
	Result.RebuildDerivedData(UnusedError);
	return Result;
}

bool FRoomGrid::Validate(FString& OutError) const
{
	if (IsEmpty())
	{
		OutError = TEXT("Layout is empty.");
		return false;
	}

	if (Doorways.Num() == 0)
	{
		OutError = TEXT("Layout has no doorways ('D'), so it can never be connected to a sector.");
		return false;
	}

	// Flood fill the walkable cells from the first one; anything left over is a sealed pocket the
	// player could never reach.
	int32 WalkableCount = 0;
	FIntPoint SeedCell = FIntPoint(-1, -1);
	for (int32 Y = 0; Y < Size.Y; ++Y)
	{
		for (int32 X = 0; X < Size.X; ++X)
		{
			if (IsWalkable(FIntPoint(X, Y)))
			{
				++WalkableCount;
				if (SeedCell.X < 0)
				{
					SeedCell = FIntPoint(X, Y);
				}
			}
		}
	}

	if (WalkableCount == 0)
	{
		OutError = TEXT("Layout has no walkable tiles.");
		return false;
	}

	TSet<FIntPoint> Visited;
	TArray<FIntPoint> Pending;
	Visited.Add(SeedCell);
	Pending.Add(SeedCell);

	static const ERoomDirection AllDirections[] = {
		ERoomDirection::North, ERoomDirection::East, ERoomDirection::South, ERoomDirection::West };

	while (Pending.Num() > 0)
	{
		const FIntPoint Current = Pending.Pop(EAllowShrinking::No);
		for (const ERoomDirection Direction : AllDirections)
		{
			const FIntPoint Neighbour = Current + RoomDirection::ToOffset(Direction);
			if (IsWalkable(Neighbour) && !Visited.Contains(Neighbour))
			{
				Visited.Add(Neighbour);
				Pending.Add(Neighbour);
			}
		}
	}

	if (Visited.Num() != WalkableCount)
	{
		OutError = FString::Printf(TEXT("Layout has %d walkable tiles but only %d are reachable from each other."),
			WalkableCount, Visited.Num());
		return false;
	}

	return true;
}
