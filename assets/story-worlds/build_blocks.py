# The story worlds' own blocks in the field: per world the fixed blocks' skin (block-unwrap.tga) and the
# destructible ones' (stone-unwrap.tga, their debris too), painted in the world's palette. Plain python:
#   python build_blocks.py [world ...]     writes client/data/game/story/world<n>/{block,stone}-unwrap.tga
# The game's block (level-castle/block.hjb) is a cube unwrapped into a cross on a 512 texture: a column of four faces
# 171 x 128 texels at x 170..341 and one face either side of its second. In the field the column's first face is the
# top (its upwards towards the north), the second the side facing south (towards the camera), the ones beside it west
# and east, the other two bottom and north (hardly ever seen). Faces are painted square in the world's proportions
# and scaled into their cells.
import math
import os
import sys

import numpy as np
from PIL import Image

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
import palettes  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
STORY = os.path.join(HERE, "..", "..", "client", "data", "game", "story")
SIZE = 512
FACE = 256  # faces are painted at this many texels a tile, then scaled into their cells
SIDE = 205  # a side face's height at that scale: the blocks are 0.8 high
# the cross' cells: left, top, right, bottom in the texture (rows from its top), and which face goes there
CELLS = [((170, 0, 341, 128), "top"), ((0, 128, 170, 256), "side"), ((170, 128, 341, 256), "side"),
         ((341, 128, 512, 256), "side"), ((170, 256, 341, 384), "bottom"), ((170, 384, 341, 512), "side")]


# --- noise and shapes ---------------------------------------------------------------------------------------------

WORLD = [1]  # the world being painted, its palette is p()'s


def p(index, shade=1.0):
    """A colour of the world's palette (assets/palettes.py), shade times as light (above 1 towards white)."""
    return palettes.color(WORLD[0], index, shade)


def noise(rng, h, w, cells, octaves=4, stretch=(1.0, 1.0)):
    """Smooth value noise in 0..1, cells across the wider side; stretch (y, x) draws it out along an axis."""
    total = np.zeros((h, w))
    amplitude = norm = 0.0
    amplitude = 1.0
    for o in range(octaves):
        n = cells * 2 ** o
        gy, gx = max(2, int(n * h / max(h, w) / stretch[0]) + 2), max(2, int(n * w / max(h, w) / stretch[1]) + 2)
        grid = rng.random((gy, gx))
        y = np.linspace(0, gy - 1.001, h)
        x = np.linspace(0, gx - 1.001, w)
        iy, ix = y.astype(int), x.astype(int)
        fy, fx = y - iy, x - ix
        fy, fx = (fy * fy * (3 - 2 * fy))[:, None], (fx * fx * (3 - 2 * fx))[None, :]
        a, b = grid[np.ix_(iy, ix)], grid[np.ix_(iy, ix + 1)]
        c, d = grid[np.ix_(iy + 1, ix)], grid[np.ix_(iy + 1, ix + 1)]
        total += amplitude * ((a * (1 - fx) + b * fx) * (1 - fy) + (c * (1 - fx) + d * fx) * fy)
        norm += amplitude
        amplitude *= 0.5
    return total / norm


def smooth(e0, e1, v):
    t = np.clip((v - e0) / (e1 - e0), 0, 1)
    return t * t * (3 - 2 * t)


def mix(a, b, t):
    a, b = np.asarray(a, float), np.asarray(b, float)
    t = np.asarray(t, float)
    if t.ndim:
        t = t[..., None]
    return a * (1 - t) + b * t


def tone(rng, h, w, low, high, cells=6, octaves=4, stretch=(1.0, 1.0)):
    return mix(low, high, noise(rng, h, w, cells, octaves, stretch))


def coords(h, w):
    y, x = np.mgrid[0:h, 0:w].astype(float)
    return y / h, x / w


def edge_distance(h, w):
    """Distance to the face's nearest edge, in face widths (0 at the edge)."""
    v, u = coords(h, w)
    return np.minimum(np.minimum(u, 1 - u) * w, np.minimum(v, 1 - v) * h) / FACE


def bevel(rgb, width=0.035, light=1.25, dark=0.62):
    """The block's rounded edge: lit along the top and left, in shade along the bottom and right, a little
    occlusion creeping in from all of them."""
    h, w = rgb.shape[:2]
    v, u = coords(h, w)
    wy, wx = width * FACE / h, width * FACE / w
    top, left = smooth(wy, 0, v), smooth(wx, 0, u)
    bottom, right = smooth(1 - wy, 1, v), smooth(1 - wx, 1, u)
    factor = 1 + (light - 1) * np.maximum(top, left * 0.7) - (1 - dark) * np.maximum(bottom, right * 0.8)
    factor *= 1 - 0.18 * smooth(0.12, 0.0, edge_distance(h, w))
    return rgb * factor[..., None]


def groove(rgb, mask, depth=0.45, highlight=0.25, shift=2):
    """Cuts mask (0..1) into the surface: dark in it, a lit lip below where light catches its lower edge."""
    lip = np.roll(mask, shift, axis=0) * (1 - mask)
    return rgb * (1 - depth * mask)[..., None] * (1 + highlight * lip)[..., None]


def lines(t, period, width):
    """1 on lines every period of t, width thick (both in t's units), soft edged."""
    f = np.abs((t / period + 0.5) % 1.0 - 0.5) * period
    return smooth(width, width * 0.4, f)


def box_frame(h, w, inset, width):
    """A rectangular groove inset from the edges (in face widths)."""
    v, u = coords(h, w)
    du, dv = np.minimum(u, 1 - u) * w / FACE, np.minimum(v, 1 - v) * h / FACE
    d = np.minimum(du, dv)
    inside = (du > inset - width) & (dv > inset - width)
    return smooth(width, width * 0.3, np.abs(d - inset)) * inside


