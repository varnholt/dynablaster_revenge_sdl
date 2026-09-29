#include "vertexbuffer.h"
#include "../nodes/node.h"
#include "geometry.h"
#include "renderdevice.h"

#include <algorithm>

VertexBuffer::VertexBuffer(Geometry* geometry)
    : _geometry(geometry), _vertex_buffer(activeDevice->createBuffer()), _index_buffer(activeDevice->createBuffer())
{
}

VertexBuffer::~VertexBuffer()
{
   activeDevice->deleteBuffer(_vertex_buffer);
   activeDevice->deleteBuffer(_index_buffer);
}

Geometry* VertexBuffer::getGeometry() const
{
   return _geometry;
}

void VertexBuffer::setIndexBuffer(uint16_t* indices, int32_t count)
{
   activeDevice->allocateIndexBuffer(_index_buffer, count * sizeof(uint16_t));
   auto* destination = static_cast<uint16_t*>(activeDevice->lockIndexBuffer(_index_buffer));
   std::copy_n(indices, count, destination);
   activeDevice->unlockIndexBuffer(_index_buffer);
   _index_count = count;
}

void VertexBuffer::update(Node** /*node_list*/)
{
}

uint32_t VertexBuffer::getVertexBuffer() const
{
   return _vertex_buffer;
}

uint32_t VertexBuffer::getIndexBuffer() const
{
   return _index_buffer;
}

int32_t VertexBuffer::getIndexCount() const
{
   return _index_count;
}

void VertexBuffer::setIndexCount(int32_t count)
{
   _index_count = count;
}
