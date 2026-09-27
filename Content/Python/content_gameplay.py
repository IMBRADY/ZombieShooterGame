"""Weapons, perks, status effects, loot tables, shop/run settings and achievements - the game's
balance, authored as Data Assets."""

import unreal

from content_lib import SHEET_INDEX, apply, effect, get_or_create, note, save, sound, tag

WEAPON_DIR = "/Game/DataAssets/Weapons"
PERK_DIR = "/Game/DataAssets/Perks"
STATUS_DIR = "/Game/DataAssets/StatusEffects"
LOOT_DIR = "/Game/DataAssets/Loot"

HITSCAN = unreal.load_class(None, "/Script/ZombieGame.WeaponFireMode_Hitscan")
PROJECTILE = unreal.load_class(None, "/Script/ZombieGame.WeaponFireMode_Projectile")
MELEE = unreal.load_class(None, "/Script/ZombieGame.WeaponFireMode_Melee")
FIRE_DAMAGE = unreal.load_class(None, "/Script/ZombieGame.DamageType_Fire")
POISON_DAMAGE = unreal.load_class(None, "/Script/ZombieGame.DamageType_Poison")

WEAPONS = {}
PERKS = {}
LOOT = {}


def stats(**values):
    block = unreal.WeaponStats()
    for key, value in values.items():
        block.set_editor_property(key, value)
    return block


def hit_effect(status, chance, potency=1.0):
    spec = unreal.HitEffectSpec()
    spec.set_editor_property("status_tag", tag(status))
    spec.set_editor_property("chance", chance)
    spec.set_editor_property("potency", potency)
    return spec


def projectile_spec(sheet, speed, radius=14.0, gravity=0.0, lifetime=3.0, impact=None, decal=None, impact_sound=None, hazard=None, scale=1.0):
    spec = unreal.ZombieProjectileSpec()
    apply(spec, {"sprite": SHEET_INDEX[sheet], "speed": speed, "collision_radius": radius, "gravity_scale": gravity,
                 "lifetime": lifetime, "sprite_scale": scale})
    if impact:
        spec.set_editor_property("impact_effect", impact)
    if decal:
        spec.set_editor_property("impact_decal", decal)
    if impact_sound:
        spec.set_editor_property("impact_sound", impact_sound)
    if hazard:
        spec.set_editor_property("impact_hazard", hazard)
    return spec


# --- status effects ------------------------------------------------------------------------------

def build_status_effects():
    for name, props in (
        ("SE_Burning", {"status_tag": tag("Status.Burning"), "display_name": "Burning", "damage_per_second": 10.0, "duration": 3.0,
                        "max_stacks": 3, "damage_type": FIRE_DAMAGE, "tint": unreal.LinearColor(1.0, 0.55, 0.25, 1.0),
                        "tick_effect": effect("Sparks", 1.3, (1.0, 0.5, 0.1, 1.0), height=40.0)}),
        ("SE_Poisoned", {"status_tag": tag("Status.Poisoned"), "display_name": "Poisoned", "damage_per_second": 6.0, "duration": 5.0,
                         "max_stacks": 4, "damage_type": POISON_DAMAGE, "move_speed_multiplier": 0.8,
                         "tint": unreal.LinearColor(0.55, 1.0, 0.4, 1.0)}),
    ):
        asset, _ = get_or_create(name, STATUS_DIR, unreal.StatusEffectDataAsset)
        apply(asset, props)
        save(asset)
    note("built status effects")


# --- weapons -------------------------------------------------------------------------------------

