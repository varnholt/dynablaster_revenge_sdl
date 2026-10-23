# The pirate ship's block skins: cube crosses of 3 x 4 cells (171 x 128 px each) like the other levels'
# block-unwrap.tga (indestructible) and stone-unwrap.tga (destructible), alpha masks the reflection.
#   python make_textures.py <out dir>
import os
import sys

import numpy as np
from PIL import Image, ImageFilter

OUT = sys.argv[1] if len(sys.argv) > 1 else "."
FACE = 256
rng = np.random.default_rng(3)


def noise(size, scale, octaves=4, seed=0):
    """Smooth value noise in 0..1."""
    r = np.random.default_rng(seed)
    total = np.zeros((size, size))
    amplitude, norm = 1.0, 0.0
    for o in range(octaves):
        cells = max(2, int(scale * 2 ** o))
        grid = r.random((cells + 1, cells + 1))
        img = Image.fromarray((grid * 255).astype(np.uint8)).resize((size, size), Image.BICUBIC)
        total += amplitude * np.asarray(img) / 255.0
        norm += amplitude
        amplitude *= 0.5
    return total / norm


def grain(size, along_x, seed):
    """Wood grain: noise stretched along the planks."""
    n = noise(size * 4, 6, 5, seed)
    img = Image.fromarray((n * 255).astype(np.uint8))
    img = img.resize((size, size // 8) if along_x else (size // 8, size), Image.BICUBIC).resize((size, size),
                                                                                               Image.BICUBIC)
    return np.asarray(img) / 255.0


def srgb(c):
    return np.array(c, dtype=np.float64)


def crate_face():
    """A light cargo crate: horizontal planks in a frame with a diagonal brace."""
    s = FACE
    y, x = np.mgrid[0:s, 0:s] / s
    wood = srgb((0.80, 0.60, 0.34))
    g = grain(s, True, 1)
    color = wood[None, None, :] * (0.82 + 0.3 * g[..., None])
    # four planks, each its own tone, dark gaps between them
    plank = np.floor(y * 4).astype(int)
    tone = np.array([1.0, 0.9, 1.05, 0.93])[plank]
    color *= tone[..., None]
    gap = np.abs((y * 4) - np.round(y * 4)) < 0.03
    color[gap] *= 0.45
    alpha = np.full((s, s), 0.1)

    frame = srgb((0.56, 0.37, 0.19))
    border = 0.13
    in_frame = (x < border) | (x > 1 - border) | (y < border) | (y > 1 - border)
    # the brace from the lower left to the upper right corner
    d = np.abs((x - border) - (1 - border - y)) / np.sqrt(2)
    brace = (d < 0.07) & ~in_frame
    fg = grain(s, False, 2)
    frame_color = frame[None, None, :] * (0.85 + 0.3 * fg[..., None])
    for mask in (in_frame, brace):
        color[mask] = frame_color[mask]
    # bevels: light on the upper and left edges of the frame, dark on the others
    edge = np.minimum.reduce([x, 1 - x, y, 1 - y])
    color *= (1.0 - 0.35 * np.clip(1 - edge / 0.03, 0, 1))[..., None]
    inner = np.minimum.reduce([x - border, 1 - border - x, y - border, 1 - border - y])
    ring = (inner < 0) & (inner > -0.02)
    color[ring] *= 0.7
    color[brace & (d > 0.055)] *= 0.75
    # nails at the frame's corners and along the brace
    nails = [(0.065, 0.065), (0.935, 0.065), (0.065, 0.935), (0.935, 0.935), (0.5, 0.065), (0.5, 0.935),
             (0.065, 0.5), (0.935, 0.5)]
    for nx, ny in nails:
        r = np.hypot(x - nx, y - ny)
        head = r < 0.022
        color[head] = srgb((0.22, 0.2, 0.19)) + 0.25 * np.clip(1 - np.hypot(x - nx + 0.007, y - ny + 0.007) / 0.012,
                                                                0, 1)[head][:, None]
        alpha[head] = 0.6
    return np.clip(color, 0, 1), alpha


def iron_face():
    """The ship's heavy blocks: tarred vertical planks bound with riveted iron bands."""
    s = FACE
    y, x = np.mgrid[0:s, 0:s] / s
    wood = srgb((0.30, 0.17, 0.10))
    g = grain(s, False, 4)
    color = wood[None, None, :] * (0.75 + 0.45 * g[..., None])
    plank = np.floor(x * 3).astype(int)
    color *= np.array([1.0, 0.88, 1.06])[plank][..., None]
    gap = np.abs((x * 3) - np.round(x * 3)) < 0.02
    color[gap] *= 0.4
    alpha = np.full((s, s), 0.08)

    iron = srgb((0.24, 0.25, 0.27))
    n = noise(s, 10, 3, 5)
    border = 0.12
    band = (x < border) | (x > 1 - border) | (y < border) | (y > 1 - border) | (np.abs(y - 0.5) < 0.06)
    iron_color = iron[None, None, :] * (0.75 + 0.5 * n[..., None])
    color[band] = iron_color[band]
    alpha[band] = 0.55
    # bevelled band edges
    edge = np.minimum.reduce([x, 1 - x, y, 1 - y])
    color *= (1.0 - 0.4 * np.clip(1 - edge / 0.025, 0, 1))[..., None]
    for lo, hi in ((border - 0.012, border), (0.44 - 0.012, 0.44), (0.56, 0.56 + 0.012), (1 - border, 1 - border + 0.012)):
        horizontal = (y > lo) & (y < hi) & (x > border) & (x < 1 - border)
        color[horizontal] *= 0.55
    for lo, hi in ((border - 0.012, border), (1 - border, 1 - border + 0.012)):
        vertical = (x > lo) & (x < hi) & (y > border) & (y < 1 - border)
        color[vertical] *= 0.55
    # rivets with a highlight
    rivets = []
    for t in np.linspace(0.06, 0.94, 7):
        rivets += [(t, 0.06), (t, 0.94), (0.06, t), (0.94, t), (t, 0.5)]
    for rx, ry in rivets:
        r = np.hypot(x - rx, y - ry)
        head = r < 0.02
        light = np.clip(1 - np.hypot(x - rx + 0.007, y - ry + 0.007) / 0.013, 0, 1)
        color[head] = (srgb((0.18, 0.18, 0.2)) + 0.55 * light[head][:, None])
        alpha[head] = 0.9
    return np.clip(color, 0, 1), alpha


def cross(face):
    color, alpha = face
    rgba = np.concatenate([color, alpha[..., None]], axis=2)
    img = Image.fromarray((rgba * 255).astype(np.uint8)).filter(ImageFilter.SMOOTH)
    w, h = 512 / 3, 128
    cell = img.resize((round(w), h), Image.LANCZOS)
    out = Image.new("RGBA", (512, 512), (0, 0, 0, 0))
    cells = [(1, 0), (0, 1), (1, 1), (2, 1), (1, 2), (1, 3)]
    for cx, cy in cells:
        out.paste(cell, (round(cx * w), cy * h))
    return out


os.makedirs(OUT, exist_ok=True)
cross(crate_face()).save(os.path.join(OUT, "stone-unwrap.tga"))
cross(iron_face()).save(os.path.join(OUT, "block-unwrap.tga"))
print("wrote", OUT)
