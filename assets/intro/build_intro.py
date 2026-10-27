# The intro scene: the lab at night, the dragon breaks out of the dome with the princess and the black bomberman,
# the doctor and the player panic, the camera follows the dragon up to the castle on the mountain.
#   blender --background --factory-startup --python build_intro.py -- <out dir> [render <frame,...>]
# Assembles the models the build_*.py scripts wrote to <out dir>, stages them and writes intro.txt, the timeline
# the game plays: per actor its model, the actions it plays from when, its transform sampled every frame; the
# camera's; the events. With "render" it also renders those frames through the camera to <out dir>/frames.
import math
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bpy  # noqa: E402
from mathutils import Euler, Matrix, Vector  # noqa: E402

import toy  # noqa: E402

FPS = 24
LENGTH = 15 * FPS
BURST = 3 * FPS + 1  # the frame the dome breaks
DOME_CENTRE = Vector((6.5, 5.6, 0.3))
DOOR_HINGE = (11.44, 5.825, 0.28)  # build_lab.py's, the door right of the dome

ARGS = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
RENDER = [int(f) for f in ARGS[2].split(",")] if len(ARGS) > 2 and ARGS[1] == "render" else []


def s(seconds):
    """Seconds to the frame they start."""
    return 1 + round(seconds * FPS)


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
               "actions": [(1, "run", "loop"), (s(4.6), "fall", "hold"), (s(6.6), "panic", "loop"),
                           (s(9.4), "stand", "loop")],
               "from": s(3.1)},
}


def load(out, name, spec):
    path = os.path.join(out, spec["model"])
    if not os.path.exists(path):
        print("RESULT missing", spec["model"])
        return None
    before = set(bpy.data.objects)
    actions_before = set(bpy.data.actions)
    bpy.ops.import_scene.gltf(filepath=path)
    new = [o for o in bpy.data.objects if o not in before]
    roots = [o for o in new if o.parent is None]
    # one empty to move the whole thing, whatever the importer made
    holder = bpy.data.objects.new(name, None)
    bpy.context.scene.collection.objects.link(holder)
    for r in roots:
        r.parent = holder
    holder.scale = (spec["scale"],) * 3
    rig = next((o for o in new if o.type == "ARMATURE"), None)
    # blender renames an action whose name is taken (a second "run" becomes "run.001")
    actions = {re.sub(r"\.\d+$", "", a.name.split("|")[-1]): a for a in bpy.data.actions if a not in actions_before}
    if rig and rig.animation_data:
        rig.animation_data.action = None
    return {"holder": holder, "rig": rig, "actions": actions, "objects": new}


def key(obj, frame, location=None, rotation=None, constant=False):
    """Keys the holder's location and/or rotation (euler degrees) at frame."""
    if location is not None:
        obj.location = location
        obj.keyframe_insert("location", frame=frame)
    if rotation is not None:
        obj.rotation_euler = Euler(tuple(math.radians(a) for a in rotation))
        obj.keyframe_insert("rotation_euler", frame=frame)


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
    # body bone, placed (feet on the origin) in the dragon's own units while it's in its rest pose
    rig = actors["dragon"]["rig"]
    bpy.context.view_layer.update()
    for name, offset in (("rider", (0.0, 0.15, 1.58)), ("princess", (0.0, -0.5, 0.04))):
        holder = actors[name]["holder"]
        # turned as the dragon is, so they face where its head points (whichever way it's turned just now)
        turned = rig.matrix_world.to_quaternion().to_matrix().to_4x4()
        world = Matrix.Translation(rig.matrix_world @ Vector(offset)) @ turned @ Matrix.Diagonal(
            (ACTORS[name]["scale"],) * 3 + (1,))
        holder.parent = rig
        holder.parent_type = "BONE"
        holder.parent_bone = "body"
        bpy.context.view_layer.update()
        holder.matrix_world = world

    # the player runs in from the left, stops in front of the dome and turns to it
    player = actors["player"]["holder"]
    key(player, s(4.0), (-4.0, 0.6, 0.27), (0, 0, -90))
    key(player, s(5.0), (3.6, 0.8, 0.27), (0, 0, -90))
    key(player, s(5.3), (3.7, 0.8, 0.27), (0, 0, 180))

    # the doctor runs out of the door right of the dome (build_lab.py), from the back of the hall behind it,
    # hidden by the door until he flings it open; straight out until he's clear of the facade, then he turns
    # towards the dome and is thrown onto his back by the blast. The door swings out to the right, clear of his
    # path and of the red lamp beside it.
    door = actors["door"]["holder"]
    key(door, 1, DOOR_HINGE, (0, 0, 0))
    key(door, s(3.1) + 4, DOOR_HINGE, (0, 0, 0))
    key(door, s(3.1) + 8, rotation=(0, 0, 88))
    key(door, s(3.1) + 13, rotation=(0, 0, 100))
    doctor = actors["doctor"]["holder"]
    key(doctor, s(3.1), (10.88, 6.95, 0.27), (0, 0, 0))
    key(doctor, s(3.75), (10.88, 5.05, 0.27), (0, 0, 0))
    key(doctor, s(4.1), rotation=(0, 0, 40))
    key(doctor, s(4.6), (9.8, 2.6, 0.27), (0, 0, 40))
    key(doctor, s(5.4), (10.2, 1.9, 0.27), (0, 0, 20))
    key(doctor, s(6.6), (10.2, 1.9, 0.27), (0, 0, 180))


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

    # wide on the lab, pushing in; tilting up after the dragon; craning up and over to the castle
    aim(1, (6.5, -19.0, 1.8), (6.5, 5.0, 3.6), 50)
    aim(BURST, (6.5, -17.0, 1.8), (6.5, 5.0, 3.8), 50)
    aim(s(6.0), (6.5, -17.0, 1.9), (6.5, 5.0, 5.2), 50)
    aim(s(8.0), (6.5, -17.0, 2.2), (6.5, 6.0, 7.5), 50)
    aim(s(11.0), (6.5, -15.0, 6.0), (6.5, 70.0, 30.0), 46)
    aim(s(15.0), (6.5, -13.0, 7.0), (6.5, 100.0, 34.0), 38)
    return cam


