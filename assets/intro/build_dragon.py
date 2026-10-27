# The dragon of the intro: a chubby green dragon with bat wings, the black bomberman rides on its head and it
# carries the princess off in its claws. One action, "fly", a flap of the wings that loops. Rigged like the intro's
# people: a spine of body, chest, neck and head, two-bone arms and legs, wings that fold at elbow and wrist, a tail of
# four bones whose end swings after the rest.
#   blender --background --factory-startup --python build_dragon.py -- <out dir>
# writes dragon.glb and dragon.blend. About 1.9 high without the wings, the player being 1.
#
# The body is one smooth shape, metaballs remeshed, the details are added on; their colours are worked out per
# vertex (darker back, a striped belly, gradients along wings and horns). refine.py then fuses body and details
# into one surface (horns, claws, spines and the wings, already in quads, stay as they are), lays it out in quads
# and bakes the colours and the ambient occlusion into textures, the way the game's glTF shader takes them.
# Faces -y, z up.
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bmesh  # noqa: E402
import bpy  # noqa: E402
from mathutils import Matrix, Quaternion, Vector, noise  # noqa: E402
from mathutils.bvhtree import BVHTree  # noqa: E402

import refine  # noqa: E402
import toy  # noqa: E402
from toy import cone, sphere, trs, tube, turn  # noqa: E402

TEXTURE = 1024

GREEN = Vector((0.24, 0.60, 0.13))
BACK = Vector((0.11, 0.38, 0.09))
BELLY = Vector((0.90, 0.86, 0.44))
SNOUT = Vector((0.50, 0.74, 0.26))
HORN = Vector((0.98, 0.90, 0.62))
HORN_TIP = Vector((1.0, 0.97, 0.86))
CLAW = Vector((0.96, 0.94, 0.86))
MEMBRANE_IN = Vector((0.46, 0.68, 0.10))
MEMBRANE_OUT = Vector((0.88, 0.86, 0.30))
WHITE = Vector((0.98, 0.98, 0.96))
PUPIL = Vector((0.04, 0.04, 0.06))
MOUTH = Vector((0.18, 0.10, 0.08))

SHOULDER = Vector((0.3, 0.24, 1.08))  # the left wing's root, the right one mirrored
WRIST = Vector((0.85, 0.66))  # in the wing's plane: x out along the span, z up
WING_ROOT = Vector((0.0, 0.16))
WING_FOOT = Vector((0.0, -0.2))
TIPS = [Vector((1.78, 0.44)), Vector((1.48, -0.42)), Vector((0.98, -0.7)), Vector((0.36, -0.52))]
SPAN = 1.78
HEAD = Vector((0.0, -0.12, 1.64))
SURFACE = []  # the body's BVHTree, to put the details on it


# --- the body: metaballs ------------------------------------------------------------------------------

