# Builds client/data/level-pirate from scratch: python build_all.py
#   build_pirate.py    the ship, baked into ship.glb (Blender)
#   render_lighting.py the blocks' and players' lighting and shadow cookie, the shadow camera (Blender)
#   make_intro.py      the fly-in camera from far out at sea
#   make_level_hjb.py  level.hjb: the castle's game cameras with that shadow camera and fly-in
#   make_textures.py   the block skins
# Blender is found via the BLENDER environment variable or in its default install folder.
import glob
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "out")
LEVEL = os.path.join(HERE, "..", "..", "client", "data", "level-pirate")
CASTLE = os.path.join(HERE, "..", "..", "client", "data", "level-castle")


def blender():
    if os.environ.get("BLENDER"):
        return os.environ["BLENDER"]
    found = sorted(glob.glob(r"C:\Program Files\Blender Foundation\Blender *\blender.exe"))
    return found[-1] if found else "blender"


def run_blender(script):
    result = subprocess.run([blender(), "--background", "--factory-startup", "--python", os.path.join(HERE, script),
                             "--", OUT], capture_output=True, text=True)
    lines = [line for line in result.stdout.splitlines() if line.startswith(("RESULT", "shadow cam", "Error"))]
    print("\n".join(lines))
    if result.returncode or not any(line.startswith("RESULT") for line in lines):
        sys.exit(f"{script} failed:\n{result.stdout[-3000:]}\n{result.stderr[-3000:]}")


os.makedirs(OUT, exist_ok=True)
os.makedirs(LEVEL, exist_ok=True)
run_blender("build_pirate.py")
run_blender("render_lighting.py")
subprocess.run([sys.executable, os.path.join(HERE, "make_intro.py"), os.path.join(OUT, "intro.json")], check=True)
subprocess.run([sys.executable, os.path.join(HERE, "make_level_hjb.py"), os.path.join(LEVEL, "level.hjb"),
                os.path.join(OUT, "cameras.json"), os.path.join(OUT, "intro.json")], check=True)
subprocess.run([sys.executable, os.path.join(HERE, "make_textures.py"), LEVEL], check=True)
for name in ("ship.glb", "diffuse_level.tga", "specular_level.tga", "shadow-cookie.tga"):
    shutil.copy(os.path.join(OUT, name), LEVEL)
# the bombs' environment and the blocks' and bombs' drop shadows are the castle's
for name in ("diffuse_bomb.tga", "shadow_block.tga", "shadowdrop-3x3.tga"):
    shutil.copy(os.path.join(CASTLE, name), LEVEL)
print("level data in", os.path.normpath(LEVEL))
