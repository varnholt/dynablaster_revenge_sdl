// chunk class (scene-graph traversal helper-class)
// a chunk holds one node of the scene graph inside the .hjb file-format
// the chunk starts at the current stream position and contains at least an id to describe the node type (mesh,light,etc),
// name and size of the node.
// since the chunk-size and stream-position are known, a chunk can be skipped at any time, no matter how much data of the
// actual stream have already been read inside the chunk, guiding the stream to the start of the next chunk.

#pragma once

#include <cstdint>
#include <functional>
#include <sstream>
#include <string>

#include "objectname.h"
#include "stream.h"

// write mode: data is collected here and flushed to the parent stream (with id, name and size) on destruction;
// a base so it is constructed before the Stream base that writes into it
struct ChunkBuffer
{
   std::ostringstream _buffer;
};

class Chunk : private ChunkBuffer, public Stream, public ObjectName
{
public:
   enum class AccessMode
   {
      Read,
      Write,
   };

   explicit Chunk(Stream& stream);
   Chunk(Stream& stream, int32_t id, const std::string& name);
   ~Chunk();

   Chunk(const Chunk&) = delete;
   Chunk& operator=(const Chunk&) = delete;

   int32_t id() const;

   int32_t dataLeft() const;
   void skip();

private:
   std::reference_wrapper<Stream> _stream;
   AccessMode _mode = AccessMode::Read;
   int32_t _id = 0;
   int32_t _size = 0;
   int32_t _chunk_position = 0;
};
