# Signed-distance modelling: shapes are smooth-unioned into one closed surface (no intersections),
# colours come from the nearest shape, face details are projected on as paint.
import math
import os
import sys

import bmesh
import bpy
import numpy as np
import openvdb
from mathutils import Vector

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
from uvs import unwrap  # noqa: E402
import skins  # noqa: E402

# nothing thinner than this (in model units, before scale) survives as geometry
MIN_RADIUS = 0.06
VOXELS_ACROSS = 150


def hexcol(h):
    h = h.lstrip("#")
    return tuple((int(h[i:i + 2], 16) / 255) ** 2.2 for i in (0, 2, 4))


WHITE = hexcol("eeeeee")
BLACK = hexcol("111111")


def _r(value):
    if value < MIN_RADIUS:
        raise ValueError(f"radius {value} is below the minimum feature size {MIN_RADIUS}")
    return value


class Shape:
    def __init__(self, color, blend):
        self.color = color
        self.blend = blend

    def bounds(self):
        raise NotImplementedError

    def sdf(self, p):
        raise NotImplementedError


class Ellipsoid(Shape):
    def __init__(self, center, radii, color, blend=0.1):
        super().__init__(color, blend)
        self.c = np.array(center, float)
        self.r = np.array([_r(v) for v in radii], float)

    def bounds(self):
        return self.c - self.r, self.c + self.r

    def sdf(self, p):
        q = (p - self.c) / self.r
        k0 = np.linalg.norm(q, axis=1)
        k1 = np.linalg.norm(q / self.r, axis=1)
        return k0 * (k0 - 1.0) / np.maximum(k1, 1e-9)


class Capsule(Shape):
    """Round cone from a (radius ra) to b (radius rb)."""

    def __init__(self, a, b, ra, rb, color, blend=0.08):
        super().__init__(color, blend)
        self.a, self.b = np.array(a, float), np.array(b, float)
        self.ra, self.rb = _r(ra), _r(rb)

    def bounds(self):
        r = max(self.ra, self.rb)
        return np.minimum(self.a, self.b) - r, np.maximum(self.a, self.b) + r

    def sdf(self, p):
        # Inigo Quilez's round cone
        ba = self.b - self.a
        l2 = ba @ ba
        rr = self.ra - self.rb
        a2 = l2 - rr * rr
        il2 = 1.0 / l2
        pa = p - self.a
        y = pa @ ba
        z = y - l2
        xv = pa * l2 - np.outer(y, ba)
        x2 = np.einsum("ij,ij->i", xv, xv)
        y2 = y * y * l2
        z2 = z * z * l2
        k = np.sign(rr) * rr * rr * x2
        d = np.where(
            np.sign(z) * a2 * z2 > k, np.sqrt(x2 + z2) * il2 - self.rb,
            np.where(np.sign(y) * a2 * y2 < k, np.sqrt(x2 + y2) * il2 - self.ra,
                     (np.sqrt(x2 * a2 * il2) + y * rr) * il2 - self.ra))
        return d


class RoundBox(Shape):
    def __init__(self, center, half, rounding, color, blend=0.08, tilt_x=0.0):
        super().__init__(color, blend)
        self.c = np.array(center, float)
        self.h = np.array(half, float)
        self.round = _r(rounding)
        self.tilt = math.radians(tilt_x)

    def bounds(self):
        e = np.linalg.norm(self.h) + self.round
        return self.c - e, self.c + e

    def sdf(self, p):
        q = p - self.c
        if self.tilt:
            c, s = math.cos(self.tilt), math.sin(self.tilt)
            q = np.stack([q[:, 0], c * q[:, 1] + s * q[:, 2], -s * q[:, 1] + c * q[:, 2]], axis=1)
        d = np.abs(q) - (self.h - self.round)
        outside = np.linalg.norm(np.maximum(d, 0.0), axis=1)
        inside = np.minimum(np.max(d, axis=1), 0.0)
        return outside + inside - self.round


