# What the story's cutscenes (build_intro.py, build_ending.py) share: loading the models the build_*.py scripts
# wrote, keying where they are, and writing the timeline the game plays (StoryCutsceneDrawable::load()): per actor its
# model, the actions it plays from when, its transform sampled every frame; captions; the camera's per frame.
# A scene is a dict of actors, {name: {"model", "scale", "actions": [(frame, action, "loop"|"hold")], "from",
# "until"}}, a stage(actors) placing them and a camera() returning the camera.
import math
import os
import re
import sys

import bpy
from mathutils import Euler

import toy

FPS = 24

ARGS = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
RENDER = [int(f) for f in ARGS[2].split(",")] if len(ARGS) > 2 and ARGS[1] == "render" else []


def s(seconds):
    """Seconds to the frame they start."""
    return 1 + round(seconds * FPS)


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


def place_actions(actors, specs, length):
    """NLA strips for the preview renders, the same timing the game uses."""
    for name, actor in actors.items():
        rig = actor["rig"]
        if rig is None:
            continue
        rig.animation_data_create()
        spec = specs[name]
        track = rig.animation_data.nla_tracks.new()
        segments = spec["actions"]
        for i, (start, action_name, mode) in enumerate(segments):
            action = actor["actions"].get(action_name)
            if action is None:
                print("RESULT", name, "has no action", action_name)
                continue
            begin = max(start, spec.get("from", 1))
            end = segments[i + 1][0] if i + 1 < len(segments) else length + 1
            strip = track.strips.new(action_name, begin, action)
            span = action.frame_range[1] - action.frame_range[0]
            if mode == "loop":
                strip.repeat = max((end - begin) / max(span, 1), 0.01)
            else:
                strip.extrapolation = "HOLD_FORWARD"
            strip.frame_end = min(strip.frame_end, end)


def visibility(actors, specs, length):
    for name, actor in actors.items():
        spec = specs[name]
        first, last = spec.get("from", 1), spec.get("until", length + 1)
        for obj in actor["objects"]:
            for frame, hidden in ((1, first > 1), (first, False), (last, True)):
                if frame <= length:
                    obj.hide_render = hidden
                    obj.keyframe_insert("hide_render", frame=frame)


def columns(m):
    """A matrix as the game's Matrix holds it: column by column, the translation last."""
    return " ".join(f"{m[row][col]:.5f}" for col in range(4) for row in range(4))


def export(out, name, actors, specs, length, cam, captions=(), indoors=False):
    """<name>.txt, see StoryCutsceneDrawable::load(): actors with their actions and a matrix per frame (one for those
    that stay put), the captions (first and last frame, text), "indoors" for a scene lit by lamps rather than the
    moon, then the camera's horizontal fov and matrix per frame."""
    scene = bpy.context.scene
    matrices = {actor: [] for actor in actors}
    camera = []
    for frame in range(1, length + 1):
        scene.frame_set(frame)
        for actor_name, actor in actors.items():
            matrices[actor_name].append(columns(actor["holder"].matrix_world))
        camera.append(f"{math.degrees(cam.data.angle):.3f} {columns(cam.matrix_world)}")
    lines = [f"fps {FPS} frames {length}"]
    for actor_name, actor in actors.items():
        spec = specs[actor_name]
        first = spec.get("from", 1)
        keep = matrices[actor_name] if len(set(matrices[actor_name])) > 1 else matrices[actor_name][:1]
        lines.append(f"actor {actor_name} {spec['model']} {(first - 1) / FPS:.4f} "
                     f"{(spec.get('until', length + 1) - 1) / FPS:.4f} {len(spec['actions'])} {len(keep)}")
        for start, action, mode in spec["actions"]:
            lines.append(f"action {(max(start, first) - 1) / FPS:.4f} {action} {mode}")
        lines.extend(keep)
    for first, last, text in captions:
        lines.append(f"caption {(first - 1) / FPS:.4f} {(last - 1) / FPS:.4f} {text}")
    if indoors:
        lines.append("indoors")
    lines.append(f"camera {len(camera)}")
    lines.extend(camera)
    with open(os.path.join(out, f"{name}.txt"), "w", newline="\n") as f:
        f.write("\n".join(lines) + "\n")
    print(f"RESULT wrote {name}.txt")


def render(out, name, cam, frames):
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
        scene.render.filepath = os.path.join(out, "frames", f"{name}_{frame:04d}.png")
        bpy.ops.render.render(write_still=True)
    print("RESULT rendered", len(frames))


def build(out, name, specs, length, stage, camera, captions=(), indoors=False):
    """Loads the actors, stages them, writes <name>.blend and <name>.txt (and renders the frames asked for)."""
    toy.reset()
    scene = bpy.context.scene
    scene.render.fps = FPS
    scene.frame_start, scene.frame_end = 1, length
    actors = {}
    for actor_name, spec in specs.items():
        actor = load(out, actor_name, spec)
        if actor:
            actors[actor_name] = actor
    stage(actors)
    place_actions(actors, specs, length)
    visibility(actors, specs, length)
    cam = camera()
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(out, f"{name}.blend"))
    export(out, name, actors, specs, length, cam, captions, indoors)
    if RENDER:
        render(out, name, cam, RENDER)
