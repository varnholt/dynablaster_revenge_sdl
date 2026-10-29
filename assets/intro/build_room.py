# The ending's set after the original's: the castle's machine room seen from the side. A green brick wall with a
# control panel (gauges, buttons, screens, blinking lights), pipes running along it and down with flanges and a
# valve, two arched windows onto the night sky, two white plates hanging on cables, caged lamps, a riveted beam
# under the ceiling, two ribbed steel pillars in front; a walkway of worn slabs along it, its front built of rounded
# purple stone blocks; a doorway at the right end where the princess comes in; a crate, a barrel and cables about.
#   blender --background --factory-startup --python build_room.py -- <out dir>
# writes room.glb (and room_mask.npy for room_detail.py: brushed steel on the metal). The wall, the floor and the
# front have their own painted textures (bricks with their mortar, grime, damp and moss; slabs; blocks), the props
# their palette. Faces -y, z up, the walkway's floor at z 0 from x -7.5 to 7.5.
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bmesh  # noqa: E402
import bpy  # noqa: E402
import numpy as np  # noqa: E402
from mathutils import Vector  # noqa: E402

import toy  # noqa: E402
from toy import cone, sphere, trs, tube  # noqa: E402

COLORS = {
    "panel": (0.78, 0.79, 0.82),
    "panel_dark": (0.52, 0.53, 0.58),
    "panel_edge": (0.90, 0.91, 0.93),
    "rust": (0.48, 0.34, 0.26),
    "gauge": (0.66, 0.64, 0.78),
    "needle": (0.30, 0.15, 0.36),
    "knob": (0.12, 0.14, 0.36),
    "screen": (0.08, 0.10, 0.18),
    "glyph": (0.92, 0.93, 0.96),
    "led_red": (1.0, 0.25, 0.2),
    "led_green": (0.3, 1.0, 0.45),
    "led_amber": (1.0, 0.75, 0.2),
    "pipe": (0.86, 0.87, 0.89),
    "steel": (0.70, 0.71, 0.75),
    "steel_dark": (0.45, 0.46, 0.52),
    "frame": (0.84, 0.85, 0.88),
    "plate": (0.93, 0.94, 0.96),
    "lamp": (1.0, 0.92, 0.7),
    "wood": (0.50, 0.36, 0.22),
    "wood_dark": (0.36, 0.25, 0.15),
    "cable": (0.16, 0.16, 0.20),
    "valve": (0.75, 0.18, 0.15),
    "night": (0.03, 0.04, 0.10),
}

WALL_Y = 1.6  # the wall's face
FRONT_Y = -0.7  # the walkway's front edge
X0, X1 = -7.5, 7.5
TOP = 6.6
BOTTOM = -2.6  # the foot of the purple front
WINDOWS = [(-3.25, 2.75), (2.3, 2.75)]  # bottom centre of each arched opening
WINDOW_WIDTH = 1.3
WINDOW_HEIGHT = 1.0  # up to where the arch begins
PILLARS = [-0.55, 4.6]
PIPE_DOWN = [-1.45, -1.8]  # where the two pipes turn down to the floor
LAMPS = [-5.0, 0.95, 3.4]

GLOWS = []  # (position, [r, g, b, size, mode]) for the game's glow sprites


# --- painted textures ----------------------------------------------------------------------------------

def noise(rng, shape, cells):
    """Smooth value noise 0..1 over shape (rows, cols), about cells blobs across."""
    h, w = shape
    gh, gw = max(2, int(cells * h / w) + 2), int(cells) + 2
    grid = rng.random((gh, gw))
    y = np.linspace(0, gh - 1.001, h)[:, None]
    x = np.linspace(0, gw - 1.001, w)[None, :]
    y0, x0 = y.astype(int), x.astype(int)
    fy, fx = y - y0, x - x0
    fy, fx = fy * fy * (3 - 2 * fy), fx * fx * (3 - 2 * fx)
    a = grid[y0, x0] * (1 - fx) + grid[y0, x0 + 1] * fx
    b = grid[y0 + 1, x0] * (1 - fx) + grid[y0 + 1, x0 + 1] * fx
    return a * (1 - fy) + b * fy


def fbm(rng, shape, cells, octaves=4):
    total, amp, norm = np.zeros(shape), 1.0, 0.0
    for o in range(octaves):
        total += noise(rng, shape, cells * 2 ** o) * amp
        norm += amp
        amp *= 0.5
    return total / norm


