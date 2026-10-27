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
from toy import FORWARD, LIFT, cone, mirror, sphere, trs, tube, turn  # noqa: E402

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
    rig = toy.Body(b, hip=0.24, shoulder=0.45, sx=0.12, neck=0.5, hand=0.26, hx=0.06, foot=0.08)
    # the dress's bell below the hips and the two bunches of hair swing after her (see FOLLOW)
    b.bone("hem", (0, 0, rig.hip), (0, 0, 0.17), "pelvis")
    hanging = rig.torso_weights(hem="hem")

    def dress(co):
        """The bell kicked along by the thigh on its side, the rest swinging on the hem."""
        weights = hanging(co)
        # under half, so the dress stays part of the trunk's surface (refine shells by the strongest bone)
        kick = weights.get("hem", 0.0) * 0.4 * min(abs(co.x) / 0.08, 1.0)
        weights["hem"] -= kick
        weights["thigh.L" if co.x > 0 else "thigh.R"] = kick
        return weights

    # dress: a bell with a lighter hem and a bodice
    b.add(cone(0.24, 0.1, 0.32, 32), "dress", weights=dress, matrix=trs((0, 0, 0.17)))
    b.add(tube([(0, 0, 0.18), (0, 0, 0.19)], [0.245, 0.245], 32, cap=False), "dress_light", weights=dress)
    b.add(sphere(), "dress", "chest", trs((0, 0, 0.45), (0, 0, 0), (0.13, 0.11, 0.09)))
    b.add(sphere(), "bow", "chest", trs((0, -0.1, 0.42), (0, 0, 0), (0.05, 0.03, 0.035)))

    # head: face, eyes, blush, a mop of hair with two bunches and a bow on top
    head = trs((0, 0, HEAD))
    for side, s in (("L", 1), ("R", -1)):
        b.bone(f"bunch.{side}", (s * 0.22, 0.06, HEAD + 0.04), (s * 0.3, 0.1, HEAD - 0.22), "head")
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
        name = "bunch.L" if s > 0 else "bunch.R"

        def bunch(co, name=name):
            """The bunch hangs from the head, more and more on its own bone towards its tip."""
            down = toy.smoothstep(HEAD + 0.05, HEAD - 0.1, co.z)
            return {"head": 1.0 - down, name: down}
        b.add(tube([(0.2, 0.05, 0.05), (0.3, 0.08, -0.08), (0.3, 0.1, -0.22)], [0.09, 0.08, 0.04], 14),
              "hair", weights=bunch, matrix=head @ side(trs()), shell="hair")
        b.add(sphere(), "bow", "head", head @ side(trs((0.08, 0.0, 0.27), (0, -30, 0), (0.08, 0.04, 0.06))), shell="hair")
    b.add(sphere(), "bow", "head", head @ trs((0, 0.0, 0.25), (0, 0, 0), (0.04, 0.04, 0.04)), shell="hair")
    b.add(tube([(-0.04, -0.215, -0.11), (0, -0.225, -0.12), (0.04, -0.215, -0.11)], [0.01, 0.012, 0.01], 6),
          "bow", "head", head, paint=True)

    # arms in puffed sleeves bending at the elbow, legs bending at the knee, shoes
    for side, s in (("L", 1), ("R", -1)):
        b.add(sphere(), "dress_light", f"upper_arm.{side}", trs((s * 0.13, 0, 0.43), (0, 0, 0), (0.07, 0.07, 0.06)))
        b.add(tube(rig.arm(s), [0.035, 0.033, 0.032, 0.031, 0.03], 10), "skin", weights=rig.arm_weights(side))
        b.add(sphere(), "skin", f"hand.{side}", trs((s * 0.15, 0, 0.25), (0, 0, 0), 0.042))
        b.add(tube(rig.leg(s), [0.04, 0.038, 0.036, 0.035, 0.034], 10), "skin", weights=rig.leg_weights(side))
        b.add(sphere(), "shoe", f"foot.{side}", trs((s * 0.06, -0.03, 0.035), (0, 0, 0), (0.05, 0.08, 0.04)))
    return b.build()


