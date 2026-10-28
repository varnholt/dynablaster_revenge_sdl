# The machine room's surfaces up close, put into the finished room.glb: small bricks on the green wall, big stone
# blocks on the walkway's purple front, brushed steel on the pillars, worn tiles on the floor.
#   python room_detail.py <out dir>
# Plain python with numpy, run by build_all.py after build_room.py (which leaves room_mask.npy).
import os
import sys

import numpy as np

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
import detail  # noqa: E402

# repeats per unit: bricks some 0.14 high, blocks 0.35, the steel's streaks, the tiles
REPEATS = [0.45, 0.36, 0.5, 0.35]
STRENGTH = 0.45


def main(out):
    mask = np.load(os.path.join(out, "room_mask.npy"))
    rng = np.random.default_rng(31)
    layers = [detail.masonry(rng, rows=16, per_row=8, mortar=0.08), detail.masonry(rng, rows=8, per_row=4),
              detail.brushed(rng), detail.worn(rng)]
    detail.embed(os.path.join(out, "room.glb"), {"room_palette": mask}, layers, REPEATS, STRENGTH)
    print("RESULT room detail", mask.shape)


if __name__ == "__main__":
    main(sys.argv[1])
