"""Room modules, sector themes, hazards, zombie archetypes, bosses, pacing settings and the maps."""

import unreal

import content_art
from content_lib import SHEET_INDEX, TEXTURE_INDEX, apply, effect, get_or_create, load, note, save, sound, tag
from content_gameplay import LOOT, hit_effect, projectile_spec

ROOM_DIR = "/Game/DataAssets/Rooms"
ZOMBIE_DIR = "/Game/DataAssets/Zombies"
THEME_DIR = "/Game/DataAssets/Themes"
HAZARD_DIR = "/Game/DataAssets/Hazards"

# Tile alphabet: '#' wall  '.' floor  'D' doorway (on an edge)  'O' obstacle  'o' optional obstacle
#                'S' zombie spawn  'P' player start  ' ' outside the module
R = unreal.RoomType
ROOMS = [
    ("R_Start", R.START, 1.0, 1, [
        "####D####",
        "#.......#",
        "#.o...o.#",
        "#..OOO..#",
        "D...P...D",
        "#..OOO..#",
        "#.o...o.#",
        "#.......#",
        "####D####",
    ]),
    ("R_Combat_Large", R.COMBAT, 1.0, 1, [
        "######D######",
        "#...........#",
        "#..OO...OO..#",
        "#.....o.....#",
        "#....S.S....#",
        "D..o.....o..D",
        "#....S.S....#",
        "#.....o.....#",
        "#..OO...OO..#",
        "#...........#",
        "######D######",
    ]),
    ("R_Combat_Small", R.COMBAT, 1.2, 1, [
        "####D####",
        "#.......#",
        "#.S...S.#",
        "#...O...#",
        "D..oOo..D",
        "#...O...#",
        "#.S...S.#",
        "#.......#",
        "####D####",
    ]),
    ("R_Combat_Pillars", R.COMBAT, 0.9, 2, [
        "#####D#####",
        "#.........#",
        "#.O.....O.#",
        "#....S....#",
        "D...o.o...D",
        "#..O...O..#",
        "D...o.o...D",
        "#....S....#",
        "#.O.....O.#",
        "#.........#",
        "#####D#####",
    ]),
    ("R_Combat_Offices", R.COMBAT, 1.0, 1, [
        "######D######",
        "#...........#",
        "#.oo.oo.oo..#",
        "#...........#",
        "#.oo.oo.oo..#",
        "D.....S.....D",
        "#.oo.oo.oo..#",
        "#...........#",
        "#.S.......S.#",
        "#...........#",
        "######D######",
    ]),
    ("R_Combat_Warehouse", R.COMBAT, 0.8, 3, [
        "#######D#######",
        "#.............#",
        "#.OO..ooo..OO.#",
        "#.OO.......OO.#",
        "#......S......#",
        "#..o.......o..#",
        "D.....OOO.....D",
        "#..o.......o..#",
        "#......S......#",
        "#.OO.......OO.#",
        "#.OO..ooo..OO.#",
        "#.............#",
        "#######D#######",
    ]),
    ("R_Hall_Straight", R.HALLWAY, 1.5, 1, [
        "#########",
        "D.......D",
        "#########",
    ]),
    ("R_Hall_Long", R.HALLWAY, 1.0, 1, [
        "#############",
        "D...........D",
        "#############",
    ]),
    ("R_Hall_Corner", R.HALLWAY, 1.1, 1, [
        "###D###",
        "#.....#",
        "#.....#",
        "#..o..D",
        "#.....#",
        "#.....#",
        "#######",
    ]),
    ("R_Hall_T", R.HALLWAY, 0.9, 1, [
        "#########",
        "D.......D",
        "#.o.....#",
        "####D####",
    ]),
    ("R_Hall_Cross", R.HALLWAY, 0.7, 2, [
        "###D###",
        "#.....#",
        "#.....#",
        "D..o..D",
        "#.....#",
        "#.....#",
        "###D###",
    ]),
    ("R_DeadEnd", R.DEAD_END, 0.6, 1, [
        "#######",
        "#..O..#",
        "#.....#",
        "D..S..#",
        "#.....#",
        "#..O..#",
        "#######",
    ]),
    ("R_Treasure", R.TREASURE, 0.3, 1, [
        "#######",
        "#.....#",
        "#..O..#",
        "D.....#",
        "#..O..#",
        "#.....#",
        "#######",
    ]),
    ("R_Boss_Arena", R.BOSS, 1.0, 1, [
        "#########D#########",
        "#.................#",
        "#..O...........O..#",
        "#.................#",
        "#.....o.....o.....#",
        "#.................#",
        "#.................#",
        "#..o...........o..#",
        "#.................#",
        "D........S........D",
        "#.................#",
        "#..o...........o..#",
        "#.................#",
        "#.................#",
        "#.....o.....o.....#",
        "#.................#",
        "#..O...........O..#",
        "#.................#",
        "#########D#########",
    ]),
    ("R_Shop", R.INTERMISSION, 1.0, 1, [
        "#####D#####",
        "#.........#",
        "#.O.....O.#",
        "#.........#",
        "#....P....D",
        "#.........#",
        "#.O.....O.#",
        "#.........#",
        "###########",
    ]),
]


