# Skeleton, skin weights and looping idle/walk cycles for the SDF models. Weights come from the shapes
# themselves: every vertex follows the bones of the shapes it is closest to, blended softly across fillets.
import math
import os

import bpy
import numpy as np
from mathutils import Quaternion, Vector

FPS = 24
# idle and walk loop, die plays once and holds
CYCLES = {"Idle": 48, "Walk": 24, "Die": 30}
# width of the weight transition between neighbouring shapes, model units
FALLOFF = 0.06
MAX_INFLUENCES = 4
X, Y, Z = Vector((1, 0, 0)), Vector((0, 1, 0)), Vector((0, 0, 1))


def _side(bone):
    return float(bone.rsplit(".", 1)[1].rstrip("b")) if "." in bone else 0.0


def _center(shape):
    return shape.c if hasattr(shape, "c") else shape.a


def build_armature(model, low):
    s = model.scale
    body = next(sh for sh in model.shapes if sh.bone == "body")
    cx, cy = _center(body)[:2]
    top = max(sh.bounds()[1][2] for sh in model.shapes if sh.bone == "body")

    data = bpy.data.armatures.new(f"{model.name}_rig")
    arm = bpy.data.objects.new(f"{model.name}_RIG", data)
    low.users_collection[0].objects.link(arm)
    arm.location = low.location
    arm.show_in_front = False
    data.display_type = "STICK"

    bpy.ops.object.select_all(action="DESELECT")
    bpy.context.view_layer.objects.active = arm
    arm.select_set(True)
    bpy.ops.object.mode_set(mode="EDIT")
    root = data.edit_bones.new("body")
    root.head, root.tail = Vector((cx, cy, 0)) * s, Vector((cx, cy, top)) * s
    for name, (parent, head, tail) in model.bones.items():
        b = data.edit_bones.new(name)
        b.head, b.tail = Vector(head) * s, Vector(tail) * s
        if b.length < 0.02:
            b.tail = b.head + Vector((0, 0, 0.1))
        b.parent = data.edit_bones[parent]
        b.inherit_scale = "NONE"
    bpy.ops.object.mode_set(mode="OBJECT")

    _skin(model, low, arm)
    return arm


def _skin(model, low, arm):
    mesh = low.data
    co = np.array([v.co for v in mesh.vertices]) / model.scale
    dist = np.stack([sh.sdf(co) for sh in model.shapes], axis=1)
    w = np.exp(-(dist - dist.min(axis=1, keepdims=True)) / FALLOFF)
    names = ["body"] + list(model.bones)
    bone_w = np.zeros((len(co), len(names)))
    for i, sh in enumerate(model.shapes):
        bone_w[:, names.index(sh.bone)] += w[:, i]
    # keep the strongest few influences, drop the faint tail
    order = np.argsort(-bone_w, axis=1)
    keep = np.zeros_like(bone_w, bool)
    np.put_along_axis(keep, order[:, :MAX_INFLUENCES], True, axis=1)
    bone_w = np.where(keep, bone_w, 0.0)
    bone_w /= bone_w.sum(axis=1, keepdims=True)
    bone_w[bone_w < 0.02] = 0.0
    bone_w /= bone_w.sum(axis=1, keepdims=True)

    low.vertex_groups.clear()
    for j, name in enumerate(names):
        group = low.vertex_groups.new(name=name)
        for weight in np.unique(bone_w[:, j][bone_w[:, j] > 0]):
            group.add(np.flatnonzero(bone_w[:, j] == weight).tolist(), float(weight), "REPLACE")
    low.parent = arm
    low.location = (0, 0, 0)
    low.matrix_parent_inverse.identity()
    mod = low.modifiers.new("Armature", "ARMATURE")
    mod.object = arm


def _lift_axis(bone):
    """Horizontal axis across a limb; a positive turn about it raises the limb's tip."""
    d = bone.tail_local - bone.head_local
    d.z = 0
    return d.normalized().cross(Z) * -1 if d.length > 1e-4 else X


def _body_radius(model):
    """How far the body reaches behind its centre, so it lies on the ground after falling over."""
    body = [sh for sh in model.shapes if sh.bone == "body"]
    return max(sh.bounds()[1][1] for sh in body) * model.scale


