# Story mode stage kits, one per world of the original game: floor tile, border block and the land around
# the field. Run headless: blender --background --factory-startup --python build_kits.py -- <out dir> [world ...]
# Pieces are one tile wide (1 unit), the floor's top is at z = 0, border blocks face -y towards the field.
import math
import os
import sys

import bmesh
import bpy
import numpy as np

ARGS = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
OUT = ARGS[0] if ARGS else os.path.join(os.path.dirname(__file__), "glb")
ONLY = [int(a) for a in ARGS[1:]]
TEX = 256
RNG = np.random.default_rng(7)


def noise(size, cells, octaves=4):
    """Smooth value noise in 0..1."""
    result = np.zeros((size, size))
    amplitude, total = 1.0, 0.0
    for o in range(octaves):
        n = cells * 2 ** o
        grid = RNG.random((n + 1, n + 1))
        grid[-1, :] = grid[0, :]
        grid[:, -1] = grid[:, 0]
        t = np.linspace(0, n, size, endpoint=False)
        i = t.astype(int)
        f = t - i
        f = f * f * (3 - 2 * f)
        a = grid[np.ix_(i, i)]
        b = grid[np.ix_(i, i + 1)]
        c = grid[np.ix_(i + 1, i)]
        d = grid[np.ix_(i + 1, i + 1)]
        fx, fy = f[None, :], f[:, None]
        result += amplitude * ((a * (1 - fx) + b * fx) * (1 - fy) + (c * (1 - fx) + d * fx) * fy)
        total += amplitude
        amplitude *= 0.5
    return result / total


def colorize(value, low, high):
    low, high = np.array(low, float), np.array(high, float)
    return low + (high - low) * value[..., None]


def tile_edges(size, width=0.05, depth=0.35):
    """Darkened tile border so the grid reads."""
    t = np.linspace(0, 1, size)
    edge = np.minimum(np.minimum(t, 1 - t)[None, :], np.minimum(t, 1 - t)[:, None])
    return 1 - depth * np.clip(1 - edge / width, 0, 1)


def bricks(size, rows=4, columns=2, mortar=0.06):
    y = np.linspace(0, rows, size, endpoint=False)
    row = y.astype(int)
    x = np.linspace(0, columns, size, endpoint=False)[None, :] + (row[:, None] % 2) * 0.5
    fy = (y - row)[:, None]
    fx = x - np.floor(x)
    joint = (np.minimum(fy, 1 - fy) < mortar) | (np.minimum(fx, 1 - fx) < mortar / 2)
    shade = RNG.random((rows, columns * 2 + 2))
    brick = shade[row[:, None] % rows, np.floor(x).astype(int) % shade.shape[1]]
    return joint, brick


def image(name, rgb):
    img = bpy.data.images.new(name, TEX, TEX, alpha=True)
    rgba = np.concatenate([np.clip(rgb, 0, 1), np.ones(rgb.shape[:2] + (1,))], axis=2)
    img.pixels.foreach_set(rgba.astype(np.float32).ravel())
    img.pack()
    return img


def material(name, img):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = next(n for n in nodes if n.type == "BSDF_PRINCIPLED")
    bsdf.inputs["Roughness"].default_value = 0.8
    tex = nodes.new("ShaderNodeTexImage")
    tex.image = img
    links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
    return mat


def box(name, size, bevel, segments=3):
    bpy.ops.mesh.primitive_cube_add(size=1)
    obj = bpy.context.object
    obj.name = name
    obj.scale = size
    bpy.ops.object.transform_apply(scale=True)
    mod = obj.modifiers.new("Bevel", "BEVEL")
    mod.width = bevel
    mod.segments = segments
    bpy.ops.object.modifier_apply(modifier=mod.name)
    return obj