# the dress and the bunches of hair swing after her: the hair lighter and bouncier than the dress
FOLLOW = [("hem", 0.3, 0.7), ("bunch.L", 0.22, 0.8), ("bunch.R", 0.22, 0.8)]
UP = (0, 0, 1)  # turns about the vertical
SIDE = (0, 1, 0)  # tips the body towards its left (+x)


def leg(side, thigh, shin, foot=0.0):
    """A leg's pose: thigh forwards (negative) or back, the knee bent by shin, the toes down by foot."""
    return {f"thigh.{side}": turn((FORWARD, thigh)), f"shin.{side}": turn((FORWARD, shin)),
            f"foot.{side}": turn((FORWARD, foot))}


def arm(side, swing, elbow, out=0.0, wrist=0.0):
    """An arm's pose: swung back (positive) or forwards, out to the side, the elbow bent forwards by elbow."""
    return {f"upper_arm.{side}": turn((LIFT[side], out), (FORWARD, swing)),
            f"forearm.{side}": turn((FORWARD, -elbow)), f"hand.{side}": turn((FORWARD, -wrist))}


def dangle(rig):
    """Gripped by the shoulders in the dragon's claws: the arms flail up and out, one after the other with the
    elbows whipping, the legs kick and bicycle, the body twists and wriggles, the head shakes; 24 frames."""
    def pose(k):
        # k runs 0..3 round the loop; the left side leads, the right follows half a loop behind
        flail = [(160, 30, 15), (75, -25, 110), (130, 15, 45), (60, -35, 120)]
        kick = [(40, 95), (-45, 15), (15, 110), (-55, 25)]
        a, b = flail[k], flail[(k + 2) % 4]
        left, right = kick[k], kick[(k + 2) % 4]
        sway = (1, 0, -1, 0)[k]
        return {"pelvis": turn((UP, sway * 10), (SIDE, sway * 4)), "chest": turn((UP, -sway * 8)),
                "neck": turn((UP, -sway * 6)), "head": turn((UP, -sway * 16), (FORWARD, -6)),
                **arm("L", -a[1], a[2], out=a[0], wrist=a[2] * 0.3),
                **arm("R", -b[1], b[2], out=b[0], wrist=b[2] * 0.3),
                **leg("L", *left, 20), **leg("R", *right, 20)}
    keys = [(1 + k * 6, pose(k)) for k in range(4)] + [(25, pose(0))]
    toy.action(rig, "dangle", 24, toy.poses(keys), follow=FOLLOW)


def idle(rig):
    """Standing, breathing, the weight shifting from foot to foot, looking about; 48 frames."""
    def pose(breath, shift):
        return {"root": (turn(), (shift * 0.006, 0, 0)), "pelvis": turn((SIDE, shift * 3)),
                "chest": turn((FORWARD, -breath * 3), (SIDE, -shift * 2)),
                "head": turn((UP, shift * 12), (SIDE, -shift * 4)),
                **arm("L", 0, 10 + breath * 6, out=8 + breath * 3), **arm("R", 0, 10 + breath * 6, out=8 + breath * 3),
                **leg("L", -2, 4), **leg("R", 2, 4)}
    keys = [(1, pose(0, 1)), (13, pose(1, 0)), (25, pose(0, -1)), (37, pose(1, 0)), (49, pose(0, 1))]
    toy.action(rig, "idle", 48, toy.poses(keys), follow=FOLLOW)


def animate(rig):
    dangle(rig)
    idle(rig)


rig, mesh = build()
mesh = refine.refine(rig, mesh, faces=3600, density={"hair": 2.5}, shell=toy.body_shells)
animate(rig)
out = toy.out_dir()
toy.export(rig, mesh, os.path.join(out, "princess.glb"))
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(out, "princess.blend"))
print("RESULT princess", tuple(round(d, 2) for d in mesh.dimensions), "verts", len(mesh.data.vertices))
