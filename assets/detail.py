# Fine surface detail for the game's glTF shader: small seamless images (grass blades, soil, cracked rock,
# pebbles) the shader tiles in world space over a material's colour, and a mask saying where each one shows.
# The images average mid grey, so they only add detail and leave the colours as they are.
#
#   images()            the four detail images, numpy rgb arrays (LAYERS order)
#   ground_mask(...)    a ground's mask from its colour texture and slopes
#   embed(glb, ...)     puts the masks and images into a .glb and points materials at them through their extras:
#                       "detail": [mask, image 0..3, repeats per unit 0..3, strength] (see GltfModel::Primitive)
#
# Plain numpy, used from Blender's python by the build scripts.
import json
import struct
import zlib

import numpy as np

SIZE = 512
LAYERS = ["grass", "soil", "rock", "pebbles"]
REPEATS = [0.5, 0.6, 0.3, 0.8]  # per unit (tile): a grass image covers a tile, a rock image three
STRENGTH = 0.6


# --- seamless noise ---------------------------------------------------------------------------------

def periodic_noise(rng, size, scale, falloff=1.0):
    """Noise that tiles: white noise low passed in frequency space, features about size / scale across."""
    white = rng.standard_normal((size, size))
    fx = np.fft.fftfreq(size)[None, :]
    fy = np.fft.fftfreq(size)[:, None]
    radius = np.sqrt(fx * fx + fy * fy) * size / scale
    spectrum = np.fft.fft2(white) * np.exp(-radius ** 2 * falloff)
    out = np.real(np.fft.ifft2(spectrum))
    return (out - out.mean()) / (out.std() + 1e-9)


def fbm(rng, size, scale, octaves=4):
    total, amp = np.zeros((size, size)), 1.0
    for o in range(octaves):
        total += amp * periodic_noise(rng, size, scale * 2 ** o)
        amp *= 0.5
    return total / total.std()


def cells(rng, size, count):
    """Distance to the nearest and second nearest of count random points, wrapping round the edges."""
    points = rng.uniform(0, size, (count, 2))
    y, x = np.mgrid[0:size, 0:size].astype(float)
    f1 = np.full((size, size), 1e9)
    f2 = np.full((size, size), 1e9)
    nearest = np.zeros((size, size), int)
    for i, (px, py) in enumerate(points):
        dx = np.abs(x - px)
        dy = np.abs(y - py)
        d = np.hypot(np.minimum(dx, size - dx), np.minimum(dy, size - dy))
        closer = d < f1
        f2 = np.where(closer, f1, np.minimum(f2, d))
        nearest = np.where(closer, i, nearest)
        f1 = np.where(closer, d, f1)
    return f1, f2, nearest


def normalised(value, contrast):
    """Mean 0.5, the given spread."""
    value = (value - value.mean()) / (value.std() + 1e-9)
    return np.clip(0.5 + value * contrast, 0.0, 1.0)


# --- the images -------------------------------------------------------------------------------------

def grass(rng):
    """Tufts seen from above, sized for the game's camera (a tile is some 45 pixels on screen): soft clumps of
    blades, each a little lighter or darker, lit on top, a few brighter blades sticking out."""
    size = SIZE
    f1, _, nearest = cells(rng, size, 1400)
    tuft = np.clip(1.0 - f1 / 10.0, 0, 1) ** 1.5
    shades = rng.uniform(-0.6, 0.6, nearest.max() + 1)
    image = tuft * 0.9 + shades[nearest] * 0.45
    for _ in range(5000):
        x, y = rng.uniform(0, size, 2)
        angle = rng.uniform(0, 2 * np.pi)
        length = rng.uniform(8, 18)
        for t in np.linspace(0, 1, int(length)):
            px = int(x + np.cos(angle) * length * t) % size
            py = int(y + np.sin(angle) * length * t) % size
            image[py, px] += 0.5 * t
    return normalised(image, 0.22)


def soil(rng):
    """Crumbly earth: fine grains over larger clods, a few small stones."""
    size = SIZE
    image = fbm(rng, size, 10) * 0.6 + periodic_noise(rng, size, 120) * 0.5
    f1, _, _ = cells(rng, size, 90)
    image += np.clip(1.0 - f1 / 6.0, 0, 1) * 1.6
    return normalised(image, 0.16)


def rock(rng):
    """Weathered stone: broad patches, cracks between plates, grain."""
    size = SIZE
    f1, f2, _ = cells(rng, size, 26)
    crack = np.clip((f2 - f1) / 4.0, 0, 1)
    image = fbm(rng, size, 4) * 0.7 + periodic_noise(rng, size, 90) * 0.35 - (1.0 - crack) * 1.6
    return normalised(image, 0.18)