def lumps(name, count, radius, height, seed):
    """A rock or bush: a few overlapping balls voxel-remeshed into one smooth surface."""
    rng = np.random.default_rng(seed)
    parts = []
    for i in range(count):
        a = 2 * math.pi * i / count + rng.random()
        r = radius * (0.5 + 0.3 * rng.random())
        bpy.ops.mesh.primitive_uv_sphere_add(radius=radius * (0.55 + 0.25 * rng.random()), segments=16, ring_count=10,
                                             location=(math.cos(a) * r * 0.6, math.sin(a) * r * 0.6,
                                                       height * (0.3 + 0.4 * rng.random())))
        parts.append(bpy.context.object)
    bpy.ops.mesh.primitive_uv_sphere_add(radius=radius * 0.75, segments=16, ring_count=10, location=(0, 0, height * 0.55))
    parts.append(bpy.context.object)
    bpy.ops.object.select_all(action="DESELECT")
    for p in parts:
        p.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    obj = bpy.context.object
    obj.name = name
    remesh = obj.modifiers.new("Remesh", "REMESH")
    remesh.mode = "VOXEL"
    remesh.voxel_size = 0.045
    smooth = obj.modifiers.new("Smooth", "SMOOTH")
    smooth.iterations = 6
    bpy.ops.object.modifier_apply(modifier=remesh.name)
    bpy.ops.object.modifier_apply(modifier=smooth.name)
    decimate = obj.modifiers.new("Decimate", "DECIMATE")
    decimate.ratio = min(1.0, 600 / max(1, len(obj.data.polygons)))
    bpy.ops.object.modifier_apply(modifier=decimate.name)
    # sit on the ground, no wider than a tile
    co = np.array([v.co for v in obj.data.vertices])
    lo, hi = co.min(axis=0), co.max(axis=0)
    scale = min(1.0, 1.02 / max(hi[0] - lo[0], hi[1] - lo[1]))
    for v in obj.data.vertices:
        v.co.x = (v.co.x - (lo[0] + hi[0]) / 2) * scale
        v.co.y = (v.co.y - (lo[1] + hi[1]) / 2) * scale
        v.co.z = (v.co.z - lo[2]) * scale
    return obj


def plane(name, size, z):
    bpy.ops.mesh.primitive_plane_add(size=size, location=(0, 0, z))
    obj = bpy.context.object
    obj.name = name
    return obj


def project_uv(obj, mode, repeat=1.0):
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    if mode == "cube":
        bpy.ops.uv.cube_project(cube_size=1.0)
    else:
        bpy.ops.uv.smart_project(angle_limit=1.15, island_margin=0.01)
    bpy.ops.object.mode_set(mode="OBJECT")
    if mode == "top":
        uv = obj.data.uv_layers.active.data
        for poly in obj.data.polygons:
            for li in poly.loop_indices:
                co = obj.data.vertices[obj.data.loops[li].vertex_index].co
                uv[li].uv = ((co.x + 0.5) * repeat, (co.y + 0.5) * repeat)
    for poly in obj.data.polygons:
        poly.use_smooth = True


def export(obj, path):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.gltf(filepath=path, export_format="GLB", use_selection=True, export_yup=True,
                              export_animations=False)
    print("RESULT", path, len(obj.data.polygons))


def clear():
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o, do_unlink=True)


# --- the worlds -----------------------------------------------------------------------------------

def floor_texture(low, high, cells=3, edges=0.35, pattern=None):
    rgb = colorize(noise(TEX, cells), low, high)
    if pattern is not None:
        rgb = pattern(rgb)
    return rgb * tile_edges(TEX, depth=edges)[..., None]


def brick_texture(mortar_color, low, high, rows=4):
    joint, brick = bricks(TEX, rows=rows)
    rgb = colorize(0.6 * brick + 0.4 * noise(TEX, 6), low, high)
    rgb[joint] = mortar_color
    return rgb


def rock_texture(low, high):
    return colorize(noise(TEX, 4, octaves=5) ** 1.3, low, high)


def stone_tiles(rgb, lines=2, color=(0.18, 0.18, 0.2)):
    t = np.linspace(0, lines, TEX, endpoint=False)
    f = t - np.floor(t)
    joint = (np.minimum(f, 1 - f) < 0.03)
    rgb[joint[None, :].repeat(TEX, 0) | joint[:, None].repeat(TEX, 1)] = color
    return rgb


def carpet(rgb):
    t = np.linspace(0, 1, TEX)
    trim = (np.abs(t - 0.5) > 0.36) & (np.abs(t - 0.5) < 0.42)
    rgb[:, trim] = (0.85, 0.65, 0.2)
    return rgb


def water(size_cells=6):
    n = noise(TEX, size_cells)
    streak = (np.sin(np.linspace(0, 12 * math.pi, TEX))[None, :] * 0.5 + 0.5) * 0.25
    return colorize(np.clip(n * 0.8 + streak, 0, 1), (0.05, 0.25, 0.65), (0.35, 0.65, 0.95))


def lava():
    n = noise(TEX, 5, octaves=5)
    crust = n < 0.45
    rgb = colorize(np.clip((n - 0.45) * 2.5, 0, 1), (1.0, 0.25, 0.02), (1.0, 0.85, 0.2))
    rgb[crust] = colorize(n[crust] / 0.45, (0.08, 0.04, 0.03), (0.25, 0.1, 0.06))
    return rgb


