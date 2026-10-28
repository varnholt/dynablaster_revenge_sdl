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
from mathutils import Matrix, Quaternion, Vector

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
import budget  # noqa: E402
import detail  # noqa: E402
import glb  # noqa: E402
import grass  # noqa: E402

ARGS = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
OUT = ARGS[0] if ARGS else os.path.join(os.path.dirname(__file__), "surroundings")
ONLY = [int(a) for a in ARGS[1:]]

# field sizes: single screen, horizontal and vertical scrolling (shared/storystage.h)
SIZES = {None: (13, 11), "H": (29, 11), "V": (13, 23)}
GROUND_TEX = 2048
PROP_TEX = 2048
PROP_MARGIN = 4  # texels between the props' uv islands
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
    """Procedural colour (baked later): noise, bricks or plates (riveted panels, bricks without the offset) on the
    world position."""
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
    if kind in ("bricks", "plates"):
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
        brick.inputs["Mortar Size"].default_value = 0.025 if kind == "bricks" else 0.012
        if kind == "plates":
            brick.offset = 0.0
            brick.inputs["Brick Width"].default_value = 1.0
            brick.inputs["Row Height"].default_value = 0.6
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


def box(loc, size, mat, rotation=0.0, bevel=None):
    """Standing on loc. Edges are rounded a little unless bevel says otherwise (0: sharp); a hard cube edge
    catches no light and looks like an unfinished model."""
    if bevel is None:
        bevel = min(0.035, min(size) * 0.12) if min(size) >= 0.08 else 0.0
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


def fuse(start, voxel, faces, smooth_steps=0, tris=None):
    """Joins the props made since start into one closed mesh: no hidden inner faces that would bake black.
    The voxel size grows until the remesh stays near faces on its own. With tris the remesh is fine enough for
    thin parts (twigs) and collapsed down to that many triangles afterwards, which keeps the vertices where the
    shape needs them; a coarse remesh would lose the thin parts, a fine one wastes triangles on flat surfaces."""
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
    if tris:
        count = sum(len(poly.vertices) - 2 for poly in obj.data.polygons)
        if count > tris:
            collapse = obj.modifiers.new("Decimate", "DECIMATE")
            collapse.decimate_type = "COLLAPSE"
            collapse.ratio = tris / count
            bpy.ops.object.modifier_apply(modifier=collapse.name)
    for poly in obj.data.polygons:
        poly.use_smooth = True
    PROPS.append(obj)
    return obj


# GLOW_FIRE: an animated flame standing on the glow's position, its halo flickering with it (glow-vert.glsl)
GLOW_STEADY, GLOW_FLAME, GLOW_BLINK, GLOW_PULSE, GLOW_FIRE = 0, 1, 2, 3, 4


def glow(loc, color, size, mode=GLOW_STEADY):
    """A light's halo for the game (GltfModel::Glow): an empty with the glow in its extras."""
    obj = bpy.data.objects.new("glow", None)
    obj.location = loc
    obj["glow"] = [float(c) for c in color] + [float(size), float(mode)]
    bpy.context.scene.collection.objects.link(obj)
    return obj


# --- wind ---------------------------------------------------------------------------------------------

WIND = (1.0, 0.35, 0.0)  # which way the gusts push the foliage
SWAY = 0.07  # how far the top of a crown moves at most, tiles (the game's "wave", see GltfModel)


def sway(obj, base, top, amount=1.0):
    """The foliage's give in the wind as its "wave" colour: nothing below base, the full amount at top."""
    colors = obj.data.color_attributes.new("wave", "FLOAT_COLOR", "POINT")
    d = np.array(WIND) / np.linalg.norm(WIND)
    for v in obj.data.vertices:
        z = (obj.matrix_world @ v.co).z
        t = min(max((z - base) / max(top - base, 1e-6), 0.0), 1.0)
        colors.data[v.index].color = (d[0] * 0.5 + 0.5, d[1] * 0.5 + 0.5, d[2] * 0.5 + 0.5, amount * t * t)
    obj.data.color_attributes.active_color = colors


# --- prop kinds -----------------------------------------------------------------------------------

def branch(p0, p1, radius, mat, taper=0.45):
    """A limb from p0 to p1 tapering to taper times its radius."""
    d = np.subtract(p1, p0)
    length = float(np.linalg.norm(d))
    bpy.ops.mesh.primitive_cone_add(vertices=6, radius1=radius, radius2=radius * taper, depth=length,
                                    location=tuple(np.add(p0, p1) * 0.5))
    obj = bpy.context.object
    obj.rotation_mode = "QUATERNION"
    from mathutils import Vector
    obj.rotation_quaternion = Vector((0, 0, 1)).rotation_difference(Vector(d / length))
    return _finish(obj, mat)


def tree_round(st, x, y, z, s, leaves, trunk):
    """A broadleaf about 1.7 s tall: a leaning trunk forking into limbs, the limbs into twigs, a lumpy cluster of
    leaves on every twig and gaps in the crown where the wood shows."""
    rng = st.rng
    start = len(PROPS)
    lean = rng.normal(0, 0.05, 2) * s
    fork = (x + lean[0], y + lean[1], z + 0.6 * s)
    branch((x, y, z - 0.1), fork, 0.12 * s, trunk, taper=0.75)
    tips = []  # per limb its twigs' ends
    count = rng.integers(4, 6)
    for i in range(count + 1):
        if i < count:
            a = 2 * math.pi * i / count + rng.uniform(-0.3, 0.3)
            rise = rng.uniform(0.55, 0.85)  # radians from upright
        else:
            a, rise = rng.uniform(0, 2 * math.pi), rng.uniform(0.0, 0.2)  # the leader, nearly straight up
        length = rng.uniform(0.42, 0.52) * s
        d = np.array([math.cos(a) * math.sin(rise), math.sin(a) * math.sin(rise), math.cos(rise)])
        end = tuple(np.add(fork, d * length))
        branch(fork, end, 0.07 * s, trunk, taper=0.7)
        # a knot where the twigs start, the remesh would part a joint of thin cone ends
        sphere(end, 0.05 * s, trunk, subdiv=1)
        ends = []
        for k in range(rng.integers(2, 4)):
            b = a + rng.uniform(-0.9, 0.9)
            up = rise * 0.6 + rng.uniform(-0.2, 0.3)
            e = np.array([math.cos(b) * math.sin(up), math.sin(b) * math.sin(up), math.cos(up)])
            twig = tuple(np.add(end, e * rng.uniform(0.2, 0.3) * s))
            branch(end, twig, 0.045 * s, trunk, taper=0.7)
            ends.append(twig)
        tips.append(ends)
    sphere(fork, 0.09 * s, trunk, subdiv=1)
    # fine enough that the twigs stay whole
    fuse(start, 0.012 * s, 2000, tris=320)
    # the leaves: per limb one mesh of clusters round its twigs' ends, so the crown stays lumpy
    for ends in tips:
        cluster = len(PROPS)
        for end in ends:
            for k in range(3):
                o = rng.normal(0, 0.07, 3) * s
                sphere((end[0] + o[0], end[1] + o[1], end[2] + o[2] + 0.05 * s), rng.uniform(0.16, 0.21) * s,
                       leaves, jitter=0.12, rng=rng)
        sway(fuse(cluster, 0.015 * s, 260, smooth_steps=1, tris=90), z + 0.5 * s, z + 1.8 * s)


def tree_pine(st, x, y, z, s, leaves, trunk):
    cylinder((x, y, z - 0.05), 0.08 * s, 0.5 * s, trunk, vertices=8)
    start = len(PROPS)
    for i, (r, h, o) in enumerate(((0.5, 0.75, 0.3), (0.4, 0.65, 0.68), (0.28, 0.55, 1.02))):
        cone((x, y, z + o * s), r * s, h * s, leaves, vertices=10)
    sway(fuse(start, 0.03 * s, 300), z + 0.3 * s, z + 1.6 * s)


def rock(st, x, y, z, s, mat, flat=0.65):
    rng = st.rng
    sphere((x, y, z + 0.1 * s), 0.5 * s, mat, scale=(1 + rng.uniform(-0.2, 0.3), 1 + rng.uniform(-0.2, 0.3), flat),
           subdiv=2, jitter=0.18, rng=rng).rotation_euler.z = rng.uniform(0, 6.3)


CRYSTALS = []  # the crystal materials of the world being built


def crystal_material(name, deep, bright, tip):
    """Glowing crystal: deep at the foot, bright inside, light at the tip (the mesh's "tip" attribute, 0..1).
    It emits for the bake only, which marks its texels: no ambient occlusion and no detail there."""
    if name in _MATERIALS:
        return _MATERIALS[name]
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = next(n for n in nodes if n.type == "BSDF_PRINCIPLED")
    bsdf.inputs["Roughness"].default_value = 0.3
    bsdf.inputs["Emission Color"].default_value = (1, 1, 1, 1)
    bsdf.inputs["Emission Strength"].default_value = 1.0
    attribute = nodes.new("ShaderNodeAttribute")
    attribute.attribute_type = "GEOMETRY"
    attribute.attribute_name = "tip"
    ramp = nodes.new("ShaderNodeValToRGB")
    ramp.color_ramp.elements[0].color = tuple(c ** 2.2 for c in deep) + (1,)
    ramp.color_ramp.elements[1].color = tuple(c ** 2.2 for c in tip) + (1,)
    ramp.color_ramp.elements[1].position = 0.97
    middle = ramp.color_ramp.elements.new(0.5)
    middle.color = tuple(c ** 2.2 for c in bright) + (1,)
    links.new(attribute.outputs["Fac"], ramp.inputs["Fac"])
    links.new(ramp.outputs["Color"], bsdf.inputs["Base Color"])
    _MATERIALS[name] = mat
    CRYSTALS.append(mat)
    return mat


