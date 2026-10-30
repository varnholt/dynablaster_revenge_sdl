# The intro scene: the lab at night, the dragon breaks out of the dome with the princess and the black bomberman,
# the doctor and the player panic, the camera follows the dragon up to the castle on the mountain.
#   blender --background --factory-startup --python build_intro.py -- <out dir> [render <frame,...>]
# Assembles the models the build_*.py scripts wrote to <out dir>, stages them and writes intro.txt, the timeline
# the game plays: per actor its model, the actions it plays from when, its transform sampled every frame; the
# camera's; the events. With "render" it also renders those frames through the camera to <out dir>/frames.
import json
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bpy  # noqa: E402
from mathutils import Matrix, Vector  # noqa: E402

import cutscene  # noqa: E402
import toy  # noqa: E402
from cutscene import FPS, key, s  # noqa: E402

LENGTH = 15 * FPS
BURST = 3 * FPS + 1  # the frame the dome breaks
# where build_lab.py found the lab's dome and the rotunda's door in world 1's scenery
with open(os.path.join(toy.out_dir(), "lab.json"), encoding="utf-8") as _f:
    LAB = json.load(_f)
DOME_CENTRE = Vector(LAB["dome_centre"])
DOME_TOP = LAB["dome_top"]
DOOR = LAB["door"]


def ground(x, y):
    """The scenery's ground height at x, y (build_lab.py samples it in front of the lab)."""
    grid = LAB["ground"]
    u = min(max((x - grid["x0"]) / grid["step"], 0.0), grid["columns"] - 1.001)
    v = min(max((y - grid["y0"]) / grid["step"], 0.0), grid["rows"] - 1.001)
    i, j = int(u), int(v)
    fu, fv = u - i, v - j

    def at(a, b):
        return grid["heights"][b * grid["columns"] + a]

    return ((at(i, j) * (1 - fu) + at(i + 1, j) * fu) * (1 - fv)
            + (at(i, j + 1) * (1 - fu) + at(i + 1, j + 1) * fu) * fv)

# who's in it: the model, its size, the frames it's seen from and until and the actions it plays from which frame
# on, no earlier than it's seen ("hold" stops at the end)
ACTORS = {
    "lab": {"model": "lab.glb", "scale": 1.0, "actions": []},
    "backdrop": {"model": "backdrop.glb", "scale": 1.0, "actions": []},
    "dome": {"model": "dome.glb", "scale": 1.0, "actions": [(BURST, "burst", "hold")]},
    "door": {"model": "door.glb", "scale": 1.0, "actions": []},
    "lights": {"model": "lights.glb", "scale": 1.0, "actions": []},
    "dragon": {"model": "dragon.glb", "scale": 1.6, "actions": [(1, "fly", "loop")], "from": BURST,
               "until": s(13.6)},
    "princess": {"model": "princess.glb", "scale": 1.1, "actions": [(1, "dangle", "loop")],
                 "from": BURST, "until": s(13.6)},
    "rider": {"model": "player_dark.glb", "scale": 2.4, "actions": [(1, "ride", "loop")], "from": BURST,
              "until": s(13.6)},
    "player": {"model": "player.glb", "scale": 2.4,
               "actions": [(1, "run", "loop"), (s(5.2), "panic", "loop"), (s(9.0), "look_up", "loop")],
               "from": s(4.0)},
    "doctor": {"model": "doctor.glb", "scale": 1.3,
               "actions": [(1, "run", "loop"), (s(4.2), "turn", "loop"), (s(4.6), "fall", "hold"),
                           (s(4.6) + 24, "cower", "loop")],
               "from": s(3.1)},
}


