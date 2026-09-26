"""Pixel art for everything that isn't a character: effects, projectiles, pickups, level props,
environment texture atlases (per theme) and UI textures."""

import math
import random

from pixel import Canvas, SpriteSheet, rgba, shade, mix

OUTLINE = (20, 16, 22, 255)


# --- effects -------------------------------------------------------------------------------------

def _anim_sheet(size, count, draw, fps=14.0, loop=False, name="Play"):
    cols = min(count, 4)
    rows = (count + cols - 1) // cols
    sheet = SpriteSheet(size, size, cols, rows)
    for i in range(count):
        c = Canvas(size, size)
        draw(c, i, count)
        sheet.set(i, c)
    sheet.add_animation(name, 0, count, fps, loop)
    return sheet


def muzzle_flash():
    def draw(c, i, n):
        s = 1.0 - i / n
        c.polygon([(4, 8), (15, 8 - 3.5 * s), (12, 8), (15, 8 + 3.5 * s)], rgba("#ffb030"))
        c.polygon([(4, 8), (13, 8 - 2 * s), (11, 8), (13, 8 + 2 * s)], rgba("#fff2a0"))
        c.circle(5, 8, 2.5 * s + 0.5, rgba("#ffffff"))
    return _anim_sheet(16, 3, draw, fps=30.0)


def burst(colors, size=16, count=4, particles=9, seed=1, fps=16.0):
    def draw(c, i, n):
        rng = random.Random(seed)
        t = (i + 1) / n
        for _ in range(particles):
            a = rng.random() * math.tau
            d = (2 + rng.random() * size * 0.4) * t
            col = colors[rng.randrange(len(colors))]
            r = max(0.6, (1.6 - t) * (0.6 + rng.random()))
            c.circle(size / 2 + math.cos(a) * d, size / 2 + math.sin(a) * d, r, col)
    return _anim_sheet(size, count, draw, fps=fps)


def blood_hit():
    return burst([rgba("#b01018"), rgba("#7a0a10"), rgba("#d02028")], seed=3)


def acid_hit():
    return burst([rgba("#8af040"), rgba("#4ab020"), rgba("#d0ff80")], seed=5)


def sparks():
    return burst([rgba("#ffd060"), rgba("#fff0b0"), rgba("#ff9030")], particles=6, seed=9, fps=20.0)


def death_puff():
    return burst([rgba("#6a6660", 200), rgba("#4a4640", 180), rgba("#8a8680", 160)], size=32, particles=14, seed=11, fps=12.0)


def revive_glow():
    def draw(c, i, n):
        t = (i + 1) / n
        c.ring(16, 16, 6 + t * 9, 4 + t * 8, rgba("#c070ff", int(230 * (1 - t) + 25)))
        for k in range(6):
            a = k / 6 * math.tau + t * 2
            c.circle(16 + math.cos(a) * 10 * t, 16 + math.sin(a) * 10 * t, 1.2, rgba("#e8b0ff"))
    return _anim_sheet(32, 6, draw, fps=12.0)


def explosion():
    def draw(c, i, n):
        t = (i + 1) / n
        rng = random.Random(21 + i)
        r = 6 + 22 * t
        if t < 0.8:
            c.circle(32, 32, r, rgba("#ff6a10", int(255 * (1 - t * 0.6))))
            c.circle(32, 32, r * 0.7, rgba("#ffb030"))
            c.circle(32, 32, r * 0.4 * (1 - t), rgba("#fff5c0"))
        for _ in range(10):
            a = rng.random() * math.tau
            d = r * (0.7 + rng.random() * 0.5)
            c.circle(32 + math.cos(a) * d, 32 + math.sin(a) * d, 3 + 5 * t, rgba("#3a3430", int(220 * (1 - t) + 30)))
    return _anim_sheet(64, 8, draw, fps=16.0)


def slam_ring():
    def draw(c, i, n):
        t = (i + 1) / n
        c.ring(32, 32, 8 + 22 * t, 5 + 21 * t, rgba("#ff4020", int(255 * (1 - t * 0.7))))
        c.ring(32, 32, 6 + 18 * t, 5 + 17 * t, rgba("#ffd080", int(200 * (1 - t))))
    return _anim_sheet(64, 6, draw, fps=14.0)


