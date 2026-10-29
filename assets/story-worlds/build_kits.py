# Story mode stage kits, one per world of the original game: floor tile, border block and the land around
# the field. Run headless: blender --background --factory-startup --python build_kits.py -- <out dir> [world ...]
# Pieces are one tile wide (1 unit), the floor's top is at z = 0, border blocks face -y towards the field.
import math
import os
import sys

import bmesh
import bpy
import numpy as np

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
import palettes  # noqa: E402

ARGS = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
OUT = ARGS[0] if ARGS else os.path.join(os.path.dirname(__file__), "glb")
ONLY = [int(a) for a in ARGS[1:]]
TEX = 256
RNG = np.random.default_rng(7)


def noise(size, cells, octaves=4):
    """Smooth value noise in 0..1."""
    result = np.zeros((size, size))
    amplitude, total = 1.0, 0.0
    for o in range(octaves):
        n = cells * 2 ** o
        grid = RNG.random((n + 1, n + 1))
        grid[-1, :] = grid[0, :]
        grid[:, -1] = grid[:, 0]
        t = np.linspace(0, n, size, endpoint=False)
        i = t.astype(int)
        f = t - i
        f = f * f * (3 - 2 * f)
        a = grid[np.ix_(i, i)]
        b = grid[np.ix_(i, i + 1)]
        c = grid[np.ix_(i + 1, i)]
        d = grid[np.ix_(i + 1, i + 1)]
        fx, fy = f[None, :], f[:, None]
        result += amplitude * ((a * (1 - fx) + b * fx) * (1 - fy) + (c * (1 - fx) + d * fx) * fy)
        total += amplitude
        amplitude *= 0.5
    return result / total


def colorize(value, low, high):
    low, high = np.array(low, float), np.array(high, float)
    return low + (high - low) * value[..., None]


def tile_edges(size, width=0.05, depth=0.35):
    """Darkened tile border so the grid reads."""
    t = np.linspace(0, 1, size)
    edge = np.minimum(np.minimum(t, 1 - t)[None, :], np.minimum(t, 1 - t)[:, None])
    return 1 - depth * np.clip(1 - edge / width, 0, 1)


def bricks(size, rows=4, columns=2, mortar=0.06):
    y = np.linspace(0, rows, size, endpoint=False)
    row = y.astype(int)
    x = np.linspace(0, columns, size, endpoint=False)[None, :] + (row[:, None] % 2) * 0.5
    fy = (y - row)[:, None]
    fx = x - np.floor(x)
    joint = (np.minimum(fy, 1 - fy) < mortar) | (np.minimum(fx, 1 - fx) < mortar / 2)
    shade = RNG.random((rows, columns * 2 + 2))
    brick = shade[row[:, None] % rows, np.floor(x).astype(int) % shade.shape[1]]
    return joint, brick


PALETTE = [1]  # the world being built: its colours are shades of its palette's (assets/palettes.py)


def image(name, rgb):
    rgb = palettes.snap(PALETTE[0], np.clip(rgb, 0, 1))
    img = bpy.data.images.new(name, rgb.shape[1], rgb.shape[0], alpha=True)
    rgba = np.concatenate([np.clip(rgb, 0, 1), np.ones(rgb.shape[:2] + (1,))], axis=2)
    img.pixels.foreach_set(rgba.astype(np.float32).ravel())
    img.pack()
    return img


def material(name, img):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = next(n for n in nodes if n.type == "BSDF_PRINCIPLED")
    bsdf.inputs["Roughness"].default_value = 0.8
    tex = nodes.new("ShaderNodeTexImage")
    tex.image = img
    links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
    return mat


def box(name, size, bevel, segments=3):
    bpy.ops.mesh.primitive_cube_add(size=1)
    obj = bpy.context.object
    obj.name = name
    obj.scale = size
    bpy.ops.object.transform_apply(scale=True)
    mod = obj.modifiers.new("Bevel", "BEVEL")
    mod.width = bevel
    mod.segments = segments
    bpy.ops.object.modifier_apply(modifier=mod.name)
    return obj


def rail(name, size, bevel, segments=2, cuts=0):
    """A box running along x with only its long edges rounded, the ends cut square so the blocks of a row meet
    without a groove between them; cuts loops along x give it vertices to shape."""
    bpy.ops.mesh.primitive_cube_add(size=1)
    obj = bpy.context.object
    obj.name = name
    obj.scale = size
    bpy.ops.object.transform_apply(scale=True)
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    along = [e for e in bm.edges if abs(e.verts[0].co.x - e.verts[1].co.x) > 0.5]
    bmesh.ops.bevel(bm, geom=along, offset=bevel, segments=segments, profile=0.5, affect="EDGES")
    if cuts:
        along = [e for e in bm.edges if abs(e.verts[0].co.x - e.verts[1].co.x) > 0.5]
        bmesh.ops.subdivide_edges(bm, edges=along, cuts=cuts, use_grid_fill=True)
    bm.to_mesh(obj.data)
    bm.free()
    sharp_ends(obj)
    return obj