def build_rooms():
    for name, room_type, weight, min_sector, layout in ROOMS:
        asset, _ = get_or_create(name, ROOM_DIR, unreal.RoomTemplateDataAsset)
        apply(asset, {"room_type": room_type, "layout_rows": layout, "selection_weight": weight, "min_sector": min_sector,
                      "allow_rotation": room_type != R.INTERMISSION})
        save(asset)

    settings, _ = get_or_create("DA_SectorGeneration", ROOM_DIR, unreal.SectorGenerationSettings)
    apply(settings, {"tile_size": 250.0, "base_room_count": 7, "extra_rooms_per_sector": 0.75, "max_room_count": 16, "min_combat_rooms": 3})
    save(settings)
    note("built %d room templates" % len(ROOMS))


def build_themes(world_materials):
    themes = [
        ("TH_Office", "Offices", 1, 1.0, (1.0, 0.86, 0.62), 0.4),
        ("TH_Industrial", "Factory Floor", 1, 1.0, (1.0, 0.62, 0.35), 0.5),
        ("TH_Lab", "Research Wing", 3, 0.8, (0.65, 0.9, 1.0), 0.3),
    ]
    for asset_name, display, min_sector, weight, light, flicker in themes:
        key = asset_name[3:]
        asset, _ = get_or_create(asset_name, THEME_DIR, unreal.RoomThemeDataAsset)
        apply(asset, {
            "display_name": display, "min_sector": min_sector, "selection_weight": weight,
            "floor_material": content_art.material_instance("MI_Floor_" + key, world_materials["Floor"], TEXTURE_INDEX["T_Floor_" + key]),
            "wall_material": content_art.material_instance("MI_Wall_" + key, world_materials["Wall"], TEXTURE_INDEX["T_Wall_" + key]),
            "obstacle_material": content_art.material_instance("MI_Obstacle_" + key, world_materials["Obstacle"], TEXTURE_INDEX["T_Obstacles_" + key]),
            "decoration_material": world_materials["Decoration"],
            "obstacle_variants": 4, "decoration_variants": 8, "decoration_chance": 0.16,
            "light_color": unreal.LinearColor(*light, 1.0), "light_intensity": 9000.0, "light_radius": 1700.0, "flicker_chance": flicker,
        })
        save(asset)
    note("built %d themes" % len(themes))


HAZARDS = {}


def build_hazards():
    for name, sheet, radius, lifetime, potency in (("HZ_Acid", "SS_AcidPuddle", 140.0, 5.0, 1.0),
                                                   ("HZ_Poison", "SS_PoisonPuddle", 110.0, 6.0, 0.8),
                                                   ("HZ_PoisonLarge", "SS_PoisonPuddle", 230.0, 9.0, 1.2)):
        asset, _ = get_or_create(name, HAZARD_DIR, unreal.ZombieHazardDataAsset)
        apply(asset, {"sprite": SHEET_INDEX[sheet], "radius": radius, "lifetime": lifetime, "status_tag": tag("Status.Poisoned"),
                      "potency": potency, "apply_interval": 0.5, "spawn_sound": sound("SFX_Acid_Splash", 0.6, count=2)})
        save(asset)
        HAZARDS[name] = asset


