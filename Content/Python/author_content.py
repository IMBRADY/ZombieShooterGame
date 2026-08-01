"""One-shot content authoring pass. DELETE THIS FILE once it has reported success.

Creates the room template / zombie archetype / tuning Data Assets and adds a navmesh bounds
volume to the test level. Kept as a script only because there is no editor GUI session in this
workflow; the assets it produces are the real, checked-in content.
"""

import os
import unreal

ROOM_DIR = "/Game/DataAssets/Rooms"
ZOMBIE_DIR = "/Game/DataAssets/Zombies"
MAP_PATH = "/Game/Maps/L_TestSector"
MARKER = os.path.join(os.path.dirname(__file__), "init_unreal_result.txt")

log = []


def note(message):
    unreal.log("[ZombieContent] " + message)
    log.append(message)


def get_or_create(name, package_path, asset_class):
    full_path = "{0}/{1}".format(package_path, name)
    if unreal.EditorAssetLibrary.does_asset_exist(full_path):
        return unreal.EditorAssetLibrary.load_asset(full_path), False
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    asset = tools.create_asset(name, package_path, asset_class, None)
    return asset, True


def apply(asset, properties):
    for key, value in properties.items():
        asset.set_editor_property(key, value)


def save(asset):
    unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)


# ---------------------------------------------------------------- room modules
# Tile alphabet: '#' wall  '.' floor  'D' doorway (on an edge)  'O' obstacle
#                'S' zombie spawn  'P' player start  ' ' outside the module

ROOMS = [
    ("R_Start", unreal.RoomType.START, 1.0, 1, [
        "####D####",
        "#.......#",
        "#.......#",
        "#..OOO..#",
        "D...P...D",
        "#..OOO..#",
        "#.......#",
        "#.......#",
        "####D####",
    ]),
    ("R_Combat_Large", unreal.RoomType.COMBAT, 1.0, 1, [
        "######D######",
        "#...........#",
        "#..OO...OO..#",
        "#...........#",
        "#....S.S....#",
        "D...........D",
        "#....S.S....#",
        "#...........#",
        "#..OO...OO..#",
        "#...........#",
        "######D######",
    ]),
    ("R_Combat_Small", unreal.RoomType.COMBAT, 1.2, 1, [
        "####D####",
        "#.......#",
        "#.S...S.#",
        "#...O...#",
        "D..OOO..D",
        "#...O...#",
        "#.S...S.#",
        "#.......#",
        "####D####",
    ]),
    ("R_Combat_Pillars", unreal.RoomType.COMBAT, 0.9, 2, [
        "#####D#####",
        "#.........#",
        "#.O.....O.#",
        "#....S....#",
        "D.........D",
        "#..O...O..#",
        "D.........D",
        "#....S....#",
        "#.O.....O.#",
        "#.........#",
        "#####D#####",
    ]),
    ("R_Hall_Straight", unreal.RoomType.HALLWAY, 1.5, 1, [
        "#########",
        "D.......D",
        "#########",
    ]),
    ("R_Hall_Long", unreal.RoomType.HALLWAY, 1.0, 1, [
        "#############",
        "D...........D",
        "#############",
    ]),
    ("R_Hall_Corner", unreal.RoomType.HALLWAY, 1.1, 1, [
        "###D###",
        "#.....#",
        "#.....#",
        "#.....D",
        "#.....#",
        "#.....#",
        "#######",
    ]),
    ("R_DeadEnd", unreal.RoomType.DEAD_END, 0.6, 1, [
        "#######",
        "#..O..#",
        "#.....#",
        "D..S..#",
        "#.....#",
        "#..O..#",
        "#######",
    ]),
    ("R_Treasure", unreal.RoomType.TREASURE, 0.3, 1, [
        "#######",
        "#.....#",
        "#..O..#",
        "D.....#",
        "#..O..#",
        "#.....#",
        "#######",
    ]),
]

for name, room_type, weight, min_sector, layout in ROOMS:
    asset, created = get_or_create(name, ROOM_DIR, unreal.RoomTemplateDataAsset)
    if asset is None:
        note("FAILED to create room template " + name)
        continue
    apply(asset, {
        "room_type": room_type,
        "layout_rows": layout,
        "selection_weight": weight,
        "min_sector": min_sector,
        "allow_rotation": True,
    })
    save(asset)
    note("room template {0} ({1})".format(name, "created" if created else "updated"))

settings, created = get_or_create("DA_SectorGeneration", ROOM_DIR, unreal.SectorGenerationSettings)
if settings:
    apply(settings, {
        "tile_size": 250.0,
        "base_room_count": 7,
        "extra_rooms_per_sector": 0.75,
        "max_room_count": 16,
        "min_combat_rooms": 3,
    })
    save(settings)
    note("DA_SectorGeneration ({0})".format("created" if created else "updated"))

# ------------------------------------------------------------ zombie archetypes
# Spawn costs follow the design brief's worked example: Common 1, Runner 2, Tank 8.

