"""Shared helpers for the content authoring scripts (see author_content.py)."""

import json
import os

import unreal

PROJECT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
ASSET_TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary

LOG = []


def note(message):
    unreal.log("[ZombieContent] " + message)
    LOG.append(message)


def load_json(relative):
    with open(os.path.join(PROJECT_DIR, relative)) as handle:
        return json.load(handle)


def get_or_create(name, package_path, asset_class, factory=None):
    full_path = "{0}/{1}".format(package_path, name)
    if EAL.does_asset_exist(full_path):
        asset = EAL.load_asset(full_path)
        if asset is not None and isinstance(asset, asset_class):
            return asset, False
        EAL.delete_asset(full_path)
    return ASSET_TOOLS.create_asset(name, package_path, asset_class, factory), True


def apply(asset, properties):
    for key, value in properties.items():
        asset.set_editor_property(key, value)


def save(asset):
    EAL.save_loaded_asset(asset, only_if_is_dirty=False)


def load(path):
    asset = EAL.load_asset(path)
    if asset is None:
        note("MISSING asset " + path)
    return asset


def import_file(source_relative, destination_path, destination_name):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", os.path.join(PROJECT_DIR, source_relative))
    task.set_editor_property("destination_path", destination_path)
    task.set_editor_property("destination_name", destination_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", False)
    ASSET_TOOLS.import_asset_tasks([task])
    paths = task.get_editor_property("imported_object_paths")
    return EAL.load_asset(paths[0]) if paths else None


def sound(name, volume=1.0, pitch=0.06, category=None, count=None):
    """An FZombieSoundSpec from one or more imported variants (SFX_Name or SFX_Name_0..n)."""
    spec = unreal.ZombieSoundSpec()
    variants = []
    names = [name] if count is None else ["%s_%d" % (name, i) for i in range(count)]
    for variant in names:
        asset = SOUND_INDEX.get(variant)
        if asset is None:
            note("MISSING sound " + variant)
            continue
        variants.append(asset)
    spec.set_editor_property("variants", variants)
    spec.set_editor_property("volume", volume)
    spec.set_editor_property("pitch_variance", pitch)
    if category is not None:
        spec.set_editor_property("category", category)
    return spec


SOUND_INDEX = {}
SHEET_INDEX = {}
TEXTURE_INDEX = {}


def effect(sheet=None, scale=1.0, tint=None, lifetime=0.0, height=0.0, random_yaw=True, niagara=None):
    spec = unreal.ZombieEffectSpec()
    if sheet:
        spec.set_editor_property("flipbook", SHEET_INDEX[sheet if sheet.startswith("SS_") else "SS_" + sheet])
    if niagara:
        spec.set_editor_property("niagara_system", niagara)
        spec.set_editor_property("niagara_color_parameter", "None")
    spec.set_editor_property("scale", scale)
    spec.set_editor_property("lifetime", lifetime)
    spec.set_editor_property("height_offset", height)
    spec.set_editor_property("random_yaw", random_yaw)
    if tint:
        spec.set_editor_property("tint", unreal.LinearColor(*tint))
    return spec


def tag(name):
    """A native gameplay tag by name (they are registered by the ZombieGame module)."""
    value = unreal.GameplayTag()
    for attempt in ('(TagName="%s")' % name, name):
        try:
            value.import_text(attempt)
            if name in value.export_text():
                return value
        except Exception:
            continue
    try:
        value.set_editor_property("tag_name", name)
        return value
    except Exception:
        note("COULD NOT build gameplay tag " + name)
        return value
