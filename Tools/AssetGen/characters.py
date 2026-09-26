"""Top-down pixel-art characters: the player, every zombie archetype, and the bosses.

All art faces +X (right). A figure is built from the same parts - shadow, feet, torso, arms, head -
parameterised by a style dictionary, so each archetype is a palette and a few proportions rather
than a hand-drawn sheet. Frame layout (4 columns x 4 rows):

    row 0: Idle (2 frames)       row 1: Walk (4 frames)
    row 2: Attack (4 frames)     row 3: Death (4 frames, non-looping)
"""

import math
import random
import zlib

from pixel import Canvas, SpriteSheet, rgba, shade, mix

OUTLINE = (20, 16, 22, 255)
BLOOD = (120, 12, 14, 255)
BLOOD_DARK = (70, 6, 10, 255)


def _style(**overrides):
    base = {
        "skin": "#7c9a63", "shirt": "#3e5a78", "pants": "#3b3530", "boots": "#231f1c", "hair": None,
        "shoulder_ry": 7.5, "shoulder_rx": 5.0, "arm_width": 2.4, "head_r": 4.2,
        "arms": "zombie", "extras": [], "eyes": "#8a1010", "bald": True,
    }
    base.update(overrides)
    return base


PLAYER = _style(skin="#d9a27a", shirt="#556b3a", pants="#2e3326", boots="#1c1a17", hair="#4a2f1d",
                arms="gun", eyes=None, bald=False)

ZOMBIES = {
    "Common": _style(),
    "Runner": _style(skin="#b7b27a", shirt="#7a2a26", shoulder_ry=6.4, shoulder_rx=4.4, arm_width=2.0, head_r=3.9),
    "Tank": _style(skin="#8a5a52", shirt="#3a2c2a", pants="#2a2422", shoulder_ry=10.5, shoulder_rx=7.0, arm_width=3.8, head_r=4.6,
                   extras=["scars"]),
    "Lobber": _style(skin="#7f6f9a", shirt="#4b3f5e", extras=["sac", "belly"]),
    "Exploder": _style(skin="#b86a3a", shirt="#5a2a1a", extras=["pustules"], eyes="#ffd040"),
    "Poison": _style(skin="#6fb53c", shirt="#27401c", extras=["drips"], eyes="#e0ff40"),
    "Armored": _style(skin="#788a6a", shirt="#4a4f55", extras=["helmet", "vest"], shoulder_ry=8.2),
    "Necromancer": _style(skin="#9c8fb0", shirt="#3a1f52", pants="#2a1540", arms="cast", extras=["hood", "robe"], eyes="#c070ff"),
}

BOSSES = {
    "Butcher": _style(skin="#b07a6a", shirt="#d8d0c0", pants="#3a2a24", shoulder_ry=11.5, shoulder_rx=8.0, arm_width=4.2, head_r=5.2,
                      extras=["apron", "cleaver", "scars"], eyes="#ff3020"),
    "PlagueMother": _style(skin="#86a84e", shirt="#3f5a22", pants="#2e3a1c", shoulder_ry=11.0, shoulder_rx=9.0, arm_width=3.4, head_r=4.8,
                           extras=["sac", "belly", "drips", "tendrils"], eyes="#f0ff50"),
}


