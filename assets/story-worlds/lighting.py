# The story worlds' own light, as the original levels have theirs baked in: per world a sun and a sky (and the
# lights its props bring), baked into its dioramas as lightmaps (build_surroundings.py), and seen from the middle of
# the field as a cube of six views, which probes.py turns into the light probes the blocks, bombs and players are lit
# by. Runs inside Blender, from build_surroundings.py.
import math
import os

import bpy
import numpy as np
from mathutils import Matrix, Vector

from atmosphere import LIGHTING  # noqa: F401  (the worlds' suns and skies)

# the lightmaps hold a SCALE-th of the light, so light brighter than white keeps its colour
SCALE = 2.0
LIGHTMAP_TEX = 2048


# a glow's light: this many watts times its size squared (a torch's glow, 0.9, lights a white wall a unit off about
# fully), times the world's "glows"; fireflies (mode 6) only glow
GLOW_POWER = 120.0
GLOW_FIREFLY = 6


def setup(world):
    """The world's sun and sky in the scene, its props' glows as lights and, in a lava world, the lava glowing;
    returns what teardown() undoes."""
    light = LIGHTING[world]
    scene = bpy.context.scene
    added, restore = [], []
    if light["sun_strength"] > 0:
        sun_data = bpy.data.lights.new("sun", "SUN")
        sun_data.energy = light["sun_strength"]
        sun_data.color = light["sun_color"]
        # a little soft, the sun's disk
        sun_data.angle = math.radians(2.5)
        sun = bpy.data.objects.new("sun", sun_data)
        sun.rotation_euler = Vector(light["sun"]).normalized().to_track_quat("Z", "Y").to_euler()
        scene.collection.objects.link(sun)
        added.append(sun)
    world_data = scene.world
    world_data.use_nodes = True
    background = next(n for n in world_data.node_tree.nodes if n.type == "BACKGROUND")
    background.inputs[0].default_value = (*light["sky"], 1.0)
    background.inputs[1].default_value = light["sky_strength"]
    strength = light.get("glows", 1.0)
    for marker in [o for o in scene.objects if "glow" in o]:
        r, g, b, size, mode = list(marker["glow"])
        if mode == GLOW_FIREFLY or strength <= 0:
            continue
        data = bpy.data.lights.new("glow_light", "POINT")
        data.energy = GLOW_POWER * size * size * strength
        data.color = (r, g, b)
        data.shadow_soft_size = 0.05 + 0.1 * size
        lamp = bpy.data.objects.new("glow_light", data)
        lamp.location = marker.matrix_world.translation
        scene.collection.objects.link(lamp)
        added.append(lamp)
    # the lava lights the cave from below: its surface glowing while the light bakes (it's hidden for the bakes,
    # the game draws it with its own shader)
    if light.get("lava"):
        glowing = bpy.data.materials.new("lava_glow")
        glowing.use_nodes = True
        nodes = glowing.node_tree.nodes
        for node in list(nodes):
            nodes.remove(node)
        emission = nodes.new("ShaderNodeEmission")
        emission.inputs["Color"].default_value = (*light["lava_color"], 1.0)
        emission.inputs["Strength"].default_value = light["lava"]
        output = nodes.new("ShaderNodeOutputMaterial")
        glowing.node_tree.links.new(emission.outputs[0], output.inputs["Surface"])
        for obj in [o for o in scene.objects if "lava" in o]:
            restore.append((obj, obj.hide_render, [slot.material for slot in obj.material_slots]))
            obj.hide_render = False
            for slot in obj.material_slots:
                slot.material = glowing
    return added, restore


def teardown(state):
    added, restore = state
    for obj in added:
        light = obj.data
        bpy.data.objects.remove(obj, do_unlink=True)
        bpy.data.lights.remove(light)
    for obj, hidden, materials in restore:
        obj.hide_render = hidden
        for slot, material in zip(obj.material_slots, materials):
            slot.material = material
    background = next(n for n in bpy.context.scene.world.node_tree.nodes if n.type == "BACKGROUND")
    background.inputs[1].default_value = 0.0


