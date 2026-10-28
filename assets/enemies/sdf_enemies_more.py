# The remaining enemies and bosses for sdf_core, same conventions as sdf_enemies.py.
import math

from sdf_core import BLACK, WHITE, Capsule, Cylinder, Decal, Ellipsoid, Model, RoundBox, Torus, hexcol


def legs(m, leg, foot, hip_z=0.62, spread=0.26, radius=0.085):
    """Short legs flowing out of the body down to rounded feet, like a bomberman's."""
    for side in (-1, 1):
        m.add(Capsule((side * spread, -0.02, hip_z), (side * (spread + 0.06), -0.06, 0.18), radius, radius - 0.01, leg,
                      blend=0.1), bone=f"leg.{side}")
        m.add(Ellipsoid((side * (spread + 0.07), -0.12, 0.1), (0.18, 0.25, 0.1), foot, blend=0.08), bone=f"leg.{side}")


def droplet():
    m = Model("Droplet", skin="slime")
    c = hexcol("12b4ff")
    m.add(Ellipsoid((0, 0, 0.75), (0.7, 0.62, 0.72), c))
    m.add(Capsule((0, 0, 1.15), (0, 0, 1.95), 0.5, 0.07, c, blend=0.25), bone="top")
    m.paint(Decal((0, 0.85), (0.13, 0.34), hexcol("d01010")))
    m.eyes(0.2, 1.42, eye=(0.09, 0.12), pupil=(0.045, 0.06))
    return m


def totem():
    m = Model("Totem", motion="float", skin="metal")
    gold = hexcol("ffc018")
    m.add(Cylinder((0, 0, 0.98), 0.6, 0.45, 0.14, hexcol("ffd860")))
    # flat bell skirt: a wide, low rounded cylinder instead of a round cone that ends in a ball
    m.add(Cylinder((0, 0, 0.24), 0.82, 0.22, 0.1, gold, blend=0.18))
    m.add(Capsule((0, 0, 1.38), (0, 0, 1.78), 0.22, 0.08, gold, blend=0.12), bone="top")
    m.paint(Decal((0, 0.9), (0.4, 0.2), BLACK, rect=True))
    m.paint(Decal((0, 0.9), (0.3, 0.08), hexcol("e01818"), rect=True))
    m.eyes(0.22, 1.25, eye=(0.1, 0.1), pupil=(0.05, 0.06), pupil_offset=(0, 0))
    return m


def imp():
    m = Model("Imp", motion="walk", skin=("fur", {"scale": 1.4}), fur=(0.12, 18))
    purple, orange = hexcol("9a28ff"), hexcol("ff8a10")
    m.add(Ellipsoid((0, 0, 1.2), (0.7, 0.62, 0.7), purple))
    for side in (-1, 1):
        m.add(Capsule((side * 0.3, 0, 1.72), (side * 0.56, 0, 2.15), 0.11, 0.06, WHITE, blend=0.08), bone=f"horn.{side}")
        m.add(Ellipsoid((side * 0.76, -0.1, 1.05), (0.24, 0.24, 0.3), orange, blend=0.1), bone=f"arm.{side}",
              head=(side * 0.55, -0.05, 1.2), tail=(side * 0.8, -0.1, 0.9))
    legs(m, hexcol("6a10c0"), orange, hip_z=0.62, spread=0.24)
    m.eyes(0.24, 1.33, eye=(0.14, 0.15), pupil=(0.07, 0.08), iris=hexcol("2080ff"), pupil_offset=(0, 0))
    m.paint(Decal((0, 1.0), (0.12, 0.05), hexcol("e01818")))
    return m


def onion():
    m = Model("Onion", motion="walk", skin=("ribbed", {"ribs": 12}))
    blue = hexcol("1a7cff")
    m.add(Ellipsoid((0, 0, 1.05), (0.72, 0.68, 0.6), blue))
    m.add(Capsule((0, 0, 1.38), (0, 0, 2.0), 0.5, 0.06, hexcol("c0b8e8"), blend=0.15), bone="top")
    legs(m, hexcol("1050c0"), hexcol("1050c0"), hip_z=0.62, spread=0.25)
    m.eyes(0.25, 1.18, eye=(0.13, 0.17), pupil=(0.06, 0.08))
    m.paint(Decal((0, 0.9), (0.07, 0.05), BLACK))
    return m


