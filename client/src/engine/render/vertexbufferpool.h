#pragma once

#include <cstdint>
#include <unordered_map>

class Geometry;
class VertexBuffer;

class VertexBufferPool
{
public:
   bool contains(Geometry* geometry) const;
   VertexBuffer* add(Geometry* geometry);
   VertexBuffer* get(Geometry* geometry);

private:
   // entries intentionally never freed: that would add GL buffer deletes on Material teardown
   std::unordered_map<int32_t, VertexBuffer*> _pool;
};
