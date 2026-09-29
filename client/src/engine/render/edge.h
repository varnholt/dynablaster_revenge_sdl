#pragma once

#include <cstdint>

class Stream;

class Edge
{
public:
   void operator<<(Stream& stream);

   uint16_t i1, i2;
   uint16_t i3, i4;
   int32_t f1, f2;
   int32_t flags;
};
