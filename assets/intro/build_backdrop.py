# The intro's last shot: the camera tilts up from the lab to the mountains behind it at night, a castle on the
# highest peak where the dragon flies off to. Night blue rock, smooth slopes and sharp cliffs, the castle's windows glow.
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
import numpy as np  # noqa: E402
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

    # a ragged rim, never cutting into the castle; each band of cliffs wanders on its own, and the noise over the
    # ground (not only round the peak) keeps the bands from reading as rings
    r = d - 1.4 * wobble(0.5, 2.2) - 1.6 * smoothstep(PLATEAU - 1.0, PLATEAU + 2.0, d) * noise.noise(
        Vector((x * 0.22, y * 0.22, 2.7)))
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


def furrows(x, y, cx, cy, seed=0.0):
    """Ridges and gullies running down from a summit at cx, cy, as rain and frost carve them: ridged noise
    stretched along the way down, so the slopes break into spurs instead of rising as round cones."""
    d = math.hypot(x - cx, y - cy)
    a = math.atan2(y - cy, x - cx)
    # the angle as a distance round the peak, so the spurs are about as wide near it as far down
    around = a * max(d, 4.0)
    warp = noise.noise(Vector((x * 0.05, y * 0.05, 6.1))) * 6.0
    ridged = 0.0
    weight = 1.0
    scale = 0.16
    for octave in range(3):
        n = noise.noise(Vector(((around + warp) * scale, d * scale * 0.25, 13.0 + seed + octave * 7.0)))
        ridged += (1.0 - abs(n) * 2.0) * weight
        weight *= 0.5
        scale *= 2.1
    return ridged / 1.75


def height(x, y):
    h = 0.0
    carved = 0.0
    for k, (sx, sy, sh, sw) in enumerate(SUMMITS):
        d = math.hypot((x - sx) / sw, (y - sy) / (sw * 0.8))
        summit = sh * max(0.0, 1.0 - d) ** 1.3
        if summit > h:
            h = summit
            # the gullies cut deeper on the steep upper slopes, and fade out at the foot
            carved = furrows(x, y, sx, sy, k * 3.1) * min(summit / 5.0, 1.0) * 1.6
    top = crag(x, y)
    rough = noise.noise(Vector((x * 0.12, y * 0.12, 0.0))) * 2.2 + noise.noise(Vector((x * 0.4, y * 0.4, 3.0))) * 0.7
    # the plateau stays level, the cliffs below it get rough
    on_crag = top > h
    d = math.hypot(x - PEAK.x, y - PEAK.y)
    rim = smoothstep(PLATEAU, PLATEAU + 1.0, d)
    # the spurs and gullies grow deeper away from the plateau
    carve = furrows(x, y, PEAK.x, PEAK.y) * 1.8 * smoothstep(PLATEAU + 1.0, PLATEAU + 6.0, d)
    if on_crag:
        return top + rough * 0.8 * rim + carve
    # the far ones fade into the night below the horizon line
    return max(h + carved + rough * min(h / 6.0, 1.0), -0.5 + rough * 0.2)


STEP = 0.6  # the terrain's grid before decimating
TRIANGLES = 12000  # what the decimation leaves, most of them where the shape changes: the crag and the ridges
TEXTURE = (2048, 1024)  # the terrain's colour, projected from above


def terrain():
    """The mountains as one mesh: a fine grid decimated to fewer, evenly shaped triangles that gather where the
    shape needs them, shaded smoothly but for the edges where the rock breaks; uvs straight from above."""
    xs = [X0 + i * STEP for i in range(int((X1 - X0) / STEP) + 1)]
    ys = [Y0 + j * STEP for j in range(int((Y1 - Y0) / STEP) + 1)]
    bm = bmesh.new()
    grid = [[bm.verts.new((x, y, height(x, y))) for y in ys] for x in xs]
    for i in range(len(xs) - 1):
        for j in range(len(ys) - 1):
            bm.faces.new((grid[i][j], grid[i + 1][j], grid[i + 1][j + 1], grid[i][j + 1]))
    mesh = bpy.data.meshes.new("terrain")
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new("terrain", mesh)
    bpy.context.scene.collection.objects.link(obj)
    decimate = obj.modifiers.new("decimate", "DECIMATE")
    decimate.ratio = TRIANGLES / (2 * len(mesh.polygons))
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.modifier_apply(modifier=decimate.name)
    uv = mesh.uv_layers.new(name="UVMap")
    for loop in mesh.loops:
        co = mesh.vertices[loop.vertex_index].co
        uv.data[loop.index].uv = ((co.x - X0) / (X1 - X0), (co.y - Y0) / (Y1 - Y0))
    # smooth over the slopes, hard where the rock breaks: cliff rims, ridges
    for poly in mesh.polygons:
        poly.use_smooth = True
    mesh.set_sharp_from_angle(angle=math.radians(40))
    mesh.materials.append(terrain_material())
    return obj, len(xs) * len(ys)


