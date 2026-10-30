# The ending after the original's: in the castle's machine room the player runs after the black bomberman, who
# flees to the far end and turns; the princess comes running in through the doorway past him to the player and
# tells him all about it, the player jumps for joy, and the black bomberman sits down and cries his eyes out (his
# tears from build_tears.py). THE END.
#   blender --background --factory-startup --python build_ending.py -- <out dir> [render <frame,...>]
# Assembles the models the build_*.py scripts wrote to <out dir>, stages them and writes ending.txt (see
# cutscene.py). Rotation 0 faces the camera (-y), 90 faces right (+x), -90 left.
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bpy  # noqa: E402
from mathutils import Vector  # noqa: E402

import cutscene  # noqa: E402
import toy  # noqa: E402
from cutscene import FPS, key, s  # noqa: E402

LENGTH = 16 * FPS
SIT = 6.4  # when the black bomberman gives up
SIT_LENGTH = 40 / 60  # build_player.py's "sit", 40 frames at 60 a second
WIN_LENGTH = 103 / 60  # the player's "win" from the game

ACTORS = {
    "room": {"model": "room.glb", "scale": 1.0, "actions": []},
    "rival": {"model": "player_dark.glb", "scale": 2.4,
              "actions": [(1, "run", "loop"), (s(3.4), "idle", "loop"), (s(SIT), "sit", "hold"),
                          (s(SIT + SIT_LENGTH), "cry", "loop")]},
    # hung on the rival in stage(), so it has his scale already
    "tears": {"model": "tears.glb", "scale": 1.0, "actions": [(s(SIT + SIT_LENGTH), "fall", "loop")],
              "from": s(SIT + SIT_LENGTH)},
    "player": {"model": "player.glb", "scale": 2.4,
               "actions": [(1, "run", "loop"), (s(3.6), "idle", "loop"), (s(6.0), "win", "hold"),
                           (s(6.0 + WIN_LENGTH), "idle", "loop")],
               "from": s(0.6)},
    "princess": {"model": "princess.glb", "scale": 1.1,
                 "actions": [(1, "run", "loop"), (s(5.6), "talk", "loop")], "from": s(3.6)},
}

CAPTIONS = [(s(12.5), LENGTH + 1, "THE END")]


def stage(actors):
    """Where everyone is when."""
    rival = actors["rival"]["holder"]
    key(rival, 1, (-8.0, -0.15, 0.0), (0, 0, 90))
    key(rival, s(3.4), (3.55, -0.15, 0.0), (0, 0, 90))
    key(rival, s(3.8), rotation=(0, 0, -90))
    key(rival, s(SIT - 0.3), rotation=(0, 0, -90))
    key(rival, s(SIT), (3.55, -0.15, 0.0), (0, 0, -25))
    actors["tears"]["holder"].parent = rival

    player = actors["player"]["holder"]
    key(player, s(0.6), (-8.5, -0.05, 0.0), (0, 0, 90))
    key(player, s(3.6), (0.6, -0.05, 0.0), (0, 0, 90))
    key(player, s(9.0), rotation=(0, 0, 90))
    key(player, s(10.0), rotation=(0, 0, 55))

    princess = actors["princess"]["holder"]
    key(princess, s(3.6), (7.8, -0.25, 0.0), (0, 0, -90))
    key(princess, s(5.6), (1.6, -0.25, 0.0), (0, 0, -90))
    key(princess, s(9.0), rotation=(0, 0, -90))
    key(princess, s(10.0), rotation=(0, 0, -55))


def camera():
    cam = bpy.data.objects.new("camera", bpy.data.cameras.new("camera"))
    bpy.context.scene.collection.objects.link(cam)
    bpy.context.scene.camera = cam
    cam.data.lens_unit = "FOV"
    cam.data.clip_end = 400

    def aim(frame, eye, target, fov):
        cam.location = eye
        cam.rotation_euler = (Vector(target) - Vector(eye)).to_track_quat("-Z", "Y").to_euler()
        cam.data.angle = math.radians(fov)
        cam.keyframe_insert("location", frame=frame)
        cam.keyframe_insert("rotation_euler", frame=frame)
        cam.data.keyframe_insert("lens", frame=frame)

    # side on like the original, the whole room; then closing in on the three of them
    aim(1, (0.0, -12.5, 2.6), (0.0, 0.5, 2.3), 60)
    aim(s(6.0), (0.4, -12.0, 2.5), (0.4, 0.5, 2.1), 58)
    aim(s(12.0), (2.2, -7.5, 1.9), (2.4, 0.5, 1.0), 50)
    aim(s(16.0), (2.4, -7.0, 1.9), (2.4, 0.5, 1.0), 50)
    return cam


cutscene.build(toy.out_dir(), "ending", ACTORS, LENGTH, stage, camera, CAPTIONS, indoors=True)