def cobbles(low, high, joint, cells=5):
    """Irregular stones, tileable: a jittered grid that wraps."""
    t = np.linspace(0, cells, TEX, endpoint=False)
    X, Y = np.meshgrid(t, t)
    jitter = RNG.random((cells, cells, 2))
    f1 = np.full(X.shape, 9.0)
    f2 = np.full(X.shape, 9.0)
    for dj in (-1, 0, 1):
        for di in (-1, 0, 1):
            cx, cy = np.floor(X) + di, np.floor(Y) + dj
            j = jitter[cy.astype(int) % cells, cx.astype(int) % cells]
            dist = np.hypot(X - cx - j[..., 0], Y - cy - j[..., 1])
            f2 = np.where(dist < f1, f1, np.minimum(f2, dist))
            f1 = np.minimum(f1, dist)
    rgb = colorize(np.clip(1.0 - f1 * 0.9 + noise(TEX, 8) * 0.3, 0, 1), low, high)
    rgb[(f2 - f1) < 0.08] = joint
    return rgb


def diamonds(light, dark, line, count=2):
    """Diamond tiles like the mechanical castle's floors."""
    t = np.linspace(0, count, TEX, endpoint=False)
    X, Y = np.meshgrid(t, t)
    u, v = X + Y, X - Y
    fu, fv = u - np.floor(u), v - np.floor(v)
    checker = (np.floor(u) + np.floor(v)) % 2
    rgb = colorize(checker * (0.8 + 0.2 * noise(TEX, 4)), dark, light)
    edge = np.minimum(np.minimum(fu, 1 - fu), np.minimum(fv, 1 - fv))
    rgb[edge < 0.05] = line
    # a soft highlight on each diamond's upper half
    rgb *= (1.0 + 0.12 * np.clip(0.5 - fu, -0.5, 0.5))[..., None]
    return rgb


def ripples():
    n = noise(TEX, 3)
    t = np.linspace(0, 2 * math.pi * 3, TEX, endpoint=False)
    X, Y = np.meshgrid(t, t)
    wave = np.sin(X + np.sin(Y) * 0.8 + n * 4) * 0.5 + 0.5
    # like the river's water shader: deep teal blue, soft caustics instead of drawn wave lines
    rgb = colorize(np.clip(n * 0.6 + wave * 0.4, 0, 1), (0.05, 0.27, 0.48), (0.14, 0.42, 0.66))
    caustic = np.clip((wave - 0.8) / 0.2, 0, 1)
    rgb = rgb + caustic[..., None] * np.array([0.18, 0.24, 0.26])
    return rgb


def trodden_earth():
    """Soft brown earth with a few pebbles, like the cave's ground around the field."""
    rgb = colorize(noise(TEX, 3, octaves=5), (0.46, 0.3, 0.15), (0.6, 0.42, 0.22))
    rgb = rgb * (0.88 + 0.12 * noise(TEX, 6))[..., None]
    pebbles = cobbles((0.5, 0.38, 0.26), (0.66, 0.52, 0.36), (0.42, 0.3, 0.18), cells=7)
    stones = noise(TEX, 7) > 0.72
    rgb[stones] = rgb[stones] * 0.4 + pebbles[stones] * 0.6
    return rgb


def metal_panels(base):
    rgb = colorize(noise(TEX, 6), np.array(base) * 0.85, np.array(base) * 1.05)
    t = np.linspace(0, 1, TEX)
    for line in (0.0, 0.5):
        band = np.abs(t - line) < 0.02
        rgb[band, :] = np.array(base) * 0.5
        rgb[:, band] = np.array(base) * 0.5
    vent = (np.abs(t - 0.25) < 0.12)[:, None] & ((np.sin(t * 60) > 0.6) & (np.abs(t - 0.75) < 0.15))[None, :]
    rgb[vent] = (0.75, 0.82, 0.25)
    return rgb


