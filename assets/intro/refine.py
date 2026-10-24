# Turns a toy character (toy.Builder's parts, each its own primitive stuck into the others, coloured from a palette)
# into a model as one would make it by hand: one closed surface fused from the parts, laid out again in evenly spread
# quads, unwrapped into islands that follow the colours, its colours, the painted details (eyes, mouths, cheeks: the
# parts added with paint=True) and its ambient occlusion baked into textures, skinned from the parts' bones.
import os
import sys

import bmesh
import bpy
import numpy as np
import openvdb
from mathutils.bvhtree import BVHTree

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
from uvs import select, unwrap  # noqa: E402


def copy(obj, name):
    dup = obj.copy()
    dup.data = obj.data.copy()
    dup.name = dup.data.name = name
    dup.parent = None
    dup.matrix_world = obj.matrix_world
    dup.modifiers.clear()
    bpy.context.scene.collection.objects.link(dup)
    return dup


def without_paint(obj, keep_paint):
    """Deletes the painted parts' faces (keep_paint=False) or everything else."""
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    layer = bm.faces.layers.int.get("paint")
    doomed = [f for f in bm.faces if (layer is not None and f[layer]) != keep_paint]
    bmesh.ops.delete(bm, geom=doomed, context="FACES")
    bm.to_mesh(obj.data)
    bm.free()


def smin(a, b, k):
    """The smooth minimum of two distances: their union with the crease between them rounded over k."""
    h = np.clip(0.5 + 0.5 * (b - a) / k, 0.0, 1.0)
    return b + (a - b) * h - k * h * (1.0 - h)


def blend(body, voxel, fillet):
    """The parts (body's loose pieces) blended into one surface the way a sculptor would join them: each part a
    distance field, unioned smoothly so one grows out of the other over about fillet instead of being stuck in."""
    bm = bmesh.new()
    bm.from_mesh(body.data)
    bmesh.ops.triangulate(bm, faces=[f for f in bm.faces if len(f.verts) > 4])
    coords = np.array([v.co for v in bm.verts], np.float32)
    lo = coords.min(axis=0) - fillet * 2 - voxel * 4
    n = np.ceil((coords.max(axis=0) + fillet * 2 + voxel * 4 - lo) / voxel).astype(int) + 1
    field = np.full(n, 1e3, np.float32)
    band = fillet / voxel + 3
    seen = set()
    for start in bm.verts:
        if start in seen:
            continue
        piece, stack = {start}, [start]
        seen.add(start)
        while stack:
            for e in stack.pop().link_edges:
                w = e.verts[1] if e.verts[0] in piece else e.verts[0]
                if w not in seen:
                    seen.add(w)
                    piece.add(w)
                    stack.append(w)
        faces = {f for v in piece for f in v.link_faces}
        index = {v: i for i, v in enumerate(piece)}
        points = ((np.array([v.co for v in piece], np.float32) - lo) / voxel).astype(np.float32)
        tris = np.array([[index[v] for v in f.verts] for f in faces if len(f.verts) == 3], np.uint32).reshape(-1, 3)
        quads = np.array([[index[v] for v in f.verts] for f in faces if len(f.verts) == 4], np.uint32).reshape(-1, 4)
        grid = openvdb.FloatGrid.createLevelSetFromPolygons(points, triangles=tris, quads=quads, halfWidth=band)
        part = np.empty(n, np.float32)
        grid.copyToArray(part)
        field = smin(field, part * voxel, fillet)
    bm.free()
    grid = openvdb.FloatGrid(background=float(band * voxel))
    grid.copyFromArray(field)
    points, quads = grid.convertToQuads(isovalue=0.0)
    mesh = bpy.data.meshes.new(body.data.name)
    mesh.from_pydata((np.asarray(points, float) * voxel + lo).tolist(), [], np.asarray(quads, int).tolist())
    mesh.validate()
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(mesh)
    bm.free()
    old = body.data
    body.data = mesh
    bpy.data.meshes.remove(old)


