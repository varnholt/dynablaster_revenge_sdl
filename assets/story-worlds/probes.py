# The story worlds' light probes, the sphere maps the blocks, bombs and players are lit by (as the original levels'
# diffuse_level.tga and specular_level.tga): from the cube of the light round the middle of the field that
# build_surroundings.py renders (lighting.py). Plain python:
#   python probes.py [world ...]     writes client/data/game/story/world<n>/{diffuse_level,specular_level,
#                                    specular_level2,diffuse_bomb,shadow-cookie}.tga
# The maps are looked up by view space directions (playermaterial-frag.glsl's textureSphere()), so they're made for
# the arena's camera, which the story mode keeps (level-castle's, looking north and down).
import os
import sys

import numpy as np
from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from atmosphere import LIGHTING  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "out", "surroundings")
STORY = os.path.join(HERE, "..", "..", "client", "data", "game", "story")

# the arena's camera (hjb_cameras.py on level-castle/level.hjb), right, up, back (towards the viewer) in the world
LOOK = np.array([0.0, 0.67, -0.74])
UP = np.array([0.0, 0.74, 0.67])
SIZE = 256
# how bright the maps are on average over what the shaders look up (as the original levels' are): the blocks add
# the specular map to their colour, a sky's full brightness there washes them out
DIFFUSE_MEAN = 0.62
SPECULAR_MEAN = 0.07


def load(folder):
    """The cube's faces: (directions, radiance) of every texel, flattened."""
    directions, colors = [], []
    for name in ("px", "nx", "py", "ny", "pz", "nz"):
        pixels = np.load(os.path.join(folder, f"probe_{name}.npy"))
        right, up, forward = np.load(os.path.join(folder, f"probe_{name}_basis.npy"))
        n = pixels.shape[0]
        # rows bottom up, as Blender keeps them
        s = (np.arange(n) + 0.5) / n * 2 - 1
        x, y = np.meshgrid(s, s)
        d = forward[None, None] + x[..., None] * right[None, None] + y[..., None] * up[None, None]
        d /= np.linalg.norm(d, axis=-1, keepdims=True)
        # each texel's solid angle on the face
        weight = 1.0 / (1 + x * x + y * y) ** 1.5
        directions.append(d.reshape(-1, 3))
        colors.append((pixels * weight[..., None]).reshape(-1, 3))
        colors[-1] = np.concatenate([colors[-1], weight.reshape(-1, 1)], axis=1)
    return np.concatenate(directions), np.concatenate(colors)


def sh9(d):
    """The first nine real spherical harmonics of the directions d."""
    x, y, z = d[..., 0], d[..., 1], d[..., 2]
    return np.stack([0.282095 * np.ones_like(x), 0.488603 * y, 0.488603 * z, 0.488603 * x, 1.092548 * x * y,
                     1.092548 * y * z, 0.315392 * (3 * z * z - 1), 1.092548 * x * z, 0.546274 * (x * x - y * y)], -1)


def sphere_directions():
    """The world direction each texel of a sphere map stands for (textureSphere()'s inverse), rows top down, and
    which texels lie on the map's disk."""
    s = (np.arange(SIZE) + 0.5) / SIZE * 2 - 1
    px, py = np.meshgrid(s, -s)
    r2 = px * px + py * py
    inside = r2 <= 1.0
    r2 = np.minimum(r2, 1.0)
    root = np.sqrt(1 - r2)
    view = np.stack([2 * px * root, 2 * py * root, 1 - 2 * r2], -1)
    right = np.cross(LOOK, UP)
    back = -LOOK
    world = view[..., :1] * right + view[..., 1:2] * UP + view[..., 2:] * back
    return world / np.linalg.norm(world, axis=-1, keepdims=True), inside


