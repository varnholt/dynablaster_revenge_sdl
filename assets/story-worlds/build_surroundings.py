# Story mode surroundings: a diorama per world and field size around the stage, terrain plus props, colour and
# ambient occlusion baked into textures. Run headless:
#   blender --background --factory-startup --python build_surroundings.py -- <out dir> [world ...]
# Units are tiles, the field covers x 0..W, y -H..0 (top left corner at the origin), z up, its floor's top at 0.
# The game draws the result untransformed, pieces outside the apron around the field only.
import math
import os
import sys

import bpy
import numpy as np
from mathutils import Matrix

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
import detail  # noqa: E402

ARGS = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
OUT = ARGS[0] if ARGS else os.path.join(os.path.dirname(__file__), "surroundings")
ONLY = [int(a) for a in ARGS[1:]]

# field sizes: single screen, horizontal and vertical scrolling (shared/storystage.h)
SIZES = {None: (13, 11), "H": (29, 11), "V": (13, 23)}
GROUND_TEX = 2048
DETAIL_IMAGES = detail.images()  # grass, soil, rock, pebbles, tiled over the ground up close
PROP_TEX = 2048
AO_TEX = 2048
FLOOR_Z = -0.14
APRON = 1.4  # border blocks plus a little walkway, kept flat and free


# --- noise ----------------------------------------------------------------------------------------

def _hash(i, j, seed):
    v = np.sin(i * 127.1 + j * 311.7 + seed * 74.7) * 43758.5453
    return v - np.floor(v)


def vnoise(x, y, seed=0):
    xi, yi = np.floor(x), np.floor(y)
    fx, fy = x - xi, y - yi
    fx, fy = fx * fx * (3 - 2 * fx), fy * fy * (3 - 2 * fy)
    a, b = _hash(xi, yi, seed), _hash(xi + 1, yi, seed)
    c, d = _hash(xi, yi + 1, seed), _hash(xi + 1, yi + 1, seed)
    return (a * (1 - fx) + b * fx) * (1 - fy) + (c * (1 - fx) + d * fx) * fy


def fbm(x, y, scale=1.0, octaves=4, seed=0):
    total, amp, norm = 0.0, 1.0, 0.0
    for o in range(octaves):
        total = total + amp * vnoise(x * scale * 2 ** o, y * scale * 2 ** o, seed + o * 13)
        norm += amp
        amp *= 0.5
    return total / norm


def voronoi(x, y, seed=0):
    """Distances to the nearest and second nearest jittered cell point."""
    xi, yi = np.floor(x), np.floor(y)
    f1 = np.full(x.shape, 9.0)
    f2 = np.full(x.shape, 9.0)
    for dj in (-1, 0, 1):
        for di in (-1, 0, 1):
            cx, cy = xi + di, yi + dj
            px = cx + _hash(cx, cy, seed)
            py = cy + _hash(cx, cy, seed + 1)
            dist = np.hypot(x - px, y - py)
            f2 = np.where(dist < f1, f1, np.minimum(f2, dist))
            f1 = np.minimum(f1, dist)
    return f1, f2


def smooth(e0, e1, v):
    t = np.clip((v - e0) / (e1 - e0), 0, 1)
    return t * t * (3 - 2 * t)


def mix(a, b, t):
    a, b = np.asarray(a, float), np.asarray(b, float)
    t = np.asarray(t, float)[..., None]
    return a * (1 - t) + b * t


# --- the stage context ----------------------------------------------------------------------------

class Stage:
    def __init__(self, w, h, seed):
        self.w, self.h = w, h
        self.x0, self.x1 = -22.0, w + 22.0
        self.y0, self.y1 = -h - 9.0, 19.0
        self.rng = np.random.default_rng(seed)
        self.cx = w * 0.5

    def outside(self, x, y, pad=APRON):
        """Distance from the field grown by pad (default: the apron), 0 on and inside it."""
        dx = np.maximum(np.maximum(-pad - x, x - (self.w + pad)), 0)
        dy = np.maximum(np.maximum(-self.h - pad - y, y - pad), 0)
        return np.hypot(dx, dy)

    def outside_box(self, x, y, pad):
        """Like outside(), with square corners."""
        dx = np.maximum(np.maximum(-pad - x, x - (self.w + pad)), 0)
        dy = np.maximum(np.maximum(-self.h - pad - y, y - pad), 0)
        return np.maximum(dx, dy)

    def north(self, y):
        """0 at the field's top edge, growing towards the back."""
        return np.maximum(y - APRON, 0)

    def scatter(self, count, density, min_dist, taken, margin=0.6, tries=40, pad=APRON):
        """Random points with the given density function (0..1), apart from each other and from taken,
        at least margin away from the field grown by pad."""
        points = []
        for _ in range(count * tries):
            if len(points) >= count:
                break
            x = self.rng.uniform(self.x0, self.x1)
            y = self.rng.uniform(self.y0, self.y1)
            if self.outside(np.array(x), np.array(y), pad) < margin:
                continue
            if self.rng.random() > float(density(np.array(x), np.array(y))):
                continue
            if any((x - px) ** 2 + (y - py) ** 2 < (min_dist + pr) ** 2 for px, py, pr in taken):
                continue
            points.append((x, y))
            taken.append((x, y, min_dist * 0.5))
        return points

    def near_border(self, count, min_dist, taken, depth=1.6):
        """Points in the strip just outside the border blocks, for low clutter only."""
        return self.scatter(count, lambda x, y: float(self.outside(x, y, 1.0) < depth), min_dist, taken, margin=0.12,
                            pad=1.0)


# --- materials ------------------------------------------------------------------------------------

_MATERIALS = {}


def material(name, color, variation=0.12, scale=3.0, kind="noise", color2=None, mortar=None):
    """Procedural colour (baked later): noise, bricks or stripes on the world position."""
    if name in _MATERIALS:
        return _MATERIALS[name]
    # colours are given like the ground's, in sRGB
    color = tuple(c ** 2.2 for c in color)
    color2 = None if color2 is None else tuple(c ** 2.2 for c in color2)
    mortar = None if mortar is None else tuple(c ** 2.2 for c in mortar)
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = next(n for n in nodes if n.type == "BSDF_PRINCIPLED")
    bsdf.inputs["Roughness"].default_value = 0.8
    geometry = nodes.new("ShaderNodeNewGeometry")
    low = tuple(max(0.0, c * (1 - variation)) for c in color) + (1,)
    high = tuple(min(1.0, c * (1 + variation)) for c in color) + (1,)
    if kind == "bricks":
        sep = nodes.new("ShaderNodeSeparateXYZ")
        links.new(geometry.outputs["Position"], sep.inputs[0])
        add = nodes.new("ShaderNodeMath")
        add.operation = "ADD"
        links.new(sep.outputs[0], add.inputs[0])
        links.new(sep.outputs[1], add.inputs[1])
        comb = nodes.new("ShaderNodeCombineXYZ")
        links.new(add.outputs[0], comb.inputs[0])
        links.new(sep.outputs[2], comb.inputs[1])
        brick = nodes.new("ShaderNodeTexBrick")
        brick.inputs["Scale"].default_value = scale
        brick.inputs["Mortar Size"].default_value = 0.025
        brick.inputs["Color1"].default_value = low
        brick.inputs["Color2"].default_value = high
        brick.inputs["Mortar"].default_value = (mortar or tuple(c * 0.55 for c in color)) + (1,)
        links.new(comb.outputs[0], brick.inputs["Vector"])
        links.new(brick.outputs["Color"], bsdf.inputs["Base Color"])
    else:
        noise = nodes.new("ShaderNodeTexNoise")
        noise.inputs["Scale"].default_value = scale
        noise.inputs["Detail"].default_value = 4.0
        links.new(geometry.outputs["Position"], noise.inputs["Vector"])
        ramp = nodes.new("ShaderNodeValToRGB")
        ramp.color_ramp.elements[0].color = low
        ramp.color_ramp.elements[1].color = high if color2 is None else tuple(color2) + (1,)
        ramp.color_ramp.elements[0].position = 0.3
        ramp.color_ramp.elements[1].position = 0.7
        links.new(noise.outputs["Fac"], ramp.inputs["Fac"])
        links.new(ramp.outputs["Color"], bsdf.inputs["Base Color"])
    _MATERIALS[name] = mat
    return mat


# --- primitive props ------------------------------------------------------------------------------

PROPS = []


def _finish(obj, mat, smooth_shade=True):
    obj.data.materials.append(mat)
    for p in obj.data.polygons:
        p.use_smooth = smooth_shade
    PROPS.append(obj)
    return obj


def sphere(loc, radius, mat, scale=(1, 1, 1), subdiv=2, jitter=0.0, rng=None):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=subdiv, radius=radius, location=loc)
    obj = bpy.context.object
    obj.scale = scale
    if jitter and rng is not None:
        for v in obj.data.vertices:
            v.co *= 1 + rng.uniform(-jitter, jitter)
    return _finish(obj, mat)


def uv_sphere(loc, radius, mat, scale=(1, 1, 1), segments=24, rings=12):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=segments, ring_count=rings, radius=radius, location=loc)
    obj = bpy.context.object
    obj.scale = scale
    return _finish(obj, mat)


def dome(loc, radius, mat, segments=32, rings=10):
    """Upper half sphere, open at the bottom (it stands on something)."""
    bpy.ops.mesh.primitive_uv_sphere_add(segments=segments, ring_count=rings * 2, radius=radius, location=loc)
    obj = bpy.context.object
    import bmesh
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if v.co.z < -1e-4], context="VERTS")
    bm.to_mesh(obj.data)
    bm.free()
    return _finish(obj, mat)


