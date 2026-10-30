# The intro's set: Dr's lab from the first world's scenery, its dome rebuilt so the dragon can break out
#   blender --background --factory-startup --python build_lab.py -- <out dir>
# writes
#   lab.glb          world1/surroundings_13x11.glb with the dome cut out of its lab mesh and the rotunda's doorway open
#   door.glb         that doorway's door, hinged on its origin
#   lights.glb       the lab's lit windows at night
#   dome.glb         the dome in its pieces, the geodesic plates, the shutter and its ladder, whole in its rest
#                    pose; "burst" blows the pieces round the top out along their normals, then they tumble down
#   lab.json         where the door, its hinge, its steps and the dome are and the ground's heights in front of
#                    the lab, for build_intro.py
# all in the scenery's own coordinates (field coordinates, z up). The scenery's lab (lab() in
# assets/story-worlds/build_surroundings.py) marks its dome, its door and its windows with empties, read here.
import json
import math
import os
import random
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, "..", "story-worlds"))
import bmesh  # noqa: E402
import bpy  # noqa: E402
import numpy as np  # noqa: E402
from mathutils import Vector  # noqa: E402
from mathutils.bvhtree import BVHTree  # noqa: E402

import build_surroundings as surroundings  # noqa: E402
import palettes  # noqa: E402
import toy  # noqa: E402
from toy import turn  # noqa: E402

SCENERY = os.path.join(HERE, "..", "..", "client", "data", "game", "story", "world1", "surroundings_13x11.glb")


def shade(rgb):
    """A colour as world 1's scenery has it."""
    return palettes.snap(1, rgb)


# the scenery's (lab() in build_surroundings.py)
COLORS = {
    "plate": shade((0.66, 0.72, 0.78)),
    "plate2": shade((0.61, 0.67, 0.74)),
    "shutter": shade((0.28, 0.3, 0.33)),
    "iron": shade((0.2, 0.21, 0.23)),
}
DOOR_COLORS = {
    "hall": (0.78, 0.70, 0.52),
    "ceiling": (0.90, 0.84, 0.66),
    "floor": (0.42, 0.42, 0.44),
    "frame": shade((0.94, 0.91, 0.83)),
    "door": shade((0.62, 0.33, 0.22)),
    "handle": (0.80, 0.82, 0.86),
}
DOOR_DEPTH = 1.0  # the hall behind the door, within the rotunda's portal
DOOR_LEAF = 0.04

random.seed(5)


class Dome:
    """The scenery's dome, from its empty "lab_dome"."""

    def __init__(self, values):
        self.centre = Vector(values[:3])
        self.radius, self.lift, self.slit_turn, self.s = values[3:7]
        # the ring it turns on: its top and its outer radius (lab() in build_surroundings.py)
        self.ring_top = self.centre.z - self.lift + 0.03 * self.s
        self.ring = self.radius / 0.93 * 1.04


class Door:
    """The rotunda's door, from its empty "lab_door"."""

    def __init__(self, values):
        self.x, self.face, self.bottom, self.top, self.width, self.ground, self.steps = values
        self.left, self.right = self.x - self.width / 2, self.x + self.width / 2
        # the leaf's hinge: at the opening's right edge, the leaf within the opening, opening outwards
        self.hinge = Vector((self.right, self.face - DOOR_LEAF / 2 - 0.005, self.bottom + 0.005))


def markers():
    """The scenery's empties: the dome, the door and the windows' panes ((centre, turn, width, height) each)."""
    dome = door = None
    panes = []
    for obj in bpy.data.objects:
        if "lab_dome" in obj:
            dome = Dome(list(obj["lab_dome"]))
        elif "lab_door" in obj:
            door = Door(list(obj["lab_door"]))
        elif "lab_pane" in obj:
            width, height = obj["lab_pane"]
            panes.append((obj.matrix_world.translation.copy(), obj.matrix_world.to_euler().z, width, height))
    print("RESULT lab markers: dome", dome is not None, "door", door is not None, "panes", len(panes))
    return dome, door, panes


