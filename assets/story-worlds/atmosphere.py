# The story worlds' air and light, written into their dioramas' scene extras for the game's glTF shader
# (GltfModel::Atmosphere): the land fades into the world's haze away from the field, the ambient light and the rim
# light at silhouettes take the world's colours.
#   python atmosphere.py [world ...]      patches the dioramas in client/data/game/story (build_all.py does it too)
import glob
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
import glb  # noqa: E402

STORY = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "client", "data", "game", "story")

# haze rgb, how much of it at most, from how far outside the field (tiles) to how far it builds up,
# ambient rgb, rim rgb
ATMOSPHERE = {
    1: [(0.66, 0.80, 0.93), 0.55, 1.0, 9.0, (1.00, 1.00, 1.02), (1.15, 1.05, 0.85)],  # a sunny morning, blue haze
    2: [(0.82, 0.76, 0.66), 0.50, 1.0, 9.0, (1.04, 1.00, 0.94), (1.20, 1.00, 0.80)],  # dusty, warm
    3: [(0.62, 0.80, 0.86), 0.55, 1.0, 9.0, (0.96, 1.00, 1.04), (0.90, 1.05, 1.15)],  # mist over the river
    4: [(0.30, 0.44, 0.34), 0.60, 1.0, 9.0, (0.90, 1.00, 0.88), (0.85, 1.10, 0.70)],  # green gloom under trees
    5: [(0.34, 0.11, 0.05), 0.65, 1.0, 9.0, (1.12, 0.88, 0.78), (1.70, 0.70, 0.25)],  # smoke and embers
    6: [(0.24, 0.16, 0.10), 0.60, 1.0, 9.0, (1.04, 0.97, 0.88), (1.45, 0.95, 0.55)],  # torchlit stone hall, warm
    7: [(0.20, 0.19, 0.16), 0.60, 1.0, 9.0, (0.98, 0.98, 0.95), (1.35, 1.00, 0.55)],  # machine hall, smoke and furnace glow
    8: [(0.03, 0.07, 0.11), 0.65, 2.0, 10.0, (0.84, 0.92, 1.08), (0.55, 0.95, 1.30)],  # the final hall, deep teal, dark past its lights
}


def values(world):
    haze, strength, near, far, ambient, rim = ATMOSPHERE[world]
    return [*haze, strength, near, far, *ambient, *rim]


def apply(world, folder=STORY):
    for path in sorted(glob.glob(os.path.join(folder, f"world{world}", "surroundings_*x*.glb"))):
        glb.set_scene_extras(path, "atmosphere", values(world))


if __name__ == "__main__":
    for world in [int(a) for a in sys.argv[1:]] or ATMOSPHERE:
        apply(world)
    print("atmosphere in", os.path.normpath(STORY))
