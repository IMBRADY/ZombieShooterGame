#include "SectorLayoutBuilder.h"
#include "ZombieGame.h"

namespace
{
	/** A doorway on an already-placed room that nothing has been attached to yet. */
	struct FOpenDoorway
	{
		int32 RoomIndex = INDEX_NONE;
		int32 DoorwayIndex = INDEX_NONE;
	};

	struct FSectorBuildState
	{
		FSectorLayout Layout;

		/** Every sector cell claimed by a module, mapped to the room that claimed it. */
		TMap<FIntPoint, int32> Occupancy;

		TArray<FOpenDoorway> Frontier;
		int32 CombatRoomCount = 0;

		bool IsFootprintFree(const FRoomGrid& Grid, const FIntPoint& Origin) const
		{
			for (int32 Y = 0; Y < Grid.Size.Y; ++Y)
			{
				for (int32 X = 0; X < Grid.Size.X; ++X)
				{
					const FIntPoint LocalCell(X, Y);
					if (!RoomTile::IsSolidFootprint(Grid.GetTile(LocalCell)))
					{
						continue;
					}

					if (Occupancy.Contains(Origin + LocalCell))
					{
						return false;
					}
				}
			}

			return true;
		}

		int32 PlaceRoom(const FSectorRoomCandidate& Candidate, const FRoomGrid& Grid, int32 QuarterTurns, const FIntPoint& Origin)
		{
			FSectorRoomPlacement Placement;
			Placement.SourceIndex = Candidate.SourceIndex;
			Placement.Type = Candidate.Type;
			Placement.Grid = Grid;
			Placement.QuarterTurns = QuarterTurns;
			Placement.Origin = Origin;

			const int32 RoomIndex = Layout.Rooms.Add(MoveTemp(Placement));

			for (int32 Y = 0; Y < Grid.Size.Y; ++Y)
			{
				for (int32 X = 0; X < Grid.Size.X; ++X)
				{
					const FIntPoint LocalCell(X, Y);
					if (RoomTile::IsSolidFootprint(Grid.GetTile(LocalCell)))
					{
						Occupancy.Add(Origin + LocalCell, RoomIndex);
					}
				}
			}

			for (int32 DoorwayIndex = 0; DoorwayIndex < Grid.Doorways.Num(); ++DoorwayIndex)
			{
				Frontier.Add(FOpenDoorway{ RoomIndex, DoorwayIndex });
			}

			if (Candidate.Type == ERoomType::Combat)
			{
				++CombatRoomCount;
			}

			return RoomIndex;
		}

		void ConnectDoorways(const FOpenDoorway& Parent, const FOpenDoorway& Child)
		{
			Layout.Rooms[Parent.RoomIndex].ConnectedDoorways.AddUnique(Parent.DoorwayIndex);
			Layout.Rooms[Child.RoomIndex].ConnectedDoorways.AddUnique(Child.DoorwayIndex);
			Layout.Connections.Add(FSectorRoomConnection{ Parent.RoomIndex, Child.RoomIndex });

			Frontier.RemoveAll([&Child](const FOpenDoorway& Entry)
			{
				return Entry.RoomIndex == Child.RoomIndex && Entry.DoorwayIndex == Child.DoorwayIndex;
			});
		}
	};

	const FSectorRoomCandidate* PickWeighted(const TArray<const FSectorRoomCandidate*>& Pool, FRandomStream& Random, int32& OutPoolIndex)
	{
		float TotalWeight = 0.0f;
		for (const FSectorRoomCandidate* Candidate : Pool)
		{
			TotalWeight += FMath::Max(Candidate->SelectionWeight, 0.0f);
		}

		if (Pool.Num() == 0 || TotalWeight <= 0.0f)
		{
			OutPoolIndex = Pool.Num() > 0 ? Random.RandRange(0, Pool.Num() - 1) : INDEX_NONE;
			return OutPoolIndex == INDEX_NONE ? nullptr : Pool[OutPoolIndex];
		}

		float Roll = Random.FRandRange(0.0f, TotalWeight);
		for (int32 Index = 0; Index < Pool.Num(); ++Index)
		{
			Roll -= FMath::Max(Pool[Index]->SelectionWeight, 0.0f);
			if (Roll <= 0.0f)
			{
				OutPoolIndex = Index;
				return Pool[Index];
			}
		}

		OutPoolIndex = Pool.Num() - 1;
		return Pool.Last();
	}