def crystal(base, axis, length, radius, mat, rng):
    """A hexagonal shard from base along axis: a prism narrowing a little, then a six sided point; faceted.
    The foot starts a bit below base so it sits in the rock, closed in case a slope lays it open."""
    import bmesh
    axis = Vector(axis).normalized()
    side = axis.orthogonal().normalized()
    side.rotate(Quaternion(axis, rng.uniform(0, 1.05)))
    other = axis.cross(side)
    foot = Vector(base) - axis * length * 0.18
    full = length * 1.18
    bm = bmesh.new()
    tip_layer = bm.verts.layers.float.new("tip")
    rings = []
    for t, r in ((0.0, 1.0), (0.7, 0.88)):
        ring = []
        for i in range(6):
            a = math.pi / 3 * i
            co = foot + axis * full * t + (side * math.cos(a) + other * math.sin(a)) * radius * r * rng.uniform(0.9, 1.1)
            v = bm.verts.new(co)
            v[tip_layer] = t
            ring.append(v)
        rings.append(ring)
    apex = bm.verts.new(foot + axis * full + side * radius * rng.uniform(-0.2, 0.2))
    apex[tip_layer] = 1.0
    for i in range(6):
        j = (i + 1) % 6
        bm.faces.new((rings[0][i], rings[0][j], rings[1][j], rings[1][i]))
        bm.faces.new((rings[1][i], rings[1][j], apex))
    bm.faces.new(list(reversed(rings[0])))
    mesh = bpy.data.meshes.new("crystal")
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new("crystal", mesh)
    bpy.context.scene.collection.objects.link(obj)
    return _finish(obj, mat, smooth_shade=False)


def crystal_cluster(st, x, y, out, height_fn, mat, glow_color, s=1.0):
    """A few shards of a few sizes growing from the rock at (x, y), fanning out from their middle and leaning away
    from the field (out), a pulsing glow among them."""
    rng = st.rng
    out = np.array([out[0], out[1], 0.0])
    count = int(rng.integers(4, 8))
    main = rng.uniform(0.55, 0.85) * s
    top = 0.0
    for i in range(count):
        if i == 0:
            offset, length = np.zeros(3), main
        else:
            a, r = rng.uniform(0, 2 * math.pi), rng.uniform(0.12, 0.3) * s
            offset, length = np.array([math.cos(a) * r, math.sin(a) * r, 0.0]), main * rng.uniform(0.3, 0.75)
        spread = offset / max(np.linalg.norm(offset), 1e-6) * rng.uniform(0.45, 0.9) if i else np.zeros(3)
        axis = np.array([0, 0, 1.0]) + spread + out * rng.uniform(0.15, 0.45) + rng.uniform(-0.12, 0.12, 3)
        px, py = x + offset[0], y + offset[1]
        z = ground_height(st, px, py, height_fn)
        crystal((px, py, z), axis, length, length * rng.uniform(0.15, 0.2), mat, rng)
        top = max(top, z + length * 0.8)
    glow((x + out[0] * 0.15, y + out[1] * 0.15, top - main * 0.35), glow_color, 0.5 + 0.45 * main, GLOW_PULSE)


def bush(st, x, y, z, s, mat):
    rng = st.rng
    start = len(PROPS)
    for i in range(3):
        a = rng.uniform(0, 6.3)
        sphere((x + math.cos(a) * 0.18 * s, y + math.sin(a) * 0.18 * s, z + 0.15 * s), (0.25 + rng.uniform(0, 0.08)) * s,
               mat, scale=(1, 1, 0.8), jitter=0.05, rng=rng)
    sway(fuse(start, 0.03 * s, 200, smooth_steps=2), z, z + 0.45 * s, 0.5)


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

def terrain(st, height_fn, color_fn, name, step=0.5):
    """step: the mesh's grid in tiles, finer where steep slopes would show its facets."""
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
    GROUND[:] = [height_fn, rgb, slope]
    # where grass, soil, rock and pebbles show up close, at half the colour's resolution
    half = GROUND_TEX // 2
    GROUND_DETAIL[:] = [detail.mask(DETAIL[0], np.clip(rgb, 0, 1), slope).reshape(half, 2, half, 2, 4).mean(axis=(1, 3))]
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
GROUND = []  # the ground's height function, colour and slope in texture space, where the grass grows
GROUND_DETAIL = []  # the ground's detail mask, see detail.ground_mask()
PROPS_DETAIL = []  # the props'
DETAIL = ["nature", "foliage"]  # the world's detail sets for the ground and the props, see detail.story_world()


SHORE_REACH = 3.0  # tiles, the water shader's scale of the distance to the shore


def shore_distance(solid, px, reach=SHORE_REACH):
    """Distance in tiles from the nearest solid pixel (px tiles apart), up to reach: grown a pixel at a time
    over the 8 neighbours, close enough to the true distance for foam to lap and drift by."""
    dist = np.where(solid, 0.0, reach)
    for _ in range(int(reach / px) + 1):
        padded = np.pad(dist, 1, constant_values=reach)
        h, w = dist.shape
        grown = dist
        for dy in (-1, 0, 1):
            for dx in (-1, 0, 1):
                if dy or dx:
                    step = px * (1.4142 if dy and dx else 1.0)
                    grown = np.minimum(grown, padded[1 + dy:1 + dy + h, 1 + dx:1 + dx + w] + step)
        dist = grown
    # the 8 neighbour steps leave faint creases along the diagonals, smoothed out so foam lines follow it evenly
    for _ in range(3):
        padded = np.pad(dist, 1, mode="edge")
        dist = sum(padded[1 + dy:1 + dy + dist.shape[0], 1 + dx:1 + dx + dist.shape[1]]
                   for dy in (-1, 0, 1) for dx in (-1, 0, 1)) / 9.0
    return np.minimum(dist, reach)


def lossless(img):
    """img as a PNG: the water's data texture goes out without JPEG's blocks (see glb.replace_image)."""
    w, h = img.size
    pixels = np.empty(w * h * 4, np.float32)
    img.pixels.foreach_get(pixels)
    return glb.png(np.ascontiguousarray(np.round(pixels.reshape(h, w, 4)[::-1] * 255).astype(np.uint8)))


def water_surface(st, height_fn, level, deep_color, field_depth=None):
    """The water as its own mesh for the game's water shader ("water": [deep r, g, b] in its extras); its texture
    is data: red the depth (0..0.8 tiles), green the foam along the shores and the island's wall, blue the
    distance to them (0.02..1 = 0..SHORE_REACH tiles) for the foam to lap against them."""
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
    # what the water laps against: the dry shore and the island's wall (where the ring of foam peaks)
    solid = (height_fn(TX, TY) >= level) | ((d > 1.0) & (d < 1.45))
    shore = shore_distance(solid, (st.x1 - st.x0) / size)
    data = np.stack([depth, foam, 0.02 + 0.98 * shore / SHORE_REACH, np.ones_like(depth)], axis=2)
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
    # big trees, thinning out towards the field so they hide little of it
    for (x, y) in st.scatter(60, lambda x, y: smooth(0.5, 1.8, st.outside(x, y)), 1.8, taken):
        z = ground_height(st, x, y, height)
        s = st.rng.uniform(1.6, 2.5)
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
        t = st.outside(x, y) - edge(x, y) - 1.2
        jag = 0.7 + fbm(x, y, 0.9, 4, 14) * 1.1 + (fbm(x, y, 2.5, 2, 26) - 0.5) * 0.5
        # wide enough that its flanks keep some of the ground texture's resolution, which is laid on from above
        return np.exp(-(t / 1.05) ** 2) * jag

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
        # layered sandstone: beds of their own shade with dark seams between them, broken into blocks by cracks,
        # sharp lines the eye reads as detail where the texture is stretched over the flanks
        layer = (TZ - FLOOR_Z) * 6 + fbm(TX, TY, 0.6, 3, 17) * 2.5
        bed = np.floor(layer)
        shade = _hash(bed, np.floor(fbm(TX, TY, 0.3, 2, 22) * 3), 23) * 0.7 + fbm(TX, TY, 1.8, 3, 24) * 0.3
        brown = mix((0.4, 0.19, 0.08), (0.66, 0.37, 0.17), shade)
        seam = layer - bed
        brown = mix(brown, (0.22, 0.1, 0.05), smooth(0.12, 0.0, np.minimum(seam, 1 - seam)) * 0.8)
        b1, b2 = voronoi(TX * 3.2, TY * 3.2, 25)
        brown = mix(brown, (0.25, 0.12, 0.06), smooth(0.07, 0.02, b2 - b1) * 0.6)
        highland = mix((0.5, 0.48, 0.44), (0.64, 0.62, 0.58), fbm(TX, TY, 0.5, 4, 18))
        highland = mix(highland, (0.32, 0.5, 0.2), smooth(0.55, 0.75, fbm(TX, TY, 0.25, 3, 19)) * 0.7)
        mountain = mix((0.46, 0.46, 0.52), (0.62, 0.62, 0.68), fbm(TX, TY, 0.4, 4, 20))
        rgb = mix(highland, mountain, smooth(1.5, 3.0, TZ - FLOOR_Z))
        rgb = mix(rgb, brown, smooth(0.25, 0.5, ridge(TX, TY)))
        rgb = mix(cobble, rgb, smooth(0.0, 0.25, d - edge(TX, TY)))
        return apron_mix(st, TX, TY, rgb, cobble, reach=0.4, ragged=0.3)

    ground, img = terrain(st, height, color, "ground", step=0.25)
    grey = material("rock_grey", (0.6, 0.6, 0.64), 0.25, 1.5)
    grey2 = material("rock_light", (0.74, 0.74, 0.78), 0.2, 1.5)
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
    # glowing crystals growing from the ridge's crest and flanks
    # purple: it stands out from the orange rock (red melts into it) and goes with the floor's grey lilac
    crystal_mat = crystal_material("crystal", (0.32, 0.07, 0.58), (0.74, 0.36, 1.0), (0.96, 0.88, 1.0))
    for (x, y) in st.scatter(30, lambda x, y: float(abs(st.outside(x, y) - edge(x, y) - 1.2) < 0.4), 1.1, taken,
                             margin=1.0, tries=200):
        e = 0.05
        out = np.array([st.outside(np.array(x + e), np.array(y)) - st.outside(np.array(x - e), np.array(y)),
                        st.outside(np.array(x), np.array(y + e)) - st.outside(np.array(x), np.array(y - e))], float)
        out = out / max(np.linalg.norm(out), 1e-6)
        crystal_cluster(st, x, y, out, height, crystal_mat, (0.5, 0.2, 0.85), st.rng.uniform(0.85, 1.25))
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


def slider(start, place, travel, strokes, phase=0.0):
    """Like spinner(), the props made since start move along place's z axis instead: travel tiles there and back,
    strokes times per loop, easing in and out at both ends; phase is a fraction of a stroke."""
    spinner(start, place, (travel, strokes), phase)


