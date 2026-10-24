# The intro's bombermen: the player imported by import_hjb.py with three more actions, and the black bomberman
# riding the dragon, the same model with the black player's colours.
#   blender --background <out dir>/player.blend --python build_player.py -- <out dir>
# adds "panic" (flailing, hopping from foot to foot), "ride" (astride the dragon, a fist raised) and "look_up"
# (standing, staring after the dragon), then writes player.glb (white) and player_dark.glb (black).
# The actions run at 60 fps like the imported ones; poses are the idle's first frame with turns on top.
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bpy  # noqa: E402
import numpy as np  # noqa: E402
from mathutils import Quaternion, Vector  # noqa: E402

import toy  # noqa: E402
from toy import turn  # noqa: E402

FPS = 60
DATA = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "client", "data", "game")

UP_L = (0, -1, 0)  # turns the left arm (at +x) up, out at the side
UP_R = (0, 1, 0)
LEGS_FORWARD = (-1, 0, 0)  # swings a hanging leg forwards, bends the head back
BEND = (1, 0, 0)  # bends the body or head forwards


def base_pose(rig):
    """Every bone's location, rotation and scale in the idle's first frame."""
    rig.animation_data.action = bpy.data.actions["idle"]
    slots = getattr(bpy.data.actions["idle"], "slots", None)
    if slots:
        rig.animation_data.action_slot = slots[0]
    bpy.context.scene.frame_set(0)
    pose = {pb.name: (pb.location.copy(), pb.rotation_quaternion.copy(), pb.scale.copy()) for pb in rig.pose.bones}
    rig.animation_data.action = None
    return pose


def action(rig, name, base, keys, frames, cyclic=True):
    """keys: [(frame, {bone: (rotation or None, offset or None)})]; armature space turns and offsets on top of the
    base pose, every bone keyed at every key so nothing drifts in from elsewhere."""
    act = bpy.data.actions.new(name)
    act.use_fake_user = True
    rig.animation_data.action = act
    for frame, changes in keys:
        for pb in rig.pose.bones:
            location, rotation, scale = base[pb.name]
            rest = pb.bone.matrix_local.to_quaternion()
            turn_, offset = changes.get(pb.name, (None, None))
            if turn_ is not None:
                rotation = (rest.inverted() @ turn_ @ rest) @ rotation
            if offset is not None:
                location = location + rest.inverted() @ Vector(offset)
            pb.rotation_mode = "QUATERNION"
            pb.location, pb.rotation_quaternion, pb.scale = location, rotation, scale
            pb.keyframe_insert("location", frame=frame)
            pb.keyframe_insert("rotation_quaternion", frame=frame)
            pb.keyframe_insert("scale", frame=frame)
    if cyclic:
        for fc in toy._fcurves(act):
            fc.modifiers.new("CYCLES")
    act.frame_range = (0, frames)
    act.use_frame_range = True
    rig.animation_data.action = None
    track = rig.animation_data.nla_tracks.new()
    track.name = name
    track.strips.new(name, 0, act)
    track.mute = True


def panic(rig, base):
    # arms flapping up and down against each other, forearms flung about, hopping from foot to foot, head shaking
    def pose(left_up, right_up, hop, shake):
        return {
            "up_arm_l": (turn((UP_L, left_up)), None), "up_arm_r": (turn((UP_R, right_up)), None),
            "low_arm_l": (turn((UP_L, left_up * 0.4)), None), "low_arm_r": (turn((UP_R, right_up * 0.4)), None),
            "root": (None, (0, 0, hop)),
            "foot_l": (None, (0, 0, max(hop, 0) * 0.8)), "foot_r": (None, (0, 0, max(-hop, 0) * 0.8)),
            "CTRL_head": (turn(((0, 0, 1), shake), (LEGS_FORWARD, 12)), None),
        }
    keys = [(0, pose(120, 30, 0.02, 12)), (8, pose(75, 75, 0.0, 0)), (15, pose(30, 120, -0.02, -12)),
            (23, pose(75, 75, 0.0, 0)), (30, pose(120, 30, 0.02, 12))]
    # the hop lifts the whole body, the foot on the ground stays down
    for _, p in keys:
        hop = p["root"][1][2]
        p["foot_l"] = (None, (0, 0, -hop + (0.02 if hop > 0 else 0)))
        p["foot_r"] = (None, (0, 0, -hop + (0.02 if hop < 0 else 0)))
    action(rig, "panic", base, keys, 30)


