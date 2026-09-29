#include "uv.h"
#include "tools/stream.h"

UV::UV(float u, float v) : u(u), v(v)
{
}

void UV::operator<<(Stream& stream)
{
   load(&stream);
}

void UV::operator>>(Stream& stream)
{
   write(&stream);
}

void UV::load(Stream* stream)
{
   u = stream->getFloat();
   v = 1.0f - stream->getFloat();
   stream->getFloat();
}

void UV::write(Stream* stream)
{
   stream->writeFloat(u);
   stream->writeFloat(1.0f - v);
   stream->writeFloat(0.0f);
}
