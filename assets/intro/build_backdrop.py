# The intro's last shot: the camera tilts up from the lab to the mountains behind it at night, a castle on the
# highest peak where the dragon flies off to. Flat shaded faces in night blues, the castle's windows glow.
#   blender --background --factory-startup --python build_backdrop.py -- <out dir>
# writes backdrop.glb in the scenery's coordinates (field coordinates, z up), north of the lab.
#
# The castle's peak ends in a crag: a plateau the castle fills, ringed by two bands of cliffs with a ledge between,
# then the mountain's slopes. The castle's walls and towers reach down into the rock, so the plateau's ragged edge
# never leaves them standing on air.
import math
import os
import random
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bmesh  # noqa: E402
import bpy  # noqa: E402
from mathutils import Vector, noise  # noqa: E402

import toy  # noqa: E402
from toy import cone, trs, tube  # noqa: E402

COLORS = {
    "rock_dark": (0.02, 0.025, 0.08),
    "rock": (0.04, 0.05, 0.15),
    "rock_light": (0.07, 0.09, 0.24),
    "rock_rim": (0.12, 0.15, 0.36),
    "wall": (0.09, 0.11, 0.28),
    "wall_light": (0.15, 0.18, 0.40),
    "roof": (0.05, 0.05, 0.16),
    "window": (1.0, 0.85, 0.35),
    "gate": (0.02, 0.02, 0.04),
}

PEAK = Vector((6.5, 100.0, 30.0))  # the middle of the castle's plateau
PLATEAU = 7.0  # its radius, give or take
SUMMITS = [  # x, y, height, width
    (PEAK.x, PEAK.y + 4.0, PEAK.z - 12.0, 30.0),
    (-30.0, 94.0, 21.0, 22.0),
    (44.0, 96.0, 23.0, 24.0),
    (-70.0, 110.0, 26.0, 28.0),
    (86.0, 112.0, 28.0, 30.0),
    (-8.0, 82.0, 12.0, 14.0),
    (24.0, 80.0, 10.0, 13.0),
]
X0, X1, Y0, Y1 = -120.0, 135.0, 64.0, 140.0
COARSE, FINE = 2.5, 0.5
FINE_REACH = 20.0  # how far round the peak the grid is fine

random.seed(3)


def smoothstep(a, b, x):
    t = min(max((x - a) / (b - a), 0.0), 1.0)
    return t * t * (3 - 2 * t)


def crag(x, y):
    """The castle's peak: the plateau, a band of cliffs, a ledge, more cliffs, then the slope down."""
    d = math.hypot(x - PEAK.x, y - PEAK.y)
    a = math.atan2(y - PEAK.y, x - PEAK.x)

    def wobble(seed, lobes):
        return noise.noise(Vector((math.cos(a) * lobes, math.sin(a) * lobes, seed)))

    # a ragged rim, never cutting into the castle; each band of cliffs wanders on its own
    r = d - 1.4 * wobble(0.5, 2.2)
    drop = 5.0 + 2.0 * wobble(4.1, 1.6)
    ledge = 2.4 + 1.6 * wobble(7.3, 2.6)
    lower = 6.0 + 3.0 * wobble(9.9, 1.3)
    if r < PLATEAU:
        return PEAK.z
    if r < PLATEAU + 1.6:
        return PEAK.z - drop * smoothstep(PLATEAU, PLATEAU + 1.6, r)
    if r < PLATEAU + 1.6 + ledge:
        return PEAK.z - drop - 0.5 * (r - PLATEAU - 1.6)
    top = PEAK.z - drop - 0.5 * ledge
    if r < PLATEAU + 3.6 + ledge:
        return top - lower * smoothstep(PLATEAU + 1.6 + ledge, PLATEAU + 3.6 + ledge, r)
    return top - lower - (r - PLATEAU - 3.6 - ledge) * 0.85