def place_parts(start, place):
    """Puts the props made since start, built around the origin, where place says."""
    bpy.context.view_layer.update()
    for part in PROPS[start:]:
        part.matrix_world = place @ part.matrix_world


def gear(x, y, z, radius, mat, angle, teeth=10, depth=0.2, turns=0, phase=0.0):
    """Upright against a wall, facing angle; turning when turns isn't 0."""
    start = len(PROPS)
    cylinder((0, 0, 0), radius, depth, mat, vertices=24)
    for i in range(teeth):
        a = 2 * math.pi * i / teeth
        # a hair thinner than the wheel, else their faces share its planes
        box((math.cos(a) * radius, math.sin(a) * radius, depth * 0.05), (radius * 0.3, radius * 0.25, depth * 0.9), mat,
            rotation=a)
    cylinder((0, 0, depth), radius * 0.25, depth * 0.6, mat, vertices=12)
    place = Matrix.Translation((x, y, z)) @ Matrix.Rotation(angle, 4, "Z") @ Matrix.Rotation(math.pi / 2, 4, "Y")
    place_parts(start, place)
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
    # thinner than the big one: where their teeth mesh they don't lie in one plane
    gear(gx + tx * distance, gy + ty * distance, height, small, metal, angle, teeth=8, depth=0.15, turns=3,
         phase=math.pi / 8)
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
        if isinstance(turns, tuple):
            # a slider: sampled, the game interpolates linearly between the keys
            travel, strokes = turns
            for frame in range(0, LOOP_FRAMES + 1, 4):
                t = strokes * frame / LOOP_FRAMES + phase
                turning.location = (0.0, 0.0, travel * 0.5 * (1.0 - math.cos(2 * math.pi * t)))
                turning.keyframe_insert("location", index=2, frame=frame)
            continue
        turning.rotation_euler = (0.0, 0.0, phase)
        turning.keyframe_insert("rotation_euler", index=2, frame=0)
        turning.rotation_euler = (0.0, 0.0, phase + 2 * math.pi * turns)
        turning.keyframe_insert("rotation_euler", index=2, frame=LOOP_FRAMES)


def torus(loc, major, minor, mat, segments=24, sides=6):
    bpy.ops.mesh.primitive_torus_add(major_radius=major, minor_radius=minor, major_segments=segments,
                                     minor_segments=sides, location=loc)
    return _finish(bpy.context.object, mat)


def pipe_run(p0, p1, z, radius, mat, flange=None, flange_mat=None, valve_mat=None, valve_every=0):
    """A pipe from p0 to p1 at height z; with flange (tiles) a bolted collar that often along it, with valve_every
    a hand wheel on every so many collars, facing up."""
    (x0, y0), (x1, y1) = p0, p1
    length = math.hypot(x1 - x0, y1 - y0)
    turn = (math.pi / 2, 0, math.atan2(y1 - y0, x1 - x0) + math.pi / 2)
    obj = cylinder(((x0 + x1) / 2, (y0 + y1) / 2, z - length / 2), radius, length, mat, vertices=12)
    obj.rotation_euler = turn
    obj.location.z = z
    for (px, py) in (p0, p1):
        uv_sphere((px, py, z), radius * 1.15, mat, segments=12, rings=8)
    if not flange:
        return
    count = int(length / flange)
    for i in range(1, count):
        t = i / count
        fx, fy = x0 + (x1 - x0) * t, y0 + (y1 - y0) * t
        collar = cylinder((fx, fy, z - 0.05), radius * 1.4, 0.1, flange_mat or mat, vertices=12)
        collar.rotation_euler = turn
        collar.location.z = z
        if valve_every and i % valve_every == 0:
            cylinder((fx, fy, z), radius * 0.25, radius + 0.12, flange_mat or mat, vertices=6)
            torus((fx, fy, z + radius + 0.14), 0.16, 0.025, valve_mat or mat, segments=16, sides=5)
            for k in range(2):
                box((fx, fy, z + radius + 0.125), (0.3, 0.03, 0.03), valve_mat or mat, rotation=k * math.pi / 2,
                    bevel=0)


def paving(TX, TY, light, dark, joint, size=1.0, seed=60):
    """Rows of cut stone slabs, every other row shifted by half a slab, each slab its own shade."""
    row = np.floor(TY / size)
    x = TX / size + (row % 2) * 0.5
    col = np.floor(x)
    fx, fy = x - col, TY / size - row
    shade = _hash(col, row, seed)
    rgb = mix(dark, light, np.clip(0.6 * shade + 0.4 * fbm(TX, TY, 1.2, 3, seed + 1), 0, 1))
    edge = np.minimum(np.minimum(fx, 1 - fx), np.minimum(fy, 1 - fy)) * size
    rgb = rgb * (0.84 + 0.16 * smooth(0.0, 0.08, edge))[..., None]
    return mix(rgb, joint, smooth(0.03, 0.012, edge))


def tread_floor(TX, TY, base, size=0.25):
    """Steel deck plate on the ground: raised lozenges in pairs at right angles, plate seams every two tiles."""
    X, Y = TX / size, TY / size
    fx, fy = X - np.floor(X) - 0.5, Y - np.floor(Y) - 0.5
    turn = (np.floor(X) + np.floor(Y)) % 2
    u = np.where(turn > 0, fx + fy, fx - fy) * 0.7071
    v = np.where(turn > 0, fx - fy, fx + fy) * 0.7071
    lozenge = smooth(1.15, 0.85, (u / 0.32) ** 2 + (v / 0.08) ** 2)
    base = np.asarray(base, float)
    rgb = mix(base * 0.78, base * 1.05, fbm(TX, TY, 0.8, 4, 70))
    rgb = rgb * (1.0 + 0.22 * lozenge)[..., None]
    sx, sy = TX / 2.0 - np.floor(TX / 2.0), TY / 2.0 - np.floor(TY / 2.0)
    seam = np.minimum(np.minimum(sx, 1 - sx), np.minimum(sy, 1 - sy)) * 2.0
    rgb = mix(rgb, base * 0.4, smooth(0.04, 0.015, seam))
    # oil and rust stains
    rgb = mix(rgb, (0.32, 0.22, 0.14), smooth(0.66, 0.8, fbm(TX, TY, 0.3, 4, 71)) * 0.5)
    return rgb


def torch(x, y, angle, z, iron, wood):
    """A torch in an iron bracket, burning. (x, y) lies on the surface it hangs on, angle is the way it faces: the
    plate sinks a little into it (a column is round), an arm reaches out into the torch's shaft."""
    dx, dy = math.cos(angle), math.sin(angle)
    out = 0.22  # the shaft's axis from the surface
    box((x, y, z - 0.2), (0.12, 0.14, 0.3), iron, rotation=angle)
    box((x + dx * out * 0.5, y + dy * out * 0.5, z - 0.08), (out + 0.04, 0.05, 0.05), iron, rotation=angle)
    cylinder((x + dx * out, y + dy * out, z - 0.15), 0.05, 0.6, wood, vertices=6)
    cylinder((x + dx * out, y + dy * out, z + 0.38), 0.09, 0.14, iron, vertices=8)
    glow((x + dx * out, y + dy * out, z + 0.5), (1.0, 0.62, 0.28), 0.9, GLOW_FIRE)


def banner(x, y, angle, z, cloth, trim):
    """A long red banner with a gold hem and a gold diamond, hanging flat against the wall."""
    dx, dy = math.cos(angle), math.sin(angle)
    box((x + dx * 0.04, y + dy * 0.04, z + 0.4), (0.95, 0.05, 2.1), cloth, rotation=angle + math.pi / 2)
    # the hem a little wider than the cloth, their ends would share a plane
    box((x + dx * 0.05, y + dy * 0.05, z + 0.32), (1.0, 0.06, 0.1), trim, rotation=angle + math.pi / 2)
    box((x + dx * 0.04, y + dy * 0.04, z + 2.5), (1.15, 0.08, 0.08), trim, rotation=angle + math.pi / 2)
    crest = box((x + dx * 0.07, y + dy * 0.07, z + 1.25), (0.32, 0.04, 0.32), trim)
    crest.rotation_euler = (0.0, 0.0, angle + math.pi / 2)
    crest.rotation_euler.rotate_axis("Y", math.pi / 4)


def column(x, y, z, height, stone, cap):
    box((x, y, z), (0.95, 0.95, 0.25), cap, bevel=0.03)
    cylinder((x, y, z + 0.25), 0.36, height - 0.55, stone, vertices=16)
    box((x, y, z + height - 0.3), (0.95, 0.95, 0.3), cap, bevel=0.03)


def brazier(x, y, z, iron, embers):
    """A fire bowl on three legs."""
    for k in range(3):
        a = 2 * math.pi * k / 3
        box((x + math.cos(a) * 0.22, y + math.sin(a) * 0.22, z), (0.07, 0.07, 0.75), iron)
    cone((x, y, z + 0.6), 0.18, 0.35, iron, vertices=12, radius2=0.42)
    # a low mound of embers heaped on the bowl's top, not a disc in the same plane (they fought for the pixels)
    dome((x, y, z + 0.92), 0.37, embers, segments=12, rings=3).scale = (1.0, 1.0, 0.22)
    glow((x, y, z + 0.98), (1.0, 0.58, 0.24), 1.3, GLOW_FIRE)


def barrel(x, y, z, wood, iron, s=1.0):
    cylinder((x, y, z), 0.3 * s, 0.8 * s, wood, vertices=12)
    for h in (0.15, 0.62):
        cylinder((x, y, z + h * s), 0.315 * s, 0.06 * s, iron, vertices=12)