# --- zombies -------------------------------------------------------------------------------------

def ability(cls_name, owner, **props):
    cls = unreal.load_class(None, "/Script/ZombieGame." + cls_name)
    instance = unreal.new_object(cls, outer=owner)
    for key, value in props.items():
        instance.set_editor_property(key, value)
    return instance


def presentation(sheet, blood_color="red"):
    if blood_color == "green":
        return {"sprite_sheet": SHEET_INDEX[sheet], "hit_effect": effect("SS_AcidHit", 1.0, height=40.0),
                "death_effect": effect("SS_AcidHit", 2.2, height=20.0), "corpse_decal": effect("SS_DecalAcid", 1.2)}
    return {"sprite_sheet": SHEET_INDEX[sheet], "hit_effect": effect("SS_BloodHit", 1.0, height=40.0),
            "death_effect": effect("SS_BloodHit", 2.0, height=20.0), "corpse_decal": effect("SS_DecalBlood", 1.0)}


def voices(idle, alert, volume=0.8):
    return {"idle_sound": sound("SFX_Zombie_" + idle, volume * 0.6, 0.1, count=4), "alert_sound": sound("SFX_Zombie_" + alert, volume, 0.1, count=4),
            "attack_sound": sound("SFX_Zombie_Swipe", 0.6, 0.1, count=3), "hurt_sound": sound("SFX_Zombie_Hurt", 0.5, 0.15, count=3),
            "death_sound": sound("SFX_Zombie_Death", 0.7, 0.1, count=3)}


def perception(sight, hearing, memory, angle=85.0):
    return {"sight_radius": sight, "lose_sight_radius": sight * 1.3, "peripheral_vision_half_angle": angle, "hearing_range": hearing, "memory_seconds": memory}


def zombie(name, props, abilities_factory=None, directory=ZOMBIE_DIR):
    asset, _ = get_or_create(name, directory, unreal.ZombieArchetypeDataAsset)
    apply(asset, props)
    if abilities_factory:
        asset.set_editor_property("abilities", abilities_factory(asset))
    save(asset)
    return asset