def height(x, y):
    h = 0.0
    for sx, sy, sh, sw in SUMMITS:
        d = math.hypot((x - sx) / sw, (y - sy) / (sw * 0.8))
        h = max(h, sh * max(0.0, 1.0 - d) ** 1.3)
    top = crag(x, y)
    rough = noise.noise(Vector((x * 0.12, y * 0.12, 0.0))) * 2.2 + noise.noise(Vector((x * 0.4, y * 0.4, 3.0))) * 0.7
    # the plateau stays level, the cliffs below it get rough
    on_crag = top > h
    rim = smoothstep(PLATEAU, PLATEAU + 1.0, math.hypot(x - PEAK.x, y - PEAK.y))
    if on_crag:
        return top + rough * 0.8 * rim
    # the far ones fade into the night below the horizon line
    return max(h + rough * min(h / 6.0, 1.0), -0.5 + rough * 0.2)


def axis(low, high, centre):
    """Grid lines from low to high, fine round centre."""
    lines = [low]
    while lines[-1] < high:
        fine = abs(lines[-1] - centre) < FINE_REACH
        lines.append(min(lines[-1] + (FINE if fine else COARSE), high))
    return lines


def mountains(builder):
    bm = bmesh.new()
    xs, ys = axis(X0, X1, PEAK.x), axis(Y0, Y1, PEAK.y)
    grid = [[bm.verts.new((x, y, height(x, y))) for y in ys] for x in xs]
    for i in range(len(xs) - 1):
        for j in range(len(ys) - 1):
            a, b, c, d = grid[i][j], grid[i + 1][j], grid[i + 1][j + 1], grid[i][j + 1]
            bm.faces.new((a, b, c))
            bm.faces.new((a, c, d))
    bm.normal_update()
    # flat faces coloured by which way they face the moon (high up behind the viewer, to the left)
    moon = Vector((-0.35, -0.6, 0.7)).normalized()
    shades = ["rock_dark", "rock", "rock_light", "rock_rim"]
    for key in shades:
        part = bmesh.new()
        mapping = {}
        for f in bm.faces:
            light = f.normal.dot(moon)
            shade = shades[min(max(int((light + 0.15) * 3.2), 0), 3)]
            if shade != key:
                continue
            verts = []
            for v in f.verts:
                if v not in mapping:
                    mapping[v] = part.verts.new(v.co)
                verts.append(mapping[v])
            part.faces.new(verts)
        builder.add(part, key)
    bm.free()
    return len(xs) * len(ys)


def box(size):
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    bm.transform(trs((0, 0, 0.5), (0, 0, 0), size))
    return bm


FOOTING = 4.0  # how far walls and towers reach down into the rock


def crenellate(builder, centre, along, length, z):
    """Merlons along a wall's top."""
    count = int(length / 0.9)
    for k in range(count):
        offset = (k - (count - 1) / 2) * length / count
        builder.add(box((0.42, 0.42, 0.42)), "wall_light", matrix=trs(centre + along * offset + Vector((0, 0, z))))


def tower(builder, windows, at, r, h, roof=True):
    builder.add(cone(r, r, h + FOOTING, 20), "wall", matrix=trs(at - Vector((0, 0, FOOTING))))
    if roof:
        builder.add(cone(r * 1.25, 0.0, r * 2.6, 20), "roof", matrix=trs(at + Vector((0, 0, h))))
        builder.add(tube([(0, 0, 0), (0, 0, 0.9)], [0.04, 0.02], 6), "wall_light",
                    matrix=trs(at + Vector((0, 0, h + r * 2.6))))
    else:
        # an open top behind merlons all round
        for k in range(10):
            a = 2 * math.pi * k / 10
            builder.add(box((0.32, 0.32, 0.45)), "wall_light",
                        matrix=trs(at + Vector((math.cos(a) * r * 0.9, math.sin(a) * r * 0.9, h)), (0, 0, math.degrees(a))))
    # windows on the side facing the lab, a little proud of the wall
    for k in range(1, int(h / 1.6) + 1):
        if random.random() < 0.6:
            spot = at + Vector((random.uniform(-0.3, 0.3) * r, -r - 0.02, k * 1.5))
            builder.add(box((0.22, 0.05, 0.38)), "window", matrix=trs(spot))
            windows.append(spot + Vector((0, -0.05, 0.19)))