def build_weapons():
    rarity, _ = get_or_create("DA_WeaponRarity", WEAPON_DIR, unreal.WeaponRaritySettings)
    apply(rarity, {"sell_value_fraction": 0.4})
    save(rarity)

    muzzle = effect("MuzzleFlash", 1.0, random_yaw=False)
    big_muzzle = effect("MuzzleFlash", 1.8, random_yaw=False)
    sparks = effect("Sparks", 1.0)
    explosion = effect("Explosion", 1.0, height=20.0)
    scorch = effect("DecalScorch", 1.0)
    reload = sound("SFX_Reload", 0.7, 0.05, count=2)
    empty = sound("SFX_Empty", 0.6, 0.05)

    def weapon(asset_name, name, description, category, frame, fire_sound, base, *, automatic=False, projectile=None,
               price=200, upgrade=350, refill=60, weight=1.0, min_sector=1, shop=True, loot=True, tracer=(1.0, 0.85, 0.4),
               tracer_width=10.0, shake=0.1, flash=None, effects=None, legendary=None, muzzle_offset=75.0, fire_mode=None,
               extra=None):
        asset, _ = get_or_create(asset_name, WEAPON_DIR, unreal.WeaponDataAsset)
        apply(asset, {
            "display_name": name, "description": description, "category": category, "base_stats": base, "automatic": automatic,
            "fire_mode": fire_mode or (PROJECTILE if projectile else HITSCAN), "base_price": price, "base_upgrade_price": upgrade,
            "ammo_refill_price": refill, "shop_weight": weight, "min_sector": min_sector, "available_in_shop": shop,
            "can_drop_as_loot": loot, "held_sprite_frame": frame, "muzzle_offset": muzzle_offset, "muzzle_flash": flash or muzzle,
            "impact_effect": sparks, "draw_tracers": projectile is None, "tracer_color": unreal.LinearColor(*tracer, 1.0),
            "tracer_width": tracer_width, "shake_strength": shake, "fire_sound": fire_sound, "reload_sound": reload, "empty_sound": empty,
            "hit_effects": effects or [], "legendary_hit_effects": legendary or [hit_effect("Status.Burning", 0.3)],
        })
        if projectile:
            asset.set_editor_property("projectile", projectile)
        if extra:
            apply(asset, extra)
        save(asset)
        WEAPONS[asset_name] = asset

    cat = unreal.WeaponCategory
    weapon("W_Pistol", "M9 Pistol", "Reliable sidearm. Never runs dry for long.", cat.PISTOL, 0, sound("SFX_Pistol_Fire", 0.8, count=3),
           stats(damage=22.0, shots_per_second=4.5, magazine_size=12, max_reserve_ammo=96, reload_time=1.1, spread_degrees=2.0, crit_chance=0.06,
                 range=3200.0, noise_range=3000.0), price=150, shop=False, loot=False, shake=0.06)
    weapon("W_Revolver", ".44 Revolver", "Six shots. Each one counts - and punches through.", cat.PISTOL, 7, sound("SFX_Revolver_Fire", 0.9, count=3),
           stats(damage=58.0, shots_per_second=1.8, magazine_size=6, max_reserve_ammo=42, reload_time=2.0, spread_degrees=0.8, crit_chance=0.15,
                 crit_multiplier=2.5, pierce=1, range=3800.0), price=320, upgrade=380, refill=50, shake=0.2)
    weapon("W_SMG", "Vector SMG", "Hose the hallway. Burns ammo fast.", cat.SMG, 1, sound("SFX_SMG_Fire", 0.6, count=3),
           stats(damage=14.0, shots_per_second=11.0, magazine_size=30, max_reserve_ammo=210, reload_time=1.6, spread_degrees=6.0, crit_chance=0.04,
                 range=2800.0), automatic=True, price=380, refill=80, weight=1.3, shake=0.05)
    weapon("W_Shotgun", "Pump Shotgun", "Eight pellets of bad news up close.", cat.SHOTGUN, 2, sound("SFX_Shotgun_Fire", 1.0, count=3),
           stats(damage=13.0, pellets_per_shot=8, shots_per_second=1.3, magazine_size=6, max_reserve_ammo=42, reload_time=2.2, spread_degrees=11.0,
                 crit_chance=0.03, range=1500.0), price=420, refill=70, weight=1.2, shake=0.35, flash=big_muzzle)
    weapon("W_AutoShotgun", "Street Sweeper", "A drum-fed shotgun that doesn't stop.", cat.SHOTGUN, 2, sound("SFX_Shotgun_Fire", 0.9, count=3),
           stats(damage=11.0, pellets_per_shot=7, shots_per_second=3.2, magazine_size=12, max_reserve_ammo=72, reload_time=2.6, spread_degrees=13.0,
                 range=1300.0), automatic=True, price=780, refill=110, min_sector=4, shake=0.3, flash=big_muzzle,
           legendary=[hit_effect("Status.Poisoned", 0.25)])
    weapon("W_Rifle", "AR-15 Rifle", "Accurate, automatic, dependable.", cat.RIFLE, 3, sound("SFX_Rifle_Fire", 0.8, count=3),
           stats(damage=26.0, shots_per_second=7.5, magazine_size=30, max_reserve_ammo=180, reload_time=1.9, spread_degrees=3.0, crit_chance=0.07,
                 range=3800.0), automatic=True, price=620, refill=95, min_sector=2, weight=1.2, shake=0.1)
    weapon("W_Sniper", "Longshot Sniper", "One shot, a whole line of them.", cat.SNIPER, 4, sound("SFX_Sniper_Fire", 1.0, count=3),
           stats(damage=145.0, shots_per_second=0.9, magazine_size=5, max_reserve_ammo=30, reload_time=2.6, spread_degrees=0.2, crit_chance=0.25,
                 crit_multiplier=2.5, pierce=3, range=6500.0, noise_range=4500.0), price=820, upgrade=600, refill=90, min_sector=3,
           tracer=(0.8, 1.0, 1.0), tracer_width=14.0, shake=0.4, flash=big_muzzle)
    weapon("W_Rocket", "RPG Launcher", "Clears rooms. Mind the splash.", cat.ROCKET_LAUNCHER, 5, sound("SFX_Rocket_Fire", 1.0, count=2),
           stats(damage=160.0, shots_per_second=0.7, magazine_size=1, max_reserve_ammo=12, reload_time=2.4, spread_degrees=1.0, crit_chance=0.0,
                 explosion_radius=380.0, range=5000.0, noise_range=5000.0),
           projectile=projectile_spec("SS_Proj_Rocket", 1900.0, 16.0, impact=explosion, decal=scorch, impact_sound=sound("SFX_Explosion", 1.0, count=3), scale=1.2),
           price=1100, upgrade=800, refill=180, min_sector=4, weight=0.6, shake=0.5, muzzle_offset=90.0)
    weapon("W_Plasma", "Plasma Caster", "Superheated bolts that pass through bodies.", cat.SPECIAL, 6, sound("SFX_Plasma_Fire", 0.7, count=2),
           stats(damage=40.0, shots_per_second=4.0, magazine_size=20, max_reserve_ammo=100, reload_time=1.8, spread_degrees=2.0, crit_chance=0.08,
                 pierce=2, range=4200.0),
           projectile=projectile_spec("SS_Proj_Plasma", 3200.0, 14.0, impact=effect("Sparks", 1.6, (0.4, 0.9, 1.0, 1.0))),
           automatic=True, price=950, upgrade=700, refill=120, min_sector=5, weight=0.8, shake=0.12)
    weapon("W_Incinerator", "Incinerator", "Short range. Sets everything alight.", cat.SPECIAL, 6, sound("SFX_Flame_Fire", 0.6, count=2),
           stats(damage=6.0, shots_per_second=14.0, magazine_size=60, max_reserve_ammo=240, reload_time=2.2, spread_degrees=9.0, range=950.0,
                 crit_chance=0.0), automatic=True, price=880, refill=100, min_sector=3, weight=0.8, tracer=(1.0, 0.45, 0.1), tracer_width=16.0,
           shake=0.04, effects=[hit_effect("Status.Burning", 0.35)])
    weapon("W_Ripper", "M60 Ripper", "A boss's prize: a hundred-round belt of fury.", cat.RIFLE, 3, sound("SFX_Rifle_Fire", 0.9, count=3),
           stats(damage=30.0, shots_per_second=10.0, magazine_size=100, max_reserve_ammo=300, reload_time=3.4, spread_degrees=4.5, crit_chance=0.08,
                 pierce=1, range=4000.0), automatic=True, price=1500, upgrade=900, refill=200, min_sector=5, shop=False, shake=0.14,
           legendary=[hit_effect("Status.Burning", 0.4), hit_effect("Status.Poisoned", 0.2)])
    # Always carried in its own melee slot (DA_RunSettings.melee_weapon) - never sold, traded or
    # dropped. One-shots shamblers at any sector; a few stabs for anything bigger.
    weapon("W_Shank", "Shank", "A sharpened scrap of steel. Never runs dry.", cat.MELEE, 8, sound("SFX_Shank_Swing", 0.7, 0.08, count=3),
           stats(damage=35.0, shots_per_second=1.6, magazine_size=1, max_reserve_ammo=0, reload_time=0.05, spread_degrees=0.0,
                 crit_chance=0.1, crit_multiplier=1.5, range=150.0, noise_range=500.0),
           automatic=True, price=0, upgrade=0, refill=0, weight=0.0, shop=False, loot=False, shake=0.08, muzzle_offset=40.0,
           flash=effect(), fire_mode=MELEE,
           extra={"uses_ammo": False, "draw_tracers": False, "impact_effect": effect("BloodHit", 1.3),
                  "melee_arc_half_angle": 70.0, "one_hit_kill_tiers": [unreal.ZombieClassTier.LOW]})
    note("built %d weapons" % len(WEAPONS))


