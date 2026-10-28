# The cutscenes' bombermen: the player imported by import_hjb.py with more actions, and the black bomberman riding
# the dragon and crying at the end, the same model with the black player's colours.
#   blender --background <out dir>/player.blend --python build_player.py -- <out dir>
# adds "panic" (flailing, hopping from foot to foot), "ride" (astride the dragon, a fist raised), "look_up"
# (standing, staring after the dragon), "sit" (sinking down onto his bottom, the hands going to the face, once) and
# "cry" (sitting, sobbing into his hands), then writes player.glb (white) and player_dark.glb (black).
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


# the antenna's ball swings after the head; at 60 frames a second a softer spring than the 24 frame models'
BOBBLE = [("bobble1", 0.1, 0.86), ("bobble2", 0.1, 0.86), ("bobble3", 0.1, 0.86)]
UP = (0, 0, 1)  # turns about the vertical
STEP = 3  # frames between the keys sampled from the poses' curves


def wave(frame, frames, lag=0.0):
    """-1..1 round a loop of frames, lag (in loops) behind."""
    return math.sin(2 * math.pi * (frame / frames - lag))


def sampled(rig, name, base, frames, pose):
    """An action from pose(frame) keyed every STEP frames round the loop: the limbs move in arcs and the parts
    further out lag behind the ones they hang on, the antenna swings after the head."""
    keys = [(frame, pose(frame)) for frame in range(0, frames + 1, STEP)]
    action(rig, name, base, keys, frames)
    act = bpy.data.actions[name]
    for bone, stiffness, damping in BOBBLE:
        rig.animation_data.action = act
        toy.follow_through(rig, act, bone, stiffness, damping, True)
    rig.animation_data.action = None


def panic(rig, base):
    """Flailing: the arms flap up and down against each other, the forearms whipping a beat behind, hopping from
    foot to foot, the body twisting and the head shaking after it; half a second a loop."""
    frames = 30

    def pose(f):
        flap = wave(f, frames)
        hop = abs(wave(f, frames, 0.1)) * 0.025
        left = 75 + 45 * flap
        right = 75 - 45 * flap
        p = {
            "up_arm_l": (turn((UP_L, left)), None), "up_arm_r": (turn((UP_R, right)), None),
            "low_arm_l": (turn((UP_L, 35 * wave(f, frames, 0.15))), None),
            "low_arm_r": (turn((UP_R, -35 * wave(f, frames, 0.15))), None),
            "hand_l": (turn((UP_L, 25 * wave(f, frames, 0.25))), None),
            "hand_r": (turn((UP_R, -25 * wave(f, frames, 0.25))), None),
            "root": (None, (0, 0, hop)),
            "spine2": (turn((UP, 8 * flap)), None),
            "CTRL_head": (turn((UP, -14 * wave(f, frames, 0.2)), (LEGS_FORWARD, 12)), None),
        }
        # the hop lifts the whole body, the foot on the ground stays down
        lead = wave(f, frames, 0.1) > 0
        p["foot_l"] = (None, (0, 0, -hop + (0.02 if lead else 0)))
        p["foot_r"] = (None, (0, 0, -hop + (0.02 if not lead else 0)))
        return p
    sampled(rig, "panic", base, frames, pose)


def ride(rig, base):
    """Astride the dragon's neck: legs forward and apart, the left hand holding on, the right fist pumping; the
    body rides the wingbeat (the dragon's 16 frame flap at 24 frames a second is 40 frames here), sinking as the
    dragon is lifted, the fist and head a beat behind."""
    frames = 40

    def pose(f):
        bob = wave(f, frames, 0.25)
        pump = wave(f, frames, 0.4)
        legs = {}
        for side, s in (("l", 1), ("r", -1)):
            legs[f"up_leg_{side}"] = (turn((LEGS_FORWARD, 70), ((0, -s, 0), 35)), None)
            legs[f"low_leg_{side}"] = (turn((LEGS_FORWARD, 10 + 4 * bob)), None)
            legs[f"foot_{side}"] = (turn((LEGS_FORWARD, 20)), (s * 0.07, -0.09, 0.09))
        return {
            **legs,
            "root": (None, (0, 0, -0.012 * bob)),
            "CTRL_pelvis": (turn((BEND, -8)), None),
            "spine2": (turn((BEND, 4 * bob)), None),
            "up_arm_l": (turn((UP_L, 15), ((0, 0, 1), 50)), None),
            "low_arm_l": (turn(((0, 0, 1), 30)), None),
            "up_arm_r": (turn((UP_R, 100 + 12 * pump)), None),
            "low_arm_r": (turn((UP_R, 60 + 25 * pump)), None),
            "hand_r": (turn((UP_R, 10 * wave(f, frames, 0.5))), None),
            "CTRL_head": (turn((BEND, 8 - 6 * wave(f, frames, 0.35))), None),
        }
    sampled(rig, "ride", base, frames, pose)


