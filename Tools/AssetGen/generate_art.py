"""Generates every pixel-art texture the game uses into SourceAssets/Art, plus art_manifest.json
describing each sprite sheet (grid, world size, animations) for the Unreal import script.

    python Tools/AssetGen/generate_art.py
"""

import json
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))

import characters as ch
import world_art as wa

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
OUT = os.path.join(ROOT, "SourceAssets", "Art")

manifest = {"sheets": [], "textures": []}


def sheet(folder, name, built, world_size):
    os.makedirs(os.path.join(OUT, folder), exist_ok=True)
    path = os.path.join(OUT, folder, "T_" + name + ".png")
    info = built.save(path)
    info.update({"name": "SS_" + name, "texture": "T_" + name, "folder": folder, "file": os.path.relpath(path, ROOT), "world_size": world_size})
    manifest["sheets"].append(info)


def texture(folder, name, canvas, kind):
    os.makedirs(os.path.join(OUT, folder), exist_ok=True)
    path = os.path.join(OUT, folder, name + ".png")
    canvas.image().save(path)
    manifest["textures"].append({"name": name, "folder": folder, "file": os.path.relpath(path, ROOT), "kind": kind})


def main():
    sheet("Characters", "Player", ch.build_sheet(ch.PLAYER, 1, True), 210)
    sheet("Characters", "PlayerWeapons", ch.build_weapon_sheet(), 210)
    for name, style in ch.ZOMBIES.items():
        sheet("Characters", "Zombie_" + name, ch.build_sheet(style), 210)
    for name, style in ch.BOSSES.items():
        sheet("Characters", "Boss_" + name, ch.build_sheet(style, 2), 420)

    sheet("Effects", "MuzzleFlash", wa.muzzle_flash(), 80)
    sheet("Effects", "BloodHit", wa.blood_hit(), 100)
    sheet("Effects", "AcidHit", wa.acid_hit(), 110)
    sheet("Effects", "Sparks", wa.sparks(), 60)
    sheet("Effects", "DeathPuff", wa.death_puff(), 170)
    sheet("Effects", "ReviveGlow", wa.revive_glow(), 220)
    sheet("Effects", "Explosion", wa.explosion(), 420)
    sheet("Effects", "SlamRing", wa.slam_ring(), 420)
    sheet("Effects", "Tracer", wa.tracer(), 100)
    sheet("Effects", "DecalBlood", wa.decal("blood", 1), 170)
    sheet("Effects", "DecalBloodLarge", wa.decal("blood", 2), 260)
    sheet("Effects", "DecalScorch", wa.decal("scorch", 3), 320)
    sheet("Effects", "DecalAcid", wa.decal("acid", 4), 160)
    sheet("Effects", "PoisonPuddle", wa.puddle("#4a9a20", "#b0ff60"), 100)
    sheet("Effects", "AcidPuddle", wa.puddle("#7a8a20", "#e0f060"), 100)

    for kind in ("Rocket", "Acid", "Bolt", "Plasma", "Bullet"):
        sheet("Projectiles", "Proj_" + kind, wa.projectile(kind), 60)

    sheet("Pickups", "Pickups", wa.pickups_with_perk(), 95)
    sheet("Props", "Props", wa.props(), 160)

    for index, theme in enumerate(wa.THEMES):
        texture("Environment", "T_Floor_" + theme, wa.floor_atlas(theme, index * 13 + 1), "world")
        texture("Environment", "T_Wall_" + theme, wa.wall_atlas(theme), "world")
        texture("Environment", "T_Obstacles_" + theme, wa.obstacle_atlas(theme), "world")
    texture("Environment", "T_Decorations", wa.decoration_atlas(), "world")

    texture("UI", "T_Crosshair", wa.crosshair(), "ui")
    texture("UI", "T_Vignette", wa.vignette(), "ui_smooth")
    texture("UI", "T_TitleBackground", wa.title_background(), "ui")

    with open(os.path.join(OUT, "art_manifest.json"), "w") as handle:
        json.dump(manifest, handle, indent=1)
    print("Generated %d sheets and %d textures into %s" % (len(manifest["sheets"]), len(manifest["textures"]), OUT))


if __name__ == "__main__":
    main()
