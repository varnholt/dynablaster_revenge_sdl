# The pirate ship level: the deck of a ship at night after "The Ambitious Pirate Level" sketch, colour and ambient
# occlusion baked into textures like the story mode's surroundings. Run headless:
#   blender --background --factory-startup --python build_pirate.py -- <out dir>
# or, for a look at the unbaked scene in a running Blender, exec() it with PIRATE_PREVIEW = True set beforehand.
# Units are tiles, the field covers x 0..13, y -11..0 (top left corner at the origin), z up, its floor's top at 0.
# The game draws the result untransformed, the 19x17 field is scaled into the same 13x11 area.
import math
import os
import sys

import bmesh
import bpy
import numpy as np
from mathutils import Matrix, Vector

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
import glb  # noqa: E402

ARGS = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
OUT = ARGS[0] if ARGS else os.path.join(os.path.dirname(os.path.abspath(__file__)), "out")
PREVIEW = globals().get("PIRATE_PREVIEW", False) or bool(os.environ.get("PIRATE_PREVIEW"))

W, H = 13, 11
DECK_Z = -0.08
POOP_X = 16.6  # the stern deck's wall facing the field
POOP_Z = 2.6
RAIL_Y = 3.2  # inner face of the bulwark behind the field
SEA_Z = -2.4
WALL = 0.35  # the bulwarks' thickness
YC = -5.5  # the ship's centre line, through the field's middle
HB = RAIL_Y + WALL - YC  # half the beam, outside the bulwarks
DECK_X0, DECK_X1 = -11.0, 24.0  # the main deck's planar mapped part, the stern
DECK_Y0 = YC - HB + WALL  # inner face of the bulwark on the viewer's side
BOW_X = -31.0  # the stem at the waterline
TAPER_X = -10.0  # where the sides start curving in towards the bow
STERN_X = DECK_X1 + 0.1
DECK_TEX = (4096, 2048)
EXTERIOR_TEX = 2048
CLOTH_TEX = 2048
# from the field towards the moon, high in the north west: the mast's shadow falls across the field
MOON = Vector((-0.42, 0.5, 0.76)).normalized()
PROP_TEX = 4096
AO_TEX = 2048

rng = np.random.default_rng(7)


# --- materials ------------------------------------------------------------------------------------

_MATERIALS = {}


def _linear(color):
    return tuple(c ** 2.2 for c in color)


def material(name, color, variation=0.12, scale=3.0, kind="noise", axes="xy", plank=(2.6, 0.32), seam=0.35):
    """Procedural colour (baked later): noise, or planks along the first of axes."""
    if name in _MATERIALS:
        return _MATERIALS[name]
    color = _linear(color)
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = next(n for n in nodes if n.type == "BSDF_PRINCIPLED")
    bsdf.inputs["Roughness"].default_value = 0.8
    geometry = nodes.new("ShaderNodeNewGeometry")
    low = tuple(max(0.0, c * (1 - variation)) for c in color) + (1,)
    high = tuple(min(1.0, c * (1 + variation)) for c in color) + (1,)

    # wood grain: stretched noise along the planks
    sep = nodes.new("ShaderNodeSeparateXYZ")
    links.new(geometry.outputs["Position"], sep.inputs[0])
    index = {"x": 0, "y": 1, "z": 2}
    comb = nodes.new("ShaderNodeCombineXYZ")
    links.new(sep.outputs[index[axes[0]]], comb.inputs[0])
    links.new(sep.outputs[index[axes[1]]], comb.inputs[1])
    grain = nodes.new("ShaderNodeMapping")
    grain.inputs["Scale"].default_value = (0.25, 6.0, 1.0) if kind == "planks" else (1.0, 1.0, 1.0)
    links.new(comb.outputs[0], grain.inputs["Vector"])
    noise = nodes.new("ShaderNodeTexNoise")
    noise.inputs["Scale"].default_value = scale
    noise.inputs["Detail"].default_value = 6.0
    links.new(grain.outputs[0], noise.inputs["Vector"])
    ramp = nodes.new("ShaderNodeValToRGB")
    ramp.color_ramp.elements[0].color = low
    ramp.color_ramp.elements[1].color = high
    ramp.color_ramp.elements[0].position = 0.3
    ramp.color_ramp.elements[1].position = 0.7
    links.new(noise.outputs["Fac"], ramp.inputs["Fac"])

    if kind == "planks":
        length, width = plank
        brick = nodes.new("ShaderNodeTexBrick")
        brick.offset = 0.37
        brick.inputs["Scale"].default_value = 1.0
        brick.inputs["Brick Width"].default_value = length
        brick.inputs["Row Height"].default_value = width
        brick.inputs["Mortar Size"].default_value = 0.012
        brick.inputs["Mortar Smooth"].default_value = 0.3
        brick.inputs["Bias"].default_value = 0.0
        # every plank a little lighter or darker
        brick.inputs["Color1"].default_value = (0.82, 0.82, 0.82, 1)
        brick.inputs["Color2"].default_value = (1.12, 1.12, 1.12, 1)
        brick.inputs["Mortar"].default_value = (0.0, 0.0, 0.0, 1)
        links.new(comb.outputs[0], brick.inputs["Vector"])
        tone = nodes.new("ShaderNodeMix")
        tone.data_type = "RGBA"
        tone.blend_type = "MULTIPLY"
        tone.inputs["Factor"].default_value = 1.0
        links.new(ramp.outputs["Color"], tone.inputs["A"])
        links.new(brick.outputs["Color"], tone.inputs["B"])
        gap = nodes.new("ShaderNodeMix")
        gap.data_type = "RGBA"
        links.new(brick.outputs["Fac"], gap.inputs["Factor"])
        links.new(tone.outputs["Result"], gap.inputs["A"])
        gap.inputs["B"].default_value = tuple(c * seam for c in color) + (1,)
        links.new(gap.outputs["Result"], bsdf.inputs["Base Color"])
    else:
        links.new(ramp.outputs["Color"], bsdf.inputs["Base Color"])

    _MATERIALS[name] = mat
    return mat


def palette():
    return dict(
        deck=material("deck", (0.43, 0.28, 0.14), 0.18, 2.0, "planks", plank=(3.1, 0.34)),
        field=material("field", (0.70, 0.45, 0.20), 0.10, 1.5, "planks", plank=(4.4, 0.5), seam=0.7),
        frame=material("frame", (0.46, 0.08, 0.06), 0.15, 3.0, "planks", plank=(6.0, 0.4)),
        hull=material("hull", (0.30, 0.17, 0.09), 0.15, 2.0, "planks", axes="xz", plank=(3.4, 0.3)),
        wall=material("wall", (0.40, 0.23, 0.12), 0.15, 2.0, "planks", axes="zy", plank=(3.0, 0.32)),
        stern_side=material("stern_side", (0.40, 0.23, 0.12), 0.15, 2.0, "planks", axes="xz", plank=(3.0, 0.32)),
        redwood=material("redwood", (0.52, 0.17, 0.10), 0.12, 4.0),
        wood=material("wood", (0.46, 0.27, 0.13), 0.15, 4.0),
        darkwood=material("darkwood", (0.24, 0.13, 0.07), 0.15, 4.0),
        stave=material("stave", (0.55, 0.28, 0.12), 0.15, 2.0, "planks", axes="zx", plank=(5.0, 0.18)),
        gold=material("gold", (0.80, 0.62, 0.24), 0.10, 8.0),
        iron=material("iron", (0.08, 0.08, 0.09), 0.25, 9.0),
        brass=material("brass", (0.72, 0.56, 0.26), 0.10, 9.0),
        sail=material("sail", (0.86, 0.80, 0.66), 0.08, 7.0),
        darkrope=material("darkrope", (0.30, 0.21, 0.12), 0.15, 12.0),
        transom=material("transom", (0.52, 0.27, 0.13), 0.15, 2.0, "planks", axes="yz", plank=(3.0, 0.3)),
        planking=material("planking", (0.58, 0.31, 0.15), 0.16, 2.0, "planks", axes="xz", plank=(3.6, 0.28)),
        wale=material("wale", (0.27, 0.14, 0.07), 0.15, 4.0),
        bottom=material("bottom", (0.44, 0.24, 0.12), 0.16, 2.0, "planks", axes="xz", plank=(3.6, 0.3)),
        black=material("black", (0.05, 0.05, 0.06), 0.2, 6.0),
        sailcloth=material("sailcloth", (0.84, 0.78, 0.64), 0.1, 5.0),
        redcross=material("redcross", (0.62, 0.08, 0.06), 0.1, 5.0),
        seam=material("seam", (0.8, 0.74, 0.6), 0.08, 5.0),
        boltrope=material("boltrope", (0.62, 0.52, 0.36), 0.1, 9.0),
        bone=material("bone", (0.92, 0.9, 0.84), 0.05, 6.0),
        rope=material("rope", (0.46, 0.34, 0.19), 0.15, 12.0),
        green=material("green", (0.18, 0.62, 0.26), 0.08, 9.0),
        glass=material("glass", (0.06, 0.10, 0.09), 0.10, 6.0),
        lamp=material("lamp", (1.00, 0.86, 0.50), 0.05, 6.0),
        hole=material("hole", (0.03, 0.02, 0.015), 0.10, 6.0),
    )


# --- primitive props ------------------------------------------------------------------------------

PROPS = []
# the ship's outside, masts and sails: seen in the fly-in only, baked into a texture of its own
EXTERIOR = []
# sails and the flag, waving in the wind (GltfModel's "wave"): a node and a texture of their own
CLOTH = []
_TARGET = [PROPS]


def _finish(obj, mat, smooth_shade=True):
    obj.data.materials.append(mat)
    for p in obj.data.polygons:
        p.use_smooth = smooth_shade
    _TARGET[0].append(obj)
    return obj


def exterior(build_parts, *args):
    """Builds into EXTERIOR instead of PROPS."""
    _TARGET[0] = EXTERIOR
    try:
        return build_parts(*args)
    finally:
        _TARGET[0] = PROPS


def _mesh(name, verts, faces, mat, smooth_shade=True):
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata([tuple(v) for v in verts], [], faces)
    mesh.validate()
    # closed meshes face outwards whichever way their faces were wound; the bake starts its rays there
    bm = bmesh.new()
    bm.from_mesh(mesh)
    if all(len(edge.link_faces) == 2 for edge in bm.edges):
        bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
        bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    return _finish(obj, mat, smooth_shade)


def box(center, size, mat, rotation=0.0, bevel=0.0):
    """Centred on center, turned about z."""
    bpy.ops.mesh.primitive_cube_add(size=1, location=center)
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


def slab(x0, x1, y0, y1, z0, z1, mat, bevel=0.0):
    return box(((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2), (x1 - x0, y1 - y0, z1 - z0), mat, bevel=bevel)


def sphere(loc, radius, mat, scale=(1, 1, 1), subdiv=2):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=subdiv, radius=radius, location=loc)
    obj = bpy.context.object
    obj.scale = scale
    return _finish(obj, mat)


def _orient(p0, p1):
    """Matrix taking the z axis from p0 to p1."""
    p0, p1 = Vector(p0), Vector(p1)
    direction = p1 - p0
    rotation = direction.to_track_quat("Z", "Y").to_matrix().to_4x4()
    return Matrix.Translation(p0) @ rotation, direction.length


def lathe(p0, p1, profile, mat, segments=16, smooth_shade=True, caps=True):
    """Turned along p0 -> p1, profile: (t 0..1 along it, radius) pairs."""
    transform, length = _orient(p0, p1)
    verts, faces = [], []
    for t, r in profile:
        for s in range(segments):
            a = 2 * math.pi * s / segments
            verts.append(transform @ Vector((r * math.cos(a), r * math.sin(a), t * length)))
    rings = len(profile)
    for i in range(rings - 1):
        for s in range(segments):
            a, b = i * segments + s, i * segments + (s + 1) % segments
            faces.append((a, b, b + segments, a + segments))
    if caps:
        faces.append(tuple(reversed(range(segments))))
        faces.append(tuple(range((rings - 1) * segments, rings * segments)))
    return _mesh("lathe", verts, faces, mat, smooth_shade)


def rod(p0, p1, radius, mat, segments=10):
    return lathe(p0, p1, ((0, radius), (1, radius)), mat, segments)


def bar(p0, p1, width, height, mat):
    """A square beam from p0 to p1."""
    transform, length = _orient(p0, p1)
    w, h = width / 2, height / 2
    corners = [(-w, -h), (w, -h), (w, h), (-w, h)]
    verts = [transform @ Vector((x, y, z)) for z in (0, length) for x, y in corners]
    faces = [(0, 1, 2, 3)[::-1], (4, 5, 6, 7), (0, 1, 5, 4), (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7)]
    return _mesh("bar", verts, faces, mat, smooth_shade=False)


def torus(loc, major, minor, mat, normal=(0, 0, 1)):
    bpy.ops.mesh.primitive_torus_add(major_radius=major, minor_radius=minor, major_segments=20, minor_segments=8,
                                     location=loc)
    obj = bpy.context.object
    obj.rotation_euler = Vector(normal).to_track_quat("Z", "Y").to_euler()
    return _finish(obj, mat)


GLOW_STEADY, GLOW_FLAME, GLOW_BLINK, GLOW_PULSE = 0, 1, 2, 3
GLOWS = []


def glow(loc, color, size, mode=GLOW_STEADY):
    """A light's halo for the game (GltfModel::Glow): an empty with the glow in its extras."""
    obj = bpy.data.objects.new("glow", None)
    obj.location = loc
    obj["glow"] = [float(c) for c in color] + [float(size), float(mode)]
    bpy.context.scene.collection.objects.link(obj)
    GLOWS.append(obj)
    return obj


# --- the deck -------------------------------------------------------------------------------------

def deck(m):
    """Floor of the field and the deck around it as one flat mesh, mapped from above."""
    frame = 0.42
    fx0, fx1, fy0, fy1 = -frame, W + frame, -H - frame, frame
    rects = [
        (0, W, -H, 0, 0.0, "field"),
        (DECK_X0, fx0, DECK_Y0, RAIL_Y, DECK_Z, "deck"),
        (fx1, POOP_X, DECK_Y0, RAIL_Y, DECK_Z, "deck"),
        (fx0, fx1, fy1, RAIL_Y, DECK_Z, "deck"),
        (fx0, fx1, DECK_Y0, fy0, DECK_Z, "deck"),
    ]
    mesh = bpy.data.meshes.new("deck")
    bm = bmesh.new()
    for x0, x1, y0, y1, z, which in rects:
        # subdivided, the bake and the vertex based fog work on small quads
        nx, ny = max(1, round(x1 - x0)), max(1, round(y1 - y0))
        grid = [[bm.verts.new((x0 + (x1 - x0) * i / nx, y0 + (y1 - y0) * j / ny, z)) for i in range(nx + 1)]
                for j in range(ny + 1)]
        for j in range(ny):
            for i in range(nx):
                face = bm.faces.new((grid[j][i], grid[j][i + 1], grid[j + 1][i + 1], grid[j + 1][i]))
                face.material_index = 0 if which == "field" else 1
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new("deck", mesh)
    bpy.context.scene.collection.objects.link(obj)
    mesh.materials.append(m["field"])
    mesh.materials.append(m["deck"])
    uv = mesh.uv_layers.new(name="UVMap")
    co = np.array([mesh.vertices[l.vertex_index].co[:] for l in mesh.loops])
    u = (co[:, 0] - DECK_X0) / (POOP_X - DECK_X0)
    v = (co[:, 1] - DECK_Y0) / (RAIL_Y - DECK_Y0)
    uv.data.foreach_set("uv", np.stack([u, v], 1).ravel())

    # the field's frame: a low red beam with mitred corners and brass corner plates
    top = 0.07
    for (x0, x1, y0, y1) in ((fx0, fx1, 0, fy1), (fx0, fx1, fy0, -H), (fx0, 0, -H, 0), (W, fx1, -H, 0)):
        slab(x0, x1, y0, y1, DECK_Z, top, m["frame"], bevel=0.03)
    for cx, cy in ((fx0, fy1), (fx1, fy1), (fx0, fy0), (fx1, fy0)):
        sx = 1 if cx > 0 else -1
        sy = 1 if cy > -H / 2 else -1
        verts = [(cx, cy, top + 0.006), (cx - sx * 0.55, cy, top + 0.006), (cx, cy - sy * 0.55, top + 0.006)]
        # facing up
        _mesh("plate", verts, [(0, 1, 2) if sx * sy > 0 else (0, 2, 1)], m["brass"], smooth_shade=False)
        sphere((cx - sx * 0.12, cy - sy * 0.12, top + 0.01), 0.04, m["iron"], subdiv=1)
    return obj


