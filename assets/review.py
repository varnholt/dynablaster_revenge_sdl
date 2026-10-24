# Renders models side by side to judge them against each other (e.g. a new enemy beside the original player) and
# measures what makes a mesh look made by hand: an even spread of well shaped faces, no parts stuck into each
# other, textures with room for detail.
#   blender --background --factory-startup --python review.py -- <out.png> <model.glb> [<model.glb> ...]
# One row per model, in its rest pose: textured from the front, three quarters, the side and the back, then the
# wireframe from the front and three quarters. The measures are printed as RESULT lines:
#   tris            triangles
#   parts           loose pieces of the mesh (after welding the vertices uv seams split)
#   crossing        pairs of triangles that cut through each other, the mark of parts stuck into one another
#   edge_cv         how much edge lengths vary (standard deviation over mean)
#   slivers         share of triangles with an angle under 12 degrees
#   texture         the colour texture's size
#   texel_cv        how much the texture's resolution varies over the surface
#   uv_used         share of the texture the uvs cover
import math
import os
import sys

import bmesh
import bpy
import numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree

ARGS = sys.argv[sys.argv.index("--") + 1:]
OUT, MODELS = ARGS[0], ARGS[1:]
TILE = int(os.environ.get("REVIEW_TILE", 320))
BACKGROUND = (0.32, 0.34, 0.4)
VIEWS = [("front", (0, -1, 0.15), False), ("three_quarter", (0.8, -1, 0.45), False), ("side", (1, 0, 0.1), False),
         ("back", (0, 1, 0.2), False), ("front", (0, -1, 0.15), True), ("three_quarter", (0.8, -1, 0.45), True)]


def load(path):
    before = set(bpy.data.objects)
    bpy.ops.import_scene.gltf(filepath=path)
    objects = [o for o in bpy.data.objects if o not in before]
    # the importer's bone display shapes are meshes too, leave them out
    shapes = {b.custom_shape for o in objects if o.type == "ARMATURE" for b in o.pose.bones if b.custom_shape}
    for o in shapes:
        o.hide_render = True
    objects = [o for o in objects if o not in shapes]
    for o in objects:
        if o.type == "ARMATURE" and o.animation_data:
            o.animation_data.action = None
            for track in o.animation_data.nla_tracks:
                track.mute = True
        if o.type == "ARMATURE":
            for bone in o.pose.bones:
                bone.matrix_basis.identity()
    return objects


def welded(objects):
    """The meshes in their rest pose as one bmesh in world space, the vertices uv seams split welded again."""
    bm = bmesh.new()
    depsgraph = bpy.context.evaluated_depsgraph_get()
    for o in objects:
        if o.type != "MESH":
            continue
        mesh = o.evaluated_get(depsgraph).to_mesh()
        mesh.transform(o.matrix_world)
        bm.from_mesh(mesh)
        o.evaluated_get(depsgraph).to_mesh_clear()
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=1e-5)
    bmesh.ops.triangulate(bm, faces=bm.faces)
    return bm


def measure(objects):
    bm = welded(objects)
    bm.verts.ensure_lookup_table()
    bm.faces.ensure_lookup_table()
    result = {"tris": len(bm.faces)}

    seen, parts = set(), 0
    for v in bm.verts:
        if v in seen:
            continue
        parts += 1
        seen.add(v)
        stack = [v]
        while stack:
            current = stack.pop()
            for e in current.link_edges:
                w = e.other_vert(current)
                if w not in seen:
                    seen.add(w)
                    stack.append(w)
    result["parts"] = parts

    tree = BVHTree.FromBMesh(bm)
    crossing = 0
    for a, b in tree.overlap(tree):
        if a < b and not set(bm.faces[a].verts) & set(bm.faces[b].verts):
            crossing += 1
    result["crossing"] = crossing

    lengths = np.array([e.calc_length() for e in bm.edges])
    result["edge_cv"] = round(float(lengths.std() / lengths.mean()), 2)
    slivers = 0
    for f in bm.faces:
        a, b, c = (v.co for v in f.verts)
        angles = [(b - a).angle(c - a, 0), (a - b).angle(c - b, 0), (a - c).angle(b - c, 0)]
        slivers += min(angles) < math.radians(12)
    result["slivers"] = round(slivers / max(len(bm.faces), 1), 3)
    bm.free()

    # texture measures on the meshes as they are, uvs and all
    image, ratios, weights, uv_area = None, [], [], 0.0
    for o in objects:
        if o.type != "MESH" or not o.data.uv_layers:
            continue
        for slot in o.material_slots:
            if slot.material and slot.material.node_tree:
                for n in slot.material.node_tree.nodes:
                    if n.type == "TEX_IMAGE" and n.image and image is None:
                        image = n.image
        uv = o.data.uv_layers.active.data
        for poly in o.data.polygons:
            if len(poly.loop_indices) < 3:
                continue
            pts = [o.matrix_world @ o.data.vertices[o.data.loops[i].vertex_index].co for i in poly.loop_indices]
            uvs = [Vector((*uv[i].uv, 0)) for i in poly.loop_indices]
            area3 = sum(((pts[i] - pts[0]).cross(pts[i + 1] - pts[0])).length / 2 for i in range(1, len(pts) - 1))
            area2 = sum(((uvs[i] - uvs[0]).cross(uvs[i + 1] - uvs[0])).length / 2 for i in range(1, len(uvs) - 1))
            uv_area += area2
            if area3 > 1e-12:
                ratios.append(math.sqrt(area2 / area3))
                weights.append(area3)
    if image:
        result["texture"] = f"{image.size[0]}x{image.size[1]}"
    mean = 0.0
    if ratios:
        ratios, weights = np.array(ratios), np.array(weights)
        mean = np.average(ratios, weights=weights)
    # a palette's uvs are points, there is no resolution to measure
    if mean > 1e-9:
        result["texel_cv"] = round(float(math.sqrt(np.average((ratios - mean) ** 2, weights=weights)) / mean), 2)
        result["uv_used"] = round(min(uv_area, 1.0), 2)
    return result