def croc():
    """A crocodile standing up: long snout with teeth along it, eyes bulging on top of the head, a row of ridges down
    its back, a heavy tail, scales all over and a pale belly."""
    m = Model("Croc", motion="walk", skin="scales")
    green, belly, dark = hexcol("3cb400"), hexcol("d8e890"), hexcol("1e6400")
    m.add(Ellipsoid((0, 0.08, 0.95), (0.62, 0.62, 0.62), green))
    m.add(Ellipsoid((0, -0.22, 0.85), (0.48, 0.4, 0.45), belly, blend=0.12), skin="rubber")
    m.add(RoundBox((0, -0.62, 1.22), (0.36, 0.42, 0.2), 0.14, green, blend=0.2))
    for side in (-1, 1):
        m.add(Ellipsoid((side * 0.22, -0.4, 1.45), (0.17, 0.17, 0.15), green, blend=0.1))
        m.paint(Decal((side * 0.1, -0.98), (0.045, 0.035), BLACK, facing=(0, 0, 1)))
    for i in range(4):
        m.add(Ellipsoid((0, -0.15 + i * 0.24, 1.6 - i * 0.12), (0.08, 0.11, 0.09), dark, blend=0.06), skin="rubber")
    m.add(Capsule((0, 0.55, 0.62), (0.05, 1.15, 0.22), 0.3, 0.08, green, blend=0.2), bone="tail")
    legs(m, green, dark, hip_z=0.55, spread=0.34, radius=0.1)
    # eyes looking ahead from their bumps, a wide mouth with teeth along the snout's sides
    for side in (-1, 1):
        m.paint(Decal((side * 0.22, 1.5), (0.1, 0.09), hexcol("ffd800")))
        m.paint(Decal((side * 0.22, 1.49), (0.03, 0.07), BLACK))
    m.paint(Decal((0, 1.1), (0.36, 0.025), hexcol("401010"), rect=True))
    for i in range(6):
        m.paint(Decal((-0.3 + i * 0.12, 1.07), (0.025, 0.035), WHITE, rect=True))
    return m


def blob():
    m = Model("Blob", skin="slime")
    c = hexcol("1088ff")
    m.add(Ellipsoid((0, 0, 1.0), (0.7, 0.62, 0.95), c))
    m.add(Cylinder((0, 0, 0.35), 0.68, 0.35, 0.15, c, blend=0.15))
    m.eyes(0.24, 1.25, eye=(0.12, 0.15), pupil=(0.06, 0.08))
    m.paint(Decal((0, 0.85), (0.16, 0.03), BLACK, rect=True))
    return m


def grinner():
    m = Model("Grinner", skin="peel")
    m.add(Ellipsoid((0, 0, 1.0), (0.82, 0.74, 0.82), hexcol("ffaa00")))
    m.eyes(0.26, 1.25, eye=(0.12, 0.2), pupil=(0.06, 0.1))
    m.paint(Decal((0, 0.72), (0.3, 0.17), BLACK))
    m.paint(Decal((0, 0.82), (0.22, 0.05), WHITE, rect=True))
    return m


def hothead():
    m = Model("Hothead", skin="skin")
    m.add(Ellipsoid((0, 0, 0.85), (0.75, 0.68, 0.62), hexcol("ff9a00")))
    for px, h in ((-0.48, 0.5), (-0.17, 0.75), (0.17, 0.85), (0.48, 0.55)):
        m.add(Capsule((px, 0, 1.2), (px * 1.25, 0, 1.25 + h), 0.26, 0.07, hexcol("ff2000"), blend=0.15),
              bone=f"spike{round(px * 100):+d}")
    m.eyes(0.28, 1.0, eye=(0.12, 0.12), pupil=(0.06, 0.06), pupil_offset=(0, 0))
    m.paint(Decal((0, 0.58), (0.3, 0.13), BLACK))
    return m


def jester():
    m = Model("Jester", motion="walk", skin="fabric")
    m.add(Ellipsoid((0, 0, 1.1), (0.68, 0.6, 0.7), hexcol("ff8a10")))
    m.add(Capsule((0, -0.48, 1.18), (0, -0.92, 1.12), 0.17, 0.06, hexcol("e01818"), blend=0.06), bone="nose")
    for side in (-1, 1):
        m.add(Ellipsoid((side * 0.66, 0.05, 1.1), (0.16, 0.42, 0.5), hexcol("8a50ff"), blend=0.1), bone=f"ear.{side}",
              head=(side * 0.5, 0.05, 1.1), tail=(side * 0.66, 0.05, 1.5))
    m.add(Ellipsoid((0, 0, 1.72), (0.45, 0.45, 0.22), hexcol("6a30e0"), blend=0.12), bone="top",
          head=(0, 0, 1.6), tail=(0, 0, 1.9))
    legs(m, hexcol("e0a000"), hexcol("ffd000"), hip_z=0.55, spread=0.22, radius=0.07)
    m.eyes(0.2, 1.42, eye=(0.12, 0.15), pupil=(0.06, 0.08))
    return m


