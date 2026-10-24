# Renders a built character to check it: views across frames of one action, tiled into one image.
#   blender --background <who>.blend --python preview.py -- <out.png> <action> <frame,frame,...> [view ...]
# views: front, back, side, three_quarter, top, below; close_<view> frames the top fifth (the head) closer
import math
import os
import sys

import bpy
from mathutils import Vector

args = sys.argv[sys.argv.index("--") + 1:]
out, action_name, frames = args[0], args[1], [int(f) for f in args[2].split(",")]
views = args[3:] or ["front", "three_quarter"]
DIRECTIONS = {
    "front": (0, -1, 0.15), "back": (0, 1, 0.2), "side": (1, 0, 0.1), "three_quarter": (0.8, -1, 0.45),
    "top": (0, -0.2, 1), "below": (0.3, -1, -0.6),
}
TILE = 360

scene = bpy.context.scene
rig = next(o for o in scene.objects if o.type == "ARMATURE")
meshes = [o for o in scene.objects if o.type == "MESH"]
if action_name != "rest":
    for track in rig.animation_data.nla_tracks:
        track.mute = True
    rig.animation_data.action = bpy.data.actions[action_name]
    slots = getattr(bpy.data.actions[action_name], "slots", None)
    if slots:
        rig.animation_data.action_slot = slots[0]

try:
    scene.render.engine = "BLENDER_EEVEE_NEXT"
except TypeError:
    scene.render.engine = "BLENDER_EEVEE"
scene.render.resolution_x = scene.render.resolution_y = TILE
scene.render.film_transparent = False
world = bpy.data.worlds.new("preview")
world.use_nodes = True
next(n for n in world.node_tree.nodes if n.type == "BACKGROUND").inputs[0].default_value = (0.32, 0.34, 0.4, 1)
scene.world = world
sun = bpy.data.objects.new("sun", bpy.data.lights.new("sun", "SUN"))
sun.data.energy = 3.5
sun.rotation_euler = (math.radians(45), math.radians(15), math.radians(-35))
scene.collection.objects.link(sun)
cam = bpy.data.objects.new("cam", bpy.data.cameras.new("cam"))
cam.data.type = "ORTHO"
scene.collection.objects.link(cam)
scene.camera = cam

# frame the rest pose's bounds generously so poses don't leave the picture
scene.frame_set(frames[0])
bpy.context.view_layer.update()
pts = [o.matrix_world @ Vector(c) for o in meshes for c in o.bound_box]
lo = Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts)))
hi = Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))
centre = (lo + hi) / 2
cam.data.ortho_scale = (hi - lo).length * 1.05

tiles = []
for view in views:
    close = view.startswith("close_")
    direction = Vector(DIRECTIONS[view.removeprefix("close_")]).normalized()
    cam.data.ortho_scale = (hi - lo).length * (0.32 if close else 1.05)
    aim = Vector((centre.x, centre.y, hi.z - (hi.z - lo.z) * 0.22)) if close else centre
    cam.location = aim + direction * 20
    cam.rotation_euler = (-direction).to_track_quat("-Z", "Y").to_euler()
    for frame in frames:
        scene.frame_set(frame)
        path = os.path.join(os.path.dirname(out), f"_tile_{view}_{frame}.png")
        scene.render.filepath = path
        bpy.ops.render.render(write_still=True)
        tiles.append(path)

# tile them: one row per view
sheet = bpy.data.images.new("sheet", TILE * len(frames), TILE * len(views))
pixels = [0.0] * (TILE * len(frames) * TILE * len(views) * 4)
for i, path in enumerate(tiles):
    image = bpy.data.images.load(path)
    row = len(views) - 1 - i // len(frames)
    col = i % len(frames)
    src = list(image.pixels)
    for y in range(TILE):
        start = ((row * TILE + y) * TILE * len(frames) + col * TILE) * 4
        pixels[start:start + TILE * 4] = src[y * TILE * 4:(y + 1) * TILE * 4]
    bpy.data.images.remove(image)
    os.remove(path)
sheet.pixels = pixels
sheet.filepath_raw = out
sheet.file_format = "PNG"
sheet.save()
print("RESULT preview", out)