def bulwark(m, gunports):
    """The ship's side behind the field: planked wall, posts, a gold capped rail, gunports cut in."""
    x0, x1 = DECK_X0, POOP_X
    thick = 0.35
    height = 1.25
    cuts = sorted(gunports)
    spans, start = [], x0
    for gx, gw in cuts:
        spans.append((start, gx - gw / 2))
        start = gx + gw / 2
    spans.append((start, x1))
    for a, b in spans:
        slab(a, b, RAIL_Y, RAIL_Y + thick, DECK_Z, height, m["hull"])
    for gx, gw in cuts:
        slab(gx - gw / 2, gx + gw / 2, RAIL_Y, RAIL_Y + thick, DECK_Z, 0.22, m["hull"])
        slab(gx - gw / 2, gx + gw / 2, RAIL_Y, RAIL_Y + thick, 0.95, height, m["hull"])
        slab(gx - gw / 2, gx + gw / 2, RAIL_Y + thick - 0.02, RAIL_Y + thick + 0.25, -0.6, 0.0, m["darkwood"])
    # cap rail
    slab(x0, x1, RAIL_Y - 0.08, RAIL_Y + thick + 0.08, height, height + 0.12, m["gold"], bevel=0.03)
    # posts
    x = x0 + 0.6
    while x < x1 - 0.3:
        if all(abs(x - gx) > gw / 2 + 0.2 for gx, gw in cuts):
            slab(x - 0.11, x + 0.11, RAIL_Y - 0.14, RAIL_Y, DECK_Z, height, m["wood"])
            sphere((x, RAIL_Y + thick * 0.5, height + 0.13), 0.05, m["brass"], subdiv=1)
        x += 1.15
    # waterway along the deck
    slab(x0, x1, RAIL_Y - 0.3, RAIL_Y, DECK_Z, 0.0, m["darkwood"])


def side_bulwarks(m):
    """The ship's side behind the viewer, like the one behind the field without gunports, and the bow's."""
    height = 1.25
    x0, y0 = DECK_X0, DECK_Y0
    slab(x0, POOP_X, y0 - WALL, y0, DECK_Z, height, m["hull"])
    slab(x0, POOP_X, y0 - WALL - 0.08, y0 + 0.08, height, height + 0.12, m["gold"], bevel=0.03)
    x = x0 + 0.6
    while x < POOP_X - 0.3:
        slab(x - 0.11, x + 0.11, y0, y0 + 0.14, DECK_Z, height, m["wood"])
        x += 1.15
    exterior(bow, m)


FORECASTLE_X = -21.0  # the forecastle's front wall, facing aft
FORE_Z = 4.6  # the forecastle's deck
AFT_X = 20.0  # the aftcastle's upper tier starts here
AFT_Z = POOP_Z + 2.6


def smoothstep(t):
    t = min(max(t, 0.0), 1.0)
    return t * t * (3 - 2 * t)


def half_beam(x):
    """Half the ship's width at its rail at x: straight along the waist, a full, round bow forward."""
    x = np.asarray(x, dtype=float)
    t = np.clip((TAPER_X - x) / (TAPER_X - BOW_X), 0, 1)
    return HB * np.clip(1 - t ** 2.2, 0, 1) ** 0.45


def sheer(x):
    """Height of the ship's side at x, after the Santa Maria: the low waist (where the game is played) sweeping up
    into the high forecastle, the stern deck's rail and the aftcastle's upper tier aft (railed on top)."""
    if x >= AFT_X:
        return AFT_Z + 0.35 + 0.35 * (x - AFT_X) / (STERN_X - AFT_X)
    if x >= POOP_X:
        return POOP_Z + 0.95
    if x <= FORECASTLE_X:
        return FORE_Z + 0.4 + 0.9 * ((FORECASTLE_X - x) / (FORECASTLE_X - BOW_X)) ** 1.6
    return 1.37 + (FORE_Z + 0.4 - 1.37) * smoothstep((DECK_X0 - x) / (DECK_X0 - FORECASTLE_X))


def bow_rake(x, z):
    """How far the stem leans forward at height z: the forecastle overhangs the bow."""
    t = max(0.0, (TAPER_X - x) / (TAPER_X - BOW_X))
    return -(z + 5.4) * 0.5 * t ** 3


def rail_sections():
    xs = np.linspace(DECK_X0, BOW_X + 1.0, 52)
    return sorted(set(np.round(np.concatenate([xs, [FORECASTLE_X + 0.02, FORECASTLE_X - 0.02]]), 3)), reverse=True)


def rail_run(m, points, height=0.85, spacing=0.45):
    """An open rail along the points (on the castles): turned balusters and a rail on top."""
    for a, b in zip(points, points[1:]):
        count = max(1, int((b - a).length / spacing))
        for i in range(count):
            baluster(m, tuple(a.lerp(b, i / count)), height)
        bar(a + Vector((0, 0, height)), b + Vector((0, 0, height)), 0.13, 0.09, m["redwood"])
    baluster(m, tuple(points[-1]), height)


def bow(m):
    """The deck forward of the planar mapped part, rising with the waist's sheer, the forecastle over the bow."""
    xs = rail_sections()
    inner = [float(half_beam(x)) - WALL for x in xs]

    def deck_strip(name, z, keep):
        verts, faces = [], []
        for x, b in zip(xs, inner):
            if not keep(x):
                continue
            x_top = x + bow_rake(x, z)
            verts += [(x_top, YC + max(b, 0.02), z), (x_top, YC - max(b, 0.02), z)]
            if len(verts) > 2:
                i = len(verts) // 2 - 1
                faces.append((2 * i - 2, 2 * i - 1, 2 * i + 1, 2 * i))
        part = _mesh(name, verts, faces, m["deck"], smooth_shade=False)
        for poly in part.data.polygons:
            if poly.normal.z < 0:
                poly.flip()

    deck_strip("bow_deck", DECK_Z, lambda x: x >= FORECASTLE_X - 0.05)
    deck_strip("forecastle_deck", FORE_Z, lambda x: x <= FORECASTLE_X + 0.05)
    # the forecastle's front wall with its door, windows, two ladders up and the rail on its edge
    b = float(half_beam(FORECASTLE_X)) - WALL
    slab(FORECASTLE_X, FORECASTLE_X + 0.3, YC - b, YC + b, DECK_Z, FORE_Z, m["wall"])
    slab(FORECASTLE_X + 0.3, FORECASTLE_X + 0.42, YC - b, YC + b, FORE_Z - 0.25, FORE_Z, m["redwood"], bevel=0.03)
    slab(FORECASTLE_X + 0.3, FORECASTLE_X + 0.36, YC - 0.7, YC + 0.7, DECK_Z, 2.0, m["redwood"])
    for y in (YC - 2.0, YC + 2.0):
        slab(FORECASTLE_X + 0.3, FORECASTLE_X + 0.36, y - 0.45, y + 0.45, 1.0, 1.8, m["glass"])
        glow((FORECASTLE_X + 0.6, y, 1.4), (1.0, 0.68, 0.34), 0.9, GLOW_PULSE)
    for y in (YC - 5.0, YC + 5.0):
        steps = 11
        for k in range(steps):
            x = FORECASTLE_X + 0.3 + (steps - k) * 0.42
            z = DECK_Z + (k + 1) * (FORE_Z - DECK_Z) / steps
            slab(x - 0.42, x, y - 0.65, y + 0.65, z - 0.12, z, m["redwood"])
        for side in (-1, 1):
            bar(Vector((FORECASTLE_X + 0.3 + steps * 0.42, y + side * 0.7, DECK_Z + 0.9)),
                Vector((FORECASTLE_X + 0.3, y + side * 0.7, FORE_Z + 0.9)), 0.08, 0.08, m["redwood"])
            # stringers carrying the steps
            bar(Vector((FORECASTLE_X + 0.3 + steps * 0.42 + 0.1, y + side * 0.68, DECK_Z + 0.05)),
                Vector((FORECASTLE_X + 0.25, y + side * 0.68, FORE_Z - 0.1)), 0.1, 0.32, m["darkwood"])
            for k in range(0, steps, 3):
                x = FORECASTLE_X + 0.3 + (steps - k) * 0.42
                z = DECK_Z + (k + 1) * (FORE_Z - DECK_Z) / steps
                bar(Vector((x, y + side * 0.6, z + 0.02)), Vector((x, y + side * 0.6, z + 0.9)), 0.05, 0.05,
                    m["redwood"])
    # the ship's bell in its frame on the forecastle's edge
    bx = FORECASTLE_X - 0.5
    for side in (-1, 1):
        slab(bx - 0.08, bx + 0.08, YC + side * 0.45 - 0.08, YC + side * 0.45 + 0.08, FORE_Z, FORE_Z + 1.6,
             m["darkwood"])
    slab(bx - 0.1, bx + 0.1, YC - 0.6, YC + 0.6, FORE_Z + 1.6, FORE_Z + 1.78, m["darkwood"], bevel=0.02)
    lathe(Vector((bx, YC, FORE_Z + 0.85)), Vector((bx, YC, FORE_Z + 1.55)),
          ((0, 0.28), (0.15, 0.26), (0.5, 0.18), (0.9, 0.15), (1, 0.06)), m["brass"], 14)
    rod(Vector((bx, YC, FORE_Z + 0.95)), Vector((bx + 0.6, YC + 0.3, FORE_Z + 0.4)), 0.025, m["darkrope"], 4)
    rail_run(m, [Vector((FORECASTLE_X + 0.15, YC - b, FORE_Z)), Vector((FORECASTLE_X + 0.15, YC + b, FORE_Z))])
    # bulwarks along both sides: planked up to the sheer, the forecastle's railed on top
    for side in (1, -1):
        rails = []
        for i in range(len(xs) - 1):
            xa, xc = xs[i], xs[i + 1]
            top_a, top_c = sheer(xa) - 0.12, sheer(xc) - 0.12
            floor = FORE_Z if xc < FORECASTLE_X else DECK_Z
            a = Vector((xa + bow_rake(xa, top_a), YC + side * (inner[i] + WALL / 2), 0.0))
            c = Vector((xc + bow_rake(xc, top_c), YC + side * (inner[i + 1] + WALL / 2), 0.0))
            mid = (top_a + top_c) / 2
            bar(Vector((a.x, a.y, (floor + mid) / 2)), Vector((c.x, c.y, (floor + mid) / 2)), WALL, mid - floor,
                m["hull"])
            castle = xc < FORECASTLE_X
            bar(Vector((a.x, a.y, top_a + 0.06)), Vector((c.x, c.y, top_c + 0.06)), WALL + 0.16, 0.12,
                m["redwood" if castle else "gold"])
            if castle:
                rails.append(Vector((a.x, a.y, top_a + 0.12)))
        if rails:
            rail_run(m, rails)


def aftcastle(m):
    """The aftcastle's upper tier over the stern deck's after part: a cabin with lit windows, railed all round."""
    y0, y1 = DECK_Y0, RAIL_Y
    slab(AFT_X, STERN_X, y0, y1, POOP_Z, AFT_Z - 0.1, m["wall"])
    slab(AFT_X - 0.1, STERN_X, y0, y1, AFT_Z - 0.1, AFT_Z, m["deck"])
    slab(AFT_X - 0.22, AFT_X, y0, y1, AFT_Z - 0.32, AFT_Z - 0.1, m["redwood"], bevel=0.03)
    for y in (y0 + 1.6, YC - 4.0, YC + 4.0, y1 - 1.6):
        z0, z1 = POOP_Z + 0.55, POOP_Z + 1.5
        slab(AFT_X - 0.1, AFT_X, y - 0.68, y + 0.68, z0 - 0.1, z1 + 0.1, m["gold"], bevel=0.02)
        slab(AFT_X - 0.13, AFT_X - 0.08, y - 0.55, y + 0.55, z0, z1, m["lamp"])
        bar(Vector((AFT_X - 0.16, y, z0)), Vector((AFT_X - 0.16, y, z1)), 0.05, 0.05, m["darkwood"])
        bar(Vector((AFT_X - 0.16, y - 0.55, (z0 + z1) / 2)), Vector((AFT_X - 0.16, y + 0.55, (z0 + z1) / 2)), 0.05,
            0.05, m["darkwood"])
        slab(AFT_X - 0.25, AFT_X, y - 0.75, y + 0.75, z0 - 0.2, z0 - 0.1, m["redwood"])
        glow((AFT_X - 0.4, y, (z0 + z1) / 2), (1.0, 0.7, 0.35), 0.9, GLOW_PULSE)
    # the door, framed, with its ring
    slab(AFT_X - 0.08, AFT_X, YC - 0.6, YC + 0.6, POOP_Z, POOP_Z + 1.8, m["redwood"])
    for yy in (YC - 0.68, YC + 0.68):
        slab(AFT_X - 0.16, AFT_X, yy - 0.08, yy + 0.08, POOP_Z, POOP_Z + 1.9, m["darkwood"])
    slab(AFT_X - 0.16, AFT_X, YC - 0.76, YC + 0.76, POOP_Z + 1.82, POOP_Z + 1.98, m["darkwood"])
    for zz in (POOP_Z + 0.4, POOP_Z + 1.4):
        slab(AFT_X - 0.12, AFT_X - 0.06, YC - 0.5, YC + 0.2, zz, zz + 0.08, m["iron"])
    torus((AFT_X - 0.14, YC - 0.35, POOP_Z + 0.9), 0.08, 0.018, m["iron"], normal=(1, 0, 0))
    # the rail round the roof, on the sides on top of the hull
    z_side = lambda x: sheer(x)
    rail_run(m, [Vector((AFT_X + 0.1, y0 + 0.1, AFT_Z)), Vector((AFT_X + 0.1, y1 - 0.1, AFT_Z))])
    for side_y in (YC + HB - 0.2, YC - HB + 0.2):
        rail_run(m, [Vector((x, side_y, z_side(x))) for x in np.linspace(AFT_X + 0.2, STERN_X - 0.2, 6)])
    rail_run(m, [Vector((STERN_X - 0.2, y, sheer(STERN_X - 0.2))) for y in (y0 + 0.1, y1 - 0.1)])


# --- the stern deck -------------------------------------------------------------------------------

def lattice_window(m, y, z, width, height):
    """A window in the stern wall (facing -x): frame, dark glass and a green diamond lattice."""
    x = POOP_X - 0.02
    slab(x - 0.12, x, y - width / 2 - 0.12, y + width / 2 + 0.12, z - 0.12, z + height + 0.12, m["darkwood"])
    slab(x - 0.13, x - 0.12, y - width / 2, y + width / 2, z, z + height, m["glass"])
    xl = x - 0.16
    step = 0.32
    y0, y1, z0, z1 = y - width / 2, y + width / 2, z, z + height
    for direction in (1, -1):
        c = -height - width
        while c < height + width:
            # the line z - z0 = direction * (y - y0) + c, clipped to the window
            points = []
            for yy in (y0, y1):
                zz = z0 + direction * (yy - y0) + c
                if z0 - 1e-6 <= zz <= z1 + 1e-6:
                    points.append((yy, zz))
            for zz in (z0, z1):
                yy = y0 + (zz - z0 - c) * direction
                if y0 - 1e-6 <= yy <= y1 + 1e-6:
                    points.append((yy, zz))
            if len(points) >= 2:
                (ya, za), (yb, zb) = points[0], points[-1]
                if math.hypot(yb - ya, zb - za) > 0.05:
                    bar((xl, ya, za), (xl, yb, zb), 0.05, 0.05, m["green"])
            c += step
    for yy in (y0, y1):
        bar((xl - 0.02, yy, z0), (xl - 0.02, yy, z1), 0.09, 0.09, m["green"])
    for zz in (z0, z1):
        bar((xl - 0.02, y0, zz), (xl - 0.02, y1, zz), 0.09, 0.09, m["green"])


def door(m, y, width=1.3, height=2.0):
    x = POOP_X - 0.02
    z0 = DECK_Z
    # planks with a round top
    segments = 12
    r = width / 2
    verts = [(x - 0.06, y - r, z0), (x - 0.06, y + r, z0)]
    for i in range(segments + 1):
        a = math.pi * i / segments
        verts.append((x - 0.06, y + r * math.cos(a), z0 + height - r + r * math.sin(a)))
    faces = [tuple(range(len(verts)))[::-1]]
    _mesh("door", verts, faces, m["redwood"], smooth_shade=False)
    # frame arch
    for i in range(segments):
        a0, a1 = math.pi * i / segments, math.pi * (i + 1) / segments
        bar((x - 0.09, y + (r + 0.08) * math.cos(a0), z0 + height - r + (r + 0.08) * math.sin(a0)),
            (x - 0.09, y + (r + 0.08) * math.cos(a1), z0 + height - r + (r + 0.08) * math.sin(a1)), 0.16, 0.12,
            m["darkwood"])
    for yy in (y - r - 0.08, y + r + 0.08):
        bar((x - 0.09, yy, z0), (x - 0.09, yy, z0 + height - r), 0.16, 0.12, m["darkwood"])
    # vertical plank seams, hinges and a ring
    for k in (-2, -1, 0, 1, 2):
        yy = y + k * width / 5.5
        top = z0 + height - r + math.sqrt(max(r * r - (yy - y) ** 2, 0)) - 0.03
        bar((x - 0.065, yy, z0 + 0.02), (x - 0.065, yy, top), 0.012, 0.03, m["darkwood"])
    for zz in (z0 + 0.4, z0 + 1.3):
        bar((x - 0.08, y + r - 0.05, zz), (x - 0.08, y - 0.1, zz), 0.03, 0.09, m["iron"])
    torus((x - 0.12, y - r * 0.55, z0 + 0.95), 0.09, 0.02, m["iron"], normal=(1, 0, 0))


