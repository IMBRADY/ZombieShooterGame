"""Imports the generated pixel art and builds the materials and sprite-sheet Data Assets."""

import unreal

from content_lib import (ASSET_TOOLS, EAL, MEL, SHEET_INDEX, TEXTURE_INDEX, apply, get_or_create, import_file,
                         load_json, note, save)

SPRITE_ROOT = "/Game/Sprites"
MATERIAL_DIR = "/Game/Materials"
TEXTURE_DIR = "/Game/Materials/Textures"
UI_TEXTURE_DIR = "/Game/UI/Textures"


# --- textures ------------------------------------------------------------------------------------

def configure_texture(texture, smooth=False):
    """Pixel art stays pixel art: nearest filtering, no mips, no compression artefacts."""
    apply(texture, {
        "lod_group": unreal.TextureGroup.TEXTUREGROUP_UI if smooth else unreal.TextureGroup.TEXTUREGROUP_PIXELS2D,
        "compression_settings": unreal.TextureCompressionSettings.TC_EDITOR_ICON,
        "mip_gen_settings": unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS,
        "filter": unreal.TextureFilter.TF_BILINEAR if smooth else unreal.TextureFilter.TF_NEAREST,
        "never_stream": True,
        "srgb": True,
    })
    save(texture)


def import_textures():
    manifest = load_json("SourceAssets/Art/art_manifest.json")
    for entry in manifest["sheets"]:
        texture = import_file(entry["file"], "%s/%s" % (SPRITE_ROOT, entry["folder"]), entry["texture"])
        if texture:
            configure_texture(texture)
            TEXTURE_INDEX[entry["texture"]] = texture
    for entry in manifest["textures"]:
        destination = UI_TEXTURE_DIR if entry["kind"].startswith("ui") else TEXTURE_DIR
        texture = import_file(entry["file"], destination, entry["name"])
        if texture:
            configure_texture(texture, entry["kind"] == "ui_smooth")
            TEXTURE_INDEX[entry["name"]] = texture
    note("imported %d textures" % len(TEXTURE_INDEX))
    return manifest


# --- materials -----------------------------------------------------------------------------------

def _expr(material, cls, x, y, **props):
    node = MEL.create_material_expression(material, cls, x, y)
    for key, value in props.items():
        node.set_editor_property(key, value)
    return node


def _custom(material, x, y, code, inputs, output_type=unreal.CustomMaterialOutputType.CMOT_FLOAT2, description="Custom"):
    node = _expr(material, unreal.MaterialExpressionCustom, x, y, code=code, output_type=output_type, description=description)
    custom_inputs = []
    for name in inputs:
        entry = unreal.CustomInput()
        entry.set_editor_property("input_name", name)
        custom_inputs.append(entry)
    node.set_editor_property("inputs", custom_inputs)
    return node


def _scalar(material, name, value, x, y):
    return _expr(material, unreal.MaterialExpressionScalarParameter, x, y, parameter_name=name, default_value=value)


def _vector(material, name, value, x, y):
    return _expr(material, unreal.MaterialExpressionVectorParameter, x, y, parameter_name=name, default_value=unreal.LinearColor(*value))


def _texture_param(material, name, texture, x, y):
    return _expr(material, unreal.MaterialExpressionTextureSampleParameter2D, x, y, parameter_name=name, texture=texture,
                 sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)


def _connect(src, src_output, dst, dst_input):
    if not MEL.connect_material_expressions(src, src_output, dst, dst_input):
        note("FAILED to connect %s.%s -> %s.%s" % (src.get_name(), src_output, dst.get_name(), dst_input))


def _fresh_material(name):
    path = "%s/%s" % (MATERIAL_DIR, name)
    if EAL.does_asset_exist(path):
        EAL.delete_asset(path)
    return ASSET_TOOLS.create_asset(name, MATERIAL_DIR, unreal.Material, unreal.MaterialFactoryNew())


SPRITE_UV = """
float elapsed = max(T - StartTime, 0.0);
float count = max(FrameCount, 1.0);
float f = floor(elapsed * FPS);
f = Loop > 0.5 ? fmod(f, count) : min(f, count - 1.0);
float idx = StartFrame + f;
float col = fmod(idx, Columns);
float row = floor(idx / Columns);
float2 uv = clamp(UV, 0.002, 0.998);
return float2((uv.x + col) / Columns, (uv.y + row) / Rows);
"""

SPRITE_COLOR = """
float3 base = TexRGB * Tint.rgb * Brightness;
return lerp(base, FlashColor.rgb, saturate(FlashAmount) * 0.85);
"""

