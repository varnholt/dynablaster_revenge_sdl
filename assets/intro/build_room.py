# The ending's set after the original's: the castle's machine room seen from the side. A green brick wall with a
# control panel (gauges, buttons, screens), pipes running along it and down, two arched windows onto the night sky,
# two white plates hanging on cables, two steel pillars in front; a pale walkway along it, its front built of
# purple stone blocks; a doorway at the right end where the princess comes in.
#   blender --background --factory-startup --python build_room.py -- <out dir>
# writes room.glb (and room_mask.npy for room_detail.py: bricks on the wall, blocks on the front). Faces -y, z up,
# the walkway's floor at z 0 from x -7.5 to 7.5.
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
    "wall": (0.36, 0.47, 0.40),
    "panel": (0.78, 0.79, 0.82),
    "panel_dark": (0.52, 0.53, 0.58),
    "gauge": (0.66, 0.64, 0.78),
    "needle": (0.30, 0.15, 0.36),
    "knob": (0.12, 0.14, 0.36),
    "screen": (0.08, 0.10, 0.18),
    "glyph": (0.92, 0.93, 0.96),
    "pipe": (0.86, 0.87, 0.89),
    "steel": (0.70, 0.71, 0.75),
    "frame": (0.84, 0.85, 0.88),
    "plate": (0.93, 0.94, 0.96),
    "floor": (0.80, 0.81, 0.84),
    "floor_edge": (0.58, 0.58, 0.64),
    "blocks_top": (0.58, 0.57, 0.72),
    "blocks": (0.40, 0.37, 0.58),
    "moss": (0.33, 0.48, 0.30),
}

WALL_Y = 1.6  # the wall's face
FRONT_Y = -0.7  # the walkway's front edge
X0, X1 = -7.5, 7.5
TOP = 6.6
WINDOWS = [(-3.25, 2.75), (2.3, 2.75)]  # bottom centre of each arched opening
WINDOW_WIDTH = 1.3
WINDOW_HEIGHT = 1.0  # up to where the arch begins


def box(lo, hi):
    """A box from corner lo to corner hi."""
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    bm.transform(trs(tuple((a + b) / 2 for a, b in zip(lo, hi)), (0, 0, 0), tuple(b - a for a, b in zip(lo, hi))))
    return bm


def facing(radius, depth, segments=32):
    """A disc depth thick facing -y, its back on the origin."""
    return cone(radius, radius, depth, segments), (-90, 0, 0)


def in_window(x, z):
    for cx, bottom in WINDOWS:
        half = WINDOW_WIDTH / 2
        if abs(x - cx) < half and bottom < z < bottom + WINDOW_HEIGHT:
            return True
        if math.hypot(x - cx, z - bottom - WINDOW_HEIGHT) < half:
            return True
    return False


def wall(b):
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
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    for f in bm.faces:
        if f.normal.y > 0:
            f.normal_flip()
    b.add(bm, "wall")
    # a little moss along its foot
    for x in np.arange(X0 + 0.4, X1, 0.9):
        b.add(sphere(1.0, 10, 6), "moss", matrix=trs((x, WALL_Y - 0.02, 0.0), (0, 0, 0), (0.25, 0.05, 0.08)))


def windows(b):
    """Frames round the arched openings, a sill below."""
    for cx, bottom in WINDOWS:
        half = WINDOW_WIDTH / 2
        top = bottom + WINDOW_HEIGHT
        arch = [(cx + half * math.cos(a), WALL_Y - 0.05, top + half * math.sin(a))
                for a in np.linspace(0, math.pi, 13)]
        outline = [(cx + half, WALL_Y - 0.05, bottom)] + arch + [(cx - half, WALL_Y - 0.05, bottom)]
        b.add(tube([Vector(p) for p in outline], [0.07] * len(outline), 8), "frame")
        b.add(box((cx - half - 0.12, WALL_Y - 0.2, bottom - 0.12), (cx + half + 0.12, WALL_Y, bottom)), "frame")
        # the night beyond: a dark sill inside the opening's depth
        b.add(box((cx - half, WALL_Y, bottom - 0.02), (cx + half, WALL_Y + 0.3, bottom)), "screen")


def panel(b):
    """The control panel at the left: an upper part with gauges and buttons, a wider lower one with three screens
    and two slots."""
    y0, y1 = WALL_Y - 0.14, WALL_Y
    b.add(box((-6.75, y0, 2.2), (-4.35, y1, 3.75)), "panel")
    b.add(box((-6.75, y0, 0.6), (-2.05, y1, 2.2)), "panel")
    b.add(box((-6.85, y0 + 0.02, 0.5), (-1.95, y1, 0.6)), "panel_dark")
    face = y0 - 0.001
    for z in (3.15, 2.25):
        disc, turned = facing(0.42, 0.04)
        b.add(disc, "gauge", matrix=trs((-5.8, face, z), turned))
        rim, turned = facing(0.46, 0.02)
        b.add(rim, "panel_dark", matrix=trs((-5.8, face + 0.01, z), turned))
        b.add(box((-5.82, face - 0.06, z - 0.02), (-5.5, face - 0.04, z + 0.02)), "needle")
        for k in range(3):
            b.add(box((-5.05 + k * 0.32, face - 0.06, z + 0.1), (-4.85 + k * 0.32, face, z + 0.3)), "glyph")
        b.add(box((-5.05, face - 0.04, z - 0.18), (-4.25, face, z - 0.12)), "needle")
    for z in (3.35, 3.0):
        knob, turned = facing(0.1, 0.06, 12)
        b.add(knob, "knob", matrix=trs((-6.55, face, z), turned))
    for k in range(3):
        x = -6.4 + k * 0.72
        b.add(box((x, face - 0.04, 0.8), (x + 0.6, face, 1.45)), "screen")
        for z0, z1 in ((0.95, 1.0), (1.1, 1.15), (1.25, 1.3)):
            b.add(box((x + 0.2, face - 0.06, z0), (x + 0.42, face - 0.04, z1)), "glyph")
        b.add(box((x + 0.18, face - 0.06, 0.95), (x + 0.23, face - 0.04, 1.3)), "glyph")
    for z in (1.55, 1.05):
        b.add(box((-4.05, face - 0.04, z), (-2.55, face, z + 0.18)), "panel_dark")
        b.add(box((-3.95, face - 0.06, z + 0.06), (-2.65, face - 0.04, z + 0.12)), "screen")


