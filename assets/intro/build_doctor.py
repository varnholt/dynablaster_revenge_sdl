# The doctor whose lab the dragon breaks out of: bald with white tufts and a big moustache, a white lab coat.
# Actions: "run", "turn" (shuffling round on the spot), "fall" (sees the dragon, flinches and falls back onto his
# bottom, once) and "cower" (sitting where he fell, hands on his head, rocking and kicking).
#   blender --background --factory-startup --python build_doctor.py -- <out dir>
# writes doctor.glb and doctor.blend, about 1.05 high, the player being 1.
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import bpy  # noqa: E402

import refine  # noqa: E402
import toy  # noqa: E402
from toy import FORWARD, LIFT, cone, mirror, sphere, trs, tube, turn  # noqa: E402

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
    rig = toy.Body(b, hip=0.27, shoulder=0.57, sx=0.16, neck=0.63, hand=0.3, hx=0.075, foot=0.1)
    # the coat's skirt below the hips, swinging after him (see FOLLOW)
    b.bone("hem", (0, 0, rig.hip), (0, 0, 0.13), "pelvis")
    hanging = rig.torso_weights(hem="hem")

    def trunk(co):
        """The coat's skirt kicked along by the thigh on its side, the rest swinging on the hem."""
        weights = hanging(co)
        # under half, so the coat stays part of the trunk's surface (refine shells by the strongest bone)
        kick = weights.get("hem", 0.0) * 0.45 * min(abs(co.x) / 0.08, 1.0)
        weights["hem"] -= kick
        weights["thigh.L" if co.x > 0 else "thigh.R"] = kick
        return weights

    # lab coat: flared below, round shoulders, a collar, buttons and a pen in the breast pocket
    b.add(cone(0.25, 0.16, 0.5, 32), "coat", weights=trunk, matrix=trs((0, 0, 0.13)))
    b.add(sphere(), "coat", "chest", trs((0, 0, 0.58), (0, 0, 0), (0.19, 0.16, 0.08)))
    # a collar round the neck, so the crease where the head sits on the coat is the coat's
    b.add(sphere(), "coat", "chest", trs((0, 0.01, 0.655), (0, 0, 0), (0.14, 0.13, 0.04)))
    for s in (1, -1):
        side = (lambda m: m) if s > 0 else mirror
        b.add(cone(0.06, 0.0, 0.14, 4), "coat_shade", "chest", side(trs((0.06, -0.12, 0.62), (200, 0, 30),
                                                                         (1, 0.4, 1))), paint=True)
    for z in (0.22, 0.34, 0.46):
        b.add(sphere(), "button", weights=trunk, matrix=trs((0, -0.2 - (0.46 - z) * 0.2, z), (0, 0, 0), 0.018),
              paint=True)
    b.add(tube([(0.0, 0.0, 0.0), (0.0, 0.0, 0.07)], [0.012, 0.012], 6), "pen", "chest", trs((0.09, -0.155, 0.47)),
          paint=True)

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

    # arms in coat sleeves bending at the elbow, legs bending at the knee, shoes
    for side, s in (("L", 1), ("R", -1)):
        b.add(tube(rig.arm(s), [0.06, 0.057, 0.055, 0.057, 0.06], 12), "coat", weights=rig.arm_weights(side))
        b.add(sphere(), "skin", f"hand.{side}", trs((s * 0.19, 0, 0.27), (0, 0, 0), 0.05))
        b.add(tube(rig.leg(s), [0.055, 0.053, 0.05, 0.047, 0.045], 10), "trousers", weights=rig.leg_weights(side))
        b.add(sphere(), "shoe", f"foot.{side}", trs((s * 0.075, -0.04, 0.04), (0, 0, 0), (0.06, 0.1, 0.045)))
    return b.build()


# the hem swings after the body: a soft spring, damped so it settles within a stride
FOLLOW = [("hem", 0.3, 0.72)]
UP = (0, 0, 1)  # turns about the vertical: the body twisting
SIDE = (0, 1, 0)  # tips the body towards its left (+x)


def leg(side, thigh, shin, foot=0.0):
    """A leg's pose: thigh forwards (negative) or back, the knee bent by shin, the toes down by foot."""
    return {f"thigh.{side}": turn((FORWARD, thigh)), f"shin.{side}": turn((FORWARD, shin)),
            f"foot.{side}": turn((FORWARD, foot))}


def arm(side, swing, elbow, out=0.0, twist=0.0):
    """An arm's pose: swung back (positive) or forwards, out to the side, the elbow bent forwards by elbow."""
    return {f"upper_arm.{side}": turn((LIFT[side], out), (FORWARD, swing), (UP, twist)),
            f"forearm.{side}": turn((FORWARD, -elbow))}


def mirrored(side):
    return "R" if side == "L" else "L"


def run(rig):
    """A hurried run, 16 frames for two strides: per stride contact, down, passing, up; the arms pump against the
    legs a frame behind them, hips and shoulders twist against each other, the body leans in."""
    phases = [  # lead leg (thigh, shin, foot), trailing leg, root height, arm on the lead side, other arm
        ((-38, 8, -12), (28, 55, 25), 0.0, (38, 60), (-48, 95)),
        ((-22, 38, 0), (8, 95, 15), -0.035, (22, 70), (-30, 95)),
        ((5, 20, 0), (-25, 110, 10), 0.0, (-5, 80), (5, 80)),
        ((30, 25, 30), (-50, 70, -5), 0.04, (-35, 95), (30, 60)),
    ]
    legs, arms = [], []
    for stride, lead in enumerate(("L", "R")):
        other = mirrored(lead)
        sign = 1 if lead == "L" else -1
        for k, (front, back, height, lead_arm, other_arm) in enumerate(phases):
            frame = 1 + stride * 8 + k * 2
            twist = 8 * sign * (1 if k < 2 else -0.5)
            legs.append((frame, {**leg(lead, *front), **leg(other, *back), "root": (turn(), (0, 0, height)),
                                 "pelvis": turn((UP, twist)), "spine": turn((FORWARD, 12)),
                                 "chest": turn((UP, -twist * 1.4)), "neck": turn((FORWARD, -8)),
                                 "head": turn((FORWARD, -4 + height * 60))}))
            arms.append((frame + 1, {**arm(lead, *lead_arm, out=12), **arm(other, *other_arm, out=12)}))
    legs.append((17, legs[0][1]))
    arms.append((18, arms[0][1]))
    keys = toy.poses(legs)
    keys.update(toy.poses(arms))
    toy.action(rig, "run", 16, keys, follow=FOLLOW)


