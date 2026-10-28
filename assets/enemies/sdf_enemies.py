# Enemy definitions for sdf_core: front is -Y, ground is z = 0, about 2 units tall before scale.
import math

from sdf_core import BLACK, WHITE, Capsule, Cylinder, Decal, Ellipsoid, Model, RoundBox, Torus, hexcol


def balloon():
    m = Model("Balloon", motion="float", skin="rubber")
    orange = hexcol("ee7a00")
    m.add(Ellipsoid((0, 0, 1.25), (0.8, 0.76, 0.86), orange))
    m.add(Capsule((0, 0, 0.5), (0, 0, 0.28), 0.14, 0.1, orange, blend=0.12), bone="dangle")
    m.add(Ellipsoid((0, 0, 0.2), (0.12, 0.12, 0.1), hexcol("d0b0d0"), blend=0.05), bone="dangle")
    m.eyes(0.26, 1.4, eye=(0.15, 0.24), pupil=(0.07, 0.12), pupil_offset=(0.04, -0.05))
    m.paint(Decal((0, 0.98), (0.09, 0.06), BLACK))
    return m


def tiger():
    m = Model("Tiger", motion="walk", skin="fur", fur=(0.09, 22))
    orange, dark = hexcol("f07800"), hexcol("c05a00")
    m.add(Ellipsoid((0, 0, 1.25), (0.82, 0.72, 0.78), orange))
    for side in (-1, 1):
        m.add(Capsule((side * 0.42, 0, 1.75), (side * 0.6, 0, 2.08), 0.2, 0.07, hexcol("d03000"), blend=0.14),
              bone=f"horn.{side}")
        m.add(Capsule((side * 0.26, -0.02, 0.62), (side * 0.32, -0.06, 0.18), 0.09, 0.08, dark, blend=0.1),
              bone=f"leg.{side}")
        m.add(Ellipsoid((side * 0.33, -0.12, 0.11), (0.19, 0.26, 0.11), orange, blend=0.08), bone=f"leg.{side}")
        for i in range(2):
            m.paint(Decal((-0.15, 1.38 - i * 0.26), (0.16, 0.04), hexcol("c02000"), facing=(side, 0, 0), rect=True))
    m.add(Ellipsoid((0, -0.55, 1.02), (0.36, 0.24, 0.22), WHITE, blend=0.06))
    m.paint(Decal((0, 1.1), (0.11, 0.07), BLACK))
    m.eyes(0.25, 1.48, eye=(0.14, 0.2), pupil=(0.07, 0.1))
    return m


def frog():
    m = Model("Frog", skin=("skin", {"spots": hexcol("128000")}))
    m.add(Ellipsoid((0, 0, 0.48), (0.76, 0.68, 0.45), WHITE))
    m.add(Capsule((0, 0, 0.9), (0, 0, 1.75), 0.7, 0.14, hexcol("2ad000"), blend=0.12), bone="top")
    m.add(Torus((0, 0, 0.88), 0.66, 0.1, hexcol("e8e020"), blend=0.08))
    m.eyes(0.27, 1.32, eye=(0.13, 0.17), pupil=(0.06, 0.08))
    m.paint(Decal((0, 1.06), (0.2, 0.08), hexcol("e01818")))
    return m


def octopus():
    m = Model("Octopus", skin=("skin", {"spots": hexcol("c02070")}))
    pink = hexcol("ff4fa8")
    m.add(Ellipsoid((0, 0, 1.3), (0.78, 0.7, 0.62), pink))
    m.add(Ellipsoid((0, 0, 1.95), (0.18, 0.15, 0.14), hexcol("ff9fd0"), blend=0.1))
    # a soft underside the tentacles grow out of, so no thin gaps remain between their roots
    m.add(Ellipsoid((0, 0, 0.82), (0.55, 0.5, 0.3), pink, blend=0.2))
    for i in range(6):
        a = math.radians(i * 60 + 30)
        c, s = math.cos(a), math.sin(a)
        root, bend, tip = (c * 0.42, s * 0.38, 0.75), (c * 0.68, s * 0.62, 0.14), (c * 0.92, s * 0.85, 0.18)
        m.add(Capsule(root, bend, 0.15, 0.1, pink, blend=0.14), bone=f"tent{i}")
        m.add(Capsule(bend, tip, 0.1, 0.075, pink, blend=0.06), bone=f"tent{i}b", parent=f"tent{i}")
    m.eyes(0.3, 1.4, eye=(0.19, 0.19), pupil=(0.08, 0.1), pupil_offset=(0.02, -0.05))
    for side in (-1, 1):
        m.paint(Decal((side * 0.3, 1.6), (0.2, 0.05), hexcol("c02070"), rect=True))
    m.paint(Decal((0, 1.0), (0.3, 0.1), hexcol("ff9fd0")))
    return m


def crab():
    m = Model("Crab", scale=1.5, boss=True, motion="walk", skin=("scales", {"scale": 0.7}))
    blue, leg = hexcol("1a6cff"), hexcol("0a3cb0")
    m.add(Ellipsoid((0, 0, 1.05), (0.9, 0.7, 0.62), blue))
    for side in (-1, 1):
        m.add(Capsule((side * 0.42, 0, 1.5), (side * 0.78, 0, 2.02), 0.22, 0.07, hexcol("e0e0e0"), blend=0.1),
              bone=f"horn.{side}")
        for i in range(3):
            y = -0.36 + i * 0.36
            hip, knee, foot = (side * 0.7, y * 1.3, 0.9), (side * 1.2, y * 1.5, 1.2), (side * 1.45, y * 1.8, 0.08)
            m.add(Capsule(hip, knee, 0.11, 0.085, leg, blend=0.2), bone=f"leg{i}.{side}")
            m.add(Capsule(knee, foot, 0.085, 0.07, leg, blend=0.05), bone=f"leg{i}.{side}b", parent=f"leg{i}.{side}")
    m.eyes(0.38, 1.2, eye=(0.17, 0.17), pupil=(0.07, 0.09), white=hexcol("ffc000"))
    m.paint(Decal((0, 0.88), (0.1, 0.05), hexcol("ff2080")))
    return m


from sdf_enemies_more import (serpent, droplet, puffer, gourd, cloud, hothead, croc, spinner, sprout, grinner, statue, chomper,
                              onion, imp, coin, jester, totem, blob, cyclops, eyeball)

ENEMIES = [balloon, octopus, droplet, tiger, totem, imp, onion, croc, blob, frog, grinner, hothead,
           jester, statue, spinner, sprout, chomper, coin, gourd, cloud]
BOSSES = [serpent, puffer, cyclops, eyeball, crab]
MODELS = ENEMIES + BOSSES