def maps(folder, mean=DIFFUSE_MEAN, saturation=1.0, tint=(1.0, 1.0, 1.0)):
    directions, weighted = load(folder)
    colors, weights = weighted[:, :3], weighted[:, 3]
    total = weights.sum()
    # the radiance in harmonics, convolved with the cosine lobe: the light falling on a surface facing each way
    basis = sh9(directions)
    coefficients = basis.T @ colors * (4 * np.pi / total)
    band = np.array([np.pi, 2.094395, 2.094395, 2.094395, 0.785398, 0.785398, 0.785398, 0.785398, 0.785398])
    world, inside = sphere_directions()
    irradiance = sh9(world) @ (coefficients * band[:, None])
    # the sun straight on: the views don't see it
    toward, sun = np.load(os.path.join(folder, "sun.npy"))
    facing = world @ toward
    irradiance += np.maximum(facing, 0)[..., None] * sun
    # the light a white surface sends back, its colours and shading as the bake has them, as bright as the
    # original levels' (the normals the shaders look up by face the camera, the inner part of the map)
    diffuse = np.clip(irradiance / np.pi, 0, None)
    looked_up = inside & (world @ -LOOK > 0)
    diffuse *= mean / diffuse[looked_up].mean()
    grey = diffuse.mean(axis=-1, keepdims=True)
    diffuse = (grey + (diffuse - grey) * saturation) * np.array(tint)
    # the mirror image: the radiance seen that way, softened a little, from the cube texels nearest each direction
    flat = world.reshape(-1, 3)
    radiance = colors / weights[:, None]
    nearest = np.empty(len(flat), np.int64)
    for start in range(0, len(flat), 4096):
        chunk = flat[start:start + 4096]
        nearest[start:start + 4096] = np.argmax(chunk @ directions.T, axis=1)
    specular = radiance[nearest].reshape(SIZE, SIZE, 3)
    # the brightest things round (the lava, the glows) a sheen, not a coat of their colour over the blocks' sides
    specular = np.minimum(specular, np.percentile(specular[inside], 90))
    specular *= SPECULAR_MEAN / specular[inside].mean()
    grey = specular.mean(axis=-1, keepdims=True)
    specular = (grey + (specular - grey) * saturation) * np.array(tint)
    # and the sun's disk in it, with a soft glow round
    if sun.max() > 0:
        glint = sun / sun.max()
        specular += (np.exp(-((1 - facing) / 0.0015)) * 0.9 + np.exp(-((1 - facing) / 0.03)) * 0.12)[..., None] * glint
    return diffuse, specular, inside, world, toward, sun


def save(rgb, inside, path):
    rgb = np.where(inside[..., None], rgb, rgb * 0.0 + rgb[inside].mean(axis=0))
    Image.fromarray(np.round(np.clip(rgb, 0, 1) * 255).astype(np.uint8)).save(path, compression="tga_rle")


def apply(world):
    folder = os.path.join(OUT, f"world{world}", "probe")
    if not os.path.exists(os.path.join(folder, "probe_px.npy")):
        return False
    light = LIGHTING.get(world, {})
    diffuse, specular, inside, directions, toward, sun = maps(folder, *light.get("probe", (DIFFUSE_MEAN, 1.0)),
                                                              tint=light.get("probe_tint", (1.0, 1.0, 1.0)))
    target = os.path.join(STORY, f"world{world}")
    save(diffuse, inside, os.path.join(target, "diffuse_level.tga"))
    save(specular, inside, os.path.join(target, "specular_level.tga"))
    # the players' shine a little duller than the blocks', the bombs' (polished black) brighter
    save(specular * 0.8, inside, os.path.join(target, "specular_level2.tga"))
    save(specular * 2.0, inside, os.path.join(target, "diffuse_bomb.tga"))
    # the scenery's shadow on the blocks (lighting.cookie()), in the shade's colour: lit by the sky alone, as much
    # of the probe's light as the side away from the sun gets
    cookie = os.path.join(folder, "cookie.npy")
    if os.path.exists(cookie):
        lit = np.load(cookie)[..., None]
        away = directions @ toward < -0.2
        shade = np.clip(diffuse[inside & away].mean(axis=0) / diffuse[inside].max(axis=0).clip(1e-3), 0.35, 0.9)             if sun.max() > 0 else np.ones(3)
        Image.fromarray(np.round(np.clip(shade + (1 - shade) * lit, 0, 1) * 255).astype(np.uint8)).save(
            os.path.join(target, "shadow-cookie.tga"), compression="tga_rle")
    return True


if __name__ == "__main__":
    for world in [int(a) for a in sys.argv[1:]] or range(1, 9):
        if apply(world):
            print("RESULT probes world", world)