def turn_round(rig):
    """Shuffling round on the spot, the intro turns him: short quick steps, the body leaning into the turn ahead of
    the legs, the arms held out a little for balance."""
    def pose(lead, lift):
        other = mirrored(lead)
        return {"root": (turn(), (0, 0, lift * 0.02)), "pelvis": turn((UP, -6)), "chest": turn((UP, -10)),
                "spine": turn((FORWARD, 6)), "head": turn((UP, -14), (FORWARD, -6)),
                **leg(lead, -25 * lift, 55 * lift, 10 * lift), **leg(other, 0, 4),
                **arm("L", -10, 40, out=25), **arm("R", -10, 40, out=25)}
    keys = [(1, pose("L", 0)), (3, pose("L", 1)), (5, pose("R", 0)), (7, pose("R", 1)), (9, pose("L", 0))]
    toy.action(rig, "turn", 8, toy.poses(keys), follow=FOLLOW)


SIT = (0, 0.19, -0.19)  # the root's offset with him on his bottom


def cowering(lead, kick, rock, shake):
    """Sitting where he fell, hands clapped to his head."""
    other = mirrored(lead)
    return {"root": (turn(), SIT), "pelvis": turn((FORWARD, -12 - rock * 6)),
            "spine": turn((FORWARD, 10 + rock * 8)), "chest": turn((FORWARD, 6 + rock * 4)),
            "neck": turn((UP, shake * 0.4)), "head": turn((UP, shake), (FORWARD, -14)),
            **arm("L", -45, 135, out=88, twist=-25), **arm("R", -45, 135, out=88, twist=25),
            **leg(lead, -78 - kick * 18, 30 + kick * 25, -15), **leg(other, -72 + kick * 6, 38, -10)}


def fall(rig):
    """He sees the dragon above the lab: looks up, flinches with his hands up before his face, stumbles back with
    his arms flung out and lands on his bottom, bounces once and ends cowering (the first pose of "cower"); the
    head whips after the body."""
    keys = [
        (1, {"neck": turn((FORWARD, -15)), "head": turn((FORWARD, -20)), **arm("L", -10, 20, out=20),
             **arm("R", -10, 20, out=20)}),
        (4, {"root": (turn(), (0, 0.02, 0.03)), "chest": turn((FORWARD, -12)), "neck": turn((FORWARD, -15)),
             "head": turn((FORWARD, -25)), **arm("L", -70, 95, out=40), **arm("R", -70, 95, out=40),
             **leg("L", -8, 12), **leg("R", 4, 8)}),
        (8, {"root": (turn(), (0, 0.1, 0.04)), "pelvis": turn((FORWARD, -30)), "chest": turn((FORWARD, -8)),
             "head": turn((FORWARD, 15)), **arm("L", -40, 50, out=110), **arm("R", -38, 50, out=105),
             **leg("L", -50, 20, -15), **leg("R", -35, 35, -5)}),
        (12, {"root": (turn(), SIT), "pelvis": turn((FORWARD, -20)), "chest": turn((FORWARD, -5)),
              "head": turn((FORWARD, -20)), **arm("L", -20, 40, out=95), **arm("R", -25, 45, out=100),
              **leg("L", -95, 25, -20), **leg("R", -85, 35, -15)}),
        (15, {"root": (turn(), (0, 0.19, -0.15)), "pelvis": turn((FORWARD, -28)), "chest": turn((FORWARD, 5)),
              "head": turn((FORWARD, 10)), **arm("L", -30, 90, out=90), **arm("R", -30, 90, out=90),
              **leg("L", -105, 10, -25), **leg("R", -95, 20, -20)}),
        (20, cowering("L", 0, 0, 18)),
        (24, cowering("L", 0, 0, 18)),
    ]
    toy.action(rig, "fall", 24, toy.poses(keys), cyclic=False, follow=FOLLOW)


def cower(rig):
    """Sitting where he fell, hands clapped to his head: rocking to and fro, shaking his head, kicking his legs one
    after the other."""
    keys = [(1, cowering("L", 0, 0, 18)), (5, cowering("L", 1, 1, 0)), (9, cowering("R", 0, 0, -18)),
            (13, cowering("R", 1, 1, 0)), (17, cowering("L", 0, 0, 18))]
    toy.action(rig, "cower", 16, toy.poses(keys), follow=FOLLOW)


def animate(rig):
    run(rig)
    turn_round(rig)
    fall(rig)
    cower(rig)


rig, mesh = build()
mesh = refine.refine(rig, mesh, faces=3000, shell=toy.body_shells)
animate(rig)
out = toy.out_dir()
toy.export(rig, mesh, os.path.join(out, "doctor.glb"))
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(out, "doctor.blend"))
print("RESULT doctor", tuple(round(d, 2) for d in mesh.dimensions), "verts", len(mesh.data.vertices))
