# The intro's night sky behind the mountains, all round as an equirectangular map: a glow above the horizon
# fading to a deep blue overhead, the milky way's soft band across it and a few thin clouds low down. The stars
# and the moon are drawn sharp by the sky's shader (nightsky-frag.glsl), this is the soft part under them.
#   python build_sky.py <out dir>
# writes sky.tga, 2048 x 1024, uncompressed, its bottom row the nadir. Azimuth 0 (u 0.5) points along +x, +y (where
# the camera looks) is u 0.75; z is up.
import os
import struct
import sys

import numpy as np

WIDTH, HEIGHT = 2048, 1024
HORIZON = np.array([0.16, 0.13, 0.30])
LOW = np.array([0.05, 0.07, 0.19])
ZENITH = np.array([0.008, 0.012, 0.05])
MILKY = np.array([0.30, 0.30, 0.45])
CLOUD = np.array([0.10, 0.10, 0.22])

rng = np.random.default_rng(11)


def fbm(octaves, base, shape):
    """Smooth value noise in 0..1 that wraps round in u, summed over octaves."""
    total = np.zeros(shape)
    amplitude, weight = 1.0, 0.0
    for o in range(octaves):
        cells = (base * 2 ** o, max(base * 2 ** o // 2, 2))
        grid = rng.random((cells[1] + 1, cells[0]))
        u = np.linspace(0, cells[0], shape[1], endpoint=False)
        v = np.linspace(0, cells[1], shape[0])
        u0 = np.floor(u).astype(int)
        v0 = np.minimum(np.floor(v).astype(int), cells[1] - 1)
        fu = (u - u0)[None, :]
        fv = (v - v0)[:, None]
        fu, fv = fu * fu * (3 - 2 * fu), fv * fv * (3 - 2 * fv)
        a = grid[v0][:, u0]
        b = grid[v0][:, (u0 + 1) % cells[0]]
        c = grid[v0 + 1][:, u0]
        d = grid[v0 + 1][:, (u0 + 1) % cells[0]]
        total += amplitude * ((a * (1 - fu) + b * fu) * (1 - fv) + (c * (1 - fu) + d * fu) * fv)
        weight += amplitude
        amplitude *= 0.5
    return total / weight


def smoothstep(a, b, x):
    t = np.clip((x - a) / (b - a), 0.0, 1.0)
    return t * t * (3 - 2 * t)


def sky():
    v = (np.arange(HEIGHT) + 0.5) / HEIGHT
    u = (np.arange(WIDTH) + 0.5) / WIDTH
    elevation = (v - 0.5) * np.pi  # -pi/2 .. pi/2
    azimuth = (u - 0.5) * 2 * np.pi
    el = elevation[:, None] * np.ones((1, WIDTH))
    az = np.ones((HEIGHT, 1)) * azimuth[None, :]
    direction = np.stack([np.cos(el) * np.cos(az), np.cos(el) * np.sin(az), np.sin(el)], axis=-1)

    # the gradient: a glow just above the horizon, deep blue overhead, dark below
    up = np.clip(el / (np.pi / 2), 0.0, 1.0)[..., None]
    color = HORIZON + (LOW - HORIZON) * smoothstep(0.0, 0.12, up) + (ZENITH - LOW) * smoothstep(0.08, 0.7, up)
    color = np.where(el[..., None] < 0, HORIZON * 0.6, color)

    # the milky way: a band round a great circle tilted across the sky behind the castle, frayed by noise
    pole = np.array([0.55, -0.25, 0.8])
    pole /= np.linalg.norm(pole)
    band = np.abs(direction @ pole)
    detail = fbm(6, 8, (HEIGHT, WIDTH))
    width = 0.2 + 0.12 * (detail - 0.5)
    milky = (1.0 - smoothstep(0.0, width, band)) ** 1.5 * (0.35 + 1.1 * smoothstep(0.25, 0.8, detail))
    # a few dark lanes of dust along its middle
    lanes = fbm(5, 16, (HEIGHT, WIDTH))
    dust = smoothstep(0.62, 0.78, lanes) * (1.0 - smoothstep(0.0, 0.06, band))
    milky = np.clip(milky * (1.0 - dust * 0.6), 0.0, 1.0) * smoothstep(0.02, 0.25, up[..., 0])
    color = color + MILKY * milky[..., None] * 0.7

    # thin clouds low down, lit faintly from above
    clouds = smoothstep(0.55, 0.8, fbm(5, 6, (HEIGHT, WIDTH))) * smoothstep(0.02, 0.1, up[..., 0])
    clouds *= 1.0 - smoothstep(0.15, 0.4, up[..., 0])
    color = color * (1 - clouds[..., None] * 0.6) + CLOUD * clouds[..., None] * 0.6
    return np.clip(color, 0.0, 1.0)


def write_tga(path, rgb):
    """Uncompressed 24 bit, bottom row first as the array's row 0 is."""
    height, width = rgb.shape[:2]
    data = (rgb[..., ::-1] * 255 + 0.5).astype(np.uint8)
    with open(path, "wb") as f:
        f.write(struct.pack("<BBBHHBHHHHBB", 0, 0, 2, 0, 0, 0, 0, 0, width, height, 24, 0))
        f.write(data.tobytes())


out = os.path.abspath(sys.argv[1]) if len(sys.argv) > 1 else os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                                                          "out")
os.makedirs(out, exist_ok=True)
write_tga(os.path.join(out, "sky.tga"), sky())
print("RESULT wrote sky.tga")