class Torus(Shape):
    """Ring lying in the XY plane (or XZ with upright=True)."""

    def __init__(self, center, major, minor, color, blend=0.06, upright=False):
        super().__init__(color, blend)
        self.c = np.array(center, float)
        self.R, self.r = major, _r(minor)
        self.upright = upright

    def bounds(self):
        e = np.array([self.R + self.r] * 3)
        return self.c - e, self.c + e

    def sdf(self, p):
        q = p - self.c
        if self.upright:
            q = q[:, [0, 2, 1]]
        ring = np.sqrt(q[:, 0] ** 2 + q[:, 1] ** 2) - self.R
        return np.sqrt(ring ** 2 + q[:, 2] ** 2) - self.r


class Cylinder(Shape):
    """Rounded upright cylinder."""

    def __init__(self, center, radius, half_height, rounding, color, blend=0.08, upright_y=False):
        super().__init__(color, blend)
        self.c = np.array(center, float)
        self.radius, self.hh, self.round = _r(radius), half_height, _r(rounding)
        self.upright_y = upright_y

    def bounds(self):
        e = np.array([max(self.radius, self.hh)] * 3) + self.round
        return self.c - e, self.c + e

    def sdf(self, p):
        q = p - self.c
        if self.upright_y:
            q = q[:, [0, 2, 1]]
        d = np.stack([np.sqrt(q[:, 0] ** 2 + q[:, 1] ** 2) - self.radius + self.round,
                      np.abs(q[:, 2]) - self.hh + self.round], axis=1)
        return np.minimum(np.max(d, axis=1), 0.0) + np.linalg.norm(np.maximum(d, 0.0), axis=1) - self.round


def smin(a, b, k):
    h = np.clip(0.5 + 0.5 * (b - a) / k, 0.0, 1.0)
    return b + (a - b) * h - k * h * (1.0 - h)


class Decal:
    """Paint projected along a direction onto the surfaces facing it: ellipse or rectangle in the
    plane across that direction, coordinates (u, v) = (x, z) for front/back, (y, z) for the sides."""

    def __init__(self, center, radii, color, facing=(0, -1, 0), rect=False):
        self.center = np.array(center, float)
        self.radii = np.array(radii, float)
        self.color = color
        self.facing = np.array(facing, float)
        self.rect = rect

    def mask(self, co, normals):
        f = self.facing
        axes = [0, 2] if abs(f[1]) > 0.5 else ([1, 2] if abs(f[0]) > 0.5 else [0, 1])
        uv = (co[:, axes] - self.center) / self.radii
        inside = np.max(np.abs(uv), axis=1) <= 1.0 if self.rect else np.sum(uv * uv, axis=1) <= 1.0
        return inside & (normals @ f > 0.25)