def lantern(m, y, z):
    x = POOP_X - 0.02
    bar((x, y, z + 0.55), (x - 0.38, y, z + 0.55), 0.05, 0.05, m["iron"])
    rod((x - 0.34, y, z + 0.55), (x - 0.34, y, z + 0.42), 0.012, m["iron"], 6)
    lathe((x - 0.34, y, z - 0.05), (x - 0.34, y, z + 0.42),
          ((0, 0.0), (0.05, 0.12), (0.15, 0.14), (0.75, 0.14), (0.85, 0.1), (1.0, 0.03)), m["iron"], 8)
    sphere((x - 0.34, y, z + 0.15), 0.115, m["lamp"], scale=(1, 1, 1.4))
    glow((x - 0.34, y, z + 0.17), (1.0, 0.62, 0.3), 0.9, GLOW_FLAME)


def stairs(m, center, radius, a0, a1, steps, width=1.15):
    """Curved stairs from the deck up to the stern deck, filled down to the deck, a balustrade on the outside."""
    cx, cy = center
    rise = (POOP_Z - DECK_Z) / steps
    rail = []
    for i in range(steps):
        a = a0 + (a1 - a0) * (i + 0.5) / steps
        da = abs(a1 - a0) / steps
        z1 = DECK_Z + rise * (i + 1)
        r0, r1 = radius - width / 2, radius + width / 2
        corners = []
        for rr in (r0, r1):
            for aa in (a - da / 2 - 0.01, a + da / 2 + 0.01):
                corners.append((cx + rr * math.cos(aa), cy + rr * math.sin(aa)))
        (p0, p1, p2, p3) = corners
        quad = [p0, p1, p3, p2]
        verts = [(x, y, DECK_Z) for x, y in quad] + [(x, y, z1) for x, y in quad]
        faces = [(3, 2, 1, 0), (4, 5, 6, 7), (0, 1, 5, 4), (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7)]
        _mesh("step", verts, faces, m["redwood"], smooth_shade=False)
        # tread nosing
        ro = r1 + 0.02
        inner = (cx + (r0 - 0.02) * math.cos(a - da / 2), cy + (r0 - 0.02) * math.sin(a - da / 2), z1 + 0.02)
        outer = (cx + ro * math.cos(a - da / 2), cy + ro * math.sin(a - da / 2), z1 + 0.02)
        bar(inner, outer, 0.07, 0.05, m["darkwood"])
        # baluster on the outer edge
        bx, by = cx + (r1 - 0.1) * math.cos(a), cy + (r1 - 0.1) * math.sin(a)
        baluster(m, (bx, by, z1), 0.85)
        rail.append((bx, by, z1 + 0.85))
    # handrail: from a newel at the foot to the top
    a = a0
    foot = (cx + (radius + width / 2 - 0.1) * math.cos(a), cy + (radius + width / 2 - 0.1) * math.sin(a), DECK_Z)
    newel(m, foot, 1.15)
    points = [(foot[0], foot[1], foot[2] + 1.05)] + rail
    for p, q in zip(points, points[1:]):
        rod(p, q, 0.05, m["redwood"], 8)
    return points[-1]


def baluster(m, base, height, mat=None):
    x, y, z = base
    lathe((x, y, z), (x, y, z + height),
          ((0, 0.055), (0.1, 0.055), (0.18, 0.03), (0.35, 0.06), (0.55, 0.035), (0.8, 0.03), (0.9, 0.05),
           (1.0, 0.05)), mat or m["redwood"], 8)


def newel(m, base, height):
    x, y, z = base
    slab(x - 0.11, x + 0.11, y - 0.11, y + 0.11, z, z + height, m["redwood"], bevel=0.02)
    sphere((x, y, z + height + 0.1), 0.13, m["redwood"])
    lathe((x, y, z + height - 0.02), (x, y, z + height + 0.04), ((0, 0.14), (1, 0.14)), m["darkwood"], 12)


def stern(m):
    top = POOP_Z
    x1 = DECK_X1
    y0, y1 = DECK_Y0 - WALL, RAIL_Y + WALL
    # the raised deck: wall facing the field, deck on top, its side towards the sea
    slab(POOP_X, x1, y0, y1, DECK_Z, top - 0.1, m["wall"])
    slab(POOP_X - 0.05, x1, y0, y1, top - 0.1, top, m["deck"])
    # moulding under the deck's edge and a skirting at the foot
    slab(POOP_X - 0.18, POOP_X, y0, y1, top - 0.32, top - 0.1, m["redwood"], bevel=0.03)
    slab(POOP_X - 0.08, POOP_X, y0, y1, DECK_Z, 0.12, m["darkwood"])
    # pillars along the wall
    for y in (-16.0, -10.0, -7.95, -3.45, -1.4, 1.0):
        slab(POOP_X - 0.16, POOP_X, y - 0.16, y + 0.16, 0.12, top - 0.32, m["redwood"], bevel=0.02)

    door(m, -5.7)
    lattice_window(m, -2.4, 0.8, 1.6, 1.25)
    lattice_window(m, -9.0, 0.8, 1.6, 1.25)
    lantern(m, -4.25, 1.45)
    lantern(m, -7.15, 1.45)

    # stairs up from both sides of the door, the north one turning towards the bow
    north_top = stairs(m, (POOP_X + 0.35, 0.15), 2.15, math.pi, math.pi * 0.5, 11)
    south_top = stairs(m, (POOP_X + 0.35, -11.4), 2.15, math.pi, math.pi * 1.5, 11)

    # balustrade along the edge between the stairs
    y_from, y_to = -9.15, -2.1
    rail_z = top + 0.85
    newel(m, (POOP_X + 0.12, y_from - 0.1, top), 1.0)
    newel(m, (POOP_X + 0.12, y_to + 0.1, top), 1.0)
    count = int((y_to - y_from) / 0.3)
    for i in range(1, count):
        baluster(m, (POOP_X + 0.12, y_from + (y_to - y_from) * i / count, top), 0.85)
    bar((POOP_X + 0.12, y_from, rail_z), (POOP_X + 0.12, y_to, rail_z), 0.13, 0.09, m["redwood"])
    bar((POOP_X + 0.12, y_from, top + 0.06), (POOP_X + 0.12, y_to, top + 0.06), 0.12, 0.08, m["redwood"])
    # rails around the stern deck's far side, mostly out of view
    for y in (y1 - 0.1,):
        x = POOP_X + 2.6
        while x < AFT_X - 0.2:
            baluster(m, (x, y, top), 0.85)
            x += 0.3
        bar((POOP_X + 2.4, y, rail_z), (AFT_X - 0.1, y, rail_z), 0.12, 0.09, m["redwood"])
    # the ship's side under the stern deck, towards the sea
    # the stern deck's other sides towards the sea, with a rail
    for (a, b) in (((POOP_X + 0.1, y0 + 0.12), (AFT_X - 0.1, y0 + 0.12)),):
        a, b = Vector((*a, top)), Vector((*b, top))
        count = int((b - a).length / 0.3)
        for i in range(1, count):
            baluster(m, tuple(a + (b - a) * i / count), 0.85)
        bar(a + Vector((0, 0, 0.85)), b + Vector((0, 0, 0.85)), 0.12, 0.09, m["redwood"])
    exterior(aftcastle, m)
    return north_top, south_top


# --- props on the deck ----------------------------------------------------------------------------

def cannonball(m, loc, r=0.16):
    sphere((loc[0], loc[1], loc[2] + r), r, m["iron"])


def ball_pile(m, x, y, z=DECK_Z, r=0.16):
    for i in range(3):
        for j in range(3 - i):
            for k in range(3 - i - j):
                pass
    # a pyramid: 3x3, 2x2, 1
    for level, n in enumerate((3, 2, 1)):
        off = (n - 1) * r
        for i in range(n):
            for j in range(n):
                cannonball(m, (x - off + 2 * r * i, y - off + 2 * r * j, z + level * r * 1.41), r)


def cannon(m, x, y, angle):
    """A cannon on its carriage, its muzzle towards angle (radians, 0 is +x)."""
    d = Vector((math.cos(angle), math.sin(angle), 0))
    s = Vector((-d.y, d.x, 0))
    base = Vector((x, y, DECK_Z))
    # carriage: two stepped cheeks, axles, wheels
    for side in (-1, 1):
        for step, (length, height) in enumerate(((1.5, 0.55), (1.05, 0.75), (0.6, 0.92))):
            back = -0.75
            p0 = base + d * back + s * side * 0.36
            p1 = base + d * (back + length) + s * side * 0.36
            c = (p0 + p1) / 2
            box((c.x, c.y, DECK_Z + 0.12 + height / 2), (length, 0.12, height), m["wood"], rotation=angle, bevel=0.02)
    box(tuple(base + Vector((0, 0, 0.24))), (1.45, 0.62, 0.12), m["wood"], rotation=angle)
    for along in (-0.5, 0.55):
        c = base + d * along
        rod(c - s * 0.55 + Vector((0, 0, 0.2)), c + s * 0.55 + Vector((0, 0, 0.2)), 0.05, m["iron"], 8)
        for side in (-1, 1):
            w = c + s * side * 0.5 + Vector((0, 0, 0.2))
            lathe(w - s * side * 0.06, w + s * side * 0.06, ((0, 0.24), (0.5, 0.27), (1, 0.24)), m["wood"], 16)
            torus(tuple(w + s * side * 0.0), 0.25, 0.03, m["iron"], normal=tuple(s))
    # the barrel, slightly raised to the front
    breech = base + d * -0.85 + Vector((0, 0, 0.95))
    muzzle = base + d * 1.55 + Vector((0, 0, 1.08))
    lathe(breech, muzzle, ((0, 0.0), (0.02, 0.17), (0.06, 0.29), (0.12, 0.3), (0.14, 0.27), (0.4, 0.25),
                           (0.42, 0.27), (0.44, 0.24), (0.9, 0.2), (0.93, 0.24), (1.0, 0.24), (1.0, 0.12),
                           (0.97, 0.11)), m["iron"], 20, caps=False)
    sphere(tuple(breech - (muzzle - breech).normalized() * 0.1), 0.1, m["iron"])
    for side in (-1, 1):
        t = breech + (muzzle - breech) * 0.42
        rod(t, t + s * side * 0.45, 0.07, m["iron"], 8)


def barrel(m, base, direction=(0, 0, 1), length=1.15, radius=0.48):
    p0 = Vector(base)
    p1 = p0 + Vector(direction).normalized() * length
    profile = [(0, radius * 0.82), (0.25, radius * 0.97), (0.5, radius), (0.75, radius * 0.97), (1, radius * 0.82)]
    lathe(p0, p1, profile, m["stave"], 20)
    for t in (0.08, 0.3, 0.7, 0.92):
        r = radius * (0.85 + 0.15 * math.sin(math.pi * t)) + 0.012
        q = p0 + (p1 - p0) * t
        lathe(q - (p1 - p0).normalized() * 0.04, q + (p1 - p0).normalized() * 0.04, ((0, r), (1, r)), m["iron"], 20,
              caps=False)
    lathe(p1 - (p1 - p0).normalized() * 0.03, p1 - (p1 - p0).normalized() * 0.01,
          ((0, radius * 0.8), (1, radius * 0.8)), m["darkwood"], 20)


def grate(m, x0, x1, y0, y1):
    """A cargo hatch: raised coaming with a gold trim around a wooden grating over the dark hold."""
    c = 0.28
    z = 0.38
    for (a, b, cc, d) in ((x0, x1, y0, y0 + c), (x0, x1, y1 - c, y1), (x0, x0 + c, y0, y1), (x1 - c, x1, y0, y1)):
        slab(a, b, cc, d, DECK_Z, z, m["redwood"], bevel=0.02)
    for (a, b, cc, d) in ((x0 - 0.03, x1 + 0.03, y0 - 0.03, y0 + 0.08), (x0 - 0.03, x1 + 0.03, y1 - 0.08, y1 + 0.03),
                          (x0 - 0.03, x0 + 0.08, y0, y1), (x1 - 0.08, x1 + 0.03, y0, y1)):
        slab(a, b, cc, d, z, z + 0.05, m["gold"])
    slab(x0 + c, x1 - c, y0 + c, y1 - c, -0.5, -0.45, m["hole"])
    gx0, gx1, gy0, gy1 = x0 + c, x1 - c, y0 + c, y1 - c
    pitch = 0.42
    x = gx0 + pitch / 2
    while x < gx1:
        slab(x - 0.07, x + 0.07, gy0, gy1, z - 0.14, z - 0.02, m["wood"])
        x += pitch
    y = gy0 + pitch / 2
    while y < gy1:
        slab(gx0, gx1, y - 0.07, y + 0.07, z - 0.08, z, m["wood"])
        y += pitch


def mast(m, x, y):
    lathe((x, y, DECK_Z), (x, y, 0.6), ((0, 0.78), (0.3, 0.74), (0.6, 0.62), (1, 0.55)), m["darkwood"], 24)
    lathe((x, y, DECK_Z), (x, y, 34), ((0, 0.48), (0.5, 0.44), (1, 0.32)), m["wood"], 20)
    for z in (1.3, 1.45, 3.4, 3.55, 7.5):
        lathe((x, y, z), (x, y, z + 0.08), ((0, 0.49), (1, 0.49)), m["iron"], 20)
    # pins around the base for the lines
    for i in range(8):
        a = 2 * math.pi * i / 8
        px, py = x + 0.62 * math.cos(a), y + 0.62 * math.sin(a)
        rod((px, py, 0.55), (px, py, 0.8), 0.035, m["brass"], 6)
        # coils of rope at their feet
    for i, a in enumerate((0.6, 2.4, 4.0)):
        torus((x + 1.05 * math.cos(a), y + 1.05 * math.sin(a), DECK_Z + 0.04), 0.24, 0.05, m["rope"])
        torus((x + 1.05 * math.cos(a), y + 1.05 * math.sin(a), DECK_Z + 0.11), 0.2, 0.05, m["rope"])


def boom_with_sail(m, mast_xy, z, tip):
    """A boom from the mast out over the side with the furled sail lashed along it."""
    root = Vector((mast_xy[0], mast_xy[1], z))
    tip = Vector(tip)
    rod(root, tip, 0.17, m["wood"], 12)
    lathe(root - Vector((0, 0, 0.25)), root + Vector((0, 0, 0.25)), ((0, 0.56), (1, 0.56)), m["iron"], 16)
    furled_sail(m, root, tip)
    lathe(tip - (tip - root).normalized() * 0.2, tip + (tip - root).normalized() * 0.05, ((0, 0.2), (1, 0.2)),
          m["iron"], 12)
    return root, tip


