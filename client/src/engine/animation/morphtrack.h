// morph track
// animates a set of vertices

#pragma once

#include "morphkey.h"
#include "track.h"

class MorphTrack : public Track<MorphKey>
{
public:
   MorphTrack();

   void get(List<Vector>& vertices, List<Vector>& normals, float time);

   void calculateNormals(const Array<uint16_t>& indices);
};
