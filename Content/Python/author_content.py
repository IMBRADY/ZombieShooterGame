"""Builds all of the game's content from source: imports the generated pixel art and audio
(Tools/AssetGen), authors the materials, and creates every Data Asset - weapons, perks, loot, rooms,
themes, zombies, bosses, settings, achievements - plus the maps.

Idempotent: re-running updates assets in place. Not auto-run; invoke explicitly after regenerating
source art/audio or editing balance here:

    UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script="<abs path>/author_content.py" -nullrhi

Writes Content/Python/author_content_result.txt as its report (delete it afterwards).
"""

import os
import sys
import traceback

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import content_art
import content_audio
import content_gameplay
import content_world
from content_lib import LOG, note

MARKER = os.path.join(os.path.dirname(os.path.abspath(__file__)), "author_content_result.txt")

try:
    sprite_material, world_materials = content_art.run()
    content_audio.run()
    content_gameplay.run()
    content_world.run(world_materials)
    note("DONE")
except Exception:
    note("FAILED:\n" + traceback.format_exc())

with open(MARKER, "w") as handle:
    handle.write("\n".join(LOG))