WORLDS = {
    1: dict(name="the_wall",
            floor=lambda: floor_texture((0.18, 0.5, 0.12), (0.32, 0.7, 0.2)),
            border=("bricks", lambda: brick_texture((0.45, 0.43, 0.4), (0.45, 0.45, 0.48), (0.7, 0.7, 0.72))),
            land=lambda: colorize(noise(TEX, 8), (0.12, 0.38, 0.1), (0.22, 0.5, 0.16))),
    2: dict(name="rocky_mountains",
            floor=lambda: cobbles((0.5, 0.5, 0.62), (0.7, 0.7, 0.8), (0.3, 0.3, 0.38)) * tile_edges(TEX, depth=0.2)[..., None],
            border=("rock", lambda: rock_texture((0.32, 0.16, 0.07), (0.62, 0.34, 0.15))),
            land=lambda: rock_texture((0.38, 0.28, 0.18), (0.55, 0.42, 0.28))),
    3: dict(name="river",
            # a shallow riverbed, the water shader floods it
            floor=lambda: cobbles((0.5, 0.46, 0.36), (0.7, 0.64, 0.5), (0.36, 0.32, 0.26), cells=6),
            border=("bricks", lambda: brick_texture((0.3, 0.32, 0.35), (0.5, 0.55, 0.6), (0.72, 0.76, 0.8), rows=3)),
            land=water),
    4: dict(name="forest",
            floor=lambda: floor_texture((0.12, 0.35, 0.1), (0.25, 0.5, 0.15)),
            border=("bush", lambda: colorize(noise(TEX, 6), (0.04, 0.22, 0.05), (0.2, 0.55, 0.12))),
            land=lambda: colorize(noise(TEX, 10), (0.05, 0.2, 0.05), (0.14, 0.34, 0.1))),
    5: dict(name="lava_cave",
            floor=lambda: trodden_earth() * tile_edges(TEX, depth=0.1)[..., None],
            border=("rock", lambda: rock_texture((0.3, 0.17, 0.08), (0.52, 0.32, 0.16))),
            land=lava),
    6: dict(name="castle_1",
            floor=lambda: diamonds((0.66, 0.44, 0.66), (0.48, 0.28, 0.5), (0.3, 0.16, 0.3)),
            border=("bricks", lambda: brick_texture((0.25, 0.12, 0.08), (0.45, 0.22, 0.14), (0.62, 0.32, 0.2))),
            land=lambda: colorize(noise(TEX, 8), (0.1, 0.1, 0.14), (0.2, 0.2, 0.26))),
    7: dict(name="castle_2",
            floor=lambda: diamonds((0.58, 0.42, 0.6), (0.38, 0.26, 0.44), (0.22, 0.14, 0.26)),
            border=("bricks", lambda: brick_texture((0.28, 0.3, 0.34), (0.42, 0.44, 0.5), (0.58, 0.6, 0.66), rows=3)),
            land=lambda: colorize(noise(TEX, 8), (0.12, 0.08, 0.14), (0.22, 0.16, 0.26))),
    8: dict(name="castle_final",
            floor=lambda: diamonds((0.14, 0.56, 0.62), (0.06, 0.38, 0.46), (0.03, 0.2, 0.26)),
            border=("bricks", lambda: metal_panels((0.62, 0.64, 0.7))),
            land=lambda: colorize(noise(TEX, 8), (0.12, 0.05, 0.05), (0.22, 0.1, 0.1))),
}


def build_world(index, spec):
    folder = os.path.join(OUT, f"world{index}")
    # every world draws its own noise, building one doesn't change the others
    global RNG
    RNG = np.random.default_rng(7 + index)

    clear()
    floor = box("floor", (1.0, 1.0, 0.12), 0.03, 2)
    floor.location.z = -0.06
    bpy.ops.object.transform_apply(location=True)
    project_uv(floor, "top")
    floor.data.materials.append(material(f"w{index}_floor", image(f"w{index}_floor", spec["floor"]())))
    export(floor, os.path.join(folder, "floor.glb"))

    clear()
    kind, texture = spec["border"]
    if kind == "bricks":
        border = box("border", (1.0, 1.0, 0.95), 0.06)
        border.location.z = 0.475
        bpy.ops.object.transform_apply(location=True)
        project_uv(border, "cube")
    else:
        border = lumps("border", 5, 0.5, 0.95 if kind == "rock" else 1.05, seed=index)
        project_uv(border, "smart")
    border.data.materials.append(material(f"w{index}_border", image(f"w{index}_border", texture())))
    export(border, os.path.join(folder, "border.glb"))

    clear()
    land = plane("surroundings", 5.0, -0.13)
    project_uv(land, "top", repeat=12.0)
    uv = land.data.uv_layers.active.data
    for loop in uv:
        loop.uv = (loop.uv[0] * 1.0, loop.uv[1] * 1.0)
    land.data.materials.append(material(f"w{index}_land", image(f"w{index}_land", spec["land"]())))
    export(land, os.path.join(folder, "surroundings.glb"))


for index, spec in WORLDS.items():
    if not ONLY or index in ONLY:
        build_world(index, spec)
