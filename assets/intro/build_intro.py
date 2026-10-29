# The intro scene: the lab at night, the dragon breaks out of the dome with the princess and the black bomberman,
# the doctor and the player panic, the camera follows the dragon up to the castle on the mountain.
#   blender --background --factory-startup --python build_intro.py -- <out dir> [render <frame,...>]
# Assembles the models the build_*.py scripts wrote to <out dir>, stages them and writes intro.txt, the timeline
# the game plays: per actor its model, the actions it plays from when, its transform sampled every frame; the
# camera's; the events. With "render" it also renders those frames through the camera to <out dir>/frames.
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
DOME_CENTRE = Vector((6.5, 5.6, 0.3))
DOOR_HINGE = (11.44, 5.825, 0.28)  # build_lab.py's, the door right of the dome

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
    # up through the hole in the dome, a hover, then off over the lab towards the castle on the mountain
    path = [
        (BURST, (6.5, 5.0, 0.5), (0, 0, 0)),
        (BURST + 6, (6.5, 4.4, 4.6), (-10, 0, 0)),
        (s(4.4), (6.4, 4.0, 6.6), (0, 0, 0)),
        (s(5.6), (6.8, 4.0, 7.1), (0, 4, -6)),
        (s(6.8), (6.2, 4.4, 7.4), (0, -4, 6)),
        (s(8.0), (6.5, 6.0, 9.0), (8, 0, 180)),
        (s(10.0), (6.5, 30.0, 18.0), (12, 0, 180)),
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
    key(player, s(4.0), (-4.0, 0.6, 0.27), (0, 0, -90))
    key(player, s(5.0), (3.6, 0.8, 0.27), (0, 0, -90))
    key(player, s(5.3), (3.7, 0.8, 0.27), (0, 0, 180))

    # the doctor runs out of the door right of the dome (build_lab.py), from the back of the hall behind it,
    # hidden by the door until he flings it open; out and away to the right, then he stops, shuffles round to the
    # dome, sees the dragon above it and falls back onto his bottom, facing it, where he stays. The door swings out
    # to the right, clear of his path and of the red lamp beside it.
    door = actors["door"]["holder"]
    key(door, 1, DOOR_HINGE, (0, 0, 0))
    key(door, s(3.1) + 4, DOOR_HINGE, (0, 0, 0))
    key(door, s(3.1) + 8, rotation=(0, 0, 88))
    key(door, s(3.1) + 13, rotation=(0, 0, 100))
    doctor = actors["doctor"]["holder"]
    # rotation 0 faces -y, out of the door; he faces the way he runs, and the dome where he turns to
    key(doctor, s(3.1), (10.88, 6.95, 0.27), (0, 0, 0))
    key(doctor, s(3.75), (10.88, 5.05, 0.27), (0, 0, 0))
    key(doctor, s(4.2), (11.6, 3.2, 0.27), (0, 0, 21))
    key(doctor, s(4.6), (11.6, 3.2, 0.27), (0, 0, -103))


def camera():
    cam = bpy.data.objects.new("camera", bpy.data.cameras.new("camera"))
    bpy.context.scene.collection.objects.link(cam)
    bpy.context.scene.camera = cam
    cam.data.lens_unit = "FOV"
    cam.data.clip_end = 400
    look = Vector((6.5, 5.0, 2.2))

    def aim(frame, eye, target, fov=50):
        cam.location = eye
        cam.rotation_euler = (Vector(target) - Vector(eye)).to_track_quat("-Z", "Y").to_euler()
        cam.data.angle = math.radians(fov)
        cam.keyframe_insert("location", frame=frame)
        cam.keyframe_insert("rotation_euler", frame=frame)
        cam.data.keyframe_insert("lens", frame=frame)

    # through the crown of the big tree south of the field (world 1 plants it for this, see world_meadow(): on
    # ground levelled to 0.16, its crown about 2.3 to 4.2 high round x 5.6, y -19.6) out into the view: wide on the
    # lab, pushing in; tilting up after the dragon; craning up and over to the castle
    aim(1, (6.5, -23.0, 3.0), (6.5, 5.0, 3.6), 50)
    aim(s(1.6), (6.5, -18.6, 2.2), (6.5, 5.0, 3.6), 50)
    aim(BURST, (6.5, -17.0, 1.8), (6.5, 5.0, 3.8), 50)
    aim(s(6.0), (6.5, -17.0, 1.9), (6.5, 5.0, 5.2), 50)
    aim(s(8.0), (6.5, -17.0, 2.2), (6.5, 6.0, 7.5), 50)
    aim(s(11.0), (6.5, -15.0, 6.0), (6.5, 70.0, 30.0), 46)
    aim(s(15.0), (6.5, -13.0, 7.0), (6.5, 100.0, 34.0), 38)
    return cam


cutscene.build(toy.out_dir(), "intro", ACTORS, LENGTH, stage, camera)
