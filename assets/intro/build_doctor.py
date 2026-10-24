# The doctor whose lab the dragon breaks out of: bald with white tufts and a big moustache, a white lab coat.
# Actions: "run", "fall" (thrown onto his bottom, once), "panic" (hands at his head, shaking it) and "stand"
# (arms out, looking up after the dragon).
#   blender --background --factory-startup --python build_doctor.py -- <out dir>
# writes doctor.glb and doctor.blend, about 1.05 high, the player being 1.
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bpy  # noqa: E402

import refine  # noqa: E402
import toy  # noqa: E402
from toy import FORWARD, LIFT, bone_keys, cone, cycle, mirror, sphere, trs, tube, turn  # noqa: E402

COLORS = {
    "skin": (1.0, 0.76, 0.56),
    "nose": (1.0, 0.62, 0.46),
    "coat": (0.96, 0.96, 0.94),
    "coat_shade": (0.80, 0.82, 0.86),
    "hair": (0.98, 0.98, 0.96),
    "trousers": (0.30, 0.28, 0.34),
    "shoe": (0.72, 0.22, 0.10),
    "black": (0.05, 0.04, 0.05),
    "button": (0.55, 0.60, 0.70),
    "pen": (0.15, 0.35, 0.85),
}

HEAD = 0.84


def build():
    toy.reset()
    palette = toy.Palette("doctor_palette", COLORS)
    b = toy.Builder("doctor", palette)
    rig = toy.Biped(b, hip=0.27, shoulder=0.57, sx=0.16, neck=0.63, hand=0.3, hx=0.075)

    # lab coat: flared below, round shoulders, a collar, buttons and a pen in the breast pocket
    b.add(cone(0.25, 0.16, 0.5, 32), "coat", "body", trs((0, 0, 0.13)))
    b.add(sphere(), "coat", "body", trs((0, 0, 0.58), (0, 0, 0), (0.19, 0.16, 0.08)))
    # a collar round the neck, so the crease where the head sits on the coat is the coat's
    b.add(sphere(), "coat", "body", trs((0, 0.01, 0.655), (0, 0, 0), (0.14, 0.13, 0.04)))
    for s in (1, -1):
        side = (lambda m: m) if s > 0 else mirror
        b.add(cone(0.06, 0.0, 0.14, 4), "coat_shade", "body", side(trs((0.06, -0.12, 0.62), (200, 0, 30),
                                                                        (1, 0.4, 1))), paint=True)
    for z in (0.22, 0.34, 0.46):
        b.add(sphere(), "button", "body", trs((0, -0.2 - (0.46 - z) * 0.2, z), (0, 0, 0), 0.018), paint=True)
    b.add(tube([(0.0, 0.0, 0.0), (0.0, 0.0, 0.07)], [0.012, 0.012], 6), "pen", "body", trs((0.09, -0.155, 0.47)), paint=True)

    # head: bald dome, white tufts over the ears and round the back, nose, moustache, bushy brows
    head = trs((0, 0, HEAD))
    b.add(sphere(), "skin", "head", head @ trs((0, 0, 0), (0, 0, 0), (0.21, 0.2, 0.22)))
    b.add(sphere(), "nose", "head", head @ trs((0, -0.21, -0.03), (0, 0, 0), (0.055, 0.05, 0.05)))
    for s in (1, -1):
        side = (lambda m: m) if s > 0 else mirror
        for i, (y, z) in enumerate(((-0.02, 0.02), (0.06, -0.02), (0.12, 0.04), (0.02, 0.1))):
            b.add(sphere(), "hair", "head", head @ side(trs((0.2 - abs(y) * 0.2, y, z), (0, 0, 0),
                                                            (0.06, 0.07, 0.07))))
        b.add(sphere(), "hair", "head", head @ side(trs((0.06, -0.205, -0.08), (0, 0, -15), (0.08, 0.035, 0.035))))
        b.add(sphere(), "hair", "head", head @ side(trs((0.08, -0.19, 0.08), (0, -10, 0), (0.06, 0.025, 0.022))))
        b.add(sphere(), "black", "head", head @ side(trs((0.075, -0.19, 0.03), (0, 0, 0), (0.022, 0.015, 0.03))), paint=True)
        b.add(sphere(), "skin", "head", head @ side(trs((0.205, 0.0, -0.01), (0, 0, 0), (0.035, 0.05, 0.06))))
    for x in (-0.06, 0.0, 0.06):
        b.add(sphere(), "hair", "head", head @ trs((x, 0.17, -0.06), (0, 0, 0), (0.07, 0.06, 0.07)))

    # arms in coat sleeves, legs, shoes
    for side, s in (("L", 1), ("R", -1)):
        b.add(tube(rig.arm(s), [0.06, 0.055, 0.06], 12), "coat", f"arm.{side}")
        b.add(sphere(), "skin", f"arm.{side}", trs((s * 0.19, 0, 0.27), (0, 0, 0), 0.05))
        b.add(tube(rig.leg(s), [0.055, 0.05, 0.045], 10), "trousers", f"leg.{side}")
        b.add(sphere(), "shoe", f"leg.{side}", trs((s * 0.075, -0.04, 0.04), (0, 0, 0), (0.06, 0.1, 0.045)))
    return b.build()


