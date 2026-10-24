# The ship's detail up close (assets/detail.py's "ship" set: wood grain, canvas, old paint, metal) and the air it
# sails in, put into the finished ship.glb: masks from each material's colour by day, the scene's atmosphere.
#   python ship_detail.py <ship.glb>
# Plain python with numpy and Pillow, run by build_all.py after Blender.
import os
import sys

import numpy as np

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
import detail  # noqa: E402
import glb  # noqa: E402

MATERIALS = ["deck_export", "props_export", "exterior_export", "cloth_export"]
MASK = 1024


def main(path):
    gltf, binary = glb.read(path)
    masks = {}
    for material in gltf["materials"]:
        if material.get("name") in MATERIALS:
            rgb = detail.base_color(gltf, binary, material, MASK)
            masks[material["name"]] = detail.mask("ship", rgb, np.zeros(rgb.shape[:2]))
    detail.embed(path, masks, detail.images("ship"), detail.repeats("ship"), detail.strength("ship"))
    print("RESULT ship detail", sorted(masks))


if __name__ == "__main__":
    main(sys.argv[1])
