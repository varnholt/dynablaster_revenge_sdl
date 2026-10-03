// morph track
// animates a set of vertices

#pragma once

#include "morphkey.h"
#include "track.h"

#include <cstdint>
#include <vector>

class MorphTrack : public Track<MorphKey>
{
public:
   MorphTrack();

   void get(std::vector<Vector>& vertices, std::vector<Vector>& normals, float time);

   void calculateNormals(const std::vector<uint16_t>& indices);
};
