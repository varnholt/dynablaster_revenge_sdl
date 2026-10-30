# Builds the story mode's eight worlds into client/data/game/story from scratch: python build_all.py [world ...]
#   build_kits.py          per world the floor tile, the border block and the plain land around a field
#   build_surroundings.py  per world and field size the diorama around the stage, colour and ambient occlusion baked,
#                          the worlds with a light of their own (atmosphere.LIGHTING, lighting.py) its lightmaps too
#   atmosphere.py          the worlds' haze and light, written into the dioramas
#   kit_detail.py          the floor's and border's detail up close
#   probes.py              the light probes the blocks, bombs and players are lit by, for the worlds with a light
# Worlds build only those. Blender is found via the BLENDER environment variable or in its default install folder.
# The dioramas take a few minutes, a GPU makes the bakes quicker.
import glob
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import atmosphere  # noqa: E402
import kit_detail  # noqa: E402
import probes  # noqa: E402

sys.path.insert(0, os.path.join(HERE, ".."))
import glb  # noqa: E402
OUT = os.path.join(HERE, "out")
STORY = os.path.join(HERE, "..", "..", "client", "data", "game", "story")

# the field sizes the stages of each world use (a stage scrolling sideways is 29x11, upwards 13x23, else 13x11;
# see StoryStage in shared/storystage.h and the stage table in shared/storystages.cpp)
SIZES = {
    1: ["13x11", "13x23", "29x11"],
    2: ["13x11", "13x23", "29x11"],
    3: ["13x23", "29x11"],
    4: ["13x23", "29x11"],
    5: ["13x23", "29x11"],
    6: ["13x23", "29x11"],
    7: ["13x23", "29x11"],
    8: ["13x11", "13x23", "29x11"],
}


def blender():
    if os.environ.get("BLENDER"):
        return os.environ["BLENDER"]
    found = sorted(glob.glob(r"C:\Program Files\Blender Foundation\Blender *\blender.exe"))
    return found[-1] if found else "blender"


def compress_lightmaps(path):
    """The dioramas' lightmaps (lighting.embed() adds them as PNGs) as JPEGs, as Blender exports their colours."""
    import io

    from PIL import Image
    gltf, binary = glb.read(path)
    for image in gltf.get("images", []):
        if image.get("name", "").endswith("_lightmap") and image.get("mimeType") == "image/png":
            view = gltf["bufferViews"][image["bufferView"]]
            start = view.get("byteOffset", 0)
            out = io.BytesIO()
            Image.open(io.BytesIO(bytes(binary[start:start + view["byteLength"]]))).convert("RGB").save(
                out, "JPEG", quality=92)
            glb.replace_image(path, image["name"], out.getvalue(), mime="image/jpeg")


def run_blender(script, out, worlds):
    result = subprocess.run([blender(), "--background", "--factory-startup", "--python", os.path.join(HERE, script),
                             "--", out, *worlds], capture_output=True, text=True)
    lines = [line for line in result.stdout.splitlines() if line.startswith(("RESULT", "Error"))]
    print("\n".join(lines))
    if result.returncode or "Traceback" in result.stdout + result.stderr:
        sys.exit(f"{script} failed:\n{result.stdout[-3000:]}\n{result.stderr[-3000:]}")


worlds = sys.argv[1:]
run_blender("build_kits.py", os.path.join(OUT, "kits"), worlds)
run_blender("build_surroundings.py", os.path.join(OUT, "surroundings"), worlds)
for world, sizes in SIZES.items():
    if worlds and str(world) not in worlds:
        continue
    target = os.path.join(STORY, f"world{world}")
    os.makedirs(target, exist_ok=True)
    for piece in ("floor", "border", "surroundings"):
        shutil.copy(os.path.join(OUT, "kits", f"world{world}", f"{piece}.glb"), target)
    for size in sizes:
        shutil.copy(os.path.join(OUT, "surroundings", f"world{world}", f"surroundings_{size}.glb"), target)
        compress_lightmaps(os.path.join(target, f"surroundings_{size}.glb"))
    atmosphere.apply(world)
    kit_detail.apply(world)
    probes.apply(world)
print("worlds in", os.path.normpath(STORY))