class Figure:
    """Draws one frame of a figure at a design scale of 32 units per frame edge."""

    def __init__(self, canvas, style, scale):
        self.c = canvas
        self.st = style
        self.s = scale

    def p(self, v):
        return v * self.s

    def shadow(self):
        self.c.ellipse(self.p(15), self.p(17), self.p(7.5), self.p(self.st["shoulder_ry"] + 1.0), (0, 0, 0, 80))

    def feet(self, step):
        boots = rgba(self.st["boots"])
        for side, offset in ((-1, step), (1, -step)):
            y = 16 + side * (self.st["shoulder_ry"] * 0.55)
            self.c.ellipse(self.p(14.0 + offset * 1.6), self.p(y), self.p(3.0), self.p(1.9), boots)

    def torso(self, breathe=0.0):
        st = self.st
        shirt = rgba(st["shirt"])
        rx, ry = st["shoulder_rx"], st["shoulder_ry"] + breathe
        if "robe" in st["extras"]:
            rx, ry = rx + 2.0, ry + 1.0
        self.c.ellipse(self.p(14.5), self.p(16), self.p(rx), self.p(ry), shade(shirt, 0.8))
        self.c.ellipse(self.p(14.8), self.p(15.3), self.p(rx - 1.0), self.p(ry - 1.3), shirt)
        self.c.ellipse(self.p(15.5), self.p(13.6), self.p(max(rx - 3.0, 1.0)), self.p(max(ry - 4.5, 1.0)), shade(shirt, 1.15))

        if "vest" in st["extras"]:
            plate = rgba("#6d747c")
            self.c.rect(self.p(12), self.p(11), self.p(18.5), self.p(21), plate)
            self.c.rect(self.p(12.5), self.p(11.5), self.p(18), self.p(13), shade(plate, 1.2))
        if "apron" in st["extras"]:
            self.c.ellipse(self.p(16.5), self.p(16), self.p(3.5), self.p(ry - 2.5), rgba("#e8e2d6"))
            for i in range(5):
                self.c.put(self.p(16 + (i % 3)), self.p(12 + i * 2), BLOOD)
                self.c.put(self.p(17 + (i % 2)), self.p(13 + i * 2), BLOOD_DARK)
        if "belly" in st["extras"]:
            self.c.ellipse(self.p(18.5), self.p(16), self.p(4.2), self.p(5.5), shade(rgba(st["skin"]), 0.95))
            self.c.ellipse(self.p(19.3), self.p(15), self.p(2.2), self.p(3.0), shade(rgba(st["skin"]), 1.15))
        if "sac" in st["extras"]:
            sac = rgba("#c9c060")
            self.c.circle(self.p(8.5), self.p(16), self.p(5.0), shade(sac, 0.8))
            self.c.circle(self.p(8.0), self.p(15.2), self.p(3.8), sac)
            self.c.line(self.p(6), self.p(13), self.p(10), self.p(18), shade(sac, 0.6))
        if "scars" in st["extras"]:
            self.c.line(self.p(12), self.p(12), self.p(16), self.p(14), shade(rgba(st["shirt"]), 0.5))

    def arms(self, mode, sway=0.0, reach=0.0):
        st = self.st
        skin = rgba(st["skin"])
        sleeve = shade(rgba(st["shirt"]), 0.9)
        w = self.p(st["arm_width"])
        ry = st["shoulder_ry"]
        top, bottom = 16 - ry + 1.5, 16 + ry - 1.5

        if mode == "gun":
            hands = ((21.5, 15.0), (21.5, 17.2))
            for (sx, sy), (hx, hy) in zip(((14, top), (14, bottom)), hands):
                self.c.line(self.p(sx), self.p(sy), self.p(hx - 1), self.p(hy), sleeve, w)
                self.c.circle(self.p(hx), self.p(hy), self.p(1.5), skin)
            return

        if mode == "cast":
            glow = rgba("#c070ff")
            for sy, dy in ((top, 1.0), (bottom, -1.0)):
                hx, hy = 21 + reach, 16 + dy * (ry - 3.5)
                self.c.line(self.p(14), self.p(sy), self.p(hx), self.p(hy), sleeve, w)
                self.c.circle(self.p(hx + 0.8), self.p(hy), self.p(1.5 + reach * 0.3), glow)
            return

        # Zombie reach: both arms straight ahead, swaying out of step.
        length = 24 + reach
        for sy, phase in ((top + 0.5, 1.0), (bottom - 0.5, -1.0)):
            hy = sy + (phase * -1.5 if reach > 0 else 0.0)
            hx = length + sway * phase
            self.c.line(self.p(14), self.p(sy), self.p(hx), self.p(hy), skin, w)
            self.c.line(self.p(15), self.p(sy), self.p(18), self.p(sy), sleeve, w)
            self.c.circle(self.p(hx + 0.5), self.p(hy), self.p(st["arm_width"] * 0.75), shade(skin, 0.9))
            if "cleaver" in st["extras"] and phase < 0:
                blade = rgba("#b8c0c8")
                self.c.rect(self.p(hx - 1), self.p(hy + 1), self.p(hx + 7), self.p(hy + 5), blade)
                self.c.rect(self.p(hx - 1), self.p(hy + 4), self.p(hx + 7), self.p(hy + 5), shade(blade, 0.6))
                self.c.put(self.p(hx + 3), self.p(hy + 3), BLOOD)
        if "tendrils" in st["extras"]:
            for i, ty in enumerate((10.0, 22.0)):
                self.c.line(self.p(12), self.p(ty), self.p(22 + sway * (1 if i else -1)), self.p(ty + (-3 if i == 0 else 3)), shade(skin, 0.75), self.p(1.6))

    def head(self, glow=0.0):
        st = self.st
        skin = rgba(st["skin"])
        r = st["head_r"]
        hx, hy = 15.5, 16.0

        if "hood" in st["extras"]:
            hood = rgba("#2a1440")
            self.c.circle(self.p(hx - 0.5), self.p(hy), self.p(r + 1.2), hood)
            self.c.circle(self.p(hx + 1.5), self.p(hy), self.p(r - 1.2), shade(hood, 0.6))
        elif "helmet" in st["extras"]:
            helmet = rgba("#5d646a")
            self.c.circle(self.p(hx), self.p(hy), self.p(r + 0.6), helmet)
            self.c.circle(self.p(hx - 0.8), self.p(hy - 1.0), self.p(r - 1.6), shade(helmet, 1.25))
            self.c.rect(self.p(hx + r - 1.5), self.p(hy - 2.5), self.p(hx + r + 0.5), self.p(hy + 2.5), rgba("#1c2024"))
        else:
            self.c.circle(self.p(hx), self.p(hy), self.p(r), shade(skin, 0.85))
            self.c.circle(self.p(hx + 0.3), self.p(hy - 0.6), self.p(r - 0.9), skin)
            if st["hair"]:
                hair = rgba(st["hair"])
                self.c.ellipse(self.p(hx - 1.2), self.p(hy), self.p(r - 0.6), self.p(r), hair)
                self.c.ellipse(self.p(hx - 1.6), self.p(hy - 1.2), self.p(r - 2.0), self.p(r - 2.0), shade(hair, 1.3))
            elif st["bald"]:
                rng = random.Random(zlib.crc32(st["skin"].encode()))
                for _ in range(3):
                    self.c.put(self.p(hx - 2 + rng.random() * 2), self.p(hy - 2 + rng.random() * 4), shade(skin, 0.6))

        if st["eyes"]:
            eye = mix(rgba(st["eyes"]), (255, 255, 255, 255), glow)
            for dy in (-1.4, 1.4):
                self.c.circle(self.p(hx + r - 1.4), self.p(hy + dy), self.p(0.7), eye)

    def pustules(self, glow):
        if "pustules" not in self.st["extras"]:
            return
        core = mix(rgba("#ff8a20"), rgba("#fff080"), glow)
        for (x, y) in ((12, 12), (16, 19), (11, 18), (17, 11.5), (14, 15)):
            self.c.circle(self.p(x), self.p(y), self.p(1.3), rgba("#ff5a10"))
            self.c.put(self.p(x), self.p(y), core)

    def drips(self, frame):
        if "drips" not in self.st["extras"]:
            return
        rng = random.Random(frame * 31 + 7)
        for _ in range(6):
            a = rng.random() * math.tau
            d = 8 + rng.random() * 4
            self.c.put(self.p(15 + math.cos(a) * d), self.p(16 + math.sin(a) * d), rgba("#9cf040", 220))


