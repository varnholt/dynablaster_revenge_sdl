#include "memorystream.h"

#include <cstring>

void MemoryStream::close()
{
   _buffer = nullptr;
   _position = 0;
   _size = 0;
}

int32_t MemoryStream::open(void* data, int32_t size)
{
   _buffer = static_cast<uint8_t*>(data);
   _size = size;
   _position = 0;

   return 1;
}

int32_t MemoryStream::size() const
{
   return _size;
}

int32_t MemoryStream::pos() const
{
   return _position;
}

void MemoryStream::getData(void* destination, int32_t size)
{
   const int32_t available = _size - _position;
   if (size > available)
   {
      size = available;
   }
   std::memcpy(destination, _buffer + _position, size);
   _position += size;
}

void MemoryStream::writeData(void* source, int32_t size)
{
   std::memcpy(_buffer + _position, source, size);
   _position += size;
}

void MemoryStream::skip(int32_t size)
{
   _position += size;
}