def furled_sail(m, root, tip, size=1.0, t0=0.1, t1=0.94, ties=6):
    """The canvas bundled on top of a spar from root to tip: pinched by the gaskets, wrinkled between them."""
    along = tip - root
    axis = along.normalized()
    side = axis.cross(Vector((0, 0, 1))).normalized()
    up = side.cross(axis).normalized()
    rings, segments = 120, 20
    verts, faces, pinch = [], [], []
    for i in range(rings + 1):
        u = i / rings
        t = t0 + (t1 - t0) * u
        taper = math.sin(math.pi * (0.06 + 0.88 * u)) ** 0.6
        bunch = 0.72 + 0.28 * abs(math.sin(math.pi * ties * u))
        radius = 0.5 * size * taper * bunch
        centre = root + along * t + up * (0.12 * size + radius * 0.55)
        for s in range(segments):
            a = 2 * math.pi * s / segments
            wrinkle = 1 + 0.07 * math.sin(5 * a + 31 * u) + 0.04 * math.sin(11 * a - 17 * u)
            # flatter below where it lies on the boom, the slack hangs to the sides
            r = radius * wrinkle
            offset = side * math.cos(a) * r * 1.25 + up * math.sin(a) * r * (0.7 if math.sin(a) < 0 else 0.95)
            verts.append(centre + offset)
    for i in range(rings):
        for s in range(segments):
            a, b = i * segments + s, i * segments + (s + 1) % segments
            faces.append((a, b, b + segments, a + segments))
    faces.append(tuple(reversed(range(segments))))
    faces.append(tuple(range(rings * segments, (rings + 1) * segments)))
    _mesh("sail", verts, faces, m["sail"])
    # the gaskets around the boom and the sail at its pinches
    for k in range(1, ties):
        u = k / ties
        t = t0 + (t1 - t0) * u
        taper = math.sin(math.pi * (0.06 + 0.88 * u)) ** 0.6
        radius = 0.5 * size * taper * 0.72
        centre = root + along * t + up * (0.12 * size + radius * 0.55 - 0.08)
        bpy.ops.mesh.primitive_torus_add(major_radius=1.0, minor_radius=0.05, major_segments=20, minor_segments=6)
        obj = bpy.context.object
        obj.location = centre
        obj.rotation_euler = axis.to_track_quat("Z", "Y").to_euler()
        obj.scale = (radius * 1.3 + 0.12, radius * 1.0 + 0.16, 0.6)
        bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
        _finish(obj, m["darkrope"])


MAIN = Vector((-2.4, -4.1, 34.0))  # the main mast's top
MAIN_TOP_Z = 23.6  # its round top (crow's nest)
FORE = Vector((-25.0, YC, 23.2))
FORE_TOP_Z = 20.6
MIZZEN = Vector((21.8, YC, 21.0))
# the bowsprit's heel lies on the forecastle deck against its bitts, it rises over the stem head at 28 degrees
BOWSPRIT_ROOT = Vector((BOW_X + 0.6, YC, FORE_Z + 0.42))
BOWSPRIT = BOWSPRIT_ROOT + Vector((-math.cos(math.radians(28)), 0.0, math.sin(math.radians(28)))) * 19.0


def mast_radius(mast_base_z, mast_top_z, base_radius, top_radius, z):
    t = min(max((z - mast_base_z) / (mast_top_z - mast_base_z), 0.0), 1.0)
    return base_radius + (top_radius - base_radius) * t


def block(m, at, size=0.16):
    """A rigging block: a small dark shell with its sheave's iron strop."""
    sphere(tuple(at), size, m["darkwood"], scale=(0.7, 0.7, 1.0), subdiv=1)
    torus(tuple(at), size * 0.75, size * 0.18, m["iron"], normal=(1, 0, 0))


def line(m, a, b, radius=0.03, with_block=True):
    """A running line from a to b, a block where it leaves a."""
    rod(a, b, radius, m["darkrope"], 5)
    if with_block:
        block(m, a)


def bowsprit_fittings(m, heel, axis):
    """What holds the bowsprit: bitts on the forecastle deck its heel butts against, a knighthead rising from the
    stem head under it and the gammoning lashing it down there."""
    # bitts: two posts and the crossbeam over the heel's end
    for side in (-1, 1):
        slab(heel.x + 0.05, heel.x + 0.45, YC + side * 0.6 - 0.2, YC + side * 0.6 + 0.2, FORE_Z, FORE_Z + 1.5,
             m["darkwood"], bevel=0.03)
        sphere((heel.x + 0.25, YC + side * 0.6, FORE_Z + 1.55), 0.17, m["darkwood"])
    slab(heel.x - 0.12, heel.x + 0.2, YC - 0.85, YC + 0.85, FORE_Z + 0.1, FORE_Z + 1.0, m["darkwood"], bevel=0.03)
    # the stem head, where the bowsprit passes over the bow
    x_stem = BOW_X + bow_rake(BOW_X, sheer(BOW_X)) + 0.6
    t = (x_stem - heel.x) / axis.x
    over = heel + axis * t
    rail = sheer(BOW_X) - 0.1
    slab(x_stem - 0.35, x_stem + 0.35, YC - 0.45, YC + 0.45, rail - 1.2, over.z - 0.36, m["darkwood"], bevel=0.04)
    # gammoning: turns of rope round the bowsprit and down through the knighthead
    for k in range(5):
        c = over + axis * (k - 2) * 0.13
        torus(tuple(c), 0.5, 0.045, m["darkrope"], normal=tuple(axis))
        rod(c - Vector((0, 0.38, 0.3)), Vector((x_stem + (k - 2) * 0.1, YC - 0.4, rail - 1.0)), 0.035,
            m["darkrope"], 5)
        rod(c + Vector((0, 0.38, -0.3)), Vector((x_stem + (k - 2) * 0.1, YC + 0.4, rail - 1.0)), 0.035,
            m["darkrope"], 5)
    # a cap at the bowsprit's end
    lathe(BOWSPRIT - axis * 0.25, BOWSPRIT + axis * 0.05, ((0, 0.2), (1, 0.2)), m["iron"], 10)


def channel(m, feet, outward, drop=0.45, reach=0.6):
    """The feet of shrouds set out on a channel outside the hull: a plank, a deadeye per shroud with its lanyard
    and an iron chainplate down the side. Returns where the shrouds start (above the deadeyes)."""
    feet = [Vector(f) for f in feet]
    out = Vector((0, outward * reach, -drop))
    seats = [f + out for f in feet]
    a, b = seats[0], seats[-1]
    along = (b - a).normalized() if (b - a).length > 1e-3 else Vector((1, 0, 0))
    bar(a - along * 0.45 - Vector((0, outward * 0.3, 0.06)), b + along * 0.45 - Vector((0, outward * 0.3, 0.06)),
        0.7, 0.1, m["wale"])
    tops = []
    for seat in seats:
        low = seat + Vector((0, 0, 0.12))
        high = seat + Vector((0, 0, 0.62))
        for p in (low, high):
            lathe(p - Vector((0, 0, 0.07)), p + Vector((0, 0, 0.07)), ((0, 0.15), (0.5, 0.17), (1, 0.15)),
                  m["darkwood"], 10)
        rod(low, high, 0.025, m["rope"], 4)
        bar(seat - Vector((0, outward * 0.05, 0.05)), seat - Vector((0, outward * 0.12, 1.6)), 0.08, 0.05,
            m["iron"])
        tops.append(high + Vector((0, 0, 0.07)))
    return tops


def shrouds(m, feet, top, ratlines=8):
    """Lines from feet (on a channel's deadeyes) up to a mast's top, ratlines across the first few metres."""
    for foot in feet:
        rod(foot, top, 0.035, m["rope"], 6)
    for k in range(1, ratlines + 1):
        t = k * 0.035
        rod(feet[0].lerp(top, t), feet[-1].lerp(top, t), 0.02, m["rope"], 5)


def cloth(m, name, point, nu, nv, wave, paint=None, base="sailcloth"):
    """A sail or flag as a thin cloth: point(u, v) gives its surface, paint(u, v) the material of a face (None for
    the base), wave(u, v) how it moves in the wind (a direction times how far, see GltfModel's "wave")."""
    verts, faces, looks = [], [], []
    for j in range(nv + 1):
        for i in range(nu + 1):
            verts.append(point(i / nu, j / nv))
    for j in range(nv):
        for i in range(nu):
            a = j * (nu + 1) + i
            faces.append((a, a + 1, a + nu + 2, a + nu + 1))
            looks.append((paint and paint((i + 0.5) / nu, (j + 0.5) / nv, i, j)) or base)
    names = [base] + sorted(set(looks) - {base})
    obj = _mesh(name, verts, faces, m[base])
    _TARGET[0].remove(obj)
    CLOTH.append(obj)
    for extra in names[1:]:
        obj.data.materials.append(m[extra])
    for poly, look in zip(obj.data.polygons, looks):
        poly.material_index = names.index(look)
    colors = obj.data.color_attributes.new("wave", "FLOAT_COLOR", "POINT")
    for j in range(nv + 1):
        for i in range(nu + 1):
            direction, amount = wave(i / nu, j / nv)
            d = Vector(direction).normalized()
            colors.data[j * (nu + 1) + i].color = (d.x * 0.5 + 0.5, d.y * 0.5 + 0.5, d.z * 0.5 + 0.5, amount)
    solidify = obj.modifiers.new("Solidify", "SOLIDIFY")
    solidify.thickness = 0.05
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.ops.object.modifier_apply(modifier=solidify.name)
    return obj


def cross_pattee(u, v, width, height, size=0.62, centre=0.46):
    """Inside a red cross with flared arms (like the Santa Maria's), in a sail's u (across) and v (down)."""
    x = (u - 0.5) * width
    y = (v - centre) * height
    arm = size * min(width, height) * 0.5
    flare = lambda d: arm * (0.1 + 0.2 * (d / arm) ** 1.6)
    horizontal = abs(x) < arm and abs(y) < flare(abs(x))
    vertical = abs(y) < arm and abs(x) < flare(abs(y))
    return horizontal or vertical


def square_sail(m, mast, top, bottom, half_top, half_bottom, billow, crossed=False, radius=0.35):
    """A square sail set from its yard at top down to bottom, bellied forward by the wind from astern: the yard
    held to the mast by its parrel and sling, the sail's head lashed to it, its edges bolt-roped, its cloths
    seamed. Returns its yardarms and clews for the braces and sheets."""
    head = mast.x - radius - 0.22
    arms = (Vector((head, mast.y + half_top + 0.4, top)), Vector((head, mast.y - half_top - 0.4, top)))
    lathe(arms[0], arms[1], ((0, 0.11), (0.08, 0.13), (0.5, 0.26), (0.92, 0.13), (1, 0.11)), m["wood"], 10)
    for arm in arms:
        lathe(arm - Vector((0, 0.06, 0)), arm + Vector((0, 0.06, 0)), ((0, 0.14), (1, 0.14)), m["iron"], 8)
    # parrel and sling
    torus((mast.x, mast.y, top), radius + 0.06, 0.07, m["darkrope"])
    bar(Vector((mast.x - radius * 0.7, mast.y, top)), Vector((head, mast.y, top)), 0.12, 0.12, m["darkrope"])
    rod(Vector((head, mast.y, top + 0.2)), Vector((mast.x - radius, mast.y, top + 2.2)), 0.05, m["darkrope"], 6)

    def point(u, v):
        half = half_top + (half_bottom - half_top) * v
        belly = math.sin(math.pi * u) * math.sin(math.pi * (0.1 + 0.9 * v)) ** 0.8
        # the foot curves up between the corners
        z = top - 0.2 - (top - 0.2 - bottom) * v + 0.6 * math.sin(math.pi * u) * v ** 2
        return Vector((head - billow * belly, mast.y + half * (1 - 2 * u), z))

    width, height = half_top + half_bottom, top - bottom
    nu, nv = 36, 28

    def paint(u, v, i, j):
        if i == 0 or i == nu - 1 or j == nv - 1:
            return "boltrope"
        if crossed and cross_pattee(u, v, width, height):
            return "redcross"
        if i % 4 == 0:
            return "seam"
        return None

    def wave(u, v):
        # fixed on the yard, the belly and the free foot move
        return (-1.0, 0.0, 0.0), 0.4 * math.sin(math.pi * u) * v ** 0.7

    cloth(m, "square_sail", point, nu, nv, wave, paint)
    # robands lashing the head to the yard
    for k in range(1, 12):
        y = mast.y + half_top * (1 - 2 * k / 12)
        torus((head, y, top - 0.08), 0.2, 0.025, m["darkrope"], normal=(0, 1, 0))
    clews = (point(0.0, 1.0), point(1.0, 1.0))
    return arms, clews


def lateen_sail(m, mast):
    """The mizzen's lateen: a triangle on a long yard slung across the mast, its lower end (the tack) forward and
    lashed down near the deck, its upper end (the peak) high aft over the mast; the third corner (the clew) is
    sheeted aft to a bumkin over the stern. It bellies out to the side away from the mizzen's shrouds."""
    y = mast.y + 0.4
    tack = Vector((mast.x - 4.6, y, AFT_Z + 1.6))
    peak = Vector((mast.x + 6.4, y, mast.z + 3.0))
    clew = Vector((STERN_X + 1.2, y, AFT_Z + 1.9))
    axis = (peak - tack).normalized()
    lathe(tack - axis * 0.5, peak + axis * 0.3, ((0, 0.1), (0.45, 0.2), (1, 0.08)), m["wood"], 10)
    # where the yard crosses the mast: parrel and sling to the masthead
    cross = tack.lerp(peak, (mast.x - tack.x) / (peak.x - tack.x))
    torus((mast.x, mast.y, cross.z), 0.33, 0.07, m["darkrope"])
    bar(Vector((mast.x, mast.y + 0.2, cross.z)), cross, 0.12, 0.12, m["darkrope"])
    rod(cross + Vector((0, 0, 0.2)), Vector((mast.x, mast.y + 0.25, mast.z - 0.3)), 0.05, m["darkrope"], 6)
    block(m, Vector((mast.x, mast.y + 0.25, mast.z - 0.3)))
    # the tack lashed down to a ring on the aftcastle's deck
    rod(tack, Vector((tack.x, mast.y + 0.3, AFT_Z + 0.05)), 0.04, m["darkrope"], 5)
    torus((tack.x, mast.y + 0.3, AFT_Z + 0.05), 0.12, 0.03, m["iron"])
    # the bumkin over the stern the clew is sheeted to
    bumkin = Vector((STERN_X + 1.8, y, sheer(STERN_X) + 0.35))
    bar(Vector((STERN_X - 0.6, y, sheer(STERN_X) + 0.1)), bumkin, 0.2, 0.2, m["darkwood"])
    line(m, clew, bumkin)

    def point(u, v):
        # u along the yard from the tack to the peak, v from the yard to the clew
        p = tack.lerp(peak, u).lerp(clew, v)
        belly = math.sin(math.pi * u) * math.sin(math.pi * min(v, 1.0) * 0.85) * 1.6
        return p + Vector((0, 0.12 + belly, -0.12))

    def paint(u, v, i, j):
        if i == 0 or i == 17:
            return "boltrope"
        if i % 4 == 2:
            return "seam"
        return None

    def wave(u, v):
        return (0.0, 1.0, 0.0), 0.45 * math.sin(math.pi * u) * math.sin(math.pi * v)

    cloth(m, "lateen", point, 18, 12, wave, paint)
    # robands along the yard
    for k in range(1, 14):
        torus(tuple(tack.lerp(peak, k / 14) - Vector((0, 0, 0.06))), 0.19, 0.025, m["darkrope"], normal=tuple(axis))


def jolly_roger(m, pole_top):
    """A black flag streaming forward from a pole on the main mast's top (the wind blows from astern), a white
    skull over crossed bones on both sides."""
    lathe(pole_top - Vector((0, 0, 3.0)), pole_top, ((0, 0.1), (1, 0.06)), m["darkwood"], 8)
    sphere(tuple(pole_top + Vector((0, 0, 0.08))), 0.13, m["gold"])
    width, height = 4.8, 3.1

    def point(u, v):
        return pole_top + Vector((-u * width, 0.28 * math.sin(u * 5.0 + 0.6) * u, -0.15 - v * height + 0.25 * u * u))

    def paint(u, v, i, j):
        # a skull over crossed bones, in the flag's proportions (u along it, v down)
        x, y = (u - 0.42) * 3.4, (v - 0.45) * 2.2
        skull = (x / 0.42) ** 2 + ((y + 0.12) / 0.4) ** 2 < 1.0 and not (
            ((x - 0.15) ** 2 + (y + 0.12) ** 2 < 0.012) or ((x + 0.15) ** 2 + (y + 0.12) ** 2 < 0.012))
        jaw = abs(x) < 0.24 and 0.2 < y < 0.38
        bones = any(abs((y - 0.62) - sign * x * 0.62) < 0.09 and abs(x) < 0.85 for sign in (1, -1))
        knuckles = any((abs(x) - 0.82) ** 2 + (y - 0.62 + sign * 0.48) ** 2 < 0.02 for sign in (1, -1))
        return "bone" if skull or jaw or bones or knuckles else None

    def wave(u, v):
        return (0.0, 1.0, 0.25), 0.55 * u ** 0.8

    cloth(m, "flag", point, 44, 28, wave, paint, base="black")
    # the halyard it is hoisted on
    rod(pole_top + Vector((0.08, 0.08, 0)), pole_top + Vector((0.08, 0.08, -3.0)), 0.02, m["darkrope"], 4)


