# Reading and writing a binary glTF (.glb) as its json and its binary chunk, for touches after Blender's export:
# detail images (detail.py), a scene's atmosphere. Plain python.
import json
import struct
import zlib


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


def png(rgba):
    """A PNG of rgba, a numpy array of height x width x 4 bytes, its top row first."""
    height, width = rgba.shape[:2]
    raw = b"".join(b"\0" + rgba[y].tobytes() for y in range(height))

    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)

    header = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    return b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", header) + chunk(b"IDAT", zlib.compress(raw, 6)) + chunk(b"IEND", b"")


def replace_image(path, name, data, mime="image/png"):
    """Swaps the glb's image called name for data. Blender exports every image in one format, and a data
    texture (the water's depth, foam and shore distance) must not go through JPEG: its 8x8 blocks and halved
    colour resolution show as squares once a shader sharpens the values."""
    gltf, binary = read(path)
    swap = {image["bufferView"]: data for image in gltf.get("images", []) if image.get("name") == name}
    for image in gltf.get("images", []):
        if image.get("name") == name:
            image["mimeType"] = mime
    # the binary chunk laid out anew, every view at its 4 byte aligned place
    packed = bytearray()
    views = gltf.get("bufferViews", [])
    for index in sorted(range(len(views)), key=lambda i: views[i].get("byteOffset", 0)):
        view = views[index]
        start = view.get("byteOffset", 0)
        content = swap.get(index, binary[start:start + view["byteLength"]])
        while len(packed) % 4:
            packed.append(0)
        view["byteOffset"] = len(packed)
        view["byteLength"] = len(content)
        packed += content
    write(path, gltf, packed)