def cut_dome(props, dome):
    """Removes the dome's pieces from the scenery's props: the faces near its shell reaching above the ring."""
    bm = bmesh.new()
    bm.from_mesh(props.data)
    world = props.matrix_world

    def near(p):
        return 0.95 < (p - dome.centre).length / dome.radius < 1.1

    doomed = []
    for f in bm.faces:
        points = [world @ v.co for v in f.verts]
        if all(near(p) for p in points) and any(p.z > dome.ring_top + 0.002 for p in points):
            doomed.append(f)
    bmesh.ops.delete(bm, geom=doomed, context="FACES")
    bm.to_mesh(props.data)
    bm.free()
    print("RESULT removed", len(doomed), "dome faces")


def cut_door(props, door):
    """Opens the rotunda's doorway: everything within the opening, from just in front of the portal's face to the
    hall's back, cut along the opening's sides and top and taken out (the closed door, the portal's face, the
    rotunda's wall behind it)."""
    bm = bmesh.new()
    bm.from_mesh(props.data)
    bm.transform(props.matrix_world)
    y0, y1 = door.face - 0.15, door.face + DOOR_DEPTH

    def around(f):
        return any(door.left - 0.5 < v.co.x < door.right + 0.5 and y0 - 0.2 < v.co.y < y1 + 0.2
                   and door.bottom - 0.1 < v.co.z < door.top + 0.5 for v in f.verts)

    for co, no in (((door.left, 0, 0), (1, 0, 0)), ((door.right, 0, 0), (1, 0, 0)), ((0, 0, door.top), (0, 0, 1))):
        faces = [f for f in bm.faces if around(f)]
        geom = list({v for f in faces for v in f.verts}) + list({e for f in faces for e in f.edges}) + faces
        bmesh.ops.bisect_plane(bm, geom=geom, dist=1e-5, plane_co=co, plane_no=no)

    def inside(f):
        c = f.calc_center_median()
        return (door.left + 1e-3 < c.x < door.right - 1e-3 and y0 < c.y < y1
                and door.bottom + 0.005 < c.z < door.top - 1e-3)

    opening = [f for f in bm.faces if inside(f)]
    bmesh.ops.delete(bm, geom=opening, context="FACES")
    bm.transform(props.matrix_world.inverted())
    bm.to_mesh(props.data)
    bm.free()
    print("RESULT door: removed", len(opening), "faces")


def block(builder, color, lo, hi, bone=None):
    """A box from corner lo to corner hi."""
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    builder.add(bm, color, bone, matrix=toy.trs(tuple((a + c) / 2 for a, c in zip(lo, hi)), (0, 0, 0),
                                                tuple(c - a for a, c in zip(lo, hi))))


def doorway(door):
    """Behind the opening a short hall in warm light, a frame round the opening."""
    palette = toy.Palette("doorway_palette", DOOR_COLORS)
    b = toy.Builder("doorway", palette)
    x0, x1 = door.left, door.right
    y0, y1 = door.face, door.face + DOOR_DEPTH
    # its floor just over the rotunda's floor, which runs on under it
    z0, z1 = door.bottom + 0.01, door.top
    for part, corners in (
            ("hall", ((x0, y1, z0), (x1, y1, z0), (x1, y1, z1), (x0, y1, z1))),
            ("hall", ((x0, y0, z0), (x0, y1, z0), (x0, y1, z1), (x0, y0, z1))),
            ("hall", ((x1, y1, z0), (x1, y0, z0), (x1, y0, z1), (x1, y1, z1))),
            ("ceiling", ((x0, y1, z1), (x1, y1, z1), (x1, y0, z1), (x0, y0, z1))),
            ("floor", ((x0, y0, z0), (x1, y0, z0), (x1, y1, z0), (x0, y1, z0)))):
        bm = bmesh.new()
        bm.faces.new([bm.verts.new(c) for c in corners])
        b.add(bm, part)
    return b.build_static()


def door_leaf(out, door):
    """door.glb, the door's leaf on its own so the intro can swing it open: hinged on the origin (door.hinge in the
    scenery), closed it runs along -x across the opening, panels and a handle on either side."""
    toy.reset()
    palette = toy.Palette("door_palette", DOOR_COLORS)
    b = toy.Builder("door", palette)
    width = door.width - 0.01
    height = door.top - door.bottom - 0.01
    half = DOOR_LEAF / 2
    block(b, "door", (-width, -half, 0.0), (0.0, half, height))
    for side in (-1, 1):
        y = side * (half + 0.01)
        for z0, z1 in ((0.08, 0.44), (0.52, 0.92)):
            block(b, "door", (-width + 0.08, y - 0.01, z0 * height), (-0.08, y + 0.01, z1 * height))
        block(b, "handle", (-width + 0.08, y - 0.02, 0.48 * height), (-width + 0.2, y + 0.02, 0.51 * height))
    b.build_static()
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.export_scene.gltf(filepath=os.path.join(out, "door.glb"), export_format="GLB", use_selection=True)
    print("RESULT exported door.glb")