class Model:
    def __init__(self, name, scale=1.0, boss=False, target=None, motion="hop", skin="rubber", fur=None):
        """skin: the pattern over the shapes' colours (see skins.py), a name or (name, options); a shape can have
        its own. fur: (length in model units, strands per unit) grows real fur where the skin is "fur", drawn as
        shells by the game (gltf-vert.glsl)."""
        self.name = name
        self.skin = skin
        self.fur = fur
        # per texel of the colour texture how much fur grows there, set by paint_texture()
        self.fur_mask = None
        self.scale = scale
        self.boss = boss
        self.target = target or (2000 if boss else 1500)
        # body animation style for sdf_rig: hop, float, walk (has legs), spin, slither
        self.motion = motion
        self.shapes = []
        self.decals = []
        # bone name -> (parent, head, tail) in model units, "body" is implicit
        self.bones = {}

    def add(self, shape, bone="body", parent="body", head=None, tail=None, skin=None):
        """Shapes drive the skin weights of their bone; a capsule's bone runs from a to b by default."""
        self.shapes.append(shape)
        shape.bone = bone
        shape.skin = skin
        if bone != "body" and bone not in self.bones:
            if head is None:
                head = shape.a if isinstance(shape, Capsule) else shape.c
            if tail is None:
                tail = shape.b if isinstance(shape, Capsule) else np.array(head, float) + (0, 0, 0.2)
            self.bones[bone] = (parent, np.array(head, float), np.array(tail, float))
        return shape

    def paint(self, decal):
        self.decals.append(decal)
        return decal

    def eyes(self, x, z, eye=(0.17, 0.24), pupil=(0.08, 0.12), pupil_offset=(0.0, -0.03), white=WHITE, iris=BLACK, cx=0.0):
        for side in (-1, 1):
            self.paint(Decal((cx + side * x, z), eye, white))
            self.paint(Decal((cx + side * (x - pupil_offset[0]), z + pupil_offset[1]), pupil, iris))

    def field(self, p):
        d = self.shapes[0].sdf(p)
        for s in self.shapes[1:]:
            d = smin(d, s.sdf(p), s.blend)
        return d

    def nearest(self, p):
        """Per point the index of the shape it belongs to."""
        return np.argmin(np.stack([s.sdf(p) for s in self.shapes], axis=1), axis=1)

    def colors(self, p):
        palette = np.array([s.color for s in self.shapes])
        return palette[self.nearest(p)]

    def furry(self, p):
        """Per point whether its shape's skin is fur."""
        kinds = [shape.skin or self.skin for shape in self.shapes]
        names = np.array([(k if isinstance(k, str) else k[0]) == "fur" for k in kinds])
        return names[self.nearest(p)]

    def patterned(self, p, normals):
        """The colours at p with every shape's skin over them."""
        which = self.nearest(p)
        col = np.array([s.color for s in self.shapes])[which]
        for i, shape in enumerate(self.shapes):
            skin = shape.skin or self.skin
            if not skin:
                continue
            kind, options = (skin, {}) if isinstance(skin, str) else skin
            hit = which == i
            if hit.any():
                col[hit] = skins.apply(kind, col[hit], p[hit], normals[hit], **options)
        return col


def surface(model):
    """Extracts the zero level set as one closed quad mesh, in model units."""
    lo = np.min([s.bounds()[0] for s in model.shapes], axis=0) - 0.15
    hi = np.max([s.bounds()[1] for s in model.shapes], axis=0) + 0.15
    voxel = float(np.max(hi - lo)) / VOXELS_ACROSS
    n = np.ceil((hi - lo) / voxel).astype(int) + 1
    axes = [lo[i] + np.arange(n[i]) * voxel for i in range(3)]
    gx, gy, gz = np.meshgrid(*axes, indexing="ij")
    pts = np.stack([gx.ravel(), gy.ravel(), gz.ravel()], axis=1)
    values = model.field(pts).reshape(n).astype(np.float32)

    grid = openvdb.FloatGrid(background=float(values.max()))
    grid.copyFromArray(values)
    points, quads = grid.convertToQuads(isovalue=0.0)
    points = np.asarray(points, float) * voxel + lo
    return points, np.asarray(quads, int), voxel


def build_mesh(model, collection):
    points, quads, voxel = surface(model)
    mesh = bpy.data.meshes.new(f"{model.name}_hi")
    mesh.from_pydata(points.tolist(), [], quads.tolist())
    mesh.validate()
    obj = bpy.data.objects.new(f"{model.name}_hi", mesh)
    collection.objects.link(obj)

    # make normals point outwards
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(mesh)
    bm.free()
    for poly in mesh.polygons:
        poly.use_smooth = True

    co = np.array([v.co for v in mesh.vertices])
    normals = np.array([v.normal for v in mesh.vertices])
    col = model.colors(co)
    for decal in model.decals:
        col[decal.mask(co, normals)] = decal.color
    attr = mesh.color_attributes.new("Col", "FLOAT_COLOR", "POINT")
    rgba = np.hstack([col, np.ones((len(col), 1))]).astype(np.float32)
    attr.data.foreach_set("color", rgba.ravel())

    mat = bpy.data.materials.new(f"{model.name}_vcol")
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    bsdf = next(n for n in nodes if n.type == "BSDF_PRINCIPLED")
    node = nodes.new("ShaderNodeAttribute")
    node.attribute_name = "Col"
    mat.node_tree.links.new(node.outputs["Color"], bsdf.inputs["Base Color"])
    mesh.materials.append(mat)
    return obj