	/**
	 * Modules eligible to be attached next: unlocked by sector, never a second start room, and
	 * biased to combat arenas until the sector has its quota of them.
	 */
	TArray<const FSectorRoomCandidate*> BuildAttachmentPool(const FSectorBuildState& State, const FSectorLayoutParams& Params)
	{
		TArray<const FSectorRoomCandidate*> Pool;
		TArray<const FSectorRoomCandidate*> CombatOnly;

		for (const FSectorRoomCandidate& Candidate : Params.Candidates)
		{
			if (!Candidate.Grid || Candidate.Grid->IsEmpty() || Candidate.Type == ERoomType::Start)
			{
				continue;
			}

			if (Candidate.MinSector > Params.Sector)
			{
				continue;
			}

			Pool.Add(&Candidate);
			if (Candidate.Type == ERoomType::Combat)
			{
				CombatOnly.Add(&Candidate);
			}
		}

		const bool bNeedsCombat = State.CombatRoomCount < Params.MinCombatRooms;
		return (bNeedsCombat && CombatOnly.Num() > 0) ? CombatOnly : Pool;
	}

	/** Tries every rotation/doorway pairing of one candidate against one open doorway. */
	bool TryAttachCandidate(FSectorBuildState& State, const FSectorRoomCandidate& Candidate,
		const FOpenDoorway& OpenDoor, FRandomStream& Random)
	{
		const FSectorRoomPlacement& ParentRoom = State.Layout.Rooms[OpenDoor.RoomIndex];
		const FRoomDoorway& ParentDoorway = ParentRoom.Grid.Doorways[OpenDoor.DoorwayIndex];
		const FIntPoint TargetCell = ParentRoom.ToSectorCell(ParentDoorway.Cell) + RoomDirection::ToOffset(ParentDoorway.Direction);
		const ERoomDirection RequiredFacing = RoomDirection::Opposite(ParentDoorway.Direction);

		TArray<int32> Rotations = Candidate.bAllowRotation ? TArray<int32>{ 0, 1, 2, 3 } : TArray<int32>{ 0 };
		for (int32 Index = Rotations.Num() - 1; Index > 0; --Index)
		{
			Rotations.Swap(Index, Random.RandRange(0, Index));
		}

		for (const int32 QuarterTurns : Rotations)
		{
			const FRoomGrid RotatedGrid = Candidate.Grid->Rotated(QuarterTurns);

			for (int32 DoorwayIndex = 0; DoorwayIndex < RotatedGrid.Doorways.Num(); ++DoorwayIndex)
			{
				const FRoomDoorway& Doorway = RotatedGrid.Doorways[DoorwayIndex];
				if (Doorway.Direction != RequiredFacing)
				{
					continue;
				}

				const FIntPoint Origin = TargetCell - Doorway.Cell;
				if (!State.IsFootprintFree(RotatedGrid, Origin))
				{
					continue;
				}

				const int32 NewRoomIndex = State.PlaceRoom(Candidate, RotatedGrid, QuarterTurns, Origin);
				State.ConnectDoorways(OpenDoor, FOpenDoorway{ NewRoomIndex, DoorwayIndex });
				return true;
			}
		}

		return false;
	}

	bool PlaceStartRoom(FSectorBuildState& State, const FSectorLayoutParams& Params, FRandomStream& Random)
	{
		TArray<const FSectorRoomCandidate*> StartCandidates;
		for (const FSectorRoomCandidate& Candidate : Params.Candidates)
		{
			if (Candidate.Type == ERoomType::Start && Candidate.Grid && !Candidate.Grid->IsEmpty())
			{
				StartCandidates.Add(&Candidate);
			}
		}

		if (StartCandidates.Num() == 0)
		{
			UE_LOG(LogZombieGame, Error, TEXT("Sector layout: no Start room template available."));
			return false;
		}

		int32 PoolIndex = INDEX_NONE;
		const FSectorRoomCandidate* Chosen = PickWeighted(StartCandidates, Random, PoolIndex);
		const int32 QuarterTurns = Chosen->bAllowRotation ? Random.RandRange(0, 3) : 0;

		State.Layout.StartRoomIndex = State.PlaceRoom(*Chosen, Chosen->Grid->Rotated(QuarterTurns), QuarterTurns, FIntPoint::ZeroValue);
		return true;
	}

