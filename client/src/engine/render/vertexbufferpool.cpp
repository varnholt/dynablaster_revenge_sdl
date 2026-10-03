#include "vertexbufferpool.h"
#include "geometry.h"

bool VertexBufferPool::contains(const Geometry& geometry) const
{
   return _pool.contains(geometry.getID());
}

std::optional<std::reference_wrapper<VertexBuffer>> VertexBufferPool::get(const Geometry& geometry)
{
   const auto iterator = _pool.find(geometry.getID());
   if (iterator != _pool.end())
   {
      return *iterator->second;
   }
   return std::nullopt;
}

VertexBuffer& VertexBufferPool::add(const Geometry& geometry)
{
   auto& buffer = _pool[geometry.getID()];
   buffer = std::make_unique<VertexBuffer>();
   return *buffer;
}
