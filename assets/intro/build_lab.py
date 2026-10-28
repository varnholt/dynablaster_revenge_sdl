# The intro's set: Dr's lab from the first world's scenery, its glass dome rebuilt so the dragon can break out
#   blender --background --factory-startup --python build_lab.py -- <out dir>
# writes
#   lab.glb          world1/surroundings_13x11.glb with the dome cut out of its props and a doorway right of it
#   door.glb         that doorway's door, hinged on its origin
#   lights.glb       the lab's lit windows and strips at night
#   dome.glb         the dome broken into voronoi cells, whole in its rest pose; "burst" blows the cells round the
#                    top out along their normals, then they tumble down
# all in the scenery's own coordinates (field coordinates, z up).
import math
import os
import random
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bmesh  # noqa: E402
import bpy  # noqa: E402
from mathutils import Vector  # noqa: E402

import toy  # noqa: E402
from toy import turn  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
SCENERY = os.path.join(HERE, "..", "..", "client", "data", "game", "story", "world1", "surroundings_13x11.glb")

# the dome: the front half of an ellipsoid against the lab's facade
CENTRE = Vector((6.5, 5.6, 0.3))
RADIUS = 3.7
HEIGHT = 3.84
FACADE_Y = 5.62
THICKNESS = 0.08

COLORS = {
    "glass": (0.74, 0.90, 0.96),
    "glass_edge": (0.90, 0.97, 1.0),
    "band": (0.10, 0.25, 0.80),
    "frame": (0.80, 0.82, 0.86),
}

# the door the doctor runs out of: in the facade right of the dome, where the scenery has a grey panel, wide and
# high enough for him to pass with his arms swinging; a short hall behind it
FRONT_Y = 5.90
DOOR_X = 10.88
DOOR_WIDTH = 1.04
DOOR_BOTTOM = 0.27
DOOR_TOP = 1.92
DOOR_DEPTH = 1.5
FRAME_Y = FRONT_Y - 0.05  # the frame's front
DOOR_LEAF = 0.04
# the leaf's hinge, right of the opening, in front of the frame: the leaf opens outwards
DOOR_HINGE = Vector((DOOR_X + DOOR_WIDTH / 2 + 0.04, FRAME_Y - DOOR_LEAF / 2 - 0.005, DOOR_BOTTOM + 0.01))

DOOR_COLORS = {
    "hall": (0.78, 0.70, 0.52),
    "ceiling": (0.90, 0.84, 0.66),
    "floor": (0.42, 0.42, 0.44),
    "frame": (0.60, 0.62, 0.66),
    "door": (0.10, 0.30, 0.85),
    "handle": (0.80, 0.82, 0.86),
}

random.seed(5)


def cut_dome(props):
    """Removes the baked dome's faces from the scenery's props."""
    bm = bmesh.new()
    bm.from_mesh(props.data)
    world = props.matrix_world

    def on_dome(v):
        p = world @ v.co
        if p.z < 0.45 or p.y > FACADE_Y:
            return False
        d = ((p.x - CENTRE.x) ** 2 + (p.y - CENTRE.y) ** 2) / RADIUS ** 2 + ((p.z - CENTRE.z) / HEIGHT) ** 2
        return 0.8 < d < 1.2

    doomed = [f for f in bm.faces if all(on_dome(v) for v in f.verts)]
    bmesh.ops.delete(bm, geom=doomed, context="FACES")
    bm.to_mesh(props.data)
    bm.free()
    print("RESULT removed", len(doomed), "dome faces")


