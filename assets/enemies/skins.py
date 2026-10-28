# Surface patterns painted over the enemies' flat colours, from each texel's position and normal on the model (model
# units): they stay put on the body as it moves, and run over the uv seams without a break.
#   fur      strands combed down the body, clumped, darker at the roots
#   scales   overlapping round scales, dark in the gaps, a light rim along their lower edge
#   slime    a darker core, bubbles in it and glossy streaks running down
#   rubber   smooth and a little glossy, faint mottling
#   skin     blotches and freckles, like a frog's or an octopus'
#   stone    grain, flecks and fine cracks
#   metal    brushed, with dents
#   peel     dimpled like an orange's
#   fabric   a fine weave
#   ribbed   long ribs from top to bottom, a gourd's or an onion's
import numpy as np


def _hash3(i, j, k, seed):
    v = np.sin(i * 127.1 + j * 311.7 + k * 74.7 + seed * 19.3) * 43758.5453
    return v - np.floor(v)


def vnoise(p, seed=0):
    """Value noise at points p (n, 3)."""
    i = np.floor(p)
    f = p - i
    f = f * f * (3 - 2 * f)
    total = 0.0
    for dx in (0, 1):
        for dy in (0, 1):
            for dz in (0, 1):
                w = (f[:, 0] if dx else 1 - f[:, 0]) * (f[:, 1] if dy else 1 - f[:, 1]) * (f[:, 2] if dz else 1 - f[:, 2])
                total = total + w * _hash3(i[:, 0] + dx, i[:, 1] + dy, i[:, 2] + dz, seed)
    return total


def fbm(p, scale, octaves=4, seed=0):
    total, amp, norm = 0.0, 1.0, 0.0
    for o in range(octaves):
        total = total + amp * vnoise(p * scale * 2 ** o, seed + o * 7)
        norm += amp
        amp *= 0.5
    return total / norm


def voronoi(p, scale, seed=0):
    """Distances to the nearest and second nearest cell point, and the nearest point's offset from p."""
    q = p * scale
    i = np.floor(q)
    f1 = np.full(len(p), 9.0)
    f2 = np.full(len(p), 9.0)
    near = np.zeros_like(p)
    for dx in (-1, 0, 1):
        for dy in (-1, 0, 1):
            for dz in (-1, 0, 1):
                c = i + (dx, dy, dz)
                point = c + np.stack([_hash3(c[:, 0], c[:, 1], c[:, 2], seed + k) for k in range(3)], axis=1)
                d = np.linalg.norm(q - point, axis=1)
                closer = d < f1
                f2 = np.where(closer, f1, np.minimum(f2, d))
                near = np.where(closer[:, None], point - q, near)
                f1 = np.minimum(f1, d)
    return f1, f2, near


def smooth(e0, e1, v):
    t = np.clip((v - e0) / (e1 - e0), 0, 1)
    return t * t * (3 - 2 * t)


def _shade(col, factor):
    return col * np.asarray(factor)[:, None]


def _down(nrm):
    """Per point the way down along the surface, where fur lies and slime runs."""
    down = np.array([0.0, 0.0, -1.0]) - nrm * (-nrm[:, 2:3])
    length = np.linalg.norm(down, axis=1, keepdims=True)
    # flat on top or underneath: comb backwards
    return np.where(length > 0.2, down / np.maximum(length, 1e-6), np.array([0.0, 1.0, 0.0]))


def fur(col, pos, nrm, scale=1.0):
    down = _down(nrm)
    # strands: noise stretched far along the combing direction, squeezed across it
    along = np.sum(pos * down, axis=1, keepdims=True)
    across = pos - down * along
    strand = vnoise(across * 35 * scale + along * 4 * scale, 3) * 0.6 + vnoise(across * 70 * scale + along * 6 * scale, 4) * 0.4
    clump = fbm(pos, 9 * scale, 3, 5)
    factor = 0.78 + 0.32 * strand + 0.18 * (clump - 0.5)
    # lighter tips where the clumps stand out, a soft sheen
    return np.clip(_shade(col, factor) + 0.04 * smooth(0.6, 0.8, clump)[:, None], 0, 1)


def scales(col, pos, nrm, scale=1.0):
    # stretched along z so they read as overlapping rows, not as a honeycomb
    f1, f2, near = voronoi(pos * (1.0, 1.0, 1.4), 9 * scale, 11)
    edge = f2 - f1
    gap = smooth(0.12, 0.02, edge)
    # the lower half of each scale catches the light, the upper half sits under the one above
    lower = smooth(0.0, 0.4, -near[:, 2])
    factor = (0.82 + 0.25 * lower) * (1 - 0.55 * gap) * (0.9 + 0.2 * fbm(pos, 3, 2, 12))
    return np.clip(_shade(col, factor), 0, 1)


