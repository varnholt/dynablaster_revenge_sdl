#pragma once

#include <cstdint>
#include <span>

class VertexBuffer
{
public:
   VertexBuffer();
   ~VertexBuffer();

   VertexBuffer(const VertexBuffer&) = delete;
   VertexBuffer& operator=(const VertexBuffer&) = delete;

   uint32_t getVertexBuffer() const;
   uint32_t getIndexBuffer() const;
   int32_t getIndexCount() const;
   void setIndexCount(int32_t count);
   void setIndexBuffer(std::span<const uint16_t> indices);

private:
   uint32_t _vertex_buffer = 0;
   uint32_t _index_buffer = 0;
   int32_t _index_count = 0;
};