def _die_pose(model, bone, t):
    """Falls over backwards with a little hop, lands with a squash; limbs flop."""
    deg = math.radians
    fall = min(t / 0.55, 1.0)
    fall = fall * fall * (3 - 2 * fall)
    settle = math.sin(math.pi * min(max((t - 0.55) / 0.25, 0.0), 1.0))
    name, side = bone.name, _side(bone.name)
    rot, loc, squash = [], Vector(), 1.0

    if name == "body" and model.motion == "slither":
        # too long to fall over backwards: it keels over onto its side where it lies
        rot.append((Y, deg(35) * fall - deg(6) * settle))
        if 0.5 < t < 0.75:
            squash = 1 - 0.1 * math.sin(math.pi * (t - 0.5) / 0.25)
        return rot, loc, squash

    if name == "body":
        rot.append((X, -deg(90) * fall + deg(8) * settle))
        if model.motion == "spin":
            rot.append((Z, deg(540) * fall))
        hop = 0.25 * model.scale * math.sin(math.pi * min(t / 0.35, 1.0)) if t < 0.35 else 0.0
        loc.z = hop + _body_radius(model) * fall
        if 0.5 < t < 0.75:
            squash = 1 - 0.15 * math.sin(math.pi * (t - 0.5) / 0.25)
        return rot, loc, squash

    if name.startswith("leg"):
        axis = _lift_axis(bone) if name[3:4].isdigit() else X
        rot.append((axis, deg(35 if name[3:4].isdigit() else 40) * fall))
    elif name.startswith(("arm", "ear")):
        rot.append((_lift_axis(bone), deg(60) * fall))
    elif name.startswith("tent"):
        rot.append((_lift_axis(bone), deg(30 if name.endswith("b") else 20) * fall))
    elif name.startswith("seg") or name == "tail":
        i = 5 if name == "tail" else int(name[3])
        rot.append((Z, deg(12) * fall * (1 + i * 0.3)))
    else:
        # horns and tops shake from the impact
        rot.append((X, deg(14) * math.sin(14 * t) * (1 - t)))
        if side:
            rot.append((Y, side * deg(10) * fall))
    return rot, loc, squash


def _pose(model, bone, rest, phase, cycle):
    """World-space (rotations, offset, squash) of one bone at phase 0..2pi."""
    if cycle == "Die":
        return _die_pose(model, bone, phase / (2 * math.pi))
    walk = cycle == "Walk"
    k = 2 if walk else 1
    name, side = bone.name, _side(bone.name)
    rot, loc, squash = [], Vector(), 1.0
    s = model.scale
    deg = math.radians

    if name == "body":
        motion = model.motion
        if motion == "hop":
            if walk:
                h = abs(math.sin(phase))
                loc.z = 0.16 * h * s
                squash = 0.92 + 0.14 * h
                rot += [(X, deg(6)), (Y, deg(6) * math.sin(phase))]
            else:
                squash = 1 + 0.035 * math.sin(phase)
                rot.append((Y, deg(2) * math.sin(phase)))
        elif motion == "float":
            loc.z = (0.06 + 0.05 * math.sin(k * phase)) * s
            if walk:
                rot += [(X, deg(8)), (Y, deg(5) * math.sin(phase))]
            else:
                rot.append((Y, deg(3) * math.sin(phase)))
        elif motion == "walk":
            if walk:
                legs = [b for b in rest if b.name.startswith("leg") and not b.name.endswith("b")]
                reach = max((b.head_local.z for b in legs), default=0.5)
                spider = any(b.name[3:4].isdigit() for b in legs)
                swing = 0.0 if spider else deg(28) * math.sin(phase)
                loc.z = (0.02 * s if spider else 0.0) - reach * (1 - math.cos(swing)) + 0.015 * s
                rot += [(X, deg(4)), (Y, deg(3) * math.sin(phase))]
            else:
                squash = 1 + 0.02 * math.sin(phase)
        elif motion == "spin":
            rot.append((Z, 2 * phase if walk else phase))
            if walk:
                loc.z = 0.05 * abs(math.sin(2 * phase)) * s
        elif motion == "slither":
            # the head only sways a little, the wave runs down the body behind it
            rot.append((Z, deg(7 if walk else 3) * math.sin(phase + 0.6)))
        return rot, loc, squash

    if name.startswith("leg"):
        if name[3:4].isdigit():
            i = int(name[3])
            if name.endswith("b"):
                p = phase + (0 if (i + (side > 0)) % 2 == 0 else math.pi)
                bend = deg(10) * max(0.0, math.cos(p)) if walk else 0.0
                return [(_lift_axis(bone), -bend)], loc, squash
            p = phase + (0 if (i + (side > 0)) % 2 == 0 else math.pi)
            if walk:
                rot += [(Z, side * deg(14) * math.sin(p)), (_lift_axis(bone), deg(18) * max(0.0, math.cos(p)))]
            else:
                rot.append((_lift_axis(bone), deg(3) * math.sin(phase + i)))
        elif walk:
            rot.append((X, side * deg(28) * math.sin(phase)))
        return rot, loc, squash

    if name.startswith("arm"):
        if walk:
            rot.append((X, -side * deg(22) * math.sin(phase)))
        rot.append((_lift_axis(bone), deg(6 if walk else 5) * math.sin(k * phase + 0.5)))
        return rot, loc, squash

    if name.startswith("ear"):
        rot.append((_lift_axis(bone), deg(16 if walk else 6) * math.sin(k * phase)))
        return rot, loc, squash

    if name.startswith("tent"):
        i = int(name[4])
        a = deg(14 if walk else 8)
        lag = 0.9 if name.endswith("b") else 0.0
        amp = 1.4 if name.endswith("b") else 1.0
        rot.append((_lift_axis(bone), amp * a * math.sin(k * phase + i * math.pi / 3 - lag)))
        return rot, loc, squash

    if name.startswith("seg") or name == "tail":
        # each joint bends a little more than the last and later, so an S-wave travels from head to tail
        i = 5 if name == "tail" else int(name[3])
        a = deg(16 if walk else 5) * (1 + 0.12 * i)
        rot.append((Z, -a * math.sin(phase - (i + 1) * 1.1)))
        return rot, loc, squash

    # horns, tops, spikes, noses and the dangling stalk trail the body's bounce
    a = deg(10 if walk else 4) * (1.6 if name == "dangle" else 1.0)
    rot.append((X, a * math.sin(k * phase - 0.8)))
    if side:
        rot.append((Y, side * a * 0.5 * math.sin(k * phase - 0.8)))
    return rot, loc, squash