def cut_door(props):
    """A doorway into the facade right of the dome, where its grey panel was: the panel's faces and the facade's
    within the opening go."""
    bm = bmesh.new()
    bm.from_mesh(props.data)
    bm.transform(props.matrix_world)
    left, right = DOOR_X - DOOR_WIDTH / 2, DOOR_X + DOOR_WIDTH / 2

    def facade(f):
        return f.normal.y < -0.99 and all(abs(v.co.y - FRONT_Y) < 1e-3 for v in f.verts)

    def panel(f):
        return all(left - 0.1 < v.co.x < right + 0.1 and FRONT_Y - 0.1 < v.co.y < FRONT_Y + 0.1
                   and DOOR_BOTTOM < v.co.z < DOOR_TOP + 0.1 for v in f.verts)

    doomed = [f for f in bm.faces if panel(f) and not facade(f)]
    bmesh.ops.delete(bm, geom=doomed, context="FACES")
    # the facade cut along the opening's sides and its top, the pieces inside it removed
    for co, no in (((left, 0, 0), (1, 0, 0)), ((right, 0, 0), (1, 0, 0)), ((0, 0, DOOR_TOP), (0, 0, 1))):
        faces = [f for f in bm.faces if facade(f)]
        geom = list({v for f in faces for v in f.verts}) + list({e for f in faces for e in f.edges}) + faces
        bmesh.ops.bisect_plane(bm, geom=geom, dist=1e-5, plane_co=co, plane_no=no)
    opening = [f for f in bm.faces if facade(f) and left < f.calc_center_median().x < right
               and f.calc_center_median().z < DOOR_TOP]
    bmesh.ops.delete(bm, geom=opening, context="FACES")
    bm.transform(props.matrix_world.inverted())
    bm.to_mesh(props.data)
    bm.free()
    print("RESULT door: removed", len(doomed), "panel faces,", len(opening), "facade faces")


LIGHT_COLORS = {
    "lamp": (1.0, 0.80, 0.48),
    "strip": (0.50, 0.70, 1.0),
}


def find_lights(props):
    """The lab's dark window panes and the blue strips over them, from the facade's colours: (kind, lo, hi) of
    each, lo and hi the corners of its rectangle on the facade."""
    image = next(i for i in bpy.data.images if i.name.startswith("props_color"))
    w, h = image.size
    pixels = image.pixels[:]
    bm = bmesh.new()
    bm.from_mesh(props.data)
    bm.transform(props.matrix_world)
    uvs = bm.loops.layers.uv.active
    found = {}
    for f in bm.faces:
        centre = f.calc_center_median()
        if f.normal.y > -0.99 or abs(centre.y - (FRONT_Y - 0.04)) > 0.02:
            continue
        u = sum(loop[uvs].uv.x for loop in f.loops) / len(f.loops)
        v = sum(loop[uvs].uv.y for loop in f.loops) / len(f.loops)
        i = ((int(v * h) % h) * w + int(u * w) % w) * 4
        r, g, b = pixels[i:i + 3]
        kind = "lamp" if max(r, g, b) < 0.25 else "strip" if b > 0.6 and r < 0.2 else None
        if kind:
            # each pane or strip is two triangles, keyed by the bay they're in
            found.setdefault((kind, round(centre.x)), []).extend(v.co.copy() for v in f.verts)
    bm.free()
    lights = []
    for (kind, _), corners in sorted(found.items()):
        lo = Vector((min(c.x for c in corners), min(c.y for c in corners), min(c.z for c in corners)))
        hi = Vector((max(c.x for c in corners), max(c.y for c in corners), max(c.z for c in corners)))
        lights.append((kind, lo, hi))
    print("RESULT lab lights", sum(k == "lamp" for k, _, _ in lights), "panes",
          sum(k == "strip" for k, _, _ in lights), "strips")
    return lights


def lights(out, found):
    """lights.glb: the lab at night, lamplight behind its window panes and the blue strips over them glowing
    like the machines inside; quads just in front of the facade the intro draws at full brightness, the panes
    with a soft halo ([r, g, b, size, mode], as the backdrop's windows)."""
    toy.reset()
    palette = toy.Palette("lights_palette", LIGHT_COLORS)
    b = toy.Builder("lights", palette)
    for i, (kind, lo, hi) in enumerate(found):
        y = lo.y - 0.01
        bm = bmesh.new()
        bm.faces.new([bm.verts.new(c) for c in ((lo.x, y, lo.z), (hi.x, y, lo.z), (hi.x, y, hi.z), (lo.x, y, hi.z))])
        b.add(bm, kind)
        if kind == "lamp":
            node = bpy.data.objects.new(f"glow.{i:03d}", None)
            node.location = ((lo.x + hi.x) / 2, y - 0.05, (lo.z + hi.z) / 2)
            node["glow"] = [1.0, 0.72, 0.38, 1.1, 0]
            bpy.context.scene.collection.objects.link(node)
    b.build_static()
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.export_scene.gltf(filepath=os.path.join(out, "lights.glb"), export_format="GLB", use_selection=True,
                              export_extras=True)
    print("RESULT exported lights.glb")