def islands(mesh):
    bm = bmesh.new()
    bm.from_mesh(mesh)
    seen, count = set(), 0
    for v in bm.verts:
        if v in seen:
            continue
        count += 1
        stack = [v]
        while stack:
            x = stack.pop()
            if x in seen:
                continue
            seen.add(x)
            stack.extend(e.other_vert(x) for e in x.link_edges)
    bm.free()
    return count


def _select(objects, active):
    bpy.ops.object.select_all(action="DESELECT")
    for o in objects:
        o.hide_set(False)
        o.select_set(True)
    bpy.context.view_layer.objects.active = active


def retopo(hi, target_tris, collection):
    """Even quads via QuadriFlow, retried on a few voxel resolutions, then on the same ones relaxed a little (the
    voxel mesh's slivers make QuadriFlow turn some down as not manifold; relaxing spreads them but thins thin
    limbs, so it comes second), before falling back to decimate."""
    name = hi.name.replace("_hi", "_SDF")
    for relax in (False, True):
        for divisions in (VOXELS_ACROSS, 120, 190, 100, 240):
            low = hi.copy()
            low.data = hi.data.copy()
            collection.objects.link(low)
            low.data.materials.clear()
            for attr in [a.name for a in low.data.color_attributes]:
                low.data.color_attributes.remove(low.data.color_attributes[attr])
            _select([low], low)
            remesh = low.modifiers.new("Remesh", "REMESH")
            remesh.mode = "VOXEL"
            remesh.voxel_size = max(low.dimensions) / divisions
            bpy.ops.object.modifier_apply(modifier=remesh.name)
            if relax:
                bm = bmesh.new()
                bm.from_mesh(low.data)
                for _ in range(3):
                    bmesh.ops.smooth_vert(bm, verts=bm.verts, factor=0.5, use_axis_x=True, use_axis_y=True,
                                          use_axis_z=True)
                bm.to_mesh(low.data)
                bm.free()
            before = len(low.data.polygons)
            try:
                bpy.ops.object.quadriflow_remesh(target_faces=target_tris // 2, use_mesh_symmetry=False,
                                                 smooth_normals=True)
            except RuntimeError:
                pass
            if len(low.data.polygons) != before:
                method = f"quadriflow@{divisions}" + (" relaxed" if relax else "")
                if relax:
                    # back onto the surface the relaxing rounded off a little
                    wrap = low.modifiers.new("wrap", "SHRINKWRAP")
                    wrap.target = hi
                    wrap.wrap_method = "NEAREST_SURFACEPOINT"
                    bpy.ops.object.modifier_apply(modifier=wrap.name)
                break
            bpy.data.objects.remove(low, do_unlink=True)
        else:
            continue
        break
    else:
        low = hi.copy()
        low.data = hi.data.copy()
        collection.objects.link(low)
        low.data.materials.clear()
        _select([low], low)
        dec = low.modifiers.new("Decimate", "DECIMATE")
        dec.ratio = target_tris / (len(low.data.polygons) * 2)
        bpy.ops.object.modifier_apply(modifier=dec.name)
        method = "decimate"
    low.name = name
    low.data.name = name
    for poly in low.data.polygons:
        poly.use_smooth = True
    return low, method


def _image(name, out_dir, size=512):
    key = f"sdf_{name}"
    if key in bpy.data.images:
        bpy.data.images.remove(bpy.data.images[key])
    img = bpy.data.images.new(key, size, size)
    img.filepath_raw = f"{out_dir}/{name}.png"
    img.file_format = "PNG"
    return img


def _target(mat, img):
    nodes = mat.node_tree.nodes
    for n in nodes:
        n.select = False
    node = next((n for n in nodes if n.type == "TEX_IMAGE" and n.image == img), None) or nodes.new("ShaderNodeTexImage")
    node.image = img
    node.select = True
    nodes.active = node


