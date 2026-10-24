# Brings the game's animated player (client/data/game/player-*.hjb, the engine's own skinned mesh format) into
# Blender: one armature of the 33 skinning bones under a "root", the "bomberman" mesh bound to it by the hjb weights,
# one action per hjb animation (idle, walk, run, win, die) and player_1.tga with player-ao.tga multiplied in as its
# colour. Saves a .blend and a .glb (every action as a glTF animation). Run headless:
#   blender -b --factory-startup -P import_hjb.py -- <out.blend> <out.glb>
# Units and axes are the engine's: z up, the player faces -y, about 0.58 high (the game scales it by 3).
#
# The format (client/src/engine/nodes/scenegraph.cpp, mesh.cpp, render/geometry.cpp, bone.cpp, animation/*):
# chunks of id, length prefixed name, size; a node's children follow its chunk, closed by a 0xffff chunk id.
# A geometry's bone holds the index of its node in load order, an inverse bind matrix and (vertex, weight) pairs; the
# game skins with world(node, t) * inverse_bind * vertex (MotionMixer::addAnimation), the mesh node's own transform is
# replaced by the player's placement. Times are 3ds max ticks (4800 a second), keyed every 80, so the actions run at
# 60 fps with one frame per key. Rotation keys and the scale keys' orientations are stored as deltas.
import os
import sys

import bpy
import numpy as np
from mathutils import Matrix

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "level-pirate"))
from hjb_cameras import Reader, quat_mul  # noqa: E402

ARGS = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
DATA = os.path.join(ROOT, "client", "data", "game")
OUT_BLEND = ARGS[0] if ARGS else os.path.join(ROOT, "assets", "intro", "out", "player.blend")
OUT_GLB = ARGS[1] if len(ARGS) > 1 else os.path.splitext(OUT_BLEND)[0] + ".glb"

# the game's MotionMixer order, the first one is the reference the player mesh is taken from
ANIMATIONS = [("idle", "player-idle.hjb"), ("walk", "player-walk.hjb"), ("run", "player-run.hjb"),
              ("die", "player-die1.hjb"), ("win", "player-win.hjb")]
COLOR_MAP = "player_1.tga"  # player_1..10 are the colour variants, PlayerMaterial picks one per player colour
AO_MAP = "player-ao.tga"
TICKS_PER_KEY = 80
FPS = 60

ID_MESH, ID_DUMMY, TERMINATOR = 1, 2, 0xFFFF
# the max rig steers the skinning bones through ik and fk helpers, these parents tie the loose ends to the pelvis
# and the toes to the feet. The feet stay children of the pelvis like the rig's ik feet: the legs squash and
# stretch, under a lower leg a foot would inherit that as shear, which neither blender bones nor glTF can hold.
PARENTS = {"CTRL_pelvis": "root", "spine1": "CTRL_pelvis", "belly": "CTRL_pelvis", "toe_l": "foot_l",
           "toe_r": "foot_r"}
# blender bones point along y: in these directions (z up, the player faces -y), else towards this child where
# there are several, else the only child, else on along the parent
DIRECTIONS = {"CTRL_pelvis": (0, 0, 1), "belly": (0, -1, 0), "foot_l": (0, -1, 0), "foot_r": (0, -1, 0),
              "toe_l": (0, -1, 0), "toe_r": (0, -1, 0)}
CHAIN = {"spine3": "CTRL_head", "hand_l": "finger1_l", "hand_r": "finger1_r"}


def keys(anim, width, deltas=False):
    _, _, track = anim.chunk()
    out = []
    for _ in range(track.int()):
        time = track.int()
        out.append((time, track.floats(width)))
    if deltas:
        # RotTrack::load, ScaleTrack::load: accumulate, staying in the hemisphere of the previous key
        for i in range(1, len(out)):
            prev = out[i - 1][1][:4]
            q = quat_mul(prev, out[i][1][:4])
            if sum(a * b for a, b in zip(prev, q)) < 0:
                q = tuple(-c for c in q)
            out[i] = (out[i][0], tuple(q) + tuple(out[i][1][4:]))
    return out


def tracks(sub):
    _, _, anim = sub.chunk()
    # position (xyz), rotation (quat xyzw), scale (orientation quat, xyz), then visibility and flip, unused
    return dict(position=keys(anim, 3), rotation=keys(anim, 4, True), scale=keys(anim, 7, True))


