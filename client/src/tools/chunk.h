// chunk class (scene-graph traversal helper-class)
// a chunk holds one node of the scene graph inside the .hjb file-format
// the chunk starts at the current stream position and contains at least an id to describe the node type (mesh,light,etc),
// name and size of the node.
// since the chunk-size and stream-position are known, a chunk can be skipped at any time, no matter how much data of the
// actual stream have already been read inside the chunk, guiding the stream to the start of the next chunk.

#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "objectname.h"
#include "stream.h"

class Chunk : public Stream, public ObjectName
{
public:
   enum class AccessMode
   {
      Read,
      Write,
   };

   explicit Chunk(Stream& stream);
   Chunk(Stream& stream, int32_t id, const std::string& name);
   ~Chunk() override;

   Chunk(const Chunk&) = delete;
   Chunk& operator=(const Chunk&) = delete;

   void getData(std::span<std::byte> destination) override;
   void writeData(std::span<const std::byte> source) override;

   int32_t id() const;

   int32_t dataLeft() const;
   void skip();

private:
   static constexpr int32_t kBufferSize = 3111;
   using Buffer = std::array<std::byte, kBufferSize>;

   Stream& _stream;
   AccessMode _mode = AccessMode::Read;
   int32_t _id = 0;
   int32_t _size = 0;
   int32_t _chunk_position = 0;

   // write mode: data is collected here and flushed to "_stream" (with id, name and size) on destruction
   std::vector<std::unique_ptr<Buffer>> _buffers;
   bool _buffer_open = false;  // last buffer still has free space
   int32_t _buffer_position = 0;
};