def world_castle_1(st):
    """6: the mechanical castle's first floor, still the old keep: sandstone paving with a red carpet up to the
    gate, ashlar walls with columns, red banners and torches, braziers by the field."""
    def height(x, y):
        return np.full_like(x, FLOOR_Z, dtype=float)

    def color(TX, TY, TZ, slope):
        rgb = paving(TX, TY, (0.8, 0.7, 0.52), (0.6, 0.48, 0.34), (0.3, 0.24, 0.18))
        # the carpet from the field's top edge to the gate, gold edged
        across = np.abs(TX - st.cx)
        on = (TY > 1.0) & (across < 1.3)
        carpet = mix((0.5, 0.06, 0.06), (0.66, 0.12, 0.1), fbm(TX, TY, 2.0, 3, 61))
        carpet = mix(carpet, (0.86, 0.66, 0.22), smooth(1.06, 1.1, across) * smooth(1.26, 1.22, across))
        rgb = np.where(on[..., None], carpet, rgb)
        return apron_mix(st, TX, TY, rgb, rgb)

    ground, img = terrain(st, height, color, "ground")
    ashlar = material("ashlar", (0.66, 0.6, 0.5), 0.1, 1.2, kind="bricks", mortar=(0.32, 0.28, 0.23))
    stone = material("column_stone", (0.74, 0.68, 0.58), 0.08, 3.0)
    cap = material("column_cap", (0.6, 0.55, 0.46), 0.08, 3.0)
    iron = material("iron", (0.2, 0.2, 0.22), 0.1, 5.0)
    wood = material("castle_wood", (0.42, 0.26, 0.14), 0.18, 4.0)
    door = material("gate_wood", (0.34, 0.2, 0.1), 0.2, 2.0)
    cloth = material("banner_red", (0.68, 0.1, 0.1), 0.12, 4.0)
    gold = material("gold_trim", (0.92, 0.72, 0.28), 0.08, 5.0)
    embers = material("embers", (0.95, 0.42, 0.12), 0.3, 8.0, color2=(1.0, 0.8, 0.3))
    x0, x1, y0, y1 = room(st, ashlar)
    z = FLOOR_Z
    # columns along the walls with a torch on each, banners between them
    spacing = 3.2
    for (x, y, a) in along_walls(x0, x1, y0, y1, spacing, skip_x=st.cx):
        dx, dy = math.cos(a), math.sin(a)
        column(x + dx * 0.45, y + dy * 0.45, z, 4.4, stone, cap)
        # on the column's face towards the room, its shaft's radius is 0.36
        torch(x + dx * 0.79, y + dy * 0.79, a, z + 2.2, iron, wood)
    for (x, y, a) in along_walls(x0 + spacing * 0.5, x1 - spacing * 0.5, y0, y1 - spacing * 0.5, spacing,
                                 skip_x=st.cx):
        if abs(a + math.pi / 2) < 0.1:
            banner(x, y, a, z + 1.0, cloth, gold)
    for (x, y, a) in along_walls(x0, x1, y0, y1 - spacing * 0.5, spacing):
        if abs(a + math.pi / 2) > 0.1:
            banner(x, y, a, z + 1.0, cloth, gold)
    # the gate at the carpet's end: an oak door with iron bands in a stone frame
    box((st.cx, y1 - 0.1, z), (3.2, 0.5, 3.6), cap, bevel=0.05)
    box((st.cx, y1 - 0.3, z), (2.4, 0.2, 3.0), door, bevel=0.03)
    for h in (0.6, 1.5, 2.4):
        box((st.cx, y1 - 0.42, z + h), (2.45, 0.06, 0.14), iron)
    for side in (-1, 1):
        # on the frame beside the door, clear of the columns next to the gate
        torch(st.cx + side * 1.4, y1 - 0.34, -math.pi / 2, z + 2.0, iron, wood)
    # braziers beside the field, barrels and crates by the walls
    for bx in (-APRON - 1.6, st.w + APRON + 1.6):
        for by in (APRON + 1.2, -st.h * 0.5, -st.h - APRON - 1.2):
            brazier(bx, by, z, iron, embers)
    taken = [(st.cx, y1, 2.5)]
    near_wall = lambda x, y: float((x < x0 + 2.2) | (x > x1 - 2.2) | (y > y1 - 2.0))  # noqa: E731
    for (x, y) in st.scatter(14, near_wall, 1.0, taken, margin=2.5):
        if st.rng.random() < 0.6:
            barrel(x, y, z, wood, iron, st.rng.uniform(0.85, 1.1))
        else:
            box((x, y, z), (0.8, 0.8, 0.7), wood, rotation=st.rng.uniform(-0.3, 0.3), bevel=0.03)
    return ground, img


def pilaster(x, y, angle, z, height, steel, dark):
    """An I-beam standing against a wall (x, y on its face, angle the way the room is), bolted to a foot plate."""
    dx, dy = math.cos(angle), math.sin(angle)
    a = angle + math.pi / 2
    box((x + dx * 0.04, y + dy * 0.04, z), (0.42, 0.08, height), steel, rotation=a)
    box((x + dx * 0.16, y + dy * 0.16, z), (0.08, 0.24, height), steel, rotation=a)
    box((x + dx * 0.29, y + dy * 0.29, z), (0.42, 0.08, height), steel, rotation=a)
    box((x + dx * 0.17, y + dy * 0.17, z), (0.6, 0.44, 0.12), dark, rotation=a)


def piston(x, y, angle, z, dark, brass, steel, phase):
    """A steam cylinder working a crosshead up and down: the cylinder on its housing, the rod, the crosshead
    riding two guide rails that are bolted to the wall."""
    dx, dy = math.cos(angle), math.sin(angle)
    tx, ty = -dy, dx
    a = angle + math.pi / 2
    px, py = x + dx * 0.55, y + dy * 0.55
    box((px, py, z), (0.9, 0.9, 0.4), dark, rotation=a)
    cylinder((px, py, z + 0.4), 0.36, 0.12, steel, vertices=16)
    cylinder((px, py, z + 0.52), 0.3, 0.9, brass, vertices=16)
    cylinder((px, py, z + 1.42), 0.36, 0.12, steel, vertices=16)
    for side in (-1, 1):
        rx, ry = x + dx * 0.12 + tx * side * 0.32, y + dy * 0.12 + ty * side * 0.32
        box((rx, ry, z + 0.4), (0.1, 0.12, 2.6), steel, rotation=a)
        box((rx - dx * 0.06, ry - dy * 0.06, z + 2.9), (0.14, 0.1, 0.12), dark, rotation=a)
    start = len(PROPS)
    cylinder((px, py, z + 1.3), 0.08, 0.9, steel, vertices=10)
    box((x + dx * 0.4, y + dy * 0.4, z + 2.1), (0.84, 0.62, 0.2), dark, rotation=a)
    slider(start, Matrix.Translation((px, py, z)), 0.45, 2, phase)


def crate(x, y, z, s, side, frame, rotation):
    """A crate: panels of side inside a frame of beams along its edges."""
    box((x, y, z + 0.01), (s * 0.94, s * 0.94, s * 0.96), side, rotation=rotation)
    c, w = math.cos(rotation), math.sin(rotation)
    for i in (-1, 1):
        for j in (-1, 1):
            ox, oy = (i * c - j * w) * s * 0.44, (i * w + j * c) * s * 0.44
            box((x + ox, y + oy, z), (s * 0.12, s * 0.12, s), frame, rotation=rotation)
    for h in (0.0, s * 0.88):
        for k in range(2):
            r = rotation + k * math.pi / 2
            for e in (-1, 1):
                box((x - math.sin(r) * e * s * 0.44, y + math.cos(r) * e * s * 0.44, z + h), (s, s * 0.12, s * 0.12),
                    frame, rotation=r)


def drum(x, y, z, body, rim, s=1.0):
    """An oil drum: rolling hoops round it, a rim round its lid."""
    cylinder((x, y, z), 0.3 * s, 0.85 * s, body, vertices=16)
    # the hoops clear of the drum's ends, a rim round the lid: no faces sharing a plane
    for h in (0.06, 0.3, 0.55):
        cylinder((x, y, z + h * s), 0.315 * s, 0.05 * s, rim, vertices=16)
    torus((x, y, z + 0.85 * s), 0.29 * s, 0.025 * s, rim, segments=16, sides=6)