def words(reader, n):
    values = np.frombuffer(reader.data, "<u2", n, reader.pos)
    reader.pos += 2 * n
    return values


def geometry(reader):
    _, _, g = reader.chunk()
    part = dict(material=g.int())
    part["vertices"] = np.array(g.floats(3 * g.int())).reshape(-1, 3)
    part["normals"] = np.array(g.floats(3 * g.int())).reshape(-1, 3)
    g.floats(3 * g.int())  # vertex colours
    part["uvs"] = {}
    for _ in range(g.int()):
        channel = g.int()
        # u, v, w with v counted from the bottom like blender's (the engine flips it for its textures)
        part["uvs"][channel] = np.array(g.floats(3 * g.int())).reshape(-1, 3)[:, :2]
    part["indices"] = words(g, g.int()).astype(int)
    part["bones"] = []
    for _ in range(g.int()):
        node = g.int()
        inverse_bind = np.array(g.floats(16)).reshape(4, 4).T  # stored column by column
        weights = []
        for _ in range(g.int()):
            vertex = int(words(g, 1)[0])
            weights.append((vertex, g.float()))
        part["bones"].append((node, inverse_bind, weights))
    return part


def read_hjb(path):
    reader = Reader(open(path, "rb").read())
    _, _, header = reader.chunk()
    header.name()  # comment
    begin, end = header.int(), header.int()
    reader.chunk()  # materials, the player's is set up in code
    nodes = []

    def load(r, parent):
        while True:
            cid, name, sub = r.chunk()
            if cid == TERMINATOR:
                return
            node = dict(kind=cid, name=name, parent=parent)
            if cid == ID_MESH:
                sub.int()  # shadow flags
                node["parts"] = [geometry(sub) for _ in range(sub.int())]
            if cid in (ID_MESH, ID_DUMMY):
                node.update(tracks(sub))
            nodes.append(node)
            load(r, len(nodes) - 1)

    load(reader, None)
    last = max((t for n in nodes for k in ("position", "rotation", "scale") for t, _ in n.get(k, [])), default=0)
    return dict(begin=begin, end=end, last=last, nodes=nodes)


def quat_matrix(q):
    """The engine's Matrix(Quat) as a column vector 3x3: the transposed rotation of q."""
    x, y, z, w = q
    return np.array([[1 - 2 * (y * y + z * z), 2 * (x * y - w * z), 2 * (x * z + w * y)],
                     [2 * (x * y + w * z), 1 - 2 * (x * x + z * z), 2 * (y * z - w * x)],
                     [2 * (x * z - w * y), 2 * (y * z + w * x), 1 - 2 * (x * x + y * y)]]).T


def sample(track, time, mix):
    if not track:
        return None
    if time <= track[0][0]:
        return track[0][1]
    if time >= track[-1][0]:
        return track[-1][1]
    for (t0, a), (t1, b) in zip(track, track[1:]):
        if t0 <= time < t1:
            return mix(a, b, (time - t0) / (t1 - t0))


def lerp(a, b, f):
    return tuple(x + (y - x) * f for x, y in zip(a, b))


def slerp(a, b, f):
    a, b = np.array(a), np.array(b)
    d = float(np.clip(a @ b, -1, 1))
    if d < 0.9999999:
        th = np.arccos(d)
        return tuple((np.sin((1 - f) * th) * a + np.sin(f * th) * b) / np.sin(th))
    return tuple(a + (b - a) * f)


def local_matrix(node, time):
    """Node::transform: translation * rotation * scale, the scale along its own orientation (ScaleTrack::get)."""
    m = np.eye(4)
    m[:3, :3] = quat_matrix(sample(node["rotation"], time, slerp) or (0, 0, 0, 1))
    s = sample(node["scale"], time, lerp)
    if s:
        r = quat_matrix(s[:4])
        m[:3, :3] = m[:3, :3] @ r @ np.diag(s[4:]) @ np.linalg.inv(r)
    p = sample(node["position"], time, lerp)
    if p:
        m[:3, 3] = p
    return m