	void FinaliseBounds(FSectorLayout& Layout)
	{
		bool bFirst = true;
		for (const FSectorRoomPlacement& Room : Layout.Rooms)
		{
			const FIntPoint RoomMin = Room.Origin;
			const FIntPoint RoomMax = Room.Origin + Room.Grid.Size - FIntPoint(1, 1);

			Layout.MinCell = bFirst ? RoomMin : FIntPoint(FMath::Min(Layout.MinCell.X, RoomMin.X), FMath::Min(Layout.MinCell.Y, RoomMin.Y));
			Layout.MaxCell = bFirst ? RoomMax : FIntPoint(FMath::Max(Layout.MaxCell.X, RoomMax.X), FMath::Max(Layout.MaxCell.Y, RoomMax.Y));
			bFirst = false;
		}
	}
}

FSectorLayout FSectorLayoutBuilder::Build(const FSectorLayoutParams& Params)
{
	FRandomStream Random(Params.Seed);

	FSectorBuildState State;
	State.Layout.Seed = Params.Seed;

	if (!PlaceStartRoom(State, Params, Random))
	{
		return FSectorLayout();
	}

	const int32 TargetRoomCount = FMath::Max(Params.TargetRoomCount, 2);
	int32 AttemptsRemaining = FMath::Max(Params.MaxPlacementAttempts, TargetRoomCount);

	while (State.Layout.Rooms.Num() < TargetRoomCount && State.Frontier.Num() > 0 && AttemptsRemaining-- > 0)
	{
		const int32 FrontierIndex = Random.RandRange(0, State.Frontier.Num() - 1);
		const FOpenDoorway OpenDoor = State.Frontier[FrontierIndex];
		State.Frontier.RemoveAtSwap(FrontierIndex, EAllowShrinking::No);

		TArray<const FSectorRoomCandidate*> Pool = BuildAttachmentPool(State, Params);
		while (Pool.Num() > 0)
		{
			int32 PoolIndex = INDEX_NONE;
			const FSectorRoomCandidate* Candidate = PickWeighted(Pool, Random, PoolIndex);
			if (!Candidate)
			{
				break;
			}

			if (TryAttachCandidate(State, *Candidate, OpenDoor, Random))
			{
				break;
			}

			// This module cannot fit here in any rotation - don't reconsider it for this doorway.
			Pool.RemoveAtSwap(PoolIndex, EAllowShrinking::No);
		}
	}

	FinaliseBounds(State.Layout);

	TArray<int32> HopCounts;
	const TSet<int32> Reachable = GatherReachableRooms(State.Layout, &HopCounts);

	int32 FurthestRoom = State.Layout.StartRoomIndex;
	for (const int32 RoomIndex : Reachable)
	{
		if (HopCounts[RoomIndex] > HopCounts[FurthestRoom])
		{
			FurthestRoom = RoomIndex;
		}
	}
	State.Layout.ExitRoomIndex = FurthestRoom;

	FString ValidationError;
	if (!Validate(State.Layout, ValidationError))
	{
		UE_LOG(LogZombieGame, Error, TEXT("Sector layout (seed %d) failed validation: %s"), Params.Seed, *ValidationError);
		return FSectorLayout();
	}

	UE_LOG(LogZombieGame, Log, TEXT("Sector layout built: seed %d, %d rooms (%d combat), %d connections, exit in room %d."),
		Params.Seed, State.Layout.Rooms.Num(), State.CombatRoomCount, State.Layout.Connections.Num(), State.Layout.ExitRoomIndex);

	return State.Layout;
}

