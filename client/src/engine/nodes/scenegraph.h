// scene graph

#pragma once

#include <cstdint>
#include <vector>
#include "node.h"
#include "tools/array.h"

class Stream;
class Material;
class Camera;
class Mesh;

class MaterialFactory;

// owns every node registered in its flat node list and every material registered with it;
// both are deleted in the destructor (nodes and external owners may also delete them earlier,
// nodes unregister themselves on destruction)
class SceneGraph : public Node
{
public:
   SceneGraph();
   ~SceneGraph() override;

   static SceneGraph* instance();

   int32_t load(const String& name, MaterialFactory* materials = nullptr, Node* parent = nullptr);  // load the scene
   void write(const String& name);

   int32_t size();  // get total number of nodes
   void addNode(Node* node);
   void removeNode(Node* node);
   Node* getNode(int32_t index);       // find node by index
   Node* getNode(const String& name);  // find node by name

   void prepare();
   void render(float frame = 0.0f, const Matrix& shake = Matrix());  // recursively render all node of the scene graph
   void exportOBJ(float frame, Stream* stream, int32_t& vertex_num);
   Matrix setupCamera(const Matrix& shake = Matrix());
   void setCamera(Node* node);    // set active camera
   Node* getCamera();             // get active camera
   int32_t getLastFrame() const;  // get last frame of animation
   void setFlags(int32_t flags);

   Material* getMaterial(int32_t index) const;  // get material by number
   int32_t getMaterialCount() const;
   void addMaterial(Material* material);
   void removeMaterial(Material* material);
   const Array<Node*>& nodeList() const;

   int32_t getMaterialStartIndex() const;
   int32_t getNodeStartIndex() const;

   const Matrix& getGlobalTransform() const;
   void setGlobalTransform(const Matrix& matrix);

   void exportOBJ(const String& name);

private:
   int32_t loadHeader(Stream* stream);  // load global information
   void writeHeader(Stream* stream);

   void loadMaterials(Stream* stream, MaterialFactory* materials);  // load list of materials
   void writeMaterials(Stream* stream);

   void loadNode(Stream* stream, Node* parent);  // recursive stream-traversal (tree)
   void writeNode(Stream* stream, Node* parent);

   void linkMaterials(Mesh* mesh);  // add mesh to used materials
   void setMaterialStartIndex(int32_t index);
   void setNodeStartIndex(int32_t index);

   Array<Node*> _nodes;                // all nodes to avoid tree traversal
   std::vector<Material*> _materials;  // materials
   Camera* _camera = nullptr;          // camera (currently active)
   int32_t _animation_begin = 0;
   int32_t _animation_end = 0;
   int32_t _flags = 0;

   int32_t _material_start_index = 0;
   int32_t _node_start_index = 0;
   Matrix _global_transform;

   static SceneGraph* _instance;
};