def block(builder, color, lo, hi):
    """A box from corner lo to corner hi."""
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    builder.add(bm, color, matrix=toy.trs(tuple((a + c) / 2 for a, c in zip(lo, hi)), (0, 0, 0),
                                          tuple(c - a for a, c in zip(lo, hi))))


def doorway():
    """Behind the opening a short hall in warm light, a frame round the opening."""
    palette = toy.Palette("doorway_palette", DOOR_COLORS)
    b = toy.Builder("doorway", palette)
    x0, x1 = DOOR_X - DOOR_WIDTH / 2, DOOR_X + DOOR_WIDTH / 2
    y0, y1 = FRONT_Y, FRONT_Y + DOOR_DEPTH
    z0, z1 = DOOR_BOTTOM, DOOR_TOP
    # the hall's walls, facing in
    for part, corners in (
            ("hall", ((x0, y1, z0), (x1, y1, z0), (x1, y1, z1), (x0, y1, z1))),
            ("hall", ((x0, y0, z0), (x0, y1, z0), (x0, y1, z1), (x0, y0, z1))),
            ("hall", ((x1, y1, z0), (x1, y0, z0), (x1, y0, z1), (x1, y1, z1))),
            ("ceiling", ((x0, y1, z1), (x1, y1, z1), (x1, y0, z1), (x0, y0, z1))),
            ("floor", ((x0, y0, z0), (x1, y0, z0), (x1, y1, z0), (x0, y1, z0)))):
        bm = bmesh.new()
        bm.faces.new([bm.verts.new(c) for c in corners])
        b.add(bm, part)
    trim = 0.06
    block(b, "frame", (x0 - trim, FRAME_Y, z0), (x0, y0 + 0.02, z1 + trim))
    block(b, "frame", (x1, FRAME_Y, z0), (x1 + trim, y0 + 0.02, z1 + trim))
    block(b, "frame", (x0 - trim, FRAME_Y, z1), (x1 + trim, y0 + 0.02, z1 + trim))
    return b.build_static()


def door(out):
    """door.glb, the door's leaf on its own so the intro can swing it open: hinged on the origin (DOOR_HINGE in
    the scenery), closed it runs along -x over the opening and its frame, a handle on either side."""
    toy.reset()
    palette = toy.Palette("door_palette", DOOR_COLORS)
    b = toy.Builder("door", palette)
    width = DOOR_WIDTH + 0.1
    half = DOOR_LEAF / 2
    block(b, "door", (-width, -half, 0.0), (0.0, half, DOOR_TOP - DOOR_BOTTOM + 0.03))
    for side in (-1, 1):
        y = side * (half + 0.015)
        block(b, "handle", (-width + 0.08, y - 0.015, 0.8), (-width + 0.2, y + 0.015, 0.86))
    b.build_static()
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.export_scene.gltf(filepath=os.path.join(out, "door.glb"), export_format="GLB", use_selection=True)
    print("RESULT exported door.glb")


def lab(out):
    toy.reset()
    bpy.ops.import_scene.gltf(filepath=SCENERY)
    cut_dome(bpy.data.objects["props"])
    cut_door(bpy.data.objects["props"])
    found = find_lights(bpy.data.objects["props"])
    doorway()
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.export_scene.gltf(filepath=os.path.join(out, "lab.glb"), export_format="GLB", use_selection=True,
                              export_extras=True)
    print("RESULT exported lab.glb")
    return found


def point(phi, theta, inset=0.0):
    """On the ellipsoid; phi from 0 (+x) round the front (-y) to pi (-x), theta from the top."""
    r = RADIUS - inset
    h = HEIGHT - inset
    return CENTRE + Vector((r * math.sin(theta) * math.cos(phi), -r * math.sin(theta) * math.sin(phi),
                            h * math.cos(theta)))


