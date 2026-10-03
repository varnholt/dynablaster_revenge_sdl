#include "packetstreambuffer.h"

#include <algorithm>

void PacketStreamBuffer::append(std::span<const char> data)
{
   _buffer.insert(_buffer.end(), data.begin(), data.end());
}

size_t PacketStreamBuffer::bytesAvailable() const
{
   return _buffer.size() - _pos;
}

BinaryReader PacketStreamBuffer::reader() const
{
   return BinaryReader(std::span(_buffer).subspan(_pos));
}

BinaryReader PacketStreamBuffer::reader(size_t bytes) const
{
   return BinaryReader(std::span(_buffer).subspan(_pos, std::min(bytes, _buffer.size() - _pos)));
}

void PacketStreamBuffer::consume(size_t bytes)
{
   _pos += bytes;
}

void PacketStreamBuffer::compact()
{
   if (_pos > 0)
   {
      _buffer.erase(_buffer.begin(), _buffer.begin() + static_cast<std::ptrdiff_t>(_pos));
      _pos = 0;
   }
}
