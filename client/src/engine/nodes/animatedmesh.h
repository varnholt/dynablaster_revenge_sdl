#pragma once

#include <cstdint>
#include "mesh.h"

class AnimatedMesh : public Mesh
{
public:
   AnimatedMesh(Node* parent = nullptr);

   int32_t getFrameCount() const;
   Geometry* getFrame(int32_t frame);
};