def fuse_and_retopo(body, voxel, faces, fillet):
    """The parts blended (see blend()), then laid out in about faces quads. The fused mesh's slivers (edges a
    millionth of its size) make QuadriFlow turn some down as not manifold; relaxing it a little spreads them but
    thins thin parts, so it's only the second try."""
    blend(body, voxel, fillet)
    hi = bpy.data.objects.new(body.name + "_hi", body.data.copy())
    bpy.context.scene.collection.objects.link(hi)
    select(body)
    for relax in (False, True):
        if relax:
            bm = bmesh.new()
            bm.from_mesh(body.data)
            for _ in range(3):
                bmesh.ops.smooth_vert(bm, verts=bm.verts, factor=0.5, use_axis_x=True, use_axis_y=True,
                                      use_axis_z=True)
            bm.to_mesh(body.data)
            bm.free()
        if any(bpy.ops.object.quadriflow_remesh(target_faces=faces, use_mesh_symmetry=symmetry, seed=0)
               == {"FINISHED"} for symmetry in (True, False)):
            break
    else:
        raise RuntimeError("QuadriFlow failed")
    # back onto the fused surface: QuadriFlow rounds off small shapes like curls and rims
    wrap = body.modifiers.new("wrap", "SHRINKWRAP")
    wrap.target = hi
    wrap.wrap_method = "NEAREST_SURFACEPOINT"
    bpy.ops.object.modifier_apply(modifier=wrap.name)
    bpy.data.objects.remove(hi)


def regions(low, parts, reach):
    """Per face of low the palette uv of the part the colour bake will see there (looking in along the normal from
    reach outside), or of the nearest part where that misses: the colour region the face belongs to."""
    tree = BVHTree.FromObject(parts, bpy.context.evaluated_depsgraph_get())
    # a "part" face attribute names the regions of models painted in gradients, else the palette swatch does
    named = parts.data.attributes.get("part")
    uv = parts.data.uv_layers.active.data if named is None else None
    result = []
    for poly in low.data.polygons:
        _, _, index, _ = tree.ray_cast(poly.center + poly.normal * reach, -poly.normal, reach * 2)
        if index is None:
            _, _, index, _ = tree.find_nearest(poly.center)
        part = parts.data.polygons[index]
        if named is not None:
            result.append(named.data[index].value)
        else:
            u, v = uv[part.loop_indices[0]].uv
            result.append((round(u, 3), round(v, 3)))
    return result


def buried(lows):
    """Per face of the shells (in order, as they will be joined) whether it lies inside another shell, out of sight."""
    trees = [BVHTree.FromObject(low, bpy.context.evaluated_depsgraph_get()) for low in lows]
    result = []
    for i, low in enumerate(lows):
        for poly in low.data.polygons:
            inside = False
            for j, tree in enumerate(trees):
                if j == i:
                    continue
                location, normal, _, _ = tree.find_nearest(poly.center)
                if location is not None and (poly.center - location).dot(normal) < 0:
                    inside = True
                    break
            result.append(inside)
    return result


def skin(low, parts):
    """The parts' bone weights onto the new surface, softened where the parts meet."""
    for group in parts.vertex_groups:
        low.vertex_groups.new(name=group.name)
    transfer = low.modifiers.new("weights", "DATA_TRANSFER")
    transfer.object = parts
    transfer.use_vert_data = True
    transfer.data_types_verts = {"VGROUP_WEIGHTS"}
    transfer.vert_mapping = "POLYINTERP_NEAREST"
    transfer.layers_vgroup_select_src = "ALL"
    transfer.layers_vgroup_select_dst = "NAME"
    select(low)
    bpy.ops.object.datalayout_transfer(modifier=transfer.name)
    bpy.ops.object.modifier_apply(modifier=transfer.name)
    bpy.ops.object.mode_set(mode="WEIGHT_PAINT")
    bpy.ops.object.vertex_group_smooth(group_select_mode="ALL", factor=0.5, repeat=4)
    bpy.ops.object.vertex_group_normalize_all(group_select_mode="ALL", lock_active=False)
    bpy.ops.object.mode_set(mode="OBJECT")


