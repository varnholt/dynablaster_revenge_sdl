#include "rottrack.h"

RotTrack::RotTrack() : Track<RotKey>(Track::idRotation)
{
}

void RotTrack::add(int32_t time, const Quat& rotation)
{
   addKey(RotKey(time, rotation));
}

void RotTrack::load(Stream* stream)
{
   Track<RotKey>::load(stream);

   // keys are stored as deltas: accumulate them
   for (int32_t i = 1; i < size(); i++)
   {
      const Quat q1 = key(i - 1).value();
      const Quat q2 = key(i).value();
      Quat q = q1 * q2;
      if ((q1 % q) < 0)
      {
         q = -q;
      }
      key(i).setValue(q);
   }
}

void RotTrack::write(Stream* stream)
{
   // TODO: write keys as deltas again
   Track<RotKey>::write(stream);
}

Quat RotTrack::get(float time)
{
   const int32_t last = size() - 1;

   if (last < 0)
   {
      return Quat(1, 0, 0, 0);
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

   const Quat q1 = prevKey().value();
   const Quat q2 = nextKey().value();

   return Quat::slerp(q1, q2, f);
}