def pennant(m, pole_top, length, height=0.7):
    """A long tapering streamer flying forward from a pole on a mast's top."""
    lathe(pole_top - Vector((0, 0, 2.2)), pole_top, ((0, 0.08), (1, 0.05)), m["darkwood"], 8)
    sphere(tuple(pole_top + Vector((0, 0, 0.06))), 0.1, m["gold"])

    def point(u, v):
        h = height * (1 - 0.85 * u)
        return pole_top + Vector((-u * length, 0.35 * math.sin(u * 6.0) * u, -0.1 - v * h - 0.3 * u * u))

    def paint(u, v, i, j):
        return "redcross" if j < 2 else None

    def wave(u, v):
        return (0.0, 1.0, 0.3), 0.6 * u ** 0.7

    cloth(m, "pennant", point, 24, 4, wave, paint, base="sailcloth")


def crows_nest(m, mast, z, radius):
    """A round top: a bowl around the mast on trestles, planked with battens, a rim round its edge."""
    lathe(Vector((mast.x, mast.y, z - 0.8)), Vector((mast.x, mast.y, z)), ((0, 0.45), (0.6, radius * 0.7),
                                                                         (1, radius)), m["darkwood"], 24)
    lathe(Vector((mast.x, mast.y, z)), Vector((mast.x, mast.y, z + 1.3)),
          ((0, radius), (0.6, radius * 1.06), (1, radius * 1.1)), m["redwood"], 24, caps=False)
    lathe(Vector((mast.x, mast.y, z + 1.25)), Vector((mast.x, mast.y, z + 1.42)), ((0, radius * 1.15),
                                                                                  (1, radius * 1.15)), m["darkwood"], 24)
    lathe(Vector((mast.x, mast.y, z - 0.05)), Vector((mast.x, mast.y, z + 0.08)), ((0, radius * 1.03),
                                                                                  (1, radius * 1.03)), m["darkwood"], 24)
    for k in range(16):
        a = 2 * math.pi * k / 16
        d = Vector((math.cos(a), math.sin(a), 0))
        bar(Vector((mast.x, mast.y, z)) + d * (radius + 0.03), Vector((mast.x, mast.y, z + 1.3)) + d * (radius * 1.1 + 0.03),
            0.08, 0.06, m["darkwood"])
    # trestles under it
    for k in range(4):
        a = math.pi / 4 + math.pi / 2 * k
        d = Vector((math.cos(a), math.sin(a), 0))
        bar(Vector((mast.x, mast.y, z - 1.6)) + d * 0.3, Vector((mast.x, mast.y, z - 0.75)) + d * radius * 0.75, 0.12,
            0.12, m["darkwood"])


def woolding(m, base, top, radius, every=1.3, start=1.0):
    """Turns of rope bound round a lower mast at intervals."""
    z = base.z + start
    while z < top.z - 2.0:
        r = mast_radius(base.z, top.z, radius[0], radius[1], z) + 0.03
        lathe(Vector((base.x, base.y, z)), Vector((base.x, base.y, z + 0.16)), ((0, r), (1, r)), m["darkrope"], 12,
              caps=False)
        z += every


def rigging(m, mast_xy):
    """The main mast's shrouds, a little abaft the mast, from a channel outside the rail behind the field (the other
    side's would cross the game's view) up to its top."""
    north = RAIL_Y + WALL
    top = Vector((MAIN.x, MAIN.y, MAIN_TOP_Z - 0.7))
    for xs in ((-1.9, -1.1, -0.3, 0.5), (1.7, 2.5, 3.3, 4.1)):
        feet = channel(m, [Vector((x, north, 1.25)) for x in xs], 1)
        shrouds(m, feet, top)


def anchor(m, cathead_end, side):
    """An anchor hanging from its cathead at the bow: ring, stock, shank, arms and flukes."""
    ring = cathead_end - Vector((0, 0, 0.6))
    rod(cathead_end, ring, 0.04, m["darkrope"], 5)
    torus(tuple(ring), 0.22, 0.05, m["iron"], normal=(1, 0, 0))
    crown = ring - Vector((0.15, 0, 3.2))
    bar(ring - Vector((0, 0, 0.25)), crown, 0.16, 0.16, m["iron"])
    bar(ring - Vector((0.9, 0, 0.45)), ring + Vector((0.9, 0, -0.45)), 0.14, 0.14, m["darkwood"])
    for sign in (1, -1):
        tip = crown + Vector((sign * 1.0, 0, 0.9))
        bar(crown, tip, 0.14, 0.14, m["iron"])
        bar(tip - Vector((sign * 0.15, 0, 0.35)), tip + Vector((sign * 0.12, 0, 0.15)), 0.3, 0.06, m["iron"])


def masts(m):
    """What only the fly-in shows, after the Santa Maria: the main mast's big course and small topsail with red
    crosses, a round top and the flag; the short foremast on the forecastle with its course; a lateen sail on the
    mizzen; a spritsail under the steep bowsprit; their braces, sheets, stays, channels and deadeyes."""
    # main mast (the mast itself stands on the deck, see mast())
    main_arms, main_clews = square_sail(m, MAIN, 21.6, 8.6, 10.4, 11.6, 3.2, crossed=True, radius=0.38)
    crows_nest(m, MAIN, MAIN_TOP_Z, 1.7)
    topmast_base = Vector((MAIN.x, MAIN.y, MAIN_TOP_Z + 1.4))
    lathe(topmast_base, Vector((MAIN.x, MAIN.y, 34.0)), ((0, 0.26), (1, 0.18)), m["wood"], 12)
    top_arms, top_clews = square_sail(m, Vector((MAIN.x, MAIN.y, 0)), 32.0, 26.0, 3.6, 5.0, 1.2, crossed=True,
                                      radius=0.22)
    jolly_roger(m, MAIN + Vector((0, 0, 2.6)))
    # foremast
    fore_base = Vector((FORE.x, FORE.y, FORE_Z))
    lathe(fore_base, FORE, ((0, 0.42), (1, 0.26)), m["wood"], 16)
    woolding(m, fore_base, FORE, (0.42, 0.26))
    fore_arms, fore_clews = square_sail(m, FORE, 19.4, 9.6, 6.4, 7.4, 2.6, radius=0.33)
    crows_nest(m, FORE, FORE_TOP_Z, 0.95)
    pennant(m, FORE + Vector((0, 0, 2.0)), 5.0)
    # mizzen on the aftcastle
    mizzen_base = Vector((MIZZEN.x, MIZZEN.y, AFT_Z))
    lathe(mizzen_base, MIZZEN, ((0, 0.34), (1, 0.22)), m["wood"], 16)
    woolding(m, mizzen_base, MIZZEN, (0.34, 0.22), every=1.5)
    lateen_sail(m, MIZZEN)
    pennant(m, MIZZEN + Vector((0, 0, 1.8)), 4.0, 0.55)
    # bowsprit with its spritsail
    root = BOWSPRIT_ROOT
    lathe(root, BOWSPRIT, ((0, 0.42), (0.3, 0.42), (1, 0.16)), m["wood"], 12)
    axis = (BOWSPRIT - root).normalized()
    bowsprit_fittings(m, root, axis)
    for t in (0.35, 0.6):
        q = root.lerp(BOWSPRIT, t)
        r = 0.45 - 0.27 * t + 0.03
        lathe(q - axis * 0.06, q + axis * 0.06, ((0, r), (1, r)), m["iron"], 12)
    sprit = root.lerp(BOWSPRIT, 0.62)
    sprit_arms, sprit_clews = square_sail(m, Vector((sprit.x + 0.5, YC, 0)), sprit.z - 0.35, sprit.z - 6.0, 4.6,
                                          5.0, 1.6, radius=0.0)
    # shrouds of the fore and mizzen masts on channels outside the castles, abaft their masts so the sails before
    # them stay clear; the mizzen's only on the side away from its lateen sail
    for sign in (1, -1):
        rails = []
        for x in (-24.6, -23.8, -23.0, -22.2):
            z = sheer(x) - 0.2
            rails.append(Vector((x + bow_rake(x, z), YC + sign * float(half_beam(x)), z)))
        shrouds(m, channel(m, rails, sign), Vector((FORE.x, FORE.y, FORE_TOP_Z - 0.7)), ratlines=6)
    rails = [Vector((x, YC - HB, sheer(x) - 0.2)) for x in (22.2, 22.8, 23.4)]
    shrouds(m, channel(m, rails, -1), Vector((MIZZEN.x, MIZZEN.y, 17.6)), ratlines=5)
    # stays between the tops and out to the bowsprit
    for a, b in ((Vector((MAIN.x, MAIN.y, MAIN_TOP_Z + 1.0)), Vector((FORE.x, FORE.y, FORE_TOP_Z + 0.4))),
                 (Vector((FORE.x, FORE.y, FORE_TOP_Z + 1.0)), BOWSPRIT - (BOWSPRIT - root).normalized() * 0.3),
                 (MAIN, FORE), (MIZZEN, Vector((MAIN.x, MAIN.y, MAIN_TOP_Z)))):
        rod(a, b, 0.045, m["rope"], 6)
        block(m, b, 0.13)
    # braces from the yardarms aft, sheets from the clews down to the rails; the main mast's only on the side
    # behind the field, on the other they would cross the game's view
    line(m, main_arms[0], Vector((14.5, YC + HB - 0.3, sheer(14.5) + 0.05)))
    line(m, top_arms[0], Vector((18.8, YC + HB - 0.3, sheer(18.8) + 0.05)))
    # the fore braces lead to the main stay, clear of the main course
    stay = Vector((MAIN.x, MAIN.y, MAIN_TOP_Z + 1.0)).lerp(Vector((FORE.x, FORE.y, FORE_TOP_Z + 0.4)), 0.35)
    for arm, side in zip(fore_arms, (1, -1)):
        line(m, arm, stay + Vector((0, side * 0.3, 0)))
    line(m, main_clews[0], Vector((6.0, YC + HB - 0.25, 1.4)))
    for clew, side in zip(fore_clews, (1, -1)):
        line(m, clew, Vector((-15.0, YC + side * (float(half_beam(-15.0)) - 0.25), sheer(-15.0) + 0.05)))
    for clew, side in zip(sprit_clews, (1, -1)):
        line(m, clew, Vector((FORECASTLE_X - 3.0, YC + side * (float(half_beam(FORECASTLE_X - 3.0)) - 0.3),
                              sheer(FORECASTLE_X - 3.0) + 0.9)))
    # catheads with the anchors at the bow
    for sign in (1, -1):
        x = FORECASTLE_X - 6.5
        z = sheer(x) - 0.3
        inboard = Vector((x + bow_rake(x, z), YC + sign * (float(half_beam(x)) - 0.6), z))
        outboard = inboard + Vector((-0.6, sign * 1.9, 0.25))
        bar(inboard, outboard, 0.32, 0.32, m["darkwood"])
        anchor(m, outboard - Vector((0, 0, 0.2)), sign)


HULL_ROWS = (
    # (share of the half beam at the rail, height at the waist, how much of the sheer's rise it follows,
    #  material of the band below): the wales sweep up with the sheer, the hull is widest at the waterline
    # (tumblehome) and round below
    (1.0, 1.37, 1.0, "planking"),
    (1.006, 0.95, 0.95, "wale"),
    (1.012, 0.7, 0.93, "planking"),
    (1.035, -0.25, 0.78, "wale"),
    (1.04, -0.5, 0.76, "planking"),
    (1.065, -1.55, 0.55, "wale"),
    (1.068, -1.8, 0.53, "bottom"),
    (1.04, -3.0, 0.32, "bottom"),
    (0.86, -4.1, 0.16, "bottom"),
    (0.46, -5.0, 0.05, "bottom"),
    (0.0, -5.4, 0.0, "bottom"),
)
# how much the rows below the waterline narrow towards the stern (the run) and the bow (the forefoot)
RUN = (0, 0, 0, 0, 0, 0, 0.05, 0.25, 0.5, 0.7, 0.0)


def hull_point(x, row, side):
    share, z_waist, follow, _ = HULL_ROWS[row]
    z = z_waist + follow * (sheer(x) - 1.37)
    aft = smoothstep((x - 12.0) / (STERN_X - 12.0))
    fore = smoothstep((TAPER_X - x) / (TAPER_X - BOW_X))
    share *= 1 - RUN[row] * (aft ** 1.5 + 0.5 * fore)
    b = max(float(half_beam(x)), 0.03)
    return Vector((x + bow_rake(x, z), YC + side * b * share, z))


def hull(m):
    """The ship's outside, lofted from the stem to the stern: warm planking with three dark wales sweeping up with
    the sheer, skids along the waist, hawse holes, the rudder and the transom with its lit gallery."""
    steps = [POOP_X, AFT_X, FORECASTLE_X]
    xs = np.concatenate([np.linspace(BOW_X, STERN_X, 120)] + [[x - 0.02, x + 0.02] for x in steps])
    xs = sorted(set(np.round(xs, 3)))
    rows = len(HULL_ROWS)
    # one ring per section: the north side from its top down to the keel, back up the south side
    ring = [(1, i) for i in range(rows)] + [(-1, i) for i in range(rows - 2, -1, -1)]
    verts = [hull_point(x, i, side) for x in xs for side, i in ring]
    n = len(ring)
    faces, bands = [], []
    for k in range(len(xs) - 1):
        for j in range(n - 1):
            a, b = k * n + j, k * n + j + 1
            faces.append((a, b, b + n, a + n))
            side, i = ring[j]
            bands.append(HULL_ROWS[i][3] if side > 0 else HULL_ROWS[ring[j + 1][1]][3])
    last = len(xs) - 1
    faces.append(tuple(last * n + j for j in range(n)))
    bands.append("transom")
    names = sorted(set(bands))
    obj = _mesh("hull", verts, faces, m[names[0]], smooth_shade=False)
    for name in names[1:]:
        obj.data.materials.append(m[name])
    for poly, name in zip(obj.data.polygons, bands):
        poly.material_index = names.index(name)
        centre = poly.center
        outward = Vector((1, 0, 0)) if name == "transom" else Vector((0, centre.y - YC, min(centre.z + 2.0, 0.0)))
        if poly.normal.dot(outward) < 0:
            poly.flip()
        poly.use_smooth = name != "transom"

    # the stem: a timber up the bow from the keel to the stem head
    stem = [hull_point(BOW_X, i, 1) for i in range(rows)]
    for a, b in zip(stem, stem[1:]):
        bar(a - Vector((0.12, 0, 0)), b - Vector((0.12, 0, 0)), 0.4, 0.3, m["wale"])
    # skids down the waist's sides, from the lowest wale to the rail
    for x in np.arange(-9.0, 16.0, 2.6):
        for side in (1, -1):
            low, high = hull_point(x, 5, side), hull_point(x, 0, side)
            out = Vector((0, side * 0.09, 0))
            bar(low + out, high + out - Vector((0, 0, 0.1)), 0.2, 0.12, m["wale"])
    # hawse holes in the bow, ports in the stern's quarters
    for x, row in ((-25.5, 2), (-24.2, 2), (20.5, 4)):
        for side in (1, -1):
            p = hull_point(x, row, side) - Vector((0, 0, 0.35))
            lathe(p - Vector((0, side * 0.25, 0)), p + Vector((0, side * 0.12, 0)), ((0, 0.26), (1, 0.26)),
                  m["hole"], 12)
            lathe(p + Vector((0, side * 0.05, 0)), p + Vector((0, side * 0.14, 0)), ((0, 0.36), (1, 0.36)),
                  m["wale"], 12, caps=False)
    # the transom's outline: the section at the stern, framed by a batten along its edge
    outline = [hull_point(STERN_X, i, side) for side, i in ring[:rows]] + \
              [hull_point(STERN_X, i, -1) for i in range(rows - 2, -1, -1)]
    for a, b in zip(outline, outline[1:]):
        bar(a + Vector((0.08, 0, 0)), b + Vector((0.08, 0, 0)), 0.22, 0.2, m["wale"])

    def transom_half(z):
        """Half the transom's width at height z, from its outline."""
        side = [hull_point(STERN_X, i, 1) for i in range(rows)]
        for upper, lower in zip(side, side[1:]):
            if lower.z <= z <= upper.z:
                t = (z - lower.z) / max(upper.z - lower.z, 1e-6)
                return (lower.y + (upper.y - lower.y) * t) - YC
        return 0.0

    # the sternpost and the rudder hung on it, broad below, narrow at its head, its tiller through the transom
    bar(hull_point(STERN_X, rows - 1, 1) + Vector((0.12, 0, 0)), Vector((STERN_X + 0.12, YC, sheer(STERN_X) - 0.3)),
        0.42, 0.3, m["wale"])
    blade = [(0.15, -5.2), (1.75, -5.2), (1.25, -1.0), (0.6, 2.2), (0.15, 2.2)]
    verts = [Vector((STERN_X + x, YC + side * 0.18, z)) for side in (-1, 1) for x, z in blade]
    k = len(blade)
    faces = [tuple(range(k)), tuple(range(2 * k - 1, k - 1, -1))] + \
            [(i, (i + 1) % k, (i + 1) % k + k, i + k) for i in range(k)]
    _mesh("rudder", verts, faces, m["wale"], smooth_shade=False)
    for z in (-3.6, -1.8, 0.1, 1.6):
        slab(STERN_X - 0.1, STERN_X + 0.55, YC - 0.24, YC + 0.24, z, z + 0.14, m["iron"])
    slab(STERN_X - 0.6, STERN_X + 0.75, YC - 0.12, YC + 0.12, 2.25, 2.45, m["darkwood"])
    # the stern gallery: two rows of windows lit from inside, gold mouldings, lanterns on the corners
    for row, (z0, z1) in enumerate(((1.2, 2.5), (POOP_Z + 0.7, POOP_Z + 1.9))):
        count = 4 if row == 0 else 5
        for k in range(count):
            y = YC + (k - (count - 1) / 2) * 2.3
            if abs(y - YC) + 0.9 > transom_half(z0) - 0.2:
                continue
            slab(STERN_X, STERN_X + 0.14, y - 0.8, y + 0.8, z0, z1, m["gold"], bevel=0.03)
            slab(STERN_X + 0.1, STERN_X + 0.17, y - 0.62, y + 0.62, z0 + 0.15, z1 - 0.15, m["lamp"])
            bar(Vector((STERN_X + 0.2, y, z0 + 0.15)), Vector((STERN_X + 0.2, y, z1 - 0.15)), 0.06, 0.06, m["darkwood"])
            bar(Vector((STERN_X + 0.2, y - 0.62, (z0 + z1) / 2)), Vector((STERN_X + 0.2, y + 0.62, (z0 + z1) / 2)),
                0.06, 0.06, m["darkwood"])
            glow((STERN_X + 0.6, y, (z0 + z1) / 2), (1.0, 0.7, 0.35), 1.6, GLOW_PULSE)
    for z in (0.9, 2.75, POOP_Z + 0.4, POOP_Z + 2.1):
        half = transom_half(z) - 0.2
        if half > 0.5:
            slab(STERN_X, STERN_X + 0.25, YC - half, YC + half, z, z + 0.22, m["gold"], bevel=0.04)
    for side in (1, -1):
        base = Vector((STERN_X - 0.4, YC + side * (HB - 0.4), sheer(STERN_X - 0.4) + 0.85))
        rod(base, base + Vector((0, 0, 0.9)), 0.05, m["iron"], 6)
        lathe(base + Vector((0, 0, 0.9)), base + Vector((0, 0, 1.6)),
              ((0, 0.12), (0.2, 0.2), (0.8, 0.2), (1, 0.05)), m["iron"], 8)
        sphere(tuple(base + Vector((0, 0, 1.25))), 0.16, m["lamp"], scale=(1, 1, 1.3))
        glow(base + Vector((0, 0, 1.25)), (1.0, 0.62, 0.3), 1.4, GLOW_FLAME)


