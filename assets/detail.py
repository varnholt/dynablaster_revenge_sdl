# Fine surface detail for the game's glTF shader: small seamless grey images the shader tiles in world space over
# a material's colour, and a mask saying where each one shows. The images average mid grey, so they only add
# detail and leave the colours as they are. Sets of four: "nature" (grass tufts, soil, cracked rock, pebbles),
# "castle" (worn tiles, brick grit, brushed metal, stone) and "ship" (wood grain along x, canvas, old paint,
# metal).
#
#   images(kind)        the set's four images, grey numpy arrays
#   mask(kind, ...)     a material's mask from its colour texture and slopes
#   repeats(kind)       how often each image repeats per unit
#   embed(glb, ...)     puts the masks and images into a .glb and points materials at them through their extras:
#                       "detail": [mask, image 0..3, repeats per unit 0..3, strength] (see GltfModel::Primitive)
#
# Plain numpy, used from Blender's python by the build scripts.
import struct
import zlib

import numpy as np

import glb

SIZE = 512
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
    image = tuft * 0.7 + shades[nearest] * 0.3
    for _ in range(5000):
        x, y = rng.uniform(0, size, 2)
        angle = rng.uniform(0, 2 * np.pi)
        length = rng.uniform(8, 18)
        for t in np.linspace(0, 1, int(length)):
            px = int(x + np.cos(angle) * length * t) % size
            py = int(y + np.sin(angle) * length * t) % size
            image[py, px] += 0.35 * t
    return normalised(image, 0.15)


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


def leaves(rng):
    """Overlapping leaves seen from anywhere: small pointed ovals every which way, each a little lighter or darker,
    a darker midrib, shadow where they overlap."""
    size = SIZE
    image = np.full((size, size), -1.2)
    y, x = np.mgrid[-20:21, -20:21].astype(float)
    for _ in range(5200):
        cx, cy = rng.integers(0, size, 2)
        a = rng.uniform(0, np.pi)
        length, width = rng.uniform(12, 18), rng.uniform(5, 7)
        u = x * np.cos(a) + y * np.sin(a)
        v = -x * np.sin(a) + y * np.cos(a)
        inside = (u / length) ** 2 + (v / (width * (1 - 0.5 * (u / length) ** 2))) ** 2 < 1
        shade = rng.uniform(-0.8, 0.8) + u / length * 0.4 - (np.abs(v) < 0.6) * 0.6
        rows, cols = (cy + y.astype(int)) % size, (cx + x.astype(int)) % size
        image[rows[inside], cols[inside]] = shade[inside] if np.ndim(shade) else shade
    return normalised(image, 0.17)


def bark(rng):
    """Bark: fibres running up, furrows between them."""
    size = SIZE
    noise = rng.standard_normal((size, size))
    kernel = np.ones(40) / 40
    fibres = np.real(np.fft.ifft(np.fft.fft(noise, axis=0) * np.fft.fft(kernel, size)[:, None], axis=0))
    furrows = np.sin(np.arange(size)[None, :] * 2 * np.pi / 14 + periodic_noise(rng, size, 5) * 2.0)
    image = fibres / fibres.std() * 0.6 - np.clip(furrows - 0.6, 0, 1) * 2.5
    return normalised(image, 0.15)


def worn(rng):
    """Worn tiles: fine speckle, a few scratches, chipped spots."""
    size = SIZE
    image = periodic_noise(rng, size, 200) * 0.5 + fbm(rng, size, 8) * 0.3
    for _ in range(140):
        x, y = rng.uniform(0, size, 2)
        angle = rng.uniform(0, np.pi)
        length = rng.uniform(15, 70)
        for t in np.linspace(0, 1, int(length)):
            image[int(y + np.sin(angle) * length * t) % size, int(x + np.cos(angle) * length * t) % size] -= 1.2
    f1, _, _ = cells(rng, size, 40)
    image -= np.clip(1.0 - f1 / 5.0, 0, 1) * 2.0
    return normalised(image, 0.14)


def grit(rng):
    """Fired clay: coarse grain, pits, a little soot."""
    size = SIZE
    image = periodic_noise(rng, size, 160) * 0.7 + periodic_noise(rng, size, 40) * 0.4 + fbm(rng, size, 5) * 0.3
    f1, _, _ = cells(rng, size, 160)
    image -= np.clip(1.0 - f1 / 3.0, 0, 1) * 1.5
    return normalised(image, 0.16)


def brushed(rng):
    """Brushed metal: long fine streaks one way, a few scuffs across."""
    size = SIZE
    streaks = rng.standard_normal(size)
    image = np.repeat(streaks[None, :], size, axis=0) * 0.6
    image += periodic_noise(rng, size, 30) * 0.5
    for _ in range(60):
        x, y = rng.uniform(0, size, 2)
        angle = rng.uniform(0, np.pi)
        for t in np.linspace(0, 1, 40):
            image[int(y + np.sin(angle) * 40 * t) % size, int(x + np.cos(angle) * 40 * t) % size] += 1.0
    return normalised(image, 0.12)