def setup():
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.render.resolution_x = scene.render.resolution_y = TILE
    scene.display.shading.light = "STUDIO"
    scene.display.shading.show_cavity = False
    scene.display.shading.background_type = "VIEWPORT"
    scene.display.shading.background_color = (0.32, 0.34, 0.4)
    scene.render.film_transparent = False
    scene.view_settings.view_transform = "Standard"
    cam = bpy.data.objects.new("review_cam", bpy.data.cameras.new("review_cam"))
    cam.data.type = "ORTHO"
    scene.collection.objects.link(cam)
    scene.camera = cam
    return scene, cam


def render(scene, cam, objects, direction, wire, path):
    meshes = [o for o in objects if o.type == "MESH"]
    pts = [o.matrix_world @ Vector(c) for o in meshes for c in o.bound_box]
    lo = Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts)))
    hi = Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))
    centre = (lo + hi) / 2
    cam.data.ortho_scale = (hi - lo).length * 0.95
    d = Vector(direction).normalized()
    cam.location = centre + d * (hi - lo).length * 3
    cam.rotation_euler = (-d).to_track_quat("-Z", "Y").to_euler()
    cam.data.clip_end = (hi - lo).length * 10
    shading = scene.display.shading
    shading.color_type = "OBJECT" if wire else "TEXTURE"
    added = []
    if wire:
        for o in meshes:
            copy = o.copy()
            copy.data = o.data.copy()
            scene.collection.objects.link(copy)
            copy.modifiers.new("wire", "WIREFRAME").thickness = (hi - lo).length * 0.0016
            copy.color = (0.05, 0.05, 0.08, 1)
            o.color = (0.85, 0.85, 0.85, 1)
            added.append(copy)
    scene.render.filepath = path
    bpy.ops.render.render(write_still=True)
    for o in added:
        bpy.data.objects.remove(o)


def tile_sheet(rows, path):
    sheet = np.zeros((TILE * len(rows), TILE * len(VIEWS), 4), dtype=np.float32)
    for r, files in enumerate(rows):
        for c, f in enumerate(files):
            image = bpy.data.images.load(f)
            pixels = np.array(image.pixels[:], dtype=np.float32).reshape(TILE, TILE, 4)
            alpha = pixels[..., 3:]
            pixels[..., :3] = pixels[..., :3] * alpha + np.array(BACKGROUND) * (1 - alpha)
            pixels[..., 3] = 1
            y = (len(rows) - 1 - r) * TILE
            sheet[y:y + TILE, c * TILE:(c + 1) * TILE] = pixels
            bpy.data.images.remove(image)
            os.remove(f)
    out = bpy.data.images.new("sheet", TILE * len(VIEWS), TILE * len(rows), alpha=True)
    out.pixels = sheet.ravel()
    out.filepath_raw = path
    out.file_format = "PNG"
    out.save()


bpy.ops.wm.read_factory_settings(use_empty=True)
scene, cam = setup()
rows = []
for model in MODELS:
    objects = load(model)
    name = os.path.splitext(os.path.basename(model))[0]
    print("RESULT", name, " ".join(f"{k} {v}" for k, v in measure(objects).items()), flush=True)
    files = []
    for i, (_, direction, wire) in enumerate(VIEWS):
        f = os.path.join(os.path.dirname(os.path.abspath(OUT)), f"_tile_{name}_{i}.png")
        render(scene, cam, objects, direction, wire, f)
        files.append(f)
    rows.append(files)
    for o in objects:
        o.hide_render = True
tile_sheet(rows, OUT)
print("RESULT sheet", OUT)
