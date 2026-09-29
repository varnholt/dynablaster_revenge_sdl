#include "renderbuffer.h"
#include "geometry.h"
#include "math/vector.h"
#include "nodes/mesh.h"
#include "renderdevice.h"
#include "weight.h"

#include <cstring>

RenderBuffer::RenderBuffer(Geometry* geometry) : mNode(geometry->getParent()), mGeometry(geometry)
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
   if (mWeights)
   {
      auto* vram = static_cast<Vector*>(activeDevice->lockVertexBuffer(mVertex));
      std::memcpy(vram, mGeometry->getVertices(), mGeometry->getVertexCount() * sizeof(Vector));
      activeDevice->unlockVertexBuffer(mVertex);
   }
}

const Matrix& RenderBuffer::getTransform() const
{
   return mNode->getTransform();
}

int32_t RenderBuffer::getSize() const
{
   return mSize;
}

void RenderBuffer::setSize(int32_t num)
{
   mSize = num;
}

uint32_t RenderBuffer::getVertexBuffer() const
{
   return mVertex;
}

uint32_t RenderBuffer::getIndexBuffer() const
{
   return mIndex;
}

Geometry* RenderBuffer::getGeometry() const
{
   return mGeometry;
}

bool RenderBuffer::getVisible() const
{
   return mGeometry->isVisible();
}

void RenderBuffer::clear()
{
}