def terrain_material():
    """Night rock seen by the moon: the slopes facing it pale, the others deep blue, bands of strata in the cliffs,
    crevices darker, a little mottling; computed per texel from the heights."""
    w, h = TEXTURE
    x = X0 + (np.arange(w) + 0.5) / w * (X1 - X0)
    y = Y0 + (np.arange(h) + 0.5) / h * (Y1 - Y0)
    x, y = np.meshgrid(x, y)
    z = np.vectorize(height)(x, y)
    gx = np.gradient(z, (X1 - X0) / w, axis=1)
    gy = np.gradient(z, (Y1 - Y0) / h, axis=0)
    normal = np.stack([-gx, -gy, np.ones_like(z)], axis=-1)
    normal /= np.linalg.norm(normal, axis=-1, keepdims=True)
    moon = np.array([-0.35, -0.6, 0.7]) / np.linalg.norm([-0.35, -0.6, 0.7])
    light = np.clip(normal @ moon * 1.15 + 0.12, 0.0, 1.0)
    ramp = np.array([COLORS[k] for k in ("rock_dark", "rock", "rock_light", "rock_rim")])
    t = light * (len(ramp) - 1)
    k = np.clip(t.astype(int), 0, len(ramp) - 2)
    color = ramp[k] + (ramp[k + 1] - ramp[k]) * (t - k)[..., None]
    # strata across the steep faces
    steep = np.clip((1.0 - normal[..., 2] - 0.45) / 0.25, 0.0, 1.0)
    wobble = np.vectorize(lambda a, b: noise.noise(Vector((a * 0.3, b * 0.3, 5.0))))(x, y)
    # the rock's layers dip across the mountain rather than following its height (round a peak those would be
    # rings), and they break off here and there
    strata = 0.5 + 0.5 * np.sin((z + 0.22 * x - 0.14 * y + wobble * 2.0) * 2 * math.pi / 1.6)
    broken = np.clip(np.vectorize(lambda a, b: noise.noise(Vector((a * 0.15, b * 0.15, 21.0))))(x, y) * 2.0 + 0.5, 0, 1)
    color *= (1.0 - 0.12 * steep * broken * strata ** 3)[..., None]
    # crevices: lower than the heights around them
    blur = z.copy()
    for _ in range(6):
        blur = (np.roll(blur, 1, 0) + np.roll(blur, -1, 0) + np.roll(blur, 1, 1) + np.roll(blur, -1, 1) + blur * 4) / 8
    color *= np.clip(1.0 + (z - blur) * 0.5, 0.65, 1.15)[..., None]
    mottle = np.vectorize(lambda a, b: noise.noise(Vector((a * 1.7, b * 1.7, 9.0))))(x, y)
    color *= (1.0 + 0.08 * mottle)[..., None]
    # the slope (rise over run) per texel, for the detail masks backdrop_detail.py adds after the export
    np.save(os.path.join(toy.out_dir(), "terrain_slope.npy"),
            (np.sqrt(np.maximum(1.0 - normal[..., 2] ** 2, 0.0)) / normal[..., 2]).astype(np.float32))
    # a byte image keeps the srgb colours as they are, like the palette's
    rgba = np.concatenate([np.clip(color, 0, 1), np.ones((h, w, 1))], axis=-1).astype(np.float32)
    image = bpy.data.images.new("terrain_color", w, h)
    image.pixels.foreach_set(rgba.ravel())
    image.file_format = "PNG"
    image.pack()
    material = bpy.data.materials.new("terrain")
    material.use_nodes = True
    nodes, links = material.node_tree.nodes, material.node_tree.links
    bsdf = next(n for n in nodes if n.type == "BSDF_PRINCIPLED")
    texture = nodes.new("ShaderNodeTexImage")
    texture.image = image
    links.new(texture.outputs["Color"], bsdf.inputs["Base Color"])
    return material


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
    _, vertices = terrain()
    windows = []
    castle(b, windows)
    mesh = b.build_static()
    # which of the castle's palette swatches get which detail (rows bottom up like the palette): walls blocks,
    # roofs slates, windows and gate none; for backdrop_detail.py
    size = toy.CELL * toy.GRID
    mask = np.zeros((size, size, 4), dtype=np.float32)
    for key, layer in (("wall", 0), ("wall_light", 0), ("roof", 1)):
        row, col = divmod(palette.index[key], toy.GRID)
        mask[row * toy.CELL:(row + 1) * toy.CELL, col * toy.CELL:(col + 1) * toy.CELL, layer] = 1.0
    np.save(os.path.join(out, "castle_mask.npy"), mask)
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
