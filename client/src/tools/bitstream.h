#pragma once

#include <cstdint>

class BitStream
{
public:
   BitStream() = default;
   BitStream(const void* data, int32_t size);

   uint32_t getBits(int32_t count);
   void skipBits(int32_t count);
   void cache();

private:
   const uint8_t* _data = nullptr;
   int32_t _size = 0;
   int32_t _position = 0;
   uint8_t _bits = 0;
   int32_t _available = 0;
};
