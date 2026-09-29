#include "weight.h"
#include "tools/stream.h"

Weight::Weight(int32_t id, float weight) : _id(id), _weight(weight)
{
}

void Weight::operator<<(Stream& stream)
{
   load(&stream);
}

void Weight::operator>>(Stream& stream)
{
   write(&stream);
}

float Weight::weight() const
{
   return _weight;
}

int32_t Weight::id() const
{
   return _id;
}

void Weight::load(Stream* stream)
{
   _id = stream->getWord();
   _weight = stream->getFloat();
}

void Weight::write(Stream* stream)
{
   stream->writeWord(_id);
   stream->writeFloat(_weight);
}