def build_zombies():
    Tier, Profile = unreal.ZombieClassTier, unreal.ZombieBehaviorProfile
    common_loot, elite_loot = LOOT["LT_ZombieCommon"], LOOT["LT_ZombieElite"]

    def base(display, tier, cost, min_sector, hp, dmg, speed, reward, loot, sheet, scale=1.0, **extra):
        props = {"display_name": display, "tier": tier, "spawn_cost": cost, "selection_weight": 1.0, "min_sector": min_sector,
                 "base_max_health": hp, "base_attack_damage": dmg, "move_speed": speed, "money_reward": reward, "loot_table": loot,
                 "body_scale": scale, "attack_interval": 1.4, "attack_windup": 0.45, "attack_range": 130.0}
        props.update(extra)
        return props

    zombie("DA_Zombie_Common", {**base("Shambler", Tier.LOW, 1, 1, 60.0, 12.0, 170.0, 10, common_loot, "SS_Zombie_Common"),
                                **presentation("SS_Zombie_Common"), **voices("Groan", "Groan"), **perception(2600.0, 3500.0, 8.0)})
    zombie("DA_Zombie_Runner", {**base("Runner", Tier.MEDIUM, 2, 1, 42.0, 15.0, 440.0, 18, common_loot, "SS_Zombie_Runner", 0.9,
                                       attack_interval=1.0, attack_windup=0.28, attack_range=120.0),
                                **presentation("SS_Zombie_Runner"), **voices("Shriek", "Shriek"), **perception(3200.0, 4000.0, 10.0, 90.0)})
    zombie("DA_Zombie_Tank", {**base("Tank", Tier.HIGH, 8, 2, 360.0, 32.0, 125.0, 60, elite_loot, "SS_Zombie_Tank", 1.5,
                                     attack_interval=2.0, attack_windup=0.8, attack_range=190.0),
                              **presentation("SS_Zombie_Tank"), **voices("Roar", "Roar", 1.0), **perception(2200.0, 3000.0, 12.0, 75.0)})

    zombie("DA_Zombie_Lobber", {**base("Lobber", Tier.MEDIUM, 4, 2, 70.0, 10.0, 160.0, 26, common_loot, "SS_Zombie_Lobber", 1.05,
                                       behavior_profile=Profile.RANGED, preferred_range=900.0),
                                **presentation("SS_Zombie_Lobber", "green"), **voices("Gurgle", "Gurgle"), **perception(2800.0, 3500.0, 9.0)},
           lambda owner: [ability("ZombieAbility_Projectile", owner, ability_tag=tag("Ability.RangedThrow"), cooldown=3.2, initial_delay=1.5,
                                  min_range=300.0, max_range=1500.0, busy_duration=0.9, damage_multiplier=1.0, lobbed=True, lob_scatter=90.0,
                                  projectile=projectile_spec("SS_Proj_Acid", 1100.0, 18.0, gravity=1.0, lifetime=4.0,
                                                             impact=effect("SS_AcidHit", 1.6), hazard=HAZARDS["HZ_Acid"]),
                                  hit_effects=[hit_effect("Status.Poisoned", 1.0)], sound=sound("SFX_Zombie_Gurgle", 0.7, count=4))])

    zombie("DA_Zombie_Exploder", {**base("Exploder", Tier.MEDIUM, 5, 2, 55.0, 0.0, 330.0, 24, common_loot, "SS_Zombie_Exploder", 1.0),
                                  **presentation("SS_Zombie_Exploder"), **voices("Shriek", "Shriek"), **perception(2800.0, 3600.0, 10.0)},
           lambda owner: [ability("ZombieAbility_Explode", owner, detonate_on_contact=True, max_range=150.0, cooldown=0.1, initial_delay=0.5,
                                  requires_line_of_sight=False, busy_duration=0.1, radius=330.0, damage=45.0,
                                  explosion_effect=effect("SS_Explosion", 0.9, height=20.0), scorch_decal=effect("SS_DecalScorch", 1.0),
                                  explosion_sound=sound("SFX_Explosion", 1.0, count=3))])

    zombie("DA_Zombie_Poison", {**base("Plague Carrier", Tier.MEDIUM, 3, 3, 80.0, 10.0, 175.0, 22, common_loot, "SS_Zombie_Poison", 1.0),
                                **presentation("SS_Zombie_Poison", "green"), **voices("Gurgle", "Groan"), **perception(2600.0, 3500.0, 8.0)},
           lambda owner: [ability("ZombieAbility_HazardTrail", owner, trail_hazard=HAZARDS["HZ_Poison"], trail_interval=2.2,
                                  death_hazard=HAZARDS["HZ_PoisonLarge"])])

    bullet = tag("Damage.Bullet")
    explosive = tag("Damage.Explosion")
    zombie("DA_Zombie_Armored", {**base("Riot Zombie", Tier.HIGH, 6, 4, 210.0, 20.0, 150.0, 45, elite_loot, "SS_Zombie_Armored", 1.15,
                                        damage_resistances={bullet: 0.5, explosive: 0.2}),
                                 **presentation("SS_Zombie_Armored"), **voices("Groan", "Roar"), **perception(2400.0, 3200.0, 10.0)})

    zombie("DA_Zombie_Necromancer", {**base("Necromancer", Tier.HIGH, 10, 5, 170.0, 14.0, 190.0, 80, elite_loot, "SS_Zombie_Necromancer", 1.1,
                                            behavior_profile=Profile.RANGED, preferred_range=1050.0),
                                     **presentation("SS_Zombie_Necromancer"), **voices("Whisper", "Whisper"), **perception(3000.0, 4000.0, 12.0)},
           lambda owner: [ability("ZombieAbility_Revive", owner, ability_tag=tag("Ability.Revive"), cooldown=7.0, initial_delay=3.0,
                                  requires_target=False, busy_duration=1.2, revive_radius=950.0, max_revived_per_cast=3, animation="Attack",
                                  cast_effect=effect("SS_ReviveGlow", 3.0), sound=sound("SFX_Necro_Cast", 0.8, count=2)),
                          ability("ZombieAbility_Projectile", owner, ability_tag=tag("Ability.RangedThrow"), cooldown=2.4, initial_delay=2.0,
                                  min_range=200.0, max_range=1700.0, busy_duration=0.6, damage_multiplier=1.0, count=3, fan_degrees=24.0,
                                  projectile=projectile_spec("SS_Proj_Bolt", 1400.0, 14.0, impact=effect("SS_ReviveGlow", 0.6)),
                                  sound=sound("SFX_Necro_Cast", 0.5, count=2))])
    note("built zombie archetypes")


