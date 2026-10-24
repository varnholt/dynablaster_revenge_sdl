# The field's own floor and border up close: detail.py's set of the world (grass, worn tiles, ...) put into the
# kits' glbs, masks from their colours, fainter than around the field so blocks and players stay easy to read;
# tiled in world space, the detail runs on across the floor tiles and breaks up their repeat.
#   python kit_detail.py [world ...]      patches the kits in client/data/game/story (build_all.py does it too)
# Plain python with numpy and Pillow.
import os
import sys

import numpy as np

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
import detail  # noqa: E402
import glb  # noqa: E402

STORY = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "client", "data", "game", "story")
STRENGTH = {"floor": 0.3, "border": 0.45}
MASK = 256


def apply(world, folder=STORY):
    kind = detail.story_world(world)
    for piece, strength in STRENGTH.items():
        path = os.path.join(folder, f"world{world}", f"{piece}.glb")
        if not os.path.exists(path):
            continue
        gltf, binary = glb.read(path)
        name = f"w{world}_{piece}"
        material = next(m for m in gltf["materials"] if m.get("name") == name)
        rgb = detail.base_color(gltf, binary, material, MASK)
        mask = detail.mask(kind, rgb, np.zeros(rgb.shape[:2]))
        detail.embed(path, {name: mask}, detail.images(kind), detail.repeats(kind), strength)


if __name__ == "__main__":
    for world in [int(a) for a in sys.argv[1:]] or range(1, 9):
        apply(world)
    print("kit detail in", os.path.normpath(STORY))
