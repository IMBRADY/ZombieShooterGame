#include "SectorNavigationGrid.h"

void FSectorNavigationGrid::Reset()
{
	MinCell = FIntPoint::ZeroValue;
	Size = FIntPoint::ZeroValue;
	Walkable.Reset();
}

void FSectorNavigationGrid::BuildFromLayout(const FSectorLayout& Layout, float InTileSize)
{
	Reset();
	TileSize = FMath::Max(InTileSize, 1.0f);

	if (Layout.Rooms.Num() == 0)
	{
		return;
	}

	MinCell = Layout.MinCell;
	Size = (Layout.MaxCell - Layout.MinCell) + FIntPoint(1, 1);
	Walkable.Init(false, Size.X * Size.Y);

	for (const FSectorRoomPlacement& Room : Layout.Rooms)
	{
		// Only doorways the layout actually connected are passable; the rest were sealed back into
		// wall when the module was built, and the grid has to agree with the geometry.
		TSet<FIntPoint> OpenDoorCells;
		for (const int32 DoorwayIndex : Room.ConnectedDoorways)
		{
			if (Room.Grid.Doorways.IsValidIndex(DoorwayIndex))
			{
				OpenDoorCells.Add(Room.Grid.Doorways[DoorwayIndex].Cell);
			}
		}

		for (int32 Y = 0; Y < Room.Grid.Size.Y; ++Y)
		{
			for (int32 X = 0; X < Room.Grid.Size.X; ++X)
			{
				const FIntPoint LocalCell(X, Y);
				const ERoomTile Tile = Room.Grid.GetTile(LocalCell);

				const bool bPassable = (Tile == ERoomTile::Door)
					? OpenDoorCells.Contains(LocalCell)
					: RoomTile::IsWalkable(Tile);

				if (!bPassable)
				{
					continue;
				}

				const FIntPoint SectorCell = Room.ToSectorCell(LocalCell);
				if (IsValidCell(SectorCell))
				{
					Walkable[ToIndex(SectorCell)] = true;
				}
			}
		}
	}
}
