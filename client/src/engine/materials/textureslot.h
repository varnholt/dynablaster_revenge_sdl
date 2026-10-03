#pragma once

#include <cstdint>
#include "tools/objectname.h"

class Stream;

class TextureSlot : public ObjectName
{
public:
   TextureSlot() = default;
   TextureSlot(Stream& stream);

   void load(Stream& stream);
   void write(Stream& stream);

   float amount() const;
   int32_t channel() const;

private:
   float _amount = 0.0f;
   int32_t _channel = 0;
};