def ride(rig, base):
    # astride the dragon's neck: legs forward and apart, the left hand holding on, the right fist up, bobbing
    def pose(fist, bob):
        legs = {}
        for side, s in (("l", 1), ("r", -1)):
            legs[f"up_leg_{side}"] = (turn((LEGS_FORWARD, 70), ((0, -s, 0), 35)), None)
            legs[f"low_leg_{side}"] = (turn((LEGS_FORWARD, 10)), None)
            legs[f"foot_{side}"] = (turn((LEGS_FORWARD, 20)), (s * 0.07, -0.09, 0.09))
        return {
            **legs,
            "root": (None, (0, 0, bob)),
            "CTRL_pelvis": (turn((BEND, -8)), None),
            "up_arm_l": (turn((UP_L, 15), ((0, 0, 1), 50)), None),
            "low_arm_l": (turn(((0, 0, 1), 30)), None),
            "up_arm_r": (turn((UP_R, 95 + fist * 0.5)), None),
            "low_arm_r": (turn((UP_R, 55 + fist)), None),
            "CTRL_head": (turn((BEND, 8)), None),
        }
    keys = [(0, pose(0, 0.0)), (15, pose(15, 0.012)), (30, pose(0, 0.0))]
    action(rig, "ride", base, keys, 30)


def look_up(rig, base):
    # standing, arms a little out, staring up after the dragon
    def pose(sway):
        return {
            "CTRL_head": (turn((LEGS_FORWARD, 30), ((0, 0, 1), sway)), None),
            "spine3": (turn((LEGS_FORWARD, 6)), None),
            "up_arm_l": (turn((UP_L, 20)), None), "up_arm_r": (turn((UP_R, 20)), None),
        }
    action(rig, "look_up", base, [(0, pose(-4)), (60, pose(4)), (120, pose(-4))], 120)


def colour_map(index):
    """player_<index>.tga with player-ao.tga's red multiplied in, like import_hjb.py bakes player_1."""
    color = bpy.data.images.load(os.path.join(DATA, f"player_{index}.tga"))
    ao = bpy.data.images.load(os.path.join(DATA, "player-ao.tga"))
    w, h = color.size
    c = np.array(color.pixels[:], dtype=np.float32).reshape(h, w, 4)
    a = np.array(ao.pixels[:], dtype=np.float32).reshape(ao.size[1], ao.size[0], 4)
    if a.shape[:2] != c.shape[:2]:
        raise SystemExit(f"player-ao.tga is {ao.size[:]}, player_{index}.tga {color.size[:]}")
    c[:, :, :3] *= a[:, :, :1]
    c[:, :, 3] = 1.0
    image = bpy.data.images.new(f"player_{index}_ao", w, h)
    image.pixels = c.ravel()
    image.pack()
    return image


def export(rig, mesh, path):
    bpy.context.scene.render.fps = FPS
    toy.export(rig, mesh, path)


out = toy.out_dir()
rig = next(o for o in bpy.data.objects if o.type == "ARMATURE")
mesh = next(o for o in bpy.data.objects if o.type == "MESH" and o.parent == rig)
rig.animation_data_create()
for name in ("panic", "ride", "look_up"):
    if name in bpy.data.actions:
        bpy.data.actions.remove(bpy.data.actions[name])
base = base_pose(rig)
panic(rig, base)
ride(rig, base)
look_up(rig, base)
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(out, "player_intro.blend"))
export(rig, mesh, os.path.join(out, "player.glb"))

texture = next(n for n in mesh.data.materials[0].node_tree.nodes if n.type == "TEX_IMAGE")
texture.image = colour_map(2)
export(rig, mesh, os.path.join(out, "player_dark.glb"))
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(out, "player_dark.blend"))