def normal_at(p):
    """The ellipsoid's outward normal at p."""
    d = p - CENTRE
    return Vector((d.x / RADIUS ** 2, d.y / RADIUS ** 2, d.z / HEIGHT ** 2)).normalized()


def dome_shell():
    """The half dome as a closed shell of glass."""
    bm = bmesh.new()
    # smooth shaded a coarse grid is round enough, the cells are cut by planes and keep their sharp edges
    columns, rows = 60, 22
    layers = []
    for inset in (0.0, THICKNESS):
        grid = []
        for i in range(columns + 1):
            phi = math.pi * i / columns
            grid.append([bm.verts.new(point(phi, (math.pi / 2) * j / rows, inset)) for j in range(rows + 1)])
        layers.append(grid)
    outer, inner = layers
    for i in range(columns):
        for j in range(rows):
            bm.faces.new((outer[i][j], outer[i + 1][j], outer[i + 1][j + 1], outer[i][j + 1]))
            bm.faces.new((inner[i][j + 1], inner[i + 1][j + 1], inner[i + 1][j], inner[i][j]))
        # along the ground
        bm.faces.new((outer[i][rows], outer[i + 1][rows], inner[i + 1][rows], inner[i][rows]))
    for j in range(rows):
        # the cut edges against the facade
        for i, flip in ((0, False), (columns, True)):
            quad = (outer[i][j], outer[i][j + 1], inner[i][j + 1], inner[i][j])
            bm.faces.new(tuple(reversed(quad)) if flip else quad)
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-5)
    # the poles' degenerate quads collapsed into triangles
    bmesh.ops.dissolve_degenerate(bm, edges=bm.edges, dist=1e-6)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    for f in bm.faces:
        f.smooth = True
    return bm


def band():
    """The blue strip round the dome, a ribbon just above the glass between two heights."""
    bm = bmesh.new()
    columns = 96
    rows = []
    for z in (0.36, 0.43):
        theta = math.acos(z)
        rows.append([bm.verts.new(point(math.pi * i / columns, theta, -0.012)) for i in range(columns + 1)])
    for i in range(columns):
        bm.faces.new((rows[0][i], rows[0][i + 1], rows[1][i + 1], rows[1][i])).smooth = True
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    return bm


IMPACT = point(math.pi / 2, 0.12)  # where the dragon hits the glass, just in front of the top
FLY_ANGLE = 0.46  # cells whose seed is within this angle of the impact, seen from the centre, fly


def angle_from_impact(p):
    return (p - CENTRE).angle(IMPACT - CENTRE)


def seeds():
    """Voronoi seeds on the dome, small cells round the impact, larger ones further down."""
    found = []
    for _ in range(3000):
        p = point(random.uniform(0.02, math.pi - 0.02), math.acos(1.0 - random.random() * 0.75))
        a = angle_from_impact(p)
        spacing = 0.3 + 0.95 * a
        if all((p - q).length > spacing for q in found):
            found.append(p)
    return sorted(found, key=angle_from_impact)


def loops(edges):
    """The edges grouped into the loops they form."""
    left, groups = set(edges), []
    while left:
        stack = [left.pop()]
        group = set(stack)
        while stack:
            for v in stack.pop().verts:
                for e in v.link_edges:
                    if e in left:
                        left.discard(e)
                        group.add(e)
                        stack.append(e)
        groups.append(group)
    return groups


