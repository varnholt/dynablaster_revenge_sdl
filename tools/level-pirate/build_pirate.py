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

ARGS = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
OUT = ARGS[0] if ARGS else os.path.join(os.path.dirname(os.path.abspath(__file__)), "out")
PREVIEW = globals().get("PIRATE_PREVIEW", False) or bool(os.environ.get("PIRATE_PREVIEW"))

W, H = 13, 11
DECK_Z = -0.08
POOP_X = 16.6  # the stern deck's wall facing the field
POOP_Z = 2.6
RAIL_Y = 3.2  # inner face of the bulwark behind the field
SEA_Z = -2.4
DECK_X0, DECK_X1 = -11.0, 24.0
DECK_Y0 = -22.0
DECK_TEX = (4096, 3072)
WALL = 0.35  # the bulwarks' thickness
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
        rope=material("rope", (0.46, 0.34, 0.19), 0.15, 12.0),
        green=material("green", (0.18, 0.62, 0.26), 0.08, 9.0),
        glass=material("glass", (0.06, 0.10, 0.09), 0.10, 6.0),
        lamp=material("lamp", (1.00, 0.86, 0.50), 0.05, 6.0),
        hole=material("hole", (0.03, 0.02, 0.015), 0.10, 6.0),
    )


# --- primitive props ------------------------------------------------------------------------------

PROPS = []


def _finish(obj, mat, smooth_shade=True):
    obj.data.materials.append(mat)
    for p in obj.data.polygons:
        p.use_smooth = smooth_shade
    PROPS.append(obj)
    return obj


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
    # the outer side down to the sea, seen in the water only
    slab(x0 - WALL, x1, RAIL_Y + thick, RAIL_Y + thick + 0.1, SEA_Z - 0.6, height, m["hull"])
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
    """The ship's sides left of the field and behind the viewer, like the one behind the field, without gunports."""
    height = 1.25
    x0, y0 = DECK_X0, DECK_Y0
    # west: planked wall, its outer side down to the sea, cap rail and posts
    slab(x0 - WALL, x0, y0 - WALL, RAIL_Y + WALL, DECK_Z, height, m["hull"])
    slab(x0 - WALL - 0.1, x0 - WALL, y0 - WALL - 0.1, RAIL_Y + WALL + 0.1, SEA_Z - 0.6, height, m["hull"])
    slab(x0 - WALL - 0.08, x0 + 0.08, y0 - WALL - 0.08, RAIL_Y + WALL + 0.08, height, height + 0.12, m["gold"],
         bevel=0.03)
    y = y0 + 0.6
    while y < RAIL_Y - 0.3:
        slab(x0, x0 + 0.14, y - 0.11, y + 0.11, DECK_Z, height, m["wood"])
        y += 1.15
    # south, up to the stern deck
    slab(x0, POOP_X, y0 - WALL, y0, DECK_Z, height, m["hull"])
    slab(x0, POOP_X, y0 - WALL - 0.1, y0 - WALL, SEA_Z - 0.6, height, m["hull"])
    slab(x0, POOP_X, y0 - WALL - 0.08, y0 + 0.08, height, height + 0.12, m["gold"], bevel=0.03)
    x = x0 + 0.6
    while x < POOP_X - 0.3:
        slab(x - 0.11, x + 0.11, y0, y0 + 0.14, DECK_Z, height, m["wood"])
        x += 1.15


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
    y0, y1 = DECK_Y0, RAIL_Y + 0.35
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
        while x < x1:
            baluster(m, (x, y, top), 0.85)
            x += 0.3
        bar((POOP_X + 2.4, y, rail_z), (x1, y, rail_z), 0.12, 0.09, m["redwood"])
    # the ship's side under the stern deck, towards the sea
    slab(POOP_X, x1 + 0.1, y1, y1 + 0.1, SEA_Z - 0.6, top + 0.9, m["stern_side"])
    # the stern deck's other sides towards the sea, with a rail
    slab(x1, x1 + 0.1, y0 - 0.1, y1, SEA_Z - 0.6, top + 0.9, m["hull"])
    slab(POOP_X, x1, y0 - 0.1, y0, SEA_Z - 0.6, top + 0.9, m["hull"])
    for (a, b) in (((x1 - 0.12, y0 + 0.1), (x1 - 0.12, y1 - 0.1)), ((POOP_X + 0.1, y0 + 0.12), (x1 - 0.1, y0 + 0.12))):
        a, b = Vector((*a, top)), Vector((*b, top))
        count = int((b - a).length / 0.3)
        for i in range(1, count):
            baluster(m, tuple(a + (b - a) * i / count), 0.85)
        bar(a + Vector((0, 0, 0.85)), b + Vector((0, 0, 0.85)), 0.12, 0.09, m["redwood"])
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
    along = tip - root
    axis = along.normalized()
    side = axis.cross(Vector((0, 0, 1))).normalized()
    up = side.cross(axis).normalized()
    # the canvas bundled on top of the boom: pinched by the gaskets, wrinkled between them
    t0, t1, ties = 0.1, 0.94, 6
    rings, segments = 120, 20
    verts, faces, pinch = [], [], []
    for i in range(rings + 1):
        u = i / rings
        t = t0 + (t1 - t0) * u
        taper = math.sin(math.pi * (0.06 + 0.88 * u)) ** 0.6
        bunch = 0.72 + 0.28 * abs(math.sin(math.pi * ties * u))
        radius = 0.5 * taper * bunch
        centre = root + along * t + up * (0.12 + radius * 0.55)
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
        radius = 0.5 * taper * 0.72
        centre = root + along * t + up * (0.12 + radius * 0.55 - 0.08)
        bpy.ops.mesh.primitive_torus_add(major_radius=1.0, minor_radius=0.05, major_segments=20, minor_segments=6)
        obj = bpy.context.object
        obj.location = centre
        obj.rotation_euler = axis.to_track_quat("Z", "Y").to_euler()
        obj.scale = (radius * 1.3 + 0.12, radius * 1.0 + 0.16, 0.6)
        bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
        _finish(obj, m["darkrope"])
    lathe(tip - axis * 0.2, tip + axis * 0.05, ((0, 0.2), (1, 0.2)), m["iron"], 12)
    return root, tip


