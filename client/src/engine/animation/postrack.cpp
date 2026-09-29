#include "postrack.h"

PosTrack::PosTrack() : Track<PosKey>(Track::idPosition)
{
}

void PosTrack::add(int32_t time, const Vector& position)
{
   addKey(PosKey(time, position));
}

Vector PosTrack::get(float time)
{
   const int32_t last = size() - 1;

   if (last < 0)
   {
      return Vector(0, 0, 0);
   }

   if (time <= key(0).time())
   {
      return key(0).value();
   }
   if (time >= key(last).time())
   {
      return key(last).value();
   }

   const float f = interpolate(time);

   const Vector v1 = prevKey().value();
   const Vector v2 = nextKey().value();

   return v1 + (v2 - v1) * f;
}