def body_shape():
    mb = bpy.data.metaballs.new("dragon_body")
    mb.resolution = mb.render_resolution = 0.02
    mb.threshold = 0.6
    obj = bpy.data.objects.new("dragon_body", mb)
    bpy.context.scene.collection.objects.link(obj)

    def ellipsoid(co, radius, size=(1, 1, 1), rotation=Quaternion()):
        e = mb.elements.new(type="ELLIPSOID")
        e.co, e.radius, e.rotation = co, radius, rotation
        e.size_x, e.size_y, e.size_z = size

    def capsule(a, b, radius):
        a, b = Vector(a), Vector(b)
        e = mb.elements.new(type="CAPSULE")
        e.co = (a + b) / 2
        e.radius = radius
        e.size_x = (b - a).length / 2
        e.rotation = Vector((1, 0, 0)).rotation_difference((b - a).normalized())

    def ball(co, radius):
        e = mb.elements.new(type="BALL")
        e.co, e.radius = co, radius

    ellipsoid((0, 0.05, 0.72), 0.62, (0.95, 0.86, 1.12))
    ellipsoid((0, -0.12, 0.66), 0.48, (0.98, 0.8, 1.1))
    ball((0, -0.04, 1.1), 0.44)
    capsule((0, -0.06, 1.15), (0, -0.1, 1.4), 0.3)
    ellipsoid(tuple(HEAD), 0.6, (1.08, 1.0, 0.92))
    ellipsoid((0, -0.46, 1.5), 0.36, (1.22, 1.0, 0.78))
    for s in (1, -1):
        ball((s * 0.27, -0.3, 1.5), 0.23)
        # legs: thigh, shin, foot
        ball((s * 0.3, 0.06, 0.42), 0.3)
        capsule((s * 0.33, 0.0, 0.32), (s * 0.34, -0.08, 0.12), 0.17)
        ellipsoid((s * 0.34, -0.16, 0.07), 0.18, (1.0, 1.5, 0.6))
        # arms reaching down and forwards
        capsule((s * 0.3, -0.14, 1.0), (s * 0.33, -0.33, 0.78), 0.12)
        capsule((s * 0.33, -0.33, 0.78), (s * 0.22, -0.47, 0.56), 0.1)
        ball((s * 0.22, -0.48, 0.52), 0.11)
    capsule((0, 0.38, 0.46), (0, 0.75, 0.3), 0.23)
    capsule((0, 0.75, 0.3), (0, 1.02, 0.3), 0.16)

    bpy.context.view_layer.update()
    evaluated = obj.evaluated_get(bpy.context.evaluated_depsgraph_get())
    mesh = bpy.data.meshes.new_from_object(evaluated)
    bpy.data.objects.remove(obj)
    body = bpy.data.objects.new("body", mesh)
    bpy.context.scene.collection.objects.link(body)

    # even quads, smoothed, then fewer of them
    select(body)
    body.data.remesh_voxel_size = 0.022
    bpy.ops.object.voxel_remesh()
    for kind, settings in (("SMOOTH", {"iterations": 4, "factor": 0.5}), ("DECIMATE", {"ratio": 0.35})):
        modifier = body.modifiers.new(kind.lower(), kind)
        for key, value in settings.items():
            setattr(modifier, key, value)
        bpy.ops.object.modifier_apply(modifier=modifier.name)
    shade_smooth(body)
    SURFACE.append(BVHTree.FromObject(body, bpy.context.evaluated_depsgraph_get()))
    return body


def on_surface(point, centre, sink=0.0):
    """Where the ray from centre through point leaves the body, moved sink back towards the centre."""
    point, centre = Vector(point), Vector(centre)
    direction = (point - centre).normalized()
    hit = SURFACE[0].ray_cast(centre, direction, 5.0)[0]
    return (hit if hit is not None else point) - direction * sink


def snapped(matrix, centre, sink=0.0):
    """matrix moved so its origin sits on the body (see on_surface())."""
    origin = matrix.translation
    return Matrix.Translation(on_surface(origin, centre, sink) - origin) @ matrix


def linear(color):
    return Vector(c ** 2.2 for c in color)


def select(obj):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj


def shade_smooth(obj):
    for polygon in obj.data.polygons:
        polygon.use_smooth = True


def body_colour(co, normal):
    """Green, darker along the back, a pale belly with plates, a lighter snout, a little mottling."""
    back = smoothstep(-0.15, 0.7, normal.y + max(normal.z, 0.0) * 0.35)
    color = GREEN.lerp(BACK, back * 0.8)
    belly = smoothstep(0.25, 0.65, -normal.y) * smoothstep(0.28, 0.42, co.z) * (1.0 - smoothstep(1.12, 1.28, co.z))
    belly *= 1.0 - smoothstep(0.26, 0.4, abs(co.x))
    plates = 1.0 - 0.14 * smoothstep(0.82, 0.97, math.cos(co.z * 2 * math.pi / 0.13))
    color = color.lerp(BELLY * plates, belly)
    snout = smoothstep(-0.52, -0.62, co.y) * (1.0 - smoothstep(1.56, 1.66, co.z))
    color = color.lerp(SNOUT, snout * 0.7)
    mottle = 1.0 + 0.06 * noise.noise(co * 9.0)
    return color * mottle


