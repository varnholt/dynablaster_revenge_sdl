#include "scenegraph.h"
#include "camera.h"
#include "dummy.h"
#include "mesh.h"
#include "node.h"
#include "omni.h"
#include "shape.h"

#include "materials/material.h"
#include "materials/materialfactory.h"
#include "tools/datapaths.h"
#include "tools/stream.h"

#include "gldevice.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <format>
#include <fstream>
#include <memory>
#include <span>
#include <string>

namespace
{
constexpr int32_t header_chunk_id = 0x33424a48;
constexpr int32_t terminator_chunk_id = 0xffff;
}  // namespace

SceneGraph::SceneGraph() : Node(Node::idRoot)
{
}

// materials first, then nodes, each newest first
SceneGraph::~SceneGraph()
{
   while (!_materials.empty())
   {
      _materials.pop_back();
   }

   while (!_nodes.empty())
   {
      _nodes.pop_back();
   }
}

const Matrix& SceneGraph::getGlobalTransform() const
{
   return _global_transform;
}

void SceneGraph::setGlobalTransform(const Matrix& matrix)
{
   _global_transform = matrix;
}

int32_t SceneGraph::getMaterialStartIndex() const
{
   return _material_start_index;
}

void SceneGraph::setMaterialStartIndex(int32_t index)
{
   _material_start_index = index;
}

int32_t SceneGraph::getMaterialCount() const
{
   return static_cast<int32_t>(_materials.size());
}

Material& SceneGraph::getMaterial(int32_t index) const
{
   return *_materials[index];
}

int32_t SceneGraph::getNodeStartIndex() const
{
   return _node_start_index;
}

void SceneGraph::setNodeStartIndex(int32_t index)
{
   _node_start_index = index;
}

const std::vector<std::unique_ptr<Node>>& SceneGraph::nodeList() const
{
   return _nodes;
}

// get frame-number of end-of-animation
int32_t SceneGraph::getLastFrame() const
{
   return _animation_end;
}

// get number of nodes
int32_t SceneGraph::size() const
{
   return static_cast<int32_t>(_nodes.size());
}

void SceneGraph::removeNode(Node& node)
{
   // collect the subtree first, unlinking changes the child lists
   std::vector<std::reference_wrapper<const Node>> subtree = {node};
   for (size_t i = 0; i < subtree.size(); i++)
   {
      const Node& current = subtree[i];
      subtree.insert(subtree.end(), current._children.begin(), current._children.end());
   }

   if (node._parent)
   {
      node._parent->get().unlinkChild(node);
   }

   std::erase_if(
      _nodes,
      [&subtree](const std::unique_ptr<Node>& candidate)
      { return std::ranges::any_of(subtree, [&candidate](const Node& removed) { return &removed == candidate.get(); }); }
   );
}

// get node by index
Node& SceneGraph::getNode(int32_t index) const
{
   return *_nodes[index];
}

// get current camera node
std::optional<std::reference_wrapper<Camera>> SceneGraph::getCamera() const
{
   return _camera;
}

// set active camera
void SceneGraph::setCamera(Camera& camera)
{
   _camera = camera;
}

void SceneGraph::clearCamera()
{
   _camera.reset();
}

int32_t SceneGraph::loadHeader(Stream& stream)
{
   Chunk chunk(stream);
   if (chunk.id() != header_chunk_id)
   {
      return 0;
   }

   // comment is not used
   stream.getPrefixedString();
   _animation_begin = chunk.getInt();
   _animation_end = chunk.getInt();

   // node count is not used
   chunk.getInt();

   chunk.skip();

   return 1;
}

void SceneGraph::writeHeader(Stream& stream)
{
   Chunk chunk(stream, header_chunk_id, "Header");

   chunk.writePrefixedString("Hfr");
   chunk.writeInt(_animation_begin);
   chunk.writeInt(_animation_end);

   chunk.writeInt(size());
}

void SceneGraph::loadMaterials(Stream& stream, std::optional<std::reference_wrapper<const MaterialFactory>> materials)
{
   Chunk material_chunk(stream);

   const int32_t count = material_chunk.getInt();

   setMaterialStartIndex(getMaterialCount());

   if (materials)
   {
      for (int32_t i = 0; i < count; i++)
      {
         Stream& material_stream = material_chunk;
         Chunk chunk(material_stream);

         if (std::unique_ptr<Material> created = materials->get().createMaterial(chunk.id()))
         {
            Material& material = addMaterial(std::move(created));
            material.load(chunk);
            material.setName(chunk.name());
         }

         chunk.skip();  // skip to next chunk
      }
   }

   material_chunk.skip();
}

void SceneGraph::writeMaterials(Stream& stream)
{
   Chunk chunk(stream, 1000, "Materials");

   chunk.writeInt(getMaterialCount());

   for (const auto& material : _materials)
   {
      material->write(chunk);
   }
}

void SceneGraph::removeMaterial(const Material& material)
{
   std::erase_if(_materials, [&material](const std::unique_ptr<Material>& candidate) { return candidate.get() == &material; });
}

