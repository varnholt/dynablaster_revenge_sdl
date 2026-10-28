# The black bomberman's tears in the ending: in each sob a couple of drops well up under his visor at both eyes, run
# down it, drip off and fall to the floor with a little splash.
#   blender --background --factory-startup --python build_tears.py -- <out dir>
# Writes tears.glb: a drop per bone, "fall" plays them round a two second loop, the same as build_player.py's "cry"
# and started with it, so the drops come with its sobs. Built in the player's own units and placed with him
# (build_ending.py hangs it on the black bomberman), so EYES are where his eyes are in "cry", hunched over sitting on
# the floor.
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bpy  # noqa: E402
from mathutils import Quaternion, Vector  # noqa: E402

import toy  # noqa: E402

FPS = 60
FRAMES = 120  # the loop, cry's
LOOP = FRAMES / FPS
EYES = [(0.035, -0.21, 0.205), (-0.035, -0.21, 0.205)]  # the visor's eye slits
RADIUS = 0.013  # a drop, a small cartoon tear
LEAVE = [0.1, 0.6]  # when each eye's drops well up, in the sob (cry's burst)
WELL = 0.18  # seconds a drop takes to well up
RUN = 0.22  # seconds it runs down the visor
RUN_DOWN = (0.008, -0.012, -0.04)  # how far: a little out to the side (x mirrored per eye), forwards, down
DRIFT = (0.12, -0.05)  # falling: out to the side past his knees, forwards; per second
GRAVITY = 1.6  # in the player's units, about three times his height a second squared
FLOOR = 0.0
SPLAT = 0.1  # seconds a drop lies flattened on the floor before it's gone


def drop():
    """A tear: round at the bottom, drawn up into a point on top."""
    bm = toy.sphere(RADIUS, segments=12, rings=8)
    for v in bm.verts:
        if v.co.z > 0:
            v.co.z *= 1.0 + 1.6 * (v.co.z / RADIUS)
            v.co.xy *= 1.0 - 0.5 * (v.co.z / (RADIUS * 2.6))
    return bm


def flight(age, eye, side):
    """Where a drop age seconds after it started welling up is, which way it's going, its size and how far it's
    stretched along the way, None once it's gone."""
    x, y, z = eye
    down = Vector((0, 0, -1))
    if age < WELL:
        return Vector((x, y, z)), down, 0.3 + 0.7 * age / WELL, 1.0
    age -= WELL
    run = Vector((RUN_DOWN[0] * side, RUN_DOWN[1], RUN_DOWN[2]))
    if age < RUN:
        # slow at first, then letting go
        return Vector((x, y, z)) + run * (age / RUN) ** 2, down, 1.0, 1.15
    age -= RUN
    x, y, z = Vector((x, y, z)) + run
    vx, vy = DRIFT[0] * side, DRIFT[1]
    land = math.sqrt((z - FLOOR) / GRAVITY)
    if age < land:
        position = Vector((x + vx * age, y + vy * age, z - GRAVITY * age * age))
        velocity = Vector((vx, vy, -2 * GRAVITY * age))
        return position, velocity, 1.0, 1.0 + 0.3 * min(velocity.length, 1.0)
    if age < land + SPLAT:
        # a little splash: flat and wide
        position = Vector((x + vx * land, y + vy * land, FLOOR + 0.004))
        return position, down, 1.5, 0.15
    return None


def drops():
    """(eye, side, when it starts welling up) of every drop."""
    for eye in EYES:
        side = 1 if eye[0] > 0 else -1
        for k, leave in enumerate(LEAVE):
            # the eyes not quite in step
            yield eye, side, leave + (0.1 if side < 0 else 0.0) + 0.03 * k


def build():
    toy.reset()
    palette = toy.Palette("tears", {"tear": (0.55, 0.85, 1.0)})
    builder = toy.Builder("tears", palette)
    bones = []
    for n, spec in enumerate(drops()):
        name = f"tear_{n}"
        builder.bone(name, (0, 0, 0), (0, 0, RADIUS))
        builder.add(drop(), "tear", bone=name)
        bones.append((name, *spec))
    rig, mesh = builder.build()
    palette.material.node_tree.nodes["Principled BSDF"].inputs["Roughness"].default_value = 0.15

    act = bpy.data.actions.new("fall")
    act.use_fake_user = True
    rig.animation_data_create()
    rig.animation_data.action = act
    for name, eye, side, leave in bones:
        pose_bone = rig.pose.bones[name]
        pose_bone.rotation_mode = "QUATERNION"
        rest = pose_bone.bone.matrix_local.to_quaternion()
        previous = None
        for frame in range(FRAMES + 1):
            age = (frame / FPS - leave) % LOOP
            state = flight(age, eye, side)
            if state is None:
                # gone: back inside the head, too small to see
                position, velocity, scale, stretch = Vector(eye) + Vector((0, 0.05, 0)), Vector((0, 0, -1)), 0.0, 1.0
            else:
                position, velocity, scale, stretch = state
            # the point trails behind: the drop's up (the bone's y) turned against the way it goes
            turn = Vector((0, 0, 1)).rotation_difference(-velocity) if velocity.length > 1e-6 else Quaternion()
            q = rest.inverted() @ turn @ rest
            if previous is not None and previous.dot(q) < 0:
                q.negate()
            previous = q
            pose_bone.location = rest.inverted() @ position
            pose_bone.rotation_quaternion = q
            pose_bone.scale = (scale, scale * stretch, scale)
            pose_bone.keyframe_insert("location", frame=frame)
            pose_bone.keyframe_insert("rotation_quaternion", frame=frame)
            pose_bone.keyframe_insert("scale", frame=frame)
    for fc in toy._fcurves(act):
        for point in fc.keyframe_points:
            point.interpolation = "LINEAR"
    act.frame_range = (0, FRAMES)
    act.use_frame_range = True
    for pose_bone in rig.pose.bones:
        pose_bone.location, pose_bone.rotation_quaternion, pose_bone.scale = (0, 0, 0), (1, 0, 0, 0), (1, 1, 1)
    track = rig.animation_data.nla_tracks.new()
    track.name = "fall"
    track.strips.new("fall", 0, act)
    track.mute = True
    rig.animation_data.action = None
    bpy.context.scene.render.fps = FPS
    toy.export(rig, mesh, os.path.join(toy.out_dir(), "tears.glb"))


build()