def animate(rig):
    # run: legs and arms swinging against each other, bobbing, leaning in
    keys = {
        "leg.L": bone_keys(cycle(12, turn((FORWARD, -45)), turn((FORWARD, 40)))),
        "leg.R": bone_keys(cycle(12, turn((FORWARD, 40)), turn((FORWARD, -45)))),
        "arm.L": bone_keys(cycle(12, turn((FORWARD, 45), (LIFT["L"], 15)), turn((FORWARD, -50), (LIFT["L"], 15)))),
        "arm.R": bone_keys(cycle(12, turn((FORWARD, -50), (LIFT["R"], 15)), turn((FORWARD, 45), (LIFT["R"], 15)))),
        "body": bone_keys(cycle(12, turn((FORWARD, 12)), turn((FORWARD, 12)))),
        "root": bone_keys(cycle(12, (0, 0, 0.0), (0, 0, 0.06), (0, 0, 0.0), (0, 0, 0.06)), rotation=False),
    }
    toy.action(rig, "run", 12, keys)

    # fall: thrown backwards, arms flung up, landing on his bottom with the legs in the air
    stand = turn()
    sit = (0, 0.06, -0.2)
    keys = {
        "root": [(1, stand, (0, 0, 0)), (5, turn((FORWARD, -25)), (0, 0.05, 0.04)), (10, turn((FORWARD, -20)), sit),
                 (14, turn((FORWARD, -8)), (0, 0.06, -0.17)), (24, turn((FORWARD, -10)), sit)],
        "leg.L": bone_keys([(1, stand), (5, turn((FORWARD, -40))), (10, turn((FORWARD, -95))),
                            (14, turn((FORWARD, -70))), (24, turn((FORWARD, -80)))]),
        "leg.R": bone_keys([(1, stand), (5, turn((FORWARD, -25))), (10, turn((FORWARD, -85))),
                            (14, turn((FORWARD, -60))), (24, turn((FORWARD, -75)))]),
        "arm.L": bone_keys([(1, stand), (5, turn((LIFT["L"], 150))), (10, turn((LIFT["L"], 110))),
                            (24, turn((LIFT["L"], 35), (FORWARD, 30)))]),
        "arm.R": bone_keys([(1, stand), (5, turn((LIFT["R"], 140))), (10, turn((LIFT["R"], 100))),
                            (24, turn((LIFT["R"], 35), (FORWARD, 30)))]),
        "head": bone_keys([(1, stand), (5, turn((FORWARD, 20))), (10, turn((FORWARD, -15))), (24, stand)]),
    }
    toy.action(rig, "fall", 24, keys, cyclic=False)

    # panic: hands clapped to the head, shaking it, hopping from foot to foot
    keys = {
        "arm.L": bone_keys(cycle(16, turn((LIFT["L"], 150), (FORWARD, -30)), turn((LIFT["L"], 125), (FORWARD, -20)))),
        "arm.R": bone_keys(cycle(16, turn((LIFT["R"], 125), (FORWARD, -20)), turn((LIFT["R"], 150), (FORWARD, -30)))),
        "head": bone_keys(cycle(16, turn(((0, 0, 1), 20), (FORWARD, -10)), turn(((0, 0, 1), -20), (FORWARD, -10)))),
        "body": bone_keys(cycle(16, turn(((0, 1, 0), 6)), turn(((0, 1, 0), -6)))),
        "leg.L": bone_keys(cycle(16, turn(), turn((FORWARD, -30)))),
        "leg.R": bone_keys(cycle(16, turn((FORWARD, -30)), turn())),
        "root": bone_keys(cycle(16, (0, 0, 0.0), (0, 0, 0.05), (0, 0, 0.0), (0, 0, 0.05)), rotation=False),
    }
    toy.action(rig, "panic", 16, keys)

    # stand: arms held out in dismay, looking up
    keys = {
        "arm.L": bone_keys(cycle(48, turn((LIFT["L"], 40)), turn((LIFT["L"], 34)))),
        "arm.R": bone_keys(cycle(48, turn((LIFT["R"], 40)), turn((LIFT["R"], 34)))),
        "head": bone_keys(cycle(48, turn((FORWARD, -28)), turn((FORWARD, -24)))),
    }
    toy.action(rig, "stand", 48, keys)


rig, mesh = build()
mesh = refine.refine(rig, mesh, faces=3000)
animate(rig)
out = toy.out_dir()
toy.export(rig, mesh, os.path.join(out, "doctor.glb"))
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(out, "doctor.blend"))
print("RESULT doctor", tuple(round(d, 2) for d in mesh.dimensions), "verts", len(mesh.data.vertices))