void SceneGraph::linkMaterials(Mesh& mesh)
{
   const int32_t count = mesh.getPartCount();
   for (int32_t i = 0; i < count; i++)
   {
      Geometry& geometry = mesh.getPart(i);
      const int32_t material_id = geometry.getMaterial();
      if (material_id >= 0 && material_id < getMaterialCount())
      {
         _materials[material_id]->add(geometry);
      }
   }
}

void SceneGraph::loadNode(Stream& stream, Node& parent)
{
   for (;;)
   {
      std::optional<std::reference_wrapper<Node>> loaded;

      // the chunk is destroyed before recursing deeper
      {
         Chunk chunk(stream);
         const int32_t id = chunk.id();

         // stop recursion when terminator chunk is found
         if (id == terminator_chunk_id)
         {
            break;
         }

         // create new node based on chunk id
         std::unique_ptr<Node> created;
         switch (id)
         {
            case Node::idMesh:
               created = std::make_unique<Mesh>();
               break;
            case Node::idDummy:
               created = std::make_unique<Dummy>();
               break;
            case Node::idShape:
               created = std::make_unique<Shape>();
               break;
            case Node::idCamera:
               created = std::make_unique<Camera>();
               break;
            case Node::idOmni:
               created = std::make_unique<Omni>();
               break;
            default:
               created = std::make_unique<Node>(Node::idNone);
               break;
         }

         Node& node = addNode(std::move(created), parent);
         loaded = node;

         node.setName(chunk.name());
         node.load(stream);

         if (!_camera && node.id() == Node::idCamera)
         {
            _camera = static_cast<Camera&>(node);
         }

         // link mesh to materials
         if (id == Node::idMesh)
         {
            auto& mesh = static_cast<Mesh&>(node);
            mesh.offsetIds(getMaterialStartIndex(), getNodeStartIndex());
            linkMaterials(mesh);
         }

         chunk.skip();  // skip to next chunk
      }

      loadNode(stream, *loaded);  // recursive load child nodes
   }
}

void SceneGraph::writeNode(Stream& stream, Node& parent)
{
   {
      Chunk chunk(stream, parent.id(), parent.name());
      parent.write(chunk);
   }

   for (int32_t i = 0; i < parent.getChildCount(); i++)
   {
      writeNode(stream, parent.getChild(i));
   }

   stream.writeInt(terminator_chunk_id);
}

int32_t SceneGraph::load(const std::string& name, std::optional<std::reference_wrapper<const MaterialFactory>> materials)
{
   std::ifstream file = DataPaths::open(name);
   Stream stream(file);

   if (!file.is_open())
   {
      return 0;
   }
   if (!loadHeader(stream))
   {
      return 0;
   }

   const int32_t count = size();

   setName(name);

   loadMaterials(stream, materials);

   setNodeStartIndex(size());
   loadNode(stream, *this);

   // find skeleton root nodes for all meshes
   for (int32_t i = count; i < size(); i++)
   {
      Node& node = *_nodes[i];
      if (node.id() == idMesh)
      {
         auto& mesh = static_cast<Mesh&>(node);

         int32_t min_depth = 0xffff;
         std::optional<std::reference_wrapper<Node>> skeleton;
         for (int32_t part = 0; part < mesh.getPartCount(); part++)
         {
            const Geometry& geometry = mesh.getPart(part);
            for (int32_t j = 0; j < geometry.getBoneCount(); j++)
            {
               const Bone& bone = geometry.getBone(j);
               Node& bone_node = *_nodes[bone.id()];
               const int32_t depth = bone_node.getDepth();

               if (depth < min_depth)
               {
                  min_depth = depth;
                  skeleton = bone_node;
               }
            }
         }

         if (skeleton)
         {
            while (skeleton->get().parent() && skeleton->get().parent()->get().id() == idDummy)
            {
               skeleton = skeleton->get().parent();
            }
            mesh.setSkeleton(*skeleton);
         }
      }
   }

   // initial transformation of all loaded nodes
   for (int32_t i = count; i < size(); i++)
   {
      _nodes[i]->transform(0.0f);
   }

   return 1;
}

void SceneGraph::write(const std::string& name)
{
   std::ofstream file(name, std::ios::binary);
   Stream stream(file);

   if (!file.is_open())
   {
      return;
   }

   writeHeader(stream);

   writeMaterials(stream);

   for (int32_t i = 0; i < getChildCount(); i++)
   {
      writeNode(stream, getChild(i));
   }

   stream.writeInt(terminator_chunk_id);
   file.close();
}

