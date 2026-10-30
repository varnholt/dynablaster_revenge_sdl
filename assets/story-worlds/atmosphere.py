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

# per world: towards the sun (z-up), its colour and strength (0: none, a cave or a hall), the sky's colour and
# strength, how bright its props' glows light their surroundings ("glows", lighting.GLOW_POWER) and in the lava
# cave how bright the lava glows ("lava", "lava_color"), and how the blocks and players take its light ("probe": the
# light probes' brightness and how much of the light's colour, probes.py; one coloured light all round would dye
# every block alike; "probe_tint" tints them, for a light the bake doesn't see, as the water's), and where its
# lamps light it more than its sun ("field": the ambient light's and the sun's colours) how the field's floor and
# border are lit, which the game lights live; strengths so a white surface in full sun comes out about 1 (the bake's light is the light
# falling on a white surface)
LIGHTING = {
    # a sunny morning: the sun low from the front left, its light warm, long shadows falling away back right; the
    # sky a pale blue all round
    1: {"sun": (-0.62, -0.48, 0.62), "sun_color": (1.0, 0.9, 0.74), "sun_strength": 3.4,
        "sky": (0.62, 0.74, 0.95), "sky_strength": 0.58},
    # a hot dusty afternoon in the mountains: the sun high from the right, hard and golden, short dark shadows; the
    # sky hazy with dust, warm; the crystals glow purple in the rocks' shade
    2: {"sun": (0.55, -0.4, 0.73), "sun_color": (1.0, 0.85, 0.62), "sun_strength": 3.8,
        "sky": (0.82, 0.74, 0.62), "sky_strength": 0.5, "glows": 1.2, "probe": (0.56, 0.85)},
    # dawn over the misty river: the sun just up behind the field, low and peach gold, long shadows running towards
    # the camera and the edges catching its light, the shade cool blue in the mist
    3: {"sun": (0.3, 0.88, 0.36), "sun_color": (1.0, 0.72, 0.5), "sun_strength": 3.4,
        "sky": (0.58, 0.72, 0.88), "sky_strength": 0.62, "probe_tint": (0.92, 0.98, 1.1)},
    # the forest's gloom: a low evening sun slanting in from the back left through the trunks, shafts of gold in
    # deep green shade, the gloom kept
    4: {"sun": (-0.55, 0.6, 0.58), "sun_color": (1.0, 0.82, 0.5), "sun_strength": 2.4,
        "sky": (0.4, 0.58, 0.42), "sky_strength": 0.36, "probe": (0.5, 0.85)},
    # the lava cave: no sky to speak of, the lava's red glow from below and the embers'
    5: {"sun": (0.0, 0.0, 1.0), "sun_color": (1.0, 1.0, 1.0), "sun_strength": 0.0,
        "sky": (0.36, 0.14, 0.08), "sky_strength": 0.22, "glows": 1.6,
        "lava": 3.5, "lava_color": (1.0, 0.36, 0.08), "probe": (0.38, 0.3)},
    # the keep by torchlight: the torches' and braziers' warm pools on the sandstone, the hall's warm dusk round
    # them, a little cold moonlight falling in from high on the left for the shade to be blue in
    6: {"sun": (-0.35, 0.45, 0.82), "sun_color": (0.55, 0.66, 1.0), "sun_strength": 0.7,
        "sky": (0.42, 0.3, 0.2), "sky_strength": 0.3, "glows": 2.2, "probe": (0.5, 0.9),
        "field": {"ambient": (1.08, 0.9, 0.7), "sun": (0.42, 0.46, 0.6)}},
    # the machine hall: grey daylight down through its skylights, the furnaces and lamps glowing orange
    7: {"sun": (0.15, 0.25, 0.96), "sun_color": (0.9, 0.93, 1.0), "sun_strength": 1.6,
        "sky": (0.36, 0.34, 0.31), "sky_strength": 0.3, "glows": 1.6, "probe": (0.48, 0.85)},
    # the final hall: dark, its teal panels and the red alarm lights all the light there is
    8: {"sun": (0.0, 0.0, 1.0), "sun_color": (1.0, 1.0, 1.0), "sun_strength": 0.0,
        "sky": (0.06, 0.13, 0.18), "sky_strength": 0.25, "glows": 2.2, "probe": (0.52, 0.5)},
}

# per world with water: the light it throws, a web of bright filaments drifting over everything near it (the game's
# "caustics"): strength, cells per unit, speed, the units above and below the water they fade out over, colour
CAUSTICS = {
    # the river at dawn: the low sun's light thrown up off the stream onto the banks, the floor and the blocks, warm
    # white, slow
    3: [0.5, 1.4, 0.6, 0.8, (1.0, 0.9, 0.76)],
}


def values(world):
    """The atmosphere's values; with a light of its own the ambient light is its sky's and its sun lights the field
    as it lights the baked dioramas (lighting.py, the bake's strengths: a white surface's light is the strength over
    pi), the game's shader takes 0.45 of the ambient light."""
    haze, strength, near, far, ambient, rim = ATMOSPHERE[world]
    # without a sun (a cave, a hall) the field keeps the light it had, the play needs seeing
    if world not in LIGHTING or LIGHTING[world]["sun_strength"] <= 0:
        return [*haze, strength, near, far, *ambient, *rim]
    light = LIGHTING[world]
    ambient = [c * light["sky_strength"] / 0.45 for c in light["sky"]]
    norm = sum(c * c for c in light["sun"]) ** 0.5
    sun = [c * light["sun_strength"] / 3.14159 for c in light["sun_color"]]
    if "field" in light:
        ambient, sun = light["field"]["ambient"], light["field"]["sun"]
    return [*haze, strength, near, far, *ambient, *rim, *(c / norm for c in light["sun"]), *sun]


def apply(world, folder=STORY):
    for path in sorted(glob.glob(os.path.join(folder, f"world{world}", "surroundings_*x*.glb"))):
        glb.set_scene_extras(path, "atmosphere", values(world))
        if world in CAUSTICS:
            strength, scale, speed, fade, color = CAUSTICS[world]
            glb.set_scene_extras(path, "caustics", [strength, scale, speed, fade, *color])


if __name__ == "__main__":
    for world in [int(a) for a in sys.argv[1:]] or ATMOSPHERE:
        apply(world)
    print("atmosphere in", os.path.normpath(STORY))
