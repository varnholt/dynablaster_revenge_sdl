// scale track

#pragma once

#include "math/matrix.h"
#include "scalekey.h"
#include "track.h"

class ScaleTrack : public Track<ScaleKey>
{
public:
   ScaleTrack();

   Matrix get(float time);

   void add(int32_t time, const Scale& scale);

   void load(Stream* stream) override;
   void write(Stream* stream) override;
};