def build_bosses():
    Tier, Profile = unreal.ZombieClassTier, unreal.ZombieBehaviorProfile
    common = load(ZOMBIE_DIR + "/DA_Zombie_Common")
    runner = load(ZOMBIE_DIR + "/DA_Zombie_Runner")
    slam_fx = effect("SS_SlamRing", 1.8, height=10.0)
    roar = sound("SFX_Boss_Roar", 1.0, 0.05, count=2)

    boss_common = {"tier": Tier.BOSS, "spawn_cost": 20, "behavior_profile": Profile.BOSS, "phase_health_thresholds": [0.66, 0.33],
                   "phase_speed_multiplier": 1.2, "loot_table": LOOT["LT_ZombieElite"], "alert_sound": roar,
                   "hurt_sound": sound("SFX_Zombie_Hurt", 0.7, 0.15, count=3), "death_sound": roar,
                   "idle_sound": sound("SFX_Zombie_Roar", 0.7, 0.1, count=4), "attack_sound": sound("SFX_Zombie_Swipe", 0.9, count=3),
                   **perception(5000.0, 6000.0, 30.0, 180.0)}

    zombie("DA_Boss_Butcher", {**boss_common, **presentation("SS_Boss_Butcher"), "display_name": "THE BUTCHER", "min_sector": 5,
                               "base_max_health": 3000.0, "base_attack_damage": 34.0, "move_speed": 215.0, "money_reward": 600, "body_scale": 1.5,
                               "attack_interval": 1.3, "attack_windup": 0.5, "attack_range": 200.0},
           lambda owner: [
               ability("ZombieAbility_Summon", owner, ability_tag=tag("Ability.Summon"), min_phase=1, cooldown=12.0, initial_delay=4.0,
                       requires_target=False, busy_duration=1.0, minion_archetype=common, count_per_cast=5, max_alive_minions=10,
                       cast_effect=effect("SS_ReviveGlow", 4.0), sound=roar),
               ability("ZombieAbility_Charge", owner, ability_tag=tag("Ability.Charge"), cooldown=6.0, initial_delay=3.0, min_range=450.0,
                       max_range=1500.0, busy_duration=1.4, charge_speed=1700.0, damage=38.0, knockback=900.0, windup=0.55, sound=roar),
               ability("ZombieAbility_Slam", owner, ability_tag=tag("Ability.Slam"), cooldown=4.5, initial_delay=2.0, max_range=320.0,
                       requires_line_of_sight=False, busy_duration=1.2, radius=400.0, damage=30.0, windup=0.7, impact_effect=slam_fx,
                       impact_sound=sound("SFX_Explosion", 0.8, count=3)),
           ])

    zombie("DA_Boss_PlagueMother", {**boss_common, **presentation("SS_Boss_PlagueMother", "green"), "display_name": "PLAGUE MOTHER",
                                    "min_sector": 10, "base_max_health": 2700.0, "base_attack_damage": 26.0, "move_speed": 170.0,
                                    "money_reward": 700, "body_scale": 1.6, "attack_range": 190.0},
           lambda owner: [
               ability("ZombieAbility_HazardTrail", owner, trail_hazard=HAZARDS["HZ_Poison"], trail_interval=1.5, death_hazard=HAZARDS["HZ_PoisonLarge"]),
               ability("ZombieAbility_Summon", owner, ability_tag=tag("Ability.Summon"), min_phase=1, cooldown=10.0, initial_delay=5.0,
                       requires_target=False, busy_duration=1.0, minion_archetype=runner, count_per_cast=3, max_alive_minions=8,
                       cast_effect=effect("SS_AcidHit", 5.0), sound=roar),
               ability("ZombieAbility_Slam", owner, ability_tag=tag("Ability.Slam"), min_phase=2, cooldown=5.0, initial_delay=1.0,
                       max_range=340.0, requires_line_of_sight=False, busy_duration=1.1, radius=420.0, damage=28.0, windup=0.6,
                       impact_effect=effect("SS_SlamRing", 1.9, (0.5, 1.0, 0.3, 1.0)), impact_sound=sound("SFX_Explosion", 0.7, count=3)),
               ability("ZombieAbility_Projectile", owner, ability_tag=tag("Ability.RangedThrow"), cooldown=3.6, initial_delay=2.0, min_range=250.0,
                       max_range=1800.0, busy_duration=1.0, damage_multiplier=0.8, count=5, fan_degrees=60.0, lobbed=True, lob_scatter=60.0,
                       projectile=projectile_spec("SS_Proj_Acid", 1100.0, 20.0, gravity=1.0, lifetime=4.0, impact=effect("SS_AcidHit", 1.8),
                                                  hazard=HAZARDS["HZ_Acid"], scale=1.4),
                       hit_effects=[hit_effect("Status.Poisoned", 1.0)], sound=sound("SFX_Zombie_Gurgle", 0.9, count=4)),
           ])
    note("built bosses")