def animate(model, arm):
    rest = list(arm.data.bones)
    arm.animation_data_create()
    for pb in arm.pose.bones:
        pb.rotation_mode = "QUATERNION"
    actions = []
    for cycle, frames in CYCLES.items():
        action = bpy.data.actions.new(f"{model.name}_{cycle}")
        arm.animation_data.action = action
        for f in range(frames + 1):
            phase = 2 * math.pi * f / frames
            for pb in arm.pose.bones:
                rot, loc, squash = _pose(model, pb.bone, rest, phase, cycle)
                r = pb.bone.matrix_local.to_3x3()
                q_world = Quaternion()
                for axis, angle in rot:
                    q_world = Quaternion(axis, angle) @ q_world
                q_rest = r.to_quaternion()
                pb.rotation_quaternion = q_rest.inverted() @ q_world @ q_rest
                pb.location = r.inverted() @ loc
                side = 1 / math.sqrt(squash)
                pb.scale = (side, squash, side)
                pb.keyframe_insert("rotation_quaternion", frame=f)
                if pb.name == "body":
                    pb.keyframe_insert("location", frame=f)
                    pb.keyframe_insert("scale", frame=f)
        actions.append((cycle, action))
    arm.animation_data.action = None
    for cycle, action in actions:
        track = arm.animation_data.nla_tracks.new()
        track.name = cycle.lower()
        track.strips.new(action.name, 0, action)
        track.mute = True
    for pb in arm.pose.bones:
        pb.rotation_quaternion = Quaternion()
        pb.location = (0, 0, 0)
        pb.scale = (1, 1, 1)
    return [a for _, a in actions]


def _export_material(name):
    from sdf_core import _gltf_output_group
    mat = bpy.data.materials.new(f"{name}_export")
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = next(n for n in nodes if n.type == "BSDF_PRINCIPLED")
    color = nodes.new("ShaderNodeTexImage")
    color.image = bpy.data.images[f"sdf_{name}_color"]
    links.new(color.outputs["Color"], bsdf.inputs["Base Color"])
    ao = nodes.new("ShaderNodeTexImage")
    ao.image = bpy.data.images[f"sdf_{name}_ao"]
    split = nodes.new("ShaderNodeSeparateColor")
    links.new(ao.outputs["Color"], split.inputs["Color"])
    out = nodes.new("ShaderNodeGroup")
    out.node_tree = _gltf_output_group()
    links.new(split.outputs[0], out.inputs["Occlusion"])
    return mat


def export_animated_glb(arm, low, name, out_dir):
    """Skinned .glb with one animation per cycle, centred on the ground like the static export."""
    os.makedirs(out_dir, exist_ok=True)
    co = np.array([v.co for v in low.data.vertices])
    lo, hi = co.min(axis=0), co.max(axis=0)
    shift = Vector(((lo[0] + hi[0]) / 2, (lo[1] + hi[1]) / 2, lo[2]))
    saved_loc = arm.location.copy()
    saved_mats = list(low.data.materials)
    mat = _export_material(name)
    low.data.materials.clear()
    low.data.materials.append(mat)
    arm.location = -shift
    for scene in bpy.data.scenes:
        for layer in scene.view_layers:
            for o in scene.objects:
                o.select_set(False, view_layer=layer)
    arm.select_set(True)
    low.select_set(True)
    bpy.context.view_layer.objects.active = arm
    path = f"{out_dir}/{name}.glb"
    bpy.ops.export_scene.gltf(filepath=path, export_format="GLB", use_selection=True, export_yup=True,
                              export_skins=True, export_animations=True, export_animation_mode="NLA_TRACKS",
                              export_frame_range=False)
    arm.location = saved_loc
    low.data.materials.clear()
    for m in saved_mats:
        low.data.materials.append(m)
    bpy.data.materials.remove(mat)
    return os.path.getsize(path)
