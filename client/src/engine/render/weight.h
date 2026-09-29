#pragma once

#include <cstdint>

class Stream;

class Weight
{
public:
   Weight() = default;
   Weight(int32_t id, float weight);

   void load(Stream* stream);
   void write(Stream* stream);
   void operator<<(Stream& stream);
   void operator>>(Stream& stream);

   float weight() const;
   int32_t id() const;

private:
   int32_t _id = -1;
   float _weight = 0.0f;
};
