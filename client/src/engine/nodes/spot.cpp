#include "spot.h"
#include "tools/stream.h"

Spot::Spot() : Light(Node::idSpot)
{
}

void Spot::load(Stream& stream)
{
   _shape = stream.getInt();
   _hotspot = stream.getFloat();
   _fall_size = stream.getFloat();

   Light::load(stream);
}

void Spot::write(Stream& stream)
{
   stream.writeInt(_shape);
   stream.writeFloat(_hotspot);
   stream.writeFloat(_fall_size);

   Light::write(stream);
}