def sharp_ends(obj):
    """Makes the edges round a block's end faces (at the tile's seams) sharp, so the smooth shading along the
    row doesn't bend towards them and darken each seam."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bm.normal_update()
    for face in bm.faces:
        if abs(face.normal.x) > 0.9:
            for e in face.edges:
                e.smooth = False
    bm.to_mesh(obj.data)
    bm.free()


# the built borders' atlas (see atlas()): where each part finds its colour, left, bottom, width, height
ATLAS = {"body": (0.0, 0.0, 0.5, 0.5), "stone": (0.5, 0.0, 0.5, 0.5), "wood": (0.0, 0.5, 0.25, 0.5),
         "accent": (0.25, 0.5, 0.25, 0.5), "top": (0.5, 0.5, 0.5, 0.5)}


def atlas_uv(obj, part, window=1.0, offset=(0.0, 0.0), smooth=True, top=None):
    """Projects each face along its normal's main axis into the part's piece of an atlas texture (see
    atlas()), the faces looking up into top's piece if given; window < 1 samples a smaller piece of it, at
    offset, so parts alike differ."""
    if not obj.data.uv_layers:
        obj.data.uv_layers.new()
    uv = obj.data.uv_layers.active.data
    for poly in obj.data.polygons:
        n = poly.normal
        u0, v0, w, h = ATLAS[top if top and n.z > 0.7 else part]
        axes = (1, 2) if abs(n.x) >= max(abs(n.y), abs(n.z)) else (0, 2) if abs(n.y) >= abs(n.z) else (0, 1)
        for li in poly.loop_indices:
            co = obj.data.vertices[obj.data.loops[li].vertex_index].co
            t = [np.clip((co[a] + (0.1 if a == 2 else 0.6)) / 1.2, 0, 1) for a in axes]
            uv[li].uv = (u0 + w * (0.04 + 0.92 * (offset[0] + t[0] * window)),
                         v0 + h * (0.04 + 0.92 * (offset[1] + t[1] * window)))
        poly.use_smooth = smooth


def mushroom(x, y, z, size, rng):
    """A cream capped mushroom like the world's pillars standing at z, leaning a little."""
    bpy.ops.mesh.primitive_cylinder_add(vertices=6, radius=0.03 * size, depth=0.12 * size,
                                        location=(0, 0, 0.06 * size))
    stem = bpy.context.object
    bpy.ops.mesh.primitive_uv_sphere_add(segments=10, ring_count=4, radius=0.11 * size,
                                         location=(0, 0, 0.11 * size))
    cap = bpy.context.object
    # both made around their foot, so they lean from there
    for obj in (stem, cap):
        bpy.ops.object.select_all(action="DESELECT")
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj
        bpy.ops.object.transform_apply(location=True)
    # a flat dome, some flatter than others
    squash = rng.uniform(0.4, 0.6)
    for v in cap.data.vertices:
        v.co.z = 0.11 * size + max(v.co.z - 0.11 * size, -0.01) * squash
    lean = (rng.uniform(-0.18, 0.18), rng.uniform(-0.18, 0.18), rng.uniform(0, math.pi))
    for obj in (stem, cap):
        obj.location = (x, y, z)
        obj.rotation_euler = lean
        bpy.ops.object.select_all(action="DESELECT")
        obj.select_set(True)
        bpy.context.view_layer.objects.active = obj
        bpy.ops.object.transform_apply(location=True, rotation=True)
        atlas_uv(obj, "accent")
    return [stem, cap]


def course(rng, count):
    """A course of count stones across the tile: (centre, width) with a joint of earth between them, kept off the
    tile's seams."""
    cuts = (np.arange(1, count) + rng.uniform(-0.25, 0.25, count - 1)) / count
    edges = np.concatenate([[-0.47], -0.47 + 0.94 * cuts, [0.47]])
    return [((left + right) / 2, right - left - rng.uniform(0.03, 0.06)) for left, right in zip(edges, edges[1:])]


# the earthwork's variants: stones in the lower and upper course, a stake at the left seam, the mushrooms
# (x, y, size); every tile picks one, so the wall doesn't repeat
EARTHWORK = [(2, 3, True, ((0.2, 0.06, 1.0), (0.33, -0.06, 0.65))),
             (3, 2, False, ()),
             (2, 4, True, ((-0.27, -0.07, 0.8),)),
             (3, 3, False, ((-0.06, 0.09, 1.15), (0.08, -0.02, 0.7), (-0.17, -0.08, 0.5))),
             (2, 3, True, ()),
             (2, 2, False, ((0.32, 0.1, 0.6),))]


