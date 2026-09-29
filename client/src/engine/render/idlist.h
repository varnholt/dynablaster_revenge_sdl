#pragma once

#include "tools/array.h"
#include "tools/stream.h"

#include <cstdint>

class IDList : public Array<int32_t>
{
public:
   IDList() = default;
   IDList(const IDList&) = default;

   IDList& operator=(const IDList& other);

   virtual int32_t load(Stream* stream);
};
