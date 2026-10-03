#pragma once

#include <cstdint>
#include "mesh.h"

class AnimatedMesh : public Mesh
{
public:
   AnimatedMesh();

   int32_t getFrameCount() const;
   Geometry& getFrame(int32_t frame);
};
