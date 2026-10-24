# Helpers for the intro's cast: toy-like characters built from smooth primitives, every part rigidly (or along a
# span) bound to one bone of a simple armature, coloured from a palette texture the parts' uvs point into, the way
# the game's glTF loader wants its colour (a base colour texture, no material factors or vertex colours).
# Blender units, z up, the characters face -y. Used by the build_*.py scripts, run headless:
#   blender --background --factory-startup --python build_<who>.py -- <out dir>
import math
import os
import sys

import bmesh
import bpy
import numpy as np
from mathutils import Euler, Matrix, Quaternion, Vector

CELL = 8  # pixels per palette swatch
GRID = 8  # swatches per row and column


def out_dir():
    args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    out = os.path.abspath(args[0]) if args else os.path.join(os.path.dirname(os.path.abspath(__file__)), "out")
    os.makedirs(out, exist_ok=True)
    return out


def reset():
    bpy.ops.wm.read_factory_settings(use_empty=True)


class Palette:
    """Named colours, each one swatch of a small texture."""

    def __init__(self, name, colors):
        self.name = name
        self.index = {key: i for i, key in enumerate(colors)}
        size = CELL * GRID
        pixels = np.ones((size, size, 4), dtype=np.float32)
        for key, i in self.index.items():
            row, col = divmod(i, GRID)
            # srgb in, blender images store linear floats for non-color-managed writes, keep srgb and tag it
            pixels[row * CELL:(row + 1) * CELL, col * CELL:(col + 1) * CELL, :3] = colors[key]
        self.image = bpy.data.images.new(name, size, size, alpha=False)
        self.image.pixels = pixels.ravel()
        self.image.file_format = "PNG"
        self.image.pack()
        self.material = bpy.data.materials.new(name)
        self.material.use_nodes = True
        nodes, links = self.material.node_tree.nodes, self.material.node_tree.links
        bsdf = next(n for n in nodes if n.type == "BSDF_PRINCIPLED")
        bsdf.inputs["Roughness"].default_value = 0.55
        tex = nodes.new("ShaderNodeTexImage")
        tex.image = self.image
        tex.interpolation = "Closest"
        links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])

    def uv(self, key):
        row, col = divmod(self.index[key], GRID)
        # image rows run bottom up in blender
        return Vector(((col + 0.5) / GRID, (row + 0.5) / GRID))


class Builder:
    """Collects parts into one mesh, bound to the bones of one armature."""

    def __init__(self, name, palette):
        self.name = name
        self.palette = palette
        self.bm = bmesh.new()
        self.uv_layer = self.bm.loops.layers.uv.new("UVMap")
        self.deform = self.bm.verts.layers.deform.new()
        self.paint = self.bm.faces.layers.int.new("paint")
        self.shell = self.bm.faces.layers.int.new("shell")
        self.shells = [""]  # shell names by index, "" for the default by bone (see refine.limbs)
        self.groups = []
        self.bones = {}  # name -> (head, tail, parent, roll)

    def bone(self, name, head, tail, parent=None):
        self.bones[name] = (Vector(head), Vector(tail), parent)
        if name not in self.groups:
            self.groups.append(name)

    def _group(self, name):
        if name not in self.groups:
            self.groups.append(name)
        return self.groups.index(name)

    def add(self, mesh_bm, color, bone=None, matrix=Matrix(), weights=None, paint=False, shell=""):
        """Copies mesh_bm in, transformed, coloured; weights(co before the transform) -> {bone: w} overrides
        the rigid bone. Painted parts (eyes, cheeks) only colour the surface under them once refined (refine.py);
        parts with a shell name fuse into a surface of their own there, like hair on a head."""
        if shell not in self.shells:
            self.shells.append(shell)
        uv = self.palette.uv(color)
        local = {v: v.co.copy() for v in mesh_bm.verts}
        mesh_bm.transform(matrix)
        if matrix.determinant() < 0:
            bmesh.ops.reverse_faces(mesh_bm, faces=mesh_bm.faces)
        mapping = {}
        for v in mesh_bm.verts:
            nv = self.bm.verts.new(v.co)
            mapping[v] = nv
            influence = weights(local[v]) if weights else {bone: 1.0}
            for b, w in influence.items():
                if w > 0.0:
                    nv[self.deform][self._group(b)] = w
        for f in mesh_bm.faces:
            nf = self.bm.faces.new([mapping[v] for v in f.verts])
            nf.smooth = f.smooth
            nf[self.paint] = paint
            nf[self.shell] = self.shells.index(shell)
            for loop in nf.loops:
                loop[self.uv_layer].uv = uv
        mesh_bm.free()

    def build_static(self):
        """The parts as one mesh without a rig, for scenery."""
        mesh = bpy.data.meshes.new(self.name)
        self.bm.verts.layers.deform.remove(self.deform)
        self.bm.to_mesh(mesh)
        self.bm.free()
        mesh.materials.append(self.palette.material)
        obj = bpy.data.objects.new(self.name, mesh)
        bpy.context.scene.collection.objects.link(obj)
        return obj

    def build(self, location=(0, 0, 0)):
        mesh = bpy.data.meshes.new(self.name)
        self.bm.to_mesh(mesh)
        self.bm.free()
        mesh["shells"] = self.shells
        mesh.materials.append(self.palette.material)
        obj = bpy.data.objects.new(self.name, mesh)
        bpy.context.scene.collection.objects.link(obj)
        for g in self.groups:
            obj.vertex_groups.new(name=g)
        # bmesh deform layer indices follow the creation order of groups, which matches the above
        arm = bpy.data.armatures.new(self.name + "_rig")
        rig = bpy.data.objects.new(self.name + "_rig", arm)
        bpy.context.scene.collection.objects.link(rig)
        bpy.context.view_layer.objects.active = rig
        bpy.ops.object.mode_set(mode="EDIT")
        for name, (head, tail, _) in self.bones.items():
            eb = arm.edit_bones.new(name)
            eb.head, eb.tail = head, tail
        for name, (_, _, parent) in self.bones.items():
            if parent:
                arm.edit_bones[name].parent = arm.edit_bones[parent]
        bpy.ops.object.mode_set(mode="OBJECT")
        obj.parent = rig
        modifier = obj.modifiers.new("rig", "ARMATURE")
        modifier.object = rig
        rig.location = location
        return rig, obj


