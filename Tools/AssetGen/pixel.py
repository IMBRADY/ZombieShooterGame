"""Tiny pixel-art toolkit used by the asset generators.

Everything is drawn into numpy RGBA arrays at native pixel resolution (no anti-aliasing), then
finished with a dark outline pass - the same steps a pixel artist takes by hand, done in code so
the art is reproducible and lives in source control next to the game that uses it.
"""

import math
import numpy as np
from PIL import Image


def rgba(hex_or_tuple, alpha=255):
    if isinstance(hex_or_tuple, str):
        h = hex_or_tuple.lstrip('#')
        return (int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16), alpha)
    if len(hex_or_tuple) == 3:
        return (*hex_or_tuple, alpha)
    return tuple(hex_or_tuple)


def shade(color, factor):
    """Darkens (<1) or lightens (>1) a colour, keeping alpha."""
    r, g, b, a = rgba(color)
    if factor >= 1.0:
        f = factor - 1.0
        return (int(r + (255 - r) * f), int(g + (255 - g) * f), int(b + (255 - b) * f), a)
    return (int(r * factor), int(g * factor), int(b * factor), a)


def mix(a, b, t):
    a, b = rgba(a), rgba(b)
    return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(4))


class Canvas:
    def __init__(self, width, height):
        self.w = width
        self.h = height
        self.px = np.zeros((height, width, 4), dtype=np.uint8)

    # --- primitives -----------------------------------------------------------------------------

    def put(self, x, y, color):
        x, y = int(round(x)), int(round(y))
        if 0 <= x < self.w and 0 <= y < self.h:
            c = rgba(color)
            if c[3] >= 255 or self.px[y, x, 3] == 0:
                self.px[y, x] = c
            else:
                a = c[3] / 255.0
                self.px[y, x, :3] = (np.array(c[:3]) * a + self.px[y, x, :3] * (1 - a)).astype(np.uint8)
                self.px[y, x, 3] = max(self.px[y, x, 3], c[3])

    def rect(self, x0, y0, x1, y1, color):
        for y in range(int(math.floor(y0)), int(math.ceil(y1))):
            for x in range(int(math.floor(x0)), int(math.ceil(x1))):
                self.put(x, y, color)

    def ellipse(self, cx, cy, rx, ry, color, angle=0.0):
        """Filled ellipse, optionally rotated (radians)."""
        ca, sa = math.cos(angle), math.sin(angle)
        r = int(max(rx, ry)) + 2
        for y in range(int(cy) - r, int(cy) + r + 1):
            for x in range(int(cx) - r, int(cx) + r + 1):
                dx, dy = x + 0.5 - cx, y + 0.5 - cy
                u = dx * ca + dy * sa
                v = -dx * sa + dy * ca
                if (u / max(rx, 0.01)) ** 2 + (v / max(ry, 0.01)) ** 2 <= 1.0:
                    self.put(x, y, color)

    def circle(self, cx, cy, r, color):
        self.ellipse(cx, cy, r, r, color)

    def ring(self, cx, cy, r_outer, r_inner, color):
        for y in range(int(cy - r_outer) - 1, int(cy + r_outer) + 2):
            for x in range(int(cx - r_outer) - 1, int(cx + r_outer) + 2):
                d = math.hypot(x + 0.5 - cx, y + 0.5 - cy)
                if r_inner <= d <= r_outer:
                    self.put(x, y, color)

    def line(self, x0, y0, x1, y1, color, width=1):
        steps = int(max(abs(x1 - x0), abs(y1 - y0)) * 2) + 1
        for i in range(steps + 1):
            t = i / steps
            x = x0 + (x1 - x0) * t
            y = y0 + (y1 - y0) * t
            if width <= 1:
                self.put(x, y, color)
            else:
                self.circle(x, y, width / 2.0, color)

    def polygon(self, points, color):
        xs = [p[0] for p in points]
        ys = [p[1] for p in points]
        for y in range(int(min(ys)), int(max(ys)) + 1):
            for x in range(int(min(xs)), int(max(xs)) + 1):
                if _point_in_polygon(x + 0.5, y + 0.5, points):
                    self.put(x, y, color)

    # --- finishing passes -------------------------------------------------------------------------

    def outline(self, color=(18, 16, 20, 255), threshold=128):
        """A 1px outline around everything opaque - the pixel-art silhouette."""
        solid = self.px[:, :, 3] >= threshold
        grown = np.zeros_like(solid)
        grown[1:, :] |= solid[:-1, :]
        grown[:-1, :] |= solid[1:, :]
        grown[:, 1:] |= solid[:, :-1]
        grown[:, :-1] |= solid[:, 1:]
        edge = grown & ~solid
        self.px[edge] = rgba(color)

    def paste(self, other, ox, oy):
        for y in range(other.h):
            for x in range(other.w):
                c = other.px[y, x]
                if c[3] > 0:
                    self.put(ox + x, oy + y, tuple(int(v) for v in c))

    def rotated(self, degrees):
        img = Image.fromarray(self.px, 'RGBA').rotate(degrees, resample=Image.NEAREST)
        out = Canvas(self.w, self.h)
        out.px = np.array(img)
        return out

    def scaled(self, factor):
        img = Image.fromarray(self.px, 'RGBA').resize((self.w * factor, self.h * factor), Image.NEAREST)
        out = Canvas(self.w * factor, self.h * factor)
        out.px = np.array(img)
        return out

    def image(self):
        return Image.fromarray(self.px, 'RGBA')


def _point_in_polygon(x, y, points):
    inside = False
    j = len(points) - 1
    for i in range(len(points)):
        xi, yi = points[i]
        xj, yj = points[j]
        if (yi > y) != (yj > y) and x < (xj - xi) * (y - yi) / (yj - yi + 1e-9) + xi:
            inside = not inside
        j = i
    return inside


class SpriteSheet:
    """Frames laid out left-to-right, top-to-bottom - the layout USpriteSheetDataAsset expects."""

    def __init__(self, frame_w, frame_h, columns, rows):
        self.fw, self.fh = frame_w, frame_h
        self.cols, self.rows = columns, rows
        self.frames = [None] * (columns * rows)
        self.animations = {}

    def set(self, index, canvas):
        self.frames[index] = canvas

    def add_animation(self, name, start, count, fps=8.0, loop=True):
        self.animations[name] = {"start": start, "count": count, "fps": fps, "loop": loop}

    def save(self, path):
        sheet = Canvas(self.fw * self.cols, self.fh * self.rows)
        for i, frame in enumerate(self.frames):
            if frame is not None:
                sheet.paste(frame, (i % self.cols) * self.fw, (i // self.cols) * self.fh)
        sheet.image().save(path)
        return {"columns": self.cols, "rows": self.rows, "animations": self.animations}
