# Reading and writing a binary glTF (.glb) as its json and its binary chunk, for touches after Blender's export:
# detail images (detail.py), a scene's atmosphere. Plain python.
import json
import struct


def read(path):
    """The glb at path as (its json as a dict, its binary chunk as a bytearray)."""
    with open(path, "rb") as f:
        data = f.read()
    length = struct.unpack_from("<I", data, 12)[0]
    gltf = json.loads(data[20:20 + length])
    binary = bytearray(data[20 + length + 8:]) if len(data) > 20 + length else bytearray()
    return gltf, binary


def write(path, gltf, binary):
    while len(binary) % 4:
        binary.append(0)
    if gltf.get("buffers"):
        gltf["buffers"][0]["byteLength"] = len(binary)
    text = json.dumps(gltf, separators=(",", ":")).encode()
    text += b" " * (-len(text) % 4)
    out = struct.pack("<III", 0x46546C67, 2, 12 + 8 + len(text) + 8 + len(binary))
    out += struct.pack("<II", len(text), 0x4E4F534A) + text
    out += struct.pack("<II", len(binary), 0x004E4942) + bytes(binary)
    with open(path, "wb") as f:
        f.write(out)


def set_scene_extras(path, key, value):
    """Sets key in the extras of the glb's scene."""
    gltf, binary = read(path)
    scene = gltf["scenes"][gltf.get("scene", 0)]
    scene.setdefault("extras", {})[key] = value
    write(path, gltf, binary)
