// visibility track

#pragma once

#include "key.h"
#include "track.h"

class VisTrack : public Track<KeyBase>
{
public:
   VisTrack();

   int32_t get(int32_t time);
};