def world_matrices(scene, time):
    out = []
    for node in scene["nodes"]:
        m = local_matrix(node, time) if "rotation" in node else np.eye(4)
        if node["parent"] is not None:
            m = out[node["parent"]] @ m
        out.append(m)
    return out


def skin_matrices(scene, time):
    """{bone name: world * inverse bind}, what the game skins every vertex with."""
    world = world_matrices(scene, time)
    names = [n["name"] for n in scene["nodes"]]
    mesh = next(n for n in scene["nodes"] if n["kind"] == ID_MESH)
    return {names[b]: world[b] @ inverse_bind for b, inverse_bind, _ in mesh["parts"][0]["bones"]}


def bone_parents(scene, bone_names):
    """The nearest skinning ancestor in the hjb tree, unless PARENTS says otherwise."""
    nodes = scene["nodes"]
    index = {n["name"]: i for i, n in enumerate(nodes)}
    parents = {}
    for name in bone_names:
        if name in PARENTS:
            parents[name] = PARENTS[name]
            continue
        p = nodes[index[name]]["parent"]
        while p is not None and nodes[p]["name"] not in bone_names:
            p = nodes[p]["parent"]
        parents[name] = nodes[p]["name"] if p is not None else "root"
    return parents


def rigid(m):
    """Drop the scale (the rig's 1/2.54) from a matrix, keeping rotation and translation."""
    u, _, vt = np.linalg.svd(m[:3, :3])
    out = np.eye(4)
    out[:3, :3] = u @ vt
    out[:3, 3] = m[:3, 3]
    return out


def along_y(bind, target):
    """The swap of the bone's own axes that turns the one closest to target into blender's bone axis, y."""
    dots = target @ bind[:3, :3]
    k = int(np.argmax(np.abs(dots)))
    y = np.zeros(3)
    y[k] = np.sign(dots[k])
    x = np.zeros(3)
    x[(k + 1) % 3] = 1
    swap = Matrix.Identity(4)
    for row in range(3):
        swap[row][0], swap[row][1], swap[row][2] = x[row], y[row], np.cross(x, y)[row]
    return swap


def build_mesh(part, bone_names):
    vertices = part["vertices"]
    # the hjb has a few doubled triangles, blender would drop them as invalid anyway
    faces, seen = [], set()
    for face in part["indices"].reshape(-1, 3).tolist():
        key = tuple(sorted(face))
        if len(set(face)) == 3 and key not in seen:
            seen.add(key)
            faces.append(face)
    mesh = bpy.data.meshes.new("bomberman")
    mesh.from_pydata(vertices.tolist(), [], faces)
    uv = part["uvs"][min(part["uvs"])]  # the game draws with channel 1, the only one
    layer = mesh.uv_layers.new(name="UVMap")
    loop_vertices = np.zeros(len(mesh.loops), int)
    mesh.loops.foreach_get("vertex_index", loop_vertices)
    layer.data.foreach_set("uv", uv[loop_vertices].ravel())
    mesh.polygons.foreach_set("use_smooth", [True] * len(mesh.polygons))
    mesh.update()
    print(f"mesh: {len(vertices)} vertices, {len(faces)} triangles ({len(part['indices']) // 3 - len(faces)} doubled "
          "or degenerate dropped), valid", not mesh.validate())
    mesh.normals_split_custom_set_from_vertices(part["normals"].tolist())
    obj = bpy.data.objects.new("bomberman", mesh)
    bpy.context.scene.collection.objects.link(obj)
    for name in bone_names:
        obj.vertex_groups.new(name=name)
    return obj


