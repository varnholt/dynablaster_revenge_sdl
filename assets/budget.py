# Triangle budgets: meshes made by voxel remeshes, fine grids or subdivision carry far more triangles than their shape
# needs, most of them on flat or gently curved stretches. reduce() collapses a mesh down to a number of triangles per
# square unit of its surface; the collapse removes vertices where they change the shape least, so silhouettes, thin
# parts and creases keep theirs. Uvs and colour attributes are interpolated along. Runs inside Blender.
import bpy

# per square unit (a tile is one unit) of surface: props seen from the game's camera, a tile about 45 px across
PROPS = 40
# the ground: mostly flat or gently rolling, its relief comes from the baked colour and occlusion
GROUND = 3
# a mesh keeps at least this many
MINIMUM = 24


def triangles(obj):
    return sum(len(poly.vertices) - 2 for poly in obj.data.polygons)


def area(obj):
    """Surface area in world units, the object's scale included."""
    scale = obj.matrix_world.to_scale()
    factor = abs(scale.x * scale.y * scale.z) ** (2.0 / 3.0)
    return sum(poly.area for poly in obj.data.polygons) * factor


def reduce(obj, per_area=PROPS, minimum=MINIMUM, keep=("spin",)):
    """Collapses obj to per_area triangles per square unit of its surface, at least minimum. Meshes with a face
    attribute named in keep (the scenery's spinning parts) stay as they are: collapsing mixes faces across them.
    Returns the triangles before and after."""
    before = triangles(obj)
    if any(name in obj.data.attributes for name in keep):
        return before, before
    target = max(minimum, int(area(obj) * per_area))
    if before <= target * 1.15:
        return before, before
    collapse = obj.modifiers.new("Budget", "DECIMATE")
    collapse.decimate_type = "COLLAPSE"
    collapse.ratio = target / before
    collapse.use_collapse_triangulate = True
    active = bpy.context.view_layer.objects.active
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.modifier_apply(modifier=collapse.name)
    bpy.context.view_layer.objects.active = active
    return before, triangles(obj)