def bake(low, sources, name, size, reach):
    """The colours with the painted details from just above the surface, each shell (low's "piece" face attribute
    indexes sources) only from its own parts so the arm hidden in a dress doesn't come out pink; then the ambient
    occlusion of the whole."""
    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.device = "CPU"
    scene.render.bake.use_clear = True
    scene.world = bpy.data.worlds.new("bake")
    low.data.materials.clear()
    work = bpy.data.materials.new(name + "_bake")
    work.use_nodes = True
    target = work.node_tree.nodes.new("ShaderNodeTexImage")
    work.node_tree.nodes.active = target
    low.data.materials.append(work)

    images = {}
    for kind in ("color", "ao"):
        image = bpy.data.images.new(f"{name}_{kind}", size, size)
        if kind == "ao":
            image.colorspace_settings.name = "Non-Color"
        target.image = image
        if kind == "color":
            # a few samples smooth the borders between colours
            scene.cycles.samples = 16
            shells = [p.value for p in low.data.attributes["piece"].data]
            for index, source in enumerate(sources):
                piece = copy(low, f"{name}_bake_{index}")
                bm = bmesh.new()
                bm.from_mesh(piece.data)
                bm.faces.ensure_lookup_table()
                bmesh.ops.delete(bm, geom=[f for f in bm.faces if shells[f.index] != index], context="FACES")
                bm.to_mesh(piece.data)
                bm.free()
                select(piece)
                source.select_set(True)
                # the islands are packed a few pixels apart, a small margin keeps the shells off each other
                scene.render.bake.use_clear = index == 0
                bpy.ops.object.bake(type="DIFFUSE", pass_filter={"COLOR"}, margin=2, use_selected_to_active=True,
                                    cage_extrusion=reach, max_ray_distance=reach * 5 / 3)
                bpy.data.objects.remove(piece)
            scene.render.bake.use_clear = True
        else:
            # only the model itself occludes, not the parts it was made from
            hidden = [o for o in scene.objects if o != low and o.type == "MESH" and not o.hide_render]
            for o in hidden:
                o.hide_render = True
            select(low)
            scene.cycles.samples = 128
            scene.world.light_settings.distance = reach * 10 / 3
            bpy.ops.object.bake(type="AO", margin=6)
            for o in hidden:
                o.hide_render = False
        image.pack()
        images[kind] = image

    final = bpy.data.materials.new(name)
    final.use_nodes = True
    nodes, links = final.node_tree.nodes, final.node_tree.links
    bsdf = next(n for n in nodes if n.type == "BSDF_PRINCIPLED")
    bsdf.inputs["Roughness"].default_value = 0.55
    color = nodes.new("ShaderNodeTexImage")
    color.image = images["color"]
    links.new(color.outputs["Color"], bsdf.inputs["Base Color"])
    ao = nodes.new("ShaderNodeTexImage")
    ao.image = images["ao"]
    split = nodes.new("ShaderNodeSeparateColor")
    links.new(ao.outputs["Color"], split.inputs["Color"])
    group = bpy.data.node_groups.get("glTF Material Output") or bpy.data.node_groups.new(
        "glTF Material Output", "ShaderNodeTree")
    if "Occlusion" not in [s.name for s in group.interface.items_tree]:
        group.interface.new_socket("Occlusion", in_out="INPUT", socket_type="NodeSocketFloat")
    out = nodes.new("ShaderNodeGroup")
    out.node_tree = group
    links.new(split.outputs[0], out.inputs["Occlusion"])
    low.data.materials.clear()
    low.data.materials.append(final)


def limbs(bone):
    """The default shells: every arm and leg on its own, so it can move without dragging the body's surface along,
    the rest as one."""
    return bone if bone.startswith(("arm", "leg")) else "body"


