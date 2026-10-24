# Unwrapping a model the way one would by hand: seams along the borders between its colours (where a seam doesn't
# show), each colour region cut further only where it wouldn't unfold flat, and the cuts placed along the sides,
# out of the camera's view. Used by intro/refine.py and enemies/sdf_core.py.
import bmesh
import bpy
from mathutils import Vector


def select(obj):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj


def tidy(low, region, passes=3, smallest=3):
    """The regions' borders straightened (a face goes with most of its neighbours) and specks merged into the
    region around them, so the seams run cleanly and no island is a handful of faces."""
    bm = bmesh.new()
    bm.from_mesh(low.data)
    bm.faces.ensure_lookup_table()
    region = list(region)
    neighbours = [[next(g for g in e.link_faces if g != f) for e in f.edges if len(e.link_faces) == 2]
                  for f in bm.faces]
    for _ in range(passes):
        changed = list(region)
        for i, near in enumerate(neighbours):
            keys = [region[n.index] for n in near]
            best = max(set(keys), key=keys.count) if keys else region[i]
            if keys.count(best) >= 3 and best != region[i]:
                changed[i] = best
        region = changed
    seen = set()
    for f in bm.faces:
        if f.index in seen:
            continue
        component, stack = [f.index], [f.index]
        seen.add(f.index)
        while stack:
            for n in neighbours[stack.pop()]:
                if n.index not in seen and region[n.index] == region[f.index]:
                    seen.add(n.index)
                    component.append(n.index)
                    stack.append(n.index)
        if len(component) < smallest:
            around = [region[n.index] for i in component for n in neighbours[i] if region[n.index] != region[f.index]]
            if around:
                best = max(set(around), key=around.count)
                for i in component:
                    region[i] = best
    bm.free()
    return region


def components(low, region):
    """The connected pieces of each region, as lists of face indices."""
    bm = bmesh.new()
    bm.from_mesh(low.data)
    seen, result = set(), []
    for f in bm.faces:
        if f.index in seen:
            continue
        component, stack = [f.index], [f]
        seen.add(f.index)
        while stack:
            for e in stack.pop().edges:
                for g in e.link_faces:
                    if g.index not in seen and region[g.index] == region[f.index]:
                        seen.add(g.index)
                        component.append(g.index)
                        stack.append(g)
        result.append(component)
    bm.free()
    return result


def is_disc(low, component):
    """Whether the faces form a disc (one border, no holes, no handles: its euler characteristic is 1) open enough
    to unfold whole without cuts, like a face framed by hair."""
    faces = [low.data.polygons[i] for i in component]
    edges, verts = set(), set()
    for poly in faces:
        verts.update(poly.vertices)
        edges.update(tuple(sorted(key)) for key in poly.edge_keys)
    if len(verts) - len(edges) + len(faces) != 1:
        return False
    # and opens wide enough: a disc curled nearly shut (a head with only the neck open) unfolds badly squeezed.
    # The faces' normals, averaged by area, come to half their length on a hemisphere, less the more it closes
    total = sum((poly.normal * poly.area for poly in faces), Vector())
    return total.length >= 0.45 * sum(poly.area for poly in faces)


# the cuts tried in turn on a piece that doesn't unfold flat: front from back (the seam runs down the sides, out of
# the camera's view), then left from right, then top from bottom
CUTS = [(0, -1, 0), (1, 0, 0), (0, 0, 1)]


HIDDEN_SCALE = 0.25  # how much texture an island out of sight gets, relative to the visible ones


def unwrap(low, region, hidden=None):
    """Seams along the colour borders; a region that isn't a disc is cut into front and back (and further, see
    CUTS) until its pieces are, then every piece unfolded with as little stretch as it allows and all packed into
    one texture. Returns the number of islands."""
    # what's out of sight gets islands of its own, so it can have less texture without taking a face along
    if hidden:
        region = [(r, h) for r, h in zip(region, hidden)]
    label = [(r,) for r in tidy(low, region)]
    for axis in CUTS:
        axis = Vector(axis)
        for component in components(low, label):
            if is_disc(low, component):
                continue
            for i in component:
                poly = low.data.polygons[i]
                # by the normal, so a limb's tube splits along its length
                label[i] = label[i] + (poly.normal.dot(axis) >= 0,)
    bm = bmesh.new()
    bm.from_mesh(low.data)
    for e in bm.edges:
        if len(e.link_faces) == 2:
            a, b = e.link_faces
            e.seam = label[a.index] != label[b.index]
    bm.to_mesh(low.data)
    bm.free()
    if not low.data.uv_layers:
        low.data.uv_layers.new(name="UVMap")
    select(low)
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.select_all(action="SELECT")
    bpy.ops.uv.unwrap(method="ANGLE_BASED", margin=0.0)
    bpy.ops.uv.average_islands_scale()
    bpy.ops.object.mode_set(mode="OBJECT")
    # islands mostly out of sight shrunk, so the texture goes where it's seen
    pieces = components(low, label)
    bm = bmesh.new()
    bm.from_mesh(low.data)
    bm.faces.ensure_lookup_table()
    uv = bm.loops.layers.uv.active
    for piece in pieces:
        if not hidden or sum(hidden[i] for i in piece) * 2 < len(piece):
            continue
        loops = [loop for i in piece for loop in bm.faces[i].loops]
        middle = sum((loop[uv].uv for loop in loops), Vector((0, 0))) / len(loops)
        for loop in loops:
            loop[uv].uv = middle + (loop[uv].uv - middle) * HIDDEN_SCALE
    bm.to_mesh(low.data)
    bm.free()
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.select_all(action="SELECT")
    bpy.ops.uv.pack_islands(rotate=True, scale=True, margin=0.008)
    bpy.ops.object.mode_set(mode="OBJECT")
    return len(pieces)
