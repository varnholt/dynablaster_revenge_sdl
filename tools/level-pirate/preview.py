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
    "far": ((-46.0, -66.0, 17.0), (0.52, 0.83, -0.2), 49.0),
    "bow": ((-70.0, 10.0, 14.0), (0.95, -0.2, -0.18), 49.0),
    "stern_out": ((60.0, -30.0, 12.0), (-0.85, 0.5, -0.15), 49.0),
    "side": ((-8.0, -80.0, 14.0), (0.0, 1.0, 0.02), 49.0),
    "flag": ((-14.0, -22.0, 34.0), (0.5, 0.75, 0.05), 30.0),
}

scene = bpy.context.scene
for engine in ("BLENDER_EEVEE_NEXT", "BLENDER_EEVEE"):
    try:
        scene.render.engine = engine
        break
    except TypeError:
        continue
scene.render.resolution_x, scene.render.resolution_y = 1280, 720
scene.camera.data.clip_end = 1000
# the bake's lights; the sky dome would keep them out
build["light_rig"]()
for obj in bpy.data.objects:
    if obj.name.startswith("sky"):
        obj.hide_render = True

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
