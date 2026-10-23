# Renders the unbaked ship from a few views for a quick look:
#   blender --background --factory-startup --python preview.py -- <out dir> [view ...]
# views: game (the game's camera), intro (where the castle's fly-in starts), sail, stern
import math
import os
import sys

import bpy
from mathutils import Vector

HERE = os.path.dirname(os.path.abspath(__file__))
ARGS = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
OUT = ARGS[0] if ARGS else os.path.join(HERE, "out")
VIEWS = ARGS[1:] or ["game"]

os.environ["PIRATE_PREVIEW"] = "1"
build = {"__name__": "build_pirate", "__file__": os.path.join(HERE, "build_pirate.py")}
exec(compile(open(build["__file__"]).read(), build["__file__"], "exec"), build)

POSES = {
    # eye, look direction, vertical fov
    "game": ((6.0, -15.9, 12.1), (0.004, 0.64, -0.765), 49.0),
    "intro": ((-9.96, -12.04, 8.45), (0.582, 0.586, -0.564), 49.0),
    "sail": ((-12.0, -9.0, 9.0), (0.55, 0.65, -0.35), 40.0),
    "stern": ((8.0, -9.0, 5.0), (0.85, 0.3, -0.25), 55.0),
    "wide": ((6.5, -40.0, 30.0), (0.0, 0.75, -0.62), 55.0),
}

scene = bpy.context.scene
for engine in ("BLENDER_EEVEE_NEXT", "BLENDER_EEVEE"):
    try:
        scene.render.engine = engine
        break
    except TypeError:
        continue
scene.render.resolution_x, scene.render.resolution_y = 1280, 720
scene.world = scene.world or bpy.data.worlds.new("World")
scene.world.color = (0.05, 0.07, 0.12)
sun = bpy.data.lights.new("sun", "SUN")
sun.energy = 3.0
obj = bpy.data.objects.new("sun", sun)
scene.collection.objects.link(obj)
obj.rotation_euler = Vector((0.35, 0.55, -0.75)).to_track_quat("-Z", "Y").to_euler()

cam = scene.camera
os.makedirs(OUT, exist_ok=True)
for view in VIEWS:
    eye, look, fov = POSES[view]
    cam.location = eye
    cam.rotation_euler = Vector(look).to_track_quat("-Z", "Y").to_euler()
    cam.data.angle_y = math.radians(fov)
    scene.render.filepath = os.path.join(OUT, f"preview_{view}.png")
    bpy.ops.render.render(write_still=True)
    print("RESULT", scene.render.filepath, flush=True)
