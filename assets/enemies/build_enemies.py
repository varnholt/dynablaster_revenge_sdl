# Builds every story mode enemy and boss from scratch: each model's shapes (sdf_enemies.py, sdf_enemies_more.py)
# become one closed smooth surface with painted textures (sdf_core.py), rigged and animated with idle, walk and die
# cycles (sdf_rig.py), exported as <name>.glb.
#   blender --background --factory-startup --python build_enemies.py -- <out dir> [name ...]
# writes <out dir>/glb/<name>.glb and the textures to <out dir>/textures; names limit the build to those models.
import os
import sys
import time

import bpy

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import sdf_core  # noqa: E402
import sdf_enemies  # noqa: E402
import sdf_rig  # noqa: E402

ARGS = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
OUT = os.path.abspath(ARGS[0]) if ARGS else os.path.join(HERE, "out")
ONLY = {name.lower() for name in ARGS[1:]}


def build():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.render.fps = sdf_rig.FPS
    start = time.time()
    enemies, bosses = sdf_enemies.ENEMIES, sdf_enemies.BOSSES
    builders = {}
    for build_model in sdf_enemies.MODELS:
        model = build_model()
        if ONLY and model.name.lower() not in ONLY:
            continue
        builders[model.name] = build_model
        # side by side as in a line up, enemies in rows of nine, the bosses behind them
        if build_model in enemies:
            row, col = divmod(enemies.index(build_model), 9)
            offset = ((col - 4) * 2.4, row * 3.0, 0)
        else:
            offset = ((bosses.index(build_model) - (len(bosses) - 1) / 2) * 4.6, 8.0, 0)
        _, info = sdf_core.convert(model, scene.collection, os.path.join(OUT, "textures"), offset)
        print("RESULT built", model.name, info, f"{time.time() - start:.0f} s", flush=True)
    for low in [o for o in scene.objects if "_SDF" in o.name and o.type == "MESH"]:
        name = low.name.split("_SDF")[0]
        arm = sdf_rig.build_armature(builders[name](), low)
        sdf_rig.animate(builders[name](), arm)
        size = sdf_rig.export_animated_glb(arm, low, name.lower(), os.path.join(OUT, "glb"))
        print("RESULT exported", name.lower(), size // 1024, "kb", flush=True)
    print("RESULT seconds", round(time.time() - start))


build()