LIGHT_COLORS = {
    "lamp": (1.0, 0.80, 0.48),
}


def lights(out, panes):
    """lights.glb: the lab at night, lamplight behind its window panes; quads just in front of them the intro
    draws at full brightness, the bigger ones with a soft halo ([r, g, b, size, mode], as the backdrop's windows)."""
    toy.reset()
    palette = toy.Palette("lights_palette", LIGHT_COLORS)
    b = toy.Builder("lights", palette)
    for i, (centre, angle, width, height) in enumerate(panes):
        along = Vector((math.cos(angle), math.sin(angle), 0.0))
        out_dir = Vector((math.sin(angle), -math.cos(angle), 0.0))
        c = centre + out_dir * 0.005
        bm = bmesh.new()
        corners = [c + along * (u * width / 2) + Vector((0, 0, v * height / 2)) for u, v in
                   ((-1, -1), (1, -1), (1, 1), (-1, 1))]
        face = bm.faces.new([bm.verts.new(p) for p in corners])
        if face.normal.dot(out_dir) < 0:
            face.normal_flip()
        b.add(bm, "lamp")
        if height > 0.4:
            node = bpy.data.objects.new(f"glow.{i:03d}", None)
            node.location = c + out_dir * 0.05
            node["glow"] = [1.0, 0.72, 0.38, 1.1, 0]
            bpy.context.scene.collection.objects.link(node)
    b.build_static()
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.export_scene.gltf(filepath=os.path.join(out, "lights.glb"), export_format="GLB", use_selection=True,
                              export_extras=True)
    print("RESULT exported lights.glb", len(panes), "panes")


def lab(out):
    toy.reset()
    bpy.ops.import_scene.gltf(filepath=SCENERY)
    dome, door, panes = markers()
    cut_dome(bpy.data.objects["lab"], dome)
    cut_door(bpy.data.objects["lab"], door)
    doorway(door)
    heights = ground_grid()
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.export_scene.gltf(filepath=os.path.join(out, "lab.glb"), export_format="GLB", use_selection=True,
                              export_extras=True)
    print("RESULT exported lab.glb")
    return dome, door, panes, heights


def piece_mesh(verts, faces):
    bm = bmesh.new()
    vs = [bm.verts.new(v) for v in verts]
    for f in faces:
        try:
            bm.faces.new([vs[i] for i in f])
        except ValueError:
            pass
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    return bm


def dome_model(out, dome, door):
    """dome.glb: the dome's pieces as the scenery builds them, the plates and the lengths of shutter and ladder
    round the impact each on a bone of its own."""
    toy.reset()
    palette = toy.Palette("dome_palette", COLORS)
    b = toy.Builder("dome", palette)
    b.bone("dome", dome.centre, dome.centre + Vector((0, 0, 1)))
    impact = dome.centre + Vector((0.0, -math.sin(0.12), math.cos(0.12))) * dome.radius
    pieces = []
    for kind, mid, verts, faces in surroundings.lab_dome_pieces(dome.centre, dome.radius, dome.lift, dome.slit_turn,
                                                                dome.s, np.random.default_rng(3)):
        bone = "dome"
        angle = (mid - dome.centre).angle(impact - dome.centre)
        if angle < FLY_ANGLE:
            bone = f"piece{len(pieces)}"
            normal = (mid - dome.centre).normalized()
            b.bone(bone, mid, mid + normal * 0.3, "dome")
            pieces.append((bone, mid, normal, angle))
        b.add(piece_mesh(verts, faces), kind, bone)
    rig, mesh = b.build()
    burst(rig, pieces, dome, door)
    toy.export(rig, mesh, os.path.join(out, "dome.glb"))
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(out, "dome.blend"))
    print("RESULT dome", len(pieces), "pieces fly")


