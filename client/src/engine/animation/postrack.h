// position track

#pragma once

#include "poskey.h"
#include "track.h"

class PosTrack : public Track<PosKey>
{
public:
   PosTrack();

   void add(int32_t time, const Vector& position);

   Vector get(float time);
};