def smoothstep(a, b, x):
    t = min(max((x - a) / (b - a), 0.0), 1.0)
    return t * t * (3 - 2 * t)


# --- parts --------------------------------------------------------------------------------------------

PARTS = []  # (object, region) joined at the end


def part(name, bm, colour, region, matrix=Matrix()):
    """A detail from a bmesh, coloured per vertex by colour(local co) or one colour, in region (see weights())."""
    local = {v: v.co.copy() for v in bm.verts}
    bm.transform(matrix)
    if matrix.determinant() < 0:
        bmesh.ops.reverse_faces(bm, faces=bm.faces)
    mesh = bpy.data.meshes.new(name)
    colours = [colour(local[v]) if callable(colour) else colour for v in bm.verts]
    bm.to_mesh(mesh)
    bm.free()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    attribute = mesh.color_attributes.new("paint", "FLOAT_COLOR", "POINT")
    for i, c in enumerate(colours):
        attribute.data[i].color = (*linear(c), 1.0)
    shade_smooth(obj)
    PARTS.append((obj, region))
    return obj


def side(s, matrix):
    return toy.mirror(matrix) if s < 0 else matrix


def face():
    head = trs(HEAD)
    centre = HEAD
    for s in (1, -1):
        # big eyes slanting down to the middle, an angry lid over each, a heavy brow above
        eye = snapped(head @ trs((s * 0.17, -0.4, 0.07), (0, -s * 12, s * 14)), centre, 0.035)
        part("eye", sphere(1.0, 32, 16), WHITE, "head", eye @ trs(scale=(0.15, 0.08, 0.165)))
        part("pupil", sphere(1.0, 24, 12), PUPIL, "head",
             eye @ trs((-s * 0.03, -0.066, -0.03), scale=(0.06, 0.03, 0.08)))
        part("shine", sphere(1.0, 12, 6), WHITE, "head", eye @ trs((-s * 0.012, -0.092, 0.0), scale=0.02))
        # the lid only over the inner top corner: a glare, not a squint
        part("lid", sphere(1.0, 32, 16), lambda co: GREEN.lerp(BACK, 0.35), "head",
             eye @ trs((-s * 0.015, -0.012, 0.118), (0, -s * 30, 0), (0.14, 0.088, 0.062)))
        brow = [on_surface(HEAD + Vector(p), centre, -0.02) for p in
                ((s * 0.05, -0.5, 0.2), (s * 0.17, -0.47, 0.23), (s * 0.3, -0.38, 0.18))]
        part("brow", tube(brow, [0.035, 0.048, 0.032], 12), BACK, "head")
        # nostrils
        snout = Vector((0, -0.46, 1.5))
        part("nostril", sphere(1.0, 12, 6), MOUTH, "head",
             snapped(trs((s * 0.09, -0.8, 1.56), (20, 0, s * 20), (0.032, 0.022, 0.02)), snout, 0.008))
        # horns sweeping back and out, lighter towards their tips with rings
        points = [(0, 0, 0), (0.06, 0.1, 0.14), (0.16, 0.26, 0.24), (0.3, 0.42, 0.26)]

        def horn(co):
            t = min(co.length / 0.55, 1.0)
            ring = 0.92 + 0.08 * smoothstep(0.3, 0.6, math.cos(t * 2 * math.pi * 3.5))
            return HORN.lerp(HORN_TIP, t) * ring

        part("horn", tube(points, [0.09, 0.075, 0.048, 0.012], 16), horn, "head",
             snapped(side(s, trs(HEAD + Vector((0.22, 0.06, 0.3)))), centre, 0.04))
        # fin ears
        part("ear", cone(0.1, 0.0, 0.24, 16), lambda co: GREEN.lerp(MEMBRANE_OUT, co.z / 0.24), "head",
             snapped(side(s, trs(HEAD + Vector((0.5, 0.04, -0.02)), (0, 75, 15), (1, 0.45, 1))), centre, 0.04))
    # the grumpy mouth, corners down
    snout = Vector((0, -0.46, 1.5))
    mouth = [on_surface(Vector(p), snout, -0.004) for p in
             ((-0.2, -0.7, 1.36), (-0.09, -0.8, 1.4), (0.09, -0.8, 1.4), (0.2, -0.7, 1.36))]
    part("mouth", tube(mouth, [0.014, 0.018, 0.018, 0.014], 8), MOUTH, "head")