def cylinder(loc, radius, depth, mat, vertices=16, scale=(1, 1, 1), smooth_shade=True):
    """Standing on loc."""
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=depth,
                                        location=(loc[0], loc[1], loc[2] + depth * 0.5 * scale[2]))
    obj = bpy.context.object
    obj.scale = scale
    _finish(obj, mat, smooth_shade)
    # flat caps
    for p in obj.data.polygons:
        if abs(p.normal.z) > 0.9:
            p.use_smooth = False
    return obj


def cone(loc, radius, depth, mat, vertices=16, radius2=0.0):
    bpy.ops.mesh.primitive_cone_add(vertices=vertices, radius1=radius, radius2=radius2, depth=depth,
                                    location=(loc[0], loc[1], loc[2] + depth * 0.5))
    return _finish(bpy.context.object, mat)


def box(loc, size, mat, rotation=0.0, bevel=0.0):
    """Standing on loc."""
    bpy.ops.mesh.primitive_cube_add(size=1, location=(loc[0], loc[1], loc[2] + size[2] * 0.5))
    obj = bpy.context.object
    obj.scale = size
    obj.rotation_euler.z = rotation
    if bevel:
        bpy.ops.object.transform_apply(scale=True)
        mod = obj.modifiers.new("Bevel", "BEVEL")
        mod.width = bevel
        mod.segments = 2
        bpy.ops.object.modifier_apply(modifier=mod.name)
    return _finish(obj, mat, smooth_shade=False)


def fuse(start, voxel, faces, smooth_steps=0):
    """Joins the props made since start into one closed mesh: no hidden inner faces that would bake black.
    The voxel size grows until the remesh stays near faces on its own; decimating it instead tears holes."""
    parts = PROPS[start:]
    del PROPS[start:]
    bpy.ops.object.select_all(action="DESELECT")
    for part in parts:
        part.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    bpy.ops.object.join()
    obj = bpy.context.object
    area = sum(poly.area for poly in obj.data.polygons)
    remesh = obj.modifiers.new("Remesh", "REMESH")
    remesh.mode = "VOXEL"
    remesh.voxel_size = max(voxel, math.sqrt(area / faces))
    bpy.ops.object.modifier_apply(modifier=remesh.name)
    if smooth_steps:
        smoothing = obj.modifiers.new("Smooth", "SMOOTH")
        smoothing.iterations = smooth_steps
        bpy.ops.object.modifier_apply(modifier=smoothing.name)
    for poly in obj.data.polygons:
        poly.use_smooth = True
    PROPS.append(obj)
    return obj


GLOW_STEADY, GLOW_FLAME, GLOW_BLINK, GLOW_PULSE = 0, 1, 2, 3


def glow(loc, color, size, mode=GLOW_STEADY):
    """A light's halo for the game (GltfModel::Glow): an empty with the glow in its extras."""
    obj = bpy.data.objects.new("glow", None)
    obj.location = loc
    obj["glow"] = [float(c) for c in color] + [float(size), float(mode)]
    bpy.context.scene.collection.objects.link(obj)
    return obj


# --- prop kinds -----------------------------------------------------------------------------------

def tree_round(st, x, y, z, s, leaves, trunk):
    rng = st.rng
    cylinder((x, y, z - 0.05), 0.09 * s, 0.75 * s, trunk, vertices=8)
    start = len(PROPS)
    blobs = 3
    for i in range(blobs):
        a = 2 * math.pi * i / blobs + rng.uniform(0, 2)
        r = 0.18 * s
        sphere((x + math.cos(a) * r, y + math.sin(a) * r, z + (0.85 + rng.uniform(0, 0.25)) * s),
               (0.36 + rng.uniform(0, 0.1)) * s, leaves, jitter=0.06, rng=rng)
    sphere((x, y, z + 1.15 * s), 0.34 * s, leaves, jitter=0.06, rng=rng)
    fuse(start, 0.035 * s, 360, smooth_steps=2)


def tree_pine(st, x, y, z, s, leaves, trunk):
    cylinder((x, y, z - 0.05), 0.08 * s, 0.5 * s, trunk, vertices=8)
    start = len(PROPS)
    for i, (r, h, o) in enumerate(((0.5, 0.75, 0.3), (0.4, 0.65, 0.68), (0.28, 0.55, 1.02))):
        cone((x, y, z + o * s), r * s, h * s, leaves, vertices=10)
    fuse(start, 0.03 * s, 300)


def rock(st, x, y, z, s, mat, flat=0.65):
    rng = st.rng
    sphere((x, y, z + 0.1 * s), 0.5 * s, mat, scale=(1 + rng.uniform(-0.2, 0.3), 1 + rng.uniform(-0.2, 0.3), flat),
           subdiv=2, jitter=0.18, rng=rng).rotation_euler.z = rng.uniform(0, 6.3)


def bush(st, x, y, z, s, mat):
    rng = st.rng
    start = len(PROPS)
    for i in range(3):
        a = rng.uniform(0, 6.3)
        sphere((x + math.cos(a) * 0.18 * s, y + math.sin(a) * 0.18 * s, z + 0.15 * s), (0.25 + rng.uniform(0, 0.08)) * s,
               mat, scale=(1, 1, 0.8), jitter=0.05, rng=rng)
    fuse(start, 0.03 * s, 200, smooth_steps=2)


def wall_run(p0, p1, z, height, thickness, mat, cap_mat=None, merlons=False):
    (x0, y0), (x1, y1) = p0, p1
    length = math.hypot(x1 - x0, y1 - y0)
    angle = math.atan2(y1 - y0, x1 - x0)
    mx, my = (x0 + x1) * 0.5, (y0 + y1) * 0.5
    box((mx, my, z - 0.2), (length, thickness, height + 0.2), mat, rotation=angle, bevel=0.03)
    if cap_mat:
        box((mx, my, z + height), (length + 0.06, thickness + 0.08, 0.08), cap_mat, rotation=angle)
    if merlons:
        count = max(2, int(length / 0.5))
        for i in range(count):
            if i % 2:
                continue
            t = (i + 0.5) / count
            box((x0 + (x1 - x0) * t, y0 + (y1 - y0) * t, z + height), (length / count, thickness * 0.9, 0.25),
                mat, rotation=angle)


# --- terrain --------------------------------------------------------------------------------------

