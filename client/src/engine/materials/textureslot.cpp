#include "textureslot.h"
#include "tools/stream.h"

TextureSlot::TextureSlot(Stream& stream)
{
   load(stream);
}

void TextureSlot::load(Stream& stream)
{
   // amount and channel are not stored in the file
   _amount = 1.0f;
   _channel = 1;
   ObjectName::load(stream);
}

void TextureSlot::write(Stream& stream)
{
   ObjectName::write(stream);
}

float TextureSlot::amount() const
{
   return _amount;
}

int32_t TextureSlot::channel() const
{
   return _channel;
}