def dots(h, w, points, radius, rgb, color, shine=0.5):
    """Round studs (rivets, nail heads) at points (u, v in 0..1), radius in face widths, lit from the top left."""
    v, u = coords(h, w)
    for pu, pv in points:
        d = np.hypot((u - pu) * w, (v - pv) * h) / (radius * FACE)
        on = d < 1
        light = np.clip(1 - np.hypot((u - pu) * w + radius * FACE * 0.35, (v - pv) * h + radius * FACE * 0.35) /
                        (radius * FACE), 0, 1)
        stud = np.asarray(color, float) * (0.7 + shine * light)[..., None]
        rgb = np.where(on[..., None], stud, rgb)
        ring = (d >= 1) & (d < 1.35)
        rgb = np.where(ring[..., None], rgb * 0.6, rgb)
    return rgb


def voronoi(rng, h, w, count):
    """Per texel the distance to the nearest and second nearest of count points, in face widths, and the nearest's
    index."""
    points = rng.random((count, 2))
    v, u = coords(h, w)
    f1 = np.full((h, w), 9.0)
    f2 = np.full((h, w), 9.0)
    index = np.zeros((h, w), int)
    for i, (pu, pv) in enumerate(points):
        d = np.hypot((u - pu) * w, (v - pv) * h) / FACE
        closer = d < f1
        f2 = np.where(closer, f1, np.minimum(f2, d))
        index = np.where(closer, i, index)
        f1 = np.where(closer, d, f1)
    return f1, f2, index


def planks(rng, h, w, count, wood_low, wood_high, vertical=False, gap=0.012):
    """Boards side by side with grain along them, each its own shade, dark gaps between."""
    v, u = coords(h, w)
    across = u if vertical else v
    span = (w if vertical else h) / FACE
    board = np.floor(across * count).astype(int)
    grain = noise(rng, h, w, 10, 4, stretch=(1.0, 8.0) if not vertical else (8.0, 1.0))
    fine = noise(rng, h, w, 40, 2, stretch=(1.0, 6.0) if not vertical else (6.0, 1.0))
    shade = rng.random(count + 1)[board]
    rgb = mix(wood_low, wood_high, np.clip(0.45 * grain + 0.25 * fine + 0.3 * shade, 0, 1))
    seam = lines(across * span, span / count, gap)
    return groove(rgb, seam, 0.6, 0.2)


# --- the skins ----------------------------------------------------------------------------------------------------

def limestone(rng, h, w, top):
    """World 1's fixed block: dressed limestone, a chiselled margin; on top a carved rosette."""
    rgb = tone(rng, h, w, p(2, 0.84), p(2, 1.0), 5)
    rgb = rgb * (0.94 + 0.12 * noise(rng, h, w, 60, 2))[..., None]
    # a faint green bloom of lichen near the foot
    v, u = coords(h, w)
    lichen = smooth(0.55, 0.75, noise(rng, h, w, 8)) * (0.3 if top else smooth(0.4, 1.0, v))
    rgb = mix(rgb, p(1, 1.05), lichen * 0.5)
    rgb = groove(rgb, box_frame(h, w, 0.1, 0.012), 0.35, 0.3)
    if top:
        r = np.hypot(u - 0.5, (v - 0.5) * h / w)
        a = np.arctan2(v - 0.5, u - 0.5)
        petals = smooth(0.012, 0.004, np.abs(r - 0.17 * (0.75 + 0.25 * np.abs(np.cos(a * 4)))))
        ring = smooth(0.012, 0.004, np.abs(r - 0.26)) + smooth(0.01, 0.003, np.abs(r - 0.06))
        rgb = groove(rgb, np.clip(petals + ring, 0, 1), 0.4, 0.3)
    return bevel(rgb)


def timber_crate(rng, h, w, top):
    """World 1's destructible block: a crate of warm planks in a frame, braced on its sides, nailed."""
    rgb = planks(rng, h, w, 4, p(4, 0.6), p(4, 0.88), vertical=top)
    v, u = coords(h, w)
    frame_w = 0.13
    edge = (np.minimum(u, 1 - u) * w / FACE < frame_w) | (np.minimum(v, 1 - v) * h / FACE < frame_w)
    frame = planks(rng, h, w, 1, p(4, 0.45), p(4, 0.62), vertical=False)
    if not top:
        # the brace from corner to corner
        d = np.abs((u * w - (1 - v) * h * w / h) * h / math.hypot(w, h)) / FACE
        edge |= d < frame_w * 0.45
    rgb = np.where(edge[..., None], frame, rgb)
    rgb = groove(rgb, box_frame(h, w, frame_w, 0.008), 0.5, 0.2)
    k = frame_w * 0.5
    nails = [(k * FACE / w, k * FACE / h), (1 - k * FACE / w, k * FACE / h), (k * FACE / w, 1 - k * FACE / h),
             (1 - k * FACE / w, 1 - k * FACE / h)]
    rgb = dots(h, w, nails, 0.018, rgb, p(3, 0.45))
    return bevel(rgb, light=1.15, dark=0.7)


def relief(rgb, height, strength):
    """Lights a height field (0..1 over the face) from the top left: slopes facing it brighter, the others darker."""
    gy, gx = np.gradient(height)
    return rgb * np.clip(1 - strength * (gx + gy) * FACE, 0.6, 1.4)[..., None]