# --- primitives, each a fresh bmesh ----------------------------------------------------------------

def sphere(radius=1.0, segments=32, rings=16):
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=segments, v_segments=rings, radius=radius)
    _smooth(bm)
    return bm


def cone(r1, r2, depth, segments=24, cap=True):
    """Along +z from 0 to depth."""
    bm = bmesh.new()
    bmesh.ops.create_cone(bm, cap_ends=cap, segments=segments, radius1=r1, radius2=r2, depth=depth)
    bm.transform(Matrix.Translation((0, 0, depth / 2)))
    _smooth(bm)
    return bm


def tube(points, radii, segments=16, cap=True):
    """A smooth tapered tube through points (list of Vector) with a radius per point."""
    bm = bmesh.new()
    rings = []
    for i, p in enumerate(points):
        a = points[max(i - 1, 0)]
        b = points[min(i + 1, len(points) - 1)]
        axis = (Vector(b) - Vector(a)).normalized()
        side = axis.orthogonal().normalized()
        up = axis.cross(side)
        ring = []
        for s in range(segments):
            t = 2 * math.pi * s / segments
            ring.append(bm.verts.new(Vector(p) + (side * math.cos(t) + up * math.sin(t)) * radii[i]))
        rings.append(ring)
    for r0, r1 in zip(rings, rings[1:]):
        for s in range(segments):
            bm.faces.new((r0[s], r0[(s + 1) % segments], r1[(s + 1) % segments], r1[s]))
    if cap:
        tip = bm.verts.new(Vector(points[-1]) + (Vector(points[-1]) - Vector(points[-2])).normalized() * radii[-1])
        for s in range(segments):
            bm.faces.new((rings[-1][s], rings[-1][(s + 1) % segments], tip))
        base = bm.verts.new(Vector(points[0]) - (Vector(points[1]) - Vector(points[0])).normalized() * radii[0])
        for s in range(segments):
            bm.faces.new((rings[0][(s + 1) % segments], rings[0][s], base))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    _smooth(bm)
    return bm