def statue():
    m = Model("Statue", skin="stone")
    grey = hexcol("d8d8d8")
    m.add(Ellipsoid((0, 0, 1.05), (0.62, 0.56, 0.9), grey))
    m.add(Cylinder((0, 0, 0.35), 0.62, 0.35, 0.15, grey, blend=0.15))
    m.add(Cylinder((0, 0, 1.98), 0.3, 0.09, 0.07, hexcol("1a90ff"), blend=0.08), bone="top",
          head=(0, 0, 1.85), tail=(0, 0, 2.1))
    for side in (-1, 1):
        m.paint(Decal((side * 0.22, 1.25), (0.11, 0.025), hexcol("555555"), rect=True))
    m.paint(Decal((0, 0.9), (0.13, 0.02), hexcol("555555"), rect=True))
    return m


def spinner():
    m = Model("Spinner", motion="spin", skin="metal")
    m.add(Capsule((0, 0, 1.45), (0, 0, 0.95), 0.6, 0.18, hexcol("d02020")))
    m.add(Capsule((0, 0, 0.95), (0, 0, 0.42), 0.18, 0.42, hexcol("ffc018"), blend=0.1))
    m.add(Cylinder((0, 0, 0.16), 0.52, 0.16, 0.08, hexcol("ffc018"), blend=0.15))
    m.add(Cylinder((0, 0, 1.62), 0.68, 0.1, 0.07, hexcol("1a90ff"), blend=0.06))
    m.eyes(0.25, 1.32, eye=(0.13, 0.13), pupil=(0.06, 0.07), pupil_offset=(0, 0))
    return m


def sprout():
    m = Model("Sprout", skin=("skin", {"spots": hexcol("1a7a00")}))
    m.add(Ellipsoid((0, 0, 1.0), (0.78, 0.7, 0.75), hexcol("2ab800")))
    m.add(Torus((0, 0, 1.55), 0.45, 0.12, hexcol("e81890"), blend=0.08), bone="top",
          head=(0, 0, 1.45), tail=(0, 0, 1.7))
    m.eyes(0.3, 1.2, eye=(0.16, 0.16), pupil=(0.07, 0.08), pupil_offset=(0, 0))
    m.paint(Decal((0, 0.75), (0.25, 0.025), BLACK, rect=True))
    return m


def chomper():
    m = Model("Chomper", skin="stone")
    m.add(RoundBox((0, 0, 0.82), (0.62, 0.45, 0.55), 0.2, hexcol("222222")))
    m.add(Capsule((0, 0, 1.3), (0, 0, 1.62), 0.3, 0.14, hexcol("ffc018"), blend=0.1), bone="top")
    m.add(RoundBox((0, -0.05, 0.16), (0.6, 0.44, 0.1), 0.08, hexcol("ffa000"), blend=0.06))
    for i in range(6):
        m.paint(Decal((-0.4 + i * 0.16, 0.7), (0.055, 0.2), WHITE, rect=True))
    m.eyes(0.3, 1.12, eye=(0.1, 0.1), pupil=(0.05, 0.05), pupil_offset=(0, 0))
    return m


def coin():
    m = Model("Coin", motion="spin", skin="metal")
    m.add(Cylinder((0, 0, 1.0), 0.8, 0.15, 0.08, hexcol("ffb000"), upright_y=True))
    m.add(Torus((0, 0, 1.0), 0.74, 0.08, hexcol("ffd860"), blend=0.04, upright=True))
    m.eyes(0.22, 1.18, eye=(0.12, 0.17), pupil=(0.06, 0.09))
    m.paint(Decal((0, 0.75), (0.17, 0.09), BLACK))
    return m


def serpent():
    # lies along its heading: the head leads towards -y and the body trails straight behind it
    m = Model("Serpent", boss=True, motion="slither", skin="scales")
    blue = hexcol("2860e0")
    for i in range(5):
        x, y, z = 0.0, 0.72 + i * 0.62, 0.5
        prev = (0.0, 0.0, 0.5) if i == 0 else (x, y - 0.62, z)
        m.add(Ellipsoid((x, y, z), (0.38, 0.38, 0.42), blue, blend=0.12), bone=f"seg{i}",
              parent="body" if i == 0 else f"seg{i - 1}", head=prev, tail=(x, y, z))
        m.paint(Decal((x, y), (0.16, 0.2), WHITE, facing=(0, 0, 1)))
    m.add(Capsule((0.0, 3.2, 0.45), (0.0, 3.95, 0.32), 0.3, 0.06, blue, blend=0.12), bone="tail", parent="seg4")
    m.add(Ellipsoid((0.0, 0.0, 0.7), (0.55, 0.62, 0.55), blue, blend=0.15))
    m.add(Ellipsoid((0.0, -0.5, 0.55), (0.3, 0.35, 0.22), hexcol("6a98ff"), blend=0.1))
    for side in (-1, 1):
        m.add(Capsule((side * 0.25, 0.1, 1.1), (side * 0.3, 0.15, 1.5), 0.1, 0.06, WHITE, blend=0.08),
              bone=f"horn.{side}")
    m.eyes(0.22, 0.95, eye=(0.12, 0.14), pupil=(0.06, 0.07))
    return m


