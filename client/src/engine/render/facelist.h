// holds an array of vertex-indices (3 per polygon)
// actually this is just an array<uint16_t>, but since integers can't load themselves,
// we use this simple helper class here

#pragma once

#include "tools/array.h"
#include "tools/streamable.h"

#include <cstdint>

class FaceList : public Array<uint16_t>, public Streamable
{
public:
   void load(Stream* stream) override;
   void write(Stream* stream) override;
};