def dressed_sandstone(rng, h, w, top):
    """World 2's fixed block: a dressed sandstone block, a smooth drafted margin round a rough pitched face with the
    mason's tool marks in it, faint beds across. Calm: one hue, the relief doing the work."""
    v, u = coords(h, w)
    margin = 0.1
    d = edge_distance(h, w)
    inner = smooth(margin - 0.01, margin + 0.01, d)
    rough = noise(rng, h, w, 7, 4) * 0.7 + noise(rng, h, w, 22, 2) * 0.3
    height = inner * (0.5 + 0.5 * rough) * 0.04 + (1 - inner) * 0.0
    rgb = mix(p(1, 1.05), p(2, 1.0), 0.55 * noise(rng, h, w, 5, 3) + 0.25 * inner)
    if not top:
        beds = (np.sin((v * h / FACE) * 2 * math.pi * 6 + noise(rng, h, w, 3, 2) * 3) * 0.5 + 0.5)
        rgb = rgb * (0.96 + 0.06 * beds)[..., None]
    # the claw chisel's strokes across the drafted margin, diagonal, faint
    strokes = lines((u * w + v * h) / FACE, 0.018, 0.004) * (1 - inner)
    rgb = rgb * (1 - 0.08 * strokes)[..., None]
    rgb = relief(rgb, height, 0.5)
    rgb = groove(rgb, smooth(0.008, 0.002, np.abs(d - margin)), 0.25, 0.25)
    return bevel(rgb, 0.035, 1.18, 0.66)


