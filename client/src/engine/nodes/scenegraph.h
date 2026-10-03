// scene graph

#pragma once

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include "materials/material.h"
#include "node.h"

class Stream;
class Camera;
class Mesh;

class MaterialFactory;

// owns every node of the scene in a flat node list (in creation order, bone ids index into it)
// and every material added to it
class SceneGraph : public Node
{
public:
   SceneGraph();
   ~SceneGraph() override;

   // load the scene, the factory creates the stored materials (none are created without one)
   int32_t load(const std::string& name, std::optional<std::reference_wrapper<const MaterialFactory>> materials = std::nullopt);
   void write(const std::string& name);

   int32_t size() const;  // get total number of nodes

   // adds the node to the scene as child of "parent" (the scene itself by default)
   template <std::derived_from<Node> T>
   T& addNode(std::unique_ptr<T> node, Node& parent);
   template <std::derived_from<Node> T>
   T& addNode(std::unique_ptr<T> node);

   // destroys the node together with all of its children
   void removeNode(Node& node);

   Node& getNode(int32_t index) const;  // find node by index

   // first node with the given name, empty if there is none or it is not a T
   template <std::derived_from<Node> T = Node>
   std::optional<std::reference_wrapper<T>> findNode(const std::string& name) const;

   void prepare();
   void render(float frame = 0.0f, const Matrix& shake = Matrix());  // recursively render all node of the scene graph
   void exportOBJ(float frame, Stream& stream, int32_t& vertex_num);
   Matrix setupCamera(const Matrix& shake = Matrix());
   void setCamera(Camera& camera);  // set active camera
   void clearCamera();
   std::optional<std::reference_wrapper<Camera>> getCamera() const;  // get active camera
   int32_t getLastFrame() const;                                     // get last frame of animation
   void setFlags(int32_t flags);

   Material& getMaterial(int32_t index) const;  // get material by number
   int32_t getMaterialCount() const;

   template <std::derived_from<Material> T>
   T& addMaterial(std::unique_ptr<T> material);

   // destroys the material
   void removeMaterial(const Material& material);

   const std::vector<std::unique_ptr<Node>>& nodeList() const;

   int32_t getMaterialStartIndex() const;
   int32_t getNodeStartIndex() const;

   const Matrix& getGlobalTransform() const;
   void setGlobalTransform(const Matrix& matrix);

   void exportOBJ(const std::string& name);

private:
   int32_t loadHeader(Stream& stream);  // load global information
   void writeHeader(Stream& stream);

   void loadMaterials(Stream& stream, std::optional<std::reference_wrapper<const MaterialFactory>> materials);  // load list of materials
   void writeMaterials(Stream& stream);

   void loadNode(Stream& stream, Node& parent);  // recursive stream-traversal (tree)
   void writeNode(Stream& stream, Node& parent);

   void linkMaterials(Mesh& mesh);  // add mesh to used materials
   void setMaterialStartIndex(int32_t index);
   void setNodeStartIndex(int32_t index);

   std::vector<std::unique_ptr<Node>> _nodes;              // all nodes to avoid tree traversal
   std::vector<std::unique_ptr<Material>> _materials;      // materials
   std::optional<std::reference_wrapper<Camera>> _camera;  // camera (currently active)
   int32_t _animation_begin = 0;
   int32_t _animation_end = 0;
   int32_t _flags = 0;

   int32_t _material_start_index = 0;
   int32_t _node_start_index = 0;
   Matrix _global_transform;
};

template <std::derived_from<Node> T>
T& SceneGraph::addNode(std::unique_ptr<T> node, Node& parent)
{
   T& added = *node;
   parent.linkChild(added);
   _nodes.push_back(std::move(node));
   return added;
}

template <std::derived_from<Node> T>
T& SceneGraph::addNode(std::unique_ptr<T> node)
{
   return addNode(std::move(node), *this);
}

template <std::derived_from<Node> T>
std::optional<std::reference_wrapper<T>> SceneGraph::findNode(const std::string& name) const
{
   const auto node = std::ranges::find_if(_nodes, [&name](const auto& candidate) { return candidate->name() == name; });
   if (node == _nodes.end())
   {
      return std::nullopt;
   }

   if constexpr (std::same_as<T, Node>)
   {
      return **node;
   }
   else
   {
      if (auto* typed = dynamic_cast<T*>(node->get()))
      {
         return *typed;
      }
      return std::nullopt;
   }
}

template <std::derived_from<Material> T>
T& SceneGraph::addMaterial(std::unique_ptr<T> material)
{
   T& added = *material;
   _materials.push_back(std::move(material));
   return added;
}