def deck_props(m):
    mast_xy = (-2.4, -4.1)
    mast(m, *mast_xy)
    boom_with_sail(m, mast_xy, 2.9, (-10.5, 3.0, 6.2))
    rigging(m, mast_xy)
    exterior(masts, m)
    exterior(hull, m)

    cannon(m, -3.4, 1.35, math.radians(88))
    ball_pile(m, -1.2, 1.9)
    cannonball(m, (-5.3, 1.6, DECK_Z))
    cannonball(m, (-1.0, 0.6, DECK_Z))
    # a coil of rope and a bucket by the rail
    torus((9.6, 2.5, DECK_Z + 0.04), 0.32, 0.06, m["rope"])
    torus((9.6, 2.5, DECK_Z + 0.12), 0.26, 0.06, m["rope"])
    lathe((11.4, 2.55, DECK_Z), (11.4, 2.55, 0.45), ((0, 0.22), (1, 0.28)), m["stave"], 16)
    lathe((11.4, 2.55, 0.4), (11.4, 2.55, 0.44), ((0, 0.29), (1, 0.29)), m["iron"], 16, caps=False)
    rod((11.2, 2.55, 0.44), (11.6, 2.55, 0.44), 0.015, m["iron"], 6)

    grate(m, -7.6, -1.5, -8.4, -5.3)

    # barrels in the lower left corner, one spilled with its cannonballs
    barrel(m, (-2.0, -11.2, DECK_Z))
    barrel(m, (-3.4, -11.9, DECK_Z))
    barrel(m, (-4.3, -9.5, DECK_Z + 0.47), direction=(0.96, -0.28, 0.0))
    cannonball(m, (-2.3, -9.9, DECK_Z))
    cannonball(m, (-1.4, -10.0, DECK_Z))
    cannonball(m, (-1.0, -12.0, DECK_Z))
    cannonball(m, (-1.0, -9.2, DECK_Z))
    # a chain from the spilled barrel
    for i in range(8):
        p = Vector((-3.6 + 0.05 * i, -10.1 - 0.13 * i, DECK_Z + 0.03))
        torus(tuple(p), 0.07, 0.022, m["iron"], normal=(0.0, 1.0, 1.0) if i % 2 else (1.0, -1.0, 0.2))
    return mast_xy


# --- sea and sky ----------------------------------------------------------------------------------

SEA = (-300.0, 300.0, -300.0, 300.0)
SKY_RADIUS = 380.0
MOON_SKY = (math.radians(58.0), math.radians(9.0))  # azimuth and elevation of the moon in the sky dome
WATER = []


def sea():
    """The water around the ship for the game's water shader ("water": [deep r, g, b] in its extras); its texture
    is data: red the depth, green the foam along the hull."""
    x0, x1, y0, y1 = SEA
    step = 25.0
    xs = np.arange(x0, x1 + 1e-6, step)
    ys = np.arange(y0, y1 + 1e-6, step)
    X, Y = np.meshgrid(xs, ys)
    nx, ny = len(xs), len(ys)
    idx = np.arange(nx * ny).reshape(ny, nx)
    quads = np.stack([idx[:-1, :-1].ravel(), idx[:-1, 1:].ravel(), idx[1:, 1:].ravel(), idx[1:, :-1].ravel()], axis=1)
    verts = np.stack([X.ravel(), Y.ravel(), np.full(X.size, SEA_Z)], axis=1)
    mesh = bpy.data.meshes.new("water")
    mesh.from_pydata(verts.tolist(), [], quads.tolist())
    mesh.validate()
    obj = bpy.data.objects.new("water", mesh)
    bpy.context.scene.collection.objects.link(obj)
    uv = mesh.uv_layers.new(name="UVMap")
    co = np.array([mesh.vertices[l.vertex_index].co[:] for l in mesh.loops])
    uv.data.foreach_set("uv", np.stack([(co[:, 0] - x0) / (x1 - x0), (co[:, 1] - y0) / (y1 - y0)], 1).ravel())

    size = 2048
    u = (np.arange(size) + 0.5) / size
    TX, TY = np.meshgrid(x0 + u * (x1 - x0), y0 + u * (y1 - y0))
    # the shallows round the islands show their sandy floor, the open sea is deep
    depth = np.clip((SEA_Z - seabed(TX, TY)) / 6.0, 0, 1)
    # foam along the hull
    side = np.maximum(np.abs(TY - YC) - 1.075 * half_beam(np.clip(TX, BOW_X, STERN_X)), 0)
    ends = np.maximum(np.maximum(BOW_X - TX, TX - STERN_X), 0)
    away = np.hypot(side, ends)
    foam = np.clip(1.0 - away / 1.3, 0, 1) ** 2 * 0.8
    # and the surf on the beaches
    foam = np.maximum(foam, np.clip(1.0 - depth / 0.05, 0, 1) * (depth > 0.0) * 0.7)
    data = np.stack([depth, foam, np.zeros_like(depth), np.ones_like(depth)], axis=2)
    img = bpy.data.images.new("water_data", size, size)
    img.pixels.foreach_set(data.astype(np.float32).ravel())
    mat = bpy.data.materials.new("water")
    mat.use_nodes = True
    bsdf = next(n for n in mat.node_tree.nodes if n.type == "BSDF_PRINCIPLED")
    if PREVIEW:
        bsdf.inputs["Base Color"].default_value = (0.01, 0.04, 0.09, 1)
        bsdf.inputs["Roughness"].default_value = 0.05
    else:
        tex = mat.node_tree.nodes.new("ShaderNodeTexImage")
        tex.image = img
        mat.node_tree.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
    mesh.materials.append(mat)
    # a caribbean deep, blue going green
    obj["water"] = [0.0, 0.30, 0.36]
    WATER.append((obj, img))
    return obj


