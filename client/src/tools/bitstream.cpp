#include "bitstream.h"

BitStream::BitStream(const void* data, int32_t size) : _data(static_cast<const uint8_t*>(data)), _size(size)
{
}

uint32_t BitStream::getBits(int32_t count)
{
   uint32_t value = 0;
   while (count > 0)
   {
      cache();

      if (count >= _available)
      {
         value = (value << _available) | (_bits >> (8 - _available));
         count -= _available;
         _available = 0;
         _bits = 0;
      }
      else
      {
         value = (value << count) | (_bits >> (8 - count));
         _bits <<= count;
         _available -= count;
         count = 0;
      }
   }

   return value;
}

void BitStream::skipBits(int32_t count)
{
   getBits(count);
}

void BitStream::cache()
{
   if (!_available)
   {
      _bits = _data[_position];
      _position++;
      _available = 8;
   }
}