# Dithered opacity: masked materials can't blend, so fades and soft edges become a screen-space
# stipple - which suits the pixel-art look anyway.
SPRITE_MASK = """
float fade = FadeDuration > 0.0 ? 1.0 - saturate((T - FadeStart) / FadeDuration) : 1.0;
float alpha = TexA * fade;
float2 p = fmod(floor(Parameters.SvPosition.xy), 4.0);
float threshold = frac(sin(dot(p, float2(12.9898, 78.233))) * 43758.5453) * 0.96 + 0.02;
return alpha > threshold ? 1.0 : 0.0;
"""


def build_sprite_material(default_texture):
    m = _fresh_material("M_PixelSprite")
    apply(m, {"blend_mode": unreal.BlendMode.BLEND_MASKED, "shading_model": unreal.MaterialShadingModel.MSM_UNLIT,
              "two_sided": True, "opacity_mask_clip_value": 0.5})

    uv = _expr(m, unreal.MaterialExpressionTextureCoordinate, -1400, -200)
    time = _expr(m, unreal.MaterialExpressionTime, -1400, -100)
    params = {name: _scalar(m, name, value, -1400, i * 60)
              for i, (name, value) in enumerate([("Columns", 1.0), ("Rows", 1.0), ("StartFrame", 0.0), ("FrameCount", 1.0),
                                                 ("FPS", 8.0), ("StartTime", 0.0), ("Loop", 1.0), ("FlashAmount", 0.0),
                                                 ("FadeStartTime", 0.0), ("FadeDuration", 0.0)])}
    tint = _vector(m, "Tint", (1, 1, 1, 1), -1000, 400)
    flash = _vector(m, "FlashColor", (1, 1, 1, 1), -1000, 500)

    uv_node = _custom(m, -900, -100, SPRITE_UV, ["UV", "T", "Columns", "Rows", "StartFrame", "FrameCount", "FPS", "StartTime", "Loop"],
                      description="SpriteUV")
    _connect(uv, "", uv_node, "UV")
    _connect(time, "", uv_node, "T")
    for name in ("Columns", "Rows", "StartFrame", "FrameCount", "FPS", "StartTime", "Loop"):
        _connect(params[name], "", uv_node, name)

    tex = _texture_param(m, "SpriteSheet", default_texture, -600, -100)
    _connect(uv_node, "", tex, "UVs")

    color = _custom(m, -300, -150, SPRITE_COLOR, ["TexRGB", "Tint", "FlashColor", "FlashAmount", "Brightness"], unreal.CustomMaterialOutputType.CMOT_FLOAT3, "SpriteColor")
    _connect(tex, "RGB", color, "TexRGB")
    _connect(_scalar(m, "Brightness", 1.0, -1000, 600), "", color, "Brightness")
    _connect(tint, "", color, "Tint")
    _connect(flash, "", color, "FlashColor")
    _connect(params["FlashAmount"], "", color, "FlashAmount")

    mask = _custom(m, -300, 100, SPRITE_MASK, ["TexA", "T", "FadeStart", "FadeDuration"], unreal.CustomMaterialOutputType.CMOT_FLOAT1, "SpriteMask")
    _connect(tex, "A", mask, "TexA")
    _connect(time, "", mask, "T")
    _connect(params["FadeStartTime"], "", mask, "FadeStart")
    _connect(params["FadeDuration"], "", mask, "FadeDuration")

    MEL.connect_material_property(color, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.connect_material_property(mask, "", unreal.MaterialProperty.MP_OPACITY_MASK)
    MEL.recompile_material(m)
    save(m)
    note("built M_PixelSprite")
    return m


FLOOR_UV = """
float2 cell = floor(WP.xy / TileSize);
float h = frac(sin(dot(cell, float2(12.9898, 78.233))) * 43758.5453);
float idx = floor(h * Cols * Rows);
float2 local = frac(WP.xy / TileSize);
float col = fmod(idx, Cols);
float row = floor(idx / Cols);
return float2((local.x + col) / Cols, (local.y + row) / Rows);
"""

WALL_UV = """
float3 n = abs(N);
float2 local;
float col;
if (n.z > 0.5)
{
    local = frac(WP.xy / TileSize);
    col = 0.0;
}
else
{
    float across = n.x > 0.5 ? WP.y : WP.x;
    local = float2(frac(across / TileSize), 1.0 - frac(WP.z / (TileSize * 0.64)));
    col = 1.0;
}
return float2((clamp(local.x, 0.002, 0.998) + col) * 0.5, clamp(local.y, 0.002, 0.998));
"""

OBSTACLE_UV = """
float col = abs(N.z) > 0.5 ? 0.0 : 1.0;
float row = clamp(floor(Variant + 0.5), 0.0, Rows - 1.0);
float2 uv = clamp(UV, 0.002, 0.998);
return float2((uv.x + col) * 0.5, (uv.y + row) / Rows);
"""

DECORATION_UV = """
float idx = floor(Variant + 0.5);
float col = fmod(idx, Cols);
float row = floor(idx / Cols);
float2 uv = clamp(UV, 0.002, 0.998);
return float2((uv.x + col) / Cols, (uv.y + row) / Rows);
"""


def _world_material(name, code, inputs, texture, extra_scalars, masked=False):
    m = _fresh_material(name)
    apply(m, {"shading_model": unreal.MaterialShadingModel.MSM_DEFAULT_LIT,
              "blend_mode": unreal.BlendMode.BLEND_MASKED if masked else unreal.BlendMode.BLEND_OPAQUE,
              "used_with_instanced_static_meshes": True})

    node = _custom(m, -700, 0, code, inputs, description=name + "UV")
    sources = {
        "WP": lambda: _expr(m, unreal.MaterialExpressionWorldPosition, -1100, -200),
        "N": lambda: _expr(m, unreal.MaterialExpressionVertexNormalWS, -1100, -100),
        "UV": lambda: _expr(m, unreal.MaterialExpressionTextureCoordinate, -1100, 0),
        "Variant": lambda: _expr(m, unreal.MaterialExpressionPerInstanceCustomData, -1100, 100, data_index=0),
    }
    for index, key in enumerate(inputs):
        if key in sources:
            _connect(sources[key](), "", node, key)
        else:
            _connect(_scalar(m, key, extra_scalars[key], -1100, 200 + index * 60), "", node, key)

    tex = _texture_param(m, "Atlas", texture, -400, 0)
    _connect(node, "", tex, "UVs")
    MEL.connect_material_property(tex, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    if masked:
        MEL.connect_material_property(tex, "A", unreal.MaterialProperty.MP_OPACITY_MASK)
    rough = _expr(m, unreal.MaterialExpressionConstant, -400, 300, r=0.92)
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    spec = _expr(m, unreal.MaterialExpressionConstant, -400, 380, r=0.15)
    MEL.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)
    MEL.recompile_material(m)
    save(m)
    note("built " + name)
    return m


def build_world_materials():
    return {
        "Floor": _world_material("M_WorldFloor", FLOOR_UV, ["WP", "TileSize", "Cols", "Rows"], TEXTURE_INDEX.get("T_Floor_Office"),
                                 {"TileSize": 250.0, "Cols": 4.0, "Rows": 4.0}),
        "Wall": _world_material("M_WorldWall", WALL_UV, ["WP", "N", "TileSize"], TEXTURE_INDEX.get("T_Wall_Office"), {"TileSize": 250.0}),
        "Obstacle": _world_material("M_WorldObstacle", OBSTACLE_UV, ["UV", "N", "Variant", "Rows"], TEXTURE_INDEX.get("T_Obstacles_Office"),
                                    {"Rows": 4.0}),
        "Decoration": _world_material("M_WorldDecoration", DECORATION_UV, ["UV", "Variant", "Cols", "Rows"], TEXTURE_INDEX.get("T_Decorations"),
                                      {"Cols": 4.0, "Rows": 2.0}, masked=True),
    }


def material_instance(name, parent, texture):
    mi, _ = get_or_create(name, MATERIAL_DIR + "/Instances", unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mi, parent)
    MEL.set_material_instance_texture_parameter_value(mi, "Atlas", texture)
    MEL.update_material_instance(mi)
    save(mi)
    return mi


# --- sprite sheets --------------------------------------------------------------------------------

def build_sprite_sheets(manifest):
    for entry in manifest["sheets"]:
        asset, _ = get_or_create(entry["name"], "%s/%s" % (SPRITE_ROOT, entry["folder"]), unreal.SpriteSheetDataAsset)
        animations = {}
        for anim_name, anim in entry["animations"].items():
            value = unreal.SpriteAnimation()
            value.set_editor_property("start_frame", anim["start"])
            value.set_editor_property("frame_count", anim["count"])
            value.set_editor_property("frames_per_second", anim["fps"])
            value.set_editor_property("loop", anim["loop"])
            animations[anim_name] = value
        apply(asset, {"texture": TEXTURE_INDEX.get(entry["texture"]), "columns": entry["columns"], "rows": entry["rows"],
                      "world_size": float(entry["world_size"]), "animations": animations})
        save(asset)
        SHEET_INDEX[entry["name"]] = asset
    note("built %d sprite sheets" % len(SHEET_INDEX))


def run():
    manifest = import_textures()
    sprite_material = build_sprite_material(TEXTURE_INDEX.get("T_Player"))
    world = build_world_materials()
    build_sprite_sheets(manifest)
    return sprite_material, world
