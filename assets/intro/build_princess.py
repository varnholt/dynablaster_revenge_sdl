# The princess the dragon carries off: big orange hair with a pink bow, a pink dress. Actions: "dangle" (held by
# the shoulders in the dragon's claws, flailing and kicking) and "idle".
#   blender --background --factory-startup --python build_princess.py -- <out dir>
# writes princess.glb and princess.blend, about 0.95 high, the player being 1.
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bpy  # noqa: E402

import refine  # noqa: E402
import toy  # noqa: E402
from toy import FORWARD, LIFT, bone_keys, cone, cycle, mirror, sphere, trs, tube, turn  # noqa: E402

COLORS = {
    "skin": (1.0, 0.80, 0.62),
    "hair": (0.98, 0.55, 0.10),
    "hair_dark": (0.85, 0.40, 0.06),
    "dress": (0.93, 0.25, 0.62),
    "dress_light": (1.0, 0.55, 0.80),
    "bow": (0.85, 0.10, 0.45),
    "black": (0.05, 0.04, 0.05),
    "white": (0.98, 0.98, 0.98),
    "blush": (1.0, 0.55, 0.55),
    "shoe": (0.75, 0.10, 0.35),
}

HEAD = 0.68


def build():
    toy.reset()
    palette = toy.Palette("princess_palette", COLORS)
    b = toy.Builder("princess", palette)
    rig = toy.Biped(b, hip=0.24, shoulder=0.45, sx=0.12, neck=0.5, hand=0.26, hx=0.06)

    # dress: a bell with a lighter hem and a bodice
    b.add(cone(0.24, 0.1, 0.32, 32), "dress", "body", trs((0, 0, 0.17)))
    b.add(tube([(0, 0, 0.18), (0, 0, 0.19)], [0.245, 0.245], 32, cap=False), "dress_light", "body")
    b.add(sphere(), "dress", "body", trs((0, 0, 0.45), (0, 0, 0), (0.13, 0.11, 0.09)))
    b.add(sphere(), "bow", "body", trs((0, -0.1, 0.42), (0, 0, 0), (0.05, 0.03, 0.035)))

    # head: face, eyes, blush, a mop of hair with two bunches and a bow on top
    head = trs((0, 0, HEAD))
    b.add(sphere(), "skin", "head", head @ trs((0, 0, 0), (0, 0, 0), (0.24, 0.22, 0.23)))
    b.add(sphere(), "hair", "head", head @ trs((0, 0.04, 0.06), (-15, 0, 0), (0.27, 0.25, 0.23)), shell="hair")
    for i, x in enumerate((-0.12, -0.04, 0.04, 0.12)):
        # the fringe
        b.add(sphere(), "hair", "head", head @ trs((x, -0.17, 0.12 - abs(x) * 0.3), (20, 0, x * 100),
                                                   (0.07, 0.05, 0.08)), shell="hair")
    for s in (1, -1):
        side = (lambda m: m) if s > 0 else mirror
        b.add(sphere(), "white", "head", head @ side(trs((0.08, -0.2, -0.01), (0, 0, 0), (0.045, 0.02, 0.06))), paint=True)
        b.add(sphere(), "black", "head", head @ side(trs((0.08, -0.215, -0.015), (0, 0, 0), (0.032, 0.02, 0.05))), paint=True)
        b.add(sphere(), "white", "head", head @ side(trs((0.095, -0.24, 0.01), (0, 0, 0), (0.012, 0.012, 0.015))),
              paint=True)
        b.add(sphere(), "blush", "head", head @ side(trs((0.14, -0.17, -0.08), (0, 0, 20), (0.04, 0.015, 0.025))), paint=True)
        b.add(tube([(0.2, 0.05, 0.05), (0.3, 0.08, -0.08), (0.3, 0.1, -0.22)], [0.09, 0.08, 0.04], 14),
              "hair", "head", head @ side(trs()), shell="hair")
        b.add(sphere(), "bow", "head", head @ side(trs((0.08, 0.0, 0.27), (0, -30, 0), (0.08, 0.04, 0.06))), shell="hair")
    b.add(sphere(), "bow", "head", head @ trs((0, 0.0, 0.25), (0, 0, 0), (0.04, 0.04, 0.04)), shell="hair")
    b.add(tube([(-0.04, -0.215, -0.11), (0, -0.225, -0.12), (0.04, -0.215, -0.11)], [0.01, 0.012, 0.01], 6),
          "bow", "head", head, paint=True)

    # arms in puffed sleeves, legs with shoes
    for side, s in (("L", 1), ("R", -1)):
        b.add(sphere(), "dress_light", f"arm.{side}", trs((s * 0.13, 0, 0.43), (0, 0, 0), (0.07, 0.07, 0.06)))
        b.add(tube(rig.arm(s), [0.035, 0.032, 0.03], 10), "skin", f"arm.{side}")
        b.add(sphere(), "skin", f"arm.{side}", trs((s * 0.15, 0, 0.25), (0, 0, 0), 0.042))
        b.add(tube(rig.leg(s), [0.04, 0.036, 0.034], 10), "skin", f"leg.{side}")
        b.add(sphere(), "shoe", f"leg.{side}", trs((s * 0.06, -0.03, 0.035), (0, 0, 0), (0.05, 0.08, 0.04)))
    return b.build()


def animate(rig):
    # gripped by the shoulders: arms flailing out to the sides, the legs kicking one after the other, swinging
    keys = {
        "arm.L": bone_keys(cycle(12, turn((LIFT["L"], 135), (FORWARD, 20)), turn((LIFT["L"], 95), (FORWARD, -10)))),
        "arm.R": bone_keys(cycle(12, turn((LIFT["R"], 95), (FORWARD, -10)), turn((LIFT["R"], 135), (FORWARD, 20)))),
        "leg.L": bone_keys(cycle(24, turn((FORWARD, 35)), turn((FORWARD, -15)))),
        "leg.R": bone_keys(cycle(24, turn((FORWARD, -15)), turn((FORWARD, 35)))),
        "body": bone_keys(cycle(24, turn(((0, 1, 0), 6)), turn(((0, 1, 0), -6)))),
        "head": bone_keys(cycle(24, turn(((0, 0, 1), 15)), turn(((0, 0, 1), -15)))),
    }
    toy.action(rig, "dangle", 24, keys)
    keys = {
        "arm.L": bone_keys(cycle(48, turn((LIFT["L"], 8)), turn((LIFT["L"], 4)))),
        "arm.R": bone_keys(cycle(48, turn((LIFT["R"], 8)), turn((LIFT["R"], 4)))),
        "head": bone_keys(cycle(48, turn(((0, 1, 0), 5)), turn(((0, 1, 0), -5)))),
    }
    toy.action(rig, "idle", 48, keys)


rig, mesh = build()
mesh = refine.refine(rig, mesh, faces=3000)
animate(rig)
out = toy.out_dir()
toy.export(rig, mesh, os.path.join(out, "princess.glb"))
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(out, "princess.blend"))
print("RESULT princess", tuple(round(d, 2) for d in mesh.dimensions), "verts", len(mesh.data.vertices))
