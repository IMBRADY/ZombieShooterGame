"""Imports the synthesised sounds and builds the audio settings Data Asset."""

import unreal

from content_lib import SOUND_INDEX, apply, get_or_create, import_file, load_json, note, save, sound

SOUND_ROOT = "/Game/Sounds"


def import_sounds():
    manifest = load_json("SourceAssets/Audio/audio_manifest.json")
    for entry in manifest["sounds"]:
        wave = import_file(entry["file"], "%s/%s" % (SOUND_ROOT, entry["folder"]), entry["name"])
        if not wave:
            note("FAILED to import sound " + entry["name"])
            continue
        wave.set_editor_property("looping", entry["loop"])
        save(wave)
        SOUND_INDEX[entry["name"]] = wave
    note("imported %d sounds" % len(SOUND_INDEX))


def build_audio_settings():
    music = unreal.ZombieSoundCategory.MUSIC
    ui = unreal.ZombieSoundCategory.UI
    settings, _ = get_or_create("DA_Audio", "/Game/DataAssets/Audio", unreal.ZombieAudioSettings)
    apply(settings, {
        "menu_music": sound("MUS_Menu", 0.8, 0.0, music),
        "exploration_layer": sound("MUS_Sector_Explore", 0.7, 0.0, music),
        "combat_layer": sound("MUS_Sector_Combat", 0.8, 0.0, music),
        "boss_music": sound("MUS_Boss", 0.85, 0.0, music),
        "intermission_music": sound("MUS_Intermission", 0.7, 0.0, music),
        "game_over_music": sound("MUS_GameOver", 0.8, 0.0, music),
        "ambient_loop": sound("AMB_Machinery", 0.35, 0.0),
        "named_sounds": {
            "UI.Click": sound("SFX_UI_Click", 0.6, 0.02, ui),
            "UI.Buy": sound("SFX_UI_Buy", 0.7, 0.02, ui),
            "UI.Denied": sound("SFX_UI_Denied", 0.6, 0.02, ui),
            "UI.Achievement": sound("SFX_Achievement", 0.8, 0.0, ui),
            "Sector.Cleared": sound("SFX_Sector_Cleared", 0.8, 0.0, ui),
            "Pickup.Money": sound("SFX_Pickup_Coin", 0.45, 0.12),
            "Pickup.Health": sound("SFX_Pickup_Health", 0.7),
            "Pickup.Armor": sound("SFX_Pickup_Armor", 0.7),
            "Pickup.Ammo": sound("SFX_Pickup_Ammo", 0.7),
            "Pickup.Weapon": sound("SFX_Pickup_Weapon", 0.8),
            "Pickup.Perk": sound("SFX_Pickup_Perk", 0.8),
            "Pickup.Key": sound("SFX_Pickup_Key", 0.9, 0.0),
            "Door.Unlock": sound("SFX_Door_Unlock", 0.9, 0.0),
            "Door.Open": sound("SFX_Door_Open", 0.9, 0.02),
            "Chest.Open": sound("SFX_Chest_Open", 0.9),
            "Player.Hurt": sound("SFX_Player_Hurt", 0.7, 0.1),
            "Player.Death": sound("SFX_Player_Death", 0.9, 0.0),
        },
    })
    save(settings)
    note("built DA_Audio")


def run():
    import_sounds()
    build_audio_settings()