def spines():
    """Soft spines from the back of the head down to the tail, dark with lighter tips."""
    path = [(0.34, 1.32), (0.46, 1.08), (0.54, 0.84), (0.58, 0.62), (0.7, 0.44), (0.92, 0.38)]
    for i, (y, z) in enumerate(path):
        size = 0.15 - i * 0.012
        region = "body"
        place = snapped(trs((0, y, z), (-70 + i * 6, 0, 0), (0.55, 1, 1)), (0, y - 0.6, z), 0.03)
        part("spine", cone(size * 0.55, 0.0, size, 16), lambda co, h=size: BACK.lerp(MEMBRANE_OUT, (co.z / h) ** 2),
             region, place)


def claws():
    for s in (1, -1):
        for k in (-1, 0, 1):
            part("claw", cone(0.035, 0.0, 0.12, 10), CLAW, "arm",
                 side(s, trs((0.22 + k * 0.045, -0.54, 0.5), (150 + k * 8, 0, k * 14))))
            part("toe", cone(0.04, 0.0, 0.1, 10), CLAW, "leg",
                 side(s, trs((0.34 + k * 0.07, -0.37, 0.05), (95, 0, k * 12))))


def tail_end():
    points = [(0, 0.98, 0.3), (0, 1.16, 0.38), (0, 1.28, 0.56), (0, 1.32, 0.72)]
    part("tail", tube(points, [0.11, 0.08, 0.055, 0.03], 16), lambda co: GREEN.lerp(BACK, 0.5), "tail")
    spade = toy.slab([(0, -0.02), (0.15, 0.1), (0.05, 0.13), (0, 0.3), (-0.05, 0.13), (-0.15, 0.1)], 0.05)
    part("spade", spade, lambda co: BACK.lerp(MEMBRANE_OUT, max(co.z, 0.0) / 0.3 * 0.6), "tail",
         trs((0, 1.32, 0.7), (-25, 0, 90)))


# --- wings --------------------------------------------------------------------------------------------

def rib(a, b, t):
    return a.lerp(b, t)


def wing_ribs():
    """The lines the membrane is stretched between, each from the wrist outwards: the fingers, then along the
    arm back to the shoulder and down the body."""
    ribs = [[WRIST, tip] for tip in TIPS]
    inner = [WRIST, (WRIST + WING_ROOT) / 2, WING_ROOT, WING_FOOT]
    return ribs + [inner]


def along(polyline, t):
    lengths = [(b - a).length for a, b in zip(polyline, polyline[1:])]
    total = sum(lengths)
    d = t * total
    for (a, b), length in zip(zip(polyline, polyline[1:]), lengths):
        if d <= length or length == lengths[-1]:
            return a.lerp(b, min(d / length, 1.0))
        d -= length
    return polyline[-1]


