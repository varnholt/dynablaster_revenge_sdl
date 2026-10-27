# The mountains' rock up close, put into the finished backdrop.glb: weathered rock and coarse slabs on the cliffs
# and steep slopes, stone grain and grit on the gentler ones. Projected along the axes in the game (triplanar), so
# the cliffs get their grain where the colour, projected from above, can't give it.
#   python backdrop_detail.py <out dir>
# Plain python with numpy, run by build_all.py after build_backdrop.py (which leaves terrain_slope.npy).
import os
import sys

import numpy as np

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
import detail  # noqa: E402

# rock, the same rock coarser for the big cracks, stone grain, grit; repeats per unit, the mountain is some
# 25 units high on screen; the shader drops detail finer than a few pixels per repeat, so none of them may be fine
REPEATS = [0.2, 0.07, 0.35, 0.3]
STRENGTH = 0.7


def main(out):
    # the mask only blends broad areas, half the colour texture's size is plenty
    slope = np.load(os.path.join(out, "terrain_slope.npy"))[::2, ::2]
    steep = np.clip((slope - 0.6) / 0.8, 0, 1)
    mask = np.stack([steep * 0.6, steep * 0.4 + (1.0 - steep) * 0.2, (1.0 - steep) * 0.5, (1.0 - steep) * 0.3],
                    axis=-1)
    rng = np.random.default_rng(23)
    layers = [detail.rock(rng), detail.rock(rng), detail.stone(rng), detail.grit(rng)]
    detail.embed(os.path.join(out, "backdrop.glb"), {"terrain": mask}, layers, REPEATS, STRENGTH)
    print("RESULT backdrop detail", mask.shape)


if __name__ == "__main__":
    main(sys.argv[1])