def denoise(rgb, baked, radius=3, sigma=1.6, edge=0.08):
    """The bake's speckle smoothed out (a few small lamps leave it grainy): each texel the weighted mean of the
    baked texels round it, the weights falling with distance and with how far their light is from its own, so
    shadow edges stay sharp and no light crosses into unbaked texels between the islands."""
    total = np.zeros_like(rgb)
    weights = np.zeros(rgb.shape[:2], np.float32)
    level = rgb.mean(axis=-1)
    for dy in range(-radius, radius + 1):
        for dx in range(-radius, radius + 1):
            near = np.exp(-(dx * dx + dy * dy) / (2 * sigma * sigma))
            shifted = np.roll(np.roll(rgb, dy, axis=0), dx, axis=1)
            other = np.roll(np.roll(level, dy, axis=0), dx, axis=1)
            mask = np.roll(np.roll(baked, dy, axis=0), dx, axis=1)
            w = near * np.exp(-((other - level) / edge) ** 2) * mask
            total += shifted * w[..., None]
            weights += w
    smoothed = total / np.maximum(weights, 1e-6)[..., None]
    return np.where(baked[..., None], smoothed, rgb)


def bake(obj, name, bake_fn):
    """The light falling on obj (direct and bounced, no colour of its own), a SCALE-th of it, as an image."""
    img = bpy.data.images.new(name, LIGHTMAP_TEX, LIGHTMAP_TEX, float_buffer=True)
    img.colorspace_settings.name = "Non-Color"
    marker = np.tile(np.array([1.0, 0.0, 1.0, 1.0], np.float32), LIGHTMAP_TEX * LIGHTMAP_TEX)
    img.pixels.foreach_set(marker)
    bake_fn(obj, img)
    texels = np.empty(LIGHTMAP_TEX * LIGHTMAP_TEX * 4, np.float32)
    img.pixels.foreach_get(texels)
    texels = texels.reshape(-1, 4)
    unbaked = (texels[:, 0] > 0.99) & (texels[:, 1] < 0.01) & (texels[:, 2] > 0.99)
    texels[unbaked, :3] = np.median(texels[~unbaked, :3], axis=0)
    texels[:, :3] = denoise(texels[:, :3].reshape(LIGHTMAP_TEX, LIGHTMAP_TEX, 3), ~unbaked.reshape(LIGHTMAP_TEX, LIGHTMAP_TEX)).reshape(-1, 3)
    texels[:, :3] = np.clip(texels[:, :3] / SCALE, 0.0, 1.0)
    texels[:, 3] = 1.0
    img.pixels.foreach_set(texels.ravel())
    return img


def to_png(img, png):
    """img's pixels as a PNG (glb.png), rows top down; the light as it is, the game multiplies the colours by it
    as they are (as by its old key light)."""
    w, h = img.size
    pixels = np.empty(w * h * 4, np.float32)
    img.pixels.foreach_get(pixels)
    pixels = pixels.reshape(h, w, 4)[::-1]
    return png(np.ascontiguousarray(np.round(np.clip(pixels, 0, 1) * 255).astype(np.uint8)))


def embed(path, maps, gltf_io):
    """Adds the lightmaps ({material name: PNG bytes}) to the glb at path, each material's extras "lightmap":
    [image, SCALE] pointing at its own (GltfModel)."""
    gltf, binary = gltf_io.read(path)
    for name, content in maps.items():
        material = next((m for m in gltf.get("materials", []) if m.get("name") == name), None)
        if material is None:
            continue
        while len(binary) % 4:
            binary.append(0)
        gltf.setdefault("bufferViews", []).append({"buffer": 0, "byteOffset": len(binary), "byteLength": len(content)})
        binary.extend(content)
        gltf.setdefault("images", []).append({"bufferView": len(gltf["bufferViews"]) - 1, "mimeType": "image/png",
                                              "name": f"{name}_lightmap"})
        material.setdefault("extras", {})["lightmap"] = [len(gltf["images"]) - 1, SCALE]
    gltf_io.write(path, gltf, binary)


# the cube's faces: looking along, up
FACES = {
    "px": ((1, 0, 0), (0, 0, 1)), "nx": ((-1, 0, 0), (0, 0, 1)),
    "py": ((0, 1, 0), (0, 0, 1)), "ny": ((0, -1, 0), (0, 0, 1)),
    "pz": ((0, 0, 1), (0, 1, 0)), "nz": ((0, 0, -1), (0, -1, 0)),
}
CUBE = 64


