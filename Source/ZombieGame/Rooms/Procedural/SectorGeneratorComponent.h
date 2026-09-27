#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Rooms/Procedural/SectorLayoutBuilder.h"
#include "Rooms/RoomTypes.h"
#include "SectorGeneratorComponent.generated.h"

class APlayerStart;
class ARoomModule;
class AZombieBossArenaTrigger;
class AZombieExitDoor;
class AZombieShopTerminal;
class AZombieTreasureChest;
class URoomTemplateDataAsset;
class URoomThemeDataAsset;
class USectorGenerationSettings;

/**
 * Owns a sector's physical layout: picks handcrafted modules, validates the assembly, and spawns
 * the room actors that realise it - plus the level's fixtures (exit door, treasure chests, the
 * boss arena trigger, the intermission's shop terminal).
 *
 * Split from AZombieGameMode deliberately - the GameMode owns *run* logic (which sector, what
 * difficulty, win/loss); this component owns *level* logic. Neither knows how the other works, and
 * the connectivity rules it relies on live in FSectorLayoutBuilder, which has no engine
 * dependencies at all and is tested separately.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ZOMBIEGAME_API USectorGeneratorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USectorGeneratorComponent();

	/**
	 * Tears down any previous sector and assembles a new one. Returns false (leaving no rooms
	 * spawned) if no valid layout could be produced, rather than dropping the player into a
	 * broken sector.
	 */
	bool GenerateSector(int32 Sector, int32 Seed, bool bBossSector);

	/** Tears down the sector and builds the intermission shop room in its place. */
	bool GenerateIntermission(int32 Sector, int32 Seed);

	void ClearSector();

	bool HasSector() const { return SpawnedRooms.Num() > 0; }

	/** Every zombie spawn point in the sector, from the modules' 'S' tiles. */
	const TArray<FVector>& GetZombieSpawnLocations() const { return ZombieSpawnLocations; }

	/** The same spawn points grouped by room, with each room's bounds. */
	const TArray<FZombieSpawnArea>& GetZombieSpawnAreas() const { return ZombieSpawnAreas; }

	/** PlayerStart placed in the generated start room. Null until a sector has been generated. */
	APlayerStart* GetGeneratedPlayerStart() const { return GeneratedPlayerStart; }

	/** World-space bounds covering every room in the current sector. */
	FBox GetSectorBounds() const { return SectorBounds; }

	AZombieExitDoor* GetExitDoor() const { return ExitDoor; }

	/** Where a boss appears: the arena centre, or the exit room's when no arena fit. */
	FVector GetBossSpawnLocation() const { return BossSpawnLocation; }
	bool HasBossArena() const { return BossTrigger != nullptr; }

protected:
	/**
	 * Tuning for pacing/size. Left as a soft reference with a conventional default path so the
	 * project still boots (with a logged error) if the asset is missing or renamed.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Sector")
	TSoftObjectPtr<USectorGenerationSettings> SettingsAsset;

	UPROPERTY(EditDefaultsOnly, Category = "Sector")
	TSubclassOf<ARoomModule> RoomModuleClass;

	UPROPERTY(EditDefaultsOnly, Category = "Fixtures")
	TSubclassOf<AZombieExitDoor> ExitDoorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Fixtures")
	TSubclassOf<AZombieTreasureChest> TreasureChestClass;

	UPROPERTY(EditDefaultsOnly, Category = "Fixtures")
	TSubclassOf<AZombieShopTerminal> ShopTerminalClass;

private:
	/** Resolves room templates through the Asset Manager and filters out unusable layouts. */
	void GatherRoomTemplates(TArray<URoomTemplateDataAsset*>& OutTemplates) const;
	const URoomThemeDataAsset* PickTheme(int32 Sector, FRandomStream& Random) const;

	USectorGenerationSettings* ResolveSettings() const;

	/** Spawns room actors for a validated layout and records spawn points and bounds. */
	bool SpawnRooms(const FSectorLayout& Layout, const USectorGenerationSettings& Settings, const URoomThemeDataAsset* Theme);
	void PlaceSectorFixtures(const FSectorLayout& Layout, int32 Sector);
	void PlacePlayerStart(const FSectorLayout& Layout);
	void PublishNavigationGrid(const FSectorLayout& Layout, float TileSize) const;

	template <typename TActor>
	TActor* SpawnFixture(TSubclassOf<TActor> Class, const FTransform& Transform);

	AZombieExitDoor* SpawnExitDoorIn(const ARoomModule& Room);

	UPROPERTY(Transient)
	TArray<TObjectPtr<ARoomModule>> SpawnedRooms;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> SpawnedFixtures;

	UPROPERTY(Transient)
	TObjectPtr<APlayerStart> GeneratedPlayerStart;

	UPROPERTY(Transient)
	TObjectPtr<AZombieExitDoor> ExitDoor;

	UPROPERTY(Transient)
	TObjectPtr<AZombieBossArenaTrigger> BossTrigger;

	TArray<FVector> ZombieSpawnLocations;
	TArray<FZombieSpawnArea> ZombieSpawnAreas;
	FBox SectorBounds = FBox(ForceInit);
	FVector BossSpawnLocation = FVector::ZeroVector;
};
