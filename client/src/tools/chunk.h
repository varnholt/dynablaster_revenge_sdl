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
#include <vector>

#include "objectname.h"
#include "stream.h"
#include "string.h"

class Chunk : public Stream, public ObjectName
{
public:
   enum class AccessMode
   {
      Read,
      Write,
   };

   Chunk(Stream* stream);
   Chunk(Stream* stream, int32_t id, const String& name);
   ~Chunk() override;

   void getData(void* destination, int32_t size) override;
   void writeData(void* source, int32_t size) override;

   int32_t id() const;

   int32_t dataLeft() const;
   void skip();

private:
   static constexpr int32_t kBufferSize = 3111;
   using Buffer = std::array<char, kBufferSize>;

   Stream* _stream = nullptr;  // not owned
   AccessMode _mode = AccessMode::Read;
   int32_t _id = 0;
   int32_t _size = 0;
   int32_t _chunk_position = 0;

   // write mode: data is collected here and flushed to "_stream" (with id, name and size) on destruction
   std::vector<std::unique_ptr<Buffer>> _buffers;
   Buffer* _buffer = nullptr;  // buffer currently being filled, null when full
   int32_t _buffer_position = 0;
};