def place_actions(actors):
    """NLA strips for the preview renders, the same timing the game uses."""
    for name, actor in actors.items():
        rig = actor["rig"]
        if rig is None:
            continue
        rig.animation_data_create()
        spec = ACTORS[name]
        track = rig.animation_data.nla_tracks.new()
        segments = spec["actions"]
        for i, (start, action_name, mode) in enumerate(segments):
            action = actor["actions"].get(action_name)
            if action is None:
                print("RESULT", name, "has no action", action_name)
                continue
            begin = max(start, spec.get("from", 1))
            end = segments[i + 1][0] if i + 1 < len(segments) else LENGTH + 1
            strip = track.strips.new(action_name, begin, action)
            length = action.frame_range[1] - action.frame_range[0]
            if mode == "loop":
                strip.repeat = max((end - begin) / max(length, 1), 0.01)
            else:
                strip.extrapolation = "HOLD_FORWARD"
            strip.frame_end = min(strip.frame_end, end)


def visibility(actors):
    for name, actor in actors.items():
        spec = ACTORS[name]
        first, last = spec.get("from", 1), spec.get("until", LENGTH + 1)
        for obj in actor["objects"]:
            for frame, hidden in ((1, first > 1), (first, False), (last, True)):
                if frame <= LENGTH:
                    obj.hide_render = hidden
                    obj.keyframe_insert("hide_render", frame=frame)


def columns(m):
    """A matrix as the game's Matrix holds it: column by column, the translation last."""
    return " ".join(f"{m[row][col]:.5f}" for col in range(4) for row in range(4))


def export(out, actors, cam):
    """intro.txt, see StoryIntroDrawable::load(): actors with their actions and a matrix per frame (one for those
    that stay put), then the camera's horizontal fov and matrix per frame."""
    scene = bpy.context.scene
    matrices = {name: [] for name in actors}
    camera = []
    for frame in range(1, LENGTH + 1):
        scene.frame_set(frame)
        for name, actor in actors.items():
            matrices[name].append(columns(actor["holder"].matrix_world))
        camera.append(f"{math.degrees(cam.data.angle):.3f} {columns(cam.matrix_world)}")
    lines = [f"fps {FPS} frames {LENGTH}"]
    for name, actor in actors.items():
        spec = ACTORS[name]
        first = spec.get("from", 1)
        keep = matrices[name] if len(set(matrices[name])) > 1 else matrices[name][:1]
        lines.append(f"actor {name} {spec['model']} {(first - 1) / FPS:.4f} "
                     f"{(spec.get('until', LENGTH + 1) - 1) / FPS:.4f} {len(spec['actions'])} {len(keep)}")
        for start, action, mode in spec["actions"]:
            lines.append(f"action {(max(start, first) - 1) / FPS:.4f} {action} {mode}")
        lines.extend(keep)
    lines.append(f"camera {len(camera)}")
    lines.extend(camera)
    with open(os.path.join(out, "intro.txt"), "w", newline="\n") as f:
        f.write("\n".join(lines) + "\n")
    print("RESULT wrote intro.txt")


def render(out, cam, frames):
    scene = bpy.context.scene
    try:
        scene.render.engine = "BLENDER_EEVEE_NEXT"
    except TypeError:
        scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x, scene.render.resolution_y = 960, 540
    world = bpy.data.worlds.new("night")
    world.use_nodes = True
    next(n for n in world.node_tree.nodes if n.type == "BACKGROUND").inputs[0].default_value = (0.01, 0.012, 0.04, 1)
    scene.world = world
    moon = bpy.data.objects.new("moon", bpy.data.lights.new("moon", "SUN"))
    moon.data.energy = 2.2
    moon.data.color = (0.75, 0.82, 1.0)
    moon.rotation_euler = (math.radians(50), math.radians(-20), math.radians(-25))
    scene.collection.objects.link(moon)
    os.makedirs(os.path.join(out, "frames"), exist_ok=True)
    for frame in frames:
        scene.frame_set(frame)
        scene.render.filepath = os.path.join(out, "frames", f"intro_{frame:04d}.png")
        bpy.ops.render.render(write_still=True)
    print("RESULT rendered", len(frames))


def build(out):
    toy.reset()
    scene = bpy.context.scene
    scene.render.fps = FPS
    scene.frame_start, scene.frame_end = 1, LENGTH
    actors = {}
    for name, spec in ACTORS.items():
        actor = load(out, name, spec)
        if actor:
            actors[name] = actor
    stage(actors)
    place_actions(actors)
    visibility(actors)
    cam = camera()
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(out, "intro.blend"))
    export(out, actors, cam)
    if RENDER:
        render(out, cam, RENDER)


build(toy.out_dir())