def earthwork(name, seed, variant=0):
    """A packed earth wall block with a mossy top, fieldstones set into both faces, a pointed stake at the tile's
    seam front and back and a few cream capped mushrooms like the world's, so the row reads as a built wall,
    a block per tile. Its variants (EARTHWORK) lay the stones differently, leave the stake or the mushrooms out;
    their seams are all alike, so any two meet."""
    rng = np.random.default_rng(seed * 101 + variant)
    lower, upper, stake, mushrooms = EARTHWORK[variant]
    parts = []
    height, depth, batter = 0.78, 0.84, 0.08

    core = rail("core", (1.0, depth, height), 0.05)
    core.location.z = height / 2
    bpy.ops.object.transform_apply(location=True)
    # the faces lean in a little towards the top
    for v in core.data.vertices:
        v.co.y *= 1.0 - batter * v.co.z / height
    atlas_uv(core, "body", 0.8, rng.uniform(0, 0.2, 2))
    parts.append(core)

    # moss on top, its long sides wavy so it doesn't read as a slab; each variant waves its own way between the
    # seams, where they all meet the same
    bpy.ops.mesh.primitive_grid_add(x_subdivisions=11, y_subdivisions=2, size=1.0)
    moss = bpy.context.object
    waves = [(rng.uniform(-0.025, 0.025), int(rng.integers(1, 4))) for _ in range(2)]
    for v in moss.data.vertices:
        back = v.co.y > 0
        amount, k = waves[back]
        v.co.y *= depth * (1.0 - batter) - 0.1
        v.co.y += math.copysign(0.035, v.co.y) * math.sin(v.co.x * 2 * math.pi * 2 + back * 1.7)
        v.co.y += math.copysign(amount, v.co.y) * math.sin(math.pi * k * (v.co.x + 0.5))
        v.co.z = height + 0.05
    solidify = moss.modifiers.new("Solidify", "SOLIDIFY")
    solidify.thickness = 0.07
    bpy.ops.object.modifier_apply(modifier=solidify.name)
    sharp_ends(moss)
    atlas_uv(moss, "top", 0.7, rng.uniform(0, 0.3, 2))
    parts.append(moss)

    def face_y(z, side):
        return side * depth / 2 * (1.0 - batter * z / height)

    # two courses: big stones below, smaller ones above, the joints staggered; another layout at the back
    for side in (-1, 1):
        for z, tall, count in ((0.2, 0.3, lower), (0.52, 0.24, upper)):
            for x, wide in course(rng, count):
                stone = box("stone", (wide, 0.2, tall * rng.uniform(0.85, 1.05)), 0.07, 1)
                for v in stone.data.vertices:
                    v.co += type(v.co)(rng.uniform(-0.03, 0.03, 3))
                stone.location = (x, face_y(z, side) - side * 0.05, z + rng.uniform(-0.02, 0.02))
                stone.rotation_euler = (rng.uniform(-0.06, 0.06), rng.uniform(-0.08, 0.08), rng.uniform(-0.06, 0.06))
                bpy.ops.object.transform_apply(location=True, rotation=True)
                atlas_uv(stone, "stone", 0.5, rng.uniform(0, 0.5, 2), smooth=False)
                parts.append(stone)

    # a stake at the seam to the left neighbour, front and back, not at every seam
    if stake:
        tall = rng.uniform(0.84, 1.0)
        for side in (-1, 1):
            y = face_y(0.5, side) + side * 0.01
            bpy.ops.mesh.primitive_cylinder_add(vertices=8, radius=0.075, depth=tall, location=(-0.5, y, tall / 2))
            post = bpy.context.object
            bpy.ops.mesh.primitive_cone_add(vertices=8, radius1=0.075, depth=0.16, location=(-0.5, y, tall + 0.08))
            tip = bpy.context.object
            for obj in (post, tip):
                atlas_uv(obj, "wood")
                parts.append(obj)

    # a little group of mushrooms in the moss, or none
    for x, y, size in mushrooms:
        parts += mushroom(x + rng.uniform(-0.03, 0.03), y, height + 0.05, size * rng.uniform(0.9, 1.1), rng)

    return join(name, parts)


def join(name, parts):
    bpy.ops.object.select_all(action="DESELECT")
    for p in parts:
        p.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    obj = bpy.context.object
    obj.name = name
    return obj


def chip(obj, rng, amount):
    """Moves the vertices off a block's seams a little, so its edges look hewn or grown; the seam faces stay
    where they are, so neighbours still meet."""
    for v in obj.data.vertices:
        if abs(v.co.x) < 0.495:
            v.co += type(v.co)(rng.uniform(-amount, amount, 3))


HEDGE_VARIANTS = 5


def hedge(name, seed, variant=0):
    """A clipped box hedge, flat topped with straight sides, leafy tufts breaking up its outline, on a plank
    edging front and back, a block per tile. Each variant has its own tufts and only some a peg at the seam;
    the hedge's and the planks' ends are the same in all, so any two meet."""
    rng = np.random.default_rng(seed * 101 + variant)
    parts = []
    height, depth = 0.88, 0.8
    body = rail("hedge", (1.0, depth, height - 0.06), 0.09, cuts=5)
    body.location.z = 0.06 + (height - 0.06) / 2
    bpy.ops.object.transform_apply(location=True)
    # a little narrower at the top, as hedges are clipped
    for v in body.data.vertices:
        v.co.y *= 1.0 - 0.07 * (v.co.z - 0.06) / height
    chip(body, rng, 0.02)
    atlas_uv(body, "body", 0.8, rng.uniform(0, 0.2, 2))
    parts.append(body)

    # leafy tufts along its top edges and on its top, so its outline isn't a box's; how many and where differs
    tufts = []
    for side in (-1, 1):
        count = int(rng.integers(3, 6))
        xs = np.sort(rng.uniform(-0.4, 0.4, count))
        tufts += [(x, side * 0.36, 0.8, rng.uniform(0.07, 0.1)) for x in xs]
    tufts += [(rng.uniform(-0.35, 0.35), rng.uniform(-0.15, 0.15), 0.88, rng.uniform(0.06, 0.09))
              for _ in range(int(rng.integers(0, 4)))]
    for x, y, z, r in tufts:
        bpy.ops.mesh.primitive_uv_sphere_add(segments=7, ring_count=4, radius=r,
                                             location=(x + rng.uniform(-0.03, 0.03), y, z + rng.uniform(-0.03, 0.03)))
        tuft = bpy.context.object
        tuft.scale = (rng.uniform(1.1, 1.5), 0.8 if abs(y) > 0.3 else 1.2, rng.uniform(0.7, 0.9))
        tuft.rotation_euler.z = rng.uniform(0, math.pi)
        bpy.ops.object.transform_apply(scale=True, rotation=True)
        atlas_uv(tuft, "stone", 0.5, rng.uniform(0, 0.5, 2))
        parts.append(tuft)

    # plank edging at its foot, pegged at some of the seams
    peg = variant % 2 == 0
    for side in (-1, 1):
        plank = box("plank", (1.0, 0.05, 0.16), 0.012, 1)
        plank.location = (0.0, side * (depth / 2 + 0.035), 0.08)
        bpy.ops.object.transform_apply(location=True)
        atlas_uv(plank, "wood", 0.7, rng.uniform(0, 0.3, 2), smooth=False)
        parts.append(plank)
        if peg:
            bpy.ops.mesh.primitive_cylinder_add(vertices=6, radius=0.04, depth=0.24,
                                                location=(-0.5, side * (depth / 2 + 0.07), 0.12))
            obj = bpy.context.object
            atlas_uv(obj, "wood")
            parts.append(obj)
    return join(name, parts)


