#include "ZombieFlowFieldSubsystem.h"
#include "AI/ZombieAISettings.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "ZombieGame.h"

namespace
{
	/** Orthogonal steps used by the flood fill; diagonals would let it leak through wall corners. */
	const FIntPoint FloodNeighbours[] = {
		FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1) };
}

void UZombieFlowFieldSubsystem::Deinitialize()
{
	ClearSectorGrid();
	Super::Deinitialize();
}

void UZombieFlowFieldSubsystem::SetSectorGrid(const FSectorNavigationGrid& InGrid)
{
	Grid = InGrid;
	Distances.Init(INDEX_NONE, Grid.NumCells());
	Directions.Init(FVector2f::ZeroVector, Grid.NumCells());

	if (const UZombieAISettings* Settings = UZombieAISettings::GetOrLoadDefault())
	{
		RebuildInterval = FMath::Max(Settings->FlowFieldRebuildInterval, 0.02f);
	}

	Rebuild();
	StartRebuildTimer();

	UE_LOG(LogZombieGame, Log, TEXT("Flow field grid set: %dx%d cells (%d walkable), rebuilding every %.2fs."),
		Grid.Size.X, Grid.Size.Y, Grid.Walkable.FilterByPredicate([](bool bValue) { return bValue; }).Num(), RebuildInterval);
}

void UZombieFlowFieldSubsystem::ClearSectorGrid()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RebuildTimer);
	}

	Grid.Reset();
	Distances.Reset();
	Directions.Reset();
}

void UZombieFlowFieldSubsystem::StartRebuildTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(RebuildTimer, this, &UZombieFlowFieldSubsystem::Rebuild, RebuildInterval, true);
	}
}

void UZombieFlowFieldSubsystem::GatherGoalCells(TArray<int32>& OutGoalIndices) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		const APlayerController* PlayerController = Iterator->Get();
		const APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
		if (!Pawn)
		{
			continue;
		}

		const FIntPoint GoalCell = Grid.WorldToCell(Pawn->GetActorLocation());
		if (Grid.IsWalkable(GoalCell))
		{
			OutGoalIndices.AddUnique(Grid.ToIndex(GoalCell));
		}
	}
}

void UZombieFlowFieldSubsystem::FloodFill(const TArray<int32>& GoalIndices)
{
	for (int32& Distance : Distances)
	{
		Distance = INDEX_NONE;
	}

	TArray<int32> Frontier;
	Frontier.Reserve(Grid.NumCells());

	for (const int32 GoalIndex : GoalIndices)
	{
		Distances[GoalIndex] = 0;
		Frontier.Add(GoalIndex);
	}

	// Plain queue rather than a priority queue: every step costs the same, so first-visit is
	// already the shortest path and one linear sweep of the grid is the whole cost.
	for (int32 QueueIndex = 0; QueueIndex < Frontier.Num(); ++QueueIndex)
	{
		const int32 CurrentIndex = Frontier[QueueIndex];
		const FIntPoint CurrentCell = Grid.FromIndex(CurrentIndex);
		const int32 NextDistance = Distances[CurrentIndex] + 1;

		for (const FIntPoint& Offset : FloodNeighbours)
		{
			const FIntPoint NeighbourCell = CurrentCell + Offset;
			if (!Grid.IsWalkable(NeighbourCell))
			{
				continue;
			}

			const int32 NeighbourIndex = Grid.ToIndex(NeighbourCell);
			if (Distances[NeighbourIndex] == INDEX_NONE)
			{
				Distances[NeighbourIndex] = NextDistance;
				Frontier.Add(NeighbourIndex);
			}
		}
	}
}

void UZombieFlowFieldSubsystem::BuildDirections()
{
	for (int32 Index = 0; Index < Distances.Num(); ++Index)
	{
		Directions[Index] = FVector2f::ZeroVector;

		const int32 Distance = Distances[Index];
		if (Distance == INDEX_NONE || Distance == 0)
		{
			continue;
		}

		const FIntPoint Cell = Grid.FromIndex(Index);

		// Central difference of the distance field. Reading the gradient rather than just picking
		// the best neighbour gives diagonal, non-staircased movement through open rooms.
		// Blocked or unreachable neighbours read as "one step further away" so walls push outward.
		const auto SampleDistance = [this, Distance](const FIntPoint& SampleCell)
		{
			const int32 SampleIndex = Grid.IsWalkable(SampleCell) ? Grid.ToIndex(SampleCell) : INDEX_NONE;
			const int32 Sampled = (SampleIndex != INDEX_NONE) ? Distances[SampleIndex] : INDEX_NONE;
			return (Sampled == INDEX_NONE) ? Distance + 1 : Sampled;
		};

		const float GradientX = static_cast<float>(SampleDistance(Cell + FIntPoint(-1, 0)) - SampleDistance(Cell + FIntPoint(1, 0)));
		const float GradientY = static_cast<float>(SampleDistance(Cell + FIntPoint(0, -1)) - SampleDistance(Cell + FIntPoint(0, 1)));

		FVector2f Direction(GradientX, GradientY);
		if (!Direction.IsNearlyZero())
		{
			Directions[Index] = Direction.GetSafeNormal();
			continue;
		}

		// Flat gradient (a corridor pinch, or equidistant neighbours): fall back to whichever
		// single neighbour is genuinely closer, so a zombie is never left without a direction.
		int32 BestDistance = Distance;
		FIntPoint BestOffset = FIntPoint::ZeroValue;
		for (const FIntPoint& Offset : FloodNeighbours)
		{
			const FIntPoint NeighbourCell = Cell + Offset;
			if (!Grid.IsWalkable(NeighbourCell))
			{
				continue;
			}

			const int32 NeighbourDistance = Distances[Grid.ToIndex(NeighbourCell)];
			if (NeighbourDistance != INDEX_NONE && NeighbourDistance < BestDistance)
			{
				BestDistance = NeighbourDistance;
				BestOffset = Offset;
			}
		}

		if (BestOffset != FIntPoint::ZeroValue)
		{
			Directions[Index] = FVector2f(static_cast<float>(BestOffset.X), static_cast<float>(BestOffset.Y)).GetSafeNormal();
		}
	}
}

void UZombieFlowFieldSubsystem::Rebuild()
{
	if (Grid.IsEmpty())
	{
		return;
	}

	TArray<int32> GoalIndices;
	GatherGoalCells(GoalIndices);

	if (GoalIndices.Num() == 0)
	{
		// No player standing anywhere on the sector grid - leave the last field in place rather
		// than blanking it, so zombies keep heading to where the players last were.
		return;
	}

	FloodFill(GoalIndices);
	BuildDirections();
}

bool UZombieFlowFieldSubsystem::GetFlowDirection(const FVector& WorldLocation, FVector& OutDirection) const
{
	if (!IsReady())
	{
		return false;
	}

	const FIntPoint Cell = Grid.WorldToCell(WorldLocation);
	if (!Grid.IsWalkable(Cell))
	{
		return false;
	}

	const FVector2f Direction = Directions[Grid.ToIndex(Cell)];
	if (Direction.IsNearlyZero())
	{
		return false;
	}

	OutDirection = FVector(Direction.X, Direction.Y, 0.0f);
	return true;
}

int32 UZombieFlowFieldSubsystem::GetDistanceToNearestPlayer(const FVector& WorldLocation) const
{
	if (!IsReady())
	{
		return INDEX_NONE;
	}

	const FIntPoint Cell = Grid.WorldToCell(WorldLocation);
	return Grid.IsWalkable(Cell) ? Distances[Grid.ToIndex(Cell)] : INDEX_NONE;
}
