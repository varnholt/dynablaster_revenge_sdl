#include "renderbuffer.h"
#include "geometry.h"
#include "math/vector.h"
#include "nodes/mesh.h"
#include "renderdevice.h"
#include "weight.h"

#include <cstring>

RenderBuffer::RenderBuffer(Geometry* geometry) : _node(geometry->getParent()), _geometry(geometry)
{
}

uint32_t RenderBuffer::createVertexBuffer(void* data, int32_t size, bool dynamic)
{
   const uint32_t buffer = activeDevice->createVertexBuffer(size, dynamic);
   void* destination = activeDevice->lockVertexBuffer(buffer);
   if (data)
   {
      std::memcpy(destination, data, size);
   }
   else
   {
      std::memset(destination, 0, size);
   }
   activeDevice->unlockVertexBuffer(buffer);
   return buffer;
}

uint32_t RenderBuffer::createIndexBuffer(void* data, int32_t size, bool dynamic)
{
   const uint32_t buffer = activeDevice->createIndexBuffer(size, dynamic);
   void* destination = activeDevice->lockIndexBuffer(buffer);
   if (data)
   {
      std::memcpy(destination, data, size);
   }
   else
   {
      std::memset(destination, 0, size);
   }
   activeDevice->unlockIndexBuffer(buffer);
   return buffer;
}

void RenderBuffer::deleteBuffer(uint32_t buffer)
{
   activeDevice->deleteBuffer(buffer);
}

void RenderBuffer::update(Node**)
{
   if (_weights)
   {
      auto* vram = static_cast<Vector*>(activeDevice->lockVertexBuffer(_vertex));
      std::memcpy(vram, _geometry->getVertices(), _geometry->getVertexCount() * sizeof(Vector));
      activeDevice->unlockVertexBuffer(_vertex);
   }
}

const Matrix& RenderBuffer::getTransform() const
{
   return _node->getTransform();
}

int32_t RenderBuffer::getSize() const
{
   return _size;
}

void RenderBuffer::setSize(int32_t num)
{
   _size = num;
}

uint32_t RenderBuffer::getVertexBuffer() const
{
   return _vertex;
}

uint32_t RenderBuffer::getIndexBuffer() const
{
   return _index;
}

Geometry* RenderBuffer::getGeometry() const
{
   return _geometry;
}

bool RenderBuffer::getVisible() const
{
   return _geometry->isVisible();
}

void RenderBuffer::clear()
{
}
