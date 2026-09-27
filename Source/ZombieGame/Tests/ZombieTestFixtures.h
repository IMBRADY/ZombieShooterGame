#pragma once

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Rooms/Procedural/RoomGrid.h"
#include "Rooms/Procedural/SectorLayoutBuilder.h"

/**
 * Shared inputs for the ZombieGame automation tests (ARCHITECTURE.md 17).
 *
 * The room layouts mirror a representative subset of the shipped modules in
 * Content/Python/content_world.py, so the layout builder is exercised against the same shapes the
 * game assembles - without loading a single asset.
 */
namespace ZombieTest
{
	/** Every test here is a fast, engine-light product test. */
	constexpr EAutomationTestFlags Flags = EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter;

	struct FRoomFixture
	{
		ERoomType Type;
		float Weight;
		int32 MinSector;
		TArray<FString> Rows;
	};

	inline const TArray<FRoomFixture>& GetRoomFixtures()
	{
		static const TArray<FRoomFixture> Fixtures = {
			{ ERoomType::Start, 1.0f, 1, {
				TEXT("####D####"),
				TEXT("#.......#"),
				TEXT("#.o...o.#"),
				TEXT("#..OOO..#"),
				TEXT("D...P...D"),
				TEXT("#..OOO..#"),
				TEXT("#.o...o.#"),
				TEXT("#.......#"),
				TEXT("####D####") } },
			{ ERoomType::Combat, 1.0f, 1, {
				TEXT("######D######"),
				TEXT("#...........#"),
				TEXT("#..OO...OO..#"),
				TEXT("#.....o.....#"),
				TEXT("#....S.S....#"),
				TEXT("D..o.....o..D"),
				TEXT("#....S.S....#"),
				TEXT("#.....o.....#"),
				TEXT("#..OO...OO..#"),
				TEXT("#...........#"),
				TEXT("######D######") } },
			{ ERoomType::Combat, 1.2f, 1, {
				TEXT("####D####"),
				TEXT("#.......#"),
				TEXT("#.S...S.#"),
				TEXT("#...O...#"),
				TEXT("D..oOo..D"),
				TEXT("#...O...#"),
				TEXT("#.S...S.#"),
				TEXT("#.......#"),
				TEXT("####D####") } },
			{ ERoomType::Hallway, 1.5f, 1, {
				TEXT("#########"),
				TEXT("D.......D"),
				TEXT("#########") } },
			{ ERoomType::Hallway, 1.1f, 1, {
				TEXT("###D###"),
				TEXT("#.....#"),
				TEXT("#.....#"),
				TEXT("#..o..D"),
				TEXT("#.....#"),
				TEXT("#.....#"),
				TEXT("#######") } },
			{ ERoomType::Hallway, 0.7f, 2, {
				TEXT("###D###"),
				TEXT("#.....#"),
				TEXT("#.....#"),
				TEXT("D..o..D"),
				TEXT("#.....#"),
				TEXT("#.....#"),
				TEXT("###D###") } },
			{ ERoomType::DeadEnd, 0.6f, 1, {
				TEXT("#######"),
				TEXT("#..O..#"),
				TEXT("#.....#"),
				TEXT("D..S..#"),
				TEXT("#.....#"),
				TEXT("#..O..#"),
				TEXT("#######") } },
			{ ERoomType::Treasure, 0.3f, 1, {
				TEXT("#######"),
				TEXT("#.....#"),
				TEXT("#..O..#"),
				TEXT("D.....#"),
				TEXT("#..O..#"),
				TEXT("#.....#"),
				TEXT("#######") } },
			{ ERoomType::Boss, 1.0f, 1, {
				TEXT("#########D#########"),
				TEXT("#.................#"),
				TEXT("#..O...........O..#"),
				TEXT("#.................#"),
				TEXT("#.....o.....o.....#"),
				TEXT("#.................#"),
				TEXT("#.................#"),
				TEXT("#..o...........o..#"),
				TEXT("#.................#"),
				TEXT("D........S........D"),
				TEXT("#.................#"),
				TEXT("#..o...........o..#"),
				TEXT("#.................#"),
				TEXT("#.................#"),
				TEXT("#.....o.....o.....#"),
				TEXT("#.................#"),
				TEXT("#..O...........O..#"),
				TEXT("#.................#"),
				TEXT("#########D#########") } },
		};
		return Fixtures;
	}

	/** Parsed grids for every fixture; the returned array owns the grids the candidates point at. */
	inline TArray<FRoomGrid> ParseFixtureGrids()
	{
		TArray<FRoomGrid> Grids;
		for (const FRoomFixture& Fixture : GetRoomFixtures())
		{
			FString Error;
			FRoomGrid& Grid = Grids.AddDefaulted_GetRef();
			ensureMsgf(Grid.ParseFrom(Fixture.Rows, Error), TEXT("Fixture failed to parse: %s"), *Error);
		}
		return Grids;
	}

	inline FSectorLayoutParams MakeLayoutParams(const TArray<FRoomGrid>& Grids, int32 Seed, int32 Sector)
	{
		const TArray<FRoomFixture>& Fixtures = GetRoomFixtures();

		FSectorLayoutParams Params;
		Params.Seed = Seed;
		Params.Sector = Sector;
		Params.TargetRoomCount = 7 + FMath::Min(Sector / 2, 5);
		Params.MinCombatRooms = 3;
		Params.bRequireBossRoom = Sector % 5 == 0;

		for (int32 Index = 0; Index < Grids.Num(); ++Index)
		{
			FSectorRoomCandidate& Candidate = Params.Candidates.AddDefaulted_GetRef();
			Candidate.Grid = &Grids[Index];
			Candidate.Type = Fixtures[Index].Type;
			Candidate.SelectionWeight = Fixtures[Index].Weight;
			Candidate.MinSector = Fixtures[Index].MinSector;
			Candidate.SourceIndex = Index;
		}
		return Params;
	}
}

#endif // WITH_DEV_AUTOMATION_TESTS