def _bake_geometry(low, kind, size):
    """Bakes world position or normal per texel into a float image, returns (pixels, covered)."""
    img = bpy.data.images.new(f"_bake_{kind}", size, size, alpha=True, float_buffer=True)
    img.colorspace_settings.name = "Non-Color"
    img.pixels.foreach_set(np.zeros(size * size * 4, np.float32))
    mat = bpy.data.materials.new(f"_bake_{kind}")
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    out = next(n for n in nodes if n.type == "OUTPUT_MATERIAL")
    geo = nodes.new("ShaderNodeNewGeometry")
    emit = nodes.new("ShaderNodeEmission")
    links.new(geo.outputs["Position" if kind == "position" else "Normal"], emit.inputs["Color"])
    links.new(emit.outputs["Emission"], out.inputs["Surface"])
    tex = nodes.new("ShaderNodeTexImage")
    tex.image = img
    nodes.active = tex
    saved = list(low.data.materials)
    low.data.materials.clear()
    low.data.materials.append(mat)
    _select([low], low)
    bpy.context.scene.render.bake.use_selected_to_active = False
    bpy.context.scene.render.bake.margin = 16
    bpy.ops.object.bake(type="EMIT")
    low.data.materials.clear()
    for m in saved:
        low.data.materials.append(m)
    px = np.empty(size * size * 4, np.float32)
    img.pixels.foreach_get(px)
    px = px.reshape(-1, 4)
    bpy.data.images.remove(img)
    bpy.data.materials.remove(mat)
    return px[:, :3], px[:, 3] > 0.5


def paint_texture(low, model, img, supersample=2):
    """Colours each texel from the shapes and projected decals at its 3D position: crisp, seam-free paint."""
    size = img.size[0] * supersample
    pos, covered = _bake_geometry(low, "position", size)
    nrm, _ = _bake_geometry(low, "normal", size)
    pos = pos / model.scale
    col = np.zeros((len(pos), 3))
    col[covered] = model.patterned(pos[covered], nrm[covered])
    fur = np.zeros(len(pos))
    if model.fur:
        fur[covered] = model.furry(pos[covered])
    for decal in model.decals:
        hit = covered.copy()
        hit[covered] = decal.mask(pos[covered], nrm[covered])
        col[hit] = decal.color
        # eyes and mouths stay bare
        fur[hit] = 0.0
    col[~covered] = col[covered].mean(axis=0)
    col = col.reshape(size, size, 3)
    col = col.reshape(img.size[0], supersample, img.size[0], supersample, 3).mean(axis=(1, 3))
    model.fur_mask = fur.reshape(img.size[0], supersample, img.size[0], supersample).mean(axis=(1, 3))
    rgba = np.concatenate([np.clip(col, 0, 1) ** (1 / 2.2), np.ones(col.shape[:2] + (1,))], axis=2)
    img.pixels.foreach_set(rgba.astype(np.float32).ravel())
    img.update()


def regions(low, model):
    """Per face the colour painted at its centre, shapes and decals: the seams go along the borders between them."""
    pos = np.array([low.matrix_world @ p.center for p in low.data.polygons]) / model.scale
    nrm = np.array([(low.matrix_world.to_3x3() @ p.normal).normalized() for p in low.data.polygons])
    col = model.colors(pos)
    for decal in model.decals:
        col[decal.mask(pos, nrm)] = decal.color
    return [tuple(np.round(c, 2)) for c in col]