def stage(actors):
    """Where everyone is when."""
    dragon = actors["dragon"]["holder"]
    # up through the hole in the dome, a hover over it, then off over the lab towards the castle on the mountain
    cx, cy, top = DOME_CENTRE.x, DOME_CENTRE.y, DOME_TOP
    path = [
        (BURST, (cx, cy - 0.6, DOME_CENTRE.z - 0.4), (0, 0, 0)),
        (BURST + 6, (cx, cy - 1.2, top + 0.5), (-10, 0, 0)),
        (s(4.4), (cx - 0.1, cy - 1.6, top + 1.1), (0, 0, 0)),
        (s(5.6), (cx + 0.3, cy - 1.6, top + 1.4), (0, 4, -6)),
        (s(6.8), (cx - 0.3, cy - 1.2, top + 1.7), (0, -4, 6)),
        (s(8.0), (cx, cy + 0.4, top + 4.9), (8, 0, 180)),
        (s(10.0), (cx, 30.0, top + 15.0), (12, 0, 180)),
        (s(12.5), (6.5, 86.0, 36.0), (6, 0, 180)),
        (s(13.6), (6.5, 100.8, 37.5), (0, 0, 180)),
    ]
    for frame, location, rotation in path:
        key(dragon, frame, location, rotation)

    # the black bomberman up on its head, the princess by the shoulders in its claws: children of the dragon's
    # head and body bones, placed (feet on the origin) in the dragon's own units while it's in its rest pose
    rig = actors["dragon"]["rig"]
    bpy.context.view_layer.update()
    for name, offset, bone in (("rider", (0.0, 0.15, 1.58), "head"), ("princess", (0.0, -0.5, 0.04), "body")):
        holder = actors[name]["holder"]
        # turned as the dragon is, so they face where its head points (whichever way it's turned just now)
        turned = rig.matrix_world.to_quaternion().to_matrix().to_4x4()
        world = Matrix.Translation(rig.matrix_world @ Vector(offset)) @ turned @ Matrix.Diagonal(
            (ACTORS[name]["scale"],) * 3 + (1,))
        holder.parent = rig
        holder.parent_type = "BONE"
        holder.parent_bone = bone
        bpy.context.view_layer.update()
        holder.matrix_world = world

    # the player runs in from the left, stops in front of the dome and turns to it
    player = actors["player"]["holder"]
    key(player, s(4.0), (-4.0, 0.6, ground(-4.0, 0.6)), (0, 0, -90))
    key(player, s(5.0), (3.6, 0.8, ground(3.6, 0.8)), (0, 0, -90))
    key(player, s(5.3), (3.7, 0.8, ground(3.7, 0.8)), (0, 0, 180))

    # the doctor runs out of the rotunda's door under the dome (build_lab.py), from the back of the hall behind it,
    # hidden by the door until he flings it open; down the steps and away to the right, then he stops, shuffles
    # round to the dome, sees the dragon above it and falls back onto his bottom, facing it, where he stays. The
    # door swings out to the right, clear of his path.
    door = actors["door"]["holder"]
    hinge = tuple(DOOR["hinge"])
    key(door, 1, hinge, (0, 0, 0))
    key(door, s(3.1) + 4, hinge, (0, 0, 0))
    key(door, s(3.1) + 8, rotation=(0, 0, 88))
    key(door, s(3.1) + 13, rotation=(0, 0, 100))
    doctor = actors["doctor"]["holder"]
    x, face, floor = DOOR["x"], DOOR["face"], DOOR["bottom"]
    foot = face - DOOR["steps"] - 0.3
    stop = (x + 2.3, foot - 1.0)

    def facing(dx, dy):
        # rotation 0 faces -y, out of the door
        return math.degrees(math.atan2(dx, -dy))

    key(doctor, s(3.1), (x, face + 0.75, floor), (0, 0, 0))
    key(doctor, s(3.35), (x, face - 0.3, floor), (0, 0, 0))
    key(doctor, s(3.65), (x + 0.3, foot, ground(x + 0.3, foot)), (0, 0, 10))
    key(doctor, s(4.2), (*stop, ground(*stop)), (0, 0, facing(stop[0] - x - 0.3, stop[1] - foot)))
    key(doctor, s(4.6), (*stop, ground(*stop)), (0, 0, facing(DOME_CENTRE.x - stop[0], DOME_CENTRE.y - stop[1])))


def camera():
    cam = bpy.data.objects.new("camera", bpy.data.cameras.new("camera"))
    bpy.context.scene.collection.objects.link(cam)
    bpy.context.scene.camera = cam
    cam.data.lens_unit = "FOV"
    cam.data.clip_end = 400
    def aim(frame, eye, target, fov=50):
        cam.location = eye
        cam.rotation_euler = (Vector(target) - Vector(eye)).to_track_quat("-Z", "Y").to_euler()
        cam.data.angle = math.radians(fov)
        cam.keyframe_insert("location", frame=frame)
        cam.keyframe_insert("rotation_euler", frame=frame)
        cam.data.keyframe_insert("lens", frame=frame)

    # from well behind the big tree south of the field, the tree seen whole against the lit lab first, then through
    # its crown (world 1 plants it for this, see world_meadow(): on ground levelled to 0.16, its crown about 2.3 to
    # 4.2 high round x 5.6, y -19.6) out into the view: wide on the lab, pushing in; tilting up after the dragon;
    # craning up and over to the castle
    # the lab's dome stands up to DOME_TOP, some 3 higher than the old lab's glass dome these heights were set for;
    # out of the tree the camera rises a little, so the dome's top stays below the castle on the mountain behind
    lift = DOME_TOP - 4.14
    aim(1, (6.5, -30.0, 3.3), (6.5, 5.0, 3.6 + lift * 0.3), 50)
    aim(s(1.6), (6.5, -18.6, 2.6), (6.5, 5.0, 3.6 + lift * 0.4), 50)
    # wide enough for the player and the doctor down on the ground and the dragon hovering over the dome at once
    aim(BURST, (6.5, -17.0, 3.0), (6.5, 5.0, 4.8), 58)
    aim(s(6.0), (6.5, -17.0, 3.2), (6.5, 5.0, 5.2), 58)
    aim(s(8.0), (6.5, -17.0, 4.2), (6.5, 6.0, 7.5 + lift), 50)
    aim(s(11.0), (6.5, -15.0, 6.0), (6.5, 70.0, 30.0), 46)
    aim(s(15.0), (6.5, -13.0, 7.0), (6.5, 100.0, 34.0), 38)
    return cam


cutscene.build(toy.out_dir(), "intro", ACTORS, LENGTH, stage, camera)