def pipes(b):
    """Two pipes along the wall's top, down to the floor beside the panel."""
    for z, x in ((5.15, -1.45), (4.85, -1.8)):
        path = [Vector((X0, WALL_Y - 0.12, z)), Vector((x - 0.15, WALL_Y - 0.12, z)), Vector((x, WALL_Y - 0.12, z - 0.15)),
                Vector((x, WALL_Y - 0.12, 0.05))]
        b.add(tube(path, [0.06] * len(path), 10), "pipe")
        for px in np.arange(X0 + 0.6, x - 0.2, 1.4):
            b.add(box((px - 0.04, WALL_Y - 0.2, z - 0.09), (px + 0.04, WALL_Y, z + 0.09)), "panel_dark")
    b.add(box((-2.0, WALL_Y - 0.25, 0.0), (-1.25, WALL_Y, 0.18)), "panel_dark")


def pillars(b):
    for x in (-0.55, 4.6):
        b.add(cone(0.62, 0.62, TOP, 32), "steel", matrix=trs((x, WALL_Y - 0.75, 0.0)))
        b.add(cone(0.7, 0.7, 0.12, 32), "panel_dark", matrix=trs((x, WALL_Y - 0.75, 0.0)))


def plates(b):
    """Two white plates on the wall, a cable hanging from each with a weight at its end."""
    for x in (1.4, 2.65):
        b.add(box((x - 0.4, WALL_Y - 0.08, 1.3), (x + 0.4, WALL_Y, 2.55)), "plate")
        for bx in (x - 0.3, x + 0.3):
            for bz in (1.4, 2.45):
                b.add(sphere(1.0, 8, 6), "panel_dark", matrix=trs((bx, WALL_Y - 0.09, bz), (0, 0, 0), 0.03))
        b.add(tube([Vector((x, WALL_Y - 0.1, 1.3)), Vector((x, WALL_Y - 0.1, 0.3))], [0.02, 0.02], 6), "plate")
        b.add(cone(0.06, 0.0, 0.12, 8), "knob", matrix=trs((x, WALL_Y - 0.1, 0.32), (180, 0, 0)))


def doorway(b):
    """The princess's way in at the right end: a tall frame in the wall."""
    b.add(box((6.2, WALL_Y - 0.12, 0.0), (6.35, WALL_Y, 6.0)), "frame")
    b.add(box((6.35, WALL_Y - 0.04, 0.0), (X1, WALL_Y, 6.0)), "panel_dark")
    b.add(box((6.2, WALL_Y - 0.12, 5.85), (X1, WALL_Y, 6.0)), "frame")


def walkway(b):
    """The floor along the wall, a darker strip at its foot, and the front of purple stone blocks below it."""
    b.add(box((X0, FRONT_Y, -0.1), (X1, WALL_Y, 0.0)), "floor")
    b.add(box((X0, WALL_Y - 0.35, 0.0), (X1, WALL_Y, 0.02)), "floor_edge")
    b.add(box((X0, FRONT_Y - 0.08, -0.45), (X1, FRONT_Y, 0.0)), "blocks_top")
    b.add(box((X0, FRONT_Y - 0.04, -2.6), (X1, FRONT_Y, -0.45)), "blocks")


def build(out):
    toy.reset()
    palette = toy.Palette("room_palette", COLORS)
    b = toy.Builder("room", palette)
    wall(b)
    windows(b)
    panel(b)
    pipes(b)
    pillars(b)
    plates(b)
    doorway(b)
    walkway(b)
    mesh = b.build_static()
    # which palette swatches get which detail (rows bottom up like the palette): bricks on the wall, big blocks on
    # the front, brushed steel on the pillars; for room_detail.py
    size = toy.CELL * toy.GRID
    mask = np.zeros((size, size, 4), dtype=np.float32)
    for key, layer in (("wall", 0), ("blocks", 1), ("blocks_top", 1), ("steel", 2), ("floor", 3)):
        row, col = divmod(palette.index[key], toy.GRID)
        mask[row * toy.CELL:(row + 1) * toy.CELL, col * toy.CELL:(col + 1) * toy.CELL, layer] = 1.0
    np.save(os.path.join(out, "room_mask.npy"), mask)
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.export_scene.gltf(filepath=os.path.join(out, "room.glb"), export_format="GLB", use_selection=True)
    print("RESULT room", tuple(round(d, 1) for d in mesh.dimensions), "faces", len(mesh.data.polygons))


build(toy.out_dir())
