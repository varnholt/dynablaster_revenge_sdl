# The machine room's props up close, put into the finished room.glb: brushed metal on the steel, the pipes and the
# pillars, worn paint on the panel, grain on the crate. The wall, floor and front carry their own painted textures.
#   python room_detail.py <out dir>
# Plain python with numpy, run by build_all.py after build_room.py (which leaves room_mask.npy).
import os
import sys

import numpy as np

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
import detail  # noqa: E402

# repeats per unit: the steel's streaks, the panel's wear, the wood's grain
REPEATS = [0.6, 0.5, 0.8, 0.5]
STRENGTH = 0.6


def main(out):
    mask = np.load(os.path.join(out, "room_mask.npy"))
    rng = np.random.default_rng(31)
    layers = [detail.brushed(rng), detail.worn(rng), detail.wood(rng), detail.grit(rng)]
    detail.embed(os.path.join(out, "room.glb"), {"room_palette": mask}, layers, REPEATS, STRENGTH)
    print("RESULT room detail", mask.shape)


if __name__ == "__main__":
    main(sys.argv[1])