def membrane():
    """Panels between consecutive ribs, the trailing edge scalloped in towards the wrist, billowing back."""
    bm = bmesh.new()
    ribs = wing_ribs()
    nt, nu = 16, 12
    for a, b in zip(ribs, ribs[1:]):
        grid = []
        for i in range(nt + 1):
            t = i / nt
            row = []
            for j in range(nu + 1):
                u = j / nu
                p = along(a, t).lerp(along(b, t), u)
                towards = (WRIST - p)
                if towards.length > 1e-6:
                    p = p + towards.normalized() * 0.17 * math.sin(math.pi * u) * t ** 3
                billow = 0.07 * math.sin(math.pi * u) * math.sin(math.pi * min(t * 1.1, 1.0))
                row.append(bm.verts.new((p.x, billow, p.y)))
            grid.append(row)
        for i in range(nt):
            for j in range(nu):
                bm.faces.new((grid[i][j], grid[i][j + 1], grid[i + 1][j + 1], grid[i + 1][j]))
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-4)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    # a little thickness so it holds up from behind
    result = bmesh.ops.solidify(bm, geom=bm.faces, thickness=0.022)
    del result
    return bm


def membrane_colour(co):
    """Light out at the edge, green near the wrist and along the fingers."""
    distance = (Vector((co.x, co.z)) - WRIST).length / 1.1
    finger = min(point_line(Vector((co.x, co.z)), WRIST, tip) for tip in TIPS)
    color = MEMBRANE_IN.lerp(MEMBRANE_OUT, smoothstep(0.1, 0.95, distance))
    return color.lerp(MEMBRANE_IN * 0.9, 1.0 - smoothstep(0.02, 0.09, finger))


def point_line(p, a, b):
    ab = b - a
    t = min(max((p - a).dot(ab) / ab.length_squared, 0.0), 1.0)
    return (a + ab * t - p).length


def wing_matrix(s):
    # swept back a little
    return side(s, trs(SHOULDER, (0, 0, 14)))


def wings():
    for s, name in ((1, "L"), (-1, "R")):
        place = wing_matrix(s)
        part("membrane", membrane(), membrane_colour, f"wing.{name}", place)
        arm = [Vector((0.0, 0.0, 0.16)), Vector((0.45, 0.0, 0.46)), Vector((WRIST.x, 0.0, WRIST.y))]
        part("wing_arm", tube(arm, [0.075, 0.065, 0.055], 14), GREEN, f"wing.{name}", place)
        part("knuckle", sphere(1.0, 16, 8), GREEN, f"wing.{name}", place @ trs((WRIST.x, 0, WRIST.y), scale=0.065))
        for tip in TIPS:
            middle = (WRIST + tip) / 2 + Vector((0.0, 0.04))
            finger = [Vector((WRIST.x, 0, WRIST.y)), Vector((middle.x, 0.02, middle.y)), Vector((tip.x, 0, tip.y))]
            part("finger", tube(finger, [0.045, 0.032, 0.012], 10), GREEN.lerp(BACK, 0.3), f"wing.{name}", place)
        part("thumb", cone(0.045, 0.0, 0.16, 10), CLAW, f"wing.{name}",
             place @ trs((WRIST.x, 0, WRIST.y + 0.03), (0, 25, 0)))


# --- rig, weights, bake ---------------------------------------------------------------------------------

ELBOW = Vector((0.45, 0.46))  # in the wing's plane, where the wing's arm bends