def shell_pieces(solid, shell):
    """solid split into one object per shell: the part's own shell name if it has one (toy.Builder.add), else
    shell(bone of the face's first vertex)."""
    names = {g.index: g.name for g in solid.vertex_groups}
    own = list(solid.data.get("shells", [""]))
    tags = solid.data.attributes.get("shell")
    keys = []
    for poly in solid.data.polygons:
        tag = own[tags.data[poly.index].value] if tags else ""
        groups = solid.data.vertices[poly.vertices[0]].groups
        bone = names[max(groups, key=lambda g: g.weight).group] if len(groups) else ""
        keys.append(tag or shell(bone))
    pieces = {}
    for key in sorted(set(keys)):
        piece = copy(solid, f"{solid.name}_{key}")
        bm = bmesh.new()
        bm.from_mesh(piece.data)
        bm.faces.ensure_lookup_table()
        bmesh.ops.delete(bm, geom=[f for f in bm.faces if keys[f.index] != key], context="FACES")
        bm.to_mesh(piece.data)
        bm.free()
        pieces[key] = piece
    return pieces


def refine(rig, parts, faces=1500, texture=512, voxel=None, shell=limbs, keep=(), reach=None, density=None,
           fillet=None):
    """Replaces parts (bound to rig) by the refined model, which it returns. shell(bone) names the closed surface
    a bone's parts fuse into; faces are shared out by the shells' areas. Shells named in keep are taken as they
    are, for parts already laid out well that wouldn't survive fusing (thin sheets). reach: how far out painted
    details stand at most, a sixteenth of the model's size by default. density: {shell: factor} for shells with
    small shapes (curls, crevices) that need more faces than their area would give them. fillet: how far parts
    blend into each other, a fortieth of the model's size by default."""
    name = parts.name
    height = max(parts.dimensions)
    reach = reach or height * 0.06
    voxel = voxel or height / 180
    fillet = fillet or height / 40
    solid = copy(parts, name + "_solid")
    without_paint(solid, keep_paint=False)
    pieces = shell_pieces(solid, shell)
    bpy.data.objects.remove(solid)
    painted = copy(parts, name + "_painted")
    sources = shell_pieces(painted, shell)
    bpy.data.objects.remove(painted)
    # the faces go to the shells that are laid out again, by their areas
    density = density or {}
    areas = {key: sum(p.area for p in piece.data.polygons) * density.get(key, 1.0) for key, piece in pieces.items()
             if key not in keep}
    lows, region = [], []
    for index, (key, piece) in enumerate(pieces.items()):
        low = copy(piece, f"{name}_{key}_low")
        if key not in keep:
            fuse_and_retopo(low, voxel, max(120, round(faces * areas[key] / sum(areas.values()))), fillet)
        region += [(key, r) for r in regions(low, piece, reach)]
        skin(low, piece)
        bpy.data.objects.remove(piece)
        # the builder's tags are done with (a voxel remesh drops them, a kept shell still has them)
        for tag in ("paint", "part", "shell"):
            if tag in low.data.attributes:
                low.data.attributes.remove(low.data.attributes[tag])
        low.data.attributes.new("piece", "INT", "FACE").data.foreach_set("value", [index] * len(low.data.polygons))
        lows.append(low)
    hidden = buried(lows)
    bpy.ops.object.select_all(action="DESELECT")
    for low in lows:
        low.select_set(True)
    bpy.context.view_layer.objects.active = lows[0]
    bpy.ops.object.join()
    low = lows[0]
    for poly in low.data.polygons:
        poly.use_smooth = True
    islands = unwrap(low, region, hidden)
    bake(low, [sources[key] for key in pieces], name, texture, reach)
    for source in sources.values():
        bpy.data.objects.remove(source)
    low.data.attributes.remove(low.data.attributes["piece"])

    bpy.data.objects.remove(parts)
    low.name = low.data.name = name
    low.parent = rig
    modifier = low.modifiers.new("rig", "ARMATURE")
    modifier.object = rig
    stats = {"quads": sum(len(p.vertices) == 4 for p in low.data.polygons), "faces": len(low.data.polygons),
             "shells": len(lows), "islands": islands}
    print("RESULT refined", name, stats, flush=True)
    return low