# --- perks ---------------------------------------------------------------------------------------

def modifier(stat, value, multiply=True):
    m = unreal.StatModifier()
    m.set_editor_property("stat", tag(stat))
    m.set_editor_property("op", unreal.StatModifierOp.MULTIPLY if multiply else unreal.StatModifierOp.ADD)
    m.set_editor_property("value", value)
    return m


def build_perks():
    def perk(asset_name, name, description, color, max_tier, base_cost, modifiers=None, effects=None, growth=1.8, rare=False, weight=1.0):
        asset, _ = get_or_create(asset_name, PERK_DIR, unreal.PerkDataAsset)
        apply(asset, {"display_name": name, "description": description, "icon_tint": unreal.LinearColor(*color, 1.0), "max_tier": max_tier,
                      "base_cost": base_cost, "cost_growth": growth, "rare": rare, "shop_weight": weight,
                      "modifiers_per_tier": modifiers or [], "hit_effects_per_tier": effects or []})
        save(asset)
        PERKS[asset_name] = asset

    perk("P_SpeedBoost", "Speed Boost", "+{0} movement speed.", (0.5, 0.9, 1.0), 5, 220, [modifier("Stat.Move.Speed", 0.07)])
    perk("P_ReloadSpeed", "Fast Hands", "+{0} reload speed.", (0.9, 0.9, 0.5), 5, 200, [modifier("Stat.Weapon.ReloadSpeed", 0.12)])
    perk("P_ExtraHealth", "Thick Skin", "+{0} maximum health.", (1.0, 0.4, 0.4), 5, 260, [modifier("Stat.Health.Max", 20.0, False)])
    perk("P_LifeSteal", "Vampire Rounds", "{0} of damage dealt heals you.", (0.8, 0.2, 0.3), 5, 350, [modifier("Stat.Hit.LifeSteal", 0.02, False)])
    perk("P_Ricochet", "Ricochet", "Bullets bounce off walls {0} extra time(s).", (0.7, 0.7, 1.0), 3, 400, [modifier("Stat.Weapon.Ricochet", 1.0, False)], growth=2.2)
    perk("P_Piercing", "Piercing Rounds", "Bullets pass through {0} extra zombie(s).", (0.9, 0.7, 0.4), 3, 380, [modifier("Stat.Weapon.Pierce", 1.0, False)], growth=2.2)
    perk("P_FireDamage", "Incendiary Ammo", "{0} chance per hit to set zombies on fire.", (1.0, 0.5, 0.1), 5, 320, effects=[hit_effect("Status.Burning", 0.12)])
    perk("P_PoisonBullets", "Toxic Ammo", "{0} chance per hit to poison zombies.", (0.5, 1.0, 0.3), 5, 300, effects=[hit_effect("Status.Poisoned", 0.12)])
    perk("P_CriticalChance", "Sharpshooter", "+{0} critical hit chance.", (1.0, 0.85, 0.3), 5, 280, [modifier("Stat.Weapon.CritChance", 0.04, False)])
    perk("P_DoubleMoney", "Scavenger", "+{0} money from every pickup.", (1.0, 0.8, 0.2), 5, 450, [modifier("Stat.Economy.MoneyMultiplier", 0.2)], growth=1.9)
    perk("P_SprintEfficiency", "Marathon", "Sprinting uses {0} less stamina.", (0.9, 0.9, 0.2), 4, 180, [modifier("Stat.Stamina.DrainRate", -0.15)])
    perk("P_MagazineCapacity", "Extended Mags", "+{0} magazine and reserve capacity.", (0.6, 0.8, 0.6), 5, 260, [modifier("Stat.Weapon.MagazineSize", 0.15)])
    perk("P_ExplosionResistance", "Blast Padding", "{0} less damage from explosions.", (0.8, 0.5, 0.3), 5, 200, [modifier("Stat.Resist.Explosion", 0.12, False)])
    perk("P_Magnet", "Magnet", "+{0} money pickup radius.", (0.7, 0.9, 0.9), 3, 150, [modifier("Stat.Pickup.Radius", 120.0, False)])
    perk("P_Deadeye", "Deadeye", "+{0} damage with every weapon.", (1.0, 0.3, 0.2), 3, 600, [modifier("Stat.Weapon.Damage", 0.15)], rare=True)
    perk("P_Adrenaline", "Adrenaline", "+{0} fire rate with every weapon.", (1.0, 0.6, 0.9), 3, 600, [modifier("Stat.Weapon.FireRate", 0.12)], rare=True)
    note("built %d perks" % len(PERKS))