def stone(rng):
    """Dressed stone: grain, faint veins, a few pores."""
    size = SIZE
    image = fbm(rng, size, 12) * 0.6 + periodic_noise(rng, size, 180) * 0.4
    vein = np.abs(fbm(rng, size, 3))
    image -= np.clip(1.0 - vein / 0.08, 0, 1) * 0.8
    return normalised(image, 0.13)


def wood(rng):
    """Planed wood along u: long fine grain lines that drift a little, a few knots."""
    size = SIZE
    v = np.arange(size)[:, None].astype(float)
    drift = periodic_noise(rng, size, 3) * 4.0
    grain = np.sin((v + drift) * 2 * np.pi / 9.0)
    # streaks: noise averaged along u
    streak = rng.standard_normal((size, size))
    kernel = np.ones(48) / 48
    streak = np.real(np.fft.ifft(np.fft.fft(streak, axis=1) * np.fft.fft(kernel, size)[None, :], axis=1))
    image = grain * 0.45 + streak / streak.std() * 0.5
    f1, _, _ = cells(rng, size, 5)
    image -= np.clip(1.0 - f1 / 8.0, 0, 1) * 1.5
    return normalised(image, 0.13)


def canvas(rng):
    """Sailcloth: a fine weave, faint stains."""
    size = SIZE
    y, x = np.mgrid[0:size, 0:size].astype(float)
    weave = np.sin(x * 2 * np.pi / 4.0) * np.sin(y * 2 * np.pi / 4.0)
    image = weave * 0.5 + fbm(rng, size, 3) * 0.6 + periodic_noise(rng, size, 60) * 0.2
    return normalised(image, 0.1)


def paint(rng):
    """Old paint on wood: brush streaks, chips where the wood shows through."""
    size = SIZE
    streak = np.cumsum(rng.standard_normal((size, size)) * 0.3, axis=1)
    streak = (streak - streak.mean(axis=1, keepdims=True)) / (streak.std(axis=1, keepdims=True) + 1e-6)
    image = streak * 0.3 + periodic_noise(rng, size, 120) * 0.3
    chips = fbm(rng, size, 6)
    image -= (chips > 1.3) * 2.0
    return normalised(image, 0.13)


# --- the sets: their images, repeats per unit (tile), and how a texture's colours pick them --------------

def nature_mask(rgb, slope):
    """Grass where green, soil where brown, rock where steep or grey and dark, pebbles where pale and grey,
    nothing on water."""
    r, g, b = rgb[..., 0], rgb[..., 1], rgb[..., 2]
    top, saturation = _value_saturation(rgb)
    green = np.clip((g - np.maximum(r, b)) / (top + 1e-6) * 4.0, 0, 1) * np.clip(saturation * 3.0, 0, 1)
    brown = np.clip((r - b) / (top + 1e-6) * 2.5, 0, 1) * np.clip(1.0 - green, 0, 1) * (r >= g * 0.9)
    grey = np.clip(1.0 - saturation * 3.0, 0, 1)
    steep = np.clip((slope - 0.5) / 0.6, 0, 1)
    light = np.clip((top - 0.45) * 4.0, 0, 1)
    weights = np.stack([green, brown, grey * (1.0 - light) + steep, grey * light], axis=-1)
    weights[..., 0] *= 1.0 - steep
    weights[..., 1] *= 1.0 - steep
    blue = np.clip((b - np.maximum(r, g)) / (top + 1e-6) * 4.0, 0, 1)
    return _normalise(weights * (1.0 - blue)[..., None])


def castle_mask(rgb, slope):
    """Worn tiles where coloured (purples, blues, greens), grit where red or brown (brick, clay), brushed metal
    where grey and dark, stone where grey and light."""
    r, g, b = rgb[..., 0], rgb[..., 1], rgb[..., 2]
    top, saturation = _value_saturation(rgb)
    coloured = np.clip(saturation * 3.0 - 0.3, 0, 1)
    warm = np.clip((r - np.maximum(g, b)) / (top + 1e-6) * 3.0, 0, 1)
    grey = 1.0 - coloured
    light = np.clip((top - 0.45) * 4.0, 0, 1)
    weights = np.stack([coloured * (1.0 - warm), coloured * warm, grey * (1.0 - light), grey * light], axis=-1)
    return _normalise(weights)


