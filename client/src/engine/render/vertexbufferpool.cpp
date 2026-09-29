#include "vertexbufferpool.h"
#include "geometry.h"
#include "vertexbuffer.h"

bool VertexBufferPool::contains(Geometry* geometry) const
{
   return _pool.contains(geometry->getID());
}

VertexBuffer* VertexBufferPool::get(Geometry* geometry)
{
   const auto iterator = _pool.find(geometry->getID());
   if (iterator != _pool.end())
   {
      return iterator->second;
   }
   return nullptr;
}

VertexBuffer* VertexBufferPool::add(Geometry* geometry)
{
   auto* buffer = new VertexBuffer(geometry);
   _pool[geometry->getID()] = buffer;
   return buffer;
}