BONES = {
    "root": ((0, 0, 0), (0, 0, 0.3), None),
    "body": ((0, 0.05, 0.4), (0, 0, 0.8), "root"),
    "chest": ((0, 0, 0.8), (0, -0.05, 1.2), "body"),
    "neck": ((0, -0.05, 1.2), (0, -0.08, 1.32), "chest"),
    "head": ((0, -0.08, 1.32), (0, -0.15, 1.92), "neck"),
    "tail1": ((0, 0.4, 0.45), (0, 0.62, 0.34), "root"),
    "tail2": ((0, 0.62, 0.34), (0, 0.85, 0.3), "tail1"),
    "tail3": ((0, 0.85, 0.3), (0, 1.08, 0.42), "tail2"),
    "tail4": ((0, 1.08, 0.42), (0, 1.32, 0.72), "tail3"),
}
for _s, _n in ((1, "L"), (-1, "R")):
    _place = wing_matrix(_s)
    _root = _place @ Vector((0, 0, 0))
    _elbow = _place @ Vector((ELBOW.x, 0, ELBOW.y))
    _wrist = _place @ Vector((WRIST.x, 0, WRIST.y))
    _tip = _place @ Vector((SPAN, 0, 0.44))
    BONES[f"wing_upper.{_n}"] = (tuple(_root), tuple(_elbow), "chest")
    BONES[f"wing_fore.{_n}"] = (tuple(_elbow), tuple(_wrist), f"wing_upper.{_n}")
    BONES[f"wing_out.{_n}"] = (tuple(_wrist), tuple(_tip), f"wing_fore.{_n}")
    BONES[f"upper_arm.{_n}"] = ((_s * 0.3, -0.14, 1.0), (_s * 0.26, -0.31, 0.76), "chest")
    BONES[f"forearm.{_n}"] = ((_s * 0.26, -0.31, 0.76), (_s * 0.22, -0.48, 0.52), f"upper_arm.{_n}")
    BONES[f"thigh.{_n}"] = ((_s * 0.3, 0.06, 0.42), (_s * 0.32, -0.04, 0.25), "root")
    BONES[f"shin.{_n}"] = ((_s * 0.32, -0.04, 0.25), (_s * 0.34, -0.14, 0.07), f"thigh.{_n}")

# the bones the body's one smooth shape blends between
BODY_BONES = ["root", "body", "chest", "neck", "head", "tail1", "tail2", "tail3", "tail4"] + [
    f"{bone}.{n}" for bone in ("upper_arm", "forearm", "thigh", "shin") for n in ("L", "R")]


def weights(co, region, x):
    """{bone: weight} for a vertex: the body blends between its nearest bones, the wings by the distance out along
    their span (x), the tail's end along its length; claws and toes ride on the forearm and the shin."""
    if region.startswith("wing."):
        name = region[5:]
        upper = 1.0 - smoothstep(0.25, 0.55, x)
        outer = smoothstep(WRIST.x - 0.15, WRIST.x + 0.15, x)
        return {f"wing_upper.{name}": upper, f"wing_fore.{name}": 1.0 - upper - outer, f"wing_out.{name}": outer}
    if region == "arm":
        return {f"forearm.{'L' if co.x > 0 else 'R'}": 1.0}
    if region == "leg":
        return {f"shin.{'L' if co.x > 0 else 'R'}": 1.0}
    if region == "tail":
        end = smoothstep(1.02, 1.16, co.y)
        return {"tail3": 1.0 - end, "tail4": end}
    if region != "body":
        return {region: 1.0}
    near = []
    for name in BODY_BONES:
        a, b, _ = BONES[name]
        d = segment_distance(co, Vector(a), Vector(b))
        near.append((1.0 / (d * d + 1e-4) ** 2, name))
    near.sort(reverse=True)
    near = near[:3]
    total = sum(w for w, _ in near)
    return {name: w / total for w, name in near}


def segment_distance(p, a, b):
    ab = b - a
    t = min(max((p - a).dot(ab) / ab.length_squared, 0.0), 1.0)
    return (a + ab * t - p).length


PAINTED = {"eye", "pupil", "shine", "nostril", "mouth"}  # only colour the surface once refined
# thin tapering shapes an even layout of quads can't follow, kept as they are on the fused body
DETAILS = {"horn", "claw", "toe", "spine", "ear", "spade"}