def tracer():
    sheet = SpriteSheet(32, 4, 1, 1)
    c = Canvas(32, 4)
    for x in range(32):
        a = int(255 * min(1.0, x / 10.0))
        c.put(x, 1, (255, 255, 255, a))
        c.put(x, 2, (255, 255, 255, a))
        if x > 8:
            c.put(x, 0, (255, 255, 255, a // 3))
            c.put(x, 3, (255, 255, 255, a // 3))
    sheet.set(0, c)
    sheet.add_animation("Play", 0, 1, 1.0, False)
    return sheet


def decal(kind, seed):
    sheet = SpriteSheet(32, 32, 1, 1)
    c = Canvas(32, 32)
    rng = random.Random(seed)
    if kind == "blood":
        base = rgba("#6e0a0e", 230)
        c.ellipse(16, 16, 7 + rng.random() * 3, 5 + rng.random() * 3, base, rng.random() * 3)
        for _ in range(9):
            a = rng.random() * math.tau
            d = 6 + rng.random() * 8
            c.circle(16 + math.cos(a) * d, 16 + math.sin(a) * d, 0.8 + rng.random() * 1.8, base)
    elif kind == "scorch":
        for r, alpha in ((14, 110), (10, 170), (6, 220)):
            c.circle(16, 16, r, (22, 18, 16, alpha))
    elif kind == "acid":
        c.ellipse(16, 16, 10, 8, rgba("#4a8a20", 200))
        c.ellipse(16, 15, 6, 4, rgba("#8ad040", 220))
    sheet.set(0, c)
    sheet.add_animation("Play", 0, 1, 1.0, False)
    return sheet


def puddle(color, highlight):
    def draw(c, i, n):
        rng = random.Random(40)
        for _ in range(7):
            a = rng.random() * math.tau
            d = rng.random() * 9
            c.circle(16 + math.cos(a) * d, 16 + math.sin(a) * d, 5 + rng.random() * 3, rgba(color, 200))
        rng2 = random.Random(100 + i)
        for _ in range(4):
            x, y = 16 + rng2.uniform(-9, 9), 16 + rng2.uniform(-9, 9)
            c.ring(x, y, 1.8, 0.8, rgba(highlight, 230))
    return _anim_sheet(32, 4, draw, fps=5.0, loop=True)


# --- projectiles -----------------------------------------------------------------------------------

def projectile(kind):
    def draw(c, i, n):
        if kind == "Rocket":
            c.rect(3, 6.5, 12, 9.5, rgba("#6c7c4a"))
            c.polygon([(12, 6.5), (15, 8), (12, 9.5)], rgba("#c03020"))
            c.circle(2.5, 8, 2.2 + (i % 2), rgba("#ffb030"))
        elif kind == "Acid":
            c.circle(8, 8, 4.5, rgba("#6fc030"))
            c.circle(7, 7, 2.2, rgba("#c8ff80"))
        elif kind == "Bolt":
            c.circle(8, 8, 3.5 + (i % 2) * 0.7, rgba("#a050ff"))
            c.circle(8, 8, 1.8, rgba("#f0d0ff"))
        elif kind == "Plasma":
            c.ellipse(8, 8, 6, 2.6, rgba("#40d8ff"))
            c.ellipse(9, 8, 3.5, 1.3, rgba("#e0ffff"))
        elif kind == "Bullet":
            c.ellipse(8, 8, 4, 1.2, rgba("#ffe080"))
        c.outline(OUTLINE) if kind in ("Rocket", "Acid") else None
    return _anim_sheet(16, 2, draw, fps=12.0, loop=True)


# --- pickups ---------------------------------------------------------------------------------------

def pickups():
    sheet = SpriteSheet(24, 24, 4, 7)
    gold, dark_gold = rgba("#ffc830"), rgba("#b07a10")

    def coin(c, i):
        w = (5.5, 3.5, 1.2, 3.5)[i]
        c.ellipse(12, 12, w, 5.5, dark_gold)
        c.ellipse(12, 11.5, max(w - 1.2, 0.5), 4.3, gold)
        if w > 3:
            c.put(12, 11, rgba("#fff5b0"))

    def cash(c, i):
        for k in range(3):
            c.rect(5, 13 - k * 2.5, 19, 17 - k * 2.5, rgba("#3f8a3a"))
            c.rect(6, 13.5 - k * 2.5, 18, 14.5 - k * 2.5, rgba("#7ad070"))
        c.circle(12, 10.5 - (i % 2), 1.5, rgba("#fff5b0"))

    def health(c, i):
        c.rect(5, 6, 19, 18, rgba("#e8e8e8"))
        c.rect(10.5, 7.5, 13.5, 16.5, rgba("#d02020"))
        c.rect(6.5, 10.5, 17.5, 13.5, rgba("#d02020"))
        if i:
            c.ring(12, 12, 11, 10, rgba("#ff8080", 160))

    def armor(c, i):
        c.polygon([(12, 4), (19, 7), (18, 15), (12, 20), (6, 15), (5, 7)], rgba("#3a6ad0"))
        c.polygon([(12, 6), (17, 8), (16, 14), (12, 17)], rgba("#7aa8ff"))
        if i:
            c.ring(12, 12, 11, 10, rgba("#80b0ff", 160))

    def ammo(c, i):
        c.rect(5, 9, 19, 18, rgba("#4c5a2c"))
        c.rect(5, 9, 19, 10.5, rgba("#6c7c40"))
        for k in range(4):
            c.rect(7 + k * 3, 5, 8.5 + k * 3, 9, rgba("#d0a030"))

    def key(c, i):
        glow = (0.0, 0.4, 0.8, 0.4)[i]
        c.circle(12, 12, 10, rgba("#ffe070", int(40 + 60 * glow)))
        c.ring(8, 12, 4, 2, gold)
        c.rect(11, 11, 20, 13, gold)
        c.rect(16, 13, 17.5, 16, gold)
        c.rect(18.5, 13, 20, 15, gold)

    def weapon(c, i, color):
        c.circle(12, 12, 10, rgba(color, 60 + 40 * i))
        c.rect(5, 10, 18, 13.5, rgba("#30343a"))
        c.rect(9, 13.5, 11, 17, rgba("#30343a"))
        c.rect(5, 10, 18, 11, rgba("#6a7078"))

    def perk(c, i):
        c.circle(12, 12, 9 + (i % 2), rgba("#50ffb0", 70))
        c.circle(12, 12, 6, rgba("#20c080"))
        c.circle(10.5, 10.5, 2.5, rgba("#c0ffe0"))

    rows = [("Money", coin), ("MoneyLarge", cash), ("Health", health), ("Armor", armor), ("Ammo", ammo), ("Key", key)]
    for r, (name, fn) in enumerate(rows):
        for i in range(4):
            c = Canvas(24, 24)
            fn(c, i)
            c.outline(OUTLINE)
            sheet.set(r * 4 + i, c)
        sheet.add_animation(name, r * 4, 4, 6.0, True)

    for i in range(2):
        for k, (name, color) in enumerate((("Weapon", "#a0a0a0"), ("WeaponRare", "#ffa020"))):
            c = Canvas(24, 24)
            weapon(c, i, color)
            c.outline(OUTLINE)
            sheet.set(24 + k * 2 + i, c)
    sheet.add_animation("Weapon", 24, 2, 3.0, True)
    sheet.add_animation("WeaponRare", 26, 2, 4.0, True)
    return sheet, perk


def pickups_with_perk():
    sheet, perk = pickups()
    # The perk orb shares the weapon row's spare cells via a dedicated 8th row.
    full = SpriteSheet(24, 24, 4, 8)
    full.frames[:28] = sheet.frames
    full.animations = dict(sheet.animations)
    for i in range(4):
        c = Canvas(24, 24)
        perk(c, i)
        c.outline(OUTLINE)
        full.set(28 + i, c)
    full.add_animation("Perk", 28, 4, 6.0, True)
    return full


# --- props -----------------------------------------------------------------------------------------

def props():
    sheet = SpriteSheet(32, 32, 4, 3)

    def door(c, unlocked):
        c.rect(2, 2, 30, 30, rgba("#3c4046"))
        c.rect(4, 4, 28, 28, rgba("#5a6068"))
        for y in range(6, 28, 5):
            c.rect(5, y, 27, y + 1, rgba("#474c53"))
        c.rect(13, 12, 19, 20, rgba("#2a2e33"))
        c.circle(16, 16, 2.2, rgba("#40ff60") if unlocked else rgba("#ff3020"))

    def marker(c, i):
        a = 90 + 60 * (i % 2)
        for k in range(3):
            y = 6 + k * 8 + (i % 2)
            c.polygon([(8, y + 4), (16, y), (24, y + 4), (24, y + 6), (16, y + 2), (8, y + 6)], rgba("#40ff60", a))

    def terminal(c, i):
        c.rect(5, 6, 27, 26, rgba("#2a3040"))
        c.rect(7, 8, 25, 18, rgba("#103a50") if i else rgba("#105a70"))
        c.rect(9, 10, 23, 11, rgba("#70f0ff"))
        c.rect(9, 13, 19, 14, rgba("#70f0ff"))
        c.rect(8, 20, 24, 24, rgba("#ffc830"))

    def chest(c, opened):
        c.rect(5, 9, 27, 25, rgba("#6a4424"))
        c.rect(5, 9, 27, 12, rgba("#8a5a30"))
        c.rect(15, 9, 17, 25, rgba("#c8a040"))
        if opened:
            c.rect(7, 12, 25, 23, rgba("#1c120a"))
            c.circle(16, 17, 2, rgba("#ffd050"))
        else:
            c.rect(14, 14, 18, 18, rgba("#e8c860"))

    frames = [("DoorLocked", lambda c, i: door(c, False)), ("DoorOpen", lambda c, i: door(c, True)),
              ("Terminal", terminal), ("ChestClosed", lambda c, i: chest(c, False)), ("ChestOpen", lambda c, i: chest(c, True))]
    index = 0
    for name, fn in frames:
        count = 2 if name == "Terminal" else 1
        for i in range(count):
            c = Canvas(32, 32)
            fn(c, i)
            c.outline(OUTLINE)
            sheet.set(index + i, c)
        sheet.add_animation(name, index, count, 2.0, count > 1)
        index += count
    for i in range(2):
        c = Canvas(32, 32)
        marker(c, i)
        sheet.set(index + i, c)
    sheet.add_animation("ExitMarker", index, 2, 3.0, True)
    return sheet


# --- environment atlases ---------------------------------------------------------------------------

THEMES = {
    "Office": {"floor": ("#5a5048", "#4a4038"), "pattern": "carpet", "wall_top": "#2c2a2e", "wall_side": ("#6a6258", "#5a5248"),
               "props": ["desk", "cabinet", "crate", "copier"]},
    "Industrial": {"floor": ("#4e5254", "#3e4244"), "pattern": "plates", "wall_top": "#23262a", "wall_side": ("#6a3a2a", "#5a3022"),
                   "props": ["crate", "barrel", "machine", "sandbags"]},
    "Lab": {"floor": ("#b8bcb6", "#9ea29c"), "pattern": "tiles", "wall_top": "#303638", "wall_side": ("#8a9294", "#737b7d"),
            "props": ["labtable", "server", "barrel", "crate"]},
}


def floor_atlas(theme, seed):
    t = THEMES[theme]
    c = Canvas(128, 128)
    base, dark = rgba(t["floor"][0]), rgba(t["floor"][1])
    rng = random.Random(seed)
    for tile in range(16):
        ox, oy = (tile % 4) * 32, (tile // 4) * 32
        c.rect(ox, oy, ox + 32, oy + 32, base)
        if t["pattern"] == "carpet":
            for _ in range(40):
                c.put(ox + rng.randrange(32), oy + rng.randrange(32), shade(base, 0.9 + rng.random() * 0.2))
            c.rect(ox, oy, ox + 32, oy + 1, dark)
            c.rect(ox, oy, ox + 1, oy + 32, dark)
        elif t["pattern"] == "plates":
            c.rect(ox, oy, ox + 32, oy + 1, dark)
            c.rect(ox, oy, ox + 1, oy + 32, dark)
            for bx, by in ((3, 3), (28, 3), (3, 28), (28, 28)):
                c.put(ox + bx, oy + by, shade(base, 1.3))
            for k in range(4):
                c.line(ox + 6 + k * 6, oy + 8, ox + 9 + k * 6, oy + 5, shade(base, 1.08))
        else:
            for gx in range(0, 32, 8):
                c.rect(ox + gx, oy, ox + gx + 1, oy + 32, dark)
                c.rect(ox, oy + gx, ox + 32, oy + gx + 1, dark)
        # Grime, cracks, and the odd stain - no two tiles alike.
        for _ in range(rng.randrange(2, 6)):
            x, y = ox + rng.randrange(2, 30), oy + rng.randrange(2, 30)
            c.circle(x, y, rng.random() * 2.5, shade(base, 0.8))
        if rng.random() < 0.4:
            x, y = ox + rng.randrange(4, 28), oy + rng.randrange(4, 28)
            for _ in range(6):
                nx, ny = x + rng.randrange(-3, 4), y + rng.randrange(-3, 4)
                c.line(x, y, nx, ny, shade(base, 0.6))
                x, y = nx, ny
        if rng.random() < 0.15:
            c.ellipse(ox + 16, oy + 16, 6, 4, (90, 10, 12, 140), rng.random() * 3)
    return c


def wall_atlas(theme):
    """Two cells: [0] the top face seen from above, [1] the side face."""
    t = THEMES[theme]
    c = Canvas(64, 32)
    top = rgba(t["wall_top"])
    c.rect(0, 0, 32, 32, top)
    rng_top = random.Random(3)
    for _ in range(60):
        c.put(rng_top.randrange(32), rng_top.randrange(32), shade(top, 0.85 + rng_top.random() * 0.3))
    side_a, side_b = rgba(t["wall_side"][0]), rgba(t["wall_side"][1])
    c.rect(32, 0, 64, 32, side_a)
    for row in range(8):
        y = row * 4
        c.rect(32, y, 64, y + 1, side_b)
        off = 0 if row % 2 == 0 else 4
        for x in range(32 + off, 64, 8):
            c.rect(x, y, x + 1, y + 4, side_b)
    rng = random.Random(len(theme))
    for _ in range(20):
        c.put(32 + rng.randrange(32), rng.randrange(32), shade(side_a, 0.75))
    return c


def _prop_top(c, kind, ox, oy, rng):
    palette = {
        "desk": ("#7a5a3a", "#5a4028"), "cabinet": ("#6a7078", "#4a5058"), "crate": ("#8a6a3a", "#6a4a24"),
        "copier": ("#c8c8c0", "#8a8a84"), "barrel": ("#8a3a24", "#5a2416"), "machine": ("#4a5a5a", "#2a3434"),
        "sandbags": ("#9a8a5a", "#6a5e3a"), "labtable": ("#d0d4d0", "#9aa09a"), "server": ("#24282c", "#14181c"),
    }[kind]
    main, edge = rgba(palette[0]), rgba(palette[1])
    if kind == "barrel":
        c.circle(ox + 16, oy + 16, 14, edge)
        c.circle(ox + 16, oy + 16, 12, main)
        c.ring(ox + 16, oy + 16, 8, 7, edge)
        return
    if kind == "sandbags":
        for yy in range(3):
            for xx in range(2):
                c.ellipse(ox + 9 + xx * 14, oy + 7 + yy * 9, 7, 4.5, main)
                c.ellipse(ox + 9 + xx * 14, oy + 6 + yy * 9, 5, 2.5, shade(main, 1.15))
        return
    c.rect(ox + 1, oy + 1, ox + 31, oy + 31, edge)
    c.rect(ox + 3, oy + 3, ox + 29, oy + 29, main)
    if kind == "crate":
        c.line(ox + 3, oy + 3, ox + 29, oy + 29, edge, 2)
        c.line(ox + 29, oy + 3, ox + 3, oy + 29, edge, 2)
    elif kind == "desk":
        c.rect(ox + 6, oy + 6, ox + 14, oy + 12, rgba("#e8e8e0"))
        c.rect(ox + 18, oy + 8, ox + 26, oy + 16, rgba("#2a2a30"))
    elif kind == "cabinet":
        for k in range(3):
            c.rect(ox + 5, oy + 5 + k * 8, ox + 27, oy + 11 + k * 8, shade(main, 1.1))
    elif kind == "copier":
        c.rect(ox + 6, oy + 6, ox + 26, oy + 14, rgba("#50505a"))
    elif kind == "machine":
        c.circle(ox + 11, oy + 11, 5, rgba("#8a9a9a"))
        c.rect(ox + 18, oy + 18, ox + 27, oy + 27, rgba("#c0a030"))
    elif kind == "labtable":
        for k in range(3):
            c.circle(ox + 9 + k * 7, oy + 10, 2.2, rgba("#60d0ff"))
        c.rect(ox + 6, oy + 18, ox + 26, oy + 22, rgba("#707a70"))
    elif kind == "server":
        for k in range(5):
            c.rect(ox + 5, oy + 5 + k * 5, ox + 27, oy + 8 + k * 5, rgba("#303840"))
            c.put(ox + 7, oy + 6 + k * 5, rgba("#40ff60") if rng.random() < 0.6 else rgba("#ff4020"))


def obstacle_atlas(theme):
    """Two columns (top face, side face) by four rows (prop variants)."""
    t = THEMES[theme]
    c = Canvas(64, 128)
    rng = random.Random(7)
    for row, kind in enumerate(t["props"]):
        _prop_top(c, kind, 0, row * 32, rng)
        top_color = rgba(tuple(int(v) for v in c.px[row * 32 + 16, 16][:3]))
        c.rect(32, row * 32, 64, row * 32 + 32, shade(top_color, 0.65))
        c.rect(32, row * 32, 64, row * 32 + 3, shade(top_color, 0.85))
        c.rect(32, row * 32 + 29, 64, row * 32 + 32, shade(top_color, 0.45))
    return c


def decoration_atlas():
    """Eight bits of floor debris, 4x2, on a transparent background."""
    c = Canvas(128, 64)
    rng = random.Random(77)

    def cell(i):
        return (i % 4) * 32, (i // 4) * 32

    x, y = cell(0)
    for _ in range(4):
        px, py = x + rng.randrange(4, 24), y + rng.randrange(4, 24)
        c.rect(px, py, px + 6, py + 8, rgba("#e8e4d8"))
        c.rect(px + 1, py + 2, px + 5, py + 3, rgba("#9a968a"))
    x, y = cell(1)
    for _ in range(12):
        c.circle(x + rng.randrange(4, 28), y + rng.randrange(4, 28), rng.random() * 2 + 0.5, rgba("#6a625a"))
    x, y = cell(2)
    c.line(x + 6, y + 10, x + 24, y + 20, rgba("#e0dccc"), 2)
    c.circle(x + 6, y + 10, 2, rgba("#e0dccc"))
    c.circle(x + 24, y + 20, 2, rgba("#e0dccc"))
    c.circle(x + 20, y + 8, 3.5, rgba("#e0dccc"))
    x, y = cell(3)
    c.ellipse(x + 16, y + 16, 11, 7, rgba("#1a2830", 150))
    x, y = cell(4)
    for k in range(5):
        c.line(x + 16, y + 16, x + 16 + math.cos(k * 1.3) * 12, y + 16 + math.sin(k * 1.3) * 12, rgba("#1c1a18", 200))
    x, y = cell(5)
    for _ in range(7):
        px, py = x + rng.randrange(4, 26), y + rng.randrange(4, 26)
        c.rect(px, py, px + 3, py + 1, rgba("#c8a040"))
    x, y = cell(6)
    c.rect(x + 8, y + 10, x + 22, y + 20, rgba("#4a5a3a"))
    c.rect(x + 20, y + 14, x + 26, y + 16, rgba("#3a3a3a"))
    x, y = cell(7)
    c.ellipse(x + 16, y + 16, 12, 3, rgba("#5a0a0c", 180), 0.6)
    return c


# --- UI ---------------------------------------------------------------------------------------------

def crosshair():
    c = Canvas(32, 32)
    white = (255, 255, 255, 255)
    for a, b in ((4, 11), (21, 28)):
        c.rect(15, a, 17, b, white)
        c.rect(a, 15, b, 17, white)
    c.rect(15, 15, 17, 17, white)
    c.outline((0, 0, 0, 200))
    return c


def vignette():
    c = Canvas(128, 128)
    for y in range(128):
        for x in range(128):
            d = math.hypot((x - 63.5) / 64, (y - 63.5) / 64)
            a = max(0.0, min(1.0, (d - 0.55) / 0.5))
            c.px[y, x] = (255, 255, 255, int(255 * a * a))
    return c


def title_background():
    c = Canvas(480, 270)
    for y in range(270):
        t = y / 270
        col = mix(rgba("#0c0a14"), rgba("#3a0c10"), t ** 1.6)
        c.rect(0, y, 480, y + 1, col)
    c.circle(360, 80, 38, rgba("#a01818"))
    c.circle(352, 72, 30, rgba("#c02820"))
    rng = random.Random(5)
    x = 0
    while x < 480:
        w = rng.randrange(18, 46)
        h = rng.randrange(50, 150)
        c.rect(x, 270 - h, x + w, 270, rgba("#07060a"))
        for wy in range(270 - h + 6, 262, 9):
            for wx in range(x + 3, x + w - 3, 7):
                if rng.random() < 0.12:
                    c.rect(wx, wy, wx + 3, wy + 4, rgba("#c89030", 180))
        x += w + rng.randrange(0, 6)
    for k in range(9):
        zx = 30 + k * 52 + rng.randrange(-10, 10)
        c.ellipse(zx, 255, 5, 8, rgba("#020203"))
        c.circle(zx, 244, 4, rgba("#020203"))
        c.line(zx + 2, 250, zx + 12, 247, rgba("#020203"), 2)
    return c
