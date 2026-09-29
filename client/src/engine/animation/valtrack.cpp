#include "valtrack.h"

ValTrack::ValTrack() : Track<ValKey>(Track::idValue)
{
}

void ValTrack::add(int32_t time, float value)
{
   addKey(ValKey(time, value));
}

float ValTrack::get(float time)
{
   const int32_t last = size() - 1;
   if (last < 0)
   {
      return 0;
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

   const float a = prevKey().value();
   const float b = nextKey().value();
   return a + (b - a) * f;
}