def join(body):
    """Everything into one mesh with its paint, its region per vertex kept for the weights, and per face what
    refine.py needs: the part it belongs to, whether it's only painted on, the surface it fuses into (the body
    with its brows, lids and tail; the thin details; each wing's arm and fingers; each membrane)."""
    regions = ["body", "head", "arm", "leg", "tail", "wing.L", "wing.R"]
    paint = body.data.color_attributes.new("paint", "FLOAT_COLOR", "POINT")
    for v in body.data.vertices:
        paint.data[v.index].color = (*linear(body_colour(v.co, v.normal)), 1.0)
    objects = [(body, "body")] + PARTS
    names = ["body"] + sorted({obj.name.split(".")[0] for obj, _ in PARTS})
    shells = ["body", "detail", "wing.L", "wing.R", "membrane.L", "membrane.R"]
    for obj, region in objects:
        layer = obj.data.attributes.new("region", "INT", "POINT")
        layer.data.foreach_set("value", [regions.index(region)] * len(obj.data.vertices))
        # the wings' weights go by the distance out along the span, in the wing's own plane
        span = obj.data.attributes.new("span", "FLOAT", "POINT")
        if region.startswith("wing."):
            inverse = wing_matrix(1 if region.endswith("L") else -1).inverted()
            span.data.foreach_set("value", [(inverse @ v.co).x for v in obj.data.vertices])
        name = obj.name.split(".")[0]
        shell = region if region.startswith("wing.") else "body"
        if name == "membrane":
            shell = shell.replace("wing", "membrane")
        elif name in DETAILS:
            shell = "detail"
        count = len(obj.data.polygons)
        for attribute, value in (("part", names.index(name)), ("paint", name in PAINTED),
                                 ("shell", shells.index(shell))):
            obj.data.attributes.new(attribute, "INT", "FACE").data.foreach_set("value", [int(value)] * count)
    bpy.ops.object.select_all(action="DESELECT")
    for obj, _ in objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = body
    bpy.ops.object.join()
    body.name = body.data.name = "dragon"
    body.data["shells"] = shells
    return body, regions


def paint_material(obj):
    """The paint as the base colour, which refine.py bakes from."""
    material = bpy.data.materials.new("dragon_paint")
    material.use_nodes = True
    nodes, links = material.node_tree.nodes, material.node_tree.links
    bsdf = next(n for n in nodes if n.type == "BSDF_PRINCIPLED")
    attribute = nodes.new("ShaderNodeAttribute")
    attribute.attribute_name = "paint"
    links.new(attribute.outputs["Color"], bsdf.inputs["Base Color"])
    obj.data.materials.clear()
    obj.data.materials.append(material)


def rig(obj, regions):
    arm = bpy.data.armatures.new("dragon_rig")
    rig_obj = bpy.data.objects.new("dragon_rig", arm)
    bpy.context.scene.collection.objects.link(rig_obj)
    select(rig_obj)
    bpy.ops.object.mode_set(mode="EDIT")
    for name, (head, tail, _) in BONES.items():
        bone = arm.edit_bones.new(name)
        bone.head, bone.tail = head, tail
    for name, (_, _, parent) in BONES.items():
        if parent:
            arm.edit_bones[name].parent = arm.edit_bones[parent]
    bpy.ops.object.mode_set(mode="OBJECT")

    groups = {name: obj.vertex_groups.new(name=name) for name in BONES}
    region = [0] * len(obj.data.vertices)
    span = [0.0] * len(obj.data.vertices)
    obj.data.attributes["region"].data.foreach_get("value", region)
    obj.data.attributes["span"].data.foreach_get("value", span)
    for v in obj.data.vertices:
        for name, w in weights(v.co, regions[region[v.index]], span[v.index]).items():
            if w > 1e-3:
                groups[name].add([v.index], w, "REPLACE")
    obj.data.attributes.remove(obj.data.attributes["region"])
    obj.data.attributes.remove(obj.data.attributes["span"])
    obj.parent = rig_obj
    modifier = obj.modifiers.new("rig", "ARMATURE")
    modifier.object = rig_obj
    return rig_obj


# the tail's end and the shins swing after the rest: the tail heavy and slow, the legs dangling
FOLLOW = [("tail3", 0.2, 0.78), ("tail4", 0.16, 0.8), ("shin.L", 0.3, 0.7), ("shin.R", 0.3, 0.7)]
PITCH = (1, 0, 0)  # tips the body, head or tail forwards (towards -y)


