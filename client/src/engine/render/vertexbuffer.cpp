#include "vertexbuffer.h"
#include "renderdevice.h"

#include <algorithm>

VertexBuffer::VertexBuffer() : _vertex_buffer(activeDevice().createBuffer()), _index_buffer(activeDevice().createBuffer())
{
}

VertexBuffer::~VertexBuffer()
{
   activeDevice().deleteBuffer(_vertex_buffer);
   activeDevice().deleteBuffer(_index_buffer);
}

void VertexBuffer::setIndexBuffer(std::span<const uint16_t> indices)
{
   const auto count = static_cast<int32_t>(indices.size());
   activeDevice().allocateIndexBuffer(_index_buffer, count * sizeof(uint16_t));
   std::ranges::copy(indices, activeDevice().lockIndexBuffer<uint16_t>(_index_buffer).begin());
   activeDevice().unlockIndexBuffer(_index_buffer);
   _index_count = count;
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
