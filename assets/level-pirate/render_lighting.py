# The pirate ship's lighting for the blocks, bombs and players, rendered from the unbaked scene (build_pirate.py):
#   probe_day/, probe_night/ - the light all round the middle of the field by day and by night (lighting.cube()),
#                              which build_all.py turns into the light probes (probes.py)
#   shadow-cookie.tga  - the mast, its sail and the rigging seen from the sun, the blocks' and players' shadows
#   cameras.json       - the "Shadow Cam" that projects the cookie, for make_level_hjb.py
# Run headless: blender --background --factory-startup --python render_lighting.py -- <out dir>
import json
import math
import os
import sys

import bpy
import numpy as np
from mathutils import Vector

HERE = os.path.dirname(os.path.abspath(__file__))
ARGS = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
OUT = ARGS[0] if ARGS else os.path.join(HERE, "out")

os.environ["PIRATE_PREVIEW"] = "1"
build = {"__name__": "build_pirate", "__file__": os.path.join(HERE, "build_pirate.py")}
exec(compile(open(build["__file__"]).read(), build["__file__"], "exec"), build)

FIELD_CENTER = Vector((6.5, -5.5, 0.6))
MOON = build["MOON"]
SHADOW = (0.50, 0.56, 0.72)


def setup():
    # the sky dome would keep the moon and the sky's light out, the world stands in for it
    for obj in bpy.data.objects:
        if obj.name.startswith("sky"):
            obj.hide_render = True
    build["setup_cycles"]()
    bpy.context.scene.cycles.samples = 96
    bpy.context.scene.render.film_transparent = False
    build["light_rig"]()


def ortho_camera(name, direction, target, scale, width, height, distance=40.0, up=Vector((0, 0, 1))):
    scene = bpy.context.scene
    cam = bpy.data.cameras.new(name)
    cam.type = "ORTHO"
    cam.ortho_scale = scale
    cam.clip_end = 200
    obj = bpy.data.objects.new(name, cam)
    scene.collection.objects.link(obj)
    obj.location = target - direction * distance
    obj.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
    scene.camera = obj
    scene.render.resolution_x, scene.render.resolution_y = width, height
    return obj


def render(path):
    scene = bpy.context.scene
    scene.render.filepath = path
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGBA"
    bpy.ops.render.render(write_still=True)
    return np.asarray(bpy.data.images.load(path).pixels[:], np.float32)


def to_tga(pixels, size, path):
    """Pixels as Blender keeps them (rows bottom up, rgba floats, sRGB as rendered)."""
    w, h = size
    img = bpy.data.images.new(os.path.basename(path), w, h)
    img.colorspace_settings.name = "Non-Color"
    pixels = np.array(pixels, np.float32).reshape(-1, 4)
    pixels[:, 3] = 1.0
    img.pixels.foreach_set(np.clip(pixels, 0, 1).ravel())
    img.filepath_raw = path
    img.file_format = "TARGA_RAW"
    img.save()
    bpy.data.images.remove(img)


def blur(mask, radius):
    """Separable gaussian."""
    x = np.arange(-3 * radius, 3 * radius + 1)
    kernel = np.exp(-x * x / (2 * radius * radius))
    kernel /= kernel.sum()
    mask = np.apply_along_axis(lambda row: np.convolve(row, kernel, mode="same"), 1, mask)
    return np.apply_along_axis(lambda col: np.convolve(col, kernel, mode="same"), 0, mask)


def light_probes():
    """The cube of the light round the field's middle, by day (the sun) and by night (the moon and the lanterns)."""
    for name, day in (("day", True), ("night", False)):
        build["light_rig"](day=day)
        build["lighting"].cube(None, FIELD_CENTER, os.path.join(OUT, f"probe_{name}"), build["DAY" if day else "NIGHT"])


def shadow_camera():
    """Orthographic along the moon light, its 16:9 box (2.8 / (tan(fov / 2) * 0.75) half wide, see
    GLDevice::setCamera) around the field from the floor to above the blocks."""
    direction = -MOON
    rotation = direction.to_track_quat("-Z", "Y")
    inverse = rotation.to_matrix().inverted()
    extent_x = extent_y = 0.0
    for x in (0, 13):
        for y in (0, -11):
            for z in (0.0, 1.6):
                local = inverse @ (Vector((x, y, z)) - FIELD_CENTER)
                extent_x = max(extent_x, abs(local.x))
                extent_y = max(extent_y, abs(local.y))
    half_x = max(extent_x, extent_y * 16 / 9) * 1.08
    fov = 2 * math.atan(2.8 / (0.75 * half_x))
    distance = 45.0
    position = FIELD_CENTER - direction * distance
    return direction, rotation, position, half_x, fov


def casters():
    """What stands high enough to shade the field: the mast with its sail, the rigging and the boom."""
    keep = []
    for obj in build["PROPS"]:
        if obj.name not in bpy.data.objects or obj.type != "MESH":
            continue
        corners = [obj.matrix_world @ Vector(c) for c in obj.bound_box]
        top = max(c.z for c in corners)
        bottom = min(c.z for c in corners)
        x = sum(c.x for c in corners) / 8
        if top > 3.0 and x < 15.0:
            keep.append(obj)
        elif bottom > 1.5 and x < 15.0:
            keep.append(obj)
    return keep


def shadow_cookie():
    direction, rotation, position, half_x, fov = shadow_camera()
    keep = set(casters())
    for obj in bpy.data.objects:
        if obj.type == "MESH":
            obj.hide_render = obj not in keep
    scene = bpy.context.scene
    scene.render.film_transparent = True
    # 16:9 squeezed into the square texture like the other levels' cookies
    camera = ortho_camera("shadow_view", direction, FIELD_CENTER, 2 * half_x, 512, 512, distance=45.0)
    scene.render.pixel_aspect_x, scene.render.pixel_aspect_y = 16.0 / 9.0, 1.0
    tmp = os.path.join(OUT, "tmp_cookie.png")
    cover = render(tmp).reshape(512, 512, 4)[..., 3]
    os.remove(tmp)
    mask = blur(cover, 1.6)
    color = 1.0 - mask[..., None] * (1.0 - np.array(SHADOW))[None, None, :]
    rgba = np.concatenate([color, np.ones((512, 512, 1))], axis=2)
    to_tga(rgba, (512, 512), os.path.join(OUT, "shadow-cookie.tga"))
    bpy.data.objects.remove(camera, do_unlink=True)

    # the engine's Matrix(Quat) is the transposed rotation of its quaternion: store the conjugate
    q = rotation
    cameras = {"Shadow Cam": {"fov": [[0, fov]], "position": [[0, *position]], "rotation": [[0, -q.x, -q.y, -q.z, q.w]]}}
    json.dump(cameras, open(os.path.join(OUT, "cameras.json"), "w"), indent=1)
    print("shadow cam", tuple(round(c, 2) for c in position), "fov", round(math.degrees(fov), 1), "half width",
          round(half_x, 2), "casters", len(keep))


os.makedirs(OUT, exist_ok=True)

setup()
light_probes()
shadow_cookie()
print("RESULT", OUT)