def cell(shell, seed, others):
    """The part of the shell nearer to seed than to any other: cut by the planes halfway, the cuts closed.
    Returns the glass and the cut faces as two bmeshes, None if nothing is left."""
    bm = shell.copy()
    for other in others:
        normal = (other - seed).normalized()
        middle = (other + seed) / 2
        geom = bm.verts[:] + bm.edges[:] + bm.faces[:]
        bmesh.ops.bisect_plane(bm, geom=geom, dist=1e-6, plane_co=middle, plane_no=normal, clear_outer=True)
        if not bm.faces:
            bm.free()
            return None
    # the cuts leave vertices a hair apart where planes meet, welded they don't split the cell open later
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-4)
    bmesh.ops.dissolve_degenerate(bm, edges=bm.edges, dist=1e-4)
    before = set(bm.faces)
    # a cell inside the dome has two rims, round its outer face and its inner one: bridged, they become its
    # sides; a cell at the dome's foot or the facade has one rim running round both, filled
    boundary = [e for e in bm.edges if e.is_boundary]
    if len(loops(boundary)) == 2:
        bmesh.ops.bridge_loops(bm, edges=boundary)
    else:
        bmesh.ops.holes_fill(bm, edges=boundary, sides=0)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    cuts = [f for f in bm.faces if f not in before]
    bmesh.ops.triangulate(bm, faces=cuts)
    cuts = {f for f in bm.faces if f not in before}
    glass, edge = bmesh.new(), bmesh.new()
    for target, keep in ((glass, False), (edge, True)):
        mapping = {}
        for f in bm.faces:
            if (f in cuts) != keep:
                continue
            verts = []
            for v in f.verts:
                if v not in mapping:
                    mapping[v] = target.verts.new(v.co)
                verts.append(mapping[v])
            target.faces.new(verts).smooth = not keep
    bm.free()
    return glass, edge


def dome(out):
    toy.reset()
    palette = toy.Palette("dome_palette", COLORS)
    b = toy.Builder("dome", palette)
    b.bone("dome", CENTRE, CENTRE + Vector((0, 0, 1)))
    shell = dome_shell()
    points = seeds()
    pieces = []
    for seed in points:
        parts = cell(shell, seed, [q for q in points if q is not seed])
        if parts is None:
            continue
        glass, edge = parts
        bone = "dome"
        if angle_from_impact(seed) < FLY_ANGLE:
            centre = sum((v.co for v in glass.verts), Vector()) / max(len(glass.verts), 1)
            bone = f"cell{len(pieces)}"
            b.bone(bone, centre, centre + normal_at(centre) * 0.3, "dome")
            pieces.append((bone, centre, normal_at(centre), angle_from_impact(seed)))
        b.add(glass, "glass", bone)
        b.add(edge, "glass_edge", bone)
    shell.free()
    b.add(band(), "band", "dome")
    rig, mesh = b.build()
    burst(rig, pieces)
    toy.export(rig, mesh, os.path.join(out, "dome.glb"))
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(out, "dome.blend"))
    print("RESULT dome", len(points), "cells", len(pieces), "fly")


def burst(rig, pieces):
    """The cells round the impact pushed out along their normals, those nearest first and furthest, then they
    tumble down to the ground around the dome: 2.5 s at 24 fps."""
    keys = {}
    gravity = 12.0
    push = 0.22  # seconds the push out takes
    for name, centre, normal, angle in pieces:
        delay = angle * 0.18
        reach = 1.1 - angle * 0.9
        spin_axis = normal.cross(Vector((random.uniform(-1, 1), random.uniform(-1, 1), 0.3))).normalized()
        spin = random.uniform(300, 600)
        # off to the sides and to the front, clear of the dome, never back into the lab's wall
        away = Vector((centre.x - CENTRE.x, min(centre.y - CENTRE.y, 0.0) - 0.8, 0.0)).normalized()
        velocity = normal * reach / push * 0.4 + away * random.uniform(3.6, 4.6) + Vector((0, 0, random.uniform(1.0, 2.0)))
        track = []
        landed = None
        for frame in range(1, 62, 2):
            t = (frame - 1) / 24 - delay
            if t <= 0:
                track.append((frame, turn(), (0, 0, 0)))
                continue
            if t < push:
                # out along the normal, easing off
                u = t / push
                offset = normal * reach * (1 - (1 - u) ** 2)
                angle_now = spin * 0.1 * u
            else:
                # then falling freely
                f = t - push
                offset = normal * reach + velocity * f + Vector((0, 0, -0.5 * gravity * f * f))
                angle_now = spin * (0.1 + f)
            if centre.z + offset.z < 0.32:
                landed = landed or (offset.copy(), angle_now)
                offset = Vector((landed[0].x, landed[0].y, 0.32 - centre.z))
                angle_now = landed[1]
            track.append((frame, turn((spin_axis, angle_now)), tuple(offset)))
        keys[name] = track
    toy.action(rig, "burst", 61, keys, cyclic=False)


out = toy.out_dir()
lights(out, lab(out))
door(out)
dome(out)