FLY_ANGLE = 0.5  # pieces within this angle of the impact, seen from the centre, fly


def ground_at(p, dome, door):
    """Where a falling piece comes to rest at p: on the ring round the dome, on a wing's roof or on the ground."""
    s = dome.s
    x, y = p.x - dome.centre.x, p.y - dome.centre.y
    if math.hypot(x, y) < dome.ring:
        return dome.ring_top
    # the wings (lab_wings() in build_surroundings.py): x 0.5 s..4.6 s out, their roofs sloping from the ridge
    wy = 0.35 * s
    reach = 1.22 * s
    if 0.5 * s < abs(x) < 4.6 * s and abs(y - wy) < reach:
        eave, ridge = door.ground + 1.18 * s, door.ground + 1.55 * s
        return ridge - (ridge - eave) * abs(y - wy) / reach
    return door.ground + 0.02


def burst(rig, pieces, dome, door):
    """The pieces round the impact pushed out along their normals, those nearest first and furthest, then they
    tumble down onto the ring, the wings' roofs and the ground: 2.5 s at 24 fps."""
    keys = {}
    gravity = 12.0
    push = 0.22  # seconds the push out takes
    for name, centre, normal, angle in pieces:
        delay = angle * 0.18
        reach = 1.2 - angle * 0.9
        spin_axis = normal.cross(Vector((random.uniform(-1, 1), random.uniform(-1, 1), 0.3))).normalized()
        spin = random.uniform(300, 600)
        # off to the sides and to the front, clear of the dome
        away = Vector((centre.x - dome.centre.x, min(centre.y - dome.centre.y, 0.0) - 0.8, 0.0)).normalized()
        velocity = normal * reach / push * 0.4 + away * random.uniform(4.2, 5.4) + Vector((0, 0, random.uniform(1.0, 2.0)))
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
            floor = ground_at(centre + offset, dome, door)
            if centre.z + offset.z < floor and (landed or offset.z < 0):
                landed = landed or (offset.copy(), angle_now)
                offset = Vector((landed[0].x, landed[0].y, floor - centre.z))
                angle_now = landed[1]
            track.append((frame, turn((spin_axis, angle_now)), tuple(offset)))
        keys[name] = track
    toy.action(rig, "burst", 61, keys, cyclic=False)


# where the intro's actors walk, sampled from the scenery's ground for build_intro.py: x0, y0, step, columns, rows
GROUND_GRID = (-6.0, -4.0, 0.25, 105, 33)


def ground_grid():
    """The scenery's ground heights over GROUND_GRID, row by row from y0."""
    ground = bpy.data.objects["ground"]
    bvh = BVHTree.FromObject(ground, bpy.context.evaluated_depsgraph_get())
    local = ground.matrix_world.inverted()
    x0, y0, step, nx, ny = GROUND_GRID
    heights = []
    for j in range(ny):
        for i in range(nx):
            origin = local @ Vector((x0 + i * step, y0 + j * step, 50.0))
            hit, _, _, _ = bvh.ray_cast(origin, (local.to_3x3() @ Vector((0, 0, -1))).normalized())
            heights.append(round((ground.matrix_world @ hit).z, 4) if hit else 0.0)
    return heights


def staging(out, dome, door, heights):
    """lab.json: what build_intro.py stages round."""
    x0, y0, step, nx, ny = GROUND_GRID
    data = {
        "ground": {"x0": x0, "y0": y0, "step": step, "columns": nx, "rows": ny, "heights": heights},
        "dome_centre": list(dome.centre), "dome_radius": dome.radius,
        "dome_top": dome.centre.z + dome.radius,
        "door": {"x": door.x, "face": door.face, "bottom": door.bottom, "top": door.top, "ground": door.ground,
                 "steps": door.steps, "hinge": list(door.hinge)},
    }
    with open(os.path.join(out, "lab.json"), "w", encoding="utf-8") as f:
        json.dump(data, f, indent=1)
    print("RESULT lab.json", json.dumps({k: v for k, v in data.items() if k != "ground"}))


out = toy.out_dir()
dome, door, panes, heights = lab(out)
lights(out, panes)
door_leaf(out, door)
dome_model(out, dome, door)
staging(out, dome, door, heights)
