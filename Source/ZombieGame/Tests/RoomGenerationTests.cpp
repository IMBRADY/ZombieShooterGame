#include "Tests/ZombieTestFixtures.h"

#if WITH_DEV_AUTOMATION_TESTS

// --- FRoomGrid: parsing and per-module rules -------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoomGridParseTest, "ZombieGame.Rooms.Grid.ParsesShippedLayouts", ZombieTest::Flags)
bool FRoomGridParseTest::RunTest(const FString& Parameters)
{
	for (const ZombieTest::FRoomFixture& Fixture : ZombieTest::GetRoomFixtures())
	{
		FRoomGrid Grid;
		FString Error;
		TestTrue(FString::Printf(TEXT("parses (%s)"), *Error), Grid.ParseFrom(Fixture.Rows, Error));
		TestTrue(FString::Printf(TEXT("validates (%s)"), *Error), Grid.Validate(Error));
		TestEqual(TEXT("width"), Grid.Size.X, Fixture.Rows[0].Len());
		TestEqual(TEXT("height"), Grid.Size.Y, Fixture.Rows.Num());
		TestTrue(TEXT("has a doorway"), Grid.Doorways.Num() > 0);
	}

	// The start room is where the player spawns; losing its 'P' would silently fall back to the level's PlayerStart.
	FRoomGrid Start;
	FString Error;
	Start.ParseFrom(ZombieTest::GetRoomFixtures()[0].Rows, Error);
	TestEqual(TEXT("start room has one player start"), Start.PlayerStartCells.Num(), 1);
	TestEqual(TEXT("start room has four doorways"), Start.Doorways.Num(), 4);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoomGridRejectsMalformedTest, "ZombieGame.Rooms.Grid.RejectsMalformedLayouts", ZombieTest::Flags)