def world_castle_2(st):
    """7: the second floor, the castle's machinery: a steel deck, dark iron walls with glowing furnace mouths, gears,
    pipes and pistons along them, hazard stripes, crates and drums."""
    def height(x, y):
        return np.full_like(x, FLOOR_Z, dtype=float)

    def color(TX, TY, TZ, slope):
        rgb = tread_floor(TX, TY, (0.5, 0.52, 0.54))
        # hazard stripes in front of the walls
        d = st.outside(TX, TY)
        stripe = ((np.floor((TX + TY) * 1.5)) % 2)
        band = smooth(0.45, 0.35, np.abs(d - 3.4))
        rgb = mix(rgb, mix((0.15, 0.12, 0.1), (0.95, 0.75, 0.1), stripe), band * float(1))
        return apron_mix(st, TX, TY, rgb, rgb)

    ground, img = terrain(st, height, color, "ground")
    iron = material("iron_wall", (0.36, 0.32, 0.29), 0.12, 1.4, kind="plates", mortar=(0.13, 0.12, 0.11))
    steel = material("steel_beam", (0.5, 0.52, 0.56), 0.1, 5.0)
    crate_side = material("crate_side", (0.62, 0.45, 0.26), 0.15, 6.0, color2=(0.48, 0.32, 0.17))
    oil = material("oil_drum", (0.2, 0.36, 0.5), 0.1, 5.0)
    brass = material("brass", (0.8, 0.62, 0.26), 0.1, 5.0)
    copper = material("copper", (0.72, 0.38, 0.2), 0.1, 5.0)
    dark = material("dark_metal", (0.22, 0.23, 0.27), 0.08, 5.0)
    lamp = material("lamp_red", (0.96, 0.4, 0.3), 0.1)
    lamp_glass = material("lamp_glass", (1.0, 0.85, 0.6), 0.05)
    green = material("lamp_green", (0.5, 0.9, 0.66), 0.1)
    rubber = material("belt_rubber", (0.12, 0.11, 0.1), 0.1)
    fire = material("furnace_fire", (1.0, 0.5, 0.12), 0.25, 6.0, color2=(1.0, 0.85, 0.35))
    yellow = material("hazard_yellow", (0.92, 0.7, 0.14), 0.08, 4.0)
    x0, x1, y0, y1 = room(st, iron, offset_x=4.6, offset_north=4.0)
    z = FLOOR_Z
    i = 0
    for (x, y, a) in along_walls(x0, x1, y0, y1, 3.0):
        i += 1
        dx, dy = math.cos(a), math.sin(a)
        if i % 3 == 0:
            gear_train(x, y, z, a, brass, dark, rubber)
        elif i % 3 == 1:
            piston(x, y, a, z, dark, brass, steel, phase=i * 0.37)
        else:
            # a furnace in the wall, the fire's glow flickering on the deck
            box((x + dx * 0.3, y + dy * 0.3, z), (1.6, 0.6, 1.6), dark, rotation=a + math.pi / 2, bevel=0.05)
            # the fire stands a little proud of the furnace's front, flush they fought for the pixels
            box((x + dx * 0.6, y + dy * 0.6, z + 0.3), (1.0, 0.04, 0.7), fire, rotation=a + math.pi / 2)
            for k in range(4):
                box((x + dx * 0.62 + dy * (k - 1.5) * 0.25, y + dy * 0.62 - dx * (k - 1.5) * 0.25, z + 0.25),
                    (0.06, 0.06, 0.8), dark, rotation=a)
            glow((x + dx * 0.9, y + dy * 0.9, z + 0.65), (1.0, 0.55, 0.2), 1.2, GLOW_FLAME)
            for k in (-1, 1):
                uv_sphere((x + dx * 0.62 + dy * k * 0.6, y + dy * 0.62 - dx * k * 0.6, z + 1.3), 0.07,
                          lamp if k > 0 else green, segments=10, rings=6)
                glow((x + dx * 0.66 + dy * k * 0.6, y + dy * 0.66 - dx * k * 0.6, z + 1.3),
                     (1.0, 0.48, 0.36) if k > 0 else (0.55, 1.0, 0.72), 0.3, GLOW_BLINK)
    # I-beams between the machines, a lamp on each, a beam along the top of the walls
    for (x, y, a) in along_walls(x0, x1, y0, y1, 3.0):
        dx, dy = math.cos(a), math.sin(a)
        bx, by = x - dy * 1.5, y + dx * 1.5
        if x0 <= bx <= x1 and y0 <= by <= y1:
            pilaster(bx, by, a, z, 4.3, steel, dark)
            wall_lamp(bx + dx * 0.3, by + dy * 0.3, a, z + 2.9, dark, lamp_glass)
    for (p0, p1) in (((x0, y1), (x1, y1)), ((x0, y0), (x0, y1)), ((x1, y0), (x1, y1))):
        mx, my = (p0[0] + p1[0]) * 0.5, (p0[1] + p1[1]) * 0.5
        box((mx, my, z + 4.15), (math.hypot(p1[0] - p0[0], p1[1] - p0[1]) + 0.5, 0.5, 0.3), steel,
            rotation=math.atan2(p1[1] - p0[1], p1[0] - p0[0]))
    # pipes running along the walls, bolted together every tile and a half, a valve now and then
    for h, r, m in ((0.7, 0.14, copper), (3.4, 0.2, brass)):
        for p0, p1 in (((x0 + 0.35, y1 + 0.2), (x1 - 0.35, y1 + 0.2)), ((x0 + 0.25, y0), (x0 + 0.25, y1 - 0.1)),
                       ((x1 - 0.25, y0), (x1 - 0.25, y1 - 0.1))):
            pipe_run(p0, p1, z + h, r, m, flange=1.5, flange_mat=dark, valve_mat=lamp, valve_every=3 if h < 1 else 0)
    # yellow crates and drums between the field and the hazard band
    taken = []
    between = lambda x, y: float(st.outside(x, y) > 0.9) * float(st.outside(x, y) < 2.6)  # noqa: E731
    for (x, y) in st.scatter(16, between, 1.1, taken, margin=0.9):
        if st.rng.random() < 0.5:
            crate(x, y, z, 0.8, crate_side, yellow, st.rng.uniform(-0.2, 0.2))
            if st.rng.random() < 0.4:
                crate(x, y, z + 0.8, 0.55, crate_side, dark, st.rng.uniform(-0.4, 0.4))
        else:
            drum(x, y, z, oil if st.rng.random() < 0.5 else copper, dark)
    return ground, img


# --- the final floor --------------------------------------------------------------------------------

PULSE_SPEED = 2.5  # how fast a pulsing glow breathes, radians a second (glow-vert.glsl)


