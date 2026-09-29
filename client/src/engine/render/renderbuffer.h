#pragma once

#include <cstdint>

class Node;
class Vector;
class Geometry;
class Weight;
class Matrix;

class RenderBuffer
{
public:
   RenderBuffer(Geometry* geometry);
   virtual ~RenderBuffer() = default;

   void update(Node** node_list);
   const Matrix& getTransform() const;  // transformation matrix of mesh
   int32_t getSize() const;             // number of indices in index buffer (triangles*3)
   uint32_t getVertexBuffer() const;    // vertex buffer handle
   uint32_t getIndexBuffer() const;     // index buffer handle
   Geometry* getGeometry() const;
   void setSize(int32_t num);
   bool getVisible() const;

   virtual void clear();

protected:
   // member names kept: accessed directly by the RenderBuffer subclasses in engine/materials
   Node* _node = nullptr;
   Geometry* _geometry = nullptr;
   uint32_t _format = 0;
   int32_t _size = 0;
   int32_t _vertex_count = 0;
   Vector* _original_vertices = nullptr;
   Weight* _weights = nullptr;

   // vertex/index buffer handles
   uint32_t _vertex = 0;
   uint32_t _index = 0;

   uint32_t createVertexBuffer(void* data, int32_t size, bool dynamic = false);
   uint32_t createIndexBuffer(void* data, int32_t size, bool dynamic = false);
   void deleteBuffer(uint32_t buffer);
};