def _corpse(canvas, style, scale, frame):
    """Death frames: the figure going down, then lying in a spreading pool."""
    s = scale
    skin, shirt, pants = rgba(style["skin"]), rgba(style["shirt"]), rgba(style["pants"])
    pool = min(frame, 3)
    canvas.ellipse(15 * s, 16 * s, (4 + pool * 1.6) * s, (3 + pool * 1.3) * s, (95, 8, 10, 235))
    ry = style["shoulder_ry"] * 0.72
    canvas.ellipse(8.5 * s, 13.5 * s, 4.5 * s, 1.8 * s, pants)
    canvas.ellipse(8.5 * s, 18.5 * s, 4.5 * s, 1.8 * s, shade(pants, 0.85))
    canvas.ellipse(15.5 * s, 16 * s, 6.5 * s, ry * s, shade(shirt, 0.75))
    canvas.line(15 * s, (16 - ry) * s, 21 * s, (8.5 - pool * 0.3) * s, skin, style["arm_width"] * s)
    canvas.line(15 * s, (16 + ry) * s, 20 * s, (24 + pool * 0.3) * s, skin, style["arm_width"] * s)
    canvas.circle(23.5 * s, 16 * s, (style["head_r"] - 0.3) * s, shade(skin, 0.75))
    canvas.outline(OUTLINE)