def sky():
    """A dome over the sea: the night sky with stars and the moon low over the horizon, its colour painted, not
    baked. Its faces look inwards, its normals all point up so the game's light leaves it as painted."""
    centre = Vector((6.5, YC, SEA_Z - 6.0))
    segments, rings = 96, 24
    verts, faces, uvs = [], [], []
    for j in range(rings + 1):
        elevation = (math.pi / 2) * j / rings
        for i in range(segments + 1):
            azimuth = 2 * math.pi * i / segments
            direction = Vector((math.cos(azimuth) * math.cos(elevation), math.sin(azimuth) * math.cos(elevation),
                                math.sin(elevation)))
            verts.append(centre + direction * SKY_RADIUS)
    for j in range(rings):
        for i in range(segments):
            a = j * (segments + 1) + i
            # wound to face the centre
            faces.append((a, a + segments + 1, a + segments + 2, a + 1))
    mesh = bpy.data.meshes.new("sky")
    mesh.from_pydata([tuple(v) for v in verts], [], faces)
    mesh.validate()
    obj = bpy.data.objects.new("sky", mesh)
    bpy.context.scene.collection.objects.link(obj)
    uv = mesh.uv_layers.new(name="UVMap")
    for loop in mesh.loops:
        index = loop.vertex_index
        uv.data[loop.index].uv = ((index % (segments + 1)) / segments, (index // (segments + 1)) / rings)
    mesh.normals_split_custom_set_from_vertices([(0.0, 0.0, 1.0)] * len(mesh.vertices))

    sw, sh = 4096, 1024
    u = (np.arange(sw) + 0.5) / sw
    v = (np.arange(sh) + 0.5) / sh
    U, V = np.meshgrid(u, v)
    elevation = V * math.pi / 2
    azimuth = U * 2 * math.pi
    moon_azimuth, moon_elevation = MOON_SKY
    cos_angle = (np.cos(elevation) * math.cos(moon_elevation) * np.cos(azimuth - moon_azimuth)
                 + np.sin(elevation) * math.sin(moon_elevation))
    angle = np.degrees(np.arccos(np.clip(cos_angle, -1, 1)))
    # by day: a pale horizon, deep blue overhead, the sun where the moon stands at night, its warm glow
    t = np.clip(elevation / math.radians(50), 0, 1) ** 0.7
    day = np.array(_linear((0.72, 0.86, 0.96))) * (1 - t[..., None]) + np.array(_linear((0.22, 0.50, 0.90))) * t[..., None]
    day += np.array(_linear((1.0, 0.86, 0.6)))[None, None, :] * (np.exp(-angle / 9.0) * 0.55)[..., None]
    day = np.where((angle < 1.8)[..., None], np.array(_linear((1.0, 0.98, 0.9))), day)
    # trade wind cumulus low along the horizon: clusters of soft puffs, flat grey bases, lit tops
    cloud_rng = np.random.default_rng(5)
    cloud = np.zeros((sh, sw))
    lit = np.zeros((sh, sw))
    for _ in range(34):
        cu = cloud_rng.uniform(0, 1)
        floor = cloud_rng.uniform(math.radians(1.2), math.radians(4.5)) / (math.pi / 2)
        width = cloud_rng.uniform(0.006, 0.02)
        for _ in range(int(width * 900)):
            pu = cu + cloud_rng.normal(0, width * 0.35)
            radius = cloud_rng.uniform(0.0025, 0.006)
            pv = floor + radius * 2.5 * cloud_rng.uniform(0.3, 1.0)
            du = np.minimum(np.abs(U - pu), 1 - np.abs(U - pu)) / radius
            dv = (V - pv) / (radius * 4.0)
            puff = np.clip(1.0 - du ** 2 - dv ** 2, 0, 1) ** 0.6 * (V > floor)
            cloud = np.maximum(cloud, puff)
            lit = np.maximum(lit, puff * np.clip((V - pv) / (radius * 4.0) + 0.5, 0, 1))
    cloud *= 0.92
    white = np.array(_linear((0.74, 0.77, 0.82)))[None, None, :] * (1 - lit[..., None]) +         np.array(_linear((1.0, 0.99, 0.96)))[None, None, :] * lit[..., None]
    day = day * (1 - cloud[..., None]) + white * cloud[..., None]
    day_pixels = np.concatenate([np.clip(day, 0, 1), np.ones((sh, sw, 1))], axis=2)
    day_img = bpy.data.images.new("sky_day", sw, sh)
    day_img.pixels.foreach_set(day_pixels.astype(np.float32).ravel())
    horizon = np.array(_linear((0.15, 0.22, 0.38)))
    zenith = np.array(_linear((0.01, 0.02, 0.06)))
    t = np.clip(elevation / math.radians(40), 0, 1) ** 0.6
    color = horizon * (1 - t[..., None]) + zenith * t[..., None]
    # the moon, its halo and the light it throws on the haze around it
    moon_azimuth, moon_elevation = MOON_SKY
    cos_angle = (np.cos(elevation) * math.cos(moon_elevation) * np.cos(azimuth - moon_azimuth)
                 + np.sin(elevation) * math.sin(moon_elevation))
    angle = np.degrees(np.arccos(np.clip(cos_angle, -1, 1)))
    color += np.array(_linear((0.55, 0.62, 0.78)))[None, None, :] * (np.exp(-angle / 7.0) * 0.6)[..., None]
    color = np.where((angle < 1.6)[..., None], np.array(_linear((0.96, 0.95, 0.88))), color)
    # stars, fewer towards the horizon's haze
    stars = rng.random((sh, sw)) > 0.9978
    bright = rng.uniform(0.3, 1.0, (sh, sw)) * np.clip((elevation - math.radians(4)) / math.radians(12), 0, 1)
    color += (stars * bright * (angle > 4))[..., None] * 0.9
    pixels = np.concatenate([np.clip(color, 0, 1), np.ones((sh, sw, 1))], axis=2)
    img = bpy.data.images.new("sky_color", sw, sh)
    img.pixels.foreach_set(pixels.astype(np.float32).ravel())
    obj.data.materials.append(export_material("sky_export", day_img, white_image(), img))
    moon = centre + Vector((math.cos(moon_azimuth) * math.cos(moon_elevation),
                            math.sin(moon_azimuth) * math.cos(moon_elevation),
                            math.sin(moon_elevation))) * (SKY_RADIUS - 10.0)
    glow(moon, (0.75, 0.82, 1.0), 22.0, GLOW_STEADY)
    return obj, (day_img, img)


# --- the caribbean: islands with white beaches, palms and turquoise shallows -----------------------------

# x, y, radius, height of the land, seed; on the horizon behind the ship as the fly-in sees it, clear of the moon
ISLANDS = [(-25.0, 125.0, 22.0, 9.0, 1), (80.0, 170.0, 14.0, 6.0, 2), (140.0, 50.0, 30.0, 13.0, 3)]
LIGHTHOUSE = (-30.0, 112.0)  # on the nearest island's seaward side, facing the ship
SHELF = 30.0  # how far the shallows reach out from the beach
DEEP = SEA_Z - 7.0
ISLAND_TEX = 1024
PALM_WAVE = 0.45  # how far a frond's tip moves in the gusts


def _coast(island, angle):
    """The island's radius towards angle: a ragged, lobed coast."""
    x, y, r, h, seed = island
    rng = np.random.default_rng(seed)
    phases = rng.uniform(0, 2 * math.pi, 4)
    wobble = sum(np.sin(angle * k + phases[k - 2]) / k for k in range(2, 6))
    return r * (1.0 + 0.18 * wobble)


def seabed(x, y):
    """The ground's height under the sea and on the islands: deep water, a sandy shelf rising to the beach,
    the land behind it."""
    z = np.full(np.shape(x), DEEP, dtype=float)
    for island in ISLANDS:
        cx, cy, r, h, seed = island
        d = np.hypot(x - cx, y - cy)
        coast = _coast(island, np.arctan2(y - cy, x - cx))
        t = d / coast
        rng = np.random.default_rng(seed + 10)
        bumps = sum(np.sin((x * fx + y * fy) + p) for fx, fy, p in rng.uniform(-0.4, 0.4, (5, 3))) * 0.25
        land = SEA_Z + 0.9 + h * np.clip(1.0 - t, 0, 1) ** 1.4 + bumps * np.clip(1.0 - t, 0, 1)
        beach = SEA_Z + 0.9 - 1.2 * np.clip((d - coast) / 4.0, 0, 1)
        shelf_t = np.clip((d - coast - 4.0) / SHELF, 0, 1)
        shelf = SEA_Z - 0.3 + (DEEP - SEA_Z + 0.3) * (shelf_t * shelf_t * (3 - 2 * shelf_t))
        height = np.where(t < 1.0, land, np.where(d < coast + 4.0, beach, shelf))
        z = np.maximum(z, height)
    return z


def island_colors(x, y, z, distance):
    """Day and night colours of the islands and the sea floor around them, the far ones fading into the horizon."""
    above = z - SEA_Z
    rng_x = np.sin(x * 1.7) * np.cos(y * 1.3) + np.sin(x * 0.37 + y * 0.51) * 1.5
    jungle = np.array(_linear((0.13, 0.36, 0.12))) + np.array(_linear((0.05, 0.10, 0.03))) * rng_x[..., None]
    sand = np.array(_linear((0.95, 0.90, 0.74)))
    wet = np.array(_linear((0.80, 0.74, 0.58)))
    floor = np.array(_linear((0.86, 0.84, 0.70)))
    grass = np.array(_linear((0.30, 0.42, 0.22)))
    t_land = np.clip((above - 1.1) / 0.8, 0, 1)[..., None]
    t_beach = np.clip((above + 0.3) / 0.6, 0, 1)[..., None]
    under = floor * (1 - 0.35 * np.clip(-above / 7.0, 0, 1))[..., None]
    patches = (np.sin(x * 0.9 + 2.0) * np.sin(y * 0.7) > 0.6)[..., None]
    under = np.where(patches & (above[..., None] < -1.5), grass, under)
    color = under * (1 - t_beach) + (wet * (1 - t_beach) + sand * t_beach) * t_beach
    color = color * (1 - t_land) + jungle * t_land
    horizon = np.array(_linear((0.78, 0.86, 0.94)))
    fade = np.clip((distance - 60.0) / 180.0, 0, 0.6)[..., None]
    day = color * (1 - fade) + horizon * fade
    night = color * np.array((0.10, 0.13, 0.22)) + np.array(_linear((0.02, 0.03, 0.07))) * fade
    return np.clip(day, 0, 1), np.clip(night, 0, 1)


def palette_image(name, colors):
    """A row of flat swatches, for parts coloured whole."""
    width = len(colors)
    img = bpy.data.images.new(name, width * 8, 8)
    pixels = np.zeros((8, width * 8, 4), np.float32)
    for i, c in enumerate(colors):
        pixels[:, i * 8:(i + 1) * 8, :3] = _linear(c)
        pixels[:, i * 8:(i + 1) * 8, 3] = 1.0
    img.pixels.foreach_set(pixels.ravel())
    return img


def palm(verts, faces, uvs, waves, base, lean, height, rng):
    """A coconut palm: a trunk curving away from lean, a crown of drooping fronds; uvs pick the palette's trunk
    or frond swatch (see islands()), waves how the fronds give in the wind."""
    wind = Vector((1.0, 0.35, 0.0)).normalized()
    segments, sides = 7, 6
    points = []
    for k in range(segments + 1):
        t = k / segments
        points.append(base + Vector((lean.x * t * t * height * 0.35, lean.y * t * t * height * 0.35, t * height)))
    for k, p in enumerate(points):
        radius = 0.32 - 0.14 * k / segments
        for s in range(sides):
            a = 2 * math.pi * s / sides
            verts.append(p + Vector((math.cos(a), math.sin(a), 0.0)) * radius)
            uvs.append((4.5 / 6, 0.5))
            waves.append((wind, 0.08 * (k / segments) ** 2))
    start = len(verts) - (segments + 1) * sides
    for k in range(segments):
        for s in range(sides):
            a = start + k * sides + s
            b = start + k * sides + (s + 1) % sides
            faces.append((a, b, b + sides, a + sides))
    top = points[-1]
    for f in range(9):
        a = 2 * math.pi * f / 9 + rng.uniform(-0.2, 0.2)
        out = Vector((math.cos(a), math.sin(a), 0.0))
        side = Vector((-out.y, out.x, 0.0))
        length = rng.uniform(3.2, 4.2)
        row = []
        for k in range(6):
            t = k / 5
            centre = top + out * length * t + Vector((0.0, 0.0, 0.9 * t - 2.2 * t * t))
            width = 0.75 * math.sin(math.pi * min(t * 1.1, 1.0)) + 0.05
            for edge in (-1, 1):
                verts.append(centre + side * width * edge + Vector((0.0, 0.0, 0.08 * t)))
                uvs.append((5.5 / 6, 0.5))
                waves.append((wind, t))
            row.append(len(verts) - 2)
        for k in range(5):
            a, b = row[k], row[k + 1]
            faces.append((a, a + 1, b + 1, b))
            faces.append((b, b + 1, a + 1, a))


def lighthouse(verts, faces, uvs, base):
    """A lighthouse: a tapering tower in red and white bands, a dark gallery, the lantern room; uvs pick the
    palette's swatches (see islands()). Returns where its lamp burns."""
    height, bands, sides = 11.0, 6, 16

    def ring(z, radius, swatch):
        start = len(verts)
        for k in range(sides):
            a = 2 * math.pi * k / sides
            verts.append(base + Vector((math.cos(a) * radius, math.sin(a) * radius, z)))
            uvs.append(((swatch + 0.5) / 6, 0.5))
        return start

    def band(z0, z1, r0, r1, swatch):
        a, b = ring(z0, r0, swatch), ring(z1, r1, swatch)
        for k in range(sides):
            faces.append((a + k, a + (k + 1) % sides, b + (k + 1) % sides, b + k))

    for i in range(bands):
        z0, z1 = height * i / bands, height * (i + 1) / bands
        band(z0, z1, 1.6 - 0.6 * z0 / height, 1.6 - 0.6 * z1 / height, i % 2)
    # gallery, lantern, cap
    band(height, height + 0.25, 1.5, 1.5, 2)
    band(height + 0.25, height + 1.6, 0.8, 0.8, 3)
    band(height + 1.6, height + 2.4, 0.95, 0.05, 2)
    return base + Vector((0.0, 0.0, height + 0.9))


# seagulls circling high over the ship, above the game's camera so they never cover the field
GULL_CENTRE = Vector((2.0, 4.0, 0.0))
GULLS = [(16.0, 24.0, 0.0), (21.0, 27.0, 2.1), (26.0, 25.0, 4.0), (19.0, 29.0, 5.2), (28.0, 26.5, 1.2)]  # radius, height, angle
GULL_SIZE = 2.2  # larger than life, a gull of a few pixels would be lost against the sky
GULL_SECONDS = 26.0  # one round of the circle
GULL_FPS = 10
GULL_WAVE = 0.3  # how far a wingtip moves


def gull(verts, faces, uvs, waves, at, heading):
    """A gull gliding along heading (z-up): a slim body, wings bent at the wrist with dark tips; the wings' "wave"
    colour lifts the tips."""
    forward = Vector((math.cos(heading), math.sin(heading), 0.0)) * GULL_SIZE
    side = Vector((-forward.y, forward.x, 0.0))
    up = Vector((0.0, 0.0, GULL_SIZE))
    start = len(verts)
    body = [(0.45, 0.0, 0.0), (0.0, 0.09, 0.05), (0.0, -0.09, 0.05), (0.0, 0.0, -0.07), (-0.5, 0.0, 0.02)]
    for f, sd, u in body:
        verts.append(at + forward * f + side * sd + up * u)
        uvs.append((0.5 / 3, 0.5))
        waves.append((Vector((0.0, 0.0, 1.0)), 0.0))
    for a, b, c in ((0, 1, 2), (0, 2, 3), (0, 3, 1), (4, 2, 1), (4, 3, 2), (4, 1, 3)):
        faces.append((start + a, start + b, start + c))
    for s in (1, -1):
        root = len(verts)
        # root front and back, wrist front and back, tip
        # the gull's "M": the arm rising to the wrist, the hand drooping to the tip
        for f, sd, u, swatch, give in ((0.15, 0.06, 0.04, 1, 0.0), (-0.15, 0.06, 0.04, 1, 0.0),
                                       (0.05, 0.42, 0.26, 1, 0.45), (-0.2, 0.42, 0.26, 1, 0.45),
                                       (-0.3, 0.8, 0.0, 2, 1.0)):
            verts.append(at + forward * f + side * sd * s + up * u)
            uvs.append(((swatch + 0.5) / 3, 0.5))
            waves.append((Vector((0.0, 0.0, 1.0)), give))
        quads = [(0, 1, 3, 2), (2, 3, 4, 4)]
        for q in quads:
            face = tuple(root + i for i in (q if s > 0 else q[::-1]))
            faces.append(tuple(dict.fromkeys(face)))
            faces.append(tuple(dict.fromkeys(face[::-1])))


def gulls():
    """The gulls as one object under a pivot turning once round the circle in GULL_SECONDS ("gulls")."""
    verts, faces, uvs, waves = [], [], [], []
    for radius, height, angle in GULLS:
        at = Vector((math.cos(angle) * radius, math.sin(angle) * radius, height))
        gull(verts, faces, uvs, waves, at, angle + math.pi / 2)
    mesh = bpy.data.meshes.new("gulls")
    mesh.from_pydata([tuple(v) for v in verts], [], faces)
    mesh.validate()
    obj = bpy.data.objects.new("gulls", mesh)
    bpy.context.scene.collection.objects.link(obj)
    uv = mesh.uv_layers.new(name="UVMap")
    for loop in mesh.loops:
        uv.data[loop.index].uv = uvs[loop.vertex_index]
    colors = mesh.color_attributes.new("wave", "FLOAT_COLOR", "POINT")
    for i, (direction, amount) in enumerate(waves):
        colors.data[i].color = (direction.x * 0.5 + 0.5, direction.y * 0.5 + 0.5, direction.z * 0.5 + 0.5, amount)
    mesh.color_attributes.active_color = colors
    mesh.color_attributes.render_color_index = mesh.color_attributes.active_color_index
    for poly in mesh.polygons:
        poly.use_smooth = False
    # seen from below against the sky: a white body, grey wings, black tips
    day = palette_image("gulls_day", [(0.97, 0.97, 0.95), (0.55, 0.58, 0.64), (0.08, 0.08, 0.1)])
    night = palette_image("gulls_night", [(0.12, 0.13, 0.18), (0.08, 0.09, 0.13), (0.02, 0.02, 0.03)])
    obj.data.materials.append(export_material("gulls_export", day, white_image(), night))
    obj["wave"] = [GULL_WAVE]
    pivot = bpy.data.objects.new("gull_pivot", None)
    pivot.location = GULL_CENTRE
    bpy.context.scene.collection.objects.link(pivot)
    obj.parent = pivot
    bpy.context.preferences.edit.keyframe_new_interpolation_type = "LINEAR"
    pivot.animation_data_create()
    pivot.animation_data.action = bpy.data.actions.new("gulls")
    frames = int(GULL_SECONDS * GULL_FPS)
    for k in range(5):
        pivot.rotation_euler = (0.0, 0.0, -2 * math.pi * k / 4)
        pivot.keyframe_insert("rotation_euler", index=2, frame=frames * k // 4)
    bpy.context.scene.render.fps = GULL_FPS
    bpy.context.scene.frame_start, bpy.context.scene.frame_end = 0, frames
    return [obj, pivot], [day, night]


def islands():
    """The islands with their sandy shallows as one object, the palms as another (they sway, see "wave")."""
    objects, images = [], []
    for index, island in enumerate(ISLANDS):
        cx, cy, r, h, seed = island
        reach = r * 1.25 + 4.0 + SHELF
        step = 1.0
        xs = np.arange(cx - reach, cx + reach + 1e-6, step)
        ys = np.arange(cy - reach, cy + reach + 1e-6, step)
        X, Y = np.meshgrid(xs, ys)
        Z = seabed(X, Y)
        nx, ny = len(xs), len(ys)
        idx = np.arange(nx * ny).reshape(ny, nx)
        quads = np.stack([idx[:-1, :-1].ravel(), idx[:-1, 1:].ravel(), idx[1:, 1:].ravel(), idx[1:, :-1].ravel()], 1)
        keep = (Z.ravel()[quads] > DEEP + 0.05).any(axis=1)
        mesh = bpy.data.meshes.new(f"island{index}")
        mesh.from_pydata(np.stack([X.ravel(), Y.ravel(), Z.ravel()], 1).tolist(), [], quads[keep].tolist())
        mesh.validate()
        obj = bpy.data.objects.new(f"island{index}", mesh)
        bpy.context.scene.collection.objects.link(obj)
        uv = mesh.uv_layers.new(name="UVMap")
        co = np.array([mesh.vertices[l.vertex_index].co[:] for l in mesh.loops])
        uv.data.foreach_set("uv", np.stack([(co[:, 0] - xs[0]) / (xs[-1] - xs[0]),
                                            (co[:, 1] - ys[0]) / (ys[-1] - ys[0])], 1).ravel())
        for poly in mesh.polygons:
            poly.use_smooth = True
        u = (np.arange(ISLAND_TEX) + 0.5) / ISLAND_TEX
        TX, TY = np.meshgrid(xs[0] + u * (xs[-1] - xs[0]), ys[0] + u * (ys[-1] - ys[0]))
        TZ = seabed(TX, TY)
        distance = np.hypot(TX - 6.5, TY - YC)
        day, night = island_colors(TX, TY, TZ, distance)
        day_img = bpy.data.images.new(f"island{index}_day", ISLAND_TEX, ISLAND_TEX)
        day_img.pixels.foreach_set(np.concatenate([day, np.ones(day.shape[:2] + (1,))], 2).astype(np.float32).ravel())
        night_img = bpy.data.images.new(f"island{index}_night", ISLAND_TEX, ISLAND_TEX)
        night_img.pixels.foreach_set(np.concatenate([night, np.ones(night.shape[:2] + (1,))], 2).astype(np.float32).ravel())
        obj.data.materials.append(export_material(f"island{index}_export", day_img, white_image(), night_img))
        objects.append(obj)
        images += [day_img, night_img]

    verts, faces, uvs, waves = [], [], [], []
    for island in ISLANDS:
        cx, cy, r, h, seed = island
        rng = np.random.default_rng(seed + 20)
        for _ in range(int(r * 0.6)):
            a = rng.uniform(0, 2 * math.pi)
            d = _coast(island, a) * rng.uniform(0.55, 0.92)
            x, y = cx + math.cos(a) * d, cy + math.sin(a) * d
            base = Vector((x, y, float(seabed(np.array(x), np.array(y))) - 0.2))
            lean = Vector((math.cos(a), math.sin(a), 0.0)) * rng.uniform(0.4, 1.0)
            palm(verts, faces, uvs, waves, base, lean, rng.uniform(5.5, 8.0), rng)
    # the lighthouse goes in with the palms, standing still (no give in the wind)
    ground = float(seabed(np.array(LIGHTHOUSE[0]), np.array(LIGHTHOUSE[1])))
    start = len(verts)
    lamp = lighthouse(verts, faces, uvs, Vector((LIGHTHOUSE[0], LIGHTHOUSE[1], ground - 0.3)))
    waves += [(Vector((1.0, 0.0, 0.0)), 0.0)] * (len(verts) - start)
    glow(lamp, (1.0, 0.85, 0.5), 2.2, GLOW_PULSE)
    mesh = bpy.data.meshes.new("palms")
    mesh.from_pydata([tuple(v) for v in verts], [], faces)
    mesh.validate()
    obj = bpy.data.objects.new("palms", mesh)
    bpy.context.scene.collection.objects.link(obj)
    uv = mesh.uv_layers.new(name="UVMap")
    for loop in mesh.loops:
        uv.data[loop.index].uv = uvs[loop.vertex_index]
    colors = mesh.color_attributes.new("wave", "FLOAT_COLOR", "POINT")
    for i, (direction, amount) in enumerate(waves):
        colors.data[i].color = (direction.x * 0.5 + 0.5, direction.y * 0.5 + 0.5, direction.z * 0.5 + 0.5, amount)
    mesh.color_attributes.active_color = colors
    mesh.color_attributes.render_color_index = mesh.color_attributes.active_color_index
    # flat: the fronds' two sides share their vertices, smooth normals would cancel out
    for poly in mesh.polygons:
        poly.use_smooth = False
    # lighthouse white, red, iron, lantern glass; palm trunk, fronds
    day = palette_image("palms_day", [(0.95, 0.94, 0.90), (0.78, 0.14, 0.12), (0.12, 0.12, 0.14), (0.95, 0.85, 0.55),
                                      (0.42, 0.30, 0.18), (0.20, 0.50, 0.14)])
    night = palette_image("palms_night", [(0.10, 0.11, 0.16), (0.08, 0.03, 0.05), (0.02, 0.02, 0.03), (1.0, 0.85, 0.5),
                                          (0.05, 0.05, 0.07), (0.03, 0.06, 0.06)])
    obj.data.materials.append(export_material("palms_export", day, white_image(), night))
    obj["wave"] = [PALM_WAVE]
    objects.append(obj)
    images += [day, night]
    return objects, images


# --- baking ---------------------------------------------------------------------------------------

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
    scene.world.light_settings.distance = 1.5


def light_rig(day=False):
    """By night the moon (casting the mast's shadow, the same light as the blocks' shadow cookie), the lanterns and
    a bluish night sky all around; by day the sun from the same direction and a bright sky."""
    scene = bpy.context.scene
    for obj in [o for o in scene.objects if o.type == "LIGHT"]:
        bpy.data.objects.remove(obj, do_unlink=True)
    scene.world = scene.world or bpy.data.worlds.new("World")
    world = scene.world
    world.use_nodes = True
    background = next(n for n in world.node_tree.nodes if n.type == "BACKGROUND")
    background.inputs["Color"].default_value = (0.62, 0.72, 0.92, 1) if day else (0.36, 0.42, 0.62, 1)
    background.inputs["Strength"].default_value = 1.3 if day else 1.0
    sun = bpy.data.lights.new("sun" if day else "moon", "SUN")
    sun.energy = 4.2 if day else 2.6
    sun.color = (1.0, 0.95, 0.86) if day else (0.86, 0.9, 1.0)
    sun.angle = math.radians(1.0 if day else 2.5)
    obj = bpy.data.objects.new(sun.name, sun)
    scene.collection.objects.link(obj)
    obj.rotation_euler = (-MOON).to_track_quat("-Z", "Y").to_euler()
    if day:
        return
    for glow_obj in GLOWS:
        if int(glow_obj["glow"][4]) != GLOW_FLAME:
            continue
        lamp = bpy.data.lights.new("lantern", "POINT")
        lamp.energy = 160
        lamp.color = (1.0, 0.6, 0.3)
        lamp.shadow_soft_size = 0.12
        light = bpy.data.objects.new("lantern", lamp)
        # in front of the lantern, its glass and housing would swallow the light
        light.location = glow_obj.location + Vector((-0.3, 0.0, 0.0))
        scene.collection.objects.link(light)


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
        bpy.ops.object.bake(type="DIFFUSE", pass_filter={"COLOR"}, margin=16)
    elif kind == "LIT":
        bpy.context.scene.cycles.samples = 96
        bpy.ops.object.bake(type="DIFFUSE", pass_filter={"COLOR", "DIRECT", "INDIRECT"}, margin=16)
    else:
        bpy.context.scene.cycles.samples = 64
        bpy.ops.object.bake(type="AO", margin=16)


def gltf_output_group():
    group = bpy.data.node_groups.get("glTF Material Output")
    if group:
        return group
    group = bpy.data.node_groups.new("glTF Material Output", "ShaderNodeTree")
    group.interface.new_socket("Occlusion", in_out="INPUT", socket_type="NodeSocketFloat")
    return group


_WHITE = []


def white_image():
    if not _WHITE:
        img = bpy.data.images.new("white", 4, 4)
        img.colorspace_settings.name = "Non-Color"
        img.pixels.foreach_set(np.ones(4 * 4 * 4, np.float32))
        _WHITE.append(img)
    return _WHITE[0]


def export_material(name, color_img, ao_img, night_img=None):
    """Colour (by day), ambient occlusion and, as the emissive texture, the colour by night."""
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = next(n for n in nodes if n.type == "BSDF_PRINCIPLED")
    bsdf.inputs["Roughness"].default_value = 0.8
    color = nodes.new("ShaderNodeTexImage")
    color.image = color_img
    links.new(color.outputs["Color"], bsdf.inputs["Base Color"])
    if night_img is not None:
        night = nodes.new("ShaderNodeTexImage")
        night.image = night_img
        links.new(night.outputs["Color"], bsdf.inputs["Emission Color"])
        bsdf.inputs["Emission Strength"].default_value = 1.0
    ao = nodes.new("ShaderNodeTexImage")
    ao.image = ao_img
    split = nodes.new("ShaderNodeSeparateColor")
    links.new(ao.outputs["Color"], split.inputs["Color"])
    out = nodes.new("ShaderNodeGroup")
    out.node_tree = gltf_output_group()
    links.new(split.outputs[0], out.inputs["Occlusion"])
    return mat


def join_props(name, parts=None):
    objs = [o for o in (PROPS if parts is None else parts) if o.name in bpy.data.objects]
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
    bpy.ops.uv.smart_project(angle_limit=1.15, island_margin=0.004, scale_to_bounds=True)
    bpy.ops.object.mode_set(mode="OBJECT")
    return obj


def fill_unbaked(img, value=None):
    """Unbaked texels (still magenta) get the average colour, or value: black there bleeds into the mipmaps."""
    size = img.size[0] * img.size[1]
    texels = np.empty(size * 4, np.float32)
    img.pixels.foreach_get(texels)
    texels = texels.reshape(-1, 4)
    unbaked = (texels[:, 0] > 0.99) & (texels[:, 1] < 0.01) & (texels[:, 2] > 0.99)
    texels[unbaked, :3] = texels[~unbaked, :3].mean(axis=0) if value is None else value
    img.pixels.foreach_set(texels.ravel())
    return texels


def pixels(img):
    texels = np.empty(img.size[0] * img.size[1] * 4, np.float32)
    img.pixels.foreach_get(texels)
    return texels.reshape(-1, 4)


def light(obj, albedo, size, field=None, exposure=None, gain=1.0):
    """Bakes the colour lit by light_rig() into a new image, exposed so that the field keeps the albedo's
    brightness (the game lights it once more by its normals), or by the given exposure."""
    lit = new_image(f"{obj.name}_lit", *size)
    bake(obj, "LIT", lit)
    fill_unbaked(lit)
    a, b = pixels(albedo), pixels(lit)
    if exposure is None:
        mask = slice(None) if field is None else field
        exposure = float(np.median(a[mask, :3].mean(axis=1)) / max(np.median(b[mask, :3].mean(axis=1)), 1e-4)) * gain
    b[:, :3] = np.clip(b[:, :3] * exposure, 0, 1)
    lit.pixels.foreach_set(b.ravel())
    return lit, exposure


def field_texels(img):
    """The deck texels on the field (planar mapped, see deck())."""
    w, h = img.size
    u = (np.arange(w) + 0.5) / w
    v = (np.arange(h) + 0.5) / h
    TX, TY = np.meshgrid(DECK_X0 + u * (POOP_X - DECK_X0), DECK_Y0 + v * (RAIL_Y - DECK_Y0))
    return ((TX > 0.3) & (TX < W - 0.3) & (TY > -H + 0.3) & (TY < -0.3)).ravel()


def new_image(name, w, h, data=False):
    img = bpy.data.images.new(name, w, h)
    if data:
        # before baking: changing it later regenerates the image and loses the bake
        img.colorspace_settings.name = "Non-Color"
    img.pixels.foreach_set(np.tile(np.array([1.0, 0.0, 1.0, 1.0], np.float32), w * h))
    return img


def shade_deck(img_ao):
    """The deck's AO, softer on the field so the blocks' own shadows read."""
    w, h = img_ao.size
    ao = np.empty(w * h * 4, np.float32)
    img_ao.pixels.foreach_get(ao)
    ao = ao.reshape(h, w, 4)
    u = (np.arange(w) + 0.5) / w
    v = (np.arange(h) + 0.5) / h
    TX, TY = np.meshgrid(DECK_X0 + u * (POOP_X - DECK_X0), DECK_Y0 + v * (RAIL_Y - DECK_Y0))
    inside = (TX > 0) & (TX < W) & (TY > -H) & (TY < 0)
    # darker towards the deck's edges, the lanterns and the moon light the middle
    dx = np.maximum(np.maximum(-4.0 - TX, TX - (W + 4.0)), 0)
    dy = np.maximum(np.maximum(-H - 3.0 - TY, TY - 2.0), 0)
    vignette = np.clip(1.0 - np.hypot(dx, dy) / 14.0, 0.55, 1.0)
    value = np.where(inside, 0.25 + 0.75 * np.maximum(ao[..., 0], 0.75), (0.3 + 0.7 * ao[..., 0] ** 1.4) * vignette)
    ao[..., :3] = value[..., None]
    img_ao.pixels.foreach_set(ao.ravel())


def clear_scene():
    if not PREVIEW:
        bpy.ops.wm.read_factory_settings(use_empty=True)
        return
    # a running Blender keeps its add-ons (the MCP server among them)
    for collection in (bpy.data.objects, bpy.data.meshes, bpy.data.materials, bpy.data.images, bpy.data.cameras,
                       bpy.data.lights):
        for item in list(collection):
            collection.remove(item)


def build_scene():
    clear_scene()
    _MATERIALS.clear()
    PROPS.clear()
    EXTERIOR.clear()
    CLOTH.clear()
    GLOWS.clear()
    WATER.clear()
    _WHITE.clear()
    m = palette()
    deck_obj = deck(m)
    bulwark(m, gunports=[(-3.4, 1.1)])
    side_bulwarks(m)
    deck_props(m)
    stern(m)
    water = sea()
    return m, deck_obj, water


def preview_camera():
    """The game's view of the field: between the castle's "Upper" and "Lower" cameras, 49 degrees high
    (CameraInterpolation's 1 / (tan(fov / 2) * 0.75) on a 62.6 degree camera)."""
    cam = bpy.data.cameras.new("view")
    cam.sensor_fit = "VERTICAL"
    cam.lens_unit = "FOV"
    cam.angle_y = math.radians(49.0)
    obj = bpy.data.objects.new("view", cam)
    bpy.context.scene.collection.objects.link(obj)
    obj.location = (6.0, -15.9, 12.1)
    obj.rotation_euler = Vector((0.004, 0.64, -0.765)).to_track_quat("-Z", "Y").to_euler()
    scene = bpy.context.scene
    scene.camera = obj
    scene.render.resolution_x, scene.render.resolution_y = 1280, 720
    return obj


def build():
    m, deck_obj, water = build_scene()
    sky_obj, sky_img = sky()
    setup_cycles()

    shore, shore_images = islands()
    flock, flock_images = gulls()
    shore += [o for o in flock if o.type == "MESH"]
    shore_images += flock_images
    water.hide_render = True
    sky_obj.hide_render = True
    for obj in shore:
        obj.hide_render = True
    outside = join_props("exterior", EXTERIOR)
    sails = join_props("cloth", CLOTH)
    sails["wave"] = [1.0]
    props = join_props("props")
    props_color = new_image("props_color", PROP_TEX, PROP_TEX)
    props_ao = new_image("props_ao", AO_TEX, AO_TEX, data=True)
    deck_color = new_image("deck_color", *DECK_TEX)
    deck_ao = new_image("deck_ao", DECK_TEX[0] // 2, DECK_TEX[1] // 2, data=True)
    bake(props, "COLOR", props_color)
    fill_unbaked(props_color)
    bake(deck_obj, "COLOR", deck_color)
    fill_unbaked(deck_color)
    bake(props, "AO", props_ao)
    soft = fill_unbaked(props_ao, 1.0)
    soft[:, :3] = 0.35 + 0.65 * soft[:, :3]
    props_ao.pixels.foreach_set(soft.ravel())
    bake(deck_obj, "AO", deck_ao)
    fill_unbaked(deck_ao, 1.0)
    shade_deck(deck_ao)

    # moon, lanterns and sky: the lit bake holds the sky's occlusion already, the ambient occlusion only deepens it
    light_rig()
    outside.visible_shadow = sails.visible_shadow = False
    deck_lit, exposure = light(deck_obj, deck_color, DECK_TEX, field_texels(deck_color), gain=0.88)
    props_lit, _ = light(props, props_color, (PROP_TEX, PROP_TEX), exposure=exposure)
    outside.visible_shadow = sails.visible_shadow = True
    cloth_color = new_image("cloth_color", CLOTH_TEX, CLOTH_TEX)
    cloth_ao = new_image("cloth_ao", CLOTH_TEX // 2, CLOTH_TEX // 2, data=True)
    bake(sails, "COLOR", cloth_color)
    fill_unbaked(cloth_color)
    bake(sails, "AO", cloth_ao)
    fill_unbaked(cloth_ao, 1.0)
    cloth_lit, _ = light(sails, cloth_color, (CLOTH_TEX, CLOTH_TEX), exposure=exposure)
    outside_color = new_image("exterior_color", EXTERIOR_TEX, EXTERIOR_TEX)
    outside_ao = new_image("exterior_ao", AO_TEX, AO_TEX, data=True)
    bake(outside, "COLOR", outside_color)
    fill_unbaked(outside_color)
    bake(outside, "AO", outside_ao)
    fill_unbaked(outside_ao, 1.0)
    outside_lit, _ = light(outside, outside_color, (EXTERIOR_TEX, EXTERIOR_TEX), exposure=exposure)
    # by day: the sun from the same direction, no lanterns
    light_rig(day=True)
    outside.visible_shadow = sails.visible_shadow = False
    deck_day, day_exposure = light(deck_obj, deck_color, DECK_TEX, field_texels(deck_color), gain=1.1)
    props_day, _ = light(props, props_color, (PROP_TEX, PROP_TEX), exposure=day_exposure)
    outside.visible_shadow = sails.visible_shadow = True
    cloth_day, _ = light(sails, cloth_color, (CLOTH_TEX, CLOTH_TEX), exposure=day_exposure)
    outside_day, _ = light(outside, outside_color, (EXTERIOR_TEX, EXTERIOR_TEX), exposure=day_exposure)
    deck_color, props_color = deck_lit, props_lit
    for img, floor in ((props_ao, 0.55), (deck_ao, 0.6), (outside_ao, 0.55), (cloth_ao, 0.7)):
        ao = pixels(img)
        ao[:, :3] = floor + (1 - floor) * ao[:, :3]
        img.pixels.foreach_set(ao.ravel())
    water.hide_render = False
    sky_obj.hide_render = False
    for obj in shore:
        obj.hide_render = False

    for obj, day_img, night_img, ao_img in ((deck_obj, deck_day, deck_lit, deck_ao),
                                            (props, props_day, props_lit, props_ao),
                                            (outside, outside_day, outside_lit, outside_ao),
                                            (sails, cloth_day, cloth_lit, cloth_ao)):
        obj.data.materials.clear()
        obj.data.materials.append(export_material(f"{obj.name}_export", day_img, ao_img, night_img))

    tris = sum(len(p.vertices) - 2 for o in (deck_obj, props, outside, sails) for p in o.data.polygons)
    os.makedirs(OUT, exist_ok=True)
    path = os.path.join(OUT, "ship.glb")
    bpy.ops.object.select_all(action="SELECT")
    # the cloth's waving goes out as its first vertex colour
    options = bpy.ops.export_scene.gltf.get_rna_type().properties
    colors = {}
    if "export_vertex_color" in options:
        colors["export_vertex_color"] = "ACTIVE"
    elif "export_colors" in options:
        colors["export_colors"] = True
    # the gulls' circling as the scene's one animation, "gulls"
    modes = [m.identifier for m in options["export_animation_mode"].enum_items] if "export_animation_mode" in options else []
    if "SCENE" in modes:
        colors["export_animation_mode"] = "SCENE"
    bpy.ops.export_scene.gltf(filepath=path, export_format="GLB", use_selection=True, export_yup=True,
                              export_image_format="JPEG", export_jpeg_quality=90, export_extras=True,
                              export_animations=True, **colors)
    gltf, binary = glb.read(path)
    channels, samplers = [], []
    for animation in gltf.get("animations", []):
        channels += [dict(c, sampler=c["sampler"] + len(samplers)) for c in animation["channels"]]
        samplers += animation["samplers"]
    if channels:
        gltf["animations"] = [{"name": "gulls", "channels": channels, "samplers": samplers}]
        glb.write(path, gltf, binary)
    for img in (props_color, props_ao, deck_color, deck_ao, outside_lit, outside_ao, cloth_lit, cloth_ao, *sky_img,
                deck_day, props_day, outside_day, cloth_day, white_image(), *shore_images,
                *(img for _, img in WATER)):
        img.pack()
    preview_camera()
    bpy.context.scene.name = "pirate_ship"
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, "ship.blend"), compress=True)
    print("RESULT", path, "tris", tris, "kb", os.path.getsize(path) // 1024, flush=True)


if PREVIEW:
    build_scene()
    sky()
    preview_camera()
else:
    build()
