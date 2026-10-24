# Builds the story mode's enemies and bosses into client/data/game/enemies from scratch: python build_all.py [name ...]
#   build_enemies.py     the models, rigged and animated (Blender)
#   sdf_core.py          shapes smooth-unioned into one surface by signed distances, retopologised, painted
#   sdf_rig.py           skeleton, skin weights from the shapes, idle, walk and die cycles
#   sdf_enemies.py       the enemies' and bosses' shapes, colours and painted faces
#   sdf_enemies_more.py
# Names build only those models. Blender is found via the BLENDER environment variable or in its default install
# folder; it needs its bundled openvdb module.
import glob
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "out")
ENEMIES = os.path.join(HERE, "..", "..", "client", "data", "game", "enemies")


def blender():
    if os.environ.get("BLENDER"):
        return os.environ["BLENDER"]
    found = sorted(glob.glob(r"C:\Program Files\Blender Foundation\Blender *\blender.exe"))
    return found[-1] if found else "blender"


os.makedirs(OUT, exist_ok=True)
result = subprocess.run([blender(), "--background", "--factory-startup", "--python",
                         os.path.join(HERE, "build_enemies.py"), "--", OUT, *sys.argv[1:]], capture_output=True, text=True)
lines = [line for line in result.stdout.splitlines() if line.startswith(("RESULT", "Error"))]
print("\n".join(lines))
if result.returncode or "Traceback" in result.stdout + result.stderr or not any("RESULT seconds" in l for l in lines):
    sys.exit(f"build_enemies.py failed:\n{result.stdout[-3000:]}\n{result.stderr[-3000:]}")
for path in sorted(glob.glob(os.path.join(OUT, "glb", "*.glb"))):
    shutil.copy(path, ENEMIES)
print("enemies in", os.path.normpath(ENEMIES))