def slime(col, pos, nrm, scale=1.0):
    # deeper colour where the surface faces away (thicker), lighter towards the top
    depth = 0.75 + 0.35 * smooth(-0.6, 0.9, nrm[:, 2])
    out = _shade(col, depth)
    # bubbles: small voronoi cells' centres, bright rims
    f1, f2, _ = voronoi(pos, 7 * scale, 21)
    keep = _hash3(np.floor(pos[:, 0] * 7 * scale), np.floor(pos[:, 1] * 7 * scale), np.floor(pos[:, 2] * 7 * scale), 22) > 0.75
    bubble = smooth(0.2, 0.15, f1) * keep
    rim = smooth(0.13, 0.17, f1) * smooth(0.22, 0.18, f1) * keep
    out = out * (1 - 0.12 * bubble[:, None]) + 0.25 * rim[:, None]
    # streaks running down
    down = _down(nrm)
    along = np.sum(pos * down, axis=1, keepdims=True)
    streak = smooth(0.7, 0.85, vnoise((pos - down * along) * 14 * scale + along * 1.2, 23))
    return np.clip(out + 0.08 * streak[:, None], 0, 1)


def rubber(col, pos, nrm, scale=1.0):
    mottle = fbm(pos, 5 * scale, 3, 31)
    return np.clip(_shade(col, 0.92 + 0.14 * mottle), 0, 1)


def skin(col, pos, nrm, scale=1.0, spots=None):
    blotch = smooth(0.5, 0.7, fbm(pos, 4 * scale, 4, 41))
    f1, _, _ = voronoi(pos, 16 * scale, 42)
    freckle = smooth(0.18, 0.1, f1) * (_hash3(np.floor(pos[:, 0] * 16), np.floor(pos[:, 1] * 16), np.floor(pos[:, 2] * 16), 43) > 0.6)
    dark = col * 0.62 if spots is None else np.broadcast_to(np.asarray(spots), col.shape)
    out = col * (1 - 0.6 * blotch[:, None]) + dark * 0.6 * blotch[:, None]
    out = out * (1 - 0.45 * freckle[:, None]) + dark * 0.45 * freckle[:, None]
    # a lighter belly
    return np.clip(out * (1 + 0.18 * smooth(0.2, -0.6, nrm[:, 2]))[:, None], 0, 1)


def stone(col, pos, nrm, scale=1.0):
    grain = fbm(pos, 14 * scale, 4, 51)
    fleck = smooth(0.82, 0.9, vnoise(pos * 60 * scale, 52))
    f1, f2, _ = voronoi(pos, 4 * scale, 53)
    crack = smooth(0.05, 0.0, f2 - f1) * smooth(0.45, 0.6, fbm(pos, 6, 2, 54))
    factor = (0.82 + 0.3 * grain) * (1 - 0.45 * crack) * (1 + 0.25 * fleck)
    return np.clip(_shade(col, factor), 0, 1)


def metal(col, pos, nrm, scale=1.0):
    brushed = vnoise(pos * (120, 120, 6) * scale, 61) * 0.5 + vnoise(pos * (60, 60, 3) * scale, 62) * 0.5
    dent = smooth(0.7, 0.85, fbm(pos, 5 * scale, 3, 63))
    factor = (0.88 + 0.2 * brushed) * (1 - 0.15 * dent)
    return np.clip(_shade(col, factor), 0, 1)


def peel(col, pos, nrm, scale=1.0):
    f1, _, _ = voronoi(pos, 22 * scale, 71)
    dimple = smooth(0.0, 0.5, f1)
    return np.clip(_shade(col, 0.86 + 0.18 * dimple + 0.06 * fbm(pos, 4, 2, 72)), 0, 1)


def fabric(col, pos, nrm, scale=1.0):
    u = np.sin(pos[:, 0] * 160 * scale + pos[:, 1] * 160 * scale)
    v = np.sin(pos[:, 2] * 160 * scale)
    weave = 0.5 + 0.25 * (u * np.sign(v))
    return np.clip(_shade(col, 0.88 + 0.18 * weave + 0.06 * fbm(pos, 6, 2, 81)), 0, 1)


def ribbed(col, pos, nrm, scale=1.0, ribs=10):
    angle = np.arctan2(pos[:, 1], pos[:, 0])
    rib = 0.5 + 0.5 * np.cos(angle * ribs + fbm(pos, 2, 2, 91) * 1.5)
    streak = vnoise(np.stack([angle * 20, pos[:, 2] * 2, pos[:, 0] * 0], axis=1), 92)
    factor = (0.8 + 0.28 * rib) * (0.92 + 0.12 * streak)
    return np.clip(_shade(col, factor), 0, 1)


SKINS = {"fur": fur, "scales": scales, "slime": slime, "rubber": rubber, "skin": skin, "stone": stone,
         "metal": metal, "peel": peel, "fabric": fabric, "ribbed": ribbed}


def apply(kind, col, pos, nrm, **options):
    """col (n, 3) linear colours at positions pos with normals nrm, patterned as kind."""
    return SKINS[kind](col, pos, nrm, **options)
