# The intro's last shot: the camera tilts up from the lab to the mountains behind it at night, a castle on the
# highest peak where the dragon flies off to. Flat shaded faces in night blues, the castle's windows glow.
#   blender --background --factory-startup --python build_backdrop.py -- <out dir>
# writes backdrop.glb in the scenery's coordinates (field coordinates, z up), north of the lab.
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
    "rock_dark": (0.03, 0.04, 0.12),
    "rock": (0.06, 0.08, 0.22),
    "rock_light": (0.11, 0.14, 0.34),
    "rock_rim": (0.19, 0.24, 0.50),
    "wall": (0.10, 0.13, 0.36),
    "wall_light": (0.17, 0.22, 0.50),
    "roof": (0.06, 0.07, 0.22),
    "window": (1.0, 0.85, 0.35),
}

PEAK = Vector((6.5, 100.0, 30.0))  # the castle's plateau
SUMMITS = [  # x, y, height, width
    (PEAK.x, PEAK.y, PEAK.z, 22.0),
    (-30.0, 94.0, 21.0, 22.0),
    (44.0, 96.0, 23.0, 24.0),
    (-70.0, 110.0, 26.0, 28.0),
    (86.0, 112.0, 28.0, 30.0),
    (-8.0, 82.0, 12.0, 14.0),
    (24.0, 80.0, 10.0, 13.0),
]
X0, X1, Y0, Y1 = -120.0, 135.0, 64.0, 140.0
STEP = 2.5

random.seed(3)


def height(x, y):
    h = 0.0
    for sx, sy, sh, sw in SUMMITS:
        d = math.hypot((x - sx) / sw, (y - sy) / (sw * 0.8))
        h = max(h, sh * max(0.0, 1.0 - d) ** 1.3)
    # the castle stands on a level top
    d = math.hypot(x - PEAK.x, y - PEAK.y)
    if d < 4.0:
        return PEAK.z
    rough = noise.noise(Vector((x * 0.12, y * 0.12, 0.0))) * 2.2 + noise.noise(Vector((x * 0.4, y * 0.4, 3.0))) * 0.7
    # the far ones fade into the night below the horizon line
    return max(h + rough * min(h / 6.0, 1.0), -0.5 + rough * 0.2)


def mountains(builder):
    bm = bmesh.new()
    nx, ny = int((X1 - X0) / STEP), int((Y1 - Y0) / STEP)
    grid = [[bm.verts.new((X0 + i * STEP, Y0 + j * STEP, height(X0 + i * STEP, Y0 + j * STEP)))
             for j in range(ny + 1)] for i in range(nx + 1)]
    for i in range(nx):
        for j in range(ny):
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


def box(size):
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    bm.transform(trs((0, 0, 0.5), (0, 0, 0), size))
    return bm


def castle(builder, windows):
    base = PEAK.copy()
    # curtain wall round the courtyard
    for (x, y), (w, d) in (((0, -3.2), (7.0, 0.6)), ((0, 3.2), (7.0, 0.6)), ((-3.2, 0), (0.6, 7.0)),
                           ((3.2, 0), (0.6, 7.0))):
        builder.add(box((w, d, 2.2)), "wall", matrix=trs(base + Vector((x, y, 0))))
        for k in range(-3, 4):
            # battlements
            along = Vector((k, 0, 0)) if w > d else Vector((0, k, 0))
            builder.add(box((0.45, 0.45, 0.4)), "wall_light", matrix=trs(base + Vector((x, y, 2.2)) + along))
    # towers at the corners and the keep with spires
    # a broad keep between two tall spires, a gatehouse in front, lower towers at the corners
    towers = [((-3.2, -3.2), 0.85, 3.6), ((3.2, -3.2), 0.85, 3.6), ((-3.2, 3.2), 0.85, 4.2), ((3.2, 3.2), 0.85, 4.2),
              ((0.0, -3.2), 0.75, 3.0), ((0.0, 1.0), 2.0, 6.0), ((-2.4, 1.2), 0.7, 10.5), ((2.4, 1.2), 0.7, 10.5)]
    for (x, y), r, h in towers:
        at = base + Vector((x, y, 0))
        builder.add(cone(r, r, h, 16), "wall", matrix=trs(at))
        builder.add(cone(r * 1.25, 0.0, r * 2.4, 16), "roof", matrix=trs(at + Vector((0, 0, h))))
        builder.add(tube([(0, 0, 0), (0, 0, 0.9)], [0.04, 0.02], 6), "wall_light",
                    matrix=trs(at + Vector((0, 0, h + r * 2.4))))
        # windows on the side facing the lab, a little proud of the wall
        for k in range(1, int(h / 1.6) + 1):
            if random.random() < 0.6:
                spot = at + Vector((random.uniform(-0.3, 0.3) * r, -r - 0.02, k * 1.5))
                builder.add(box((0.22, 0.05, 0.38)), "window", matrix=trs(spot))
                windows.append(spot + Vector((0, -0.05, 0.19)))


def build(out):
    toy.reset()
    palette = toy.Palette("backdrop_palette", COLORS)
    b = toy.Builder("backdrop", palette)
    mountains(b)
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
    print("RESULT backdrop", tuple(round(d, 1) for d in mesh.dimensions), "faces", len(mesh.data.polygons),
          "windows", len(windows))


build(toy.out_dir())
