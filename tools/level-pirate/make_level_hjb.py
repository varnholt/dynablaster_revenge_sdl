# Writes the pirate ship's level.hjb: no geometry (the ship is a .glb), only the cameras the game looks for -
# "Camera001" (the fly-in), "Upper" and "Lower" (following the players) and "Shadow Cam" (the blocks' shadows).
#   python make_level_hjb.py <out level.hjb> [cameras.json ...]
# Cameras not in a cameras.json are copied from the castle as they are. A cameras.json holds per camera
# {"fov": [[time, radians]...], "position": [[time, x, y, z]...], "rotation": [[time, x, y, z, w]...]}
# in the engine's convention (rotation keys absolute here, written as the deltas the loader accumulates).
import json
import os
import struct
import sys

from hjb_cameras import ID_CAMERA, TERMINATOR, Reader, quat_mul

HERE = os.path.dirname(os.path.abspath(__file__))
CASTLE = os.path.join(HERE, "..", "..", "client", "data", "level-castle", "level.hjb")
ANIMATION_ID = 2000
TRACK_VALUE, TRACK_POSITION, TRACK_ROTATION, TRACK_SCALE, TRACK_VISIBILITY = 2001, 2002, 2003, 2004, 2005
NAMES = ("Camera001", "Upper", "Lower", "Shadow Cam")


def name_bytes(name):
    data = name.encode("latin-1")
    return bytes([len(data)]) + data


def chunk(cid, name, payload):
    return struct.pack("<i", cid) + name_bytes(name) + struct.pack("<i", len(payload)) + payload


def castle_chunks():
    """The raw camera chunks of the castle, with the header's and the materials' chunk ids."""
    data = open(CASTLE, "rb").read()
    reader = Reader(data)
    reader.chunk()
    header = data[:reader.pos]
    materials_start = reader.pos
    material_id = struct.unpack_from("<i", data, materials_start)[0]
    reader.chunk()
    cameras = {}

    def nodes(r, depth):
        while r.pos < r.end:
            start = r.pos
            cid, name, _ = r.chunk()
            if cid == TERMINATOR:
                return
            if cid == ID_CAMERA:
                if depth:
                    raise SystemExit(f"camera {name} has a parent, its transform would be lost")
                cameras[name] = data[start:r.pos]
            nodes(r, depth + 1)

    nodes(reader, 0)
    return header, material_id, cameras


def keys_track(cid, name, keys, width):
    payload = struct.pack("<i", len(keys))
    for key in keys:
        payload += struct.pack("<i", int(key[0])) + struct.pack(f"<{width}f", *key[1:1 + width])
    return chunk(cid, name, payload)


def inverse(q):
    x, y, z, w = q
    return (-x, -y, -z, w)


def camera_chunk(name, cam):
    rotation = cam["rotation"]
    deltas = [rotation[0]]
    for prev, cur in zip(rotation, rotation[1:]):
        deltas.append([cur[0], *quat_mul(inverse(prev[1:]), cur[1:])])
    tracks = (keys_track(TRACK_VALUE, "Value", cam["fov"], 1)
              + keys_track(TRACK_POSITION, "Position", cam["position"], 3)
              + keys_track(TRACK_ROTATION, "Rotation", deltas, 4)
              + keys_track(TRACK_SCALE, "Scale", [], 7)
              + keys_track(TRACK_VISIBILITY, "Visible", [], 0)
              + keys_track(TRACK_VALUE, "Value", [], 1))
    return chunk(ID_CAMERA, name, chunk(ANIMATION_ID, "Animation", tracks))


def main():
    out = sys.argv[1]
    header, material_id, cameras = castle_chunks()
    for path in sys.argv[2:]:
        for name, cam in json.load(open(path)).items():
            cameras[name] = camera_chunk(name, cam)
    data = header + chunk(material_id, "Materials", struct.pack("<i", 0))
    for name in NAMES:
        data += cameras[name] + struct.pack("<i", TERMINATOR)
    data += struct.pack("<i", TERMINATOR)
    os.makedirs(os.path.dirname(os.path.abspath(out)), exist_ok=True)
    open(out, "wb").write(data)
    print("wrote", out, len(data), "bytes")


if __name__ == "__main__":
    main()
