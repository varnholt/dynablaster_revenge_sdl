// rotation track

#pragma once

#include "rotkey.h"
#include "track.h"

class RotTrack : public Track<RotKey>
{
public:
   RotTrack();

   Quat get(float time);

   void add(int32_t time, const Quat& rotation);

   void load(Stream& stream) override;
   void write(Stream& stream) override;
};