def lit_material(name, color, variation=0.1, scale=4.0, color2=None):
    """A surface giving its own light (a screen, a lamp, a charged coil): baked like any other colour, it emits for
    the bake only, which marks its texels like the crystals': no ambient occlusion and no detail there."""
    mat = material(name, color, variation, scale, color2=color2)
    if mat not in CRYSTALS:
        bsdf = next(n for n in mat.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
        bsdf.inputs["Emission Color"].default_value = (1, 1, 1, 1)
        bsdf.inputs["Emission Strength"].default_value = 1.0
        CRYSTALS.append(mat)
    return mat


def glow_phase(x, y):
    """How many seconds a glow's animation runs ahead: GlowSprites::set() takes it from the glow's position."""
    return math.fmod(abs(x * 12.9898 + y * 78.233), 100.0)


def chase(p0, p1, z, color, size, spacing=0.6, lag=0.2):
    """Pulsing glows from p0 to p1 whose pulses run along them, from p0 to p1: each one nudged (a tenth of a tile at
    most) to where its phase (glow_phase()) lags the one before by lag seconds."""
    period = 2 * math.pi / PULSE_SPEED
    p0, p1 = np.array(p0, float), np.array(p1, float)
    length = float(np.linalg.norm(p1 - p0))
    d = (p1 - p0) / max(length, 1e-6)
    count = max(2, int(round(length / spacing)) + 1)
    nudges = np.linspace(-0.1, 0.1, 801)
    first = glow_phase(*p0)
    for i in range(count):
        base = p0 + d * (length * i / (count - 1))
        want = first - i * lag
        err = [abs((glow_phase(*(base + d * n)) - want + period / 2) % period - period / 2) for n in nudges]
        x, y = base + d * nudges[int(np.argmin(err))] if i else base
        glow((float(x), float(y), z), color, size, GLOW_PULSE)


def reel(x, y, z, radius, mat, spoke, angle, turns, phase=0.0):
    """A tape reel upright on a front facing angle: a disc with three light spokes, so its turning shows."""
    start = len(PROPS)
    cylinder((0, 0, 0), radius, 0.05, mat, vertices=20)
    for i in range(3):
        a = 2 * math.pi * i / 3
        box((math.cos(a) * radius * 0.5, math.sin(a) * radius * 0.5, 0.05), (radius * 0.8, 0.07, 0.015), spoke,
            rotation=a)
    cylinder((0, 0, 0.05), radius * 0.22, 0.03, spoke, vertices=10)
    place = Matrix.Translation((x, y, z)) @ Matrix.Rotation(angle, 4, "Z") @ Matrix.Rotation(math.pi / 2, 4, "Y")
    place_parts(start, place)
    spinner(start, place, turns, phase)


def mainframe(x, y, z, white, dark, reel_mat, spoke, lamps, status):
    """A computer of the stage's hard blocks' kind against the north wall (its front at y): a white cabinet, two tape
    reels turning behind a dark window, a row of blinking lamps, a lit status bar on top."""
    box((x, y + 0.36, z), (1.6, 0.76, 2.4), white, bevel=0.04)
    box((x, y - 0.01, z + 1.2), (1.34, 0.06, 0.92), dark)
    for k, side in enumerate((-1, 1)):
        reel(x + side * 0.34, y - 0.05, z + 1.66, 0.28, reel_mat, spoke, -math.pi / 2, turns=3 * side,
             phase=k * 0.7)
    for k in range(6):
        color, light = lamps[k % 3]
        lx = x - 0.55 + k * 0.22
        box((lx, y - 0.01, z + 0.72), (0.12, 0.05, 0.08), color)
        glow((lx, y - 0.08, z + 0.76), light, 0.22, GLOW_BLINK)
    box((x, y - 0.01, z + 2.12), (1.2, 0.05, 0.08), status)


def wall_screen(x, y, z, width, height, frame, screen, light):
    """A screen hanging on the north wall (its face at y), its light breathing on the room."""
    box((x, y + 0.04, z), (width + 0.16, 0.1, height + 0.16), frame, bevel=0.02)
    box((x, y - 0.02, z + 0.08), (width, 0.04, height), screen)
    glow((x, y - 0.35, z + 0.08 + height * 0.5), light, 0.45 * width, GLOW_PULSE)


def control_desk(x, y, z, grey, panel, dark, screen, lamps):
    """An operator's desk facing the wall screens: its slanted panel and its monitors face the room's south."""
    box((x, y, z), (1.8, 0.62, 0.7), grey, bevel=0.03)
    top = box((x, y - 0.06, z + 0.66), (1.7, 0.48, 0.05), panel)
    top.rotation_euler.x = 0.35
    for k, side in enumerate((-1, 1)):
        box((x + side * 0.45, y + 0.2, z + 0.72), (0.56, 0.1, 0.42), dark, bevel=0.015)
        box((x + side * 0.45, y + 0.14, z + 0.78), (0.46, 0.03, 0.3), screen)
        _, light = lamps[k]
        glow((x + side * 0.45, y + 0.02, z + 0.93), (0.35, 0.65, 1.0), 0.4, GLOW_PULSE)
        glow((x - side * 0.1, y - 0.14, z + 0.74), light, 0.16, GLOW_BLINK)


def power_coil(x, y, z, angle, dark, copper, charge, red):
    """A tall coil charging the castle: a drum, copper windings round its core, a charged ball crackling on top, a
    red lamp at its foot towards the room."""
    cylinder((x, y, z), 0.55, 0.35, dark, vertices=16)
    cylinder((x, y, z + 0.3), 0.2, 1.96, dark, vertices=10)
    for k in range(4):
        cylinder((x, y, z + 0.55 + k * 0.4), 0.36, 0.2, copper, vertices=14)
    cylinder((x, y, z + 2.2), 0.3, 0.1, dark, vertices=14)
    uv_sphere((x, y, z + 2.58), 0.3, charge, segments=14, rings=8)
    glow((x, y, z + 2.58), (0.35, 0.8, 1.0), 1.5, GLOW_PULSE)
    glow((x, y, z + 2.58), (0.7, 0.95, 1.0), 0.55, GLOW_FLAME)
    dx, dy = math.cos(angle), math.sin(angle)
    box((x + dx * 0.55, y + dy * 0.55, z + 0.12), (0.1, 0.16, 0.12), red, rotation=angle)
    glow((x + dx * 0.64, y + dy * 0.64, z + 0.18), (1.0, 0.25, 0.15), 0.35, GLOW_BLINK)


def power_press(x, y, z, angle, dark, steel, hazard, hot, phase):
    """A press stamping a glowing ingot: two posts, a beam with the ram's sleeve, the ram and its head going down
    and up again."""
    dx, dy = math.cos(angle), math.sin(angle)
    tx, ty = -dy, dx
    a = angle + math.pi / 2
    box((x, y, z), (1.5, 1.0, 0.3), dark, rotation=a, bevel=0.03)
    box((x, y, z + 0.3), (0.8, 0.6, 0.12), steel, rotation=a)
    box((x, y, z + 0.42), (0.5, 0.3, 0.12), hot, rotation=a)
    glow((x, y, z + 0.5), (1.0, 0.5, 0.18), 0.7, GLOW_FLAME)
    for side in (-1, 1):
        box((x + tx * side * 0.62, y + ty * side * 0.62, z + 0.3), (0.18, 0.18, 2.4), steel, rotation=a)
    box((x, y, z + 2.7), (1.5, 0.34, 0.3), hazard, rotation=a, bevel=0.02)
    cylinder((x, y, z + 2.15), 0.17, 0.56, dark, vertices=12)
    start = len(PROPS)
    # long enough to stay in its sleeve at the bottom of the stroke
    cylinder((x, y, z + 1.15), 0.1, 1.45, steel, vertices=10)
    box((x, y, z + 0.95), (1.0, 0.62, 0.22), hazard, rotation=a, bevel=0.02)
    slider(start, Matrix.Translation((x, y, z)), -0.4, 2, phase)


def floor_fan(x, y, z, frame_mat, blade_mat, pit, turns):
    """A fan in a pit in the floor, its blades turning over the furnace's glow below."""
    # the glowing floor of the pit: wider than the opening, its edges hidden in the frame
    box((x, y, z - 0.08), (1.2, 1.2, 0.1), pit)
    for (ox, oy, sx, sy) in ((0, 0.575, 1.3, 0.15), (0, -0.575, 1.3, 0.15), (0.575, 0, 0.15, 1.0),
                             (-0.575, 0, 0.15, 1.0)):
        box((x + ox, y + oy, z - 0.05), (sx, sy, 0.19), frame_mat)
    glow((x, y, z + 0.02), (1.0, 0.42, 0.14), 0.85, GLOW_FLAME)
    start = len(PROPS)
    cylinder((0, 0, 0.04), 0.1, 0.07, blade_mat, vertices=10)
    for i in range(5):
        a = 2 * math.pi * i / 5
        blade = box((math.cos(a) * 0.27, math.sin(a) * 0.27, 0.06), (0.34, 0.13, 0.02), blade_mat)
        blade.rotation_euler = (0.45, 0.0, a)
    place = Matrix.Translation((x, y, z))
    place_parts(start, place)
    spinner(start, place, turns)


def capacitors(x, y, z, steel, dark, charge):
    """A bank of four capacitors on a plate, charged, their caps lit."""
    box((x, y, z - 0.05), (1.0, 1.0, 0.17), steel, bevel=0.02)
    for ox in (-0.24, 0.24):
        for oy in (-0.24, 0.24):
            cylinder((x + ox, y + oy, z + 0.1), 0.19, 0.6, dark, vertices=12)
            cylinder((x + ox, y + oy, z + 0.7), 0.14, 0.05, charge, vertices=12)
    glow((x, y, z + 0.8), (0.3, 0.75, 1.0), 0.8, GLOW_PULSE)


def beacon(x, y, z, dark, lamp, light, turns):
    """A warning light: a lamp in a shade that sweeps round it."""
    cylinder((x, y, z), 0.17, 0.08, dark, vertices=12)
    cylinder((x, y, z + 0.08), 0.11, 0.26, lamp, vertices=12)
    cylinder((x, y, z + 0.34), 0.17, 0.05, dark, vertices=12)
    start = len(PROPS)
    box((0.0, 0.125, 0.1), (0.27, 0.04, 0.22), dark)
    place = Matrix.Translation((x, y, z))
    place_parts(start, place)
    spinner(start, place, turns)
    glow((x, y, z + 0.22), light, 0.9, GLOW_BLINK)


def reactor(x, y, z, s, dark, steel, charge, red, ring_mat):
    """The castle's heart: an energy column on a plinth, clamped, two tilted rings wobbling round it, red lamps on
    four pylons."""
    cylinder((x, y, z), 1.25 * s, 0.3, dark, vertices=24)
    cylinder((x, y, z + 0.3), 0.95 * s, 0.2, steel, vertices=24)
    top = z + 0.5 + 2.7 * s
    cylinder((x, y, z + 0.45), 0.36 * s, top - z - 0.4, charge, vertices=16)
    for k in range(3):
        cylinder((x, y, z + 0.5 + (0.6 + k * 0.75) * s), 0.46 * s, 0.14, dark, vertices=16)
    cylinder((x, y, top), 0.56 * s, 0.26, dark, vertices=16)
    cylinder((x, y, top + 0.26), 0.3 * s, 0.12, steel, vertices=16)
    for k in range(4):
        a = math.pi / 4 + k * math.pi / 2
        px, py = x + math.cos(a) * 1.08 * s, y + math.sin(a) * 1.08 * s
        box((px, py, z + 0.3), (0.14, 0.14, 1.9 * s), steel, rotation=a)
        box((px, py, z + 0.3 + 1.9 * s), (0.2, 0.2, 0.12), red, rotation=a)
        glow((px, py, z + 0.45 + 1.9 * s), (1.0, 0.22, 0.12), 0.5, GLOW_BLINK)
    for k, (height, tilt, turns) in enumerate(((1.25, 0.32, 2), (2.25, -0.38, -3))):
        start = len(PROPS)
        ring = torus((0, 0, 0), 0.8 * s, 0.05 * s, ring_mat)
        ring.rotation_euler.x = tilt
        place = Matrix.Translation((x, y, z + 0.5 + height * s))
        place_parts(start, place)
        spinner(start, place, turns, phase=k * 1.3)
    glow((x, y, z + 0.5 + 1.5 * s), (0.3, 0.75, 1.0), 2.8 * s, GLOW_PULSE)
    for k in range(3):
        glow((x, y, z + 0.5 + (0.3 + k * 0.95) * s), (0.75, 0.97, 1.0), 0.75 * s, GLOW_FLAME)
    glow((x, y, top + 0.45), (0.45, 0.85, 1.0), 1.1 * s, GLOW_PULSE)


def world_castle_final(st):
    """8: the final floor, the castle's control room and the heart of its machinery. The teal diamond floor and the
    grey metal border of the original; mainframes with turning tape reels, wall screens and operators' desks behind
    the field, the reactor in their middle, power coils and presses along the side walls, fans in the floor, glowing
    conduits carrying the power round the field, warning lights: cold teal and blue against red and amber. The final
    fight (the single screen stage) has the reactor bigger and the big screen in alarm red."""
    final_fight = (st.w, st.h) == SIZES[None]
    offset_x, offset_north = 4.2, 3.8
    x0, x1 = -APRON - offset_x, st.w + APRON + offset_x
    y0, y1 = -st.h - APRON - 6.0, APRON + offset_north
    z = FLOOR_Z
    s = 1.25 if final_fight else 1.0

    # where the machines stand, known before the floor is painted: it gets hazard pads under them
    north = [x for x in np.arange(x0 + 1.05, x1 - 0.8, 2.0) if abs(x - st.cx) > 3.4]
    sides = [y for y in np.arange(y1 - 1.9, y0 + 1.2, -3.0)]
    fans = [y - 1.5 for y in sides if y - 1.5 > y0 + 1.0]
    coil_x, fan_x = 0.95, 2.8  # in from the side walls
    bus_y, bus_x = APRON + 0.75, 2.05  # the conduits' main lines: in front of the desks, between coils and fans
    pads = [(st.cx, 3.1, 1.45 * s, True)]
    for i, y in enumerate(sides):
        for x in (x0 + coil_x, x1 - coil_x):
            pads.append((x, y, 0.72 if i % 2 == 0 else 0.95, i % 2 == 0))
    for y in fans:
        for x in (x0 + fan_x, x1 - fan_x):
            pads.append((x, y, 0.82, False))
    # the conduits, axis aligned segments: the bus round the field from the reactor, branches into the field's ring
    # and to the coils
    ring = 1.27  # from the field's edge, just off the border blocks
    lines = [((x0 + bus_x, bus_y), (x1 - bus_x, bus_y)), ((st.cx, bus_y), (st.cx, 3.1 - 1.2 * s)),
             ((x0 + bus_x, bus_y), (x0 + bus_x, sides[-1])), ((x1 - bus_x, bus_y), (x1 - bus_x, sides[-1]))]
    for i, y in enumerate(sides):
        if i % 2 == 0:
            lines += [((x0 + coil_x + 0.5, y), (-ring, y)), ((st.w + ring, y), (x1 - coil_x - 0.5, y))]
    for x in north:
        if 0 < x < st.w:
            lines.append(((x, ring), (x, bus_y)))

    def height(x, y):
        return np.full_like(x, FLOOR_Z, dtype=float)

    def color(TX, TY, TZ, slope):
        rgb = diamond_floor(st, TX, TY, (0.12, 0.5, 0.56), (0.06, 0.36, 0.44), (0.04, 0.22, 0.28), size=1.4)
        rgb = apron_mix(st, TX, TY, rgb, rgb)
        # the hall a shade darker than the field, its lights stand out
        rgb = rgb * (1.0 - 0.28 * smooth(1.4, 3.0, st.outside(TX, TY)))[..., None]
        # dark steel pads with black and amber hazard edges under the machines
        stripe = np.floor((TX + TY) * 2.2) % 2
        hazard = mix((0.08, 0.07, 0.06), (0.95, 0.62, 0.1), stripe)
        plate = tread_floor(TX, TY, (0.26, 0.29, 0.32), 0.2)
        for (px, py, half, round_pad) in pads:
            d = np.hypot(TX - px, TY - py) if round_pad else np.maximum(np.abs(TX - px), np.abs(TY - py))
            inside = smooth(half + 0.02, half - 0.02, d)
            rgb = mix(rgb, plate, inside)
            rgb = mix(rgb, hazard, inside * smooth(half - 0.2, half - 0.16, d))
        # the conduits: a dark groove, a bright core, its light on the floor around
        dist = np.abs(st.outside_box(TX, TY, ring))
        for (ax, ay), (bx, by) in lines:
            lo_x, hi_x, lo_y, hi_y = min(ax, bx), max(ax, bx), min(ay, by), max(ay, by)
            dx = np.maximum(np.maximum(lo_x - TX, TX - hi_x), 0)
            dy = np.maximum(np.maximum(lo_y - TY, TY - hi_y), 0)
            dist = np.minimum(dist, np.hypot(dx, dy))
        rgb = rgb + np.array([0.1, 0.32, 0.4]) * smooth(0.5, 0.08, dist)[..., None] * 0.35
        rgb = mix(rgb, (0.03, 0.05, 0.07), smooth(0.12, 0.09, dist))
        rgb = mix(rgb, (0.55, 0.95, 1.0), smooth(0.045, 0.025, dist))
        return rgb

    ground, img = terrain(st, height, color, "ground")
    wall = material("tech_wall", (0.5, 0.53, 0.58), 0.06, 1.0, kind="bricks", mortar=(0.26, 0.28, 0.32))
    white = material("console_white", (0.86, 0.87, 0.9), 0.04, 5.0)
    grey = material("console_grey", (0.5, 0.53, 0.58), 0.05, 5.0)
    dark = material("tech_dark", (0.11, 0.12, 0.15), 0.05)
    steel = material("steel", (0.42, 0.45, 0.5), 0.08, 5.0)
    copper = material("coil_copper", (0.78, 0.42, 0.2), 0.1, 6.0)
    hazard = material("hazard_amber", (0.95, 0.66, 0.12), 0.08, 4.0)
    spoke = material("reel_spoke", (0.8, 0.82, 0.86), 0.04)
    pipe = material("pipe_steel", (0.36, 0.4, 0.46), 0.08, 5.0)
    screen = lit_material("screen", (0.08, 0.3, 0.75), 0.25, 9.0, color2=(0.45, 0.85, 1.0))
    alarm = lit_material("alarm_screen", (0.6, 0.04, 0.04), 0.25, 9.0, color2=(1.0, 0.35, 0.2))
    panel = lit_material("desk_panel", (0.05, 0.12, 0.25), 0.2, 14.0, color2=(0.3, 0.7, 0.95))
    charge = lit_material("charge", (0.45, 0.9, 1.0), 0.15, 6.0, color2=(0.85, 1.0, 1.0))
    status = lit_material("status_cyan", (0.4, 0.9, 1.0), 0.05)
    red = lit_material("lamp_red", (1.0, 0.18, 0.12), 0.05)
    amber = lit_material("lamp_amber", (1.0, 0.62, 0.12), 0.05)
    green = lit_material("lamp_green", (0.4, 1.0, 0.55), 0.05)
    hot = lit_material("hot_metal", (1.0, 0.45, 0.1), 0.25, 8.0, color2=(1.0, 0.8, 0.35))
    furnace = lit_material("fan_furnace", (0.6, 0.14, 0.03), 0.25, 6.0, color2=(0.95, 0.45, 0.1))
    lamps = [(red, (1.0, 0.25, 0.15)), (amber, (1.0, 0.62, 0.2)), (green, (0.4, 1.0, 0.55))]
    assert (x0, x1, y0, y1) == room(st, wall, offset_x=offset_x, offset_north=offset_north)

    # behind the field: mainframes and wall screens over desks, the big screen and the reactor in the middle
    for i, x in enumerate(north):
        if i % 2 == 0:
            mainframe(x, y1 - 0.74, z, white, dark, dark, spoke, lamps, status)
        else:
            box((x, y1 - 0.31, z), (1.6, 0.62, 1.1), grey, bevel=0.04)
            for k in range(3):
                color, light = lamps[(i + k) % 3]
                box((x - 0.4 + k * 0.4, y1 - 0.63, z + 0.8), (0.14, 0.04, 0.1), color)
                glow((x - 0.4 + k * 0.4, y1 - 0.7, z + 0.85), light, 0.22, GLOW_BLINK)
            wall_screen(x, y1 - 0.06, z + 1.5, 1.5, 1.0, dark, screen, (0.35, 0.65, 1.0))
        if i % 4 == 1 and abs(x - st.cx) > 4.0:
            control_desk(x - 1.0, APRON + 1.55, z, grey, panel, dark, screen, lamps[:2])
    big = (1.0, 0.2, 0.12) if final_fight else (0.35, 0.65, 1.0)
    wall_screen(st.cx, y1 - 0.06, z + 1.2, 4.4, 2.3, dark, alarm if final_fight else screen, big)
    reactor(st.cx, 3.1, z, s, dark, steel, charge, red, hazard)
    # warning lights: on brackets either side of the big screen and on the floor in the room's back corners
    for bx in (st.cx - 2.75, st.cx + 2.75, x0 + 0.45, x1 - 0.45):
        high = abs(bx - st.cx) < 3
        if high:
            box((bx, y1 - 0.25, z + 2.3), (0.36, 0.5, 0.3), dark)
        beacon(bx, y1 - 0.4, z + 2.6 if high else z, dark, red, (1.0, 0.22, 0.12), 4)
    # an alarm strip along the top of the north wall
    box(((x0 + x1) * 0.5, y1 - 0.03, z + 4.05), (x1 - x0 - 0.2, 0.06, 0.1), red)
    for x in np.arange(x0 + 1.5, x1 - 1.0, 3.0):
        glow((float(x), y1 - 0.12, z + 4.1), (1.0, 0.22, 0.12), 0.5, GLOW_BLINK)

    # the side walls: coils and presses, fans in the floor and capacitors between them
    for i, y in enumerate(sides):
        for x, a in ((x0 + coil_x, 0.0), (x1 - coil_x, math.pi)):
            if i % 2 == 0:
                power_coil(x, y, z, a, dark, copper, charge, red)
            else:
                power_press(x - math.cos(a) * 0.1, y, z, a, dark, steel, hazard, hot, phase=(i * 0.37) % 1.0)
    for i, y in enumerate(fans):
        for k, x in enumerate((x0 + fan_x, x1 - fan_x)):
            if (i + k) % 2 == 0:
                floor_fan(x, y, z, steel, dark, furnace, 5 * (1 if k else -1))
            else:
                capacitors(x, y, z, steel, dark, status)

    # pipes along the walls, clamped, the power running along them
    for (p0, p1) in (((x0 + 0.3, y1 - 0.25), (x1 - 0.3, y1 - 0.25)), ((x0 + 0.25, y1 - 0.3), (x0 + 0.25, y0)),
                     ((x1 - 0.25, y1 - 0.3), (x1 - 0.25, y0))):
        pipe_run(p0, p1, z + 3.7, 0.18, pipe)
        length = math.hypot(p1[0] - p0[0], p1[1] - p0[1])
        for t in np.arange(0.75, length - 0.3, 1.5):
            px = p0[0] + (p1[0] - p0[0]) * t / length
            py = p0[1] + (p1[1] - p0[1]) * t / length
            coupling = cylinder((px, py, z + 3.64), 0.215, 0.12, dark, vertices=12)
            coupling.rotation_euler = (math.pi / 2, 0, math.atan2(p1[1] - p0[1], p1[0] - p0[0]) + math.pi / 2)
            coupling.location.z = z + 3.7
    # the power running along the floor's conduits and the pipes: out from the reactor, round the field
    cyan = (0.3, 0.78, 1.0)
    for x_end in (x0 + bus_x, x1 - bus_x):
        chase((st.cx, bus_y), (x_end, bus_y), z + 0.05, cyan, 0.34)
        chase((x_end, bus_y), (x_end, sides[-1]), z + 0.05, cyan, 0.34)
        wall_x = x0 + 0.25 if x_end < st.cx else x1 - 0.25
        chase((st.cx, y1 - 0.25), (wall_x, y1 - 0.25), z + 3.7, cyan, 0.42, spacing=0.75)
        chase((wall_x, y1 - 0.3), (wall_x, y0 + 2.0), z + 3.7, cyan, 0.42, spacing=0.75)
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


# --- grass ----------------------------------------------------------------------------------------

GRASS_WORLDS = {1, 2, 3, 4}  # 5's green is moss on cave earth, 6..8 are indoors
GRASS_SHADES = 4  # the atlas' columns, light to dark ground
GRASS_CLUMPS = 2000  # at most, three quads each
GRASS_SWAY = 0.06  # how far the blades' tips move at most, tiles


def grassy(rgb):
    """How much a ground colour reads as grass, 0..1: green well above red and blue (not sand, soil or rock)."""
    r, g, b = rgb[..., 0], rgb[..., 1], rgb[..., 2]
    return smooth(1.1, 1.4, g / np.maximum(r, 1e-3)) * smooth(1.2, 1.7, g / np.maximum(b, 1e-3))


def grass_meadow(st, props):
    """Clumps of grass on the grassy ground around the field: thick in the strip the camera looks at, thinning out
    into the land, none on the walkway around the field, on steep ground, under the water or under the props. Made
    after the bakes: they neither occlude nor take a colour from them, the atlas shades them like the ground."""
    from mathutils import Vector
    from mathutils.bvhtree import BVHTree

    height_fn, rgb, slope = GROUND
    rng = np.random.default_rng(st.rng.integers(1 << 30))
    n = len(rgb)

    # candidates on a jittered grid, at most one clump a cell
    cell = 0.3
    X, Y = np.meshgrid(np.arange(st.x0, st.x1, cell), np.arange(st.y0, st.y1, cell))
    X = (X + rng.uniform(0, cell, X.shape)).ravel()
    Y = (Y + rng.uniform(0, cell, Y.shape)).ravel()
    ix = np.clip(((X - st.x0) / (st.x1 - st.x0) * n).astype(int), 0, n - 1)
    iy = np.clip(((Y - st.y0) / (st.y1 - st.y0) * n).astype(int), 0, n - 1)
    ground = rgb[iy, ix]
    d = st.outside(X, Y)
    density = smooth(0.3, 0.7, d) * (0.04 + 0.96 * smooth(5.0, 1.5, d))
    density = density * grassy(ground) * smooth(0.7, 0.4, slope[iy, ix])
    # in patches, a lawn would look like a carpet
    density = density * (0.45 + 0.55 * smooth(0.35, 0.6, fbm(X, Y, 0.45, 3, 61)))
    Z = height_fn(X, Y)
    for water, _ in WATER:
        density = density * (Z > water.data.vertices[0].co.z + 0.12)
    keep = rng.random(X.shape) < density
    if keep.sum() > GRASS_CLUMPS:
        # over the budget the far ones go first
        kept = np.flatnonzero(keep)
        keep[kept[np.argsort(d[kept] + rng.uniform(0, 2.0, len(kept)))[GRASS_CLUMPS:]]] = False
    X, Y, Z, ground = X[keep], Y[keep], Z[keep], ground[keep]

    # not inside or under rocks, walls, trunks and bushes; tree crowns high above are fine
    matrix = props.matrix_world
    tree = BVHTree.FromPolygons([matrix @ v.co for v in props.data.vertices], [p.vertices[:] for p in props.data.polygons])
    free = []
    for x, y, z in zip(X, Y, Z):
        hit, *_ = tree.ray_cast(Vector((x, y, z + 0.7)), Vector((0, 0, -1)), 0.75)
        near = tree.find_nearest(Vector((x, y, z + 0.15)), 0.3)
        free.append(hit is None and near[0] is None)
    free = np.array(free, bool)
    X, Y, Z, ground = X[free], Y[free], Z[free], ground[free]
    if len(X) == 0:
        return None

    # the atlas' shades: the grassy ground's colours from dark to light, each clump takes the nearest
    lum = ground @ np.array([0.3, 0.59, 0.11])
    edges = np.quantile(lum, np.linspace(0, 1, GRASS_SHADES + 1)[1:-1])
    shade = np.digitize(lum, edges)
    colors = [ground[shade == k].mean(axis=0) if (shade == k).any() else ground.mean(axis=0) for k in range(GRASS_SHADES)]

    verts, faces, uvs, waves, normals = [], [], [], [], []
    wind = np.array(WIND) / np.linalg.norm(WIND)
    for x, y, z, k in zip(X, Y, Z, shade):
        size = rng.uniform(0.7, 1.3)
        tall = rng.uniform(0.3, 0.45)
        # the camera looks from the south, down onto the field: a fan of three quads some 50 degrees apart, each
        # one's front towards it, so it sees fronts only (the backs are lit with the normal flipped, which turns
        # them dark in the open); a full star would always turn one quad edge on, its back to half the screen
        facing = math.atan2(-20.0, st.cx - x) + rng.uniform(-0.2, 0.2)
        lean = rng.normal(0, 0.03, 2) + wind[:2] * 0.03
        for q in (-1, 0, 1):
            # along the quad, its front's normal a quarter turn clockwise of that
            a = facing + q * 0.85 + rng.uniform(-0.1, 0.1) + math.pi / 2
            dx, dy = math.cos(a) * 0.26 * size, math.sin(a) * 0.26 * size
            ends = ((x - dx, y - dy), (x + dx, y + dy))
            # each foot just below the ground under it, so the clump stands on slopes too
            feet = [ground_height(st, ex, ey, height_fn) - 0.04 for ex, ey in ends]
            i = len(verts)
            verts += [(ends[0][0], ends[0][1], feet[0]), (ends[1][0], ends[1][1], feet[1]),
                      (ends[1][0] + lean[0], ends[1][1] + lean[1], feet[1] + tall),
                      (ends[0][0] + lean[0], ends[0][1] + lean[1], feet[0] + tall)]
            faces.append((i, i + 1, i + 2, i + 3))
            u0, v0, u1, v1 = grass.cell(int(k), GRASS_SHADES, int(rng.integers(grass.ROWS)), rng.random() < 0.5)
            uvs += [(u0, v0), (u1, v0), (u1, v1), (u0, v1)]
            # the base holds, the tips give
            waves += [0.0, 0.0, 1.0, 1.0]
            # lit nearly like the ground under it: the front's normal bent up
            normal = np.array([math.sin(a) * 0.5, -math.cos(a) * 0.5, 1.0])
            normals += [normal / np.linalg.norm(normal)] * 4

    mesh = bpy.data.meshes.new("grass")
    mesh.from_pydata(verts, [], faces)
    obj = bpy.data.objects.new("grass", mesh)
    bpy.context.scene.collection.objects.link(obj)
    uv = mesh.uv_layers.new(name="UVMap")
    uv.data.foreach_set("uv", np.array(uvs, np.float32).ravel())
    for p in mesh.polygons:
        p.use_smooth = True
    mesh.normals_split_custom_set_from_vertices(normals)
    colors_attr = mesh.color_attributes.new("wave", "FLOAT_COLOR", "CORNER")
    for loop in mesh.loops:
        colors_attr.data[loop.index].color = (wind[0] * 0.5 + 0.5, wind[1] * 0.5 + 0.5, wind[2] * 0.5 + 0.5,
                                              waves[loop.vertex_index])
    mesh.color_attributes.active_color = colors_attr
    # else the exporter puts a white one first
    mesh.color_attributes.render_color_index = mesh.color_attributes.active_color_index
    obj["wave"] = [GRASS_SWAY]
    # the material's colour and cutout go in after the export, see grass.embed()
    mat = bpy.data.materials.new("grass_export")
    mesh.materials.append(mat)
    return grass.atlas(colors)


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
    elif kind == "EMIT":
        bpy.context.scene.cycles.samples = 1
        bpy.ops.object.bake(type="EMIT", margin=4)
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
    # every prop down to the triangles its shape needs
    before = after = 0
    for o in objs:
        b, a = budget.reduce(o)
        before, after = before + b, after + a
    print("RESULT props", before, "->", after, "tris", flush=True)
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
    # the margin a fixed few texels: scaled with the islands (the default), thousands of them leave next to no room
    # for the islands themselves, a crown's face gets less than a texel and shows a neighbour's colour
    bpy.ops.uv.smart_project(angle_limit=1.15, margin_method="FRACTION", island_margin=PROP_MARGIN / PROP_TEX,
                             scale_to_bounds=True)
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
    DETAIL[:] = [detail.story_world(world), detail.story_world(world, props=True)]
    w, h = SIZES[size_key]
    bpy.ops.wm.read_factory_settings(use_empty=True)
    _MATERIALS.clear()
    PROPS.clear()
    WATER.clear()
    SPINNERS.clear()
    CRYSTALS.clear()
    setup_cycles()
    st = Stage(w, h, seed=world * 100 + w + h)
    ground, ground_color = fn(st)
    before, after = budget.reduce(ground, budget.GROUND)
    print("RESULT ground", before, "->", after, "tris", flush=True)

    occluders = border_occluders(w, h)
    for water, _ in WATER:
        water.hide_render = True
    props = join_props("props")
    # the foliage sways, everything else has no "wave" colour and stands still
    if props.data.color_attributes.get("wave"):
        props["wave"] = [SWAY]
        # the game reads the first vertex colour
        for other in [c.name for c in props.data.color_attributes if c.name != "wave"]:
            props.data.color_attributes.remove(props.data.color_attributes[other])
        props.data.color_attributes.active_color = props.data.color_attributes["wave"]
        # else the exporter puts a white one first
        props.data.color_attributes.render_color_index = props.data.color_attributes.active_color_index
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
    props_mask = detail.mask(DETAIL[1], rgb, np.zeros(rgb.shape[:2]))
    # the crystals' texels, they glow: no detail and no occlusion there
    glowing = np.zeros(PROP_TEX * PROP_TEX, bool)
    if CRYSTALS:
        crystal_mask = bpy.data.images.new("crystal_mask", PROP_TEX, PROP_TEX)
        crystal_mask.colorspace_settings.name = "Non-Color"
        crystal_mask.generated_color = (0, 0, 0, 1)
        bake(props, "EMIT", crystal_mask)
        emitted = np.empty(PROP_TEX * PROP_TEX * 4, np.float32)
        crystal_mask.pixels.foreach_get(emitted)
        glowing = emitted.reshape(-1, 4)[:, 0] > 0.5
        props_mask[glowing.reshape(PROP_TEX, PROP_TEX)] = 0
    PROPS_DETAIL[:] = [props_mask.reshape(half, 2, half, 2, 4).mean(axis=(1, 3))]
    props_ao.pixels.foreach_set(np.tile(np.array([1.0, 0.0, 1.0, 1.0], np.float32), AO_TEX * AO_TEX))
    bake(props, "AO", props_ao)
    bake(ground, "AO", ground_ao)
    soft = np.empty(AO_TEX * AO_TEX * 4, np.float32)
    props_ao.pixels.foreach_get(soft)
    soft = soft.reshape(-1, 4)
    soft[(soft[:, 0] > 0.99) & (soft[:, 1] < 0.01) & (soft[:, 2] > 0.99), :3] = 1.0
    soft[:, :3] = 0.4 + 0.6 * soft[:, :3]
    soft[glowing, :3] = 1.0
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
    blades = grass_meadow(st, props) if world in GRASS_WORLDS else None
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
    # the foliage's swaying goes out as the first vertex colour
    options = bpy.ops.export_scene.gltf.get_rna_type().properties
    if "export_vertex_color" in options:
        animation["export_vertex_color"] = "ACTIVE"
    elif "export_colors" in options:
        animation["export_colors"] = True
    bpy.ops.export_scene.gltf(filepath=path, export_format="GLB", use_selection=True, export_yup=True,
                              export_image_format="JPEG", export_jpeg_quality=88, export_extras=True, **animation)
    if SPINNERS:
        merge_animations(path, "spin")
    for material, mask, kind in (("ground_export", GROUND_DETAIL[0], DETAIL[0]), ("props_export", PROPS_DETAIL[0], DETAIL[1])):
        detail.embed(path, {material: mask}, detail.images(kind), detail.repeats(kind), detail.strength(kind))
    if blades is not None:
        grass.embed(path, "grass_export", blades)
    for _, img in WATER:
        glb.replace_image(path, img.name, lossless(img))
    for img in (ground_color, props_color, props_ao, ground_ao, *(img for _, img in WATER)):
        img.pack()
    bpy.context.scene.name = f"world{world}_{title}_{w}x{h}"
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(folder, f"surroundings_{w}x{h}.blend"), compress=True)
    print("RESULT", path, "tris", tris, "kb", os.path.getsize(path) // 1024, flush=True)


for world in (ONLY or sorted(WORLDS)):
    for size_key in [k for k in SIZES if not os.environ.get("SURR_SIZES") or str(k) in os.environ["SURR_SIZES"].split(",")]:
        build(world, size_key)