# --- loot ----------------------------------------------------------------------------------------

def entry(kind, weight, low=0, high=0, weapon=None, minimum=None, rare_perk=False, scale=True):
    e = unreal.LootEntry()
    apply(e, {"type": kind, "weight": weight, "min_amount": low, "max_amount": high, "rare_perk_only": rare_perk, "scale_with_rewards": scale})
    if weapon:
        e.set_editor_property("weapon", weapon)
    if minimum is not None:
        e.set_editor_property("minimum_rarity", minimum)
    return e


def build_loot():
    L = unreal.LootRewardType
    R = unreal.WeaponRarity
    tables = {
        "LT_ZombieCommon": (1, 88.0, [entry(L.HEALTH, 3, 8, 14), entry(L.AMMO, 5, 12, 25, scale=False), entry(L.ARMOR, 1, 8, 15)]),
        "LT_ZombieElite": (1, 45.0, [entry(L.HEALTH, 18, 15, 25), entry(L.AMMO, 25, 25, 40, scale=False), entry(L.ARMOR, 10, 15, 25),
                                      entry(L.WEAPON, 3)]),
        "LT_Boss": (2, 0.0, [entry(L.WEAPON, 4, minimum=R.LEGENDARY), entry(L.PERK, 3, rare_perk=True), entry(L.MONEY, 3, 400, 700),
                             entry(L.HEALTH, 2, 50, 50, scale=False)]),
        "LT_TreasureChest": (2, 0.0, [entry(L.WEAPON, 3, minimum=R.RARE), entry(L.MONEY, 4, 120, 250), entry(L.PERK, 1),
                                      entry(L.ARMOR, 2, 50, 50, scale=False), entry(L.AMMO, 3, 60, 60, scale=False), entry(L.HEALTH, 2, 40, 40, scale=False)]),
        "LT_MysteryBox": (1, 0.0, [entry(L.WEAPON, 30), entry(L.WEAPON, 3, minimum=R.LEGENDARY), entry(L.AMMO, 20, 100, 100, scale=False),
                                   entry(L.MONEY, 15, 100, 400, scale=False), entry(L.ARMOR, 15, 50, 75, scale=False),
                                   entry(L.HEALTH, 15, 40, 70, scale=False), entry(L.PERK, 5)]),
    }
    for name, (rolls, nothing, entries) in tables.items():
        asset, _ = get_or_create(name, LOOT_DIR, unreal.LootTableDataAsset)
        apply(asset, {"rolls": rolls, "nothing_weight": nothing, "entries": entries})
        save(asset)
        LOOT[name] = asset
    note("built %d loot tables" % len(LOOT))