//! export scenegraph to obj
void SceneGraph::exportOBJ(const std::string& name)
{
   const std::unique_ptr<std::FILE, decltype(&std::fclose)> file(std::fopen(name.c_str(), "wb"), &std::fclose);
   if (!file)
   {
      return;
   }

   const auto print = [&file](const std::string& text) { std::fputs(text.c_str(), file.get()); };

   int32_t index_offset = 1;  // indices start with 1 (not 0)
   for (int32_t object = 0; object < getChildCount(); object++)
   {
      Node& node = getChild(object);
      if (node.id() == Node::idMesh)
      {
         const auto& mesh = static_cast<const Mesh&>(node);
         const std::string& node_name = node.name();
         const int32_t part_count = mesh.getPartCount();
         for (int32_t part = 0; part < part_count; part++)
         {
            const Geometry& geometry = mesh.getPart(part);
            const int32_t vertex_count = geometry.getVertexCount();
            const int32_t index_count = geometry.getIndexCount();

            const std::span<const Vector> vertices = geometry.getVertices();
            const std::span<const Vector> normals = geometry.getNormals();
            const std::span<const UV> texcoords = geometry.getUV(1);
            const std::span<const uint16_t> indices = geometry.getIndices();

            // write object info comment
            print(std::format("# object: {}-{}\n", node_name, part));
            print(std::format("# vertices: {}\n", vertex_count));
            print(std::format("# triangles: {}\n", index_count / 3));

            // write vertices
            print("\n");
            const Matrix& transform = geometry.getTransform();
            for (int32_t i = 0; i < vertex_count; i++)
            {
               const Vector v = transform * vertices[i];
               print(std::format("v {:.9f} {:.9f} {:.9f}\n", v.x, v.y, v.z));
            }

            // write normals
            print("\n");
            for (int32_t i = 0; i < vertex_count; i++)
            {
               const Vector& n = normals[i];
               print(std::format("vn {:.6f} {:.6f} {:.6f}\n", n.x, n.y, n.z));
            }

            // write uv channel
            print("\n");
            for (int32_t i = 0; i < vertex_count; i++)
            {
               if (!texcoords.empty())
               {
                  print(std::format("vt {:.6f} {:.6f} 0.0\n", texcoords[i].u, 1.0f - texcoords[i].v));
               }
               else
               {
                  print("vt 0.0 0.0 0.0\n");
               }
            }

            // write triangles - indices start with 1 (not 0)
            print("\n");
            print(std::format("g {}-{} \n", node_name, part));
            print("s off \n");
            for (int32_t i = 0; i < index_count; i += 3)
            {
               const int32_t i1 = indices[i];
               const int32_t i2 = indices[i + 1];
               const int32_t i3 = indices[i + 2];
               const bool valid = i1 >= 0 && i1 < vertex_count && i2 >= 0 && i2 < vertex_count && i3 >= 0 && i3 < vertex_count;

               if (valid)
               {
                  const int32_t f1 = i1 + index_offset;
                  const int32_t f2 = i2 + index_offset;
                  const int32_t f3 = i3 + index_offset;
                  print(std::format("f {0}/{0}/{0} {1}/{1}/{1} {2}/{2}/{2}\n", f1, f2, f3));
               }
            }
            print("\n\n");

            index_offset += vertex_count;
         }
      }
   }
}

void SceneGraph::exportOBJ(float /*frame*/, Stream& stream, int32_t& vertex_num)
{
   // render() needs to be called first!
   const int32_t count = getMaterialCount();
   for (int32_t i = 0; i < count; i++)
   {
      _materials[i]->exportOBJ(stream, vertex_num);
   }
}

Matrix SceneGraph::setupCamera(const Matrix& shake)
{
   Matrix camera;
   float fov = 1.0f;
   float z_near = 1.0f;
   float z_far = 500.0f;
   bool perspective = true;
   if (_camera)
   {
      const Camera& active = *_camera;
      const Matrix& object = active.getTransform();
      camera = object.invert();

      fov = active.getFOV();
      fov = std::tan(fov * 0.5f) * 9.0f / 16.0f;
      z_near = active.getNear();
      z_far = active.getFar();
      camera = _global_transform * camera * shake;
      perspective = active.getPerspectiveMode();
   }
   else
   {
      camera = _global_transform * shake;
   }
   activeDevice().setCamera(camera, fov, z_near, z_far, perspective);
   return camera;
}

// index loops with a fixed count: processing a material must not see materials registered meanwhile
void SceneGraph::prepare()
{
   const int32_t count = getMaterialCount();
   for (int32_t i = 0; i < count; i++)
   {
      _materials[i]->prepare();
   }
}

void SceneGraph::render(float frame, const Matrix& shake)
{
   // call "transform" all nodes
   for (int32_t i = 0; i < size(); i++)
   {
      _nodes[i]->transform(frame);
   }

   // get view matrix from camera node
   const Matrix camera = setupCamera(shake);

   // process all materials
   const int32_t count = getMaterialCount();
   for (int32_t i = 0; i < count; i++)
   {
      _materials[i]->update(frame, camera);
   }

   for (int32_t i = 0; i < count; i++)
   {
      _materials[i]->renderDiffuse();
   }
}

void SceneGraph::setFlags(int32_t flags)
{
   _flags = flags;
}