def courses(x, z, height, length, mortar, rng, shift=0.5):
    """A bond of blocks over the points x, z: each block's id, the distance to its nearest edge (units) and how far
    up it it is (0 bottom .. 1 top)."""
    row = np.floor(z / height).astype(int)
    offset = np.where(row % 2 == 1, shift * length, 0.0)
    col = np.floor((x + offset) / length).astype(int)
    u = (x + offset) / length - col
    v = z / height - row
    edge = np.minimum(np.minimum(u, 1 - u) * length, np.minimum(v, 1 - v) * height)
    ids = (row * 7919 + col * 104729) % 100003
    return ids, edge, v


def shade_per(ids, rng, spread):
    """A random -spread..spread per block id."""
    table = rng.uniform(-spread, spread, 100003)
    return table[ids]


def wall_texture(size=(3072, 1344)):
    """The green brick wall: three greens with a shade of their own per brick, sunk mortar, the bricks' top edges
    catching the light and their bottoms in shadow, streaks of grime down from the windows' sills and the pipes,
    damp patches, moss creeping up from the floor, a few cracked and broken bricks."""
    w, h = size
    rng = np.random.default_rng(41)
    x = X0 + (np.arange(w) + 0.5) / w * (X1 - X0)
    z = (np.arange(h) + 0.5) / h * TOP
    x, z = np.meshgrid(x, z)
    ids, edge, up = courses(x, z, 0.165, 0.38, 0.018, rng)
    greens = np.array([(0.31, 0.45, 0.37), (0.37, 0.52, 0.42), (0.27, 0.39, 0.33)])
    color = greens[ids % 3] * (1.0 + shade_per(ids, rng, 0.08))[..., None]
    grain = fbm(rng, (h, w), 160, 3)
    color *= (0.9 + 0.2 * grain)[..., None]
    # the bricks' bevels: light from above
    bevel = np.clip(edge / 0.03, 0, 1)
    color *= (1.0 + 0.12 * (1 - bevel) * np.where(up > 0.5, 1.0, -1.6))[..., None]
    mortar = (0.20, 0.25, 0.23)
    joint = np.clip((edge - 0.006) / 0.01, 0, 1)
    color = color * joint[..., None] + np.array(mortar) * (1 - joint)[..., None]
    # broken bricks: some recessed and dark, some cracked across
    broken = shade_per(ids, rng, 1.0) > 0.965
    color = np.where(broken[..., None], color * 0.55, color)
    crack_id = shade_per(ids, rng, 1.0) < -0.97
    crack = np.abs(np.sin((x * 9 + z * 23) + fbm(rng, (h, w), 40, 2) * 6)) < 0.08
    color = np.where((crack_id & crack)[..., None], color * 0.5, color)
    # grime down from the sills and the pipes' turns
    streaks = fbm(rng, (h, w), 60, 2)
    stretched = noise(rng, (h, w // 8), 200)
    stretched = np.repeat(stretched, 8, axis=1)[:, :w]
    grime = np.zeros((h, w))
    for cx, bottom in WINDOWS:
        under = np.clip(1 - np.abs(x - cx) / (WINDOW_WIDTH * 0.6), 0, 1) * np.clip((bottom - z) / 1.8, 0, 1)
        grime = np.maximum(grime, under * (z < bottom) * np.clip(1 - (bottom - z) / 2.2, 0, 1))
    for px in PIPE_DOWN:
        grime = np.maximum(grime, np.clip(1 - np.abs(x - px) / 0.35, 0, 1) * 0.6)
    grime = grime * (0.5 + stretched)
    color *= (1 - 0.35 * np.clip(grime, 0, 1))[..., None]
    # damp patches and soot at the top
    damp = np.clip((fbm(rng, (h, w), 6, 4) - 0.55) * 4, 0, 1)
    color *= (1 - 0.18 * damp)[..., None]
    color *= (1 - 0.3 * np.clip((z - TOP + 1.2) / 1.2, 0, 1))[..., None]
    # moss from the floor up, in the joints first
    reach = 0.25 + 0.6 * fbm(rng, (h, w), 12, 3)
    moss = np.clip((reach - z) / 0.25, 0, 1) * (0.6 + 0.4 * (1 - joint)) * (streaks > 0.35)
    color = color * (1 - moss[..., None] * 0.8) + np.array((0.30, 0.50, 0.22)) * (moss[..., None] * 0.8)
    return color


def floor_texture(size=(2048, 320)):
    """The walkway: big pale slabs, worn paler where people walk, cracks and stains, a dark strip along the wall."""
    w, h = size
    rng = np.random.default_rng(43)
    x = X0 + (np.arange(w) + 0.5) / w * (X1 - X0)
    y = FRONT_Y + (np.arange(h) + 0.5) / h * (WALL_Y - FRONT_Y)
    x, y = np.meshgrid(x, y)
    ids, edge, _ = courses(x, y - FRONT_Y, (WALL_Y - FRONT_Y) / 2, 1.1, 0.02, rng, shift=0.35)
    color = np.array((0.78, 0.79, 0.82)) * (1.0 + shade_per(ids, rng, 0.05))[..., None]
    color *= (0.88 + 0.2 * fbm(rng, (h, w), 80, 4))[..., None]
    joint = np.clip((edge - 0.008) / 0.012, 0, 1)
    color = color * joint[..., None] + np.array((0.42, 0.42, 0.46)) * (1 - joint)[..., None]
    wear = np.clip(1 - np.abs(y - 0.3) / 0.7, 0, 1) * 0.08
    color *= (1 + wear)[..., None]
    stains = np.clip((fbm(rng, (h, w), 10, 3) - 0.6) * 5, 0, 1)
    color *= (1 - 0.2 * stains)[..., None]
    crack = (np.abs(fbm(rng, (h, w), 30, 3) - 0.5) < 0.008) & (shade_per(ids, rng, 1) > 0.4)
    color = np.where(crack[..., None], color * 0.55, color)
    color *= (1 - 0.35 * np.clip((y - WALL_Y + 0.4) / 0.4, 0, 1))[..., None]
    return color


def front_texture(size=(2048, 384)):
    """The walkway's front: rows of rounded purple stone blocks, the top row paler, darker and mossy in the joints,
    deeper rows darker as they go down into the dark."""
    w, h = size
    rng = np.random.default_rng(47)
    x = X0 + (np.arange(w) + 0.5) / w * (X1 - X0)
    z = BOTTOM + (np.arange(h) + 0.5) / h * (0.0 - BOTTOM)
    x, z = np.meshgrid(x, z)
    ids, edge, up = courses(x, z - BOTTOM, 0.43, 0.82, 0.03, rng)
    row_from_top = np.floor((0.0 - z) / 0.43)
    base = np.where(row_from_top[..., None] < 1, np.array((0.60, 0.58, 0.74)), np.array((0.42, 0.39, 0.60)))
    color = base * (1.0 + shade_per(ids, rng, 0.07))[..., None]
    color *= (0.88 + 0.22 * fbm(rng, (h, w), 70, 4))[..., None]
    # rounded: the edges fall away into shadow, the tops catch light
    round_ = np.clip(edge / 0.09, 0, 1) ** 0.6
    color *= (0.55 + 0.45 * round_)[..., None]
    color *= (1 + 0.1 * (1 - np.clip(edge / 0.05, 0, 1)) * (up > 0.5))[..., None]
    joint = np.clip((edge - 0.01) / 0.012, 0, 1)
    color = color * joint[..., None] + np.array((0.12, 0.10, 0.18)) * (1 - joint)[..., None]
    moss = (1 - joint) * (fbm(rng, (h, w), 20, 3) > 0.55) * (row_from_top < 2)
    color = color * (1 - moss[..., None] * 0.7) + np.array((0.32, 0.48, 0.28)) * (moss[..., None] * 0.7)
    color *= (1 - 0.45 * np.clip(row_from_top / 5, 0, 1))[..., None]
    return color


def image(name, color):
    """color: rows from the bottom up, rgb 0..1 srgb."""
    h, w = color.shape[:2]
    rgba = np.concatenate([np.clip(color, 0, 1), np.ones((h, w, 1))], axis=-1).astype(np.float32)
    img = bpy.data.images.new(name, w, h)
    img.pixels.foreach_set(rgba.ravel())
    img.file_format = "PNG"
    img.pack()
    return img


def textured(name, bm, uv_of, color):
    """An object of its own for bm, its uvs uv_of(co), coloured by a texture painted from color."""
    uvs = bm.loops.layers.uv.new("UVMap")
    for f in bm.faces:
        for loop in f.loops:
            loop[uvs].uv = uv_of(loop.vert.co)
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    material = bpy.data.materials.new(name)
    material.use_nodes = True
    nodes, links = material.node_tree.nodes, material.node_tree.links
    bsdf = next(n for n in nodes if n.type == "BSDF_PRINCIPLED")
    texture = nodes.new("ShaderNodeTexImage")
    texture.image = image(name + "_color", color)
    links.new(texture.outputs["Color"], bsdf.inputs["Base Color"])
    mesh.materials.append(material)
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return obj


# --- the shapes --------------------------------------------------------------------------------------------

def box(lo, hi):
    """A box from corner lo to corner hi."""
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    bm.transform(trs(tuple((a + b) / 2 for a, b in zip(lo, hi)), (0, 0, 0), tuple(b - a for a, b in zip(lo, hi))))
    return bm


def bevelled(lo, hi, width):
    """A box with its edges bevelled, catching the light like pressed metal."""
    bm = box(lo, hi)
    bmesh.ops.bevel(bm, geom=list(bm.edges), offset=width, segments=2, affect="EDGES", profile=0.5)
    return bm


def facing(radius, depth, segments=32):
    """A disc depth thick facing -y, its back on the origin."""
    return cone(radius, radius, depth, segments), (-90, 0, 0)


def rivet(b, at, size=0.03, color="panel_dark"):
    b.add(sphere(1.0, 8, 6), color, matrix=trs(at, (0, 0, 0), (size, size * 0.6, size)))


def in_window(x, z):
    for cx, bottom in WINDOWS:
        half = WINDOW_WIDTH / 2
        if abs(x - cx) < half and bottom < z < bottom + WINDOW_HEIGHT:
            return True
        if math.hypot(x - cx, z - bottom - WINDOW_HEIGHT) < half:
            return True
    return False


def wall():
    """The wall as a fine grid with the windows left open, so the night sky shows through them."""
    step = 0.05
    bm = bmesh.new()
    nx, nz = int((X1 - X0) / step), int(TOP / step)
    grid = {}

    def vert(i, k):
        if (i, k) not in grid:
            grid[(i, k)] = bm.verts.new((X0 + i * step, WALL_Y, k * step))
        return grid[(i, k)]
    for i in range(nx):
        for k in range(nz):
            if not in_window(X0 + (i + 0.5) * step, (k + 0.5) * step):
                bm.faces.new((vert(i, k), vert(i + 1, k), vert(i + 1, k + 1), vert(i, k + 1)))
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-5)
    bmesh.ops.dissolve_limit(bm, angle_limit=0.01, verts=bm.verts, edges=bm.edges)
    bmesh.ops.triangulate(bm, faces=bm.faces)
    textured("room_wall", bm, lambda co: ((co.x - X0) / (X1 - X0), co.z / TOP), wall_texture())


def walkway():
    """The floor's slabs and the front's blocks, each a painted face; the rest of the walkway's body plain."""
    bm = bmesh.new()
    corners = [(X0, FRONT_Y, 0.0), (X1, FRONT_Y, 0.0), (X1, WALL_Y, 0.0), (X0, WALL_Y, 0.0)]
    bm.faces.new([bm.verts.new(c) for c in corners])
    textured("room_floor", bm, lambda co: ((co.x - X0) / (X1 - X0), (co.y - FRONT_Y) / (WALL_Y - FRONT_Y)),
             floor_texture())
    bm = bmesh.new()
    corners = [(X0, FRONT_Y, BOTTOM), (X1, FRONT_Y, BOTTOM), (X1, FRONT_Y, 0.0), (X0, FRONT_Y, 0.0)]
    bm.faces.new([bm.verts.new(c) for c in corners])
    textured("room_front", bm, lambda co: ((co.x - X0) / (X1 - X0), (co.z - BOTTOM) / (0.0 - BOTTOM)),
             front_texture())


def windows(b):
    """Frames round the arched openings with a keystone, bars across, a sill below; the night beyond."""
    for cx, bottom in WINDOWS:
        half = WINDOW_WIDTH / 2
        top = bottom + WINDOW_HEIGHT
        arch = [(cx + half * math.cos(a), WALL_Y - 0.05, top + half * math.sin(a))
                for a in np.linspace(0, math.pi, 17)]
        outline = [(cx + half, WALL_Y - 0.05, bottom)] + arch + [(cx - half, WALL_Y - 0.05, bottom)]
        b.add(tube([Vector(p) for p in outline], [0.08] * len(outline), 10), "frame")
        b.add(bevelled((cx - 0.12, WALL_Y - 0.14, top + half - 0.08), (cx + 0.12, WALL_Y, top + half + 0.22), 0.02),
              "frame")
        b.add(bevelled((cx - half - 0.16, WALL_Y - 0.24, bottom - 0.14), (cx + half + 0.16, WALL_Y, bottom), 0.025),
              "frame")
        for bx in (cx - half / 3, cx + half / 3):
            b.add(tube([Vector((bx, WALL_Y + 0.05, bottom)), Vector((bx, WALL_Y + 0.05, top + half * 0.9))],
                       [0.018, 0.018], 6), "steel_dark")
        b.add(tube([Vector((cx - half, WALL_Y + 0.05, top)), Vector((cx + half, WALL_Y + 0.05, top))],
                   [0.018, 0.018], 6), "steel_dark")
        # the opening's depth: the wall's thickness, dark inside
        b.add(box((cx - half, WALL_Y, bottom - 0.02), (cx + half, WALL_Y + 0.35, bottom)), "night")


def panel(b):
    """The control panel at the left: an upper part with gauges, buttons and blinking lights, a wider lower one
    with three glowing screens and two slots; pressed edges, rivets, a little rust."""
    y0, y1 = WALL_Y - 0.14, WALL_Y
    b.add(bevelled((-6.75, y0, 2.2), (-4.35, y1, 3.75), 0.04), "panel")
    b.add(bevelled((-6.75, y0, 0.6), (-2.05, y1, 2.2), 0.04), "panel")
    b.add(bevelled((-6.85, y0 + 0.02, 0.45), (-1.95, y1, 0.62), 0.03), "panel_dark")
    face = y0 - 0.001
    for x, z in ((-6.65, 3.65), (-4.45, 3.65), (-6.65, 2.3), (-6.65, 2.1), (-2.15, 2.1), (-6.65, 0.7), (-2.15, 0.7)):
        rivet(b, (x, face, z))
    for z in (3.15, 2.5):
        disc, turned = facing(0.36, 0.04)
        b.add(disc, "gauge", matrix=trs((-5.8, face, z), turned))
        rim, turned = facing(0.41, 0.03)
        b.add(rim, "panel_edge", matrix=trs((-5.8, face + 0.01, z), turned))
        for k in range(9):
            a = math.radians(-120 + k * 30)
            b.add(box((-5.8 + 0.28 * math.sin(a) - 0.008, face - 0.05, z + 0.28 * math.cos(a) - 0.008),
                      (-5.8 + 0.28 * math.sin(a) + 0.008, face - 0.04, z + 0.28 * math.cos(a) + 0.008)), "needle")
        b.add(box((-5.82, face - 0.06, z - 0.015), (-5.52, face - 0.045, z + 0.015)), "needle")
        b.add(sphere(1.0, 10, 6), "needle", matrix=trs((-5.8, face - 0.05, z), (0, 0, 0), 0.03))
        for k in range(3):
            b.add(bevelled((-5.1 + k * 0.3, face - 0.06, z + 0.08), (-4.9 + k * 0.3, face, z + 0.26), 0.01), "glyph")
        b.add(box((-5.1, face - 0.04, z - 0.16), (-4.5, face, z - 0.11)), "needle")
    for z in (3.5, 3.25):
        knob, turned = facing(0.08, 0.07, 14)
        b.add(knob, "knob", matrix=trs((-6.5, face, z), turned))
    # blinking lights along the upper part's top
    for k, color in enumerate(("led_red", "led_green", "led_amber", "led_green", "led_red")):
        at = (-6.45 + k * 0.4, face - 0.03, 3.62)
        b.add(sphere(1.0, 10, 6), color, matrix=trs(at, (0, 0, 0), 0.035))
        rgb = COLORS[color]
        GLOWS.append((Vector(at) - Vector((0, 0.03, 0)), [rgb[0], rgb[1], rgb[2], 0.18, 2]))
    for k in range(3):
        x = -6.4 + k * 0.72
        b.add(bevelled((x - 0.05, face - 0.02, 0.75), (x + 0.65, face, 1.5), 0.015), "panel_edge")
        b.add(box((x, face - 0.04, 0.8), (x + 0.6, face - 0.01, 1.45)), "screen")
        # readouts: a bar graph, a trace, lines of text (three bars and a stroke read as a letter E)
        if k == 0:
            for i, h in enumerate((0.18, 0.34, 0.26, 0.46, 0.3)):
                b.add(box((x + 0.07 + i * 0.1, face - 0.06, 0.9), (x + 0.13 + i * 0.1, face - 0.04, 0.9 + h)), "glyph")
        elif k == 1:
            for i in range(14):
                z = 1.12 + 0.17 * math.sin(i * 0.9) * math.exp(-i * 0.06)
                b.add(box((x + 0.05 + i * 0.036, face - 0.06, z - 0.018), (x + 0.085 + i * 0.036, face - 0.04, z + 0.018)),
                      "glyph")
        else:
            for i, w in enumerate((0.42, 0.3, 0.46, 0.22, 0.36)):
                z = 1.33 - i * 0.1
                b.add(box((x + 0.07, face - 0.06, z - 0.02), (x + 0.07 + w, face - 0.04, z + 0.02)), "glyph")
        GLOWS.append((Vector((x + 0.3, face - 0.08, 1.12)), [0.55, 0.7, 1.0, 0.35, 3]))
    for z in (1.55, 1.05):
        b.add(bevelled((-4.05, face - 0.04, z), (-2.55, face, z + 0.18), 0.015), "panel_dark")
        b.add(box((-3.95, face - 0.06, z + 0.06), (-2.65, face - 0.04, z + 0.12)), "screen")
    # rust spots and a dent near the edges
    for x, z, sx, sz in ((-2.3, 0.75, 0.12, 0.07), (-6.6, 1.0, 0.08, 0.14), (-4.6, 2.35, 0.1, 0.06)):
        b.add(sphere(1.0, 10, 6), "rust", matrix=trs((x, face + 0.005, z), (0, 0, 0), (sx, 0.02, sz)))


def pipes(b):
    """Two pipes along the wall's top, down to the floor beside the panel: flanges at their joints, brackets
    holding them to the wall, a red valve wheel."""
    for z, x in ((5.15, PIPE_DOWN[0]), (4.85, PIPE_DOWN[1])):
        y = WALL_Y - 0.14
        path = [Vector((X0, y, z)), Vector((x - 0.2, y, z)), Vector((x, y, z - 0.2)), Vector((x, y, 0.05))]
        b.add(tube(path, [0.065] * len(path), 12), "pipe")
        for px in np.arange(X0 + 0.9, x - 0.3, 1.5):
            b.add(cone(0.09, 0.09, 0.05, 16), "steel", matrix=trs((px, y, z), (0, 90, 0)))
            b.add(bevelled((px + 0.32, y + 0.02, z - 0.1), (px + 0.4, WALL_Y, z + 0.1), 0.01), "steel_dark")
        for pz in np.arange(0.8, z - 0.4, 1.3):
            b.add(cone(0.09, 0.09, 0.05, 16), "steel", matrix=trs((x, y, pz)))
            b.add(bevelled((x - 0.1, y + 0.02, pz + 0.3), (x + 0.1, WALL_Y, pz + 0.38), 0.01), "steel_dark")
    b.add(bevelled((-2.05, WALL_Y - 0.28, 0.0), (-1.2, WALL_Y, 0.2), 0.02), "steel_dark")
    # the valve on the near pipe
    vx, vz, vy = PIPE_DOWN[0], 1.7, WALL_Y - 0.3
    b.add(tube([Vector((vx, WALL_Y - 0.14, vz)), Vector((vx, vy, vz))], [0.025, 0.025], 8), "steel_dark")
    ring = [Vector((vx + 0.14 * math.cos(a), vy, vz + 0.14 * math.sin(a))) for a in np.linspace(0, 2 * math.pi, 17)]
    b.add(tube(ring, [0.018] * len(ring), 6, cap=False), "valve")
    for a in (0, 60, 120):
        r = math.radians(a)
        b.add(tube([Vector((vx - 0.14 * math.cos(r), vy, vz - 0.14 * math.sin(r))),
                    Vector((vx + 0.14 * math.cos(r), vy, vz + 0.14 * math.sin(r)))], [0.012, 0.012], 6), "valve")


def pillars(b):
    """Steel pillars with ribs up their length, riveted bands, a stepped base and a capital under the beam."""
    for x in PILLARS:
        centre = Vector((x, WALL_Y - 0.75, 0.0))
        b.add(cone(0.6, 0.6, TOP, 40), "steel", matrix=trs(centre))
        for k in range(16):
            a = 2 * math.pi * k / 16
            at = centre + Vector((0.6 * math.cos(a), 0.6 * math.sin(a), 0))
            b.add(box((at.x - 0.025, at.y - 0.025, 0.35), (at.x + 0.025, at.y + 0.025, TOP - 0.5)), "steel_dark")
        for z0 in (0.0, 0.18):
            b.add(cone(0.76 - z0 * 0.4, 0.76 - z0 * 0.4, 0.18, 40), "steel_dark", matrix=trs(centre + Vector((0, 0, z0))))
        b.add(cone(0.66, 0.78, 0.25, 40), "steel_dark", matrix=trs(centre + Vector((0, 0, TOP - 0.55))))
        for band in (1.4, 3.6):
            b.add(cone(0.65, 0.65, 0.12, 40), "steel_dark", matrix=trs(centre + Vector((0, 0, band))))
            for k in range(10):
                a = math.pi + math.pi * (k + 0.5) / 10
                rivet(b, centre + Vector((0.66 * math.cos(a), 0.66 * math.sin(a), band + 0.06)), 0.022, "steel")


def plates(b):
    """Two white plates on the wall, a cable hanging from each with a weight at its end."""
    for x in (1.4, 2.65):
        b.add(bevelled((x - 0.4, WALL_Y - 0.08, 1.3), (x + 0.4, WALL_Y, 2.55), 0.025), "plate")
        b.add(box((x - 0.02, WALL_Y - 0.09, 1.45), (x + 0.02, WALL_Y - 0.08, 2.4)), "panel_edge")
        for bx in (x - 0.3, x + 0.3):
            for bz in (1.4, 2.45):
                rivet(b, (bx, WALL_Y - 0.09, bz), 0.03)
        b.add(tube([Vector((x, WALL_Y - 0.1, 1.3)), Vector((x, WALL_Y - 0.1, 0.32))], [0.015, 0.015], 6), "cable")
        b.add(cone(0.06, 0.0, 0.12, 8), "knob", matrix=trs((x, WALL_Y - 0.1, 0.33), (180, 0, 0)))


def lamps(b):
    """Caged lamps on the wall, lighting the room warm."""
    for x in LAMPS:
        z = 4.1
        b.add(bevelled((x - 0.12, WALL_Y - 0.06, z - 0.2), (x + 0.12, WALL_Y, z + 0.2), 0.02), "steel_dark")
        b.add(tube([Vector((x, WALL_Y - 0.05, z)), Vector((x, WALL_Y - 0.3, z))], [0.03, 0.03], 8), "steel_dark")
        b.add(sphere(1.0, 16, 10), "lamp", matrix=trs((x, WALL_Y - 0.38, z - 0.05), (0, 0, 0), 0.11))
        for k in range(6):
            a = 2 * math.pi * k / 6
            arc = [Vector((x + 0.14 * math.cos(a) * math.sin(t), WALL_Y - 0.38 + 0.14 * math.sin(a) * math.sin(t),
                           z - 0.05 + 0.14 * math.cos(t))) for t in np.linspace(0.2, math.pi - 0.2, 7)]
            b.add(tube(arc, [0.008] * len(arc), 4, cap=False), "steel_dark")
        b.add(cone(0.16, 0.06, 0.06, 16), "steel_dark", matrix=trs((x, WALL_Y - 0.38, z + 0.08)))
        GLOWS.append((Vector((x, WALL_Y - 0.5, z - 0.05)), [1.0, 0.8, 0.5, 1.4, 0]))


def beam(b):
    """A riveted steel beam along the top of the wall."""
    b.add(bevelled((X0, WALL_Y - 0.5, TOP - 0.4), (X1, WALL_Y, TOP), 0.03), "steel_dark")
    b.add(box((X0, WALL_Y - 0.55, TOP - 0.45), (X1, WALL_Y - 0.45, TOP - 0.38)), "steel")
    for x in np.arange(X0 + 0.25, X1, 0.5):
        rivet(b, (x, WALL_Y - 0.51, TOP - 0.2), 0.03, "steel")


def doorway(b):
    """The princess's way in at the right end: a tall frame in the wall, the dark corridor beyond."""
    b.add(bevelled((6.15, WALL_Y - 0.16, 0.0), (6.35, WALL_Y, 6.0), 0.03), "frame")
    b.add(box((6.35, WALL_Y - 0.02, 0.0), (X1, WALL_Y, 6.0)), "night")
    b.add(bevelled((6.15, WALL_Y - 0.16, 5.8), (X1, WALL_Y, 6.05), 0.03), "frame")
    for z in (1.0, 3.0, 5.0):
        rivet(b, (6.25, WALL_Y - 0.17, z), 0.03)


def clutter(b):
    """A crate and a barrel by the panel, cables snaking over the floor from it."""
    b.add(bevelled((-7.3, 0.7, 0.0), (-6.5, 1.5, 0.75), 0.03), "wood")
    for z in (0.12, 0.62):
        b.add(box((-7.32, 0.68, z - 0.04), (-6.48, 1.52, z + 0.04)), "wood_dark")
    b.add(box((-7.32, 0.68, 0.0), (-7.24, 0.76, 0.75)), "wood_dark")
    b.add(cone(0.3, 0.3, 0.8, 24), "steel_dark", matrix=trs((-6.1, 1.15, 0.0)))
    for z in (0.12, 0.68):
        b.add(cone(0.32, 0.32, 0.05, 24), "steel", matrix=trs((-6.1, 1.15, z)))
    for start, end, sway in (((-4.5, WALL_Y - 0.15, 0.6), (-3.2, 0.6, 0.02), 0.4),
                             ((-3.5, WALL_Y - 0.15, 0.6), (-2.6, 0.9, 0.02), -0.3)):
        s, e = Vector(start), Vector(end)
        points = []
        for t in np.linspace(0, 1, 9):
            p = s.lerp(e, t)
            p.z = max(0.02, s.z * (1 - t) ** 3 + 0.02)
            p.x += sway * math.sin(t * math.pi)
            points.append(p)
        b.add(tube(points, [0.025] * len(points), 6), "cable")


def build(out):
    toy.reset()
    wall()
    walkway()
    palette = toy.Palette("room_palette", COLORS)
    b = toy.Builder("room", palette)
    windows(b)
    panel(b)
    pipes(b)
    pillars(b)
    plates(b)
    lamps(b)
    beam(b)
    doorway(b)
    clutter(b)
    # the walkway's body behind its painted faces
    b.add(box((X0, FRONT_Y + 0.01, BOTTOM), (X1, WALL_Y, -0.01)), "steel_dark")
    mesh = b.build_static()
    for i, (at, glow) in enumerate(GLOWS):
        node = bpy.data.objects.new(f"glow.{i:03d}", None)
        node.location = at
        node["glow"] = glow
        bpy.context.scene.collection.objects.link(node)
    # which palette swatches get which detail (rows bottom up like the palette): brushed metal on the steel and
    # the panel; for room_detail.py
    size = toy.CELL * toy.GRID
    mask = np.zeros((size, size, 4), dtype=np.float32)
    for key, layer in (("steel", 0), ("steel_dark", 0), ("pipe", 0), ("panel", 1), ("panel_dark", 1),
                       ("wood", 2), ("wood_dark", 2)):
        row, col = divmod(palette.index[key], toy.GRID)
        mask[row * toy.CELL:(row + 1) * toy.CELL, col * toy.CELL:(col + 1) * toy.CELL, layer] = 1.0
    np.save(os.path.join(out, "room_mask.npy"), mask)
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.export_scene.gltf(filepath=os.path.join(out, "room.glb"), export_format="GLB", use_selection=True,
                              export_extras=True)
    print("RESULT room", tuple(round(d, 1) for d in mesh.dimensions), "faces", len(mesh.data.polygons),
          "glows", len(GLOWS))


build(toy.out_dir())
