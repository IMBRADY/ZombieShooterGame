#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SectorGeneratorComponent.generated.h"

class APlayerStart;
class ARoomModule;
class URoomTemplateDataAsset;
class USectorGenerationSettings;

/**
 * Owns a sector's physical layout: picks handcrafted modules, validates the assembly, and spawns
 * the room actors that realise it.
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
	bool GenerateSector(int32 Sector, int32 Seed);

	void ClearSector();

	bool HasSector() const { return SpawnedRooms.Num() > 0; }

	/** Every zombie spawn point in the sector, from the modules' 'S' tiles. */
	const TArray<FVector>& GetZombieSpawnLocations() const { return ZombieSpawnLocations; }

	/** PlayerStart placed in the generated start room. Null until a sector has been generated. */
	APlayerStart* GetGeneratedPlayerStart() const { return GeneratedPlayerStart; }

	/** World-space bounds covering every room in the current sector. */
	FBox GetSectorBounds() const { return SectorBounds; }

	/** Room the exit to the intermission belongs in - the furthest room from the start. */
	ARoomModule* GetExitRoom() const;

protected:
	/**
	 * Tuning for pacing/size. Left as a soft reference with a conventional default path so the
	 * project still boots (with a logged error) if the asset is missing or renamed.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Sector")
	TSoftObjectPtr<USectorGenerationSettings> SettingsAsset;

	UPROPERTY(EditDefaultsOnly, Category = "Sector")
	TSubclassOf<ARoomModule> RoomModuleClass;

private:
	/** Resolves room templates through the Asset Manager and filters out unusable layouts. */
	void GatherRoomTemplates(TArray<URoomTemplateDataAsset*>& OutTemplates) const;

	USectorGenerationSettings* ResolveSettings() const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ARoomModule>> SpawnedRooms;

	UPROPERTY(Transient)
	TObjectPtr<APlayerStart> GeneratedPlayerStart;

	TArray<FVector> ZombieSpawnLocations;
	FBox SectorBounds = FBox(ForceInit);
	int32 ExitRoomIndex = INDEX_NONE;
};