def build_sheet(style, scale=1, is_player=False):
    size = 32 * scale
    sheet = SpriteSheet(size, size, 4, 4)

    def frame(step=0.0, breathe=0.0, sway=0.0, reach=0.0, arm_mode=None, glow=0.0, index=0):
        c = Canvas(size, size)
        f = Figure(c, style, scale)
        f.feet(step)
        f.torso(breathe)
        f.arms(arm_mode or style["arms"], sway, reach)
        f.pustules(glow)
        f.head(glow)
        c.outline(OUTLINE)
        shadow = Canvas(size, size)
        Figure(shadow, style, scale).shadow()
        shadow.paste(c, 0, 0)
        f2 = Figure(shadow, style, scale)
        f2.drips(index)
        return shadow

    sheet.set(0, frame(breathe=0.0, index=0))
    sheet.set(1, frame(breathe=0.35, sway=0.4, glow=0.4, index=1))
    for i in range(4):
        phase = i / 4.0 * math.tau
        sheet.set(4 + i, frame(step=3.0 * math.sin(phase), sway=1.2 * math.sin(phase), glow=(i % 2) * 0.5, index=4 + i))
    for i in range(4):
        reach = (0.0, 2.5, 4.0, 1.5)[i]
        mode = "gun" if is_player else None
        sheet.set(8 + i, frame(reach=reach, arm_mode=mode, glow=0.3 + 0.2 * i, index=8 + i))
    for i in range(4):
        c = Canvas(size, size)
        _corpse(c, style, scale, i)
        sheet.set(12 + i, c)

    sheet.add_animation("Idle", 0, 2, 2.0, True)
    sheet.add_animation("Walk", 4, 4, 8.0, True)
    sheet.add_animation("Attack", 8, 4, 10.0, False)
    sheet.add_animation("Death", 12, 4, 10.0, False)
    return sheet


# --- held weapons ---------------------------------------------------------------------------------

WEAPON_FRAMES = ["Pistol", "SMG", "Shotgun", "Rifle", "Sniper", "RocketLauncher", "Special", "Revolver"]


def _weapon(name):
    c = Canvas(32, 32)
    metal, dark, wood = rgba("#4a4e54"), rgba("#23262a"), rgba("#6a4424")
    if name == "Pistol":
        c.rect(19, 15, 26, 17.5, dark)
        c.rect(19.5, 15, 25.5, 16, metal)
    elif name == "Revolver":
        c.rect(19, 15, 28, 17, rgba("#8a8f96"))
        c.circle(21.5, 16, 1.8, rgba("#5a5e64"))
    elif name == "SMG":
        c.rect(17, 14.8, 27, 17.6, dark)
        c.rect(21, 17.6, 23, 20.5, metal)
        c.rect(17.5, 15, 26, 15.8, metal)
    elif name == "Shotgun":
        c.rect(13, 15, 19, 17.6, wood)
        c.rect(19, 15.2, 30, 16.8, metal)
        c.rect(22, 16.6, 26, 18, wood)
    elif name == "Rifle":
        c.rect(13, 15, 17, 17.5, dark)
        c.rect(17, 14.8, 29, 17.2, dark)
        c.rect(21, 17.2, 23, 20, metal)
        c.rect(17, 14.8, 28, 15.6, metal)
    elif name == "Sniper":
        c.rect(12, 15, 17, 17.5, wood)
        c.rect(17, 15.3, 31, 16.7, dark)
        c.ellipse(21, 14, 3.2, 1.3, rgba("#2c3c46"))
        c.put(24, 14, rgba("#80d0ff"))
    elif name == "RocketLauncher":
        c.rect(11, 13.5, 29, 18.5, rgba("#4c5a34"))
        c.rect(11, 13.5, 29, 14.5, rgba("#6c7c4a"))
        c.polygon([(29, 14), (31.5, 16), (29, 18)], rgba("#b83020"))
        c.rect(18, 18.5, 20, 21, dark)
    elif name == "Special":
        c.rect(14, 14, 27, 18, rgba("#1c3a44"))
        c.rect(14.5, 14.5, 26.5, 15.5, rgba("#3a8aa0"))
        c.circle(27.5, 16, 2.0, rgba("#60f0ff"))
        c.rect(18, 18, 21, 20.5, rgba("#28505c"))
    c.outline(OUTLINE)
    return c


def build_weapon_sheet():
    sheet = SpriteSheet(32, 32, 4, 2)
    for i, name in enumerate(WEAPON_FRAMES):
        sheet.set(i, _weapon(name))
        sheet.add_animation(name, i, 1, 1.0, False)
    return sheet
