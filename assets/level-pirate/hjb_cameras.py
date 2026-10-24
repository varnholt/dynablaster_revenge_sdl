# Reads the cameras of a level's .hjb scene (client/src/engine/nodes/scenegraph.cpp, tools/chunk.cpp):
#   python hjb_cameras.py <level.hjb>
# prints every camera's keys and its world transform at a few times, z up, looking down its local -z.
import json
import struct
import sys

import numpy as np

HEADER, MATERIALS, TERMINATOR = 1, 2, 0xFFFF
ID_CAMERA, ID_DUMMY = 4, 2


class Reader:
    def __init__(self, data, pos=0, end=None):
        self.data, self.pos, self.end = data, pos, len(data) if end is None else end

    def int(self):
        value, = struct.unpack_from("<i", self.data, self.pos)
        self.pos += 4
        return value

    def float(self):
        value, = struct.unpack_from("<f", self.data, self.pos)
        self.pos += 4
        return value

    def floats(self, n):
        values = struct.unpack_from(f"<{n}f", self.data, self.pos)
        self.pos += 4 * n
        return values

    def name(self):
        length = self.data[self.pos]
        text = self.data[self.pos + 1:self.pos + 1 + length].rstrip(b"\0").decode("latin-1")
        self.pos += 1 + length
        return text

    def chunk(self):
        """id, name and a reader limited to the chunk; the stream continues behind it."""
        cid = self.int()
        if cid == TERMINATOR:
            return cid, None, None
        name = self.name()
        size = self.int()
        sub = Reader(self.data, self.pos, self.pos + size)
        self.pos += size
        return cid, name, sub


def quat_mul(a, b):
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return (aw * bx + ax * bw + ay * bz - az * by,
            aw * by + ay * bw + az * bx - ax * bz,
            aw * bz + az * bw + ax * by - ay * bx,
            aw * bw - ax * bx - ay * by - az * bz)


def track(reader, width):
    _, name, sub = reader.chunk()
    keys = []
    for _ in range(sub.int()):
        time = sub.int()
        keys.append((time, sub.floats(width)))
    return name, keys


def camera(sub):
    _, _, anim = sub.chunk()
    _, fov = track(anim, 1)
    _, pos = track(anim, 3)
    _, rot = track(anim, 4)
    # rotations are stored as deltas
    acc = []
    for i, (t, q) in enumerate(rot):
        if i:
            prev = acc[-1][1]
            q2 = quat_mul(prev, q)
            if sum(a * b for a, b in zip(prev, q2)) < 0:
                q2 = tuple(-c for c in q2)
            q = q2
        acc.append((t, q))
    return dict(fov=fov, position=pos, rotation=acc)


def sample(keys, time, lerp):
    if time <= keys[0][0]:
        return keys[0][1]
    if time >= keys[-1][0]:
        return keys[-1][1]
    for (t0, a), (t1, b) in zip(keys, keys[1:]):
        if t0 <= time < t1:
            return lerp(a, b, (time - t0) / (t1 - t0))


def slerp(a, b, f):
    a, b = np.array(a), np.array(b)
    d = float(a @ b)
    if d < 0:
        b, d = -b, -d
    if d > 0.9995:
        q = a + (b - a) * f
    else:
        th = np.arccos(d)
        q = (np.sin((1 - f) * th) * a + np.sin(f * th) * b) / np.sin(th)
    return tuple(q / np.linalg.norm(q))


def world(cam, time):
    """Column vector 4x4: the engine's Matrix(Quat) is the transposed rotation of q."""
    x, y, z, w = sample(cam["rotation"], time, slerp)
    r = np.array([[1 - 2 * (y * y + z * z), 2 * (x * y - w * z), 2 * (x * z + w * y)],
                  [2 * (x * y + w * z), 1 - 2 * (x * x + z * z), 2 * (y * z - w * x)],
                  [2 * (x * z - w * y), 2 * (y * z + w * x), 1 - 2 * (x * x + y * y)]]).T
    p = sample(cam["position"], time, lambda a, b, f: tuple(np.array(a) + (np.array(b) - np.array(a)) * f))
    m = np.eye(4)
    m[:3, :3] = r
    m[:3, 3] = p
    fov = sample(cam["fov"], time, lambda a, b, f: (a[0] + (b[0] - a[0]) * f,))[0]
    return m, fov


def read(path):
    data = open(path, "rb").read()
    reader = Reader(data)
    reader.chunk()  # header
    reader.chunk()  # materials
    cameras = {}

    def nodes(r, depth):
        while r.pos < r.end:
            cid, name, sub = r.chunk()
            if cid == TERMINATOR:
                return
            if cid == ID_CAMERA:
                cameras[name] = camera(sub)
            nodes(r, depth + 1)

    nodes(reader, 0)
    return cameras


if __name__ == "__main__":
    cams = read(sys.argv[1])
    out = {}
    for name, cam in cams.items():
        print(name, "keys fov/pos/rot", len(cam["fov"]), len(cam["position"]), len(cam["rotation"]),
              "time", cam["position"][0][0] if cam["position"] else None, "..",
              cam["position"][-1][0] if cam["position"] else None)
        out[name] = {}
        times = sorted({t for t, _ in cam["position"]} | {t for t, _ in cam["rotation"]})
        for t in (times[0], times[len(times) // 2], times[-1]):
            m, fov = world(cam, t)
            eye = m[:3, 3]
            look = -m[:3, 2]
            print(f"   t {t:6d} eye {np.round(eye, 2)} look {np.round(look, 3)} up {np.round(m[:3, 1], 3)} "
                  f"fov {np.degrees(fov):.1f}")
        out[name] = {str(t): world(cam, t)[0].tolist() + [world(cam, t)[1]] for t in times}
    if len(sys.argv) > 2:
        json.dump(out, open(sys.argv[2], "w"))