def rigging(m, mast_xy):
    """Shrouds from the rail to the mast top and the lines of the masts further forward."""
    mx, my = mast_xy
    top = Vector((mx, my, 34))
    for x in (-8.6, -7.8, -7.0, -6.2):
        foot = Vector((x, RAIL_Y + 0.2, 1.37))
        rod(foot, top, 0.035, m["rope"], 6)
        sphere(tuple(foot + Vector((0, 0, 0.05))), 0.1, m["darkwood"], scale=(1, 1, 0.7), subdiv=1)
    # ratlines across the shrouds
    for k in range(1, 9):
        t = k * 0.03
        a = Vector((-8.6, RAIL_Y + 0.2, 1.37)).lerp(top, t)
        b = Vector((-6.2, RAIL_Y + 0.2, 1.37)).lerp(top, t)
        rod(a, b, 0.02, m["rope"], 5)
    # lines up to the foremast beyond the rail
    for x, target in ((3.0, (5.0, 26.0, 30.0)), (4.2, (5.0, 26.0, 30.0)), (9.0, (7.5, 26.0, 30.0)),
                      (10.2, (7.5, 26.0, 30.0))):
        foot = Vector((x, RAIL_Y + 0.18, 1.37))
        rod(foot, Vector(target), 0.035, m["rope"], 6)
        sphere(tuple(foot + Vector((0, 0, 0.05))), 0.1, m["darkwood"], scale=(1, 1, 0.7), subdiv=1)
    # the foremast, far ahead with its yard
    lathe((6.2, 26.0, -1.0), (6.2, 26.0, 34), ((0, 0.5), (1, 0.34)), m["wood"], 16)
    rod((-1.5, 26.0, 11.0), (14.0, 26.0, 11.0), 0.2, m["wood"], 10)
    for x in (0.0, 2.5, 4.5, 8.0, 10.0, 12.5):
        sphere((x, 26.0, 10.75), 0.22, m["darkwood"], scale=(1, 0.6, 1.2), subdiv=1)


def deck_props(m):
    mast_xy = (-2.4, -4.1)
    mast(m, *mast_xy)
    boom_with_sail(m, mast_xy, 2.9, (-10.5, 3.0, 6.2))
    rigging(m, mast_xy)

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

SEA = (-90.0, 110.0, -90.0, 100.0)
HULL = (DECK_X0 - WALL - 0.1, DECK_X1 + 0.1, DECK_Y0 - WALL - 0.1, RAIL_Y + WALL + 0.1)
WATER = []


def sea():
    """The water around the ship for the game's water shader ("water": [deep r, g, b] in its extras); its texture
    is data: red the depth, green the foam along the hull."""
    x0, x1, y0, y1 = SEA
    step = 5.0
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

    size = 1024
    u = (np.arange(size) + 0.5) / size
    TX, TY = np.meshgrid(x0 + u * (x1 - x0), y0 + u * (y1 - y0))
    depth = np.ones_like(TX)
    # foam along the hull
    hx0, hx1, hy0, hy1 = HULL
    away = np.hypot(np.maximum(np.maximum(hx0 - TX, TX - hx1), 0), np.maximum(np.maximum(hy0 - TY, TY - hy1), 0))
    foam = np.clip(1.0 - away / 1.1, 0, 1) ** 2 * 0.8
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
    obj["water"] = [0.02, 0.07, 0.16]
    WATER.append((obj, img))
    return obj


