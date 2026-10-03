#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "binaryreader.h"

// buffers raw bytes read from a non-blocking transport and hands out a BinaryReader positioned
// at the first unconsumed byte, so a length-prefix reassembly loop can attempt a read and then
// commit only the bytes it actually consumed via consume()
class PacketStreamBuffer
{
public:
   // append raw bytes just read from the transport
   void append(std::span<const char> data);

   // bytes not yet consumed from the stream
   [[nodiscard]] size_t bytesAvailable() const;

   // reader positioned at the first unconsumed byte
   [[nodiscard]] BinaryReader reader() const;

   // reader limited to the next 'bytes' bytes, e.g. one packet's block
   [[nodiscard]] BinaryReader reader(size_t bytes) const;

   // commit that 'bytes' bytes were consumed from the current read position
   void consume(size_t bytes);

   // drop already-consumed bytes so the buffer doesn't grow without bound; call once a
   // reassembly loop has drained everything it currently can
   void compact();

private:
   std::vector<uint8_t> _buffer;
   size_t _pos = 0;
};
