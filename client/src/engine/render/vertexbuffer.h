#pragma once

#include <cstdint>

class Geometry;
class Matrix;
class Node;

class VertexBuffer
{
public:
   VertexBuffer(Geometry* geometry);
   ~VertexBuffer();

   VertexBuffer(const VertexBuffer&) = delete;
   VertexBuffer& operator=(const VertexBuffer&) = delete;

   void update(Node** node_list);

   Geometry* getGeometry() const;
   uint32_t getVertexBuffer() const;
   uint32_t getIndexBuffer() const;
   int32_t getIndexCount() const;
   void setIndexCount(int32_t count);
   void setIndexBuffer(uint16_t* indices, int32_t count);

private:
   Geometry* _geometry = nullptr;
   uint32_t _vertex_buffer = 0;
   uint32_t _index_buffer = 0;
   int32_t _index_count = 0;
};