def bake(low, hi, out_dir, name, model):
    os.makedirs(out_dir, exist_ok=True)
    unwrap(low, regions(low, model))

    color_img = _image(f"{name}_color", out_dir)
    ao_img = _image(f"{name}_ao", out_dir)
    mat = bpy.data.materials.new(low.name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = next(n for n in nodes if n.type == "BSDF_PRINCIPLED")
    bsdf.inputs["Roughness"].default_value = 0.5
    color = nodes.new("ShaderNodeTexImage")
    color.image = color_img
    ao = nodes.new("ShaderNodeTexImage")
    ao.image = ao_img
    ao_img.colorspace_settings.name = "Non-Color"
    mix = nodes.new("ShaderNodeMix")
    mix.data_type = "RGBA"
    mix.blend_type = "MULTIPLY"
    mix.inputs["Factor"].default_value = 1.0
    links.new(color.outputs["Color"], mix.inputs[6])
    links.new(ao.outputs["Color"], mix.inputs[7])
    links.new(mix.outputs[2], bsdf.inputs["Base Color"])
    low.data.materials.append(mat)

    scene = bpy.context.scene
    scene.render.engine = "CYCLES"
    scene.cycles.samples = 8
    size = max(low.dimensions)
    b = scene.render.bake
    b.margin = 8

    paint_texture(low, model, color_img)

    scene.cycles.samples = 64
    if scene.world:
        scene.world.light_settings.distance = size * 0.5
    others = [o for o in scene.objects if o != low and not o.hide_render]
    for o in others:
        o.hide_render = True
    _target(mat, ao_img)
    _select([low], low)
    bpy.ops.object.bake(type="AO", use_selected_to_active=False)
    for o in others:
        o.hide_render = False
    color_img.save()
    ao_img.save()


def _gltf_output_group():
    group = bpy.data.node_groups.get("glTF Material Output")
    if not group:
        group = bpy.data.node_groups.new("glTF Material Output", "ShaderNodeTree")
        group.interface.new_socket("Occlusion", in_out="INPUT", socket_type="NodeSocketFloat")
    return group


def export_glb(low, name, out_dir):
    """One .glb per model, centred on the ground, colour as base colour and AO as occlusion."""
    os.makedirs(out_dir, exist_ok=True)
    mesh = low.data.copy()
    co = np.array([v.co for v in mesh.vertices])
    lo, hi = co.min(axis=0), co.max(axis=0)
    shift = np.array([(lo[0] + hi[0]) / 2, (lo[1] + hi[1]) / 2, lo[2]])
    mesh.vertices.foreach_set("co", (co - shift).ravel())
    mesh.materials.clear()
    mat = bpy.data.materials.new(f"{name}_export")
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = next(n for n in nodes if n.type == "BSDF_PRINCIPLED")
    color = nodes.new("ShaderNodeTexImage")
    color.image = bpy.data.images[f"sdf_{name}_color"]
    links.new(color.outputs["Color"], bsdf.inputs["Base Color"])
    ao = nodes.new("ShaderNodeTexImage")
    ao.image = bpy.data.images[f"sdf_{name}_ao"]
    split = nodes.new("ShaderNodeSeparateColor")
    links.new(ao.outputs["Color"], split.inputs["Color"])
    out = nodes.new("ShaderNodeGroup")
    out.node_tree = _gltf_output_group()
    links.new(split.outputs[0], out.inputs["Occlusion"])
    mesh.materials.append(mat)
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.scene.collection.objects.link(obj)
    for scene in bpy.data.scenes:
        for layer in scene.view_layers:
            for o in scene.objects:
                o.select_set(False, view_layer=layer)
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    path = f"{out_dir}/{name}.glb"
    bpy.ops.export_scene.gltf(filepath=path, export_format="GLB", use_selection=True, export_yup=True)
    bpy.data.objects.remove(obj, do_unlink=True)
    bpy.data.meshes.remove(mesh)
    bpy.data.materials.remove(mat)
    return os.path.getsize(path)


def convert(model, collection, out_dir, offset):
    hi = build_mesh(model, collection)
    hi.scale = (model.scale,) * 3
    bpy.context.view_layer.objects.active = hi
    _select([hi], hi)
    bpy.ops.object.transform_apply(scale=True)
    low, method = retopo(hi, model.target, collection)
    bake(low, hi, out_dir, model.name.lower(), model)
    for o in (hi, low):
        o.location = offset
    hi.hide_set(True)
    hi.hide_render = True
    low.data.calc_loop_triangles()
    return low, {"tris": len(low.data.loop_triangles), "pieces": islands(low.data), "method": method,
                 "hi_pieces": islands(hi.data)}