def sky():
    """A backdrop far behind the sea: the night sky with stars and the moon, its colour painted, not baked."""
    x0, x1 = -150.0, 170.0
    z0, z1 = SEA_Z - 1.0, 70.0
    y = 110.0
    verts = [(x0, y, z0), (x1, y, z0), (x1, y, z1), (x0, y, z1)]
    obj = _mesh("sky", verts, [(0, 3, 2, 1)], material("sky_tmp", (0, 0, 0)), smooth_shade=False)
    PROPS.remove(obj)
    obj.data.materials.clear()
    uv = obj.data.uv_layers.new(name="UVMap")
    uv.data.foreach_set("uv", [0, 0, 0, 1, 1, 1, 1, 0])

    sw, sh = 2048, 512
    u = (np.arange(sw) + 0.5) / sw
    v = (np.arange(sh) + 0.5) / sh
    U, V = np.meshgrid(u, v)
    horizon = np.array(_linear((0.13, 0.20, 0.36)))
    zenith = np.array(_linear((0.01, 0.02, 0.06)))
    t = np.clip(V / 0.55, 0, 1) ** 0.7
    color = horizon * (1 - t[..., None]) + zenith * t[..., None]
    # moon halo
    mu, mv = 0.62, 0.42
    d = np.hypot((U - mu) * (x1 - x0), (V - mv) * (z1 - z0))
    color += np.array(_linear((0.55, 0.62, 0.75)))[None, None, :] * np.exp(-d / 9.0)[..., None] * 0.5
    color = np.where((d < 3.2)[..., None], np.array(_linear((0.95, 0.94, 0.86))), color)
    # stars
    stars = rng.random((sh, sw)) > 0.9975
    bright = rng.uniform(0.3, 1.0, (sh, sw)) * (V > 0.12)
    color += (stars * bright)[..., None] * 0.9
    pixels = np.concatenate([np.clip(color, 0, 1), np.ones((sh, sw, 1))], axis=2)
    img = bpy.data.images.new("sky_color", sw, sh)
    img.pixels.foreach_set(pixels.astype(np.float32).ravel())
    obj.data.materials.append(export_material("sky_export", img, white_image()))
    moon = ((x0 + mu * (x1 - x0)), y - 1.0, z0 + mv * (z1 - z0))
    glow(moon, (0.75, 0.82, 1.0), 9.0, GLOW_STEADY)
    return obj, img


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


def light_rig():
    """The moon (casting the mast's shadow, the same light as the blocks' shadow cookie), the lanterns and a
    bluish night sky all around."""
    scene = bpy.context.scene
    scene.world = scene.world or bpy.data.worlds.new("World")
    world = scene.world
    world.use_nodes = True
    background = next(n for n in world.node_tree.nodes if n.type == "BACKGROUND")
    background.inputs["Color"].default_value = (0.36, 0.42, 0.62, 1)
    background.inputs["Strength"].default_value = 1.0
    sun = bpy.data.lights.new("moon", "SUN")
    sun.energy = 2.6
    sun.color = (0.86, 0.9, 1.0)
    sun.angle = math.radians(2.5)
    obj = bpy.data.objects.new("moon", sun)
    scene.collection.objects.link(obj)
    obj.rotation_euler = (-MOON).to_track_quat("-Z", "Y").to_euler()
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


def light(obj, albedo, size, field=None, exposure=None):
    """Bakes the colour lit by light_rig() into a new image, exposed so that the field keeps the albedo's
    brightness (the game lights it once more by its normals), or by the given exposure."""
    lit = new_image(f"{obj.name}_lit", *size)
    bake(obj, "LIT", lit)
    fill_unbaked(lit)
    a, b = pixels(albedo), pixels(lit)
    if exposure is None:
        mask = slice(None) if field is None else field
        exposure = float(np.median(a[mask, :3].mean(axis=1)) / max(np.median(b[mask, :3].mean(axis=1)), 1e-4))
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

    water.hide_render = True
    sky_obj.hide_render = True
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
    deck_lit, exposure = light(deck_obj, deck_color, DECK_TEX, field_texels(deck_color))
    props_lit, _ = light(props, props_color, (PROP_TEX, PROP_TEX), exposure=exposure)
    deck_color, props_color = deck_lit, props_lit
    for img, floor in ((props_ao, 0.55), (deck_ao, 0.6)):
        ao = pixels(img)
        ao[:, :3] = floor + (1 - floor) * ao[:, :3]
        img.pixels.foreach_set(ao.ravel())
    water.hide_render = False
    sky_obj.hide_render = False

    for obj, color_img, ao_img in ((deck_obj, deck_color, deck_ao), (props, props_color, props_ao)):
        obj.data.materials.clear()
        obj.data.materials.append(export_material(f"{obj.name}_export", color_img, ao_img))

    tris = sum(len(p.vertices) - 2 for o in (deck_obj, props) for p in o.data.polygons)
    os.makedirs(OUT, exist_ok=True)
    path = os.path.join(OUT, "ship.glb")
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.export_scene.gltf(filepath=path, export_format="GLB", use_selection=True, export_yup=True,
                              export_image_format="JPEG", export_jpeg_quality=90, export_extras=True)
    for img in (props_color, props_ao, deck_color, deck_ao, sky_img, white_image(), *(img for _, img in WATER)):
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
