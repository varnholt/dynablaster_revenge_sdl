# The black bomberman's tears in the ending, cartoon style: in each sob a fountain of big fat drops gushes out of
# both eyes in arcs, splatting on the floor either side of him; between the sobs a few drops dribble off his visor.
#   blender --background --factory-startup --python build_tears.py -- <out dir>
# Writes tears.glb: a drop per bone, "fall" plays them round a two second loop, the same as build_player.py's "cry"
# and started with it, so the gushes come with its sobs. Built in the player's own units and placed with him
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
EYES = [(0.075, -0.2, 0.25), (-0.075, -0.2, 0.25)]
RADIUS = 0.04  # a drop, far bigger than a tear
GUSH = 22  # drops from each eye in a sob
SOB = 0.95  # seconds the sob gushes, from the start of the loop like cry's burst
DRIBBLE = [1.15, 1.45, 1.75]  # when the drops between the sobs leave
SPEED = (0.75, -0.18, 0.5)  # a gushing drop: out to the side (x mirrored per eye), forwards, up; per second
GRAVITY = 1.6  # in the player's units, about three times his height a second squared
FLOOR = 0.0
SPLAT = 0.12  # seconds a drop lies flattened on the floor before it's gone


def drop():
    """A tear: round at the bottom, drawn up into a point on top."""
    bm = toy.sphere(RADIUS, segments=14, rings=10)
    for v in bm.verts:
        if v.co.z > 0:
            v.co.z *= 1.0 + 1.6 * (v.co.z / RADIUS)
            v.co.xy *= 1.0 - 0.5 * (v.co.z / (RADIUS * 2.6))
    return bm


def flight(age, eye, side, push, size):
    """Where a drop age seconds after it left the eye is, which way it's going, its size and how far it's stretched
    along the way, None once it's gone. push scales its speed: the sob's strength, or nearly nothing for a dribble."""
    x, y, z = eye
    vx, vy, vz = SPEED[0] * side * push, SPEED[1] * push, SPEED[2] * push
    if age < 0.04:
        # bursting out
        return Vector((x, y, z)), Vector((vx, vy, vz)), size * (0.4 + 15 * age), 1.0
    # when it hits the floor: z + vz t - g t^2 = FLOOR
    land = (vz + math.sqrt(vz * vz + 4 * GRAVITY * (z - FLOOR))) / (2 * GRAVITY)
    if age < land:
        position = Vector((x + vx * age, y + vy * age, z + vz * age - GRAVITY * age * age))
        velocity = Vector((vx, vy, vz - 2 * GRAVITY * age))
        return position, velocity, size, 1.0 + 0.4 * min(velocity.length, 1.0)
    if age < land + SPLAT:
        # a splat: flat and wide
        position = Vector((x + vx * land, y + vy * land, FLOOR + 0.005))
        return position, Vector((0, 0, -1)), size * 1.7, 0.12
    return None


def drops():
    """(eye, side, when it leaves, push, size) of every drop."""
    for eye in EYES:
        side = 1 if eye[0] > 0 else -1
        for i in range(GUSH):
            leave = SOB * i / GUSH
            # the sob swells and ebbs; the drops spread a little so the stream isn't one line
            strength = math.sin(math.pi * (i + 0.5) / GUSH) ** 0.5
            jitter = 0.85 + 0.3 * ((i * 7) % 5) / 4
            yield eye, side, leave, (0.55 + 0.45 * strength) * jitter, 0.8 + 0.35 * strength
        for leave in DRIBBLE:
            yield eye, side, leave, 0.12, 0.6


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
    for name, eye, side, leave, push, size in bones:
        pose_bone = rig.pose.bones[name]
        pose_bone.rotation_mode = "QUATERNION"
        rest = pose_bone.bone.matrix_local.to_quaternion()
        previous = None
        for frame in range(FRAMES + 1):
            age = (frame / FPS - leave) % LOOP
            state = flight(age, eye, side, push, size)
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