def build_armature(reference, part):
    names = [n["name"] for n in reference["nodes"]]
    inverse_binds = {names[b]: inverse_bind for b, inverse_bind, _ in part["bones"]}
    bone_names = list(inverse_binds)
    parents = bone_parents(reference, set(bone_names))
    # rest: the bind pose, i.e. the inverse of the inverse bind, scale dropped, its axes swapped round so that y
    # points along the bone. Any rigid rest works, the actions are baked against it.
    binds = {name: rigid(np.linalg.inv(m)) for name, m in inverse_binds.items()}
    rest = {"root": Matrix(((1, 0, 0, 0), (0, 0, -1, 0), (0, 1, 0, 0), (0, 0, 0, 1)))}  # y along z
    pending = ["root"]
    while pending:
        parent = pending.pop(0)
        for name in [c for c in bone_names if parents[c] == parent]:
            children = [c for c in bone_names if parents[c] == name]
            head = binds[name][:3, 3]
            if name in DIRECTIONS:
                target = np.array(DIRECTIONS[name], float)
            elif name in CHAIN or len(children) == 1:
                target = binds[CHAIN.get(name, children[0])][:3, 3] - head
            else:
                target = np.array(rest[parent].to_3x3().col[1])
            rest[name] = Matrix(binds[name].tolist()) @ along_y(binds[name], target)
            pending.append(name)

    data = bpy.data.armatures.new("bomberman_rig")
    rig = bpy.data.objects.new("bomberman_rig", data)
    bpy.context.scene.collection.objects.link(rig)
    bpy.context.view_layer.objects.active = rig
    bpy.ops.object.mode_set(mode="EDIT")
    order = ["root"] + bone_names
    for name in order:
        bone = data.edit_bones.new(name)
        children = [rest[c].translation for c, p in parents.items() if p == name and CHAIN.get(name, c) == c]
        axis = rest[name].to_3x3().col[1]
        reach = max([(c - rest[name].translation).dot(axis) for c in children], default=0.0)
        bone.length = reach if reach > 0.01 else (0.1 if name == "root" else 0.03)
        bone.matrix = rest[name]
    for name in bone_names:
        data.edit_bones[name].parent = data.edit_bones[parents[name]]
    bpy.ops.object.mode_set(mode="OBJECT")
    for name in order:
        data.bones[name].use_deform = name != "root"
    return rig, rest, parents, ["root"] + bone_names


def fcurves(act):
    # blender 5 actions keep their curves in layered channelbags
    if hasattr(act, "fcurves") and len(act.fcurves):
        return list(act.fcurves)
    curves = []
    for layer in getattr(act, "layers", []):
        for strip in layer.strips:
            for bag in strip.channelbags:
                curves.extend(bag.fcurves)
    return curves


def build_action(rig, name, scene, rest, parents, order):
    """Bakes the game's skinning matrices at every key into loc/rot/scale of the blender bones."""
    times = range(0, scene["last"] + 1, TICKS_PER_KEY)
    values = {b: [] for b in order}
    for time in times:
        skins = skin_matrices(scene, time)
        pose = {"root": rest["root"]}
        for bone in order[1:]:
            pose[bone] = Matrix(skins[bone].tolist()) @ rest[bone]  # armature space, the rig sits at the origin
        for bone in order:
            parent = parents.get(bone)
            if parent:
                basis = (rest[parent].inverted() @ rest[bone]).inverted() @ pose[parent].inverted() @ pose[bone]
            else:
                basis = rest[bone].inverted() @ pose[bone]
            loc, rot, scale = basis.decompose()
            if values[bone] and values[bone][-1][1].dot(rot) < 0:
                rot.negate()
            values[bone].append((loc, rot, scale))

    act = bpy.data.actions.new(name)
    act.use_fake_user = True
    rig.animation_data_create()
    rig.animation_data.action = act
    for bone in order:
        pose_bone = rig.pose.bones[bone]
        pose_bone.rotation_mode = "QUATERNION"
        for path in ("location", "rotation_quaternion", "scale"):
            pose_bone.keyframe_insert(path, frame=0)
    curves = {(fc.data_path, fc.array_index): fc for fc in fcurves(act)}
    frames = np.arange(len(times), dtype=float)  # frame 0 at tick 0, so the glTF times start at 0
    for bone in order:
        for channel, path in enumerate(("location", "rotation_quaternion", "scale")):
            track = np.array([[*v[channel]] for v in values[bone]])
            for i in range(track.shape[1]):
                fc = curves[(f'pose.bones["{bone}"].{path}', i)]
                fc.keyframe_points.add(len(times) - 1)
                fc.keyframe_points.foreach_set("co", np.c_[frames, track[:, i]].ravel())
                fc.keyframe_points.foreach_set("interpolation", [bpy.types.Keyframe.bl_rna.properties[
                    "interpolation"].enum_items["LINEAR"].value] * len(times))
                fc.update()
    act.frame_range = (0, len(times) - 1)
    act.use_frame_range = True
    for pose_bone in rig.pose.bones:
        pose_bone.matrix_basis = Matrix()
    track = rig.animation_data.nla_tracks.new()
    track.name = name
    track.strips.new(name, 0, act)
    track.mute = True
    rig.animation_data.action = None
    return act, len(times)