def slab(outline, thickness, smooth=False):
    """An outline in the xz plane (list of (x, z)) extruded thickness along y, centred."""
    bm = bmesh.new()
    front = [bm.verts.new((x, -thickness / 2, z)) for x, z in outline]
    back = [bm.verts.new((x, thickness / 2, z)) for x, z in outline]
    face = bm.faces.new(front)
    bm.faces.new(list(reversed(back)))
    n = len(outline)
    for i in range(n):
        bm.faces.new((front[i], back[i], back[(i + 1) % n], front[(i + 1) % n]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bmesh.ops.triangulate(bm, faces=[f for f in bm.faces if len(f.verts) > 4])
    if smooth:
        _smooth(bm)
    return bm


def _smooth(bm):
    for f in bm.faces:
        f.smooth = True


def trs(location=(0, 0, 0), rotation=(0, 0, 0), scale=(1, 1, 1)):
    """Matrix from location, euler degrees and scale."""
    rot = Euler(tuple(math.radians(a) for a in rotation)).to_matrix().to_4x4()
    sca = Matrix.Diagonal(Vector(scale if not isinstance(scale, (int, float)) else (scale,) * 3)).to_4x4()
    return Matrix.Translation(location) @ rot @ sca


def mirror(matrix):
    """The matrix's mirror image across the yz plane, for the other side of a symmetric part."""
    return Matrix.Diagonal((-1, 1, 1, 1)) @ matrix


class Biped:
    """The skeleton of a small person, feet on the ground at the origin: hips at hip, shoulders at shoulder
    (half width sx), the neck at neck; arms and legs hang straight down."""

    def __init__(self, builder, hip, shoulder, sx, neck, hand, hx=0.08):
        self.hip, self.shoulder, self.sx, self.neck, self.hand, self.hx = hip, shoulder, sx, neck, hand, hx
        builder.bone("root", (0, 0, 0), (0, 0, hip * 0.6))
        builder.bone("body", (0, 0, hip), (0, 0, neck), "root")
        builder.bone("head", (0, 0, neck), (0, 0, neck + 0.3), "body")
        for side, s in (("L", 1), ("R", -1)):
            builder.bone(f"arm.{side}", (s * sx, 0, shoulder), (s * sx, 0, hand), "body")
            builder.bone(f"leg.{side}", (s * hx, 0, hip), (s * hx, 0, 0.02), "root")

    def arm(self, s):
        return [(s * self.sx, 0, self.shoulder), (s * (self.sx + 0.02), 0, (self.shoulder + self.hand) / 2),
                (s * (self.sx + 0.03), 0, self.hand)]

    def leg(self, s):
        return [(s * self.hx, 0, self.hip), (s * self.hx, 0, self.hip / 2), (s * self.hx, 0, 0.06)]


LIFT = {"L": (0, -1, 0), "R": (0, 1, 0)}  # turns an arm or leg out sideways and up
FORWARD = (1, 0, 0)  # bends the body or head forwards (towards -y), swings a hanging arm or leg backwards


def cycle(frames, *poses):
    """Keys spread evenly over a loop of frames, the first pose repeated at the end."""
    step = frames / len(poses)
    keys = [(1 + round(i * step), pose) for i, pose in enumerate(poses)]
    keys.append((frames + 1, poses[0]))
    return keys


def bone_keys(keys, rotation=True):
    """[(frame, rotation)] -> the action()'s [(frame, rotation, None)], or offsets with rotation=False."""
    return [(frame, value, None) if rotation else (frame, None, value) for frame, value in keys]


# --- animation -------------------------------------------------------------------------------------

def turn(*steps):
    """A rotation from (axis, degrees) steps around armature space axes, applied in order."""
    q = Quaternion()
    for axis, degrees in steps:
        q = Quaternion(Vector(axis).normalized(), math.radians(degrees)) @ q
    return q


def action(rig, name, frames, keys, cyclic=True):
    """keys: {bone: [(frame, rotation or None, offset or None), ...]}, rotations (see turn()) and offsets in
    armature space, on top of the rest pose and the parents' poses."""
    act = bpy.data.actions.new(name)
    act.use_fake_user = True
    rig.animation_data_create()
    rig.animation_data.action = act
    for bone_name, track in keys.items():
        pose_bone = rig.pose.bones[bone_name]
        pose_bone.rotation_mode = "QUATERNION"
        rest = pose_bone.bone.matrix_local.to_quaternion()
        previous = None
        for frame, rotation, location in track:
            if rotation is not None:
                q = rest.inverted() @ rotation @ rest
                # stay in one hemisphere so the keys interpolate the short way
                if previous is not None and previous.dot(q) < 0:
                    q.negate()
                previous = q
                pose_bone.rotation_quaternion = q
                pose_bone.keyframe_insert("rotation_quaternion", frame=frame)
            if location is not None:
                pose_bone.location = rest.inverted() @ Vector(location)
                pose_bone.keyframe_insert("location", frame=frame)
    if cyclic:
        for fc in _fcurves(act):
            fc.modifiers.new("CYCLES")
    act.frame_range = (1, frames)
    act.use_frame_range = True
    # leave the rest pose untouched for the next action
    for pose_bone in rig.pose.bones:
        pose_bone.rotation_quaternion = (1, 0, 0, 0)
        pose_bone.location = (0, 0, 0)
    track = rig.animation_data.nla_tracks.new()
    track.name = name
    track.strips.new(name, 1, act)
    track.mute = True
    rig.animation_data.action = None
    return act


def _fcurves(act):
    # blender 5 actions keep their curves in layered channelbags
    if hasattr(act, "fcurves") and len(act.fcurves):
        return list(act.fcurves)
    curves = []
    for layer in getattr(act, "layers", []):
        for strip in layer.strips:
            for bag in strip.channelbags:
                curves.extend(bag.fcurves)
    return curves


def export(rig, mesh, path):
    bpy.ops.object.select_all(action="DESELECT")
    rig.select_set(True)
    mesh.select_set(True)
    bpy.context.view_layer.objects.active = rig
    bpy.ops.export_scene.gltf(filepath=path, export_format="GLB", use_selection=True, export_animations=True,
                              export_animation_mode="NLA_TRACKS", export_apply=True)
    print("RESULT exported", path)