def castle(builder, windows):
    base = PEAK.copy()
    half = 4.2
    # the curtain wall round the courtyard, down into the rock
    for centre, along in (((0, -half), (1, 0)), ((0, half), (1, 0)), ((-half, 0), (0, 1)), ((half, 0), (0, 1))):
        centre = base + Vector((*centre, 0))
        along = Vector((*along, 0))
        size = (2 * half, 0.7, 2.6 + FOOTING) if along.x else (0.7, 2 * half, 2.6 + FOOTING)
        builder.add(box(size), "wall", matrix=trs(centre - Vector((0, 0, FOOTING))))
        crenellate(builder, centre, along, 2 * half - 1.2, 2.6)
    # the gate towards the lab: a gatehouse with a dark arch
    gate = base + Vector((0, -half - 0.2, 0))
    builder.add(box((2.4, 1.2, 3.6 + FOOTING)), "wall", matrix=trs(gate - Vector((0, 0, FOOTING))))
    builder.add(box((1.0, 0.1, 1.5)), "gate", matrix=trs(gate + Vector((0, -0.62, 0))))
    builder.add(cone(0.5, 0.5, 0.1, 16), "gate", matrix=trs(gate + Vector((0, -0.62, 1.5)), (90, 0, 0)))
    crenellate(builder, gate + Vector((0, -0.4, 0)), Vector((1, 0, 0)), 2.0, 3.6)
    # towers at the corners, open topped at the front, roofed behind
    for x, y, h, roof in ((-half, -half, 4.4, False), (half, -half, 4.4, False), (-half, half, 5.2, True),
                          (half, half, 5.2, True)):
        tower(builder, windows, base + Vector((x, y, 0)), 1.0, h, roof)
    # the keep, broad and tall, between two slim spires
    keep = base + Vector((0, 1.2, 0))
    builder.add(box((4.2, 3.4, 7.0 + FOOTING)), "wall", matrix=trs(keep - Vector((0, 0, FOOTING))))
    crenellate(builder, keep + Vector((0, -1.7, 0)), Vector((1, 0, 0)), 3.6, 7.0)
    for k in range(-1, 2):
        spot = keep + Vector((k * 1.2, -1.72, 4.6))
        builder.add(box((0.35, 0.05, 0.7)), "window", matrix=trs(spot))
        windows.append(spot + Vector((0, -0.05, 0.35)))
    for s in (-1, 1):
        tower(builder, windows, keep + Vector((s * 2.3, 0.2, 0)), 0.75, 11.0)
    tower(builder, windows, keep + Vector((0, 0.6, 7.0)), 1.1, 3.2)


def build(out):
    toy.reset()
    palette = toy.Palette("backdrop_palette", COLORS)
    b = toy.Builder("backdrop", palette)
    vertices = mountains(b)
    windows = []
    castle(b, windows)
    mesh = b.build_static()
    # the windows' glow, read by the game from the nodes' extras: [r, g, b, size, mode], 1 flickers
    for i, spot in enumerate(windows):
        glow = bpy.data.objects.new(f"glow.{i:03d}", None)
        glow.location = spot
        glow["glow"] = [1.0, 0.75, 0.3, 0.7, 1]
        bpy.context.scene.collection.objects.link(glow)
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.export_scene.gltf(filepath=os.path.join(out, "backdrop.glb"), export_format="GLB", use_selection=True,
                              export_extras=True)
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(out, "backdrop.blend"))
    print("RESULT backdrop", tuple(round(d, 1) for d in mesh.dimensions), "terrain vertices", vertices,
          "windows", len(windows))


build(toy.out_dir())
