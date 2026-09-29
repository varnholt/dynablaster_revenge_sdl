#include "packetstreambuffer.h"

#include <cstring>

void PacketStreamBuffer::append(const char* data, size_t length)
{
   const auto offset = _buffer.size();
   _buffer.resize(offset + length);
   std::memcpy(_buffer.data() + offset, data, length);
}

size_t PacketStreamBuffer::bytesAvailable() const
{
   return _buffer.size() - _pos;
}

BinaryReader PacketStreamBuffer::reader() const
{
   return BinaryReader(_buffer.data() + _pos, _buffer.size() - _pos);
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