def animate(rig_obj):
    """One flap in 16 frames. The downstroke drives from the shoulder, the forearm a frame behind and the hand's
    span two, spread wide; on the upstroke the wing folds at elbow and wrist and opens again at the top. The body
    is lifted by each downstroke and sinks between them, the chest rocking with the beat while neck and head make
    up for it so the head stays steady; tail and legs trail along."""
    frames = 16
    keys = {}
    for name, s in (("L", 1), ("R", -1)):
        lift = (0, -s, 0)  # turns the span up
        sweep = (0, 0, s)  # turns the span back
        # (frame, up, sweep) of each segment round the beat; later segments lag and fold on the way up
        upper = [(1, 50, 8), (7, -36, -10), (11, -10, 0), (17, 50, 8)]
        fore = [(1, -15, 18), (3, 10, 0), (8, 6, -4), (12, -35, 30), (17, -15, 18)]
        out = [(1, -10, 25), (4, 25, 0), (9, -20, -6), (13, -40, 35), (17, -10, 25)]
        for bone, track in (("wing_upper", upper), ("wing_fore", fore), ("wing_out", out)):
            keys[f"{bone}.{name}"] = [(f, turn((lift, a), (sweep, b)), None) for f, a, b in track]
        # the arms hold on to the princess; the thighs pull up a little as the body is lifted
        keys[f"upper_arm.{name}"] = [(1, turn((PITCH, 4)), None), (9, turn((PITCH, -3)), None),
                                     (17, turn((PITCH, 4)), None)]
        keys[f"forearm.{name}"] = [(1, turn((PITCH, -6)), None), (10, turn((PITCH, 4)), None),
                                   (17, turn((PITCH, -6)), None)]
        keys[f"thigh.{name}"] = [(1, turn((PITCH, -14 + s * 4)), None), (9, turn((PITCH, 10 - s * 4)), None),
                                 (17, turn((PITCH, -14 + s * 4)), None)]
    keys["root"] = [(1, None, (0, 0, -0.05)), (5, None, (0, 0, -0.07)), (10, None, (0, 0, 0.11)),
                    (17, None, (0, 0, -0.05))]
    keys["chest"] = [(1, turn((PITCH, -5)), None), (8, turn((PITCH, 6)), None), (17, turn((PITCH, -5)), None)]
    keys["neck"] = [(1, turn((PITCH, 4)), None), (9, turn((PITCH, -5)), None), (17, turn((PITCH, 4)), None)]
    keys["head"] = [(1, turn((PITCH, 2)), None), (10, turn((PITCH, -3)), None), (17, turn((PITCH, 2)), None)]
    keys["tail1"] = [(1, turn(((0, 0, 1), 8), (PITCH, 6)), None), (9, turn(((0, 0, 1), -8), (PITCH, -6)), None),
                     (17, turn(((0, 0, 1), 8), (PITCH, 6)), None)]
    keys["tail2"] = [(1, turn(((0, 0, 1), 6)), None), (11, turn(((0, 0, 1), -6)), None),
                     (17, turn(((0, 0, 1), 6)), None)]
    toy.action(rig_obj, "fly", frames, keys, follow=FOLLOW)


def build():
    toy.reset()
    body = body_shape()
    face()
    spines()
    claws()
    tail_end()
    wings()
    dragon, regions = join(body)
    paint_material(dragon)
    rig_obj = rig(dragon, regions)
    dragon = refine.refine(rig_obj, dragon, faces=7000, texture=TEXTURE, voxel=0.012, shell=lambda bone: "body",
                           keep=("detail", "wing.L", "wing.R", "membrane.L", "membrane.R"), reach=0.1)
    animate(rig_obj)
    return rig_obj, dragon


rig_obj, mesh = build()
out = toy.out_dir()
toy.export(rig_obj, mesh, os.path.join(out, "dragon.glb"))
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(out, "dragon.blend"))
print("RESULT dragon", tuple(round(d, 2) for d in mesh.dimensions), "verts", len(mesh.data.vertices))