def build_pacing():
    director, _ = get_or_create("DA_SpawnDirector", ZOMBIE_DIR, unreal.SpawnDirectorSettings)
    apply(director, {"base_budget": 55, "budget_growth_per_sector": 1.22, "health_increase_per_sector": 0.16,
                     "damage_increase_per_sector": 0.1, "reward_increase_per_sector": 0.1, "spawn_interval": 0.9,
                     "max_concurrent_zombies": 45, "min_spawn_distance_from_player": 900.0})
    save(director)

    ai_settings, _ = get_or_create("DA_ZombieAI", ZOMBIE_DIR, unreal.ZombieAISettings)
    apply(ai_settings, {"roam_radius": 800.0, "idle_time": 1.5, "idle_time_deviation": 1.0, "investigate_look_around_time": 1.5,
                        "flow_field_rebuild_interval": 0.25, "use_flow_field_for_chase": True})
    save(ai_settings)


# --- maps ----------------------------------------------------------------------------------------

def build_maps():
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    if not unreal.EditorAssetLibrary.does_asset_exist("/Game/Maps/L_MainMenu"):
        levels.new_level("/Game/Maps/L_MainMenu")
        levels.save_current_level()
        note("created L_MainMenu")

    levels.load_level("/Game/Maps/L_TestSector")
    for actor in actors.get_all_level_actors():
        if isinstance(actor, unreal.DirectionalLight):
            component = actor.get_component_by_class(unreal.DirectionalLightComponent)
            component.set_editor_property("intensity", 2.2)
            component.set_editor_property("light_color", unreal.Color(170, 180, 215, 255))
            actor.set_actor_rotation(unreal.Rotator(0.0, -55.0, 35.0), False)
        elif isinstance(actor, unreal.SkyLight):
            component = actor.get_component_by_class(unreal.SkyLightComponent)
            component.set_editor_property("intensity", 0.6)
            component.set_editor_property("lower_hemisphere_color", unreal.LinearColor(0.05, 0.05, 0.07, 1.0))
        elif isinstance(actor, unreal.NavigationData):
            # Never let a baked (empty) navmesh be saved into this runtime-built level (CLAUDE.md gotcha #4).
            actors.destroy_actor(actor)
    levels.save_current_level()
    note("updated L_TestSector lighting")


def run(world_materials):
    build_rooms()
    build_themes(world_materials)
    build_hazards()
    build_zombies()
    build_bosses()
    build_pacing()
    build_maps()