TSet<int32> FSectorLayoutBuilder::GatherReachableRooms(const FSectorLayout& Layout, TArray<int32>* OutHopCounts)
{
	TSet<int32> Reachable;
	TArray<int32> HopCounts;
	HopCounts.Init(TNumericLimits<int32>::Max(), Layout.Rooms.Num());

	if (Layout.Rooms.IsValidIndex(Layout.StartRoomIndex))
	{
		TArray<TArray<int32>> Adjacency;
		Adjacency.SetNum(Layout.Rooms.Num());
		for (const FSectorRoomConnection& Connection : Layout.Connections)
		{
			if (Adjacency.IsValidIndex(Connection.RoomA) && Adjacency.IsValidIndex(Connection.RoomB))
			{
				Adjacency[Connection.RoomA].AddUnique(Connection.RoomB);
				Adjacency[Connection.RoomB].AddUnique(Connection.RoomA);
			}
		}

		TArray<int32> Queue;
		Queue.Add(Layout.StartRoomIndex);
		Reachable.Add(Layout.StartRoomIndex);
		HopCounts[Layout.StartRoomIndex] = 0;

		for (int32 QueueIndex = 0; QueueIndex < Queue.Num(); ++QueueIndex)
		{
			const int32 RoomIndex = Queue[QueueIndex];
			for (const int32 Neighbour : Adjacency[RoomIndex])
			{
				if (!Reachable.Contains(Neighbour))
				{
					Reachable.Add(Neighbour);
					HopCounts[Neighbour] = HopCounts[RoomIndex] + 1;
					Queue.Add(Neighbour);
				}
			}
		}
	}

	if (OutHopCounts)
	{
		*OutHopCounts = MoveTemp(HopCounts);
	}

	return Reachable;
}

bool FSectorLayoutBuilder::Validate(const FSectorLayout& Layout, FString& OutError)
{
	if (Layout.Rooms.Num() < 2)
	{
		OutError = FString::Printf(TEXT("Sector has %d rooms; at least 2 are required."), Layout.Rooms.Num());
		return false;
	}

	if (!Layout.Rooms.IsValidIndex(Layout.StartRoomIndex))
	{
		OutError = TEXT("Sector has no valid start room.");
		return false;
	}

	// "No overlapping geometry" - rebuilt from scratch rather than trusting the builder's own map.
	TMap<FIntPoint, int32> Occupancy;
	for (int32 RoomIndex = 0; RoomIndex < Layout.Rooms.Num(); ++RoomIndex)
	{
		const FSectorRoomPlacement& Room = Layout.Rooms[RoomIndex];

		FString RoomError;
		if (!Room.Grid.Validate(RoomError))
		{
			OutError = FString::Printf(TEXT("Room %d has an invalid module layout: %s"), RoomIndex, *RoomError);
			return false;
		}

		for (int32 Y = 0; Y < Room.Grid.Size.Y; ++Y)
		{
			for (int32 X = 0; X < Room.Grid.Size.X; ++X)
			{
				const FIntPoint LocalCell(X, Y);
				if (!RoomTile::IsSolidFootprint(Room.Grid.GetTile(LocalCell)))
				{
					continue;
				}

				const FIntPoint SectorCell = Room.ToSectorCell(LocalCell);
				if (const int32* ExistingRoom = Occupancy.Find(SectorCell))
				{
					OutError = FString::Printf(TEXT("Rooms %d and %d overlap at cell (%d, %d)."),
						*ExistingRoom, RoomIndex, SectorCell.X, SectorCell.Y);
					return false;
				}

				Occupancy.Add(SectorCell, RoomIndex);
			}
		}
	}

	const TSet<int32> Reachable = GatherReachableRooms(Layout);
	if (Reachable.Num() != Layout.Rooms.Num())
	{
		OutError = FString::Printf(TEXT("Only %d of %d rooms are reachable from the start room."),
			Reachable.Num(), Layout.Rooms.Num());
		return false;
	}

	if (!Layout.Rooms.IsValidIndex(Layout.ExitRoomIndex) || !Reachable.Contains(Layout.ExitRoomIndex))
	{
		OutError = TEXT("Sector has no reachable exit room.");
		return false;
	}

	return true;
}