def look_up(rig, base):
    """Standing, staring up after the dragon: breathing, the weight shifting, the head following it a little from
    side to side, the arms hanging slightly out; two seconds a loop."""
    frames = 120

    def pose(f):
        breath = wave(f, frames / 2)
        sway = wave(f, frames)
        return {
            "CTRL_head": (turn((LEGS_FORWARD, 30 + 2 * breath), ((0, 0, 1), 5 * wave(f, frames, 0.1))), None),
            "spine3": (turn((LEGS_FORWARD, 6 + 1.5 * breath)), None),
            "CTRL_pelvis": (turn(((0, 1, 0), 2 * sway)), (0.004 * sway, 0, 0)),
            "up_arm_l": (turn((UP_L, 20 + 3 * breath)), None), "up_arm_r": (turn((UP_R, 20 + 3 * breath)), None),
            "low_arm_l": (turn((UP_L, 6)), None), "low_arm_r": (turn((UP_R, 6)), None),
        }
    sampled(rig, "look_up", base, frames, pose)


SIT_DROP = 0.13  # how far the body sinks sitting down: the hips (0.18 up) end up just above the floor
FOOT_LIFT = 0.05  # the feet tipped onto their heels would reach below the floor
ARM_LIFT, ARM_IN = 80, 30  # the upper arms forwards and in towards the face
FOREARM, FOREARM_IN = 50, 22  # the forearms up and in, the fists ending just in front of the visor


def sitting(sob=0.0, rub=0.0, depth=1.0):
    """On his bottom, legs out in front, hunched over with the hands at his eyes; sob shakes the shoulders and
    drops the head, rub moves the hands over the eyes; depth 0..1 from standing to sitting."""
    drop = SIT_DROP * depth
    pose = {
        "root": (None, (0, 0, -drop)),
        "CTRL_pelvis": (turn((BEND, -10 * depth)), None),
        "spine2": (turn((BEND, (18 + 4 * sob) * depth)), None),
        "spine3": (turn((BEND, (10 + 5 * sob) * depth)), None),
        "CTRL_head": (turn((BEND, (22 + 8 * sob) * depth)), None),
        # the hanging arms swing forwards and up, the forearms bend in to the eyes
        # (the fists stay in front of the visor, not in the helmet)
        "up_arm_l": (turn((LEGS_FORWARD, ARM_LIFT * depth), ((0, 0, 1), (-ARM_IN + 4 * rub) * depth)), None),
        "up_arm_r": (turn((LEGS_FORWARD, ARM_LIFT * depth), ((0, 0, 1), (ARM_IN + 4 * rub) * depth)), None),
        "low_arm_l": (turn((LEGS_FORWARD, (FOREARM + 8 * rub) * depth), ((0, 0, 1), -FOREARM_IN * depth)), None),
        "low_arm_r": (turn((LEGS_FORWARD, (FOREARM - 8 * rub) * depth), ((0, 0, 1), FOREARM_IN * depth)), None),
    }
    for side, s in (("l", 1), ("r", -1)):
        pose[f"up_leg_{side}"] = (turn((LEGS_FORWARD, 75 * depth), ((0, -s, 0), 12 * depth)), None)
        pose[f"low_leg_{side}"] = (turn((LEGS_FORWARD, -15 * depth)), None)
        # the feet hang off the pelvis: back down onto the floor, out in front
        pose[f"foot_{side}"] = (turn((LEGS_FORWARD, 25 * depth)), (s * 0.015 * depth, -0.08 * depth, drop + FOOT_LIFT * depth))
    return pose


def sit(rig, base):
    """Giving up: the knees buckle, he drops onto his bottom with a little bounce and his hands go to his face;
    once, 40 frames, ending in cry's first pose."""
    keys = [(0, sitting(depth=0.0)), (10, sitting(depth=0.35)), (20, sitting(depth=1.08)), (26, sitting(depth=0.97)),
            (40, sitting())]
    action(rig, "sit", base, keys, 40, cyclic=False)


def cry(rig, base):
    """Sitting and crying into his hands: sobs shake his shoulders in bursts, the hands rub his eyes, the antenna
    trembles after the head; two seconds a loop."""
    frames = 120

    def pose(f):
        burst = max(0.0, wave(f, frames)) ** 0.5
        sob = burst * abs(wave(f, 20))
        rub = wave(f, 40, 0.2)
        return sitting(sob=sob, rub=rub)
    sampled(rig, "cry", base, frames, pose)


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
for name in ("panic", "ride", "look_up", "sit", "cry"):
    if name in bpy.data.actions:
        bpy.data.actions.remove(bpy.data.actions[name])
base = base_pose(rig)
panic(rig, base)
ride(rig, base)
look_up(rig, base)
sit(rig, base)
cry(rig, base)
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(out, "player_intro.blend"))
export(rig, mesh, os.path.join(out, "player.glb"))

texture = next(n for n in mesh.data.materials[0].node_tree.nodes if n.type == "TEX_IMAGE")
texture.image = colour_map(2)
export(rig, mesh, os.path.join(out, "player_dark.glb"))
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(out, "player_dark.blend"))