def terrain(st, height_fn, color_fn, name):
    step = 0.5
    xs = np.arange(st.x0, st.x1 + 1e-6, step)
    ys = np.arange(st.y0, st.y1 + 1e-6, step)
    X, Y = np.meshgrid(xs, ys)
    Z = height_fn(X, Y)
    nx, ny = len(xs), len(ys)
    verts = np.stack([X.ravel(), Y.ravel(), Z.ravel()], axis=1)
    idx = np.arange(nx * ny).reshape(ny, nx)
    a, b, c, d = idx[:-1, :-1].ravel(), idx[:-1, 1:].ravel(), idx[1:, 1:].ravel(), idx[1:, :-1].ravel()
    faces = np.stack([a, b, c, d], axis=1)
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(verts.tolist(), [], faces.tolist())
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    uv = mesh.uv_layers.new(name="UVMap")
    co = verts[np.array([l.vertex_index for l in mesh.loops])]
    uvs = np.stack([(co[:, 0] - st.x0) / (st.x1 - st.x0), (co[:, 1] - st.y0) / (st.y1 - st.y0)], axis=1)
    uv.data.foreach_set("uv", uvs.ravel())
    for p in mesh.polygons:
        p.use_smooth = True

    # colour straight from the height field, in texture space
    u = (np.arange(GROUND_TEX) + 0.5) / GROUND_TEX
    TX, TY = np.meshgrid(st.x0 + u * (st.x1 - st.x0), st.y0 + u * (st.y1 - st.y0))
    TZ = height_fn(TX, TY)
    gy, gx = np.gradient(TZ, (st.y1 - st.y0) / GROUND_TEX, (st.x1 - st.x0) / GROUND_TEX)
    slope = np.hypot(gx, gy)
    rgb = color_fn(TX, TY, TZ, slope)
    # where grass, soil, rock and pebbles show up close, at half the colour's resolution
    half = GROUND_TEX // 2
    GROUND_DETAIL[:] = [detail.ground_mask(np.clip(rgb, 0, 1), slope).reshape(half, 2, half, 2, 4).mean(axis=(1, 3))]
    img = bpy.data.images.new(f"{name}_color", GROUND_TEX, GROUND_TEX)
    img.pixels.foreach_set(np.concatenate([np.clip(rgb, 0, 1), np.ones((GROUND_TEX, GROUND_TEX, 1))], 2)
                           .astype(np.float32).ravel())
    mat = bpy.data.materials.new(f"{name}_mat")
    mat.use_nodes = True
    bsdf = next(n for n in mat.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
    tex = mat.node_tree.nodes.new("ShaderNodeTexImage")
    tex.image = img
    mat.node_tree.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
    mesh.materials.append(mat)
    return obj, img


WATER = []
GROUND_DETAIL = []  # the ground's detail mask, see detail.ground_mask()
PROPS_DETAIL = []  # the props'


def water_surface(st, height_fn, level, deep_color, field_depth=None):
    """The water as its own mesh for the game's water shader ("water": [deep r, g, b] in its extras); its texture
    is data: red the depth (0..0.8 tiles), green the foam along the shores and the island's wall."""
    step = 0.5
    xs = np.arange(st.x0, st.x1 + 1e-6, step)
    ys = np.arange(st.y0, st.y1 + 1e-6, step)
    X, Y = np.meshgrid(xs, ys)
    wet = height_fn(X, Y) < level + 0.15
    nx, ny = len(xs), len(ys)
    idx = np.arange(nx * ny).reshape(ny, nx)
    quads = np.stack([idx[:-1, :-1].ravel(), idx[:-1, 1:].ravel(), idx[1:, 1:].ravel(), idx[1:, :-1].ravel()], axis=1)
    keep = wet.ravel()[quads].any(axis=1)
    verts = np.stack([X.ravel(), Y.ravel(), np.full(X.size, level)], axis=1)
    mesh = bpy.data.meshes.new("water")
    mesh.from_pydata(verts.tolist(), [], quads[keep].tolist())
    mesh.validate()
    obj = bpy.data.objects.new("water", mesh)
    bpy.context.scene.collection.objects.link(obj)
    uv = mesh.uv_layers.new(name="UVMap")
    co = np.array([mesh.vertices[l.vertex_index].co[:] for l in mesh.loops])
    uv.data.foreach_set("uv", np.stack([(co[:, 0] - st.x0) / (st.x1 - st.x0), (co[:, 1] - st.y0) / (st.y1 - st.y0)], 1).ravel())

    size = 1024
    u = (np.arange(size) + 0.5) / size
    TX, TY = np.meshgrid(st.x0 + u * (st.x1 - st.x0), st.y0 + u * (st.y1 - st.y0))
    depth = np.clip((level - height_fn(TX, TY)) / 0.8, 0, 1)
    if field_depth is not None:
        # over the field the floor tiles lie just below, the water still looks as deep as it should
        depth = np.where(st.outside(TX, TY, 1.0) <= 0, field_depth, depth)
    d = st.outside(TX, TY)
    # along the shores and in a ring around the island's wall (1.2 tiles out from the apron)
    foam = np.maximum(smooth(0.12, 0.0, depth), smooth(0.8, 1.05, d) * smooth(1.9, 1.45, d))
    data = np.stack([depth, foam, np.zeros_like(depth), np.ones_like(depth)], axis=2)
    img = bpy.data.images.new("water_data", size, size)
    img.pixels.foreach_set(data.astype(np.float32).ravel())
    mat = bpy.data.materials.new("water")
    mat.use_nodes = True
    bsdf = next(n for n in mat.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
    tex = mat.node_tree.nodes.new("ShaderNodeTexImage")
    tex.image = img
    mat.node_tree.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
    mesh.materials.append(mat)
    obj["water"] = [float(c) for c in deep_color]
    WATER.append((obj, img))
    return obj


def ground_height(st, x, y, fn):
    return float(fn(np.array([x]), np.array([y]))[0])


# --- the worlds -----------------------------------------------------------------------------------

def grass_colors(TX, TY, base_low, base_high, seed=1):
    n = fbm(TX, TY, 0.35, 5, seed)
    fine = vnoise(TX * 6, TY * 6, seed + 5)
    return mix(base_low, base_high, np.clip(n * 1.1 - 0.05 + (fine - 0.5) * 0.25, 0, 1))


def apron_mix(st, TX, TY, rgb, apron_rgb, reach=0.9, ragged=0.9, soft=0.25, square=False):
    """The strip around the border blocks in apron_rgb (a colour or a texture), fraying into the land,
    with the blocks' drop shadow painted in."""
    border = st.outside_box(TX, TY, 1.0) if square else st.outside(TX, TY, 1.0)
    d = border - (fbm(TX, TY, 0.7, 3, 99) - 0.5) * ragged
    rgb = mix(apron_rgb, rgb, smooth(reach - soft, reach + soft, d))
    return rgb * (1 - 0.4 * smooth(0.75, 0.0, st.outside(TX, TY, 1.0)))[..., None]


def pebbles(TX, TY, base, stone, scale=3.0, seed=90):
    """Small stones on a base texture."""
    f1, f2 = voronoi(TX * scale, TY * scale, seed)
    size = 0.22 + 0.2 * _hash(np.floor(TX * scale), np.floor(TY * scale), seed + 2)
    shade = 0.75 + 0.5 * _hash(np.floor(TX * scale) + 3, np.floor(TY * scale), seed + 3)
    stones = smooth(size, size - 0.06, f1)
    return mix(base, np.asarray(stone) * shade[..., None], stones)


def world_meadow(st):
    """1: the meadow with Dr's lab and its white walls (the intro and the first map stop)."""
    def height(x, y):
        d = st.outside(x, y)
        hills = fbm(x, y, 0.09, 4, 3) * 1.8 + smooth(6, 16, st.north(y)) * 2.2
        return FLOOR_Z + smooth(0.6, 5.0, d) * hills

    path_y = APRON + 0.3

    def color(TX, TY, TZ, slope):
        rgb = grass_colors(TX, TY, (0.16, 0.42, 0.1), (0.36, 0.66, 0.18))
        # a sandy path from the lab down to the field
        path = (np.abs(TX - st.cx - 0.6 * np.sin(TY * 0.4)) < 0.75) & (TY > path_y - 1) & (TY < 6.2)
        rgb = mix(rgb, (0.72, 0.6, 0.4), path * (0.85 + 0.15 * vnoise(TX * 5, TY * 5, 9)))
        rgb = mix(rgb, (0.12, 0.3, 0.08), smooth(0.25, 0.8, slope) * 0.5)
        gravel = mix((0.6, 0.55, 0.44), (0.74, 0.68, 0.55), fbm(TX, TY, 2.0, 3, 91))
        gravel = pebbles(TX, TY, gravel, (0.66, 0.64, 0.6), 3.5)
        return apron_mix(st, TX, TY, rgb, gravel)

    ground, img = terrain(st, height, color, "ground")
    leaves = [material(f"leaves{i}", c) for i, c in enumerate(((0.14, 0.45, 0.1), (0.2, 0.55, 0.14), (0.1, 0.36, 0.12)))]
    trunk = material("trunk", (0.35, 0.22, 0.12))
    stone = material("boulder", (0.55, 0.55, 0.58), 0.25, 2.0)
    white = material("white_wall", (0.88, 0.88, 0.86), 0.06, 5.0, kind="bricks", mortar=(0.62, 0.62, 0.62))
    cap = material("wall_cap", (0.95, 0.95, 0.93), 0.03)

    taken = []
    # Dr's lab behind the field, its dome just behind the border
    lab(st, st.cx, APRON + 0.5, height, taken, 1.6)
    if st.w > 20:
        lab(st, st.cx + 15.5, APRON + 1.5, height, taken, 1.0)
        lab(st, st.cx - 15.5, APRON + 1.5, height, taken, 1.0)
    taken.append((st.cx, 3.5, 1.2))  # the path

    # white walls like on the map: open enclosures left and right of the field
    for side in (-1, 1):
        ex = (-5.5 if side < 0 else st.w + 5.5)
        ey = 2.6
        z = ground_height(st, ex, ey, height)
        pts = [(ex - 2.0 * side, ey + 1.6), (ex + 1.4 * side, ey + 1.6), (ex + 1.4 * side, ey - 1.2)]
        for p0, p1 in zip(pts, pts[1:]):
            wall_run(p0, p1, z, 1.0, 0.6, white, cap)
        taken.append((ex, ey, 2.4))
    for (x, y) in st.scatter(70, lambda x, y: 0.4 + 0.6 * smooth(2, 6, st.outside(x, y)), 1.3, taken):
        z = ground_height(st, x, y, height)
        s = st.rng.uniform(0.9, 1.6)
        if st.rng.random() < 0.35:
            tree_pine(st, x, y, z, s * 1.3, leaves[2], trunk)
        else:
            tree_round(st, x, y, z, s, leaves[st.rng.integers(0, 2)], trunk)
    for (x, y) in st.scatter(26, lambda x, y: 1.0, 0.6, taken):
        rock(st, x, y, ground_height(st, x, y, height), st.rng.uniform(0.35, 0.9), stone)
    for (x, y) in st.scatter(30, lambda x, y: 1.0, 0.5, taken, margin=0.3):
        bush(st, x, y, ground_height(st, x, y, height), st.rng.uniform(0.6, 1.0), leaves[1])
    petals = [material(f"flower{i}", c, 0.05) for i, c in enumerate(((0.95, 0.9, 0.3), (0.95, 0.4, 0.5), (0.9, 0.92, 0.95)))]
    for (x, y) in st.near_border(45, 0.35, taken):
        z = ground_height(st, x, y, height)
        if st.rng.random() < 0.5:
            rock(st, x, y, z - 0.05, st.rng.uniform(0.12, 0.25), stone)
        else:
            for k in range(3):
                sphere((x + st.rng.uniform(-0.12, 0.12), y + st.rng.uniform(-0.12, 0.12), z + 0.05), 0.05,
                       petals[st.rng.integers(0, 3)], subdiv=1)
    return ground, img


def lab(st, x, front, height_fn, taken, s=1.0):
    """Dr's lab from the intro: a long white building with a glass dome in front, the dome's front edge at y = front."""
    y = front + 3.8 * s
    z = ground_height(st, x, y, height_fn)
    white = material("lab_white", (0.82, 0.83, 0.85), 0.04, 6.0)
    grey = material("lab_grey", (0.6, 0.62, 0.66), 0.05, 6.0)
    blue = material("lab_blue", (0.1, 0.3, 0.85), 0.1, 4.0)
    red = material("lab_red", (0.92, 0.36, 0.28), 0.05)
    glass = material("lab_glass", (0.72, 0.9, 0.95), 0.05, 2.0)
    dark = material("lab_dark", (0.15, 0.16, 0.2), 0.05)
    width, depth = 11.0 * s, 2.6 * s
    box((x, y, z - 0.3), (width, depth, 1.9 * s + 0.3), white, bevel=0.05)
    box((x - 1.2 * s, y + 0.3 * s, z + 1.9 * s), (7.6 * s, 2.0 * s, 0.6 * s), grey, bevel=0.05)
    # vents and panels on the front
    for i in range(6):
        px = x - width * 0.42 + i * width * 0.168
        box((px, y - depth * 0.5, z + 1.25 * s), (1.1 * s, 0.08, 0.16 * s), blue)
        box((px, y - depth * 0.5, z + 0.35 * s), (0.5 * s, 0.08, 0.5 * s), dark if i % 2 else grey)
        uv_sphere((px + 0.45 * s, y - depth * 0.5, z + 0.85 * s), 0.1 * s, red, segments=10, rings=6)
        glow((px + 0.45 * s, y - depth * 0.5 - 0.05, z + 0.85 * s), (1.0, 0.5, 0.36), 0.35 * s, GLOW_PULSE)
    for i in range(5):
        box((x - 3.8 * s + i * 1.1 * s, y - 0.7 * s, z + 2.5 * s), (0.7 * s, 0.08, 0.06 * s), dark)
    # the dome with the capsule
    dz = z - 0.1
    dome((x, y - depth * 0.5 - 0.2 * s, dz), 2.3 * s, glass)
    box((x, y - depth * 0.5 - 0.2 * s, dz + 0.9 * s), (3.4 * s, 0.4 * s, 0.22 * s), blue, bevel=0.05)
    taken.append((x, y, width * 0.55))


def world_rocky(st):
    """2: rocky mountains, the cobbled field ringed by a jagged brown ridge, a highland of grey boulders beyond."""
    def edge(x, y):
        # where the ridge starts, it wobbles a little
        return 0.9 + fbm(x, y, 0.5, 3, 13) * 0.9

    def ridge(x, y):
        t = st.outside(x, y) - edge(x, y) - 0.8
        jag = 0.8 + fbm(x, y, 0.9, 4, 14) * 1.4
        return np.exp(-(t / 0.75) ** 2) * jag

    def height(x, y):
        d = st.outside(x, y)
        highland = smooth(0.0, 1.6, d - edge(x, y)) * 0.35
        far = smooth(9, 17, st.north(y)) * 4.5 + smooth(12, 20, d) * 2.0
        bumps = fbm(x, y, 0.6, 3, 12) * 0.5 * smooth(3, 6, d)
        return FLOOR_Z + ridge(x, y) + highland + far * smooth(4, 8, d) + bumps

    def color(TX, TY, TZ, slope):
        d = st.outside(TX, TY)
        f1, f2 = voronoi(TX * 2.4, TY * 2.4, 15)
        crack = smooth(0.12, 0.04, f2 - f1)
        cobble = mix((0.52, 0.52, 0.62), (0.68, 0.68, 0.76), smooth(0.7, 0.0, f1) * 0.6 + fbm(TX, TY, 2.0, 2, 16) * 0.4)
        cobble = mix(cobble, (0.3, 0.3, 0.38), crack)
        strata = np.sin(TZ * 9 + fbm(TX, TY, 0.6, 3, 17) * 4) * 0.5 + 0.5
        brown = mix((0.34, 0.16, 0.07), (0.62, 0.34, 0.15), strata)
        highland = mix((0.5, 0.48, 0.44), (0.64, 0.62, 0.58), fbm(TX, TY, 0.5, 4, 18))
        highland = mix(highland, (0.32, 0.5, 0.2), smooth(0.55, 0.75, fbm(TX, TY, 0.25, 3, 19)) * 0.7)
        mountain = mix((0.46, 0.46, 0.52), (0.62, 0.62, 0.68), fbm(TX, TY, 0.4, 4, 20))
        rgb = mix(highland, mountain, smooth(1.5, 3.0, TZ - FLOOR_Z))
        rgb = mix(rgb, brown, smooth(0.25, 0.5, ridge(TX, TY)))
        rgb = mix(cobble, rgb, smooth(0.0, 0.25, d - edge(TX, TY)))
        return apron_mix(st, TX, TY, rgb, cobble, reach=0.4, ragged=0.3)

    ground, img = terrain(st, height, color, "ground")
    grey = material("rock_grey", (0.6, 0.6, 0.64), 0.25, 1.5)
    grey2 = material("rock_light", (0.74, 0.74, 0.78), 0.2, 1.5)
    brown = material("rock_brown", (0.5, 0.3, 0.16), 0.3, 1.5)
    bush_mat = material("bush", (0.18, 0.5, 0.14), 0.2, 4.0)
    taken = []
    # boulder piles like the one on the map
    for (x, y) in st.scatter(12, lambda x, y: smooth(3.5, 5, st.outside(x, y)), 3.0, taken):
        z = ground_height(st, x, y, height)
        for i in range(st.rng.integers(4, 8)):
            a, r = st.rng.uniform(0, 6.3), st.rng.uniform(0, 1.1)
            rock(st, x + math.cos(a) * r, y + math.sin(a) * r, z + st.rng.uniform(0, 0.4), st.rng.uniform(0.7, 1.5),
                 grey if i % 2 else grey2, flat=0.8)
    for (x, y) in st.scatter(110, lambda x, y: float(st.outside(x, y) > edge(x, y) + 1.6), 0.7, taken, margin=0.5):
        rock(st, x, y, ground_height(st, x, y, height), st.rng.uniform(0.4, 1.1), grey if st.rng.random() < 0.6 else grey2,
             flat=0.75)
    # crags on the cliff rim
    for (x, y) in st.scatter(60, lambda x, y: float(abs(st.outside(x, y) - edge(x, y) - 0.8) < 0.35), 0.6, taken,
                             margin=1.0):
        z = ground_height(st, x, y, height)
        cone((x, y, z - 0.5), st.rng.uniform(0.3, 0.5), st.rng.uniform(0.9, 1.5), brown, vertices=5).rotation_euler.z = \
            st.rng.uniform(0, 6)
    for (x, y) in st.scatter(30, lambda x, y: float(st.outside(x, y) > edge(x, y) + 1), 0.6, taken, margin=0.5):
        bush(st, x, y, ground_height(st, x, y, height), st.rng.uniform(0.6, 1.0), bush_mat)
    for (x, y) in st.near_border(40, 0.4, taken, depth=1.0):
        rock(st, x, y, ground_height(st, x, y, height) - 0.05, st.rng.uniform(0.15, 0.3), grey if st.rng.random() < 0.5 else grey2)
    return ground, img


def dead_tree(st, x, y, z, s, wood):
    start = len(PROPS)
    cylinder((x, y, z - 0.3), 0.12 * s, 1.5 * s, wood, vertices=7)
    for i in range(3):
        a = st.rng.uniform(0, 6.3)
        obj = cylinder((x, y, z + (0.7 + i * 0.25) * s), 0.05 * s, 0.6 * s, wood, vertices=5)
        obj.rotation_euler = (0.8, 0, a)
    fuse(start, 0.025 * s, 260)


def world_river(st):
    """3: crossing the great river: the field flooded like the river around it, one water surface over both,
    its walled island just under the surface among boulders and dead trees, woods on the far bank."""
    # just above the field's floor, the blocks stand ankle deep
    water_z = 0.03

    def bank(x, y):
        # the north bank's line, south of it everything is river
        return 6.5 + np.sin(x * 0.25) * 1.2 + fbm(x, y, 0.2, 3, 22) * 1.5

    def height(x, y):
        d = st.outside(x, y)
        island = smooth(1.4, 1.0, d)
        land = smooth(-0.4, 0.6, y - bank(x, y)) * (0.95 + fbm(x, y, 0.1, 4, 21) * 1.6 + smooth(10, 16, st.north(y)) * 1.5)
        bed = water_z - 0.25 - 0.55 * fbm(x, y, 0.15, 3, 25) * smooth(0.0, 1.5, d - 1.4)
        return FLOOR_Z * island + bed * (1 - island) + land

    def color(TX, TY, TZ, slope):
        grass = grass_colors(TX, TY, (0.12, 0.4, 0.1), (0.3, 0.6, 0.16), seed=4)
        sand = mix((0.62, 0.56, 0.4), (0.74, 0.68, 0.5), fbm(TX, TY, 1.5, 3, 26))
        # the riverbed under the water surface: pebbles and weed, darker the deeper it lies
        bed = pebbles(TX, TY, sand * 0.8, (0.5, 0.48, 0.44), 2.5, 27)
        bed = mix(bed, (0.24, 0.34, 0.16), smooth(0.55, 0.75, fbm(TX, TY, 0.5, 3, 28)) * 0.6)
        bed = mix(bed, (0.1, 0.17, 0.2), smooth(0.0, 0.8, water_z - TZ))
        shore = TZ - water_z
        rgb = mix(bed, sand, smooth(-0.05, 0.05, shore))
        rgb = mix(rgb, grass, smooth(0.15, 0.35, shore))
        f1, f2 = voronoi(TX * 1.6, TY * 1.6, 92)
        flags = mix((0.55, 0.55, 0.58), (0.7, 0.7, 0.72), _hash(np.floor(TX * 1.6), np.floor(TY * 1.6), 93))
        flags = mix(flags, grass, smooth(0.1, 0.03, f2 - f1))
        return apron_mix(st, TX, TY, rgb, flags, reach=0.35, ragged=0.4)

    ground, img = terrain(st, height, color, "ground")
    water_surface(st, height, water_z, (0.04, 0.27, 0.5), field_depth=0.6)
    leaves = [material(f"leaves{i}", c) for i, c in enumerate(((0.14, 0.45, 0.1), (0.2, 0.55, 0.14), (0.08, 0.32, 0.1)))]
    trunk = material("trunk", (0.35, 0.22, 0.12))
    dead = material("dead_wood", (0.45, 0.3, 0.18), 0.2)
    stone = material("island_wall", (0.62, 0.62, 0.66), 0.1, 4.0, kind="bricks", mortar=(0.36, 0.36, 0.4))
    boulder = material("boulder", (0.6, 0.6, 0.64), 0.25, 2.0)
    reed = material("reed", (0.42, 0.55, 0.18))
    # the island's retaining wall
    x0, x1, y0, y1 = -APRON - 1.2, st.w + APRON + 1.2, -st.h - APRON - 1.2, APRON + 1.2
    for p0, p1 in (((x0, y1), (x1, y1)), ((x0, y0), (x1, y0)), ((x0, y0), (x0, y1)), ((x1, y0), (x1, y1))):
        wall_run(p0, p1, water_z - 0.6, 0.57, 0.5, stone)
    taken = []
    in_water = lambda x, y: float(ground_height(st, x, y, height) <= water_z + 0.01)
    for (x, y) in st.scatter(30, in_water, 1.4, taken, margin=1.8):
        dead_tree(st, x, y, water_z, st.rng.uniform(1.2, 1.8), dead)
    for (x, y) in st.scatter(30, in_water, 1.0, taken, margin=1.6):
        rock(st, x, y, water_z - 0.1, st.rng.uniform(0.4, 1.0), boulder, flat=0.7)
    on_bank = lambda x, y: float(ground_height(st, x, y, height) > water_z + 0.3)
    for (x, y) in st.scatter(70, on_bank, 1.2, taken):
        z = ground_height(st, x, y, height)
        if st.rng.random() < 0.4:
            tree_pine(st, x, y, z, st.rng.uniform(1.1, 1.7), leaves[2], trunk)
        else:
            tree_round(st, x, y, z, st.rng.uniform(1.0, 1.5), leaves[st.rng.integers(0, 2)], trunk)
    shore = lambda x, y: float(water_z + 0.02 < ground_height(st, x, y, height) < water_z + 0.25)
    for (x, y) in st.scatter(40, shore, 0.4, taken, margin=1.6):
        for i in range(4):
            cone((x + st.rng.uniform(-0.15, 0.15), y + st.rng.uniform(-0.15, 0.15), water_z - 0.05), 0.04,
                 st.rng.uniform(0.4, 0.7), reed, vertices=5)
    for (x, y) in st.near_border(30, 0.45, taken, depth=0.9):
        z = ground_height(st, x, y, height)
        for k in range(3):
            cone((x + st.rng.uniform(-0.1, 0.1), y + st.rng.uniform(-0.1, 0.1), z - 0.02), 0.035, st.rng.uniform(0.15, 0.25),
                 reed, vertices=5)
    return ground, img


def world_forest(st):
    """4: deep forest, dense pines and broadleaves, mushrooms and fallen logs."""
    def height(x, y):
        d = st.outside(x, y)
        return FLOOR_Z + smooth(0.8, 6, d) * (fbm(x, y, 0.12, 4, 31) * 2.0 + smooth(6, 16, st.north(y)) * 1.5)

    def color(TX, TY, TZ, slope):
        rgb = grass_colors(TX, TY, (0.08, 0.26, 0.06), (0.22, 0.44, 0.12), seed=6)
        leaf_litter = smooth(0.55, 0.75, fbm(TX, TY, 0.7, 3, 7))
        rgb = mix(rgb, (0.38, 0.28, 0.12), leaf_litter * 0.6)
        dirt = mix((0.3, 0.22, 0.12), (0.42, 0.32, 0.18), fbm(TX, TY, 1.5, 4, 94))
        dirt = mix(dirt, (0.2, 0.36, 0.1), smooth(0.6, 0.75, fbm(TX, TY, 1.2, 3, 95)) * 0.7)
        dirt = pebbles(TX, TY, dirt, (0.4, 0.38, 0.34), 2.5)
        return apron_mix(st, TX, TY, rgb, dirt)

    ground, img = terrain(st, height, color, "ground")
    pines = [material(f"pine{i}", c) for i, c in enumerate(((0.06, 0.26, 0.1), (0.1, 0.34, 0.12)))]
    leaves = material("leaves_dark", (0.12, 0.4, 0.1))
    trunk = material("trunk", (0.32, 0.2, 0.1))
    cap = material("mushroom_cap", (0.85, 0.12, 0.08), 0.08)
    stem = material("mushroom_stem", (0.92, 0.88, 0.78), 0.04)
    log = material("log", (0.4, 0.26, 0.14))
    taken = []
    for (x, y) in st.scatter(160, lambda x, y: 0.5 + 0.5 * smooth(1, 4, st.outside(x, y)), 1.0, taken):
        z = ground_height(st, x, y, height)
        if st.rng.random() < 0.65:
            tree_pine(st, x, y, z, st.rng.uniform(1.2, 2.2), pines[st.rng.integers(0, 2)], trunk)
        else:
            tree_round(st, x, y, z, st.rng.uniform(1.1, 1.7), leaves, trunk)
    for (x, y) in st.scatter(16, lambda x, y: 1.0, 0.5, taken, margin=0.3):
        z = ground_height(st, x, y, height)
        s = st.rng.uniform(0.5, 1.0)
        cylinder((x, y, z - 0.05), 0.07 * s, 0.32 * s, stem, vertices=8)
        sphere((x, y, z + 0.3 * s), 0.22 * s, cap, scale=(1, 1, 0.55))
    for (x, y) in st.scatter(8, lambda x, y: 1.0, 1.2, taken, margin=0.5):
        z = ground_height(st, x, y, height)
        obj = cylinder((x, y, z), 0.16, 1.4, log, vertices=10)
        obj.rotation_euler = (math.pi / 2, 0, st.rng.uniform(0, 3.1))
        obj.location.z = z + 0.12
    for (x, y) in st.scatter(40, lambda x, y: 1.0, 0.45, taken, margin=0.3):
        bush(st, x, y, ground_height(st, x, y, height), st.rng.uniform(0.5, 0.9), leaves)
    for (x, y) in st.scatter(18, lambda x, y: 1.0, 1.5, [], margin=0.5):
        glow((x, y, ground_height(st, x, y, height) + st.rng.uniform(0.4, 1.4)), (0.7, 1.0, 0.3), 0.18, GLOW_PULSE)
    for (x, y) in st.near_border(40, 0.4, taken):
        z = ground_height(st, x, y, height)
        if st.rng.random() < 0.4:
            s = st.rng.uniform(0.35, 0.55)
            cylinder((x, y, z - 0.05), 0.07 * s, 0.32 * s, stem, vertices=8)
            sphere((x, y, z + 0.3 * s), 0.22 * s, cap, scale=(1, 1, 0.55))
        else:
            bush(st, x, y, z - 0.05, st.rng.uniform(0.3, 0.45), leaves)
    return ground, img


def cream_rock(st, x, y, z, s, rock_mat, cream_mat):
    """A brown rock with a cream cap, like the stage's pillars and the mountain on the map."""
    cylinder((x, y, z - 0.2), 0.42 * s, 0.75 * s, rock_mat, vertices=10, scale=(1, 1, 1)).rotation_euler.z = st.rng.uniform(0, 6)
    start = len(PROPS)
    sphere((x, y, z + 0.6 * s), 0.5 * s, cream_mat, scale=(1.0, 1.0, 0.55), jitter=0.08, rng=st.rng)
    for i in range(4):
        a = 2 * math.pi * i / 4 + st.rng.uniform(0, 1)
        sphere((x + math.cos(a) * 0.36 * s, y + math.sin(a) * 0.36 * s, z + 0.42 * s), 0.13 * s, cream_mat,
               scale=(1, 1, 1.5), subdiv=1)
    fuse(start, 0.03 * s, 300, smooth_steps=2)


def world_lava_cave(st):
    """5: the cold cave of volcanic rock under the cream capped mountain: brown earth, cream topped pillars, moss."""
    mx, my = st.cx, 11.0

    def height(x, y):
        d = st.outside(x, y)
        walls = smooth(5, 9, d) * (2.0 + fbm(x, y, 0.2, 4, 41) * 2.5)
        r = np.hypot((x - mx) * 0.8, y - my)
        mountain = np.clip(7.0 - r * 1.1, 0, None)
        return FLOOR_Z + smooth(0.6, 4, d) * fbm(x, y, 0.25, 3, 42) * 0.6 + walls + mountain

    def color(TX, TY, TZ, slope):
        earth = mix((0.42, 0.26, 0.12), (0.58, 0.38, 0.18), fbm(TX, TY, 0.6, 4, 44))
        crack = smooth(0.04, 0.0, np.abs(fbm(TX, TY, 0.35, 3, 45) - 0.5))
        earth = mix(earth, (0.22, 0.12, 0.06), crack * 0.8)
        moss = smooth(0.62, 0.78, fbm(TX, TY, 0.4, 3, 46))
        earth = mix(earth, (0.3, 0.5, 0.2), moss * 0.8)
        rock_c = mix((0.36, 0.2, 0.1), (0.5, 0.3, 0.16), np.sin(TZ * 6 + fbm(TX, TY, 0.5, 3, 47) * 3) * 0.5 + 0.5)
        rgb = mix(earth, rock_c, smooth(0.4, 1.0, slope))
        # cream crust on the heights and the mountain's top
        cream = mix((0.92, 0.84, 0.62), (1.0, 0.96, 0.84), fbm(TX, TY, 1.0, 3, 48))
        rgb = mix(rgb, cream, smooth(3.2, 3.8, TZ + fbm(TX, TY, 0.5, 3, 49) * 1.2))
        trodden = pebbles(TX, TY, earth * 0.85, (0.5, 0.36, 0.22), 3.0)
        return apron_mix(st, TX, TY, rgb, trodden)

    ground, img = terrain(st, height, color, "ground")
    rock_mat = material("cave_rock", (0.46, 0.28, 0.14), 0.25, 2.5)
    cream = material("cream", (0.96, 0.9, 0.72), 0.06, 4.0)
    moss = material("moss", (0.3, 0.52, 0.2), 0.2, 4.0)
    taken = [(mx, my, 6.5)]
    for (x, y) in st.scatter(36, lambda x, y: float(st.outside(x, y) < 7), 1.3, taken):
        cream_rock(st, x, y, ground_height(st, x, y, height), st.rng.uniform(0.7, 1.3), rock_mat, cream)
    for (x, y) in st.scatter(30, lambda x, y: 1.0, 0.7, taken, margin=0.4):
        rock(st, x, y, ground_height(st, x, y, height), st.rng.uniform(0.3, 0.8), rock_mat)
    for (x, y) in st.scatter(30, lambda x, y: 1.0, 0.5, taken, margin=0.3):
        bush(st, x, y, ground_height(st, x, y, height), st.rng.uniform(0.5, 0.9), moss)
    for (x, y) in st.near_border(40, 0.4, taken):
        z = ground_height(st, x, y, height)
        if st.rng.random() < 0.5:
            rock(st, x, y, z - 0.05, st.rng.uniform(0.15, 0.3), rock_mat)
        else:
            bush(st, x, y, z - 0.05, st.rng.uniform(0.3, 0.45), moss)
    return ground, img


# --- the mechanical castle ------------------------------------------------------------------------

def diamond_floor(st, TX, TY, light, dark, line, size=1.0):
    u, v = (TX + TY) / size, (TX - TY) / size
    fu, fv = u - np.floor(u), v - np.floor(v)
    checker = ((np.floor(u) + np.floor(v)) % 2)
    rgb = mix(dark, light, checker * (0.8 + 0.2 * fbm(TX, TY, 1.0, 3, 5)))
    edge = np.minimum(np.minimum(fu, 1 - fu), np.minimum(fv, 1 - fv))
    return mix(rgb, line, smooth(0.06, 0.02, edge))


def room(st, wall_mat, offset_x=4.0, offset_north=3.6, height=4.5):
    """The hall around the field: walls left, right and behind (the camera looks in from the south)."""
    x0, x1 = -APRON - offset_x, st.w + APRON + offset_x
    y0, y1 = -st.h - APRON - 6.0, APRON + offset_north
    z = FLOOR_Z
    wall_run((x0 - 0.5, y1 + 0.5), (x1 + 0.5, y1 + 0.5), z, height, 1.0, wall_mat)
    wall_run((x0 - 0.5, y0), (x0 - 0.5, y1 + 0.5), z, height, 1.0, wall_mat)
    wall_run((x1 + 0.5, y0), (x1 + 0.5, y1 + 0.5), z, height, 1.0, wall_mat)
    return x0, x1, y0, y1


def along_walls(x0, x1, y0, y1, spacing, skip_x=None):
    """Points along the inside of the north, west and east walls with the direction they face."""
    for x in np.arange(x0 + spacing * 0.5, x1, spacing):
        if skip_x is None or abs(x - skip_x) > spacing * 0.6:
            yield x, y1, -math.pi / 2
    for y in np.arange(y1 - spacing * 0.5, y0, -spacing):
        yield x0, y, 0.0
        yield x1, y, math.pi


def wall_lamp(x, y, angle, z, metal, glass):
    dx, dy = math.cos(angle), math.sin(angle)
    box((x + dx * 0.1, y + dy * 0.1, z), (0.25, 0.25, 0.3), metal, rotation=angle)
    uv_sphere((x + dx * 0.28, y + dy * 0.28, z + 0.15), 0.16, glass, segments=12, rings=8)
    glow((x + dx * 0.32, y + dy * 0.32, z + 0.15), (1.0, 0.52, 0.34), 1.0, GLOW_PULSE)


def pyramid_block(x, y, z, s, mat):
    box((x, y, z), (s, s, s * 0.35), mat, bevel=0.02)
    cone((x, y, z + s * 0.35), s * 0.7, s * 0.4, mat, vertices=4).rotation_euler.z = math.pi / 4


# the scenery's animation loop, every spinning part makes whole turns in it
LOOP_FRAMES = 96
LOOP_FPS = 24
SPINNERS = []


def spinner(start, place, turns, phase=0.0):
    """The props made since start turn about place's z axis, turns whole revolutions per loop; they are baked
    with everything else and split off again afterwards (see separate_spinners())."""
    spin_id = len(SPINNERS) + 1
    for part in PROPS[start:]:
        attribute = part.data.attributes.new("spin", "INT", "FACE")
        attribute.data.foreach_set("value", [spin_id] * len(part.data.polygons))
    SPINNERS.append((spin_id, place, turns, phase))


def gear(x, y, z, radius, mat, angle, teeth=10, depth=0.2, turns=0, phase=0.0):
    """Upright against a wall, facing angle; turning when turns isn't 0."""
    start = len(PROPS)
    cylinder((0, 0, 0), radius, depth, mat, vertices=24)
    for i in range(teeth):
        a = 2 * math.pi * i / teeth
        box((math.cos(a) * radius, math.sin(a) * radius, 0), (radius * 0.3, radius * 0.25, depth), mat, rotation=a)
    cylinder((0, 0, depth), radius * 0.25, depth * 0.6, mat, vertices=12)
    bpy.context.view_layer.update()
    place = Matrix.Translation((x, y, z)) @ Matrix.Rotation(angle, 4, "Z") @ Matrix.Rotation(math.pi / 2, 4, "Y")
    for part in PROPS[start:]:
        part.matrix_world = place @ part.matrix_world
    if turns:
        spinner(start, place, turns, phase)


def gear_train(x, y, z, angle, metal, dark, rubber):
    """Two meshing gears on the wall, a belt from the big one's pulley down to a motor on the floor."""
    dx, dy = math.cos(angle), math.sin(angle)
    tx, ty = -dy, dx
    big, small = 0.9, 0.6
    height = z + 2.3
    gx, gy = x + dx * 0.15 - tx * 0.5, y + dy * 0.15 - ty * 0.5
    # 12 and 8 teeth: 2 and 3 turns per loop, the small one starts half a tooth on
    gear(gx, gy, height, big, metal, angle, teeth=12, turns=-2)
    distance = big + small + 0.03
    gear(gx + tx * distance, gy + ty * distance, height, small, metal, angle, teeth=8, turns=3, phase=math.pi / 8)
    # the pulley in front of the big gear, the motor's pulley below it, the belt between them
    pulley = 0.22
    px, py = gx + dx * 0.32, gy + dy * 0.32
    motor_height = z + 0.85
    gear(px, py, height, pulley, dark, angle, teeth=0, depth=0.12, turns=-2)
    box((gx + dx * 0.45, gy + dy * 0.45, z), (0.75, 0.7, 0.6), dark, rotation=angle, bevel=0.04)
    gear(px, py, motor_height, pulley, dark, angle, teeth=0, depth=0.12, turns=-2)
    for side in (-1, 1):
        box((px + dx * 0.06 + tx * pulley * side, py + dy * 0.06 + ty * pulley * side, motor_height),
            (0.08, 0.06, height - motor_height), rubber, rotation=angle)


def merge_animations(path, name):
    """The exporter writes an animation per spinner, the game plays one: all channels into a single animation."""
    import json
    import struct
    data = open(path, "rb").read()
    length = struct.unpack("<I", data[12:16])[0]
    gltf = json.loads(data[20:20 + length])
    rest = data[20 + length:]
    merged = {"name": name, "channels": [], "samplers": []}
    for animation in gltf.get("animations", []):
        offset = len(merged["samplers"])
        merged["samplers"] += animation["samplers"]
        for channel in animation["channels"]:
            merged["channels"].append(dict(channel, sampler=channel["sampler"] + offset))
    gltf["animations"] = [merged]
    text = json.dumps(gltf, separators=(",", ":")).encode()
    text += b" " * (-len(text) % 4)
    chunk = struct.pack("<I4s", len(text), b"JSON") + text
    total = 12 + len(chunk) + len(rest)
    open(path, "wb").write(struct.pack("<4sII", b"glTF", 2, total) + chunk + rest)


def separate_spinners(props):
    """Splits the spinning parts off the baked props: a pivot placed like the part, a spinner under it turning
    about its z axis over the loop, the part's mesh under that."""
    if not SPINNERS:
        return
    scene = bpy.context.scene
    scene.render.fps = LOOP_FPS
    scene.frame_start, scene.frame_end = 0, LOOP_FRAMES
    bpy.context.preferences.edit.keyframe_new_interpolation_type = "LINEAR"
    for spin_id, place, turns, phase in SPINNERS:
        bpy.ops.object.select_all(action="DESELECT")
        props.select_set(True)
        bpy.context.view_layer.objects.active = props
        bpy.ops.object.mode_set(mode="EDIT")
        import bmesh
        bm = bmesh.from_edit_mesh(props.data)
        layer = bm.faces.layers.int.get("spin")
        found = 0
        for face in bm.faces:
            face.select_set(face[layer] == spin_id)
            found += face[layer] == spin_id
        bmesh.update_edit_mesh(props.data)
        if found:
            bpy.ops.mesh.separate(type="SELECTED")
        bpy.ops.object.mode_set(mode="OBJECT")
        if not found:
            continue
        part = next(o for o in bpy.context.selected_objects if o != props)
        pivot = bpy.data.objects.new("pivot", None)
        scene.collection.objects.link(pivot)
        pivot.matrix_world = place
        turning = bpy.data.objects.new("spinner", None)
        scene.collection.objects.link(turning)
        turning.parent = pivot
        part.parent = turning
        part.matrix_parent_inverse = place.inverted()
        turning.rotation_euler = (0.0, 0.0, phase)
        turning.keyframe_insert("rotation_euler", index=2, frame=0)
        turning.rotation_euler = (0.0, 0.0, phase + 2 * math.pi * turns)
        turning.keyframe_insert("rotation_euler", index=2, frame=LOOP_FRAMES)


def pipe_run(p0, p1, z, radius, mat):
    (x0, y0), (x1, y1) = p0, p1
    length = math.hypot(x1 - x0, y1 - y0)
    obj = cylinder(((x0 + x1) / 2, (y0 + y1) / 2, z - length / 2), radius, length, mat, vertices=12)
    obj.rotation_euler = (math.pi / 2, 0, math.atan2(y1 - y0, x1 - x0) + math.pi / 2)
    obj.location.z = z
    for (px, py) in (p0, p1):
        uv_sphere((px, py, z), radius * 1.15, mat, segments=12, rings=8)


def world_castle_1(st):
    """6: the mechanical castle's first floor: purple diamond floor, brick walls with red lamps, metal studs."""
    def height(x, y):
        return np.full_like(x, FLOOR_Z, dtype=float)

    def color(TX, TY, TZ, slope):
        rgb = diamond_floor(st, TX, TY, (0.62, 0.42, 0.62), (0.46, 0.28, 0.48), (0.3, 0.16, 0.3))
        return apron_mix(st, TX, TY, rgb, rgb)

    ground, img = terrain(st, height, color, "ground")
    brick = material("castle_brick", (0.55, 0.28, 0.18), 0.12, 3.0, kind="bricks", mortar=(0.25, 0.12, 0.08))
    metal = material("metal", (0.66, 0.68, 0.76), 0.08, 5.0)
    lamp = material("lamp_red", (0.96, 0.4, 0.3), 0.1)
    olive = material("olive_brick", (0.56, 0.54, 0.34), 0.1, 4.0, kind="bricks", mortar=(0.3, 0.28, 0.16))
    dark = material("dark_metal", (0.25, 0.26, 0.3), 0.08, 5.0)
    x0, x1, y0, y1 = room(st, brick)
    z = FLOOR_Z
    for (x, y, a) in along_walls(x0, x1, y0, y1, 2.5):
        wall_lamp(x, y, a, z + 1.6, dark, lamp)
    # pillars of metal studs along the walls
    for (x, y, a) in along_walls(x0 + 1.2, x1 - 1.2, y0, y1 - 1.2, 3.5):
        pyramid_block(x, y, z, 0.8, metal)
    # olive brick stacks and a gate in the north wall
    for (x, y, a) in along_walls(x0 + 2.4, x1 - 2.4, y0, y1 - 2.4, 7.0):
        dx, dy = math.cos(a), math.sin(a)
        for (ox, oy, oz) in ((0, 0, 0), (-1, 1, 0), (-0.5, 0.5, 0.9)):
            box((x + dx * 0.6 + ox * dy, y + dy * 0.6 + oy * dx, z + oz), (0.9, 0.9, 0.9), olive, bevel=0.03)
    box((st.cx, y1 - 0.05, z), (2.2, 0.2, 2.6), dark, bevel=0.04)
    for i in range(4):
        box((st.cx - 0.75 + i * 0.5, y1 - 0.2, z), (0.12, 0.12, 2.5), metal)
    return ground, img


def world_castle_2(st):
    """7: the second floor, the castle's machinery: gears, pipes and pistons along the walls."""
    def height(x, y):
        return np.full_like(x, FLOOR_Z, dtype=float)

    def color(TX, TY, TZ, slope):
        rgb = diamond_floor(st, TX, TY, (0.56, 0.4, 0.58), (0.36, 0.26, 0.42), (0.22, 0.14, 0.26))
        # hazard stripes in front of the walls
        d = st.outside(TX, TY)
        stripe = ((np.floor((TX + TY) * 1.5)) % 2)
        band = smooth(0.45, 0.35, np.abs(d - 3.4))
        rgb = mix(rgb, mix((0.15, 0.12, 0.1), (0.95, 0.75, 0.1), stripe), band * float(1))
        return apron_mix(st, TX, TY, rgb, rgb)

    ground, img = terrain(st, height, color, "ground")
    steel = material("steel_wall", (0.5, 0.52, 0.58), 0.08, 2.0, kind="bricks", mortar=(0.28, 0.3, 0.34))
    brass = material("brass", (0.8, 0.62, 0.26), 0.1, 5.0)
    copper = material("copper", (0.72, 0.38, 0.2), 0.1, 5.0)
    dark = material("dark_metal", (0.22, 0.23, 0.27), 0.08, 5.0)
    lamp = material("lamp_red", (0.96, 0.4, 0.3), 0.1)
    green = material("lamp_green", (0.5, 0.9, 0.66), 0.1)
    rubber = material("belt_rubber", (0.12, 0.11, 0.1), 0.1)
    x0, x1, y0, y1 = room(st, steel, offset_x=4.6, offset_north=4.0)
    z = FLOOR_Z
    i = 0
    for (x, y, a) in along_walls(x0, x1, y0, y1, 3.0):
        i += 1
        dx, dy = math.cos(a), math.sin(a)
        if i % 3 == 0:
            gear_train(x, y, z, a, brass, dark, rubber)
        elif i % 3 == 1:
            # a piston in its housing
            box((x + dx * 0.5, y + dy * 0.5, z), (0.9, 0.9, 0.5), dark, rotation=a, bevel=0.03)
            cylinder((x + dx * 0.5, y + dy * 0.5, z + 0.5), 0.22, 1.6, brass, vertices=14)
            box((x + dx * 0.5, y + dy * 0.5, z + 2.1), (0.8, 0.8, 0.25), dark, rotation=a)
        else:
            box((x + dx * 0.35, y + dy * 0.35, z), (1.4, 0.6, 1.8), dark, rotation=a + math.pi / 2, bevel=0.04)
            for k in range(3):
                uv_sphere((x + dx * 0.66 + dy * (k - 1) * 0.35, y + dy * 0.66 - dx * (k - 1) * 0.35, z + 1.4), 0.08,
                          lamp if k == 1 else green, segments=10, rings=6)
                glow((x + dx * 0.7 + dy * (k - 1) * 0.35, y + dy * 0.7 - dx * (k - 1) * 0.35, z + 1.4),
                     (1.0, 0.48, 0.36) if k == 1 else (0.55, 1.0, 0.72), 0.3, GLOW_BLINK)
    # pipes running along the walls
    for h, r, m in ((0.7, 0.14, copper), (3.4, 0.2, brass)):
        pipe_run((x0 + 0.35, y1 + 0.2), (x1 - 0.35, y1 + 0.2), z + h, r, m)
        pipe_run((x0 + 0.25, y0), (x0 + 0.25, y1 - 0.1), z + h, r, m)
        pipe_run((x1 - 0.25, y0), (x1 - 0.25, y1 - 0.1), z + h, r, m)
    return ground, img


def world_castle_final(st):
    """8: the final floor, the castle's control room: teal floor, computer banks with red lights, screens."""
    def height(x, y):
        return np.full_like(x, FLOOR_Z, dtype=float)

    def color(TX, TY, TZ, slope):
        rgb = diamond_floor(st, TX, TY, (0.12, 0.5, 0.56), (0.06, 0.36, 0.44), (0.04, 0.22, 0.28), size=1.4)
        return apron_mix(st, TX, TY, rgb, rgb)

    ground, img = terrain(st, height, color, "ground")
    wall = material("tech_wall", (0.68, 0.7, 0.74), 0.05, 1.0, kind="bricks", mortar=(0.4, 0.42, 0.46))
    white = material("console_white", (0.88, 0.88, 0.9), 0.04, 5.0)
    grey = material("console_grey", (0.55, 0.57, 0.62), 0.05, 5.0)
    screen = material("screen", (0.2, 0.45, 0.95), 0.25, 6.0, color2=(0.5, 0.8, 1.0))
    red = material("led_red", (0.95, 0.38, 0.3), 0.05)
    yellow = material("vent_yellow", (0.95, 0.78, 0.42), 0.1)
    dark = material("tech_dark", (0.12, 0.13, 0.16), 0.05)
    x0, x1, y0, y1 = room(st, wall, offset_x=4.2, offset_north=3.8)
    z = FLOOR_Z
    i = 0
    for (x, y, a) in along_walls(x0, x1, y0, y1, 1.8, skip_x=st.cx):
        i += 1
        dx, dy = math.cos(a), math.sin(a)
        cx, cy = x + dx * 0.45, y + dy * 0.45
        # a computer like the stage's hard blocks: white case, slanted panel, red light
        box((cx, cy, z), (1.5, 0.8, 1.0), white, rotation=a + math.pi / 2, bevel=0.04)
        box((cx + dx * 0.05, cy + dy * 0.05, z + 1.0), (1.5, 0.6, 0.5), grey, rotation=a + math.pi / 2, bevel=0.03)
        box((cx + dx * 0.36, cy + dy * 0.36, z + 1.05), (1.0, 0.04, 0.35), screen if i % 2 else dark, rotation=a + math.pi / 2)
        box((cx + dx * 0.41, cy + dy * 0.41, z + 0.45), (0.3, 0.04, 0.08), red, rotation=a + math.pi / 2)
        glow((cx + dx * 0.46, cy + dy * 0.46, z + 0.49), (1.0, 0.46, 0.36), 0.3, GLOW_BLINK)
        if i % 2:
            glow((cx + dx * 0.42, cy + dy * 0.42, z + 1.22), (0.45, 0.72, 1.0), 0.55, GLOW_PULSE)
        # vents above
        box((x + dx * 0.08, y + dy * 0.08, z + 2.6), (1.2, 0.06, 0.3), yellow if i % 2 else dark, rotation=a + math.pi / 2)
    # the big screen and the core behind the field
    box((st.cx, y1 + 0.02, z + 1.4), (4.2, 0.12, 2.2), dark)
    box((st.cx, y1 - 0.05, z + 1.55), (3.8, 0.1, 1.9), screen)
    glow((st.cx, y1 - 0.4, z + 2.5), (0.45, 0.72, 1.0), 2.2, GLOW_PULSE)
    for side in (-1, 1):
        cylinder((st.cx + side * 2.8, y1 - 0.6, z), 0.45, 3.2, white, vertices=20)
        cylinder((st.cx + side * 2.8, y1 - 0.6, z + 3.2), 0.5, 0.2, grey, vertices=20)
        for k in range(4):
            box((st.cx + side * 2.8, y1 - 1.05, z + 0.5 + k * 0.6), (0.3, 0.04, 0.12), red if k % 2 else yellow)
            glow((st.cx + side * 2.8, y1 - 1.1, z + 0.56 + k * 0.6), (1.0, 0.46, 0.36) if k % 2 else (1.0, 0.82, 0.48), 0.3,
                 GLOW_BLINK)
    return ground, img


WORLDS = {
    1: ("the_wall", world_meadow),
    2: ("rocky_mountains", world_rocky),
    3: ("river", world_river),
    4: ("forest", world_forest),
    5: ("lava_cave", world_lava_cave),
    6: ("castle_1", world_castle_1),
    7: ("castle_2", world_castle_2),
    8: ("castle_final", world_castle_final),
}


# --- bake and export ------------------------------------------------------------------------------

def setup_cycles():
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    prefs = bpy.context.preferences.addons["cycles"].preferences
    for device_type in ("OPTIX", "CUDA"):
        try:
            prefs.compute_device_type = device_type
            prefs.get_devices()
            if any(d.type == device_type for d in prefs.devices):
                for d in prefs.devices:
                    d.use = d.type == device_type
                scene.cycles.device = "GPU"
                break
        except TypeError:
            continue
    scene.cycles.samples = 48
    # keep what the images hold before a bake, the unbaked texels are filled in afterwards
    scene.render.bake.use_clear = False
    scene.world = scene.world or bpy.data.worlds.new("World")
    scene.world.light_settings.distance = 1.2


def bake_target(obj, img):
    for slot in obj.material_slots:
        nodes = slot.material.node_tree.nodes
        node = nodes.get("bake_target") or nodes.new("ShaderNodeTexImage")
        node.name = "bake_target"
        node.image = img
        nodes.active = node


def bake(obj, kind, img):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bake_target(obj, img)
    if kind == "COLOR":
        bpy.context.scene.cycles.samples = 1
        bpy.ops.object.bake(type="DIFFUSE", pass_filter={"COLOR"}, margin=24)
    else:
        bpy.context.scene.cycles.samples = 48
        bpy.ops.object.bake(type="AO", margin=24)


def gltf_output_group():
    group = bpy.data.node_groups.get("glTF Material Output")
    if group:
        return group
    group = bpy.data.node_groups.new("glTF Material Output", "ShaderNodeTree")
    group.interface.new_socket("Occlusion", in_out="INPUT", socket_type="NodeSocketFloat")
    return group


def export_material(name, color_img, ao_img):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = next(n for n in nodes if n.type == "BSDF_PRINCIPLED")
    bsdf.inputs["Roughness"].default_value = 0.8
    color = nodes.new("ShaderNodeTexImage")
    color.image = color_img
    links.new(color.outputs["Color"], bsdf.inputs["Base Color"])
    ao = nodes.new("ShaderNodeTexImage")
    ao.image = ao_img
    split = nodes.new("ShaderNodeSeparateColor")
    links.new(ao.outputs["Color"], split.inputs["Color"])
    out = nodes.new("ShaderNodeGroup")
    out.node_tree = gltf_output_group()
    links.new(split.outputs[0], out.inputs["Occlusion"])
    return mat


def join_props(name):
    objs = [o for o in PROPS if o.name in bpy.data.objects]
    bpy.ops.object.select_all(action="DESELECT")
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    bpy.ops.object.join()
    obj = bpy.context.object
    obj.name = name
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(angle_limit=1.15, island_margin=0.012, scale_to_bounds=True)
    bpy.ops.object.mode_set(mode="OBJECT")
    return obj


def border_occluders(w, h):
    """The field's floor and border blocks, only for the ground's ambient occlusion."""
    objs = []
    for (x, y, z, sx, sy, sz) in ((w / 2, -h / 2, -0.12, w, h, 0.12), (w / 2, 0.5, 0, w + 2, 1, 0.95),
                                  (w / 2, -h - 0.5, 0, w + 2, 1, 0.95), (-0.5, -h / 2, 0, 1, h, 0.95),
                                  (w + 0.5, -h / 2, 0, 1, h, 0.95)):
        bpy.ops.mesh.primitive_cube_add(size=1, location=(x, y, z + sz / 2))
        obj = bpy.context.object
        obj.scale = (sx, sy, sz)
        obj.name = "occluder"
        objs.append(obj)
    return objs


def build(world, size_key):
    title, fn = WORLDS[world]
    w, h = SIZES[size_key]
    bpy.ops.wm.read_factory_settings(use_empty=True)
    _MATERIALS.clear()
    PROPS.clear()
    WATER.clear()
    SPINNERS.clear()
    setup_cycles()
    st = Stage(w, h, seed=world * 100 + w + h)
    ground, ground_color = fn(st)

    occluders = border_occluders(w, h)
    for water, _ in WATER:
        water.hide_render = True
    props = join_props("props")
    props_color = bpy.data.images.new("props_color", PROP_TEX, PROP_TEX)
    props_ao = bpy.data.images.new("props_ao", AO_TEX, AO_TEX)
    ground_ao = bpy.data.images.new("ground_ao", AO_TEX, AO_TEX)
    # before baking: changing it later regenerates the image and loses the bake;
    # white where nothing is baked so filtering doesn't darken the islands' edges
    for img in (props_ao, ground_ao):
        img.colorspace_settings.name = "Non-Color"
        img.generated_color = (1, 1, 1, 1)
    # unbaked texels get the average colour afterwards: black there bleeds into small islands in the mipmaps
    marker = np.tile(np.array([1.0, 0.0, 1.0, 1.0], np.float32), PROP_TEX * PROP_TEX)
    props_color.pixels.foreach_set(marker)
    bake(props, "COLOR", props_color)
    texels = np.empty(PROP_TEX * PROP_TEX * 4, np.float32)
    props_color.pixels.foreach_get(texels)
    texels = texels.reshape(-1, 4)
    unbaked = (texels[:, 0] > 0.99) & (texels[:, 1] < 0.01) & (texels[:, 2] > 0.99)
    texels[unbaked, :3] = texels[~unbaked, :3].mean(axis=0)
    props_color.pixels.foreach_set(texels.ravel())
    # the props' detail from their colours, as for the ground but with nothing steep
    half = PROP_TEX // 2
    rgb = texels.reshape(PROP_TEX, PROP_TEX, 4)[..., :3]
    PROPS_DETAIL[:] = [detail.ground_mask(rgb, np.zeros(rgb.shape[:2])).reshape(half, 2, half, 2, 4).mean(axis=(1, 3))]
    props_ao.pixels.foreach_set(np.tile(np.array([1.0, 0.0, 1.0, 1.0], np.float32), AO_TEX * AO_TEX))
    bake(props, "AO", props_ao)
    bake(ground, "AO", ground_ao)
    soft = np.empty(AO_TEX * AO_TEX * 4, np.float32)
    props_ao.pixels.foreach_get(soft)
    soft = soft.reshape(-1, 4)
    soft[(soft[:, 0] > 0.99) & (soft[:, 1] < 0.01) & (soft[:, 2] > 0.99), :3] = 1.0
    soft[:, :3] = 0.4 + 0.6 * soft[:, :3]
    props_ao.pixels.foreach_set(soft.ravel())
    for obj in occluders:
        bpy.data.objects.remove(obj, do_unlink=True)
    for water, _ in WATER:
        water.hide_render = False
    ao = np.empty(AO_TEX * AO_TEX * 4, np.float32)
    ground_ao.pixels.foreach_get(ao)
    ao = ao.reshape(AO_TEX, AO_TEX, 4)
    u = (np.arange(AO_TEX) + 0.5) / AO_TEX
    TX, TY = np.meshgrid(st.x0 + u * (st.x1 - st.x0), st.y0 + u * (st.y1 - st.y0))
    value = np.where(st.outside(TX, TY, 1.0) <= 0, np.maximum(ao[..., 0], 0.7), 0.3 + 0.7 * ao[..., 0] ** 1.6)
    ao[..., :3] = value[..., None]
    ground_ao.pixels.foreach_set(ao.ravel())

    for obj, color_img, ao_img in ((ground, ground_color, ground_ao), (props, props_color, props_ao)):
        obj.data.materials.clear()
        obj.data.materials.append(export_material(f"{obj.name}_export", color_img, ao_img))
    separate_spinners(props)

    tris = sum(len(p.vertices) - 2 for o in (ground, props) for p in o.data.polygons)
    folder = os.path.join(OUT, f"world{world}")
    os.makedirs(folder, exist_ok=True)
    path = os.path.join(folder, f"surroundings_{w}x{h}.glb")
    bpy.ops.object.select_all(action="SELECT")
    # the spinners' turns as one animation of the whole scene
    animation = dict(export_animations=bool(SPINNERS))
    if SPINNERS:
        modes = [m.identifier for m in bpy.ops.export_scene.gltf.get_rna_type().properties["export_animation_mode"].enum_items]
        animation["export_animation_mode"] = "SCENE" if "SCENE" in modes else modes[0]
    bpy.ops.export_scene.gltf(filepath=path, export_format="GLB", use_selection=True, export_yup=True,
                              export_image_format="JPEG", export_jpeg_quality=88, export_extras=True, **animation)
    if SPINNERS:
        merge_animations(path, "spin")
    detail.embed(path, {"ground_export": GROUND_DETAIL[0], "props_export": PROPS_DETAIL[0]}, DETAIL_IMAGES)
    for img in (ground_color, props_color, props_ao, ground_ao, *(img for _, img in WATER)):
        img.pack()
    bpy.context.scene.name = f"world{world}_{title}_{w}x{h}"
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(folder, f"surroundings_{w}x{h}.blend"), compress=True)
    print("RESULT", path, "tris", tris, "kb", os.path.getsize(path) // 1024, flush=True)


for world in (ONLY or sorted(WORLDS)):
    for size_key in [k for k in SIZES if not os.environ.get("SURR_SIZES") or str(k) in os.environ["SURR_SIZES"].split(",")]:
        build(world, size_key)
