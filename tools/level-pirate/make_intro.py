# The pirate ship's fly-in ("Camera001"), like the space level's: from far out at sea, the whole ship in view with
# the moon behind it, gliding in to where the game's camera takes over (the castle's last fly-in key).
#   python make_intro.py <out intro.json>
# Times are the game's camera ticks (3750 a second), see LevelPirate for when the game's cameras take over.
import json
import math
import sys

import numpy as np

END = 15000
STEP = 100
FOV = math.radians(62.6)

# eye and the point looked at
FAR_EYE = np.array((-50.0, -64.0, 15.0))
FAR_TARGET = np.array((-5.0, -5.5, 10.0))
BEND = np.array((-14.0, -58.0, 24.0))  # pulls the path along the ship's side before it turns in
GAME_EYE = np.array((6.04, -16.2, 13.34))
GAME_LOOK = np.array((0.002, 0.627, -0.779))
GAME_TARGET = GAME_EYE + GAME_LOOK / np.linalg.norm(GAME_LOOK) * 17.0


def smootherstep(t):
    return t * t * t * (t * (t * 6 - 15) + 10)


def bezier(a, b, c, t):
    return (1 - t) ** 2 * a + 2 * (1 - t) * t * b + t * t * c


def quaternion(matrix):
    """(x, y, z, w) of a rotation matrix."""
    m = matrix
    trace = m[0, 0] + m[1, 1] + m[2, 2]
    if trace > 0:
        s = math.sqrt(trace + 1.0) * 2
        return ((m[2, 1] - m[1, 2]) / s, (m[0, 2] - m[2, 0]) / s, (m[1, 0] - m[0, 1]) / s, 0.25 * s)
    if m[0, 0] > m[1, 1] and m[0, 0] > m[2, 2]:
        s = math.sqrt(1.0 + m[0, 0] - m[1, 1] - m[2, 2]) * 2
        return (0.25 * s, (m[0, 1] + m[1, 0]) / s, (m[0, 2] + m[2, 0]) / s, (m[2, 1] - m[1, 2]) / s)
    if m[1, 1] > m[2, 2]:
        s = math.sqrt(1.0 + m[1, 1] - m[0, 0] - m[2, 2]) * 2
        return ((m[0, 1] + m[1, 0]) / s, 0.25 * s, (m[1, 2] + m[2, 1]) / s, (m[0, 2] - m[2, 0]) / s)
    s = math.sqrt(1.0 + m[2, 2] - m[0, 0] - m[1, 1]) * 2
    return ((m[0, 2] + m[2, 0]) / s, (m[1, 2] + m[2, 1]) / s, 0.25 * s, (m[1, 0] - m[0, 1]) / s)


def look_at(eye, target):
    """The engine's rotation key for a camera at eye looking at target, z up: the conjugate of the camera's
    rotation (its Matrix(Quat) is the transposed rotation)."""
    back = eye - target
    back /= np.linalg.norm(back)
    right = np.cross((0.0, 0.0, 1.0), back)
    right /= np.linalg.norm(right)
    up = np.cross(back, right)
    x, y, z, w = quaternion(np.stack([right, up, back], axis=1))
    return (-x, -y, -z, w)


def main():
    positions, rotations = [], []
    previous = None
    for time in range(0, END + 1, STEP):
        t = smootherstep(time / END)
        eye = bezier(FAR_EYE, BEND, GAME_EYE, t)
        # the view turns towards the field a little later than the camera moves
        target = FAR_TARGET + (GAME_TARGET - FAR_TARGET) * smootherstep(min(1.0, time / END * 1.08))
        q = np.array(look_at(eye, target))
        if previous is not None and q @ previous < 0:
            q = -q
        previous = q
        positions.append([time, *eye.tolist()])
        rotations.append([time, *q.tolist()])
    json.dump({"Camera001": {"fov": [[0, FOV]], "position": positions, "rotation": rotations}},
              open(sys.argv[1], "w"), indent=1)
    print("wrote", sys.argv[1], len(positions), "keys")


if __name__ == "__main__":
    main()
