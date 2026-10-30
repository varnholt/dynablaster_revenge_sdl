# Builds client/data/level-pirate from scratch: python build_all.py
#   build_pirate.py    the ship, baked into ship.glb with its lightmaps by day and by night (Blender)
#   render_lighting.py the light round the field by day and by night, the shadow cookie and camera (Blender)
#   probes.py          (story-worlds) the blocks' and players' light probes by day and by night from that light
#   make_intro.py      the fly-in camera from far out at sea
#   make_level_hjb.py  level.hjb: the castle's game cameras with that shadow camera and fly-in
#   make_textures.py   the block skins
#   ship_detail.py     the ship's detail up close, put into ship.glb
# Blender is found via the BLENDER environment variable or in its default install folder.
import glob
import io
import os
import shutil
import subprocess
import sys

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "out")
LEVEL = os.path.join(HERE, "..", "..", "client", "data", "level-pirate")
CASTLE = os.path.join(HERE, "..", "..", "client", "data", "level-castle")
sys.path.insert(0, os.path.join(HERE, ".."))
sys.path.insert(0, os.path.join(HERE, "..", "story-worlds"))
import glb  # noqa: E402
import probes  # noqa: E402

# the light probes (probes.maps()): how bright on average and how much of the light's colour, by day the warm sun,
# by night the moon's blue with the lanterns' orange, a little brighter than the deck, the players must stay readable
PROBES = {"day": (0.64, 0.85), "night": (0.5, 0.8)}

# the ship's air (GltfModel::Atmosphere): no haze, the sky's painted colours left as they are (ambient 1 and the
# sun 0.45 + 0.69 * its colour on the dome's upward normals), the sun towards build_pirate.SUN; the sea's light
# thrown up onto the hull at the waterline, warm white
ATMOSPHERE = [0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, -0.38, 0.62, 0.69, 0.84, 0.8, 0.72]
CAUSTICS = [0.65, 1.1, 0.7, 1.1, 1.0, 0.94, 0.82]


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
subprocess.run([sys.executable, os.path.join(HERE, "ship_detail.py"), os.path.join(OUT, "ship.glb")], check=True)


def compress_lightmaps(path):
    """The lightmaps (lighting.embed() adds them as PNGs) as JPEGs, as Blender exports the colours."""
    gltf, binary = glb.read(path)
    for image in gltf.get("images", []):
        if image.get("name", "").endswith(("_lightmap", "_lightmap_night")) and image.get("mimeType") == "image/png":
            view = gltf["bufferViews"][image["bufferView"]]
            start = view.get("byteOffset", 0)
            out = io.BytesIO()
            Image.open(io.BytesIO(bytes(binary[start:start + view["byteLength"]]))).convert("RGB").save(
                out, "JPEG", quality=92)
            glb.replace_image(path, image["name"], out.getvalue(), mime="image/jpeg")


def air(path):
    gltf, binary = glb.read(path)
    scene = gltf["scenes"][gltf.get("scene", 0)]
    scene.setdefault("extras", {}).update({"atmosphere": ATMOSPHERE, "caustics": CAUSTICS})
    glb.write(path, gltf, binary)


ship = os.path.join(OUT, "ship.glb")
compress_lightmaps(ship)
air(ship)
for name in ("ship.glb", "shadow-cookie.tga"):
    shutil.copy(os.path.join(OUT, name), LEVEL)
# by day under their usual names, then towards the night in NIGHT_STEPS steps (_1 .. _4, the last the night's), the
# game takes the next one as the evening falls (LevelPirate::setDayTime())
NIGHT_STEPS = 4
maps = {time: probes.maps(os.path.join(OUT, f"probe_{time}"), mean, saturation) for time, (mean, saturation) in PROBES.items()}
for step in range(NIGHT_STEPS + 1):
    night = step / NIGHT_STEPS
    suffix = f"_{step}" if step else ""
    for index, name in ((0, "diffuse_level"), (1, "specular_level")):
        blend = maps["day"][index] * (1 - night) + maps["night"][index] * night
        probes.save(blend, maps["day"][2], os.path.join(LEVEL, f"{name}{suffix}.tga"))
# the bombs' environment and the blocks' and bombs' drop shadows are the castle's
for name in ("diffuse_bomb.tga", "shadow_block.tga", "shadowdrop-3x3.tga"):
    shutil.copy(os.path.join(CASTLE, name), LEVEL)
print("level data in", os.path.normpath(LEVEL))