def color_image(out_dir):
    """PlayerMaterial: colour map times the ao map's red, the alpha (a specular mask) left out for the glTF."""
    color = bpy.data.images.load(os.path.join(DATA, COLOR_MAP))
    ao = bpy.data.images.load(os.path.join(DATA, AO_MAP))
    w, h = color.size
    if tuple(ao.size) != (w, h):
        ao.scale(w, h)
    c = np.empty(w * h * 4, np.float32)
    a = np.empty(w * h * 4, np.float32)
    color.pixels.foreach_get(c)
    ao.pixels.foreach_get(a)
    c = c.reshape(-1, 4)
    c[:, :3] *= a.reshape(-1, 4)[:, :1]
    c[:, 3] = 1.0
    image = bpy.data.images.new("player_1_ao", w, h, alpha=False)
    image.pixels.foreach_set(c.ravel())
    image.filepath_raw = os.path.join(out_dir, "player_1_ao.png")
    image.file_format = "PNG"
    image.save()
    image.pack()
    for unused in (color, ao):
        bpy.data.images.remove(unused)
    return image


def build_material(image):
    mat = bpy.data.materials.new("player_1")
    if mat.node_tree is None:
        mat.use_nodes = True
    nodes = mat.node_tree.nodes
    bsdf = next(n for n in nodes if n.type == "BSDF_PRINCIPLED")
    tex = nodes.new("ShaderNodeTexImage")
    tex.image = image
    tex.location = (-400, 200)
    mat.node_tree.links.new(tex.outputs["Color"], bsdf.inputs["Base Color"])
    bsdf.inputs["Roughness"].default_value = 0.45
    return mat


def main():
    for leftover in list(bpy.data.objects):
        bpy.data.objects.remove(leftover)
    out_dir = os.path.dirname(os.path.abspath(OUT_BLEND))
    os.makedirs(out_dir, exist_ok=True)
    scenes = {name: read_hjb(os.path.join(DATA, file)) for name, file in ANIMATIONS}
    reference = scenes[ANIMATIONS[0][0]]
    mesh_node = next(n for n in reference["nodes"] if n["kind"] == ID_MESH)
    part = mesh_node["parts"][0]

    rig, rest, parents, order = build_armature(reference, part)
    obj = build_mesh(part, order[1:])
    names = [n["name"] for n in reference["nodes"]]
    for b, _, weights in part["bones"]:
        group = obj.vertex_groups[names[b]]
        for vertex, weight in weights:
            group.add([vertex], weight, "REPLACE")
    obj.parent = rig
    modifier = obj.modifiers.new("Armature", "ARMATURE")
    modifier.object = rig
    obj.data.materials.append(build_material(color_image(out_dir)))

    scene = bpy.context.scene
    scene.render.fps = FPS
    print("bones (parent):")
    for bone in order:
        print(f"   {bone} ({parents.get(bone, '-')})")
    for name, _ in ANIMATIONS:
        _, frames = build_action(rig, name, scenes[name], rest, parents, order)
        print(f"action {name}: frames 0..{frames - 1} at {FPS} fps, {(frames - 1) / FPS:.3f} s")
    scene.frame_start, scene.frame_end = 0, int(max(a.frame_range[1] for a in bpy.data.actions))

    size = part["vertices"].max(0) - part["vertices"].min(0)
    print("bind pose bounding box size", np.round(size, 4), "min", np.round(part["vertices"].min(0), 4))
    bpy.ops.wm.save_as_mainfile(filepath=os.path.abspath(OUT_BLEND))
    print("saved", OUT_BLEND)

    bpy.ops.object.select_all(action="DESELECT")
    rig.select_set(True)
    obj.select_set(True)
    bpy.context.view_layer.objects.active = rig
    bpy.ops.export_scene.gltf(filepath=os.path.abspath(OUT_GLB), export_format="GLB", use_selection=True,
                              export_yup=True, export_animations=True, export_animation_mode="NLA_TRACKS")
    print("exported", OUT_GLB)


main()
