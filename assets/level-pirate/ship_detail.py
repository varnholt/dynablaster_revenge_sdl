# The ship's detail up close (assets/detail.py's "ship" set: wood grain, canvas, old paint, metal) and the air it
# sails in, put into the finished ship.glb: masks from each material's colour by day, the scene's atmosphere.
#   python ship_detail.py <ship.glb>
# Plain python with numpy and Pillow, run by build_all.py after Blender.
import io
import os
import sys

import numpy as np
from PIL import Image

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
import detail  # noqa: E402
import glb  # noqa: E402

MATERIALS = ["deck_export", "props_export", "exterior_export", "cloth_export"]
MASK = 1024


def base_color(gltf, binary, material):
    """A material's base colour texture as rgb 0..1, rows bottom up (as detail.mask() takes them)."""
    texture = gltf["textures"][material["pbrMetallicRoughness"]["baseColorTexture"]["index"]]
    view = gltf["bufferViews"][gltf["images"][texture["source"]]["bufferView"]]
    start = view.get("byteOffset", 0)
    image = Image.open(io.BytesIO(bytes(binary[start:start + view["byteLength"]]))).convert("RGB")
    image = image.resize((MASK, MASK), Image.BILINEAR)
    return np.asarray(image, dtype=np.float32)[::-1] / 255.0


def main(path):
    gltf, binary = glb.read(path)
    masks = {}
    for material in gltf["materials"]:
        if material.get("name") in MATERIALS:
            rgb = base_color(gltf, binary, material)
            masks[material["name"]] = detail.mask("ship", rgb, np.zeros(rgb.shape[:2]))
    detail.embed(path, masks, detail.images("ship"), detail.repeats("ship"), detail.strength("ship"))
    print("RESULT ship detail", sorted(masks))


if __name__ == "__main__":
    main(sys.argv[1])