SLAB_VARIANTS = 5


def rock_slabs(name, seed, variant=0):
    """Slabs of the ridge's layered rock laid dry into a block: three courses, each of one to three slabs with
    the joints in different places, each course set back a little, the edges chipped. The variants lay the
    joints elsewhere; the courses' ends at the seams are the same in all, so any two meet."""
    rng = np.random.default_rng(seed * 101 + variant)
    parts = []
    z = 0.0
    for i, (tall, deep, shift) in enumerate(((0.32, 0.92, 0.0), (0.28, 0.84, 0.03), (0.26, 0.76, -0.02))):
        count = int(rng.choice([1, 2, 2, 2, 3]))
        joints = [rng.uniform(-0.3, -0.1), rng.uniform(0.1, 0.3)] if count == 3 else list(rng.uniform(-0.3, 0.3, count - 1))
        edges = [-0.5] + joints + [0.5]
        for left, right in zip(edges, edges[1:]):
            slab = box("slab", (right - left - 0.02, deep, tall), 0.045, 1)
            slab.location = ((left + right) / 2, shift, z + tall / 2)
            bpy.ops.object.transform_apply(location=True)
            # a gap at the joint, none at the tile's seam; every variant's course is as deep there, so they meet
            for v in slab.data.vertices:
                if abs(v.co.x) > 0.485:
                    v.co.x = math.copysign(0.5, v.co.x)
            chip(slab, rng, 0.025)
            atlas_uv(slab, "body" if i % 2 == 0 else "stone", 0.6, rng.uniform(0, 0.4, 2), smooth=False, top="top")
            parts.append(slab)
        z += tall - 0.01
    return join(name, parts)


def plane(name, size, z):
    bpy.ops.mesh.primitive_plane_add(size=size, location=(0, 0, z))
    obj = bpy.context.object
    obj.name = name
    return obj


def project_uv(obj, mode, repeat=1.0):
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    if mode == "cube":
        bpy.ops.uv.cube_project(cube_size=1.0)
    else:
        bpy.ops.uv.smart_project(angle_limit=1.15, island_margin=0.01)
    bpy.ops.object.mode_set(mode="OBJECT")
    if mode == "top":
        uv = obj.data.uv_layers.active.data
        for poly in obj.data.polygons:
            for li in poly.loop_indices:
                co = obj.data.vertices[obj.data.loops[li].vertex_index].co
                uv[li].uv = ((co.x + 0.5) * repeat, (co.y + 0.5) * repeat)
    for poly in obj.data.polygons:
        poly.use_smooth = True


def export(obj, path):
    """Writes obj, or a list of objects side by side, into path."""
    objs = obj if isinstance(obj, list) else [obj]
    os.makedirs(os.path.dirname(path), exist_ok=True)
    bpy.ops.object.select_all(action="DESELECT")
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    bpy.ops.export_scene.gltf(filepath=path, export_format="GLB", use_selection=True, export_yup=True,
                              export_animations=False)
    print("RESULT", path, " ".join(str(len(o.data.polygons)) for o in objs))


def clear():
    for o in list(bpy.data.objects):
        bpy.data.objects.remove(o, do_unlink=True)


# --- the worlds -----------------------------------------------------------------------------------

def floor_texture(low, high, cells=3, edges=0.35, pattern=None):
    rgb = colorize(noise(TEX, cells), low, high)
    if pattern is not None:
        rgb = pattern(rgb)
    return rgb * tile_edges(TEX, depth=edges)[..., None]


def brick_texture(mortar_color, low, high, rows=4):
    joint, brick = bricks(TEX, rows=rows)
    rgb = colorize(0.6 * brick + 0.4 * noise(TEX, 6), low, high)
    rgb[joint] = mortar_color
    return rgb