bool FRoomGridRejectsMalformedTest::RunTest(const FString& Parameters)
{
	FRoomGrid Grid;
	FString Error;

	TestFalse(TEXT("too few rows"), Grid.ParseFrom({ TEXT("###"), TEXT("#.#") }, Error));
	TestFalse(TEXT("ragged rows"), Grid.ParseFrom({ TEXT("#####"), TEXT("#..#"), TEXT("#####") }, Error));
	TestFalse(TEXT("unknown character"), Grid.ParseFrom({ TEXT("###"), TEXT("D?#"), TEXT("###") }, Error));
	TestFalse(TEXT("door in a corner"), Grid.ParseFrom({ TEXT("D##"), TEXT("#.#"), TEXT("###") }, Error));

	// Parses, but can never join a sector.
	TestTrue(TEXT("sealed room parses"), Grid.ParseFrom({ TEXT("###"), TEXT("#.#"), TEXT("###") }, Error));
	TestFalse(TEXT("sealed room fails validation"), Grid.Validate(Error));

	// "No inaccessible spaces": a wall splitting the room leaves a pocket nobody can reach.
	TestTrue(TEXT("split room parses"), Grid.ParseFrom({
		TEXT("#####"),
		TEXT("D.#.#"),
		TEXT("#####") }, Error));
	TestFalse(TEXT("split room fails validation"), Grid.Validate(Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoomGridRotationTest, "ZombieGame.Rooms.Grid.Rotation", ZombieTest::Flags)
bool FRoomGridRotationTest::RunTest(const FString& Parameters)
{
	FRoomGrid Corner;
	FString Error;
	Corner.ParseFrom(ZombieTest::GetRoomFixtures()[4].Rows, Error);

	for (int32 Turns = 0; Turns < 4; ++Turns)
	{
		const FRoomGrid Rotated = Corner.Rotated(Turns);
		TestTrue(FString::Printf(TEXT("rotated %d stays valid"), Turns), Rotated.Validate(Error));
		TestEqual(TEXT("doorways survive rotation"), Rotated.Doorways.Num(), Corner.Doorways.Num());
		TestEqual(TEXT("tile count survives rotation"), Rotated.Tiles.Num(), Corner.Tiles.Num());
	}

	// A straight hallway turned a quarter swaps its extents and faces its doors north/south.
	FRoomGrid Hall;
	Hall.ParseFrom(ZombieTest::GetRoomFixtures()[3].Rows, Error);
	const FRoomGrid Turned = Hall.Rotated(1);
	TestTrue(TEXT("quarter turn swaps extents"), Turned.Size == FIntPoint(Hall.Size.Y, Hall.Size.X));
	for (const FRoomDoorway& Door : Turned.Doorways)
	{
		TestTrue(TEXT("turned hallway doors face north or south"),
			Door.Direction == ERoomDirection::North || Door.Direction == ERoomDirection::South);
	}

	// Four quarter turns is the identity.
	const FRoomGrid FullCircle = Corner.Rotated(1).Rotated(1).Rotated(1).Rotated(1);
	TestTrue(TEXT("four turns restore the layout"), FullCircle.Tiles == Corner.Tiles && FullCircle.Size == Corner.Size);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRoomGridOptionalTilesTest, "ZombieGame.Rooms.Grid.OptionalObstaclesResolve", ZombieTest::Flags)
bool FRoomGridOptionalTilesTest::RunTest(const FString& Parameters)
{
	FString Error;
	for (const ZombieTest::FRoomFixture& Fixture : ZombieTest::GetRoomFixtures())
	{
		for (int32 Seed = 0; Seed < 8; ++Seed)
		{
			FRoomGrid Grid;
			Grid.ParseFrom(Fixture.Rows, Error);
			FRandomStream Random(Seed);
			Grid.ResolveOptionalTiles(Random, 0.5f);

			TestFalse(TEXT("no optional tiles remain"), Grid.Tiles.Contains(ERoomTile::OptionalObstacle));
			// Optional cover is validated as if present, so resolving it either way can't cut a room in two.
			TestTrue(FString::Printf(TEXT("still fully connected (%s)"), *Error), Grid.Validate(Error));
		}
	}
	return true;
}

// --- FSectorLayoutBuilder: the design's generation rules --------------------------------------

namespace
{
	/** Independent of the builder's own checks: no two rooms may claim the same sector cell. */
	bool HasOverlappingFootprints(const FSectorLayout& Layout)
	{
		TSet<FIntPoint> Claimed;
		for (const FSectorRoomPlacement& Room : Layout.Rooms)
		{
			for (int32 Y = 0; Y < Room.Grid.Size.Y; ++Y)
			{
				for (int32 X = 0; X < Room.Grid.Size.X; ++X)
				{
					const FIntPoint Local(X, Y);
					if (!RoomTile::IsSolidFootprint(Room.Grid.GetTile(Local)))
					{
						continue;
					}

					bool bAlreadyClaimed = false;
					Claimed.Add(Room.ToSectorCell(Local), &bAlreadyClaimed);
					if (bAlreadyClaimed)
					{
						return true;
					}
				}
			}
		}
		return false;
	}

	/** Rooms reachable from the start over recorded connections - the "all rooms connected" rule. */
	int32 CountReachableRooms(const FSectorLayout& Layout)
	{
		TSet<int32> Reached = { Layout.StartRoomIndex };
		TArray<int32> Pending = { Layout.StartRoomIndex };
		while (Pending.Num() > 0)
		{
			const int32 Current = Pending.Pop();
			for (const FSectorRoomConnection& Link : Layout.Connections)
			{
				const int32 Other = Link.RoomA == Current ? Link.RoomB : (Link.RoomB == Current ? Link.RoomA : INDEX_NONE);
				if (Other != INDEX_NONE && !Reached.Contains(Other))
				{
					Reached.Add(Other);
					Pending.Add(Other);
				}
			}
		}
		return Reached.Num();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSectorLayoutRulesTest, "ZombieGame.Rooms.Sector.GenerationRules", ZombieTest::Flags)
bool FSectorLayoutRulesTest::RunTest(const FString& Parameters)
{
	const TArray<FRoomGrid> Grids = ZombieTest::ParseFixtureGrids();
	int32 BossSectors = 0;
	int32 BossArenasPlaced = 0;

	for (int32 Sector = 1; Sector <= 10; ++Sector)
	{
		for (int32 Seed = 1; Seed <= 25; ++Seed)
		{
			const FSectorLayoutParams Params = ZombieTest::MakeLayoutParams(Grids, Seed * 7919 + Sector, Sector);
			const FSectorLayout Layout = FSectorLayoutBuilder::Build(Params);
			const FString Context = FString::Printf(TEXT("sector %d seed %d"), Sector, Params.Seed);

			if (!TestFalse(Context + TEXT(": produced a layout"), Layout.IsEmpty()))
			{
				continue;
			}

			FString Error;
			TestTrue(Context + TEXT(": passes builder validation - ") + Error, FSectorLayoutBuilder::Validate(Layout, Error));
			TestFalse(Context + TEXT(": no overlapping geometry"), HasOverlappingFootprints(Layout));
			TestEqual(Context + TEXT(": every room reachable"), CountReachableRooms(Layout), Layout.Rooms.Num());
			TestTrue(Context + TEXT(": has a start room"), Layout.Rooms.IsValidIndex(Layout.StartRoomIndex)
				&& Layout.Rooms[Layout.StartRoomIndex].Type == ERoomType::Start);
			TestTrue(Context + TEXT(": exit is a different room"), Layout.Rooms.IsValidIndex(Layout.ExitRoomIndex)
				&& Layout.ExitRoomIndex != Layout.StartRoomIndex);

			const int32 CombatRooms = Layout.Rooms.FilterByPredicate([](const FSectorRoomPlacement& Room) { return Room.Type == ERoomType::Combat; }).Num();
			TestTrue(Context + TEXT(": enough combat arenas"), CombatRooms >= Params.MinCombatRooms);

			for (const FSectorRoomPlacement& Room : Layout.Rooms)
			{
				const int32 MinSector = ZombieTest::GetRoomFixtures()[Room.SourceIndex].MinSector;
				TestTrue(Context + TEXT(": no room before its minimum sector"), MinSector <= Sector);
			}

			if (Params.bRequireBossRoom)
			{
				++BossSectors;
				if (Layout.BossRoomIndex != INDEX_NONE)
				{
					++BossArenasPlaced;
					TestEqual(Context + TEXT(": boss arena is the exit"), Layout.ExitRoomIndex, Layout.BossRoomIndex);
				}
			}
			else
			{
				TestEqual(Context + TEXT(": no boss arena outside boss sectors"), Layout.BossRoomIndex, static_cast<int32>(INDEX_NONE));
			}
		}
	}

	TestEqual(TEXT("every boss sector fits its arena"), BossArenasPlaced, BossSectors);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSectorLayoutDeterminismTest, "ZombieGame.Rooms.Sector.SeedIsDeterministic", ZombieTest::Flags)
bool FSectorLayoutDeterminismTest::RunTest(const FString& Parameters)
{
	// Mid-run saves restore a sector from its seed, so the same seed must rebuild the same sector.
	const TArray<FRoomGrid> Grids = ZombieTest::ParseFixtureGrids();
	for (int32 Seed : { 1, 42, -87813, 1115000916 })
	{
		const FSectorLayoutParams Params = ZombieTest::MakeLayoutParams(Grids, Seed, 3);
		const FSectorLayout First = FSectorLayoutBuilder::Build(Params);
		const FSectorLayout Second = FSectorLayoutBuilder::Build(Params);

		if (!TestEqual(TEXT("same room count"), Second.Rooms.Num(), First.Rooms.Num()))
		{
			continue;
		}
		for (int32 Index = 0; Index < First.Rooms.Num(); ++Index)
		{
			TestEqual(TEXT("same module"), Second.Rooms[Index].SourceIndex, First.Rooms[Index].SourceIndex);
			TestTrue(TEXT("same origin"), Second.Rooms[Index].Origin == First.Rooms[Index].Origin);
			TestEqual(TEXT("same rotation"), Second.Rooms[Index].QuarterTurns, First.Rooms[Index].QuarterTurns);
		}
		TestEqual(TEXT("same exit"), Second.ExitRoomIndex, First.ExitRoomIndex);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSectorLayoutNoStartTest, "ZombieGame.Rooms.Sector.FailsCleanlyWithoutStartRoom", ZombieTest::Flags)
bool FSectorLayoutNoStartTest::RunTest(const FString& Parameters)
{
	const TArray<FRoomGrid> Grids = ZombieTest::ParseFixtureGrids();
	FSectorLayoutParams Params = ZombieTest::MakeLayoutParams(Grids, 7, 1);
	Params.Candidates.RemoveAll([](const FSectorRoomCandidate& Candidate) { return Candidate.Type == ERoomType::Start; });

	AddExpectedError(TEXT("no Start room template"), EAutomationExpectedErrorFlags::Contains, 1);
	TestTrue(TEXT("an impossible template set yields an empty layout, not a broken one"), FSectorLayoutBuilder::Build(Params).IsEmpty());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