def crystal_block(rng, h, w, top):
    """World 2's destructible block: dark slate grown through with violet crystals like the ridge's, faceted prisms
    on the sides, their cut ends on top, glowing softly from inside; the rock round them matte and calm, the
    crystals only a part of each face so it doesn't sparkle all over."""
    from PIL import ImageDraw
    scale = 2  # drawn at twice the size, smoothed down
    H, W = h * scale, w * scale
    rgb = mix(p(3, 0.62), p(3, 0.86), 0.6 * noise(rng, h, w, 6, 4) + 0.4 * noise(rng, h, w, 30, 2))
    layers = {name: Image.new("L", (W, H), 0) for name in ("dark", "light", "ridge", "glow")}
    draw = {name: ImageDraw.Draw(img) for name, img in layers.items()}

    def prism(x, y, angle, length, width):
        """A crystal from (x, y) (pixels) towards angle (radians, 0 up), its two facets and the ridge between."""
        ax, ay = math.sin(angle), -math.cos(angle)
        px, py = -ay, ax
        tip = (x + ax * length, y + ay * length)
        shoulder = 0.78
        sx, sy = x + ax * length * shoulder, y + ay * length * shoulder
        left = [(x - px * width, y - py * width), (sx - px * width, sy - py * width), tip, (sx, sy), (x, y)]
        right = [(x, y), (sx, sy), tip, (sx + px * width, sy + py * width), (x + px * width, y + py * width)]
        draw["dark"].polygon(left, fill=255)
        draw["light"].polygon(right, fill=255)
        draw["ridge"].line([(x, y), (sx, sy), tip], fill=255, width=max(1, int(width * 0.18)))
        # brighter towards the tip, where the light gathers
        for k in range(6):
            t = k / 6
            cx, cy = x + ax * length * (0.3 + 0.6 * t), y + ay * length * (0.3 + 0.6 * t)
            r = width * (1.0 - 0.6 * t)
            draw["glow"].ellipse((cx - r, cy - r, cx + r, cy + r), fill=int(90 + 120 * t))

    def hexagon(x, y, r, turn):
        """A crystal's cut end seen from above: six facets lit from the top left."""
        corners = [(x + r * math.cos(turn + k * math.pi / 3), y + r * math.sin(turn + k * math.pi / 3)) for k in range(6)]
        for k in range(6):
            a = turn + (k + 0.5) * math.pi / 3
            facing = math.cos(a + 2.4)  # towards the top left
            target = "light" if facing > 0 else "dark"
            draw[target].polygon([(x, y), corners[k], corners[(k + 1) % 6]], fill=255)
        draw["ridge"].polygon([(x + 0.35 * (cx - x), y + 0.35 * (cy - y)) for cx, cy in corners], fill=255)
        draw["glow"].ellipse((x - r * 0.9, y - r * 0.9, x + r * 0.9, y + r * 0.9), fill=200)

    def cluster(cx, cy, count, reach, spread, lean):
        """Shards radiating from a point (pixels), longest in the middle of the fan."""
        for k in range(count):
            a = lean + (k / max(count - 1, 1) - 0.5) * spread + rng.uniform(-0.15, 0.15)
            length = reach * (1.0 - 0.45 * abs(k / max(count - 1, 1) - 0.5) * 2) * rng.uniform(0.75, 1.05)
            prism(cx, cy, a, length, length * rng.uniform(0.13, 0.18))

    rim = Image.new("L", (W, H), 0)
    if top:
        # a geode broken open: a pale agate rim round a hollow lined with small crystal points, lit from the top left
        cx, cy = rng.uniform(0.42, 0.58) * W, rng.uniform(0.42, 0.58) * H
        radius = 0.33 * W
        outline = [(cx + math.cos(a) * radius * rng.uniform(0.85, 1.05) * 1.05,
                    cy + math.sin(a) * radius * rng.uniform(0.85, 1.05) * 0.95) for a in np.linspace(0, 2 * math.pi, 14, endpoint=False)]
        ImageDraw.Draw(rim).polygon(outline, fill=255)
        hollow = Image.new("L", (W, H), 0)
        ImageDraw.Draw(hollow).polygon([(cx + (x - cx) * 0.88, cy + (y - cy) * 0.88) for x, y in outline], fill=255)
        inside = np.asarray(hollow) > 0
        size = 0.045 * W
        for y in np.arange(cy - radius, cy + radius, size * 0.8):
            for x in np.arange(cx - radius, cx + radius, size * 0.8):
                px, py = x + rng.uniform(-0.35, 0.35) * size, y + rng.uniform(-0.35, 0.35) * size
                if 0 <= int(py) < H and 0 <= int(px) < W and inside[int(py), int(px)]:
                    r = size * rng.uniform(0.55, 0.85)
                    # a point seen from above: four facets, the two towards the top left lit
                    n, e, so, we = (px, py - r), (px + r, py), (px, py + r), (px - r, py)
                    draw["light"].polygon([(px, py), we, n], fill=255)
                    draw["light"].polygon([(px, py), n, e], fill=170)
                    draw["dark"].polygon([(px, py), e, so], fill=255)
                    draw["dark"].polygon([(px, py), so, we], fill=200)
                    draw["glow"].ellipse((px - r * 0.4, py - r * 0.4, px + r * 0.4, py + r * 0.4), fill=230)
        layers["glow"] = Image.composite(layers["glow"], Image.new("L", (W, H), 120), hollow)
        rim = Image.composite(rim, Image.new("L", (W, H), 0), Image.eval(hollow, lambda v: 255 - v))
    else:
        # fans growing up from the block's foot, one bigger, leaning apart
        for i in range(rng.integers(3, 5)):
            cluster(rng.uniform(0.12, 0.88) * W, H * rng.uniform(0.95, 1.05), rng.integers(3, 6),
                    rng.uniform(0.45, 0.72 if i == 0 else 0.5) * H, rng.uniform(0.9, 1.4), rng.uniform(-0.3, 0.3))

    def mask(name):
        img = layers[name].resize((w, h), Image.LANCZOS)
        return np.asarray(img, float) / 255

    dark, light, ridge, glow = mask("dark"), mask("light"), mask("ridge"), mask("glow")
    crystal = np.clip(dark + light, 0, 1)
    agate = np.asarray(rim.resize((w, h), Image.LANCZOS), float) / 255
    rgb = mix(rgb, mix(p(2, 0.58), p(2, 0.78), noise(rng, h, w, 25, 2)), agate * 0.85)
    rgb = mix(rgb, p(4, 0.55), dark)
    rgb = mix(rgb, p(4, 0.95), light)
    rgb = mix(rgb, p(4, 1.45), ridge * crystal * 0.8)
    # the rock darkens round the crystals, where they've pushed through
    halo = np.clip(np.asarray(Image.fromarray((crystal * 255).astype(np.uint8)).resize((w // 8, h // 8)).resize((w, h), Image.BILINEAR), float) / 255 * 1.5, 0, 1)
    rgb = rgb * (1 - 0.25 * halo * (1 - crystal))[..., None]
    rgb = bevel(rgb, 0.04, 1.12, 0.68)
    return rgb, crystal * (0.25 + 0.45 * glow)


def glass_block(rng, h, w, top):
    """World 2's destructible block: a cube of ruby like the ridge's crystals, deep and clear: lit
    from inside, brightest at its edges where the glass is thickest seen edge on, a few large planes inside where
    the light breaks, one soft highlight sweeping across. Calm on purpose, no small detail."""
    from PIL import ImageDraw
    v, u = coords(h, w)
    # deep in the middle, lighter towards the top left where the light comes in
    rgb = mix(p(4, 0.42), p(4, 0.78), smooth(1.4, 0.0, u + v) * 0.7 + 0.3 * noise(rng, h, w, 3, 2))
    # large planes inside, each a little lighter or darker than the glass round it
    scale = 2
    planes = Image.new("L", (w * scale, h * scale), 128)
    draw = ImageDraw.Draw(planes)
    for _ in range(4):
        corners = [(rng.uniform(-0.2, 1.2) * w * scale, rng.uniform(-0.2, 1.2) * h * scale) for _ in range(3)]
        draw.polygon(corners, fill=int(rng.uniform(95, 165)))
    planes = np.asarray(planes.resize((w, h), Image.LANCZOS), float) / 255 - 0.5
    # like a cut stone the planes stand apart clearly, a ruby flashes light and dark across its facets
    rgb = rgb * (1 + 0.6 * planes)[..., None]
    # where the planes meet, a thin bright line of light caught inside
    gy, gx = np.gradient(planes)
    rgb = mix(rgb, p(4, 1.45), np.clip(np.hypot(gx, gy) * 18, 0, 1) * 0.18)
    # the glow from inside, strongest in the middle
    core = smooth(0.75, 0.0, np.hypot(u - 0.5, (v - 0.5) * h / w) * 2)
    rgb = mix(rgb, p(4, 1.25), core * 0.3)
    # the edges: a bright rim where the glass is seen through its thickness, a dark line just inside it
    d = edge_distance(h, w)
    rgb = mix(rgb, p(4, 1.7), smooth(0.03, 0.0, d) * 0.85)
    rgb = rgb * (1 - 0.25 * smooth(0.012, 0.0, np.abs(d - 0.045)))[..., None]
    # one soft highlight across the face from the top left
    sweep = smooth(0.12, 0.0, np.abs((u * w + v * h) / FACE - 0.55)) * smooth(0.9, 0.3, u + v)
    rgb = mix(rgb, p(4, 1.85), sweep * 0.3)
    return rgb, 0.3 + 0.25 * core + 0.3 * smooth(0.03, 0.0, d)


def sandstone(rng, h, w, top):
    """World 2's fixed block: banded sandstone, beds running level round the sides, worn soft."""
    v, u = coords(h, w)
    if top:
        rgb = tone(rng, h, w, p(1, 0.88), p(2, 0.92), 4)
        crack = smooth(0.012, 0.0, np.abs(noise(rng, h, w, 3, 3) - 0.5)) * smooth(0.5, 0.7, noise(rng, h, w, 4))
        rgb = groove(rgb, crack, 0.4, 0.1)
    else:
        layer = v * 5 + (noise(rng, h, w, 3, 3) - 0.5) * 0.6
        bed = np.floor(layer)
        shade = (bed * 0.37 % 1.0) * 0.6 + noise(rng, h, w, 8, 3) * 0.4
        rgb = mix(p(0, 1.05), p(1, 1.08), shade)
        f = layer - bed
        rgb = rgb * (1 - 0.18 * smooth(0.1, 0.0, np.minimum(f, 1 - f)))[..., None]
    rgb = rgb * (0.93 + 0.14 * noise(rng, h, w, 70, 2))[..., None]
    return bevel(rgb, 0.05, 1.2, 0.6)


def rubble(rng, h, w, top):
    """World 2's destructible block: dry-stone walling, rough grey and brown stones laid in courses without mortar
    (on top as they lie, round), longer than high on the sides so they read as built, not as crazy paving."""
    if top:
        f1, f2, index = voronoi(rng, h, w, 14)
    else:
        # seeds in rows, the distances measured squashed so the stones come out long
        rows = 4
        points = []
        for r in range(rows):
            for k in range(4):
                points.append(((k + 0.5 + rng.uniform(-0.25, 0.25) + (r % 2) * 0.5) / 4 % 1.0,
                               (r + 0.5 + rng.uniform(-0.12, 0.12)) / rows))
        v, u = coords(h, w)
        f1 = np.full((h, w), 9.0)
        f2 = np.full((h, w), 9.0)
        index = np.zeros((h, w), int)
        for i, (pu, pv) in enumerate(points):
            for wrap in (-1, 0, 1):
                d = np.hypot((u - pu - wrap) * w * 0.45, (v - pv) * h) / FACE
                closer = d < f1
                f2 = np.where(closer, f1, np.minimum(f2, d))
                index = np.where(closer, i, index)
                f1 = np.where(closer, d, f1)
    shades = rng.random(index.max() + 1)
    warm = rng.random(index.max() + 1) < 0.4
    base = mix(p(3, 0.85), p(3, 1.2), shades[index])
    base = np.where(warm[index][..., None], mix(p(1, 0.8), p(2, 0.9), shades[index]), base)
    # each stone rounded: lighter towards its middle, the gaps deep
    rgb = base * (0.75 + 0.35 * smooth(0.0, 0.08, f2 - f1))[..., None]
    rgb = rgb * (0.9 + 0.2 * noise(rng, h, w, 50, 2))[..., None]
    rgb = groove(rgb, smooth(0.014, 0.004, f2 - f1), 0.8, 0.0)
    return bevel(rgb, 0.04, 1.15, 0.65)


def slate_stack(rng, h, w, top):
    """World 2's destructible block: split slate slabs stacked dry, thick and thin, their edges chipped; on top the
    last slab's face with its cleavage grain. No cells: the floor and the walkway round it are cobbled already."""
    v, u = coords(h, w)
    grain = noise(rng, h, w, 14, 3, stretch=(1.0, 7.0))
    if top:
        rgb = mix(p(3, 0.82), p(3, 1.12), 0.6 * grain + 0.4 * noise(rng, h, w, 4, 3))
        # the top slab a little smaller than the block, the one under it showing round its edge
        rgb = groove(rgb, box_frame(h, w, 0.07, 0.008), 0.6, 0.35)
    else:
        # slabs of their own thickness and shade, set a little in or out of each other
        edges = np.cumsum(rng.uniform(0.6, 1.6, 9))
        edges = edges / edges[-1]
        slab = np.searchsorted(edges, v)
        shade = rng.random(10)[slab]
        warm = (rng.random(10) < 0.3)[slab]
        rgb = mix(p(3, 0.72), p(3, 1.08), 0.55 * shade + 0.45 * grain)
        rgb = np.where(warm[..., None], mix(p(0, 0.9), p(1, 0.85), shade)[..., :] * 0.85 + rgb * 0.15, rgb)
        # the gaps, ragged where the slabs' edges have chipped
        ragged = (noise(rng, h, w, 20, 3) - 0.5) * 0.02
        gap = np.zeros((h, w))
        for e in edges[:-1]:
            gap = np.maximum(gap, smooth(0.012, 0.004, np.abs(v - e + ragged)))
        rgb = groove(rgb, gap, 0.75, 0.35)
        # each slab's lower edge in its own shadow, the upper one catching light
        local = (v - np.concatenate([[0.0], edges])[slab]) / np.diff(np.concatenate([[0.0], edges]))[slab]
        rgb = rgb * (1.08 - 0.2 * smooth(0.6, 1.0, local))[..., None]
    return bevel(rgb, 0.04, 1.15, 0.65)


def granite(rng, h, w, top):
    """World 3's fixed block: speckled blue grey granite, dark and wet below the waterline, weed along it."""
    rgb = tone(rng, h, w, p(2, 0.66), p(2, 0.86), 5)
    speck = rng.random((h, w))
    rgb = np.where((speck < 0.06)[..., None], rgb * 0.55, rgb)
    rgb = np.where((speck > 0.97)[..., None], np.minimum(rgb * 1.4, 1), rgb)
    rgb = groove(rgb, box_frame(h, w, 0.09, 0.01), 0.3, 0.25)
    if not top:
        v, u = coords(h, w)
        line = 0.62 + (noise(rng, h, w, 6, 2) - 0.5) * 0.08
        wet = smooth(line - 0.02, line + 0.02, v)
        rgb = rgb * (1 - 0.35 * wet)[..., None]
        weed = smooth(0.05, 0.0, np.abs(v - line)) * smooth(0.45, 0.6, noise(rng, h, w, 20, 3))
        rgb = mix(rgb, p(3, 0.75), weed * 0.8)
    return bevel(rgb, 0.04, 1.2, 0.62)


def mossy_crate(rng, h, w, top):
    """World 3's destructible block: a crate gone silver grey in the wet, moss creeping up from below."""
    rgb = planks(rng, h, w, 5, p(4, 0.48), p(4, 0.72), vertical=top)
    rgb = groove(rgb, box_frame(h, w, 0.1, 0.01), 0.45, 0.25)
    v, u = coords(h, w)
    reach = noise(rng, h, w, 7, 3)
    moss = smooth(0.4, 0.75, reach * 0.6 + (v if not top else 0.3) * 0.7) * smooth(0.3, 0.6, noise(rng, h, w, 30, 2))
    rgb = mix(rgb, mix(p(3, 0.75), p(3, 1.1), noise(rng, h, w, 40, 2)), moss * 0.85)
    rgb = dots(h, w, [(0.07, 0.08), (0.93, 0.08), (0.07, 0.92), (0.93, 0.92)], 0.02, rgb, p(0, 0.9))
    return bevel(rgb, 0.035, 1.15, 0.68)


def log_block(rng, h, w, top):
    """World 4's fixed block: a section of a great trunk, its rings on top, deep furrowed bark round its sides."""
    v, u = coords(h, w)
    if top:
        r = np.hypot((u - 0.48) * w, (v - 0.53) * h) / FACE
        warp = (noise(rng, h, w, 4, 3) - 0.5) * 0.06
        rings = np.sin((r + warp) * 2 * math.pi * 11) * 0.5 + 0.5
        rgb = mix(p(3, 1.3), p(3, 1.55), rings * 0.6 + noise(rng, h, w, 20, 2) * 0.4)
        rgb = mix(rgb, p(3, 1.05), smooth(0.04, 0.0, r) * 0.6)
        a = np.arctan2((v - 0.53) * h, (u - 0.48) * w)
        check = smooth(0.012, 0.0, np.abs(((a / (2 * math.pi) * 3 + 0.1) % 1.0) - 0.5) * r * 6) * smooth(0.1, 0.3, r)
        rgb = groove(rgb, check * 0.8, 0.6, 0.0)
        d = np.minimum(np.minimum(u, 1 - u) * w, np.minimum(v, 1 - v) * h) / FACE
        bark = smooth(0.07, 0.05, d)
        rgb = mix(rgb, tone(rng, h, w, p(3, 0.7), p(3, 1.05), 30, 2), bark)
    else:
        ridges = noise(rng, h, w, 9, 4, stretch=(9.0, 1.0))
        # furrowed bark, kept light enough that it reads in the game's light, not as a black box
        rgb = mix(p(3, 0.75), p(3, 1.3), smooth(0.35, 0.7, ridges))
        rgb = rgb * (0.85 + 0.3 * noise(rng, h, w, 40, 2, stretch=(4.0, 1.0)))[..., None]
        moss = smooth(0.6, 0.8, noise(rng, h, w, 6)) * smooth(0.3, 1.0, v)
        rgb = mix(rgb, p(1, 1.0), moss * 0.6)
    return bevel(rgb, 0.04, 1.15, 0.6)


def hedge(rng, h, w, top):
    """World 4's destructible block: a clipped box hedge, leaves lit on top, deeper green in its sides' hollows."""
    f1, f2, index = voronoi(rng, h, w, 120 if top else 100)
    leaf = smooth(0.03, 0.0, f1) * (0.6 + 0.4 * rng.random(index.max() + 1)[index])
    hollow = noise(rng, h, w, 10, 3)
    low, high = (p(1, 0.85), p(2, 1.0)) if top else (p(0, 1.0), p(1, 1.1))
    rgb = mix(low, high, np.clip(leaf * 0.7 + hollow * 0.5 - 0.1, 0, 1))
    rgb = rgb * (1 - 0.45 * smooth(0.45, 0.25, hollow))[..., None]
    return bevel(rgb, 0.06, 1.12, 0.72)


def basalt(rng, h, w, top):
    """World 5's fixed block: dark basalt in columns, matte and calm, the anchor between the floor and the glowing
    magma blocks; on top the columns' polygonal ends."""
    v, u = coords(h, w)
    if top:
        f1, f2, index = voronoi(rng, h, w, 7)
        joint = smooth(0.01, 0.003, f2 - f1)
    else:
        x = u * w / FACE + (noise(rng, h, w, 4, 2) - 0.5) * 0.04
        joint = lines(x, 0.24, 0.007)
    rgb = tone(rng, h, w, p(1, 0.78), p(1, 1.12), 6)
    rgb = rgb * (0.88 + 0.24 * noise(rng, h, w, 60, 2))[..., None]
    rgb = groove(rgb, joint, 0.55, 0.25)
    return bevel(rgb, 0.035, 1.35, 0.7)


def magma_block(rng, h, w, top):
    """World 5's destructible block: a lump of cooled magma, black crust split into a few large plates, the lava
    glowing in the cracks between them, hotter and wider towards the middle; it lights itself."""
    v, u = coords(h, w)
    f1, f2, index = voronoi(rng, h, w, 6 if top else 5)
    # the cracks wobble, wider where the lump is hottest (its middle)
    centre = smooth(0.75, 0.0, np.hypot(u - 0.5, (v - 0.5) * h / w) * 2)
    width = 0.016 + 0.026 * centre
    crack = smooth(width, width * 0.3, f2 - f1 + (noise(rng, h, w, 18, 2) - 0.5) * 0.01)
    crust = mix(p(0, 1.0), p(1, 0.75), 0.6 * noise(rng, h, w, 8, 4) + 0.4 * rng.random(index.max() + 1)[index])
    crust = crust * (0.85 + 0.3 * noise(rng, h, w, 50, 2))[..., None]
    # each plate's edge lit a little by the lava beside it
    rim = smooth(0.05, 0.0, f2 - f1) * (1 - crack)
    crust = mix(crust, p(2, 0.9), rim * 0.5)
    lava = mix(p(3, 0.95), p(4, 1.1), centre * 0.8 + 0.2 * noise(rng, h, w, 10, 2))
    rgb = mix(crust, lava, crack)
    rgb = bevel(rgb, 0.04, 1.2, 0.7)
    return rgb, crack * (0.55 + 0.4 * centre)


def terracotta(rng, h, w, top):
    """World 5's destructible block: small fired clay bricks, sooted from the lava."""
    v, u = coords(h, w)
    rows = 6 if not top else 5
    row = np.floor(v * rows)
    x = u * 3 + (row % 2) * 0.5
    joint = np.maximum(lines(v * h / FACE, h / FACE / rows, 0.008), lines(x * w / FACE / 3, w / FACE / 3, 0.006))
    brick = rng.random((rows + 1, 8))[row.astype(int), np.floor(x).astype(int) % 8]
    rgb = mix(p(2, 0.85), p(2, 1.2), brick * 0.6 + noise(rng, h, w, 30, 3) * 0.4)
    soot = smooth(0.5, 0.8, noise(rng, h, w, 5)) * (0.5 if top else smooth(0.0, 1.0, v))
    rgb = rgb * (1 - 0.5 * soot)[..., None]
    rgb = np.where(joint[..., None] > 0.5, np.array(p(1, 0.8)) * (1 - 0.3 * soot)[..., None], rgb)
    rgb = groove(rgb, joint, 0.3, 0.1)
    return bevel(rgb, 0.03, 1.15, 0.65)


def ashlar_gold(rng, h, w, top):
    """World 6's fixed block: a sandstone block like the hall's, its top inlaid with the banners' gold diamond."""
    rgb = tone(rng, h, w, p(1, 1.12), p(2, 0.95), 5)
    rgb = rgb * (0.92 + 0.16 * noise(rng, h, w, 70, 2))[..., None]
    rgb = groove(rgb, box_frame(h, w, 0.08, 0.012), 0.35, 0.3)
    if top:
        v, u = coords(h, w)
        d = np.abs(u - 0.5) * w / FACE + np.abs(v - 0.5) * h / FACE
        rgb = np.where((d < 0.2)[..., None], mix(p(3, 0.85), p(3, 1.1), noise(rng, h, w, 20, 2)), rgb)
        gold = smooth(0.02, 0.008, np.abs(d - 0.21)) + smooth(0.015, 0.005, np.abs(d - 0.07))
        rgb = mix(rgb, mix(p(4, 0.9), p(4, 1.25), noise(rng, h, w, 30, 2)), np.clip(gold, 0, 1))
    return bevel(rgb, 0.04, 1.2, 0.62)


def iron_chest(rng, h, w, top):
    """World 6's destructible block: an oak chest, upright boards held by two iron bands studded with rivets."""
    rgb = planks(rng, h, w, 5, p(0, 0.95), p(0, 1.45), vertical=True)
    v, u = coords(h, w)
    bands = (0.2, 0.8)
    for b in bands:
        on = np.abs(v - b) * h / FACE < 0.045
        rgb = np.where(on[..., None], tone(rng, h, w, p(1, 0.3), p(1, 0.48), 30, 2), rgb)
        rgb = groove(rgb, smooth(0.008, 0.002, np.abs(np.abs(v - b) * h / FACE - 0.045)), 0.5, 0.3)
        rgb = dots(h, w, [(x, b) for x in (0.12, 0.37, 0.63, 0.88)], 0.016, rgb, p(4, 0.7))
    return bevel(rgb, 0.035, 1.15, 0.65)


def steel_grille(rng, h, w, top):
    """World 7's fixed block: dark riveted steel, brushed; a grille of slots on top."""
    rgb = tone(rng, h, w, p(0, 1.0), p(1, 0.8), 4)
    rgb = rgb * (0.9 + 0.2 * noise(rng, h, w, 60, 2, stretch=(1.0, 12.0)))[..., None]
    rgb = groove(rgb, box_frame(h, w, 0.1, 0.012), 0.5, 0.35)
    v, u = coords(h, w)
    if top:
        inside = (np.abs(u - 0.5) * w / FACE < 0.26) & (np.abs(v - 0.5) * h / FACE < 0.26)
        slot = lines(v * h / FACE, 0.07, 0.016) * inside
        rgb = groove(rgb, slot, 0.85, 0.35)
    rust = smooth(0.65, 0.85, noise(rng, h, w, 8, 3)) * (0.5 if top else smooth(0.3, 1.0, v))
    rgb = mix(rgb, p(4, 0.6), rust * 0.6)
    k = 0.05
    studs = [(k * FACE / w, k * FACE / h), (1 - k * FACE / w, k * FACE / h), (k * FACE / w, 1 - k * FACE / h),
             (1 - k * FACE / w, 1 - k * FACE / h), (0.5, k * FACE / h), (0.5, 1 - k * FACE / h)]
    rgb = dots(h, w, studs, 0.017, rgb, p(2, 0.9))
    return bevel(rgb, 0.04, 1.3, 0.6)


def hazard_crate(rng, h, w, top):
    """World 7's destructible block: a steel crate painted safety yellow (grey it read like the fixed blocks), black
    and yellow stripes along its foot, the paint worn and rusting at the seams."""
    rgb = tone(rng, h, w, p(3, 0.8), p(3, 1.0), 5)
    worn = smooth(0.62, 0.8, noise(rng, h, w, 14, 3))
    rgb = mix(rgb, p(1, 1.0), worn * 0.7)
    v, u = coords(h, w)
    if top:
        d = np.minimum(np.abs(u * w - v * h), np.abs(u * w - (1 - v) * h)) / FACE
        rgb = groove(rgb, smooth(0.03, 0.022, d) * (1 - smooth(0.024, 0.03, d)), 0.4, 0.3)
    else:
        band = v > 0.68
        stripe = ((u * w + v * h) / FACE / 0.12) % 1.0 < 0.5
        rgb = np.where((band & stripe)[..., None], np.array(p(3, 1.05)) * (0.9 + 0.2 * noise(rng, h, w, 30, 2))[..., None], rgb)
        rgb = np.where((band & ~stripe)[..., None], np.array(p(0, 0.5)), rgb)
        rgb = groove(rgb, smooth(0.01, 0.003, np.abs(v - 0.68) * h / FACE), 0.5, 0.3)
    rust = smooth(0.6, 0.8, noise(rng, h, w, 9, 3)) * smooth(0.12, 0.0, edge_distance(h, w))
    rgb = mix(rgb, p(4, 0.65), rust * 0.8)
    rgb = groove(rgb, box_frame(h, w, 0.07, 0.01), 0.45, 0.3)
    return bevel(rgb, 0.035, 1.25, 0.62)


def alloy_seams(rng, h, w, top):
    """World 8's fixed block: near black alloy plates, the power running teal through the seams between them."""
    rgb = tone(rng, h, w, p(0, 1.0), p(1, 1.0), 4)
    rgb = rgb * (0.9 + 0.2 * noise(rng, h, w, 50, 2, stretch=(1.0, 8.0)))[..., None]
    seam = box_frame(h, w, 0.11, 0.01)
    if top:
        seam = np.maximum(seam, box_frame(h, w, 0.3, 0.008))
    else:
        v, u = coords(h, w)
        seam = np.maximum(seam, smooth(0.008, 0.002, np.abs(u - 0.5) * w / FACE) * (np.abs(v - 0.5) * h / FACE < 0.29))
    halo = np.clip(seam * 0.6 + smooth(0.04, 0.0, np.abs(seam - 0.5)) * 0.0, 0, 1)
    rgb = mix(rgb, p(2, 1.25), halo)
    rgb = mix(rgb, p(2, 1.7), np.clip(seam - 0.7, 0, 1) * 2)
    return bevel(rgb, 0.04, 1.4, 0.6)


def light_panel(rng, h, w, top):
    """World 8's destructible block: a light grey machine panel like the border's, a blue lamp and vents in it."""
    rgb = tone(rng, h, w, p(3, 0.88), p(3, 1.06), 4)
    rgb = rgb * (0.93 + 0.14 * noise(rng, h, w, 60, 2, stretch=(1.0, 10.0)))[..., None]
    rgb = groove(rgb, box_frame(h, w, 0.08, 0.012), 0.45, 0.35)
    v, u = coords(h, w)
    lamp = (np.abs(u - 0.5) * w / FACE < 0.07) & (np.abs(v - (0.5 if top else 0.4)) * h / FACE < 0.07)
    rgb = np.where(lamp[..., None], mix(p(2, 1.3), p(2, 1.75), smooth(0.07, 0.0, np.hypot(u - 0.5, v - 0.4))), rgb)
    rgb = groove(rgb, smooth(0.01, 0.003, np.abs(np.maximum(np.abs(u - 0.5) * w, np.abs(v - (0.5 if top else 0.4)) * h) / FACE - 0.075)), 0.5, 0.3)
    if not top:
        vents = (np.abs(u - 0.5) * w / FACE < 0.22) & (v > 0.66) & (v < 0.86)
        rgb = groove(rgb, lines(v * h / FACE, 0.035, 0.007) * vents, 0.7, 0.3)
    return bevel(rgb, 0.035, 1.3, 0.6)


# per world: the fixed blocks' skin, the destructible ones'
SKINS = {
    1: (limestone, timber_crate),
    2: (dressed_sandstone, glass_block),
    3: (granite, mossy_crate),
    4: (log_block, hedge),
    5: (basalt, magma_block),
    6: (ashlar_gold, iron_chest),
    7: (steel_grille, hazard_crate),
    8: (alloy_seams, light_panel),
}


# worlds whose destructible blocks are glass (the game's block shader draws them, see BlockMaterial::setGlass): the
# palette colours the blocks take, one each, picked by where a block stands
# world 2: ruby, amber, dust (slate went colourless on the grey cobbles); world 8: teal, mostly, and the alarm red
GLASS = {2: (4, 1, 2), 8: (2, 2, 4)}


def write_glass(world):
    """glass= in the world's world.ini, the tints as #rrggbb from its palette; any other settings stay."""
    path = os.path.join(STORY, f"world{world}", "world.ini")
    lines = open(path, encoding="utf-8").read().splitlines() if os.path.exists(path) else ["[world]"]
    lines = [line for line in lines if not line.startswith("glass=") and "build_blocks.py writes" not in line]
    if world in GLASS:
        tints = ",".join(palettes.PALETTES[world][1][i] for i in GLASS[world])
        at = lines.index("[world]") + 1
        lines[at:at] = ["# the destructible blocks are glass in these colours (assets/story-worlds/build_blocks.py writes this)",
                        f"glass={tints}"]
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines) + "\n")


def unwrap(painter, seed):
    """The cross texture: every face painted by painter (rng, height, width, top) and scaled into its cell. A painter
    returns the colour, or the colour and how much of it glows (0..1), which goes out as alpha 1 - glow: the game's
    block shader keeps glowing texels' colour whatever the light (blockmaterial-frag.glsl)."""
    rng = np.random.default_rng(seed)
    image = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 255))
    for (x0, y0, x1, y1), kind in CELLS:
        h, w = (FACE, FACE) if kind != "side" else (SIDE, FACE)
        painted = painter(rng, h, w, kind != "side")
        rgb, glow = painted if isinstance(painted, tuple) else (painted, np.zeros((h, w)))
        rgba = np.concatenate([np.clip(rgb, 0, 1), 1 - np.clip(glow, 0, 1)[..., None]], axis=2)
        face = Image.fromarray((rgba * 255).astype(np.uint8), "RGBA")
        image.paste(face.resize((x1 - x0, y1 - y0), Image.LANCZOS), (x0, y0))
    return image


def build(world):
    WORLD[0] = world
    if world in GLASS or os.path.exists(os.path.join(STORY, f"world{world}", "world.ini")):
        write_glass(world)
    fixed, destructible = SKINS[world]
    folder = os.path.join(STORY, f"world{world}")
    for name, painter, seed in (("block-unwrap", fixed, 100 + world), ("stone-unwrap", destructible, 200 + world)):
        path = os.path.join(folder, f"{name}.tga")
        unwrap(painter, seed).save(path, compression="tga_rle")
        print("RESULT", os.path.normpath(path), os.path.getsize(path) // 1024, "kb")


if __name__ == "__main__":
    for world in [int(a) for a in sys.argv[1:]] or SKINS:
        build(world)