ZOMBIES = [
    ("DA_Zombie_Common", {
        "display_name": "Common Zombie",
        "tier": unreal.ZombieClassTier.LOW,
        "spawn_cost": 1,
        "selection_weight": 1.0,
        "min_sector": 1,
        "base_max_health": 60.0,
        "base_attack_damage": 12.0,
        "attack_interval": 1.4,
        "attack_windup": 0.45,
        "attack_range": 130.0,
        "move_speed": 170.0,
        "money_reward": 10,
        "sight_radius": 1400.0,
        "lose_sight_radius": 1900.0,
        "peripheral_vision_half_angle": 80.0,
        "hearing_range": 3000.0,
        "memory_seconds": 6.0,
        "body_scale": 1.0,
        "debug_tint": unreal.LinearColor(0.13, 0.40, 0.16, 1.0),
    }),
    ("DA_Zombie_Runner", {
        "display_name": "Runner",
        "tier": unreal.ZombieClassTier.MEDIUM,
        "spawn_cost": 2,
        "selection_weight": 1.0,
        "min_sector": 1,
        "base_max_health": 45.0,
        "base_attack_damage": 16.0,
        "attack_interval": 1.0,
        "attack_windup": 0.28,
        "attack_range": 120.0,
        "move_speed": 430.0,
        "money_reward": 18,
        "sight_radius": 1800.0,
        "lose_sight_radius": 2400.0,
        "peripheral_vision_half_angle": 90.0,
        "hearing_range": 3500.0,
        "memory_seconds": 8.0,
        "body_scale": 0.85,
        "debug_tint": unreal.LinearColor(0.75, 0.72, 0.10, 1.0),
    }),
    ("DA_Zombie_Tank", {
        "display_name": "Tank",
        "tier": unreal.ZombieClassTier.HIGH,
        "spawn_cost": 8,
        "selection_weight": 1.0,
        "min_sector": 2,
        "base_max_health": 340.0,
        "base_attack_damage": 30.0,
        "attack_interval": 2.0,
        "attack_windup": 0.8,
        "attack_range": 190.0,
        "move_speed": 120.0,
        "money_reward": 60,
        "sight_radius": 1200.0,
        "lose_sight_radius": 1600.0,
        "peripheral_vision_half_angle": 70.0,
        "hearing_range": 2600.0,
        "memory_seconds": 10.0,
        "body_scale": 1.55,
        "debug_tint": unreal.LinearColor(0.45, 0.09, 0.09, 1.0),
    }),
]

for name, properties in ZOMBIES:
    asset, created = get_or_create(name, ZOMBIE_DIR, unreal.ZombieArchetypeDataAsset)
    if asset is None:
        note("FAILED to create zombie archetype " + name)
        continue
    apply(asset, properties)
    save(asset)
    note("zombie archetype {0} ({1})".format(name, "created" if created else "updated"))

director, created = get_or_create("DA_SpawnDirector", ZOMBIE_DIR, unreal.SpawnDirectorSettings)
if director:
    # Budget/tier-mix defaults come from the C++ CDO; only pacing is overridden here.
    apply(director, {
        "base_budget": 60,
        "budget_growth_per_sector": 1.25,
        "spawn_interval": 1.1,
        "max_concurrent_zombies": 40,
        "min_spawn_distance_from_player": 900.0,
    })
    save(director)
    note("DA_SpawnDirector ({0})".format("created" if created else "updated"))

# ------------------------------------------------------------------- test level
level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

level_subsystem.load_level(MAP_PATH)

# The old greybox floor plane is obsolete: generated room modules bring their own floors, and a
# world-spanning plane would both z-fight with them and let pawns walk outside the sector.
for actor in actor_subsystem.get_all_level_actors():
    if isinstance(actor, unreal.StaticMeshActor):
        note("removing legacy level geometry: " + actor.get_actor_label())
        actor_subsystem.destroy_actor(actor)

has_nav_bounds = any(isinstance(a, unreal.NavMeshBoundsVolume) for a in actor_subsystem.get_all_level_actors())
if not has_nav_bounds:
    volume = actor_subsystem.spawn_actor_from_class(unreal.NavMeshBoundsVolume, unreal.Vector(0.0, 0.0, 300.0))
    if volume:
        volume.set_actor_label("NavMeshBounds_Sector")
        volume.set_actor_scale3d(unreal.Vector(200.0, 200.0, 12.0))
        origin, extent = volume.get_actor_bounds(only_colliding_components=False)
        note("spawned NavMeshBoundsVolume, origin={0} extent={1}".format(origin, extent))
    else:
        note("FAILED to spawn NavMeshBoundsVolume")
else:
    note("NavMeshBoundsVolume already present")

level_subsystem.save_current_level()
note("saved level " + MAP_PATH)

with open(MARKER, "w") as handle:
    handle.write("\n".join(log))