def ashlar_texture(mortar_color, low, high, cap_low, cap_high):
    """The castle border's atlas: its wall's two courses of blocks on the left half, the coping slab on the right,
    darker at the slab's ends where it meets the next one (see ashlar_uv())."""
    wall = brick_texture(mortar_color, low, high, rows=2)[:, ::2]
    t = np.linspace(0, 1, TEX // 2)
    slab = colorize(0.7 * noise(TEX, 5)[:, :TEX // 2] + 0.3 * noise(TEX, 14)[:, :TEX // 2], cap_low, cap_high)
    joint = np.clip(1 - np.minimum(t, 1 - t) / 0.025, 0, 1)
    slab = slab * (1 - 0.55 * joint)[None, :, None]
    slab[:, (t < 0.008) | (t > 0.992)] = mortar_color
    return np.concatenate([wall, slab], axis=1)


def ashlar_uv(obj, body_height, body_depth, cap_bottom, cap_height, cap_depth):
    """Lays the castle border's wall faces onto the atlas' left half, a course per half of the height and the
    blocks' joints where the next block's continue, and the coping's onto the right half, one slab per block."""
    if not obj.data.uv_layers:
        obj.data.uv_layers.new()
    uv = obj.data.uv_layers.active.data
    for poly in obj.data.polygons:
        n = poly.normal
        cap = poly.center.z > cap_bottom + 0.005
        depth = cap_depth if cap else body_depth
        for li in poly.loop_indices:
            co = obj.data.vertices[obj.data.loops[li].vertex_index].co
            along = co.x + 0.5 if abs(n.x) < max(abs(n.y), abs(n.z)) else co.y / depth + 0.5
            if abs(n.z) > max(abs(n.x), abs(n.y)):
                v = co.y / depth + 0.5
            elif cap:
                v = 0.25 * (co.z - cap_bottom) / cap_height
            else:
                v = co.z / body_height
            u = min(max(along, 0.0), 1.0)
            uv[li].uv = ((0.5 if cap else 0.0) + 0.004 + 0.492 * u, min(max(v, 0.0), 1.0))
        poly.use_smooth = True


def rock_texture(low, high):
    return colorize(noise(TEX, 4, octaves=5) ** 1.3, low, high)


def stone_tiles(rgb, lines=2, color=(0.18, 0.18, 0.2)):
    t = np.linspace(0, lines, TEX, endpoint=False)
    f = t - np.floor(t)
    joint = (np.minimum(f, 1 - f) < 0.03)
    rgb[joint[None, :].repeat(TEX, 0) | joint[:, None].repeat(TEX, 1)] = color
    return rgb


def carpet(rgb):
    t = np.linspace(0, 1, TEX)
    trim = (np.abs(t - 0.5) > 0.36) & (np.abs(t - 0.5) < 0.42)
    rgb[:, trim] = (0.85, 0.65, 0.2)
    return rgb


def water(size_cells=6):
    n = noise(TEX, size_cells)
    streak = (np.sin(np.linspace(0, 12 * math.pi, TEX))[None, :] * 0.5 + 0.5) * 0.25
    return colorize(np.clip(n * 0.8 + streak, 0, 1), (0.05, 0.25, 0.65), (0.35, 0.65, 0.95))


def lava():
    n = noise(TEX, 5, octaves=5)
    crust = n < 0.45
    rgb = colorize(np.clip((n - 0.45) * 2.5, 0, 1), (1.0, 0.25, 0.02), (1.0, 0.85, 0.2))
    rgb[crust] = colorize(n[crust] / 0.45, (0.08, 0.04, 0.03), (0.25, 0.1, 0.06))
    return rgb


def cobbles(low, high, joint, cells=5):
    """Irregular stones, tileable: a jittered grid that wraps."""
    t = np.linspace(0, cells, TEX, endpoint=False)
    X, Y = np.meshgrid(t, t)
    jitter = RNG.random((cells, cells, 2))
    f1 = np.full(X.shape, 9.0)
    f2 = np.full(X.shape, 9.0)
    for dj in (-1, 0, 1):
        for di in (-1, 0, 1):
            cx, cy = np.floor(X) + di, np.floor(Y) + dj
            j = jitter[cy.astype(int) % cells, cx.astype(int) % cells]
            dist = np.hypot(X - cx - j[..., 0], Y - cy - j[..., 1])
            f2 = np.where(dist < f1, f1, np.minimum(f2, dist))
            f1 = np.minimum(f1, dist)
    rgb = colorize(np.clip(1.0 - f1 * 0.9 + noise(TEX, 8) * 0.3, 0, 1), low, high)
    rgb[(f2 - f1) < 0.08] = joint
    return rgb


def diamonds(light, dark, line, count=2):
    """Diamond tiles like the mechanical castle's floors."""
    t = np.linspace(0, count, TEX, endpoint=False)
    X, Y = np.meshgrid(t, t)
    u, v = X + Y, X - Y
    fu, fv = u - np.floor(u), v - np.floor(v)
    checker = (np.floor(u) + np.floor(v)) % 2
    rgb = colorize(checker * (0.8 + 0.2 * noise(TEX, 4)), dark, light)
    edge = np.minimum(np.minimum(fu, 1 - fu), np.minimum(fv, 1 - fv))
    rgb[edge < 0.05] = line
    # a soft highlight on each diamond's upper half
    rgb *= (1.0 + 0.12 * np.clip(0.5 - fu, -0.5, 0.5))[..., None]
    return rgb


def ripples():
    n = noise(TEX, 3)
    t = np.linspace(0, 2 * math.pi * 3, TEX, endpoint=False)
    X, Y = np.meshgrid(t, t)
    wave = np.sin(X + np.sin(Y) * 0.8 + n * 4) * 0.5 + 0.5
    # like the river's water shader: deep teal blue, soft caustics instead of drawn wave lines
    rgb = colorize(np.clip(n * 0.6 + wave * 0.4, 0, 1), (0.05, 0.27, 0.48), (0.14, 0.42, 0.66))
    caustic = np.clip((wave - 0.8) / 0.2, 0, 1)
    rgb = rgb + caustic[..., None] * np.array([0.18, 0.24, 0.26])
    return rgb


def trodden_earth():
    """Soft brown earth with a few pebbles, like the cave's ground around the field."""
    rgb = colorize(noise(TEX, 3, octaves=5), (0.46, 0.3, 0.15), (0.6, 0.42, 0.22))
    rgb = rgb * (0.88 + 0.12 * noise(TEX, 6))[..., None]
    pebbles = cobbles((0.5, 0.38, 0.26), (0.66, 0.52, 0.36), (0.42, 0.3, 0.18), cells=7)
    stones = noise(TEX, 7) > 0.72
    rgb[stones] = rgb[stones] * 0.4 + pebbles[stones] * 0.6
    return rgb


def metal_panels(base):
    """The final floor's border like the original's: grey metal, a small blue lit window in each panel, a dark
    vent grille below it."""
    rgb = colorize(noise(TEX, 6), np.array(base) * 0.85, np.array(base) * 1.05)
    t = np.linspace(0, 1, TEX)
    for line in (0.0, 0.5):
        band = np.abs(t - line) < 0.02
        rgb[band, :] = np.array(base) * 0.5
        rgb[:, band] = np.array(base) * 0.5
    # rows run bottom up (Blender's pixel order): the windows in the upper half, the vents below
    for cx in (0.25, 0.75):
        frame = (np.abs(t - 0.7) < 0.1)[:, None] & (np.abs(t - cx) < 0.1)[None, :]
        window = (np.abs(t - 0.7) < 0.065)[:, None] & (np.abs(t - cx) < 0.065)[None, :]
        rgb[frame] = (0.16, 0.18, 0.22)
        rgb[window] = colorize(np.clip(1.2 - np.abs(t - 0.7)[:, None] * 12 + 0 * t[None, :], 0, 1),
                               (0.15, 0.45, 0.9), (0.6, 0.9, 1.0))[window]
        vent = (np.abs(t - 0.28) < 0.08)[:, None] & (np.abs(t - cx) < 0.16)[None, :] & (np.sin(t * 160) > 0.2)[:, None]
        rgb[vent] = (0.25, 0.26, 0.3)
    return rgb


def flagstones(light, dark, joint, slabs=2):
    """Cut stone slabs, slabs x slabs per tile, every other one a shade off, worn at the corners."""
    t = np.linspace(0, slabs, TEX, endpoint=False)
    X, Y = np.meshgrid(t, t)
    fx, fy = X - np.floor(X), Y - np.floor(Y)
    checker = (np.floor(X) + np.floor(Y)) % 2
    rgb = colorize(np.clip(0.45 * checker + 0.55 * noise(TEX, 5, octaves=5), 0, 1), dark, light)
    edge = np.minimum(np.minimum(fx, 1 - fx), np.minimum(fy, 1 - fy))
    # a soft bevel into the joint, the joint itself dark
    rgb *= (0.82 + 0.18 * np.clip(edge / 0.08, 0, 1))[..., None]
    rgb[edge < 0.025] = joint
    # chips and stains
    stain = noise(TEX, 9) > 0.7
    rgb[stain] *= 0.9
    return rgb


def tread_plate(base, count=4):
    """Steel deck plate: raised lozenges in pairs at right angles, bolted at the plate's corners."""
    t = np.linspace(0, count, TEX, endpoint=False)
    X, Y = np.meshgrid(t, t)
    fx, fy = X - np.floor(X) - 0.5, Y - np.floor(Y) - 0.5
    turn = (np.floor(X) + np.floor(Y)) % 2
    u = np.where(turn > 0, fx + fy, fx - fy) * 0.7071
    v = np.where(turn > 0, fx - fy, fx + fy) * 0.7071
    lozenge = (u / 0.32) ** 2 + (v / 0.07) ** 2 < 1.0
    rgb = colorize(0.6 * noise(TEX, 6) + 0.4 * noise(TEX, 24, octaves=2), np.array(base) * 0.8, np.array(base) * 1.05)
    rgb[lozenge] = rgb[lozenge] * 1.25
    # a dark seam under each lozenge's lower side
    shadow = ((u / 0.32) ** 2 + ((v + 0.03) / 0.07) ** 2 < 1.0) & ~lozenge
    rgb[shadow] *= 0.65
    rgb = rgb * tile_edges(TEX, width=0.03, depth=0.5)[..., None]
    s = np.linspace(0, 1, TEX)
    for cx in (0.07, 0.93):
        for cy in (0.07, 0.93):
            bolt = np.hypot(s[None, :] - cx, s[:, None] - cy) < 0.025
            rgb[bolt] = np.array(base) * 1.3
    return np.clip(rgb, 0, 1)


def hazard_crate(base):
    """A riveted steel crate face: tread plate inside a black and yellow striped frame."""
    rgb = tread_plate(base, count=3)
    s = np.linspace(0, 1, TEX)
    X, Y = np.meshgrid(s, s)
    edge = np.minimum(np.minimum(X, 1 - X), np.minimum(Y, 1 - Y))
    frame = (edge > 0.03) & (edge < 0.16)
    stripe = np.floor((X + Y) * 7) % 2
    rgb[frame] = colorize(stripe[frame], (0.1, 0.09, 0.08), (0.93, 0.7, 0.12))
    rgb[edge <= 0.03] = np.array(base) * 0.55
    for c in (0.095,):
        for cx in (c, 1 - c):
            for cy in (c, 1 - c):
                rivet = np.hypot(X - cx, Y - cy) < 0.03
                rgb[rivet] = np.array(base) * 1.3
    return rgb


def atlas(body, stone, wood, accent, top):
    """A built border's texture: the parts' colours (TEX by TEX each, wood and accent only use their left half)
    where ATLAS has them."""
    wood = wood.copy()
    wood[:, TEX // 2:] = accent[:, TEX // 2:]
    return np.concatenate([np.concatenate([body, stone], axis=1), np.concatenate([wood, top], axis=1)], axis=0)


def strata(low, high, layers=7):
    """Rock or earth laid down in wavy layers, the layers horizontal on a block's sides."""
    rows = np.linspace(0, 1, TEX)[:, None].repeat(TEX, 1)
    bands = np.sin(rows * 2 * math.pi * layers + noise(TEX, 3) * 5) * 0.5 + 0.5
    return colorize(np.clip(0.55 * noise(TEX, 5, octaves=5) + 0.3 * bands, 0, 1), low, high)


def wood_grain(low, high):
    grain = np.sin(np.linspace(0, 1, TEX)[None, :] * 2 * math.pi * 9 + noise(TEX, 2) * 4) * 0.5 + 0.5
    return colorize(np.clip(0.6 * grain + 0.4 * noise(TEX, 6), 0, 1), low, high)


def leaves(low, high):
    """Small leaves packed close: fine cells, each its own shade, dark gaps between them."""
    rgb = colorize(np.clip(0.5 * noise(TEX, 24, octaves=2) + 0.5 * noise(TEX, 5), 0, 1), low, high)
    gaps = noise(TEX, 32, octaves=2) < 0.3
    rgb[gaps] *= 0.75
    return rgb


def earthwork_texture():
    """Packed earth in layers, fieldstone, weathered stake wood, the mushrooms' cream, moss."""
    earth = strata((0.25, 0.14, 0.07), (0.48, 0.3, 0.15))
    grit = noise(TEX, 16, octaves=2) > 0.68
    earth[grit] *= 1.25
    stone = colorize(np.clip(noise(TEX, 4, octaves=5) * 1.2 - 0.1, 0, 1), (0.34, 0.3, 0.26), (0.6, 0.55, 0.47))
    lichen = noise(TEX, 6) > 0.7
    stone[lichen] = stone[lichen] * 0.6 + np.array((0.55, 0.56, 0.3)) * 0.4
    wood = wood_grain((0.22, 0.16, 0.1), (0.42, 0.33, 0.23))
    cream = colorize(noise(TEX, 4), (0.86, 0.78, 0.58), (0.98, 0.93, 0.8))
    moss = colorize(noise(TEX, 6, octaves=5), (0.17, 0.29, 0.09), (0.33, 0.47, 0.16))
    bare = noise(TEX, 5) > 0.68
    moss[bare] = moss[bare] * 0.4 + np.array((0.36, 0.24, 0.12)) * 0.6
    return atlas(earth, stone, wood, cream, moss)


def hedge_texture():
    """Dark green leaves for the hedge, lighter ones for its tufts, oak planks."""
    inner = leaves((0.03, 0.14, 0.04), (0.1, 0.3, 0.07))
    tufts = leaves((0.05, 0.19, 0.05), (0.15, 0.38, 0.1))
    plank = wood_grain((0.3, 0.2, 0.11), (0.52, 0.38, 0.22))
    return atlas(inner, tufts, plank, plank, inner)


def rock_slab_texture():
    """The ridge's orange rock in layers, a darker and a lighter course."""
    light = strata((0.44, 0.27, 0.15), (0.72, 0.5, 0.3), layers=9)
    dark = strata((0.34, 0.2, 0.11), (0.58, 0.38, 0.22), layers=9)
    # the slabs' tops: weathered, no layers
    top = rock_texture((0.46, 0.3, 0.17), (0.7, 0.5, 0.31))
    return atlas(light, dark, dark, dark, top)


WORLDS = {
    1: dict(name="the_wall",
            # the meadow's greens (build_surroundings.world_meadow), the field's turf a touch fresher than round it
            floor=lambda: floor_texture((0.3, 0.46, 0.21), (0.46, 0.6, 0.27)),
            border=("bricks", lambda: brick_texture((0.45, 0.43, 0.4), (0.5, 0.49, 0.47), (0.72, 0.7, 0.66))),
            land=lambda: colorize(noise(TEX, 8), (0.22, 0.36, 0.2), (0.4, 0.52, 0.25))),
    2: dict(name="rocky_mountains",
            floor=lambda: cobbles((0.5, 0.5, 0.62), (0.7, 0.7, 0.8), (0.3, 0.3, 0.38)) * tile_edges(TEX, depth=0.2)[..., None],
            # slabs of the ridge's layered rock
            border=("slabs", rock_slab_texture),
            land=lambda: rock_texture((0.38, 0.28, 0.18), (0.55, 0.42, 0.28))),
    3: dict(name="river",
            # a shallow riverbed, the water shader floods it
            floor=lambda: cobbles((0.5, 0.46, 0.36), (0.7, 0.64, 0.5), (0.36, 0.32, 0.26), cells=6),
            border=("bricks", lambda: brick_texture((0.3, 0.32, 0.35), (0.5, 0.55, 0.6), (0.72, 0.76, 0.8), rows=3)),
            land=water),
    4: dict(name="forest",
            floor=lambda: floor_texture((0.12, 0.35, 0.1), (0.25, 0.5, 0.15)),
            # a clipped hedge on a plank edging
            border=("hedge", hedge_texture),
            land=lambda: colorize(noise(TEX, 10), (0.05, 0.2, 0.05), (0.14, 0.34, 0.1))),
    5: dict(name="lava_cave",
            floor=lambda: trodden_earth() * tile_edges(TEX, depth=0.1)[..., None],
            # a built wall of packed earth and fieldstones between stakes, turf on top
            border=("earthwork", earthwork_texture),
            land=lava),
    6: dict(name="castle_1",
            # the keep's ground floor: sandstone flags, ashlar walls with a coping
            floor=lambda: flagstones((0.8, 0.7, 0.52), (0.62, 0.5, 0.36), (0.3, 0.24, 0.18)),
            border=("ashlar", lambda: ashlar_texture((0.32, 0.29, 0.26), (0.5, 0.47, 0.42), (0.7, 0.66, 0.58),
                                                     (0.6, 0.56, 0.49), (0.74, 0.7, 0.62))),
            land=lambda: colorize(noise(TEX, 8), (0.3, 0.24, 0.18), (0.44, 0.36, 0.27))),
    7: dict(name="castle_2",
            # the castle's machine floor: steel deck plate, riveted crates striped black and yellow
            floor=lambda: tread_plate((0.46, 0.48, 0.5)),
            border=("crate", lambda: hazard_crate((0.4, 0.42, 0.45))),
            land=lambda: colorize(noise(TEX, 8), (0.14, 0.13, 0.12), (0.24, 0.22, 0.2))),
    8: dict(name="castle_final",
            floor=lambda: diamonds((0.14, 0.56, 0.62), (0.06, 0.38, 0.46), (0.03, 0.2, 0.26)),
            border=("bricks", lambda: metal_panels((0.62, 0.64, 0.7))),
            land=lambda: colorize(noise(TEX, 8), (0.12, 0.05, 0.05), (0.22, 0.1, 0.1))),
}


def build_world(index, spec):
    folder = os.path.join(OUT, f"world{index}")
    # every world draws its own noise, building one doesn't change the others
    global RNG
    RNG = np.random.default_rng(7 + index)
    PALETTE[0] = index

    clear()
    floor = box("floor", (1.0, 1.0, 0.12), 0.03, 2)
    floor.location.z = -0.06
    bpy.ops.object.transform_apply(location=True)
    project_uv(floor, "top")
    floor.data.materials.append(material(f"w{index}_floor", image(f"w{index}_floor", spec["floor"]())))
    export(floor, os.path.join(folder, "floor.glb"))

    clear()
    kind, texture = spec["border"]
    if kind == "bricks":
        border = box("border", (1.0, 1.0, 0.95), 0.06)
        border.location.z = 0.475
        bpy.ops.object.transform_apply(location=True)
        project_uv(border, "cube")
    elif kind == "ashlar":
        # a wall block with a coping slab overhanging front and back
        border = box("border", (1.0, 0.9, 0.8), 0.05)
        border.location.z = 0.4
        cap = box("cap", (1.0, 1.04, 0.17), 0.04)
        cap.location.z = 0.875
        bpy.ops.object.select_all(action="SELECT")
        bpy.context.view_layer.objects.active = border
        bpy.ops.object.transform_apply(location=True)
        bpy.ops.object.join()
        border = bpy.context.object
        border.name = "border"
        ashlar_uv(border, 0.8, 0.9, 0.79, 0.17, 1.04)
    elif kind == "crate":
        # rounded off, the stripes run on over the bevel
        border = box("border", (0.98, 0.98, 0.95), 0.035, 2)
        border.location.z = 0.475
        bpy.ops.object.transform_apply(location=True)
        project_uv(border, "cube")
    elif kind == "earthwork":
        border = [earthwork(f"border_{v}", index, v) for v in range(len(EARTHWORK))]
    elif kind == "hedge":
        border = [hedge(f"border_{v}", index, v) for v in range(HEDGE_VARIANTS)]
    else:
        border = [rock_slabs(f"border_{v}", index, v) for v in range(SLAB_VARIANTS)]
    # the built borders come in variants side by side, nodes border_0, border_1, ... sharing a material; the
    # client picks one per tile
    border_material = material(f"w{index}_border", image(f"w{index}_border", texture()))
    for obj in border if isinstance(border, list) else [border]:
        obj.data.materials.append(border_material)
    export(border, os.path.join(folder, "border.glb"))

    clear()
    land = plane("surroundings", 5.0, -0.13)
    project_uv(land, "top", repeat=12.0)
    uv = land.data.uv_layers.active.data
    for loop in uv:
        loop.uv = (loop.uv[0] * 1.0, loop.uv[1] * 1.0)
    land.data.materials.append(material(f"w{index}_land", image(f"w{index}_land", spec["land"]())))
    export(land, os.path.join(folder, "surroundings.glb"))


for index, spec in WORLDS.items():
    if not ONLY or index in ONLY:
        build_world(index, spec)
