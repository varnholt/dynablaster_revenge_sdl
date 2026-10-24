# Grass for the story mode's meadows: clumps of crossed quads (a star seen from above), each showing a cutout image
# of grass blades. The images sit in one atlas, a column per shade of the ground the clumps stand on (a clump has
# no vertex colour of its own, the game reads the first one as its sway) and a row per shape.
#
#   atlas(colors)       the atlas for the given ground colours (sRGB), rgba floats, rows bottom up
#   cell(...)           the uv rectangle of a column and row
#   embed(glb, ...)     puts the atlas into a .glb as the material's cutout colour (alphaMode MASK, double sided)
#
# Plain numpy, used from Blender's python by the build scripts.
import numpy as np

import detail
import glb

CELL = 128  # pixels per image: a clump is some 20 pixels wide on screen, the mipmaps do the rest
ROWS = 2  # shapes
CUTOFF = 0.5

# the clumps are lit nearly like the ground: tips a little lighter than the ground under them, the base in its
# own shade
TIP, BASE = 1.25, 0.65


def blades(rng):
    """One image's blades: coverage and how far up each covered pixel is on its blade (0 base, 1 tip), and a shade
    per pixel. Fewer, broader blades than in a real tuft: thin ones only flicker at the game's camera."""
    v, u = np.meshgrid((np.arange(CELL) + 0.5) / CELL, (np.arange(CELL) + 0.5) / CELL, indexing="ij")
    alpha = np.zeros((CELL, CELL))
    along = np.zeros((CELL, CELL))
    shade = np.ones((CELL, CELL))
    for _ in range(22):
        x0 = float(np.clip(rng.normal(0.5, 0.15), 0.15, 0.85))
        # taller in the middle, fanning out to the sides
        height = rng.uniform(0.45, 0.95) * (1.0 - 0.6 * abs(x0 - 0.5))
        lean = rng.normal(0.0, 0.08) + (x0 - 0.5) * 0.5
        width = rng.uniform(0.04, 0.065)
        t = v / height
        center = x0 + lean * t * t
        half = width * 0.5 * np.clip(1.0 - t, 0, 1) ** 0.7
        cover = np.clip((half - np.abs(u - center)) * CELL + 0.5, 0, 1) * (t < 1)
        # later blades in front of the earlier ones
        along = np.where(cover > 0.5, t, along)
        shade = np.where(cover > 0.5, rng.uniform(0.85, 1.1), shade)
        alpha = np.maximum(alpha, cover)
    # a few pixels clear at the image's borders so neighbours in the atlas don't bleed in
    alpha[:, :3] = alpha[:, -3:] = 0
    alpha[-3:, :] = 0
    return alpha, along, shade


def atlas(colors, seed=5):
    """A column per ground colour (sRGB), ROWS shapes each; rgba, rows bottom up. Where it's clear the colour is
    the blades' gradient all the same, so the mipmaps don't darken their edges."""
    rng = np.random.default_rng(seed)
    shapes = [blades(rng) for _ in range(ROWS)]
    image = np.zeros((CELL * ROWS, CELL * len(colors), 4))
    v = ((np.arange(CELL) + 0.5) / CELL)[:, None]
    for column, color in enumerate(colors):
        color = np.asarray(color, float)
        # sunlit tips turn a little yellow
        tip = np.clip(color * TIP + np.array([0.04, 0.03, -0.01]), 0, 1)
        base = color * BASE
        for row, (alpha, along, shade) in enumerate(shapes):
            t = np.where(alpha > 0, along, v)[..., None]
            rgb = (base + (tip - base) * t ** 0.8) * np.where(alpha > 0, shade, 1.0)[..., None]
            cell = image[row * CELL:(row + 1) * CELL, column * CELL:(column + 1) * CELL]
            cell[..., :3] = np.clip(rgb, 0, 1)
            cell[..., 3] = alpha
    return image


def cell(column, columns, row, flip=False):
    """The uv rectangle (u0, v0, u1, v1, Blender's: v up) of an atlas image, mirrored if flip."""
    u0, u1 = column / columns, (column + 1) / columns
    if flip:
        u0, u1 = u1, u0
    return u0, row / ROWS, u1, (row + 1) / ROWS


def embed(path, material_name, image):
    """Makes the material named so in the glb at path a double sided cutout showing image (rgba, rows bottom up)."""
    gltf, binary = glb.read(path)
    material = next(m for m in gltf["materials"] if m.get("name") == material_name)
    content = detail.png(detail.to_bytes(image[::-1]))
    while len(binary) % 4:
        binary.append(0)
    gltf.setdefault("bufferViews", []).append({"buffer": 0, "byteOffset": len(binary), "byteLength": len(content)})
    binary.extend(content)
    gltf.setdefault("images", []).append({"bufferView": len(gltf["bufferViews"]) - 1, "mimeType": "image/png"})
    gltf.setdefault("samplers", []).append({"magFilter": 9729, "minFilter": 9987, "wrapS": 33071, "wrapT": 33071})
    gltf.setdefault("textures", []).append({"source": len(gltf["images"]) - 1, "sampler": len(gltf["samplers"]) - 1})
    material.setdefault("pbrMetallicRoughness", {})["baseColorTexture"] = {"index": len(gltf["textures"]) - 1}
    material["alphaMode"] = "MASK"
    material["alphaCutoff"] = CUTOFF
    material["doubleSided"] = True
    glb.write(path, gltf, binary)