def pebbles(rng):
    """Small round stones packed together, each its own shade, darker in the gaps."""
    size = SIZE
    f1, f2, nearest = cells(rng, size, 260)
    dome = np.clip((f2 - f1) / 9.0, 0, 1) ** 0.5
    shades = rng.uniform(-1, 1, nearest.max() + 1)
    image = dome * 1.5 + shades[nearest] * 0.6 + periodic_noise(rng, size, 140) * 0.2
    return normalised(image, 0.2)


def images(seed=11):
    rng = np.random.default_rng(seed)
    grey = [grass(rng), soil(rng), rock(rng), pebbles(rng)]
    return [np.repeat(g[..., None], 3, axis=2) for g in grey]


# --- masks ------------------------------------------------------------------------------------------

def ground_mask(rgb, slope):
    """Per texel of a ground's colour texture (rgb 0..1, rows bottom up like Blender's) and its slope (rise over
    run) the weight of each LAYERS image: grass where green, soil where brown, rock where steep or grey and dark,
    pebbles where pale and grey."""
    r, g, b = rgb[..., 0], rgb[..., 1], rgb[..., 2]
    top = np.max(rgb, axis=-1)
    bottom = np.min(rgb, axis=-1)
    saturation = (top - bottom) / (top + 1e-6)
    green = np.clip((g - np.maximum(r, b)) / (top + 1e-6) * 4.0, 0, 1) * np.clip(saturation * 3.0, 0, 1)
    brown = np.clip((r - b) / (top + 1e-6) * 2.5, 0, 1) * np.clip(1.0 - green, 0, 1) * (r >= g * 0.9)
    grey = np.clip(1.0 - saturation * 3.0, 0, 1)
    steep = np.clip((slope - 0.5) / 0.6, 0, 1)
    light = np.clip((top - 0.45) * 4.0, 0, 1)
    weights = np.stack([green, brown, grey * (1.0 - light) + steep, grey * light], axis=-1)
    weights[..., 0] *= 1.0 - steep
    weights[..., 1] *= 1.0 - steep
    # water and anything bluish keeps its colour plain
    blue = np.clip((b - np.maximum(r, g)) / (top + 1e-6) * 4.0, 0, 1)
    weights *= (1.0 - blue)[..., None]
    total = weights.sum(axis=-1, keepdims=True)
    return np.where(total > 1.0, weights / np.maximum(total, 1e-6), weights)


# --- glb --------------------------------------------------------------------------------------------

def png(pixels):
    """pixels: rows top down, uint8 grey, rgb or rgba."""
    if pixels.ndim == 2:
        pixels = pixels[..., None]
    height, width, channels = pixels.shape
    kind = {1: 0, 3: 2, 4: 6}[channels]
    raw = b"".join(b"\x00" + pixels[row].tobytes() for row in range(height))

    def chunk(tag, data):
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    header = struct.pack(">IIBBBBB", width, height, 8, kind, 0, 0, 0)
    return b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", header) + chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b"")


def to_bytes(image):
    return (np.clip(image, 0, 1) * 255 + 0.5).astype(np.uint8)


def embed(path, masks, layers=None, repeats=REPEATS, strength=STRENGTH):
    """Adds the detail images and masks ({material name: mask}, rows bottom up, rgba weights) to the glb at path
    and points each material at its mask and the images."""
    layers = layers if layers is not None else images()
    with open(path, "rb") as f:
        data = f.read()
    length = struct.unpack_from("<I", data, 12)[0]
    gltf = json.loads(data[20:20 + length])
    binary = bytearray(data[20 + length + 8:]) if len(data) > 20 + length else bytearray()

    def add_image(content):
        while len(binary) % 4:
            binary.append(0)
        gltf.setdefault("bufferViews", []).append({"buffer": 0, "byteOffset": len(binary), "byteLength": len(content)})
        binary.extend(content)
        gltf.setdefault("images", []).append({"bufferView": len(gltf["bufferViews"]) - 1, "mimeType": "image/png"})
        return len(gltf["images"]) - 1

    # the images are grey, one channel each keeps the file small (the game reads them as rgba either way)
    shared = [add_image(png(to_bytes(layer[..., 0] if layer.ndim == 3 else layer))) for layer in layers]
    for name, mask in masks.items():
        # gltf images run top down
        index = add_image(png(to_bytes(mask[::-1])))
        material = next(m for m in gltf["materials"] if m.get("name") == name)
        material.setdefault("extras", {})["detail"] = [index] + shared + list(repeats) + [strength]
    while len(binary) % 4:
        binary.append(0)
    gltf["buffers"][0]["byteLength"] = len(binary)

    text = json.dumps(gltf, separators=(",", ":")).encode()
    text += b" " * (-len(text) % 4)
    out = struct.pack("<III", 0x46546C67, 2, 12 + 8 + len(text) + 8 + len(binary))
    out += struct.pack("<II", len(text), 0x4E4F534A) + text
    out += struct.pack("<II", len(binary), 0x004E4942) + bytes(binary)
    with open(path, "wb") as f:
        f.write(out)