def cube(world, position, folder):
    """Renders the light all round position into six faces, CUBE square, 90 degrees each: <folder>/probe_<face>.npy
    (rgb, linear, rows bottom up) and probe_<face>_basis.npy (right, up, forward as rows); the sun, which the views
    don't see, in sun.npy (towards it, its colour times its strength)."""
    scene = bpy.context.scene
    os.makedirs(folder, exist_ok=True)
    light = LIGHTING[world]
    np.save(os.path.join(folder, "sun.npy"), np.array([Vector(light["sun"]).normalized()[:],
                                                       [c * light["sun_strength"] for c in light["sun_color"]]]))
    cam = bpy.data.objects.new("probe", bpy.data.cameras.new("probe"))
    cam.data.angle = math.radians(90)
    cam.data.clip_start = 0.05
    cam.data.clip_end = 500
    scene.collection.objects.link(cam)
    scene.camera = cam
    cam.location = position
    render = scene.render
    saved = (render.resolution_x, render.resolution_y, render.resolution_percentage, render.filepath,
             render.image_settings.file_format, scene.cycles.samples, scene.view_settings.view_transform)
    render.resolution_x = render.resolution_y = CUBE
    render.resolution_percentage = 100
    render.image_settings.file_format = "OPEN_EXR"
    scene.cycles.samples = 64
    scene.view_settings.view_transform = "Standard"
    for name, (forward, up) in FACES.items():
        forward, up = Vector(forward), Vector(up)
        right = forward.cross(up)
        cam.matrix_world = Matrix((
            (right.x, up.x, -forward.x, position[0]),
            (right.y, up.y, -forward.y, position[1]),
            (right.z, up.z, -forward.z, position[2]),
            (0, 0, 0, 1)))
        path = os.path.join(folder, f"probe_{name}.exr")
        render.filepath = path
        bpy.ops.render.render(write_still=True)
        img = bpy.data.images.load(path)
        pixels = np.empty(CUBE * CUBE * 4, np.float32)
        img.pixels.foreach_get(pixels)
        bpy.data.images.remove(img)
        os.remove(path)
        np.save(os.path.join(folder, f"probe_{name}.npy"), pixels.reshape(CUBE, CUBE, 4)[..., :3])
        np.save(os.path.join(folder, f"probe_{name}_basis.npy"), np.array([right[:], up[:], forward[:]]))
    (render.resolution_x, render.resolution_y, render.resolution_percentage, render.filepath,
     render.image_settings.file_format, scene.cycles.samples, scene.view_settings.view_transform) = saved
    bpy.data.objects.remove(cam, do_unlink=True)


# the arena's shadow camera (level-castle's "Shadow Cam", hjb_cameras.py), which projects the shadow cookie onto
# the blocks, orthographic as BlockMaterial sets it up: 2.8 / (tan(fov / 2) * 0.75) wide either side, 9/16 that high
SHADOW_EYE = Vector((-5.92, -2.39, 6.54))
SHADOW_LOOK = Vector((0.862, -0.193, -0.469)).normalized()
SHADOW_UP = Vector((0.458, -0.102, 0.883)).normalized()
SHADOW_HALF = (2.8 / (math.tan(math.radians(25.0)) * 0.75), 2.8 / (math.tan(math.radians(25.0)) * 0.75) * 9 / 16)
COOKIE = 256


def cookie(world, folder, height=0.5):
    """How much of the world's sun reaches the blocks, as the shadow camera sees them (at height): its scenery's
    shadows on them, in place of the castle's towers. <folder>/cookie.npy, rows top down, 0 shadow to 1 sun."""
    scene = bpy.context.scene
    depsgraph = bpy.context.evaluated_depsgraph_get()
    os.makedirs(folder, exist_ok=True)
    if LIGHTING[world]["sun_strength"] <= 0:
        # no sun, no shadows from it: nothing of the castle's towers either
        np.save(os.path.join(folder, "cookie.npy"), np.ones((COOKIE, COOKIE), np.float32))
        return
    toward = Vector(LIGHTING[world]["sun"]).normalized()
    # a few rays across the sun's disk soften the edges
    side = toward.orthogonal().normalized()
    other = toward.cross(side)
    rays = [toward] + [(toward + (side * a + other * b) * 0.02).normalized() for a, b in ((1, 0), (-1, 0), (0, 1), (0, -1))]
    right = SHADOW_LOOK.cross(SHADOW_UP).normalized()
    up = right.cross(SHADOW_LOOK).normalized()
    lit = np.ones((COOKIE, COOKIE), np.float32)
    for row in range(COOKIE):
        sy = 1 - 2 * (row + 0.5) / COOKIE
        for column in range(COOKIE):
            sx = 2 * (column + 0.5) / COOKIE - 1
            start = SHADOW_EYE + right * (sx * SHADOW_HALF[0]) + up * (sy * SHADOW_HALF[1])
            if abs(SHADOW_LOOK.z) < 1e-6:
                continue
            point = start + SHADOW_LOOK * ((height - start.z) / SHADOW_LOOK.z)
            reached = 0
            for ray in rays:
                hit = scene.ray_cast(depsgraph, point + ray * 0.02, ray)[0]
                reached += 0 if hit else 1
            lit[row, column] = reached / len(rays)
    os.makedirs(folder, exist_ok=True)
    np.save(os.path.join(folder, "cookie.npy"), lit)