def ship_mask(rgb, slope):
    """Canvas where pale and grey (sails), wood where brown, paint where strongly coloured otherwise, metal where
    dark grey."""
    r, g, b = rgb[..., 0], rgb[..., 1], rgb[..., 2]
    top, saturation = _value_saturation(rgb)
    grey = np.clip(1.0 - saturation * 3.0, 0, 1)
    light = np.clip((top - 0.5) * 4.0, 0, 1)
    brown = np.clip((r - b) / (top + 1e-6) * 2.5, 0, 1) * (r >= g) * (g >= b * 0.9) * (1.0 - grey)
    coloured = np.clip(saturation * 3.0 - 0.6, 0, 1) * (1.0 - brown)
    weights = np.stack([brown, grey * light, coloured, grey * (1.0 - light)], axis=-1)
    return _normalise(weights)


SETS = {
    "nature": ([grass, soil, rock, pebbles], [0.5, 0.6, 0.3, 0.8], nature_mask),
    "castle": ([worn, grit, brushed, stone], [0.5, 0.8, 0.6, 0.4], castle_mask),
    "ship": ([wood, canvas, paint, brushed], [0.45, 1.2, 0.6, 0.8], ship_mask),
    # the nature worlds' props: leaves on the green, bark on the brown
    "foliage": ([leaves, bark, rock, pebbles], [1.6, 1.4, 0.3, 0.8], nature_mask),
}


def _value_saturation(rgb):
    top = np.max(rgb, axis=-1)
    return top, (top - np.min(rgb, axis=-1)) / (top + 1e-6)


def _normalise(weights):
    total = weights.sum(axis=-1, keepdims=True)
    return np.where(total > 1.0, weights / np.maximum(total, 1e-6), weights)


def images(kind="nature", seed=11):
    """The set's four images, grey numpy arrays."""
    rng = np.random.default_rng(seed)
    return [make(rng) for make in SETS[kind][0]]


def mask(kind, rgb, slope):
    """Per texel of a colour texture (rgb 0..1, rows bottom up like Blender's) and its slope (rise over run) the
    weight of each of the set's images."""
    return SETS[kind][2](rgb, slope)


STRENGTHS = {"ship": 0.5, "nature": 0.45, "foliage": 0.5}

# the story worlds' sets, "nature" for the others
STORY_WORLDS = {6: "castle", 7: "castle", 8: "castle"}


def story_world(world, props=False):
    """The detail set of a story world, of its props (trees, bushes) with props."""
    kind = STORY_WORLDS.get(world, "nature")
    return "foliage" if props and kind == "nature" else kind


def strength(kind):
    """How strongly the set's detail shows."""
    return STRENGTHS.get(kind, STRENGTH)


def repeats(kind):
    """How often each of the set's images repeats per unit."""
    return SETS[kind][1]


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


def base_color(gltf, binary, material, size):
    """A material's base colour texture from a glb (glb.read()) as rgb 0..1, size by size, rows bottom up as
    mask() takes them. Needs Pillow, so not in Blender's python."""
    import io

    from PIL import Image

    texture = gltf["textures"][material["pbrMetallicRoughness"]["baseColorTexture"]["index"]]
    view = gltf["bufferViews"][gltf["images"][texture["source"]]["bufferView"]]
    start = view.get("byteOffset", 0)
    image = Image.open(io.BytesIO(bytes(binary[start:start + view["byteLength"]]))).convert("RGB")
    image = image.resize((size, size), Image.BILINEAR)
    return np.asarray(image, dtype=np.float32)[::-1] / 255.0


def embed(path, masks, layers, repeats, strength=STRENGTH):
    """Adds the detail images and masks ({material name: mask}, rows bottom up, rgba weights) to the glb at path
    and points each material at its mask and the images; a glb that has them already stays as it is. strength
    is one for all or {material name: strength}."""
    gltf, binary = glb.read(path)
    if any("detail" in m.get("extras", {}) for m in gltf.get("materials", []) if m.get("name") in masks):
        return

    def add_image(content):
        while len(binary) % 4:
            binary.append(0)
        gltf.setdefault("bufferViews", []).append({"buffer": 0, "byteOffset": len(binary), "byteLength": len(content)})
        binary.extend(content)
        gltf.setdefault("images", []).append({"bufferView": len(gltf["bufferViews"]) - 1, "mimeType": "image/png"})
        return len(gltf["images"]) - 1

    # the images are grey, one channel each keeps the file small (the game reads them as rgba either way)
    shared = [add_image(png(to_bytes(layer))) for layer in layers]
    for name, mask in masks.items():
        # gltf images run top down
        index = add_image(png(to_bytes(mask[::-1])))
        material = next(m for m in gltf["materials"] if m.get("name") == name)
        amount = strength.get(name, STRENGTH) if isinstance(strength, dict) else strength
        material.setdefault("extras", {})["detail"] = [index] + shared + list(repeats) + [amount]
    glb.write(path, gltf, binary)
