#pragma once

#include "CoreMinimal.h"
#include "Rooms/Procedural/RoomGrid.h"
#include "Rooms/RoomTypes.h"

/** One handcrafted module offered to the builder, plus the rules governing when it may be used. */
struct FSectorRoomCandidate
{
	/** Not owned - the caller keeps the parsed grids alive for the duration of the build. */
	const FRoomGrid* Grid = nullptr;

	ERoomType Type = ERoomType::Combat;
	float SelectionWeight = 1.0f;
	int32 MinSector = 1;
	bool bAllowRotation = true;

	/** Index back into whatever template collection the caller passed in. */
	int32 SourceIndex = INDEX_NONE;
};

/** A module placed into the sector, in sector-wide cell space. */
struct FSectorRoomPlacement
{
	int32 SourceIndex = INDEX_NONE;
	ERoomType Type = ERoomType::Combat;

	/** Rotated copy of the candidate grid actually used here. */
	FRoomGrid Grid;

	int32 QuarterTurns = 0;

	/** Sector-space cell that the rotated grid's (0, 0) tile occupies. */
	FIntPoint Origin = FIntPoint::ZeroValue;

	/** Indices into Grid.Doorways that were connected to a neighbour; the rest stay sealed. */
	TArray<int32> ConnectedDoorways;

	FIntPoint ToSectorCell(const FIntPoint& LocalCell) const { return Origin + LocalCell; }
};

struct FSectorRoomConnection
{
	int32 RoomA = INDEX_NONE;
	int32 RoomB = INDEX_NONE;
};

struct FSectorLayoutParams
{
	TArray<FSectorRoomCandidate> Candidates;

	int32 Seed = 0;
	int32 Sector = 1;
	int32 TargetRoomCount = 8;

	/** Enough combat arenas that a sector is never a corridor crawl. */
	int32 MinCombatRooms = 3;

	/** Guards against pathological template sets; the build stops early rather than spinning. */
	int32 MaxPlacementAttempts = 512;
};

struct FSectorLayout
{
	TArray<FSectorRoomPlacement> Rooms;
	TArray<FSectorRoomConnection> Connections;

	int32 StartRoomIndex = INDEX_NONE;

	/** Room furthest from the start by door count - where the exit to the intermission goes. */
	int32 ExitRoomIndex = INDEX_NONE;

	FIntPoint MinCell = FIntPoint::ZeroValue;
	FIntPoint MaxCell = FIntPoint::ZeroValue;

	int32 Seed = 0;

	bool IsEmpty() const { return Rooms.Num() == 0; }
};

/**
 * Assembles a sector out of handcrafted room modules: it chooses which module goes where and how
 * they connect, and never invents geometry (ARCHITECTURE.md 7).
 *
 * Plain C++ on purpose - no UObject, no world, no assets - so the guarantees the design calls for
 * (all rooms connected, no inaccessible spaces, guaranteed path to the exit, no overlapping
 * geometry) can be exercised by Automation Tests without loading a level.
 */
class ZOMBIEGAME_API FSectorLayoutBuilder
{
public:
	/** Builds a layout. Returns an empty layout if the candidate set cannot produce a valid one. */
	static FSectorLayout Build(const FSectorLayoutParams& Params);

	/**
	 * Re-checks the design's generation rules against a finished layout, independently of how it
	 * was produced: no overlapping footprints, every room reachable from the start, exit reachable.
	 */
	static bool Validate(const FSectorLayout& Layout, FString& OutError);

private:
	/** Rooms reachable from StartRoomIndex, walking only the recorded door connections. */
	static TSet<int32> GatherReachableRooms(const FSectorLayout& Layout, TArray<int32>* OutHopCounts = nullptr);
};