# --- run / shop settings -------------------------------------------------------------------------

def build_settings():
    shop, _ = get_or_create("DA_Shop", "/Game/DataAssets/Shop", unreal.ShopSettings)
    apply(shop, {"min_weapon_offers": 1, "max_weapon_offers": 3, "min_perk_offers": 1, "max_perk_offers": 3, "armor_amount": 50,
                 "armor_price": 175, "med_kit_heal_amount": 50, "med_kit_price": 150, "mystery_box_price": 450,
                 "mystery_box_table": LOOT["LT_MysteryBox"], "slot_upgrade_base_price": 900, "slot_upgrade_growth": 2.0,
                 "starting_slots": 3, "price_increase_per_sector": 0.06})
    save(shop)

    run, _ = get_or_create("DA_RunSettings", "/Game/DataAssets", unreal.ZombieRunSettings)
    apply(run, {"starting_weapons": [WEAPONS["W_Pistol"]], "melee_weapon": WEAPONS["W_Shank"], "starting_money": 0, "boss_sector_interval": 5,
                "boss_reward_table": LOOT["LT_Boss"], "difficulty_increase_per_sector": 0.15})
    save(run)
    note("built shop and run settings")


# --- achievements --------------------------------------------------------------------------------

def build_achievements():
    S = unreal.AchievementStat
    lifetime, run = unreal.AchievementScope.LIFETIME, unreal.AchievementScope.SINGLE_RUN
    achievements = [
        ("ACH_FirstBlood", "First Blood", "Kill your first zombie.", S.KILLS, lifetime, 1, False),
        ("ACH_Exterminator", "Exterminator", "Kill 500 zombies.", S.KILLS, lifetime, 500, False),
        ("ACH_Genocide", "Walking Apocalypse", "Kill 5,000 zombies.", S.KILLS, lifetime, 5000, False),
        ("ACH_Massacre", "Massacre", "Kill 200 zombies in a single run.", S.KILLS, run, 200, False),
        ("ACH_Sector5", "Deeper", "Reach sector 5.", S.SECTOR_REACHED, run, 5, False),
        ("ACH_Sector10", "Into the Dark", "Reach sector 10.", S.SECTOR_REACHED, run, 10, False),
        ("ACH_Sector20", "No Way Out", "Reach sector 20.", S.SECTOR_REACHED, run, 20, True),
        ("ACH_BossKiller", "Giant Slayer", "Defeat a boss.", S.BOSSES_DEFEATED, lifetime, 1, False),
        ("ACH_BossHunter", "Boss Hunter", "Defeat 10 bosses.", S.BOSSES_DEFEATED, lifetime, 10, False),
        ("ACH_Tycoon", "Tycoon", "Earn $5,000 in a single run.", S.MONEY_EARNED, run, 5000, False),
        ("ACH_Legendary", "Legendary", "Find a Legendary weapon.", S.LEGENDARIES_FOUND, lifetime, 1, False),
        ("ACH_PerkAddict", "Perk Addict", "Buy 25 perk tiers.", S.PERKS_BOUGHT, lifetime, 25, False),
        ("ACH_TryAgain", "Try Again", "Die for the first time.", S.DEATHS, lifetime, 1, False),
        ("ACH_Veteran", "Veteran", "Start 25 runs.", S.RUNS_STARTED, lifetime, 25, False),
    ]
    for asset_name, name, description, stat, scope, threshold, hidden in achievements:
        asset, _ = get_or_create(asset_name, "/Game/DataAssets/Achievements", unreal.AchievementDataAsset)
        apply(asset, {"display_name": name, "description": description, "stat": stat, "scope": scope, "threshold": threshold, "hidden": hidden})
        save(asset)
    note("built %d achievements" % len(achievements))


def run():
    build_status_effects()
    build_weapons()
    build_perks()
    build_loot()
    build_settings()
    build_achievements()