def puffer():
    m = Model("Puffer", scale=1.5, boss=True, skin=("skin", {"spots": hexcol("1a70c0")}))
    c, pale = hexcol("38b0ff"), hexcol("a0e0ff")
    m.add(Ellipsoid((0, 0, 0.8), (0.85, 0.7, 0.65), c))
    for i in range(5):
        a = math.radians(-60 + i * 30)
        root = (math.sin(a) * 0.45, 0, 0.8 + math.cos(a) * 0.45)
        tip = (math.sin(a) * 1.0, 0, 0.85 + math.cos(a) * 0.95)
        m.add(Capsule(root, tip, 0.17, 0.06, pale, blend=0.08), bone=f"spike{i}")
    for side in (-1, 1):
        m.add(Capsule((side * 0.72, -0.15, 0.6), (side * 1.18, -0.3, 0.5), 0.2, 0.07, c, blend=0.1), bone=f"arm.{side}")
    m.eyes(0.3, 0.95, eye=(0.12, 0.14), pupil=(0.06, 0.07), pupil_offset=(0, 0))
    m.paint(Decal((0, 0.62), (0.12, 0.16), hexcol("e01818")))
    return m


def cyclops():
    m = Model("Cyclops", scale=1.5, boss=True, motion="float", skin=("fur", {"scale": 0.8}), fur=(0.18, 12))
    c = hexcol("1a7cff")
    m.add(Ellipsoid((0, 0, 1.25), (0.62, 0.56, 1.0), c))
    m.add(Cylinder((0, 0, 0.35), 0.62, 0.35, 0.15, c, blend=0.2))
    for side in (-1, 1):
        m.add(Capsule((side * 0.52, -0.05, 1.25), (side * 0.74, -0.15, 0.78), 0.16, 0.13, c, blend=0.12),
              bone=f"arm.{side}")
    m.paint(Decal((0, 1.4), (0.32, 0.32), WHITE))
    m.paint(Decal((0, 1.4), (0.22, 0.22), hexcol("ff9000")))
    m.paint(Decal((0, 1.4), (0.09, 0.13), BLACK))
    return m


def eyeball():
    # its own model instead of a separate piece floating next to the cyclops
    m = Model("Eyeball", target=800, skin="rubber")
    pink = hexcol("ff70d0")
    m.add(Ellipsoid((0, 0, 0.5), (0.34, 0.3, 0.38), pink))
    m.add(Cylinder((0, 0, 0.18), 0.32, 0.18, 0.08, pink, blend=0.1))
    m.paint(Decal((0, 0.55), (0.14, 0.14), WHITE))
    m.paint(Decal((0, 0.55), (0.06, 0.08), BLACK))
    return m


def gourd():
    # bafa: a magenta gourd with a crest, frowning brows and a red flank
    m = Model("Gourd", skin=("ribbed", {"ribs": 8}))
    pink, crest, red = hexcol("ff28f0"), hexcol("d860e0"), hexcol("ff1030")
    m.add(Ellipsoid((0, 0, 0.62), (0.8, 0.7, 0.6), pink))
    m.add(Ellipsoid((0, 0, 1.28), (0.56, 0.5, 0.5), pink, blend=0.28))
    m.add(Capsule((0, 0, 1.62), (0, 0, 2.18), 0.16, 0.07, crest, blend=0.14), bone="top")
    m.eyes(0.22, 1.3, eye=(0.15, 0.18), pupil=(0.09, 0.11), pupil_offset=(0.0, 0.02))
    for side in (-1, 1):
        m.paint(Decal((side * 0.25, 1.53), (0.17, 0.045), hexcol("a00010"), rect=True))
    m.paint(Decal((-0.38, 0.7), (0.26, 0.2), hexcol("ffb4f8")))
    m.paint(Decal((0.48, 0.55), (0.24, 0.24), red))
    return m


def cloud():
    # puffer' pink cloud: four soft lobes around a darker heart, two big eyes
    m = Model("Cloud", target=1000, motion="float", skin=("fur", {"scale": 0.6}), fur=(0.2, 10))
    pink, heart = hexcol("ff9ae0"), hexcol("d860c0")
    m.add(Ellipsoid((0, 0, 0.92), (0.42, 0.4, 0.42), heart))
    for x, z in ((-0.44, 1.32), (0.44, 1.32), (-0.44, 0.52), (0.44, 0.52)):
        m.add(Ellipsoid((x, 0, z), (0.4, 0.36, 0.38), pink, blend=0.16))
    m.eyes(0.4, 0.98, eye=(0.16, 0.13), pupil=(0.08, 0.08), pupil_offset=(0.0, 0.0))
    return m
